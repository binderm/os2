#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include "signal_handling.h"

void on_sigint(int, siginfo_t *, void *);

/**
 * @see header file
 */
int setup_signal_handling() {
	struct sigaction act;
	act.sa_sigaction = &on_sigint;
	// use the extended version to determine the sender of the signal
	act.sa_flags = SA_SIGINFO;
	if (sigaction(SIGINT, &act, NULL)) {
		perror("seash: Failed to register signal handler for SIGINT");
		return -1;
	}
	return 0;
}

/**
 * Handler for SIGINT.
 * Forwarded to all child processes but does not cause the shell to terminate.
 *
 * @param sig the signal number for which this handler has been called by OS
 * @param siginfo detail information about the signal to be handled
 * @param context nobody knows :-D
 */
void on_sigint(int sig, siginfo_t *siginfo, void *context) {
	// SIGINT is forwarded to all processes in the process group (see below)
	// --> shell will receive another SIGINT
	// --> ignore SIGINT has been sent by shell
	if (siginfo->si_pid != getpid()) {
		// signal handler --> global
		// --> PIDs of child processes unknown
		// send SIGINT to all processes of process group
		kill(0, SIGINT);
		// wait for all child processes to terminate (due to exit or SIGINT)
		while (wait(NULL) > 0);
	}
}
