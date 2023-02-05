#pragma once

#include "provider.h"

#include <iostream>

class Consumer {
    pid_t root_pid;
    volatile bool exiting = false;
    bool hex_input;

  public:
    Consumer(pid_t root_pid);

    void consume(Provider &provider);
    void start(Provider &provider, bool hex_input) { 
      this->hex_input = hex_input;
      while (!exiting) consume(provider); 
    };
    void stop();
};
