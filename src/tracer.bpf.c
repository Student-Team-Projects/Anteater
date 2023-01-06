#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

#include "constants.h"

#define MIN(a, b) a < b ? a : b

char LICENSE[] SEC("license") = "Dual BSD/GPL";

// from /sys/kernel/debug/tracing/events/syscalls/sys_enter_write/format
struct write_enter_ctx {
	struct trace_entry ent;
	long int id;
	long unsigned int fd;
	const char *buf;
	size_t count;
};

struct write_event {
	enum event_type event_type;
	pid_t pid;
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

// writes in progress
struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__type(key, __u64);
	__type(value, const char *);
	__uint(max_entries, MAX_CONCURRENT_WRITES);
} writes SEC(".maps");

SEC("tp/sched/sched_process_fork")
int handle_fork(struct trace_event_raw_sched_process_fork *ctx)
{
	pid_t pid = ctx->child_pid, ppid = ctx->parent_pid;
	if (!bpf_map_lookup_elem(&pid_tree, &ppid))
		return 0;

	bpf_map_update_elem(&pid_tree, &pid, &ppid, BPF_ANY);

	/* reserve sample from BPF ringbuf */
	struct event *e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e) {
		bpf_printk("Dropping fork of process %lu from %lu\n", pid, ppid);
		return 0;
	}

	e->event_type = FORK;
	e->pid = pid;
	e->ppid = ppid;

	bpf_ringbuf_submit(e, 0);
	return 0;
}

SEC("tp/sched/sched_process_exit")
int handle_exit(struct trace_event_raw_sched_process_template* ctx)
{
	__u64 id = bpf_get_current_pid_tgid();
	
	pid_t pid = id >> 32, tgid = (pid_t)id;

	/* ignore thread exits and non-descendant */
	if (pid != tgid || !bpf_map_lookup_elem(&pid_tree, &pid))
		return 0;

	/* reserve sample from BPF ringbuf */
	struct event *e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e) {
		bpf_printk("Dropping exit of process %lu\n", pid);
		return 0;
	}

	/* fill out the sample with data */
	struct task_struct *task = (struct task_struct *)bpf_get_current_task();

	e->event_type = EXIT;
	e->pid = pid;
	e->exit_code = (BPF_CORE_READ(task, exit_code) >> 8) & 0xff;

	bpf_ringbuf_submit(e, 0);
	return 0;
}

// ctx is just a reformatted version of trace_event_raw_sys_enter
SEC("tp/syscalls/sys_enter_write")
int handle_write_enter(struct write_enter_ctx *ctx)
{
	__u64 id = bpf_get_current_pid_tgid();
	pid_t pid = id >> 32;

	// ignore non-descendant
	if (!bpf_map_lookup_elem(&pid_tree, &pid))
		return 0;

	const char *buf = ctx->buf;
	bpf_map_update_elem(&writes, &id, &buf, BPF_ANY);
	return 0;
}

SEC("tp/syscalls/sys_exit_write")
int handle_write_exit(struct write_exit_ctx *ctx)
{
	/* get PID of writing process */
	__u64 id = bpf_get_current_pid_tgid();
	pid_t pid = id >> 32;

	/* ignore non-descendant */
	if (!bpf_map_lookup_elem(&pid_tree, &pid))
		return 0;

	const char **buf = bpf_map_lookup_elem(&writes, &id);
	if (!buf)
		return 0;
	bpf_map_delete_elem(&writes, &id);

	if (ctx->ret < 0)
		return 0;
	size_t wsize = MIN(ctx->ret, MAX_WRITE_SIZE);

	u32 cpuid = bpf_get_smp_processor_id();
	struct write_event *aux = bpf_map_lookup_elem(&auxmaps, &cpuid);
	if (!aux)
		return 0;

	aux->event_type = WRITE;
	aux->pid = pid;

	long err = bpf_probe_read_user(aux->data, wsize, *buf);
	if (err) {
		bpf_printk("Error %ld on copying data from address %p of size %u\n", err, *buf, wsize);
		return 0;
	}

	/* send data to user-space for post-processing, have to use bpf_ringbuf_output due to variable size */
	err = bpf_ringbuf_output(&rb, aux, wsize + offsetof(struct write_event, data), 0);
	if (err) {
		bpf_printk("Dropping write of size %lu!\n", wsize);
	}
    return 0;
}