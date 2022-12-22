#include "log_consumer.h"

#include <iostream>
#include <string>
#include <cstdio>

void LogConsumer::consume(std::string_view line) {
    printf(line.data());
}
