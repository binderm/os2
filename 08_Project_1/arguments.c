#include <stdlib.h>
#include "arguments.h"

/**
 * @see header file
 */
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

