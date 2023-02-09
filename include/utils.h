#pragma once 

#include <string>

// Converts hex string (e.g. "\xa4\xbc") to normal string in english alphabet
std::string hex_to_string(const char *hex_string, int len);
void convert_spaces(char *buf, size_t len);