// This include has to be the first one.
#include "vmlinux.h"

#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>

#include "event.h"

char LICENSE[] SEC("license") = "GPL";

struct {
  __uint(type, BPF_MAP_TYPE_RINGBUF);
  __uint(max_entries, 256 * 1024);
} queue __weak SEC(".maps");

struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __type(key, pid_t);
  __type(value, int);
  __uint(max_entries, 256 * 1024);
} processes __weak SEC(".maps");

struct write_data {
  const char *buf;
  int fd;
};

struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __type(key, pid_t);
  __type(value, struct write_data);
  __uint(max_entries, 256 * 1024);
} writes __weak SEC(".maps");

struct {
  __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
  __type(key, u32);
  __type(value, char[2048]);
  __uint(max_entries, 1);
} aux_maps __weak SEC(".maps");

inline bool is_process_traced() {
  pid_t pid = bpf_get_current_pid_tgid();
  return bpf_map_lookup_elem(&processes, &pid) != NULL;
}

SEC("tp/sched/sched_process_exec")
int handle_exec(struct trace_event_raw_sched_process_exec *ctx) {
  if (!is_process_traced()) return 0;

  pid_t pid = bpf_get_current_pid_tgid();

  struct task_struct *task = (void *) bpf_get_current_task();
  u64 args_start =  BPF_CORE_READ(task, mm, arg_start);
  u64 args_end = BPF_CORE_READ(task, mm, arg_end);
  if(args_end <= args_start) return 0;
  u64 args_size = args_end - args_start;
  if (args_size > 1024) args_size = 1024; 

  u32 key = 0;
  struct exec_event *e = bpf_map_lookup_elem(&aux_maps, &key);
  if (e == NULL) return 0;

  make_exec_event(e, pid, args_size);

  if (bpf_probe_read_user(e->args, args_size, (void *) args_start)) return 0;
  bpf_ringbuf_output(&queue, e, args_size + offsetof(struct exec_event, args), 0);
  return 0;
}

SEC("tp/sched/sched_process_fork")
int handle_fork(struct trace_event_raw_sched_process_fork *ctx) {
  if (!is_process_traced()) return 0;

  pid_t parent = ctx->parent_pid;
  pid_t child = ctx->child_pid;
  int value = 0;

  bpf_map_update_elem(&processes, &child, &value, BPF_ANY);
  struct fork_event *event =
      bpf_ringbuf_reserve(&queue, sizeof(struct fork_event), 0);
  if (event == NULL) return 0;
  make_fork_event(event, parent, child);
  bpf_ringbuf_submit(event, 0);
  return 0;
}

SEC("tp/sched/sched_process_exit")
int handle_exit(struct trace_event_raw_sched_process_template *ctx) {
  if (!is_process_traced()) return 0;
  pid_t pid = ctx->pid;
  struct exit_event *event =
      bpf_ringbuf_reserve(&queue, sizeof(struct exit_event), 0);
  if (event == NULL) return 0;
  struct task_struct *task = (struct task_struct *) bpf_get_current_task();
  make_exit_event(event, pid, BPF_CORE_READ(task, exit_code));
  bpf_ringbuf_submit(event, 0);
  bpf_map_delete_elem(&processes, &pid);
  return 0;
}

// from /sys/kernel/debug/tracing/events/syscalls/sys_enter_write/format
struct write_enter_ctx {
  struct trace_entry ent;
  long int id;
  long unsigned int fd;
  const char *buf;
  size_t count;
};

// from /sys/kernel/debug/tracing/events/syscalls/sys_exit_write/format
struct write_exit_ctx {
  struct trace_entry ent;
  long int id;
  long ret;
};

SEC("tp/syscalls/sys_enter_write")
int handle_write_enter(struct write_enter_ctx *ctx) {
  if (!is_process_traced()) return 0;

  pid_t pid = bpf_get_current_pid_tgid();

  struct write_data data = {
    .buf = ctx->buf,
    .fd = ctx->fd,
  };
  bpf_map_update_elem(&writes, &pid, &data, BPF_ANY);
  return 0;
}

#define MAX_WRITE_SIZE 1024

SEC("tp/syscalls/sys_exit_write")
int handle_write_exit(struct write_exit_ctx *ctx) {
  if (!is_process_traced()) return 0;

  pid_t pid = bpf_get_current_pid_tgid();

  struct write_data *data = bpf_map_lookup_elem(&writes, &pid);
  if (data == NULL) return 0;

  bpf_map_delete_elem(&writes, &pid);

  if (ctx->ret < 0) return 0;

  size_t wsize = ctx->ret;
  if (wsize > MAX_WRITE_SIZE) wsize = MAX_WRITE_SIZE;

  u32 key = 0;

  // Kernel correctness checker requires extra copy via auxillary array.
  struct write_event *e = bpf_map_lookup_elem(&aux_maps, &key);
  if (e == NULL) return 0;

  make_write_event(e, pid, data->fd, wsize);
  if (bpf_probe_read_user(e->data, wsize, data->buf)) return 0;

  bpf_ringbuf_output(&queue, e, wsize + offsetof(struct write_event, data), 0);
  return 0;
}
