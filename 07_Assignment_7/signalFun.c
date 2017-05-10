#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define HANDLED_SIGNAL_COUNT 5

int register_signal_handler(int);
void handle_signal(int, siginfo_t *, void *);

int main(int argc, char **argv) {
	if (argc != 2) {
		printf("Usage: signalFun <child_file>\n");
		return -1;
	}

	int handled_signals[HANDLED_SIGNAL_COUNT] = {SIGINT, SIGILL, SIGQUIT, SIGTERM, SIGCLD};
	for (int i = 0; i < HANDLED_SIGNAL_COUNT; i++) {
		if (register_signal_handler(handled_signals[i])) {
			return -1;
		}
	}

	char *child_file = argv[1];
	pid_t child_pid = fork();
	if (child_pid == 0) {
		execvp(child_file, &argv[2]);
		perror("Failed to execute child");
		exit(-1);
	} else if (child_pid == -1) {
		perror("Failed to fork a child process");
		return -1;
	}

	while (1) {
		printf("Waiting for signal...\n");
		sleep(10);
	}
}

int register_signal_handler(int signum) {
	struct sigaction act;
	act.sa_sigaction = &handle_signal;
	act.sa_flags = SA_SIGINFO;

	if (sigaction(signum, &act, NULL)) {
		fprintf(stderr, "Failed to change signal action for signal %d: %s\n",
				signum, strerror(errno));
		return -1;
	}
	return 0;
}

void handle_signal(int signum, siginfo_t *siginfo, void *context) {
	switch (signum) {
		case SIGINT:
			printf("%d: How rude!\n", signum);
			break;
		case SIGILL:
			printf("%d: You be illin'\n", signum);
			break;
		case SIGQUIT:
			printf("%d: Too legit to quit!\n", signum);
			break;
		case SIGTERM:
			printf("%d: I'll be back\n", signum);
			break;
		case SIGCLD:
			;
			int child_status;
			pid_t child_pid = waitpid(-1, &child_status, WNOHANG);
			if (child_pid < 0) {
				perror("Failed to query status of child");
			} else if (child_pid == 0) {
				printf("%d: My child is growing up!\n", signum);
			} else if (WIFEXITED(child_status) || WIFSIGNALED(child_status)) {
				printf("%d: First you forked my child(%d), and now this...\n", signum, child_pid);
			} else {
				fprintf(stderr, "Received signal %d. Child %d has changed state but neither terminated nor stopped by a signal. Ignoring.\n", signum, child_pid);
			}
			break;
		default:
			fprintf(stderr, "Handler registered for unknown signal %d\n", signum);
			return;
	}
}
