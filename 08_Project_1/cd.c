#include <stdio.h>
#include <unistd.h>
#include "cd.h"

int cd_check_argc(command *);

/**
 * @see header file
 */
void cd(command *com) {
	if (cd_check_argc(com)) {
		return;
	}

	char *path = com->args->head->str;
	if (chdir(path)) {
		perror("seash: [ERROR] Failed to change directory");
	}
}

/**
 * Check argument count.
 * Only exactly one argument is valid for cd.
 *
 * @return 0 if argument count is valid, != 0 otherwise
 */
int cd_check_argc(command *com) {
	int at_least_one_arg = com->args != NULL
		&& com->args->head != NULL
		&& com->args->head->str != NULL;
	if (!at_least_one_arg) {
		fprintf(stderr, "seash: [ERROR] No directory specified.\nUsage: cd <path>\n");
		return -1;
	}
	int exactly_one_arg = com->args->head->next == NULL;
	if (!exactly_one_arg) {
		fprintf(stderr, "seash: [ERROR] Too many parameters specified.\nUsage: cd <path>\n");
		return -1;
	}
	return 0;
}
