#include <errno.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include "execute_commandlist.h"

int execute_command(command *, int *, int);

/**
 * Close the given file descriptor.
 * If closing fails an error message is printed to stderr.
 * Standard file descriptors (stdin, stdout) are not closed.
 *
 * @param fd the file descriptor to be closed
 * @return 0 if file descriptor has been closed successfully or it was one of stdin or stdout, otherwise not 0
 */
int safe_close(int fd) {
	if (fd == STDIN_FILENO || fd == STDOUT_FILENO) {
		return 0;
	}
	if (close(fd)) {
		fprintf(stderr, "seash: Failed to close file descriptor %d: %s\n", fd, strerror(errno));
		return -1;
	}
	return 0;
}

void execute_commandlist(commandlist *clist) {
	int pipeline_running = 0;
	int in = STDIN_FILENO;
	for (command *com = clist->head;
			!(pipeline_running || com == NULL);
			com = com->next_one) {
		int last = com == clist->tail;
		if ((pipeline_running = execute_command(com, &in, last))) {
			safe_close(in);
			fprintf(stderr, "seash: Pipeline aborted\n");
		}
	}
}

int execute_command(command *com, int *in, int last) {
	// extract command name and arguments
	char *file = com->cmd;
	int argc = com->args->len;
	char *argv[2 + argc];
	argv[0] = file;
	struct listnode *argn = com->args->head;
	for (int arg = 0; arg < argc; arg++, argn = argn->next) {
		argv[1 + arg] = argn->str;
	}
	argv[1 + argc] = NULL;

	// redirection and pipeing
	int out = STDOUT_FILENO, next_in;
	if (com->in != NULL && (*in = open(com->in, O_RDWR)) < 0) {
		perror("Failed to open file for input redirection.");
		return -1;
	}
	if (!last) {
		int pipe_fd[2];
		if (pipe(pipe_fd)) {
			perror("Failed to create pipe");
			return -1;
		}
		out = pipe_fd[1];
		next_in = pipe_fd[0];
	} else if (com->out != NULL && (out = open(com->out, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) < 0) {
		perror("Failed to open file for output redirection.");
		return -1;
	}

	// create child process
	pid_t child_pid = fork();
	if (child_pid == 0) {
		if (*in != STDIN_FILENO && dup2(*in, STDIN_FILENO) != STDIN_FILENO) {
			perror("Failed to redirect input from file");
			exit(-1);
		}
		if (out != STDOUT_FILENO) {
			if (dup2(out, STDOUT_FILENO) != STDOUT_FILENO) {
				perror("Failed to redirect output to file");
				exit(-1);
			}
			if (close(next_in)) {
				perror("Failed to close unneeded read end of pipeline in child process");
				exit(-1);
			}
		}
		execvp(file, argv);
		perror("Failed to exec");
		exit(-1);
	} else if (child_pid < 0) {
		perror("Failed to fork");
		return -1;
	}

	// close redirection and pipe streams, remember read end of pipe for next command
	if (safe_close(*in)) {
		return -1;
	}
	if (safe_close(out)) {
		return -1;
	}
	if (!last) {
		*in = next_in;
	}

	// wait for termination of child process (either natural or by signal)
	int child_status;
	do {
		if (waitpid(child_pid, &child_status, 0) != child_pid) {
			perror("Failed to wait for child");
			return -1;
		}
	} while (!(WIFEXITED(child_status) || WIFSIGNALED(child_status)));

	// query exit status of child process in case of natural termination
	return WIFEXITED(child_status)
		?  WEXITSTATUS(child_status)
		: -1;
}
