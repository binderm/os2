#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include "arguments.h"
#include "signal_handling.h"
#include "execute_commandlist.h"

#define PIPELINE_INTERMEDIATE 0x0
#define PIPELINE_START 0x1
#define PIPELINE_END 0x2
#define IS_PIPELINE_START(command_location) ((command_location & PIPELINE_START) == PIPELINE_START)
#define IS_PIPELINE_END(command_location) ((command_location & PIPELINE_END) == PIPELINE_END)

int safe_close(int);
void kill_pipeline();
int execute_command(command *, int, int *);
int setup_streams(command *, int, int *, int *, int *);

void execute_commandlist(commandlist *clist) {
	if (block(SIGINT)) {
		fprintf(stderr, "seash: [ERROR] failed to block SIGINT before pipeline setup --> Cancel\n");
		return;
	}

	int command_location = PIPELINE_START, in;
	for (command *com = clist->head; com != NULL; com = com->next_one) {
		if (com == clist->tail) {
			command_location |= PIPELINE_END;
		}
		if (execute_command(com, command_location, &in)) {
			fprintf(stderr, "seash: [ERROR] during pipeline setup at point %s --> Kill pipeline\n", com->cmd);
			safe_close(in);
			kill_pipeline();
			return;
		}
		command_location = PIPELINE_INTERMEDIATE;
	}

	if (unblock(SIGINT)) {
		fprintf(stderr, "seash: [FATAL] Failed to unblock SIGINT after pipeline setup --> Terminate shell\n");
		kill_pipeline();
		exit(-1);
	}

	while (wait(NULL) > 0);
	fflush(stdout);
}

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

void kill_pipeline() {
	if (kill(0, SIGINT)) {
		perror("seash: Failed to kill pipeline");
	}
}

int redirect(int old_fd, int new_fd) {
	if (old_fd != new_fd && dup2(old_fd, new_fd) != new_fd) {
		fprintf(stderr, "seash: Failed to rebind file descriptor %d to %d: %s\n",
				old_fd, new_fd, strerror(errno));
		return -1;
	}
	return 0;
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
		if (reset_signal_handling()) {
			fprintf(stderr, "seash: [ERROR] Failed to remove signal handling for child process\n");
			exit(-1);
		}
		if (unblock(SIGINT)) {
			fprintf(stderr, "seash: [ERROR] Failed to unblock SIGINT for child process\n");
			exit(-1);
		}
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
		exit(-1);
	} else if (child_pid < 0) {
		perror("seash: [ERROR] Failed to fork new child process");
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
	return 0;
}

int from_to_file(char *pathname, int flags, mode_t mode) {
	int fd = open(pathname, flags, mode);
	if (fd < 0) {
		fprintf(stderr, "seash: [ERROR] Failed to open file %s: %s\n", pathname, strerror(errno));
	}
	return fd;
}

/**
 * Determine the file descriptor from which the input of the first command should be read.
 * This is either stdin or a file.
 *
 * @param the input file or NULL if stdin should be used instead
 * @return the file descriptor to read input from
 */
int from_stdin_or_file(char *in) {
	return in != NULL
		? from_to_file(in, O_RDWR, 0)
		: STDIN_FILENO;
}

/**
 * Determine the file descriptor to which the output of the last command should be redirected.
 * This is either stdout or a file. In case of a file its lenght is truncated to zero.
 *
 * @param the output file or NULL if stdout should be used instead
 * @return the file descriptor to redirect output to
 */
int to_stdout_or_file(char *out) {
	return out != NULL
		? from_to_file(out, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)
		: STDOUT_FILENO;
}

/**
 * Determine the file descriptor to which the output of a command in the middle of the pipeline should be redirected.
 * This is always the writing end of a newly created pipe. Its reading end is set to the provided parameter and is used as the input of the next command.
 *
 * @param the file descriptor for the input of the next command
 * @return the file descriptor for the output of the current command, or -1 if creating the pipe fails
 */
int to_pipe(int *next_in) {
	int pipe_fd[2];
	if (pipe(pipe_fd)) {
		perror("seash: [ERROR] Failed to create pipe between two processes");
		return -1;
	}
	*next_in = pipe_fd[0];
	return pipe_fd[1];
}

/**
 * Setup the streams for the current and next command.
 *
 * @param com the current command
 * @param command_location the location of the command within the pipeline
 * @param in the file descriptor from which the output of the previous command can be read, ignored if this is the first command in the pipeline
 * @param out afterwards the file descriptor to which the output of the current command should be wrote
 * @param next_in the file descriptor from which the next command can read the output of the current command
 * @return 0 if setup was successful, != 0 otherwise
 */
int setup_streams(command *com, int command_location, int *in, int *out, int *next_in) {
	if (IS_PIPELINE_START(command_location)) {
		*in = from_stdin_or_file(com->in);
	}
	if (*in < 0) {
		return -1;
	}

	*out = IS_PIPELINE_END(command_location)
		? to_stdout_or_file(com->out)
		: to_pipe(next_in);
	if (*out < 0) {
		safe_close(*in);
		return -1;
	}
	return 0;
}
