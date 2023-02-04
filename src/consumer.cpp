#include "consumer.h"
#include "utils.h"

#include <cstdio>

#include <iostream>

Consumer::Consumer(pid_t root_pid) : root_pid(root_pid) {};

void Consumer::consume(Provider &provider) {
    Provider::event_ref e = provider.provide();

    if (exiting)
        return;

    if (!e) {
        std::cerr << "[Consumer] Got null pointer from refs queue. Stopping consuming\n";
        stop();
        return;
    }

    switch (e->event_type) {
        case FORK:
            printf("%-20lu: %-8s %-7u %-13u\n", e->timestamp, "FORK", e->fork.child_pid, e->pid);
            return;
        case EXIT:
            printf("%-20lu: %-8s %-7u %-13u\n", e->timestamp, "EXIT", e->pid, e->exit.code);
            if (e->pid == root_pid) {
                stop();
                provider.stop();
            }
            return;
        case WRITE:
            if (e->write.length > 0) 
                printf(
                    "%-20lu: %-8s %-7u %-8s %.*s\n", 
                    e->timestamp,
                    "WRITE", 
                    e->pid,
		    (e->write.stream == STDOUT) ? "STDOUT" : "STDERR",
                    static_cast<int>(e->write.length), 
                    (hex_input) ? hex_to_string(e->write.data, e->write.length).data() : e->write.data
                );
            return;
        case EXEC:
            if (e->exec.length > 0) 
                printf(
                    "%-20lu: %-8s %-7u %.*s\n",
                    e->timestamp,
                    "EXEC",
                    e->pid,
                    static_cast<int>(e->exec.length),
                    (hex_input) ? hex_to_string(e->exec.data, e->exec.length).data() : e->exec.data
                );
            return;
        default:
            fprintf(stderr, "MALFORMED EVENT WITH TYPE %d\n", e->event_type);
            stop();
            provider.stop();
    }
}
