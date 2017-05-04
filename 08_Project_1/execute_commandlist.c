#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>
#include "execute_commandlist.h"

#define DEBUG 1

int execute_command(command *, int *, int);

int execute_commandlist(commandlist *clist) {
	int command_result = 0;
	int in = STDIN_FILENO;
	for (command *com = clist->head;
			!(command_result || com == NULL);
			com = com->next_one) {
		int last = com == clist->tail;
		command_result = execute_command(com, &in, last);
	}
	return command_result;
}

int execute_command(command *com, int *in, int last) {
	char *file = com->cmd;
	int argc = com->args->len;
	char *argv[2 + argc];
	argv[0] = file;
	struct listnode *argn = com->args->head;
	for (int arg = 0; arg < argc; arg++, argn = argn->next) {
		argv[1 + arg] = argn->str;
	}
	argv[1 + argc] = NULL;

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

	pid_t child_pid = fork();
	if (child_pid == 0) {
		if (*in != STDIN_FILENO && dup2(*in, STDIN_FILENO) != STDIN_FILENO) {
			perror("Failed to redirect input from file");
			return -1;
		}
		if (out != STDOUT_FILENO && dup2(out, STDOUT_FILENO) != STDOUT_FILENO) {
			perror("Failed to redirect output to file");
			return -1;
		}
		execvp(file, argv);
		perror("Failed to exec");
		int real_child_pid = getpid();
		kill(real_child_pid, SIGKILL);
		return -1;
	} else if (child_pid < 0) {
		perror("Failed to fork");
		return -1;
	}

	if (*in != STDIN_FILENO && close(*in)) {
		perror("Failed to close parent copy of file for input redirection");
		return -1;
	}
	if (out != STDOUT_FILENO && close(out)) {
		perror("Failed to close parent copy of file for output redirection");
		return -1;
	}
	if (!last) {
		*in = next_in;
	}

#if DEBUG
	printf("child_pid=%d\n", child_pid);
#endif

	int child_status;
	do {
		if (waitpid(child_pid, &child_status, 0) != child_pid) {
			perror("Failed to wait for child");
			return -1;
		}
	} while (!(WIFEXITED(child_status) || WIFSIGNALED(child_status)));
	return WIFEXITED(child_status)
		?  WEXITSTATUS(child_status)
		: -1;
}
