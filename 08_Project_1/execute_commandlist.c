#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include "util.h"
#include "arguments.h"
#include "signal_handling.h"
#include "execute_commandlist.h"
#include "pipeline.h"

int get_command_count(commandlist *);
void kill_pipeline(int *);
int execute_command(command *, int, int *);

void execute_commandlist(commandlist *clist) {
	int command_count = get_command_count(clist);
	int children_pids[command_count + 1];
	memset(children_pids, 0, (command_count + 1) * sizeof(int));

	if (block(SIGINT)) {
		return;
	}

	int command_location = PIPELINE_START, in;
	int i = 0;
	for (command *com = clist->head; com != NULL; com = com->next_one) {
		if (com == clist->tail) {
			command_location |= PIPELINE_END;
		}
		if ((children_pids[i++] = execute_command(com, command_location, &in)) < 0) {
			safe_close(in);
			kill_pipeline(children_pids);
			return;
		}
		command_location = PIPELINE_INTERMEDIATE;
	}

	if (unblock(SIGINT)) {
		kill_pipeline(children_pids);
		exit(-1);
	}

	while (wait(NULL) > 0);
	fflush(stdout);
}

int get_command_count(commandlist *clist) {
	int command_count = 0;
	for (command *com = clist->head; com != NULL; com = com->next_one) {
		command_count++;
	}
	return command_count;
}

void kill_pipeline(int *children_pids) {
	for (int *child_pid = children_pids; *child_pid > 0; child_pid++) {
		if (kill(*child_pid, SIGTERM)) {
			fprintf(stderr, "Failed to kill child %d: %s\n", *child_pid, strerror(errno));
		}
	}
	while (wait(NULL) > 0);
}

int execute_command(command *com, int command_location, int *in) {
	// redirection and pipeing
	int out, next_in;
	if (setup_piping(com, command_location, in, &out, &next_in)) {
		return -1;
	}

	// create child process
	pid_t child_pid = fork();
	if (child_pid == 0) {
		if (reset_signal_handling()
			|| unblock(SIGINT)
			|| redirect(*in, STDIN_FILENO)
			|| redirect(out, STDOUT_FILENO)
			|| (!IS_PIPELINE_END(command_location) && safe_close(next_in))) {
			exit(-1);
		}

		// extract command name and arguments
		char **argv = get_argv(com);
		execvp(argv[0], argv);
		fprintf(stderr, "seash: Failed to change image of child process to %s: %s\n", argv[0], strerror(errno));
		exit(-1);
	} else if (child_pid < 0) {
		perror("seash: Failed to fork new child process");
		return -1;
	}

	// close redirection and pipe streams, remember read end of pipe for next command
	if (safe_close(*in) | safe_close(out)) {
		safe_close(next_in);
		return -1;
	}
	if (!IS_PIPELINE_END(command_location)) {
		*in = next_in;
	}
	return child_pid;
}

