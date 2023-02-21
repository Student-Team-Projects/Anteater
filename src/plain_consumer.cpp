#include "plain_consumer.h"
#include "utils.h"

#include "spdlog/spdlog.h"

int PlainConsumer::start(Provider &provider, pid_t root_pid, bool hex_input) {
    this->root_pid = root_pid;
    this->hex_input = hex_input;

    SPDLOG_INFO("Starting plain consumer");

    while (!exiting) consume(provider);

    return 0;
}

int PlainConsumer::consume(Provider &provider) {
    Provider::event_ref e = provider.provide();

    if (exiting)
        return 0;

    if (!e) {
        SPDLOG_ERROR("Got null pointer from provider. Stopping consuming");
        stop();
        return 1;
    }

    switch (e->event_type) {
        case FORK:
            printf("%-20lu: %-8s %-7u %-13u\n", e->timestamp, "FORK", e->fork.child_pid, e->pid);
            return 0;
        case EXIT:
            printf("%-20lu: %-8s %-7u %-13u\n", e->timestamp, "EXIT", e->pid, e->exit.code);
            if (e->pid == root_pid) {
                stop();
                provider.stop();
            }
            return 0;
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
            return 0;
        case EXEC:
            convert_spaces(e->exec.data, e->exec.length);
            if (e->exec.length > 0) 
                printf(
                    "%-20lu: %-8s %-7u %-7u %.*s\n",
                    e->timestamp,
                    "EXEC",
                    e->pid,
                    e->exec.uid,
                    static_cast<int>(e->exec.length),
                    (hex_input) ? hex_to_string(e->exec.data, e->exec.length).data() : e->exec.data
                );
            return 0;
        default:
            stop();
            provider.stop();
    }

    return 0;
}

void PlainConsumer::stop() {
    SPDLOG_INFO("Stopping plain consumer");
    exiting = true;
}
