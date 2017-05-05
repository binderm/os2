#include <stdio.h>
#include <unistd.h>
#include "cd.h"

int validate_cd(command *);

void cd(command *com) {
	if (validate_cd(com)) {
		return;
	}

	char *path = com->args->head->str;
	if (chdir(path)) {
		perror("Failed to change directory");
	}
}

int validate_cd(command *com) {
	int at_least_one_arg = com->args != NULL
		&& com->args->head != NULL
		&& com->args->head->str != NULL;
	if (!at_least_one_arg) {
		fprintf(stderr, "No directory specified.\nUsage: cd <path>\n");
		return -1;
	}
	int exactly_one_arg = com->args->head->next == NULL;
	if (!exactly_one_arg) {
		fprintf(stderr, "Too many parameters specified.\nUsage: cd <path>\n");
		return -1;
	}
	return 0;
}
