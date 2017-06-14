#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "../openFileSearch.h"

int main(int argc, char **argv) {
	if (argc < 3) {
		printf("Usage: demo <ioctl_cmd> <ioctl_arg>\n");
		return -1;
	}

	int fd = open("/dev/openFileSearchDev", O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "[FAILED] open: %s\n", strerror(errno));
		return -1;
	}
	printf("[  OK  ] open\n");
	printf("[ INFO ] current pid: %d\n", getpid());

	char *ioctl_cmd_str = argv[1];
	int ioctl_cmd;
	unsigned int uint_arg;
	char *str_arg;
	void *ioctl_arg;
	if (strcmp("OFS_PID", ioctl_cmd_str) == 0) {
		ioctl_cmd = OFS_PID;
		uint_arg = atoi(argv[2]);
		ioctl_arg = &uint_arg;
	} else if (strcmp("OFS_UID", ioctl_cmd_str) == 0) {
		ioctl_cmd = OFS_UID;
		uint_arg = atoi(argv[2]);
		ioctl_arg = &uint_arg;
	} else if (strcmp("OFS_OWNER", ioctl_cmd_str) == 0) {
		ioctl_cmd = OFS_OWNER;
		uint_arg = atoi(argv[2]);
		ioctl_arg = &uint_arg;
	} else if (strcmp("OFS_NAME", ioctl_cmd_str) == 0) {
		ioctl_cmd = OFS_NAME;
		str_arg = argv[2];
		ioctl_arg = str_arg;
	} else {
		fprintf(stderr, "Unknown ioctl command %s\n", ioctl_cmd_str);
	}

	int search_failed;
	if ((search_failed = ioctl(fd, ioctl_cmd, ioctl_arg))) {
		fprintf(stderr, "[FAILED] ioctl %s: %s\n", ioctl_cmd_str, strerror(errno));
	} else {
		printf("[  OK  ] ioctl %s\n", ioctl_cmd_str);
	}

	struct ofs_result result;
	if (search_failed) {
		printf("[ SKIP ] read\n");
	} else {
		int result_count;
		while ((result_count = read(fd, &result, 1)) > 0) {
			printf("         - %s\n              pid: %d, uid: %d, owner: %d, permissions: %u, fsize: %u, inode_no: %lu\n",
				result.name, result.pid, result.uid, result.owner, result.permissions, result.fsize, result.inode_no);
		}
		if (result_count < 0) {
			fprintf(stderr, "[FAILED] read: %s\n", strerror(errno));
		} else {
			printf("[  OK  ] read\n");
		}
	}

	if (close(fd)) {
		fprintf(stderr, "[FAILED] close: %s\n", strerror(errno));
		return -1;
	}
	printf("[  OK  ] close\n");
	return 0;
}

