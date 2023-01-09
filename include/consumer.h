#pragma once

#include "provider.h"

class Consumer {
    pid_t root_pid;
    volatile bool exiting = false;
  public:
    Consumer(pid_t root_pid);

    void consume(Provider &provider);
    void start(Provider &provider) { while (!exiting) consume(provider); };
    void stop() { exiting = true; }
};