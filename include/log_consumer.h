#pragma once

#include <string>

class LogConsumer {
public:
    void consume(std::string_view line);
};