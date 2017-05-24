#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "../ofs/ofs.h"

int test_open(int *, int);
int test_ioctl(int, int, char *, long); 
int test_read(int, int, int);
int test_close(int);

int main(int argc, char **argv) {
	int fd;
	if (test_open(&fd, 1)) {
		return -1;
	}
	int fd2;
	test_open(&fd2, 0);

	int pid = 123;
	test_ioctl(fd, OFS_PID, "OFS_PID", &pid);
	if (ioctl(fd, OFS_PID, &pid)) {
		fprintf(stderr, "[FAILED] ioctl OFS_PID: %s\n", strerror(errno));
	}
	printf("[  OK  ] ioctl OFS_PID\n");

	int uid = 123;
	if (ioctl(fd, OFS_UID, &uid)) {
		fprintf(stderr, "[FAILED] ioctl OFS_UID: %s\n", strerror(errno));
	}
	printf("[  OK  ] ioctl OFS_UID\n");

	if (ioctl(fd, OFS_OWNER, &uid)) {
		fprintf(stderr, "[FAILED] ioctl OFS_OWNER: %s\n", strerror(errno));
	}
	printf("[  OK  ] ioctl OFS_OWNER\n");

	char *filename = "/etc/hostname";
	if (ioctl(fd, OFS_NAME, filename)) {
		fprintf(stderr, "[FAILED] ioctl OFS_NAME: %s\n", strerror(errno));
	}
	printf("[  OK  ] ioctl OFS_NAME\n");

	test_read(fd, 0, 0);
	test_read(fd, 256, 256);
	test_read(fd, 260, 256);	

	if (test_close(fd)) {
		return -1;
	}
	return 0;
}

int test_open(int *fd, int expected_success) {
	*fd = open("/dev/openFileSearchDev", O_RDONLY);
	if (expected_success) {
		if (*fd < 0) {
			fprintf(stderr, "[FAILED] open failed but was expected to be successful: %s\n", strerror(errno));
			return -1;
		}
		printf("[  OK  ] open succeeded as expected\n");
		return 0;
	}	
	if (*fd < 0) {
		printf("[  OK  ] open failed as expected: %s\n", strerror(errno));
		return 0;
	}
	fprintf(stderr, "[FAILED] open was expected to fail but was successful\n");
	return -1;

}

int test_ioctl(int fd, int cmd, char *cmd_string, long arg) {
	if (ioctl(fd, cmd, arg)) {
		fprintf(stderr, "[FAILED] ioctl command %s: %s\n", cmd_string, strerror(errno));
		return -1;
	}
	printf("[  OK  ] ioctl command %s\n", cmd_string);
	return 0;
}

int test_read(int fd, int count, int expected_count) {
	struct ofs_result results[count];
	ssize_t actual_count = read(fd, &results, count);
	if (actual_count < 0) {
		fprintf(stderr, "[FAILED] read %d results: %s\n", count, strerror(errno));
		return -1;
	}
	if (actual_count != expected_count) {
		fprintf(stderr, "[FAILED] read expected %d results but was %ld\n", count, actual_count);
		return -1;
	}
	printf("[  OK  ] read count=%d\n", count);
	return 0;
}

int test_close(int fd) {
	if (close(fd)) {
		fprintf(stderr, "[FAILED] close /dev/openFileSearchDev: %s\n", strerror(errno));
		return -1;
	}
	printf("[  OK  ] close /dev/openFileSearchDev\n");
	return 0;
}
