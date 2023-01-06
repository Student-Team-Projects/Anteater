#pragma once

#include <unistd.h>

// These are handled this way because function pointers in C cannot handle state
extern volatile bool exiting;
extern pid_t root_pid;