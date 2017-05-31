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
#include "../ofs.h"

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("Usage: ofs-demo <pid>\n");
		return -1;
	}

	int fd = open("/dev/openFileSearchDev", O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "[FAILED] open: %s\n", strerror(errno));
		return -1;
	}
	printf("[  OK  ] open\n");

	int pid = atoi(argv[1]);
	if (ioctl(fd, OFS_PID, &pid)) {
		fprintf(stderr, "[FAILED] ioctl OFS_PID: %s\n", strerror(errno));
	} else {
		printf("[  OK  ] ioctl OFS_PID\n");
	}

	struct ofs_result results[OFS_MAX_RESULTS];
	int result_count = read(fd, &results, OFS_MAX_RESULTS);
	if (result_count < 0) {
		fprintf(stderr, "[FAILED] read: %s\n", strerror(errno));
	} else {
		printf("[  OK  ] read: %d results\n", result_count);

		for (int result_index = 0; result_index < result_count; result_index++) {
			struct ofs_result *result = &results[result_index];
			printf("         %3d: %s\n              pid: %d, uid: %d, owner: %d, permissions: %u, fsize: %u, inode_no: %lu\n",
				result_index, result->name, result->pid, result->uid, result->owner, result->permissions, result->fsize, result->inode_no);
		}
	}

	if (close(fd)) {
		fprintf(stderr, "[FAILED] close: %s\n", strerror(errno));
		return -1;
	}
	printf("[  OK  ] close\n");
	return 0;
}

