#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#define SIGCOUNT 5

int main(int argc, char **argv) {
	pid_t parent_pid = getppid();
	int signums[SIGCOUNT] = {SIGINT, SIGILL, SIGQUIT, SIGTERM, SIGCLD};
	for (int i = 0; i < SIGCOUNT; i++) {
		kill(parent_pid, signums[i]);
		sleep(2);
	}
	return 0;
}
