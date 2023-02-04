#include "vmlinux.h"

#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

#include "constants.h"
#include "event.h"

#define MIN(a, b) a < b ? a : b

#define SET_TIMESTAMP(e) e->timestamp = boot_time + bpf_ktime_get_boot_ns()

char LICENSE[] SEC("license") = "Dual BSD/GPL";

// from /sys/kernel/debug/tracing/events/syscalls/sys_enter_write/format
struct write_enter_ctx {
    struct trace_entry ent;
    long int id;
    long unsigned int fd;
    const char *buf;
    size_t count;
};

// has to be castable to Event
struct write_event {
    enum EventType event_type;
    pid_t pid;
    u64 timestamp;

    enum Stream stream;
    size_t length;
    char data[MAX_WRITE_SIZE];
};

// percpu auxiliary maps to copy write buffers
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __type(key, u32);
    __type(value, struct write_event);
} auxmaps SEC(".maps");

// from /sys/kernel/debug/tracing/events/syscalls/sys_exit_write/format
struct write_exit_ctx {
    struct trace_entry ent;
    long int id;
    long ret;
};

// process tree with root set by userspace
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __type(key, pid_t);
    __type(value, pid_t);
    __uint(max_entries, PROC_COUNT_MAX);
} pid_tree SEC(".maps");

// buffer for all events sent to userspace
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, EVENT_RING_BUFFER_SIZE);
} rb SEC(".maps");

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
} writes SEC(".maps");

// time of last boot, injected by user
const volatile uint64_t boot_time;

SEC("tp/sched/sched_process_fork")
int handle_fork(struct trace_event_raw_sched_process_fork *ctx)
{
    pid_t cpid = BPF_CORE_READ(ctx, child_pid), pid = BPF_CORE_READ(ctx, parent_pid);
    if (!bpf_map_lookup_elem(&pid_tree, &pid))
        return 0;

    bpf_map_update_elem(&pid_tree, &cpid, &pid, BPF_ANY);

    /* reserve sample from BPF ringbuf */
    struct Event *e = bpf_ringbuf_reserve(&rb, sizeof(struct Event), 0);
    if (!e) {
        bpf_printk("Dropping fork of process %lu from %lu\n", cpid, pid);
        return 0;
    }

    e->event_type = FORK;
    e->pid = pid;
    e->fork.child_pid = cpid;
    SET_TIMESTAMP(e);

    bpf_ringbuf_submit(e, 0);
    return 0;
}

SEC("tp/sched/sched_process_exit")
int handle_exit(struct trace_event_raw_sched_process_template* ctx)
{
    u64 id = bpf_get_current_pid_tgid();
    pid_t pid = id >> 32, tgid = (pid_t)id;

    /* ignore thread exits and non-descendant */
    if (pid != tgid || !bpf_map_lookup_elem(&pid_tree, &pid))
        return 0;

    /* reserve sample from BPF ringbuf */
    struct Event *e = bpf_ringbuf_reserve(&rb, sizeof(struct Event), 0);
    if (!e) {
        bpf_printk("Dropping exit of process %lu\n", pid);
        return 0;
    }

    /* fill out the sample with data */
    struct task_struct *task = (struct task_struct *)bpf_get_current_task();

    e->event_type = EXIT;
    e->pid = pid;
    e->exit.code = (BPF_CORE_READ(task, exit_code) >> 8) & 0xff;
    SET_TIMESTAMP(e);

    bpf_ringbuf_submit(e, 0);
    return 0;
}

SEC("tp/sched/sched_process_exec")
int handle_exec(struct trace_event_raw_sched_process_exec *ctx)
{
    pid_t pid = bpf_get_current_pid_tgid() >> 32;

    // ignore non-descendant
    if (!bpf_map_lookup_elem(&pid_tree, &pid)) {
        return 0;
    }

    struct Event *e = bpf_ringbuf_reserve(&rb, sizeof(struct Event) + TASK_COMM_LEN, 0);
    if (!e) {
        bpf_printk("Dropping exec of process %lu\n", pid);
        return 0;
    }

    e->event_type = EXEC;
    e->pid = pid;
    SET_TIMESTAMP(e);
    bpf_get_current_comm(&e->exec.data, TASK_COMM_LEN);
    e->exec.length = TASK_COMM_LEN;

    bpf_ringbuf_submit(e, 0);
    return 0;
}

// ctx is just a reformatted version of trace_event_raw_sys_enter
SEC("tp/syscalls/sys_enter_write")
int handle_write_enter(struct write_enter_ctx *ctx)
{
    u64 id = bpf_get_current_pid_tgid();
    pid_t pid = id >> 32;

    // ignore non-descendant
    if (!bpf_map_lookup_elem(&pid_tree, &pid))
        return 0;

    // get file descriptor table for given process
    struct task_struct *task = (struct task_struct *)bpf_get_current_task();
    struct file **fdt = BPF_CORE_READ(task, files, fdt, fd);

    // get pointers to open file descriptions of stdout, stderr and target
    struct file *stdout, *stderr, *dst;
    if (bpf_probe_read_kernel(&stdout, sizeof(stdout), fdt + 1))
        return 0;
    if (bpf_probe_read_kernel(&stderr, sizeof(stdout), fdt + 2))
        return 0;
    if (bpf_probe_read_kernel(&dst, sizeof(stdout), fdt + ctx->fd))
        return 0;

    // if the underlying open file description is different from stdin and stdout, then ignore
    if (dst != stdout && dst != stderr)
        return 0;

    struct write_params params;
    params.buf = ctx->buf;
    params.fd = dst == stdout ? STDOUT : STDERR;

    const char *buf = ctx->buf;
    bpf_map_update_elem(&writes, &id, &params, BPF_ANY);
    return 0;
}

SEC("tp/syscalls/sys_exit_write")
int handle_write_exit(struct write_exit_ctx *ctx)
{
    /* get PID of writing process */
    u64 id = bpf_get_current_pid_tgid();
    pid_t pid = id >> 32;

    /* ignore non-descendant */
    if (!bpf_map_lookup_elem(&pid_tree, &pid))
        return 0;

    struct write_params *params = bpf_map_lookup_elem(&writes, &id);
    if (!params)
        return 0;
    bpf_map_delete_elem(&writes, &id);

    if (ctx->ret < 0)
        return 0;

    size_t wsize = MIN(ctx->ret, MAX_WRITE_SIZE);

    u32 cpuid = bpf_get_smp_processor_id();
    struct write_event *e = bpf_map_lookup_elem(&auxmaps, &cpuid);
    if (!e)
        return 0;

    e->event_type = WRITE;
    e->pid = pid;
    e->length = wsize;
    e->stream = params->fd;
    SET_TIMESTAMP(e);

    long err = bpf_probe_read_user(e->data, wsize, params->buf);
    if (err) {
        bpf_printk("Error %ld on copying data from address %p of size %u\n", err, params->buf, wsize);
        return 0;
    }

    /* send data to user-space for post-processing, have to use bpf_ringbuf_output due to variable size */
    err = bpf_ringbuf_output(&rb, e, wsize + offsetof(struct write_event, data), 0);
    if (err) {
        bpf_printk("Dropping write of size %lu!\n", wsize);
    }
    return 0;
}