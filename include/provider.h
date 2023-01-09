#pragma once

#include <memory>
#include <functional>

#include "event.h"

struct delete_free {
    void operator()(void *x) { free(x); };
};

typedef std::unique_ptr<event, delete_free> unique_malloc_ptr;

class Provider {
  public:
    struct event_ref {
        unique_malloc_ptr ptr;
        size_t sz;
    };
    /** Consumes a pointer to the oldest unconsumed value and
    lets the provider free the pointer from the previous call.
    Blocks if there is no value to be consumed. */
    virtual event_ref provide() = 0;
    // start listening to events
    virtual int start() = 0;
    // end provider execution
    virtual void stop() = 0;

    virtual ~Provider() {} 
};
