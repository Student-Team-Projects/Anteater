#include "consumer.h"

#include <cstdio>

Consumer::Consumer(pid_t root_pid) : root_pid(root_pid) {};

void Consumer::consume(Provider &provider) {
    Provider::event_ref ref = provider.provide();
    if (exiting)
        return;
    event *e = ref.ptr.get();
    switch (e->event_type) {
		case FORK:
			printf("%-8s %-7u %-13u\n", "FORK", e->pid, e->ppid);
            return;
		case EXIT:
			printf("%-8s %-7u %-13u\n", "EXIT", e->pid, e->exit_code);
			if (e->pid == root_pid) {
                stop();
				provider.stop();
            }
            return;
		case WRITE:
			printf("%-8s %-7u %.*s\n", "WRITE", e->pid, static_cast<int>(ref.sz - offsetof(struct event, buf)), reinterpret_cast<const char *>(&e->buf));
            return;
		default:
			fprintf(stderr, "MALFORMED EVENT WITH TYPE %d\n", e->event_type);
            stop();
            provider.stop();
	}
}