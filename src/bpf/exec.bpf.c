#include "vmlinux.h"

#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

#include "constants.h"
#include "event.h"
#include "maps.bpf.h"
#include "utils.bpf.h"

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