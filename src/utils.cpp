#include "utils.h"

#include <string>
#include <iostream>
#include <cstdio>

std::string hex_to_string(const char *hex_string, int len) {
    std::string result = "";
    for (int i = 2; i < len; i += 4)
        result += (char)(stoul(std::string{hex_string[i], hex_string[i+1]}, 0, 16));
    return result;
}

void convert_spaces(char *buf, size_t len) {
    for (int i = 0; i < len; ++i)
        if (buf[i] == '\0')
            buf[i] = ' ';
}
