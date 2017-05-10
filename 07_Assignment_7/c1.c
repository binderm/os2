#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define SIGCOUNT 4

int main(int argc, char **argv) {
	pid_t parent_pid = getppid();
	int signums[SIGCOUNT] = {SIGINT, SIGILL, SIGQUIT, SIGTERM};
	for (int i = 0; i < SIGCOUNT; i++) {
		int signum = signums[i];
		if (kill(parent_pid, signum)) {
			fprintf(stderr, "c1: Failed to send signal %d to parent %d: %s\n",
				signum, parent_pid, strerror(errno));
		}
		sleep(2);
	}
	return 0;
}
