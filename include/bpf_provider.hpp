#pragma once

#include <optional>
#include <queue>
#include <set>

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
  static int buf_process_sample(void *ctx, void *data, size_t len);
  tracer *skel;
  ring_buffer *buffer;
  std::queue<events::event> messages;
  std::set<pid_t> tracked_processes;
};
