#include "bpf_provider.hpp"

#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include <iostream>
#include <stdexcept>

#include "backend/event.h"

void static_init() {
  static bool called = false;
  if (called) return;

  if (geteuid())
    throw std::runtime_error{"This program should be run with sudo privileges"};

  rlimit lim{
      .rlim_cur = RLIM_INFINITY,
      .rlim_max = RLIM_INFINITY,
  };

  if (setrlimit(RLIMIT_MEMLOCK, &lim))
    throw std::runtime_error{"Failed to increase RLIMIT_MEMLOCK"};

  called = true;
}

bpf_provider::bpf_provider() {
  static_init();

  skel = tracer::open_and_load();
  tracer::attach(skel);
  buffer = ring_buffer__new(bpf_map__fd(skel->maps.queue), buf_process_sample,
                            this, nullptr);
};

bpf_provider::~bpf_provider() {
  tracer::detach(skel);
  tracer::destroy(skel);
};

bool bpf_provider::is_active() {
  return !tracked_processes.empty() || !messages.empty();
}

std::optional<events::event> bpf_provider::provide() {
  if (messages.empty()) ring_buffer__poll(buffer, 100);
  if (messages.empty()) return {};
  auto result = messages.front();
  messages.pop();
  return {std::move(result)};
}

void bpf_provider::run(char *argv[]) {
  pid_t child = fork();
  int value = 0;

  if (child == 0) {
    pid_t pid = getpid();
    bpf_map__update_elem(skel->maps.processes, &pid, sizeof(pid), &value,
                         sizeof(value), BPF_ANY);
    execvp(argv[0], argv);
    throw std::runtime_error{"execvp() failed"};
  } else {
    tracked_processes.insert(child);
  }
}

/**
 * The timestamps returned in events are relative to system boot time.
 * To return meaningful timestamps we have to get boot time from kernel, which
 * is done here.
 */
std::chrono::system_clock::time_point get_boot_time() {
  struct stat kernel_proc_entry;
  int err = stat("/proc/1", &kernel_proc_entry);
  if (err) throw std::runtime_error("Failed to fetch last boot time");

  using namespace std::chrono;
  auto seconds_part = std::chrono::seconds{kernel_proc_entry.st_ctim.tv_sec};
  auto nanoseconds_part =
      std::chrono::nanoseconds{kernel_proc_entry.st_ctim.tv_nsec};

  return system_clock::time_point(seconds_part + nanoseconds_part);
}

std::chrono::system_clock::time_point into_timestamp(uint64_t event_timestamp) {
  using namespace std::chrono;
  static auto boot_time = get_boot_time();
  auto duration_to_event = std::chrono::nanoseconds{event_timestamp};
  return boot_time + duration_to_event;
}

static events::write_event from(const backend::write_event *e) {
  return {
      e->proc,
      into_timestamp(e->timestamp),
      e->fd,
      {e->data, static_cast<size_t>(e->size)},
  };
}

static events::fork_event from(const backend::fork_event *e) {
  return {
      e->parent,
      into_timestamp(e->timestamp),
      e->child,
  };
}

static events::exit_event from(const backend::exit_event *e) {
  return {e->proc, into_timestamp(e->timestamp), e->code};
}

static events::exec_event from(const backend::exec_event *e) {
  return {
      e->proc,
      into_timestamp(e->timestamp),
      0,
      {e->args, static_cast<size_t>(e->args_size)},
  };
}

int bpf_provider::buf_process_sample(void *ctx, void *data, size_t len) {
  bpf_provider *me = static_cast<bpf_provider *>(ctx);
  const backend::event *e = static_cast<backend::event *>(data);
  switch (e->type) {
    case backend::FORK:
      me->tracked_processes.insert(e->fork.child);
      me->messages.push(from(&(e->fork)));
      break;
    case backend::EXIT:
      me->tracked_processes.erase(e->exit.proc);
      me->messages.push(from(&(e->exit)));
      break;
    case backend::EXEC:
      me->messages.push(from(&(e->exec)));
      break;
    case backend::WRITE:
      me->messages.push(from(&(e->write)));
      break;
  }
  return 0;
}
