#pragma once

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
