#pragma once

#include <functional>
#include <memory>

#include "event.h"

struct delete_free {
    void operator()(void *x) { free(x); };
};

class Provider {
  public:
    typedef std::unique_ptr<Event, delete_free> event_ref;
    /* Consumes a pointer to the oldest unconsumed value and
     * lets the provider free the pointer from the previous call.
     * Blocks if there is no value to be consumed. */
    virtual event_ref provide() = 0;
    // start listening to events
    virtual int start() = 0;
    // end provider execution
    virtual void stop() = 0;

    virtual ~Provider() {} 
};
