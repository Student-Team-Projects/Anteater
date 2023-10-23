#include "bpf_provider.hpp"

#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include <iostream>

#include "backend/event.h"

bpf_provider::bpf_provider() {
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
    exit(-1);
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

static events::write_event from(backend::write_event *e) {
  return {
    {
      .source_pid = e->proc,
      .timestamp = into_timestamp(e->timestamp),
    },
    .file_descriptor = e->fd,
    .data = {e->data, static_cast<size_t>(e->size)},
  };
}

static events::fork_event from(backend::fork_event *e) {
  return {
    {
      .source_pid = e->parent,
      .timestamp = into_timestamp(e->timestamp),
    },
    .child_pid = e->child,
  };
}

static events::exit_event from(backend::exit_event *e) {
  return {
    {
      .source_pid = e->proc,
      .timestamp = into_timestamp(e->timestamp),
    },
    .exit_code = e->code,
  };
}

static events::exec_event from(backend::exec_event *e) {
  std::cout << e->args_size << std::endl;
  return {
    {
      .source_pid = e->proc,
      .timestamp = into_timestamp(e->timestamp),
    },
    .user_id = 0,
    .command = {e->args, static_cast<size_t>(e->args_size)},
  };
}


int bpf_provider::buf_process_sample(void *ctx, void *data, size_t len) {
  bpf_provider *me = static_cast<bpf_provider *>(ctx);
  backend::event *e = static_cast<backend::event *>(data);
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
