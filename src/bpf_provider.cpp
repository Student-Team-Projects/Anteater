#include "bpf_provider.hpp"

#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <pwd.h>

#include <iostream>
#include <thread>
#include <stdexcept>

#include <boost/lockfree/spsc_queue.hpp>

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


bpf_provider::bpf_provider() : interthread_queue{2048} {
  static_init();

  skel = tracer::open_and_load();
  tracer::attach(skel);
  buffer = ring_buffer__new(bpf_map__fd(skel->maps.queue), buf_process_sample,
                            this, nullptr);
};

void bpf_provider::main_loop() {
  // give thread highest priority.
  // this ensures that buffers are emptied as frequently as possible.
  // probably also breaks posix
  setpriority(PRIO_PROCESS, gettid(), -20);

  while(!tracked_processes.empty() || !messages.empty()) {
    while (messages.empty() || interthread_queue.write_available() == 0) 
      ring_buffer__poll(buffer, 100);
    while(!messages.empty() && interthread_queue.write_available() > 0) {
      auto message = messages.front();
      messages.pop();
      interthread_queue.push(std::move(message));
    }
  }
  active = false;
  tracer::detach(skel);
  tracer::destroy(skel);
}


bpf_provider::~bpf_provider() {
  receiver_thread.join();
};

bool bpf_provider::is_active() {
  return active || interthread_queue.read_available() > 0;
}

std::optional<events::event> bpf_provider::provide() {
  events::event result;
  if(interthread_queue.pop(result))
    return {std::move(result)};
  else
    return {};
}

static void fix_user() {
  setuid(getuid());
}

void bpf_provider::run(char *argv[]) {
  int stdout_pipe[2];
  int stderr_pipe[2];
  if(pipe(stdout_pipe))
    throw std::runtime_error{"cannot redirect stdout"};
  
  pid_t stdout_writer = fork();
  if(stdout_writer == 0) {
    fix_user();
    close(stdout_pipe[1]);
    dup2(stdout_pipe[0], STDIN_FILENO);
    execl("/bin/cat", "cat", NULL);
    exit(-1);
  }

  if(pipe(stderr_pipe))
    throw std::runtime_error{"cannot redirect stderr"};
  pid_t stderr_writer = fork();
  if(stderr_writer == 0) {
    fix_user();
    close(stderr_pipe[1]);
    dup2(stderr_pipe[0], STDIN_FILENO);
    execl("/bin/cat", "cat", NULL);
    std::cout << errno << std::endl;
    exit(-1);
  }

  close(stdout_pipe[0]);
  close(stderr_pipe[0]);
  dup2(stdout_pipe[1], STDOUT_FILENO);
  dup2(stderr_pipe[1], STDERR_FILENO);

  pid_t child = fork();
  int value = 0;

  if (child == 0) {
    pid_t pid = getpid();
    bpf_map__update_elem(skel->maps.processes, &pid, sizeof(pid), &value,
                         sizeof(value), BPF_ANY);
    fix_user();
    execvp(argv[0], argv);
    throw std::runtime_error{"execvp() failed"};
  } else {
    tracked_processes.insert(child);
  }

  active = true;
  receiver_thread = std::thread {&bpf_provider::main_loop, this};
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

  auto seconds_part = std::chrono::seconds{kernel_proc_entry.st_ctim.tv_sec};
  auto nanoseconds_part =
      std::chrono::nanoseconds{kernel_proc_entry.st_ctim.tv_nsec};

  return std::chrono::system_clock::time_point(seconds_part + nanoseconds_part);
}

std::chrono::system_clock::time_point into_timestamp(uint64_t event_timestamp) {
  static auto boot_time = get_boot_time();
  auto duration_to_event = std::chrono::nanoseconds{event_timestamp};
  return boot_time + duration_to_event;
}

static events::write_event from(const backend::write_event *e) {
  events::write_event::descriptor fd = e->fd == backend::STDOUT ? 
    events::write_event::descriptor::STDOUT :
    events::write_event::descriptor::STDERR;

  return {
    e->proc,
    into_timestamp(e->timestamp),
    fd,
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
  std::string command{e->data, e->data + e->args_size};
  std::replace(command.begin(), command.end(), '\0', ' ');
  std::string working_directory{e->data + e->args_size, e->data + e->args_size + e->working_directory_size};
  if(working_directory == "") working_directory = "/";
  struct passwd *pws = getpwuid(e->uid);
  std::string user_name{pws->pw_name};

  return {
    {
      .source_pid = e->proc,
      .timestamp = into_timestamp(e->timestamp),
    },
    .user_id = 0,
    .user_name = user_name,
    .working_directory = working_directory,
    .command = command,
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
