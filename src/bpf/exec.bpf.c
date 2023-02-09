#include "vmlinux.h"

#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

#include "constants.h"
#include "event.h"
#include "maps.bpf.h"
#include "utils.bpf.h"

#define MAX_EXEC_SIZE MAX_EVENT_SIZE - offsetof(struct Event, exec.data)

SEC("tp/sched/sched_process_exec")
int handle_exec(struct trace_event_raw_sched_process_exec *ctx)
{
    pid_t pid = bpf_get_current_pid_tgid() >> 32;

    // ignore non-descendant
    if (!bpf_map_lookup_elem(&pid_tree, &pid))
        return 0;

    struct task_struct *task = (struct task_struct *)bpf_get_current_task();

    // get address range of args
    u64 arg_start = BPF_CORE_READ(task, mm, arg_start), arg_end = BPF_CORE_READ(task, mm, arg_end);
    if (arg_end <= arg_start) {
        bpf_printk("Args start after they end: %lu %lu\n", arg_start, arg_end);
        return 0;
    }

    // make sure data fits in buffer
    u64 wsize = MIN(arg_end - arg_start - 1, MAX_EXEC_SIZE);

    // get event to fill
    u32 cpuid = bpf_get_smp_processor_id();
    struct Event *e = bpf_map_lookup_elem(&aux_maps, &cpuid);

    if (!e) {
        bpf_printk("Dropping exec of process %lu\n", pid);
        return 0;
    }

    // fill event data
    long err = bpf_probe_read_user(e->exec.data, wsize, (const void *)arg_start);
    if (err) {
        bpf_printk("Copying exec args failed with code: %ld\n", err);
        return 0;
    }
    e->event_type = EXEC;
    e->pid = pid;
    e->exec.length = wsize;
    e->exec.uid = (u32)bpf_get_current_uid_gid();
    SET_TIMESTAMP(e);

    // send data to user-space for post-processing, have to use bpf_ringbuf_output due to variable size
    err = bpf_ringbuf_output(&rb, e, wsize + offsetof(struct Event, exec.data), 0);
    if (err)
        bpf_printk("Dropping exec of size %lu!\n", wsize);
    return 0;
}
