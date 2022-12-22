#pragma once

#include "log_consumer.h"

class Runner {
    LogConsumer consumer;

public:
    int run(char **args);
};