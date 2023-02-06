#pragma once

#include "event.h"
#include "constants.h"

// percpu auxiliary maps to copy write buffers
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __type(key, u32);
    __type(value, struct write_event);
} auxmaps __weak SEC(".maps");

// process tree with root set by userspace
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __type(key, pid_t);
    __type(value, pid_t);
    __uint(max_entries, PROC_COUNT_MAX);
} pid_tree __weak SEC(".maps");

// buffer for all events sent to userspace
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, EVENT_RING_BUFFER_SIZE);
} rb __weak SEC(".maps");

// saved write params
struct write_params {
    enum Stream fd;
    const char *buf;
};

// writes in progress
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __type(key, u64);
    __type(value, struct write_params);
    __uint(max_entries, MAX_CONCURRENT_WRITES);
} writes __weak SEC(".maps");
