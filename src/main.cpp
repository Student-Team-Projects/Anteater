#include <signal.h>

#include "tracer_runner.h"
#include "globals.h"

volatile bool exiting = false;
pid_t root_pid;

static void sig_handler(int sig) {
	exiting = true;
}

int main(int argc, char **argv) {
    sigset_t sig_usr, default_set;
    sigemptyset(&sig_usr);
    sigaddset(&sig_usr, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sig_usr, &default_set);

	int pid = fork();
	if (!pid) {
		struct sigaction resume;
        resume.sa_handler = [](int signal){};
        resume.sa_flags = 0;
        sigaction(SIGUSR1, &resume, NULL);

        sigsuspend(&default_set);
        sigprocmask(SIG_SETMASK, &default_set, NULL);

		execvp(argv[1], argv+1);
		return 1;
	}

    root_pid = pid;

    sigprocmask(SIG_SETMASK, &default_set, NULL);

	/* Cleaner handling of Ctrl-C */
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

    TracerRunner runner;
    runner.init();
    kill(pid, SIGUSR1);
    return runner.listen();
}