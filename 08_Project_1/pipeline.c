#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "util.h"
#include "pipeline.h"

int use_file(char *, int, mode_t);
int from_stdin_or_file(char *);
int to_stdout_or_file(char *);
int to_pipe(int *);

int redirect(int old_fd, int new_fd) {
	if (old_fd != new_fd && dup2(old_fd, new_fd) != new_fd) {
		fprintf(stderr, "seash: Failed to rebind file descriptor %d to %d: %s\n",
				old_fd, new_fd, strerror(errno));
		return -1;
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
int setup_piping(command *com, int command_location, int *in, int *out, int *next_in) {
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

