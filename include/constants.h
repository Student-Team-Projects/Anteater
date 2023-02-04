#pragma once

// BPF constants
#define MAX_WRITE_SIZE 128 * 1024
#define EVENT_RING_BUFFER_SIZE 128 * 1024 * 1024
#define PROC_COUNT_MAX 1 << 22
#define MAX_CONCURRENT_WRITES 1024
#define DEFAULT_QUEUE_CAPACITY 32 * 1024 * 1024

#ifndef TASK_COMM_LEN
#define TASK_COMM_LEN 16
#endif

// Sysdig constants
#ifndef CHISEL
#define CHISEL "" // This is defined in Makefile
#endif

#define SYSDIG "sysdig"

// SPDLOG constants
#ifndef LOGSDIR
#define LOGSDIR "" // This is defined in Makefile
#endif

// Other constants
#define SUDO "sudo"
