#include <csignal>
#include <cstdio>

#include <future>
#include <functional>
#include <iostream>

#include "bpf_provider.h"
#include "consumer.h"
#include "sysdig_provider.h"

std::function<void(int)> exit_handler;

void sig_handler(int signal) {
    exit_handler(signal);
}

int main(int argc, char **argv) {
    std::cout << "[Main] Starting!\n";

    // TODO: use existing arguments parser (argp??)
    argv++; argc--;

    // BPF is the default provider
    bool use_sysdig = false;
    if (!strcmp(argv[0], "--sysdig")) {  
        use_sysdig = true;
        argv++; argc--;
    }

    sigset_t sig_usr, default_set;
    sigemptyset(&sig_usr);
    sigaddset(&sig_usr, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sig_usr, &default_set);

    pid_t pid = fork();
    std::cout << "[Main] Forked " << pid << "\n";
    
    if (!pid) {
        struct sigaction resume;
        resume.sa_handler = [](int signal){};
        resume.sa_flags = 0;
        sigaction(SIGUSR1, &resume, nullptr);

        std::cout << "[Main] Program process going to sleep\n";
        
        sigsuspend(&default_set);
        sigprocmask(SIG_SETMASK, &default_set, nullptr);

        execvp(*argv, argv);
        return 1;
    }

    sigprocmask(SIG_SETMASK, &default_set, nullptr);

    Consumer consumer(pid);

    Provider* provider_ptr = nullptr;
    if (use_sysdig)
        provider_ptr = new SysdigProvider(pid);
    else
        provider_ptr = new BPFProvider(pid);

    exit_handler = [&](int signal) { 
        consumer.stop();
        provider_ptr->stop();
    };

    // Cleaner handling of Ctrl-C */
    std::signal(SIGTERM, sig_handler);
    std::signal(SIGINT, sig_handler);

    std::thread consumer_thread = std::thread([&]() {
        consumer.start(
            *provider_ptr,
            (use_sysdig) ? true : false // BPF doesn't convert buffers to hex yet
        );
    });

    int ret = provider_ptr->start();
    provider_ptr->stop();
    consumer_thread.join();

    delete provider_ptr;

    return ret;
}