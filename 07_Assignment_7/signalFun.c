#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <wait.h>

int register_signal_handler(int);
void handle_signal(int, siginfo_t *, void *);

int main(int argc, char **argv) {
	if (argc != 2) {
		printf("Usage: signalFun <child_file>\n");
		return -1;
	}

	if (register_signal_handler(SIGINT)
			|| register_signal_handler(SIGILL)
			|| register_signal_handler(SIGQUIT)
			|| register_signal_handler(SIGTERM)
			|| register_signal_handler(SIGCHLD)) {
		return -1;
	}

	char *child_file = argv[1];
	pid_t child_pid = fork();
	if (child_pid == 0) {
		if (execvp(child_file, &argv[2])) {
			perror("Failed to execute child");
			return -1;
		}
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
		fprintf(stderr, "Failed to change signal action for signal %d: ", signum);
		perror("");
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
			pid_t child_pid = siginfo->si_pid;
			int child_status;
			pid_t wait_result = waitpid(child_pid, &child_status, WNOHANG);
			if (wait_result < 0) {
				fprintf(stderr, "Failed to query status of child %d: ", child_pid);
				perror("");
			} else if (wait_result == 0) {
				printf("%d: My child(%d) is growing up!\n", signum, child_pid);
			} else if (WIFEXITED(child_status)
				|| WIFSIGNALED(child_status)) {
					printf("%d: First you forked my child(%d), and now this...\n", signum, child_pid);
			} else {
				fprintf(stderr, "Status of child(%d) changed but failed to determine if it has terminated", child_pid);
			}
			break;
		default:
			fprintf(stderr, "Handler registered for unknown signal %d\n", signum);
			return;
	}
}
