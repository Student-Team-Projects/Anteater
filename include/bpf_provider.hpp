#pragma once

#include <optional>
#include <queue>
#include <set>
#include <thread>
#include <atomic>

#include <boost/lockfree/spsc_queue.hpp>

#include "events.hpp"
#include "tracer.skel.h"

class bpf_provider {
 public:
  bpf_provider();
  ~bpf_provider();
  void run(char *argv[]);
  bool is_active();
  std::optional<events::event> provide();

 private:
  void main_loop();
  static int buf_process_sample(void *ctx, void *data, size_t len);
  tracer *skel;
  ring_buffer *buffer;
  std::thread receiver_thread;
  std::queue<events::event> messages;
  std::atomic<bool> active;
  boost::lockfree::spsc_queue<events::event> interthread_queue;
  std::set<pid_t> tracked_processes;
};
