#include "vmlinux.h"

#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

#include "event.h"
#include "maps.bpf.h"
#include "utils.bpf.h"

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

#define MAX_WRITE_SIZE MAX_EVENT_SIZE - offsetof(struct Event, write.data)

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
    struct Event *e = bpf_map_lookup_elem(&aux_maps, &cpuid);
    if (!e)
        return 0;

    e->event_type = WRITE;
    e->pid = pid;
    e->write.length = wsize;
    e->write.stream = params->fd;
    SET_TIMESTAMP(e);

    long err = bpf_probe_read_user(e->write.data, wsize, params->buf);
    if (err) {
        bpf_printk("Error %ld on copying data from address %p of size %u\n", err, params->buf, wsize);
        return 0;
    }

    // send data to user-space for post-processing, have to use bpf_ringbuf_output due to variable size
    err = bpf_ringbuf_output(&rb, e, wsize + offsetof(struct Event, write.data), 0);
    if (err)
        bpf_printk("Dropping write of size %lu!\n", wsize);
    return 0;
}
