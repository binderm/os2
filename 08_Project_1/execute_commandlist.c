#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "execute_commandlist.h"

#define DEBUG 1

int execute_command(command *);

int execute_commandlist(commandlist *clist) {
	int command_result = 0;
	for (command *com = clist->head;
		!(command_result || com == NULL);
		com = com->next_one) {
		command_result = execute_command(com);
	}
	return command_result;
}

int execute_command(command *com) {
	char *file = com->cmd;
	int argc = com->args->len;
	char *argv[2 + argc];
	argv[0] = file;
	struct listnode *argn = com->args->head;
	for (int arg = 0; arg < argc; arg++, argn = argn->next) {
		argv[1 + arg] = argn->str;
	}
	argv[1 + argc] = NULL;

	pid_t child_pid = fork();
	if (child_pid == 0) {
		execvp(file, argv);
		perror("Failed to exec");
		return -1;
	} else if (child_pid < 0) {
		perror("Failed to fork");
		return -1;
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
