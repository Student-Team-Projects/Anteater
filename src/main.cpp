#include <csignal>
#include <cstdio>
#include <future>
#include <functional>

#include "bpf_provider.h"
#include "consumer.h"

std::function<void(int)> exit_handler;

void sig_handler(int signal) {
    exit_handler(signal);
}

int main(int argc, char **argv) {
    fprintf(stderr, "Main starting!\n");

    sigset_t sig_usr, default_set;
    sigemptyset(&sig_usr);
    sigaddset(&sig_usr, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sig_usr, &default_set);

    pid_t pid = fork();
    fprintf(stderr, "Forked: %d!\n", pid);
    
    if (!pid) {
        struct sigaction resume;
        resume.sa_handler = [](int signal){};
        resume.sa_flags = 0;
        sigaction(SIGUSR1, &resume, nullptr);

        fprintf(stderr, "Child going to sleep\n");
        sigsuspend(&default_set);
        sigprocmask(SIG_SETMASK, &default_set, nullptr);

        execvp(argv[1], argv+1);
        return 1;
    }

    sigprocmask(SIG_SETMASK, &default_set, nullptr);

    BPFProvider provider(pid);
    Consumer consumer(pid);

    exit_handler = [&](int signal) { 
        consumer.stop();
        provider.stop();
    };

    /* Cleaner handling of Ctrl-C */
    std::signal(SIGTERM, sig_handler);
    std::signal(SIGINT, sig_handler);

    std::thread consumer_thread = std::thread([&]() {
        consumer.start(provider);
    });

    int ret = provider.start();
    provider.stop();
    consumer_thread.join();
    return ret;
}