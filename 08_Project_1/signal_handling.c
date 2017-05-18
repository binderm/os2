#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include "signal_handling.h"

void on_sigint(int, siginfo_t *, void *);
int set_block_state(int, int);

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

/**
 * @see header file
 */
int reset_signal_handling() {
struct sigaction act;
		act.sa_handler = SIG_DFL;
		if (sigaction(SIGINT, &act, NULL)) {
			perror("seash: Failed to unregister signal handler for SIGINT in child process");
			return -1;
		}
		return 0;
}

/**
 * @see header file
 */
int block(int signal) {
	return set_block_state(signal, SIG_BLOCK);
}

/**
 * @see header file
 */
int unblock(int signal) {
	return set_block_state(signal, SIG_UNBLOCK);
}

/**
 * Change block state of a single signal.
 *
 * @param signal the number of the signal to change the state for
 * @param block_state the new block state (either SIG_BLOCK or SIG_UNBLOCK)
 * @return 0 if changing the state was successful, != 0 otherwise
 */
int set_block_state(int signal, int block_state) {
	sigset_t signals;
	if (sigemptyset(&signals)
			|| sigaddset(&signals, signal)
			|| sigprocmask(block_state, &signals, NULL)) {
		fprintf(stderr, "seash: [ERROR] Failed to change block state for signal %d to %d: %s\n", signal, block_state, strerror(errno));
		return -1;
	}
	return 0;
}
