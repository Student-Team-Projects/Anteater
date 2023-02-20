#pragma once

#include <iostream>

#include "event.h"
#include "provider.h"

class Consumer {
public:
    /* Starts consuming events provided by provider */
    virtual int start(Provider &provider, pid_t root_pid, bool hex_input) = 0;
    /* Forces gentle stop of consuming */
    virtual void stop() = 0;

    virtual ~Consumer() {}
};