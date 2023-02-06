#pragma once

#include "constants.h"

enum Stream {
    STDOUT,
    STDERR
};

enum EventType {
    WRITE,
    EXEC,
    EXIT,
    FORK
};

struct Event {
    enum EventType event_type;
    pid_t pid;
    uint64_t timestamp;

    union {
        struct { // write
            enum Stream stream;
            size_t length;
            char data[];
        } write;
        struct { // fork
            pid_t child_pid;
        } fork;
        struct { // exec
            size_t length;
            char data[];
        } exec;
        struct { // exit
            int code;
        } exit;
    };
};

// has to be castable to Event
struct write_event {
    enum EventType event_type;
    pid_t pid;
    uint64_t timestamp;

    enum Stream stream;
    size_t length;
    char data[MAX_WRITE_SIZE];
};
