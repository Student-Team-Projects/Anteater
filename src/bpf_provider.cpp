#include "bpf_provider.h"

#include <unistd.h>
#include <bpf/libbpf.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>

#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ostream>
#include <string>

#include "fmt/format.h"
#include "spdlog/spdlog.h"

#include "constants.h"
#include "event.h"

#include "tracer.skel.h"

static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args) {
    va_list args_cpy;
    va_copy(args_cpy, args);
    std::vector<char> buf(1+std::vsnprintf(nullptr, 0, format, args_cpy));
    va_end(args_cpy);
    std::vsnprintf(buf.data(), buf.size(), format, args);
    switch (level) {
        case LIBBPF_WARN:
            SPDLOG_WARN(buf.data());
            return 0;
        case LIBBPF_DEBUG:
            SPDLOG_DEBUG(buf.data());
            return 0;
        default:
            SPDLOG_INFO(buf.data());
    }
    return 0;
}

BPFProvider::BPFProvider(pid_t root_pid, size_t capacity) : refs(capacity), root_pid(root_pid){
    /* Set up libbpf errors and debug info callback */
    libbpf_set_strict_mode(LIBBPF_STRICT_ALL);
    libbpf_set_print(libbpf_print_fn);
}

int handle_event(void *ctx, void *data, size_t data_sz) {
    BPFProvider *ths = static_cast<BPFProvider *>(ctx);
    ths->alloc_and_push(static_cast<const Event *>(data), data_sz);
    return 0;
}

int BPFProvider::start() {
    SPDLOG_INFO("Initializing BPF programs");
    int ret = init();
    if (!ret) {
        SPDLOG_INFO("Beginning listening");
        ret = listen();
    }
    return ret;
}

int BPFProvider::init() {
    // Load and verify BPF application
    SPDLOG_INFO("Loading BPF skeleton");
    skel = tracer_bpf__open();
    if (!skel) {
        SPDLOG_ERROR("Failed to open and load BPF skeleton");
        return 1;
    }
    
    int err;

    // Parametrize BPF code with boot time as creation time of kernel proc entry
    SPDLOG_INFO("Parametrizing BPF skeleton with last boot time");
    struct stat kernel_proc_entry;
    err = stat("/proc/1", &kernel_proc_entry);
    if (err) {
        SPDLOG_ERROR("Failed to fetch last boot time");
        return cleanup(err);
    }
    skel->rodata->boot_time = kernel_proc_entry.st_ctim.tv_sec * (uint64_t) 1'000'000'000 + kernel_proc_entry.st_ctim.tv_nsec;

    // create per-cpu auxiliary maps
    SPDLOG_INFO("Creating auxiliary maps");
    err = bpf_map__set_max_entries(skel->maps.aux_maps, get_nprocs());
    if (err) {
        SPDLOG_ERROR("Failed to make per-cpu auxiliary maps");
        return cleanup(err);
    }
    
    // Load and verify BPF programs
    SPDLOG_INFO("Loading programs into BPF skeleton");
    err = tracer_bpf__load(skel);
    if (err) {
        SPDLOG_ERROR("Failed to load and verify BPF skeleton");
        return cleanup(err);
    }

    // Parametrize BPF code with root process PID
    SPDLOG_INFO("Parametrizing BPF skeleton with root PID");
    err = bpf_map__update_elem(skel->maps.pid_tree, &root_pid, sizeof(pid_t), &root_pid, sizeof(pid_t), BPF_ANY);
    if (err) {
        SPDLOG_ERROR("Failed to initialize root pid in BPF program");
        return cleanup(err);
    }

    // Attach tracepoints
    SPDLOG_INFO("Attaching tracepoints");
    err = tracer_bpf__attach(skel);
    if (err) {
        SPDLOG_ERROR("Failed to attach BPF skeleton");
        return cleanup(err);
    }

    // Set up ring buffer polling
    SPDLOG_INFO("Setting up ring buffer");
    rb = ring_buffer__new(bpf_map__fd(skel->maps.rb), handle_event, this, NULL);
    if (!rb) {
        SPDLOG_ERROR("Failed to create ring buffer");
        return cleanup(-1);
    }
    SPDLOG_INFO("Done initializing");
    return 0;
}

int BPFProvider::listen() {
    // unpause traced program
    SPDLOG_INFO("Waking up program to be debugged");
    kill(root_pid, SIGUSR1);
    // Process events
    while (!exiting) {
        int err = ring_buffer__poll(rb, 100);
        // Ctrl-C will cause -EINTR
        if (err == -EINTR)
            return cleanup(0);
        if (err < 0) {
            SPDLOG_ERROR("Error polling buffer: {}", err);
            return cleanup(err);
        }
    }
    SPDLOG_INFO("Done with capturing events");
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
