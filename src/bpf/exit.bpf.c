#include "vmlinux.h"

#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

#include "event.h"
#include "maps.bpf.h"
#include "utils.bpf.h"

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
