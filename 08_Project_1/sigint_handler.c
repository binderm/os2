#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include "sigint_handler.h"

void on_sigint(int, siginfo_t *, void *);

int register_sigint_handler() {
	struct sigaction act;
	act.sa_sigaction = &on_sigint;
	act.sa_flags = SA_SIGINFO;
	if (sigaction(SIGINT, &act, NULL)) {
		perror("seash: Failed to register signal handler for SIGINT");
	}
	return 0;
}

void on_sigint(int sig, siginfo_t *siginfo, void *context) {
	if (siginfo->si_pid != getpid()) {
		kill(0, SIGINT);
		while (wait(NULL) > 0);
	}
}
