#pragma once

#include <queue>
#include <bpf/libbpf.h>

#include "constants.h"
#include "event.h"
#include "provider.h"
#include "synchronized_queue.h"

// header generated during compilation
#include "tracer.skel.h"

class BPFProvider : public Provider {
    synchronized_queue<event_ref> refs;

    ring_buffer *rb = nullptr;
    tracer_bpf *skel = nullptr;
    
    pid_t root_pid;
    volatile bool exiting = false;

  public:
    BPFProvider(pid_t root_pid, size_t capacity = DEFAULT_QUEUE_CAPACITY, bool verbose = false);
    
    event_ref provide() override;
    int start() override;

    inline pid_t get_root() const { return root_pid; };
    void stop() override { exiting = true; }
    void alloc_and_push(const Event *pt, size_t sz);

  private:
    int cleanup(int err);
    int init();
    int listen();
};