#include "consumer.h"
#include "utils.h"

#include <cstdio>

#include <iostream>

Consumer::Consumer(pid_t root_pid) : root_pid(root_pid) {};

void Consumer::consume(Provider &provider) {
    Provider::event_ref ref = provider.provide();

    if (exiting)
        return;

    if (!ref.ptr) {
        std::cerr << "[Consumer] Got null pointer from refs queue. Stopping consuming\n";
        stop();
        return;
    }

    event *e = ref.ptr.get();
    int buf_len = static_cast<int>(ref.sz - offsetof(struct event, buf));
    const char* buf = reinterpret_cast<const char *>(&e->buf);

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
            if (buf_len > 0) 
			    printf(
                    "%-8s %-7u %.*s\n", 
                    "WRITE", 
                    e->pid, 
                    buf_len, 
                    (hex_input) ? hex_to_string(buf, buf_len).data() : buf
                );
            return;
        case EXEC:
            if (buf_len > 0) 
                printf(
                    "%-8s %-7u %.*s\n",
                    "EXEC",
                    e->pid,
                    buf_len,
                    (hex_input) ? hex_to_string(buf, buf_len).data() : buf
                );
            return;
		default:
			fprintf(stderr, "MALFORMED EVENT WITH TYPE %d\n", e->event_type);
            stop();
            provider.stop();
	}
}