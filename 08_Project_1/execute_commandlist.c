#include <errno.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include "execute_commandlist.h"

#define PIPELINE_INTERMEDIATE 0x0
#define PIPELINE_START 0x1
#define PIPELINE_END 0x2
#define IS_PIPELINE_START(command_location) ((command_location & PIPELINE_START) == PIPELINE_START)
#define IS_PIPELINE_END(command_location) ((command_location & PIPELINE_END) == PIPELINE_END)

int execute_command(command *, int, int *);
char **get_argv(command *);
int setup_streams(command *, int, int *, int *, int *);

/**
 * Close the given file descriptor.
 * If closing fails an error message is printed to stderr.
 * Standard file descriptors (stdin, stdout) are not closed.
 *
 * @param fd the file descriptor to be closed
 * @return 0 if file descriptor has been closed successfully or it was one of stdin or stdout, otherwise not 0
 */
int safe_close(int fd) {
	if (fd < 0 || fd == STDIN_FILENO || fd == STDOUT_FILENO) {
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
	int command_location = PIPELINE_START;
	int in;
	for (command *com = clist->head;
			!(pipeline_running || com == NULL);
			com = com->next_one) {
		if (com == clist->tail) {
			command_location |= PIPELINE_END;
		}
		if ((pipeline_running = execute_command(com, command_location, &in))) {
			safe_close(in);
			fprintf(stderr, "seash: Pipeline aborted\n");
		}
		command_location = PIPELINE_INTERMEDIATE;
	}
}

int redirect(int old_fd, int new_fd) {
	if (old_fd != new_fd && dup2(old_fd, new_fd) != new_fd) {
		perror("Failed to redirect input from file");
		return -1;
	}
	return 0;
}

void safe_kill(pid_t pid) {
	if (kill(pid, SIGKILL)) {
		fprintf(stderr, "seash: Failed to kill child %d: %s\n", pid, strerror(errno));
	}
}

int execute_command(command *com, int command_location, int *in) {
	// redirection and pipeing
	int out, next_in;
	if (setup_streams(com, command_location, in, &out, &next_in)) {
		return -1;
	}

	// create child process
	pid_t child_pid = fork();
	if (child_pid == 0) {
		if (redirect(*in, STDIN_FILENO) | redirect(out, STDOUT_FILENO)) {
			exit(-1);
		}
		if (!IS_PIPELINE_END(command_location) && safe_close(next_in)) {
			exit(-1);
		}

		// extract command name and arguments
		char **argv = get_argv(com);
		execvp(argv[0], argv);
		fprintf(stderr, "seash: Failed to change process image of child %d to %s: %s\n", getpid(), argv[0], strerror(errno));
		free(argv);
		exit(-1);
	} else if (child_pid < 0) {
		perror("seash: Failed to fork");
		return -1;
	}

	// close redirection and pipe streams, remember read end of pipe for next command
	if (safe_close(*in) | safe_close(out)) {
		safe_kill(child_pid);
		return -1;
	}
	if (!IS_PIPELINE_END(command_location)) {
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

char **get_argv(command *com) {
	char *file = com->cmd;
	int argc = com->args->len;
	char **argv = calloc(2 + argc, sizeof(char *));
	argv[0] = file;
	struct listnode *argn = com->args->head;
	for (int arg = 0; arg < argc; arg++, argn = argn->next) {
		argv[1 + arg] = argn->str;
	}
	argv[1 + argc] = NULL;
	return argv;
}

int setup_streams(command *com, int command_location, int *in, int *out, int *next_in) {
	if (IS_PIPELINE_START(command_location)) {
		int in_redirect = com->in != NULL;
		if (in_redirect) {
			if ((*in = open(com->in, O_RDWR)) < 0) {
				perror("Failed to open file for input redirection.");
				return -1;
			}
		} else {
			*in = STDIN_FILENO;
		}
	}

	if (IS_PIPELINE_END(command_location)) {
		int out_redirect = com->out != NULL;
		if (out_redirect) {
			if ((*out = open(com->out, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) < 0) {
				perror("Failed to open file for output redirection.");
				return -1;		
			}	
		} else {
			*out = STDOUT_FILENO;
		}
	} else {
		int pipe_fd[2];
		if (pipe(pipe_fd)) {
			perror("Failed to create pipe");
			return -1;
		}
		*out = pipe_fd[1];
		*next_in = pipe_fd[0];
	}
	return 0;
}
