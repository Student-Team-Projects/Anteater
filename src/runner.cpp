#include "runner.h"

#include <signal.h>
#include <unistd.h>
#include <wait.h>
#include <iostream>

#include "constants.h"

int run_sysdig(int pipe_fds[2], pid_t executable_pid) {
    // run sysdig with output sent to pipe

    std::cerr << "running sysdig\n";

    close(pipe_fds[0]);
    dup2(pipe_fds[1], STDOUT_FILENO);
    close(pipe_fds[1]);

    execlp(SUDO, SUDO, SYSDIG, "-c", CHISEL, std::to_string(executable_pid).c_str(), nullptr);
    return 1;
}

int run_consumer(int pipe_fds[2], LogConsumer &consumer) {
    // consume sysdig output line by line

    close(pipe_fds[1]);
    FILE *sysdig_info = fdopen(pipe_fds[0], "r");

    char *buffer = nullptr;
    size_t buffer_size = 0;

    while (getline(&buffer, &buffer_size, sysdig_info) != -1)
        consumer.consume(buffer);

    free(buffer);
    while (wait(nullptr) != -1);
    close(pipe_fds[0]);

    return 0;
}

int Runner::run(char **args) {
    sigset_t sig_usr, default_set;
    sigemptyset(&sig_usr);
    sigaddset(&sig_usr, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sig_usr, &default_set);

    pid_t executable_pid = fork();
    if (executable_pid == 0) {
        // run executable after sysdig process starts

        std::cerr << "child goes asleep\n";

        struct sigaction resume;
        resume.sa_handler = [](int signal) {};
        resume.sa_flags = 0;
        sigaction(SIGUSR1, &resume, nullptr);

        sigsuspend(&default_set);
        sigprocmask(SIG_SETMASK, &default_set, nullptr);

        std::cerr << "executable starts\n";
        execvp(args[0], args);
        return 1;
    }

    sigprocmask(SIG_SETMASK, &default_set, nullptr);

    int pipe_fds[2];
    pipe(pipe_fds);

    if (fork() == 0)
        return run_sysdig(pipe_fds, executable_pid);
    return run_consumer(pipe_fds, consumer);
}