#include "tracer_runner.h"

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <bpf/libbpf.h>
#include <sys/sysinfo.h>
#include <functional>

#include "tracer.skel.h"
#include "constants.h"
#include "globals.h"

int TracerRunner::cleanup(int err) {
	ring_buffer__free(rb);
	tracer_bpf__destroy(skel);

	return err < 0 ? -err : 0;
}

static int verbose_libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args) {
	return vfprintf(stderr, format, args);
}

static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args) {
	if (level == LIBBPF_DEBUG)
		return 0;
	return vfprintf(stderr, format, args);
}

TracerRunner::TracerRunner(bool verbose) {
	/* Set up libbpf errors and debug info callback */
	libbpf_set_strict_mode(LIBBPF_STRICT_ALL);
	libbpf_set_print(verbose ? verbose_libbpf_print_fn : libbpf_print_fn);
}

int handle_event(void *ctx, void *data, size_t data_sz) {
	const event *e = static_cast<const event *>(data);
	switch (e->event_type) {
		case FORK:
			printf("%-8s %-7u %-13u\n", "FORK", e->pid, e->ppid);
			break;
		case EXIT:
			printf("%-8s %-7u %-13u\n", "EXIT", e->pid, e->exit_code);
			if (e->pid == root_pid)
				exiting = true;
			break;
		case WRITE:
			printf("%-8s %-7u %.*s\n", "WRITE", e->pid, static_cast<int>(data_sz - offsetof(struct event, buf)), reinterpret_cast<const char *>(&e->buf));
			break;
		default:
			return 1;
	}
	return 0;
}

int TracerRunner::init() {
	/* Load and verify BPF application */
	skel = tracer_bpf__open();
	if (!skel) {
		fprintf(stderr, "Failed to open and load BPF skeleton\n");
		return 1;
	}
	
	int err;

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
	rb = ring_buffer__new(bpf_map__fd(skel->maps.rb), handle_event, NULL, NULL);
	if (!rb) {
		fprintf(stderr, "Failed to create ring buffer\n");
		return cleanup(-1);
	}
	return 0;
}

int TracerRunner::listen() {
	/* Process events */
	int err;
	while (!exiting) {
		err = ring_buffer__poll(rb, 100);
		/* Ctrl-C will cause -EINTR */
		if (err == -EINTR) {
			return cleanup(0);
		}
		if (err < 0) {
			fprintf(stderr, "Error polling buffer: %d\n", err);
			return cleanup(err);
		}
	}
	
	return cleanup(0);
}
