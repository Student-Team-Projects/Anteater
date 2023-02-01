#include "bpf_provider.h"

#include <unistd.h>
#include <bpf/libbpf.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>

#include <csignal>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "constants.h"
#include "event.h"

#include "tracer.skel.h"

static int verbose_libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args) {
    return vfprintf(stderr, format, args);
}

static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args) {
    if (level == LIBBPF_DEBUG)
        return 0;
    return vfprintf(stderr, format, args);
}

BPFProvider::BPFProvider(pid_t root_pid, size_t capacity, bool verbose) : refs(capacity), root_pid(root_pid){
    /* Set up libbpf errors and debug info callback */
    libbpf_set_strict_mode(LIBBPF_STRICT_ALL);
    libbpf_set_print(verbose ? verbose_libbpf_print_fn : libbpf_print_fn);
}

int handle_event(void *ctx, void *data, size_t data_sz) {
    BPFProvider *ths = static_cast<BPFProvider *>(ctx);
    ths->alloc_and_push(static_cast<const Event *>(data), data_sz);
    return 0;
}

int BPFProvider::start() {
    int ret = init();
    if (!ret) ret = listen();
    return ret;
}

int BPFProvider::init() {
    // Load and verify BPF application
    skel = tracer_bpf__open();
    if (!skel) {
        fprintf(stderr, "Failed to open and load BPF skeleton\n");
        return 1;
    }
    
    int err;

    // Parametrize BPF code with boot time as creation time of kernel proc entry
    struct stat kernel_proc_entry;
    err = stat("/proc/1", &kernel_proc_entry);
    if (err) {
        fprintf(stderr, "Failed to fetch last boot time\n");
        return cleanup(err);
    }
    skel->rodata->boot_time = kernel_proc_entry.st_ctim.tv_sec * (uint64_t) 1'000'000'000 + kernel_proc_entry.st_ctim.tv_nsec;

    // create per-cpu auxiliary maps
    err = bpf_map__set_max_entries(skel->maps.auxmaps, get_nprocs());
    if (err) {
        fprintf(stderr, "Failed to make per-cpu auxiliary maps\n");
        return cleanup(err);
    }
    
    // Load and verify BPF programs
    err = tracer_bpf__load(skel);
    if (err) {
        fprintf(stderr, "Failed to load and verify BPF skeleton\n");
        return cleanup(err);
    }

    // Parametrize BPF code with root process PID
    err = bpf_map__update_elem(skel->maps.pid_tree, &root_pid, sizeof(pid_t), &root_pid, sizeof(pid_t), BPF_ANY);
    if (err) {
        fprintf(stderr, "Failed to initialize root pid in BPF program\n");
        return cleanup(err);
    }

    // Attach tracepoints
    err = tracer_bpf__attach(skel);
    if (err) {
        fprintf(stderr, "Failed to attach BPF skeleton\n");
        return cleanup(err);
    }

    // Set up ring buffer polling
    rb = ring_buffer__new(bpf_map__fd(skel->maps.rb), handle_event, this, NULL);
    if (!rb) {
        fprintf(stderr, "Failed to create ring buffer\n");
        return cleanup(-1);
    }
    return 0;
}

int BPFProvider::listen() {
    // unpause traced program
    kill(root_pid, SIGUSR1);
    // Process events
    while (!exiting) {
        int err = ring_buffer__poll(rb, 100);
        // Ctrl-C will cause -EINTR
        if (err == -EINTR)
            return cleanup(0);
        if (err < 0) {
            fprintf(stderr, "Error polling buffer: %d\n", err);
            return cleanup(err);
        }
    }
    
    return cleanup(0);
}

int BPFProvider::cleanup(int err) {
    refs.unblock();
    ring_buffer__free(rb);
    tracer_bpf__destroy(skel);

    return err < 0 ? -err : 0;
}

void BPFProvider::alloc_and_push(const Event *pt, size_t sz) {
    Event *pt_cpy = static_cast<Event *>(malloc(sz));
    if (pt_cpy == nullptr)
        return;
    memmove(pt_cpy, pt, sz);
    refs.push(event_ref(pt_cpy));
}

Provider::event_ref BPFProvider::provide() {
    return refs.pop();
}
