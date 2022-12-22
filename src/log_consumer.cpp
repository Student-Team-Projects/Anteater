#include "log_consumer.h"

#include <iostream>
#include <string>
#include <cstdio>

#include "msg_handler.h"

void LogConsumer::consume(std::string_view line) {
    printf(line.data());
}
