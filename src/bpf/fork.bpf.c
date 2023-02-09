#include "vmlinux.h"

#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

#include "event.h"
#include "maps.bpf.h"
#include "utils.bpf.h"


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
