#ifndef OFS_RESULT_H
#define OFS_RESULT_H

#define OFS_RESULT_NAME_MAX_LENGTH 64

struct ofs_result {
	pid_t pid;
	uid_t uid;
	uid_t owner;
	unsigned short permissions;
	char name[OFS_RESULT_NAME_MAX_LENGTH];
	unsigned int fsize;
	unsigned long inode_no;
};

#endif
