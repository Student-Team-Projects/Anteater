#pragma once

#include "consumer.h"
#include "provider.h"

class PlainConsumer : public Consumer {
    pid_t root_pid;
    volatile bool exiting = false;
    bool hex_input;

public:
    PlainConsumer() = default;
    int start(Provider &provider, pid_t root_pid, bool hex_input) override;
    void stop() override;

private: 
    int consume(Provider &provider);
};
