#pragma once

#include <unistd.h>
#include <bpf/libbpf.h>

// generated header during make
#include "tracer.skel.h"

class TracerRunner {
    ring_buffer *rb = nullptr;
    tracer_bpf *skel = nullptr;

  public:
    TracerRunner(bool verbose = false);
    // Start tracing pid root_pid
    int listen();
    int init();
    
  private:
    int cleanup(int err);
};