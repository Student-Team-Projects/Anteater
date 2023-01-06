#pragma once

#define MAX_WRITE_SIZE 128 * 1024
#define EVENT_RING_BUFFER_SIZE 128 * 1024 * 1024
#define PROC_COUNT_MAX 1 << 22
#define MAX_CONCURRENT_WRITES 1024

enum event_type {
	FORK,
	EXIT,
	WRITE
};

struct event {
	enum event_type event_type;
	pid_t pid;
	union {
		pid_t ppid;
		unsigned exit_code;
		void *buf;
	};
};
