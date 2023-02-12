#pragma once

#include "vmlinux.h"

#include <bpf/bpf_helpers.h>

#define MIN(a, b) a < b ? a : b
#define SET_TIMESTAMP(e) e->timestamp = boot_time + bpf_ktime_get_boot_ns()

// time of last boot, injected by user
const volatile __weak uint64_t boot_time;
char LICENSE[] __weak SEC("license") = "GPL";
