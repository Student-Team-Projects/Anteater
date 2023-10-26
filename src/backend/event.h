#pragma once 

#ifdef __cplusplus
namespace backend {
#endif

enum event_type {
    FORK,
    EXIT,
    EXEC,
    WRITE
};

struct fork_event {
    enum event_type type;
    unsigned long long timestamp;
    pid_t parent;
    pid_t child;
};

struct exec_event {
    enum event_type type;
    unsigned long long timestamp;
    pid_t proc;
    int args_size;
    char args[];
};

struct exit_event {
    enum event_type type;
    unsigned long long timestamp;
    pid_t proc;
    int code;
};

struct write_event {
    enum event_type type;
    unsigned long long timestamp;
    int fd;
    pid_t proc;
    int size;
    char data[];
};

union event {
    /**
     * Type is common between all event types and we add it here for memory savings.
     * The behavior of such pattern is implementation specific, but we trust that our compiler does it the right way.
     * Remember that for it to work all event structs need to have event_type as the first field.
    */
    enum event_type type;

    struct fork_event fork;
    struct exec_event exec;
    struct exit_event exit;
    struct write_event write;
};

#ifdef __cplusplus
}
#endif

#ifndef __cplusplus

static inline void make_fork_event(struct fork_event *event, pid_t parent, pid_t child) {
    event->type = FORK;
    event->timestamp = bpf_ktime_get_ns();
    event->parent = parent;
    event->child = child;
}

static inline void make_exit_event(struct exit_event *event, pid_t proc, int code) {
    event->type = EXIT;
    event->timestamp = bpf_ktime_get_ns();
    event->proc = proc;
    event->code = code;
}

static inline void make_exec_event(struct exec_event *event, pid_t proc, int args_size) {
    event->type = EXEC;
    event->timestamp = bpf_ktime_get_ns();
    event->proc = proc;
    event->args_size = args_size;
}

static inline void make_write_event(struct write_event *event, pid_t proc, int fd, int size) {
    event->type = WRITE;
    event->timestamp = bpf_ktime_get_ns();
    event->fd = fd;
    event->proc = proc;
    event->size = size;
}

#endif
