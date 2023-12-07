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

struct {
  __uint(type, BPF_MAP_TYPE_ARRAY);
  __type(key, u32);
  __type(value, struct file *);
  __uint(max_entries, 2);
} tracked_descriptors __weak SEC(".maps");

struct write_data {
  const char *buf;
  enum descriptor fd;
  int padding;
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
  __type(value, char[4096]);
  __uint(max_entries, 1);
} aux_maps __weak SEC(".maps");

#define MAX_PATH_COMPONENTS 20
struct {
  __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
  __type(key, u32);
  __type(value, struct qstr [MAX_PATH_COMPONENTS]);
  __uint(max_entries, 1);
} path_storage __weak SEC(".maps");

inline bool is_process_traced() {
  pid_t pid = bpf_get_current_pid_tgid();
  return bpf_map_lookup_elem(&processes, &pid) != NULL;
}


// DO NOT TOUCH. IT CAN AND WILL HURT YOU.
// may not work with paths through multiple mountpoints.
static inline long path_to_str(struct path *path, char *buf, int size) {
  struct dentry *dentry = BPF_CORE_READ(path, dentry);
  u32 key = 0;
  struct qstr *storage = bpf_map_lookup_elem(&path_storage, &key);
  if(storage == NULL) return -1;
  long path_len = 0;
  long component = 0;
  for(; component < MAX_PATH_COMPONENTS; component++) {
    struct dentry *parent = BPF_CORE_READ(dentry, d_parent);
    if(dentry == parent) break;
    storage[component] = BPF_CORE_READ(dentry, d_name);
    path_len += storage[component].len + 1;
    dentry = parent;
  }
  if(component >= MAX_PATH_COMPONENTS) return -1;
  component--;
  if(path_len > size) return -1;
  char *cur = buf;
  for(;component >= 0; component--) {
    *(cur++) = '/';
    struct qstr name = storage[component];
    long ret = bpf_probe_read_kernel_str(cur, 64, name.name);
    if(ret < 0) return -1;
    cur += name.len;
    if(cur >= buf + size - 64) return -1;
  }
  return path_len;
}


SEC("tp/sched/sched_process_exec")
int handle_exec(struct trace_event_raw_sched_process_exec *ctx) {
  if (!is_process_traced()) return 0;

  pid_t pid = bpf_get_current_pid_tgid();

  struct task_struct *task = (void *) bpf_get_current_task();

  // on first exec save file descriptors
  u32 stdout_id = 0;
  u32 stderr_id = 1;
  struct file **current_stdout = bpf_map_lookup_elem(&tracked_descriptors, &stdout_id);
  if(current_stdout == NULL)
    return 0;
  if(*current_stdout == NULL) {
    struct file **fdt = BPF_CORE_READ(task, files, fdt, fd);
    struct file *stdout, *stderr;
    if(bpf_probe_read_kernel(&stdout, sizeof(stdout), fdt + 1))
      return 0;
    if(bpf_probe_read_kernel(&stderr, sizeof(stderr), fdt + 2))
      return 0;

    bpf_map_update_elem(&tracked_descriptors, &stdout_id, &stdout, BPF_ANY);
    bpf_map_update_elem(&tracked_descriptors, &stderr_id, &stderr, BPF_ANY);
  }

  // reserve space for event
  u32 key = 0;
  struct exec_event *e = bpf_map_lookup_elem(&aux_maps, &key);
  if (e == NULL) return 0;

  // find args
  u64 args_start =  BPF_CORE_READ(task, mm, arg_start);
  u64 args_end = BPF_CORE_READ(task, mm, arg_end);
  if(args_end <= args_start) return 0;
  u64 args_size = args_end - args_start - 1;
  if (args_size > 1024) args_size = 1024;
  if (bpf_probe_read_user(e->data, args_size, (void *) args_start)) return 0;

  // find pwd
  struct path pwd = BPF_CORE_READ(task, fs, pwd);
  int pwd_size = path_to_str(&pwd, e->data + args_size, 1024);
  if(pwd_size < 0) return 0;
  if(pwd_size > 1024) return 0;

  make_exec_event(e, pid, args_size, pwd_size);
  int data_size = pwd_size + args_size;
  if(data_size < 0) return 0;
  if(data_size > 2047) return 0;
  data_size &= 2047;

  bpf_ringbuf_output(&queue, e, data_size + offsetof(struct exec_event, data), 0);
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
  make_exit_event(event, pid, (BPF_CORE_READ(task, exit_code) >> 8) & 0xFF);
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

  struct task_struct *task = (void *) bpf_get_current_task();

  struct file *dst;
  bpf_probe_read_kernel(&dst, sizeof(dst), (struct file **) BPF_CORE_READ(task, files, fdt, fd) + ctx->fd);
  u32 stdout_id = 0;
  u32 stderr_id = 1;
  struct file *stdout, *stderr;
  struct file **f = bpf_map_lookup_elem(&tracked_descriptors, &stdout_id);
  if(f == NULL) return 0;
  stdout = *f;
  f = bpf_map_lookup_elem(&tracked_descriptors, &stderr_id);
  if(f == NULL) return 0;
  stderr = *f;

  enum descriptor fd;
  if(dst == stdout)
    fd = STDOUT;
  else if(dst == stderr)
    fd = STDERR;
  else return 0;

  pid_t pid = bpf_get_current_pid_tgid();

  struct write_data data = {
    .buf = ctx->buf,
    .fd = fd,
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
