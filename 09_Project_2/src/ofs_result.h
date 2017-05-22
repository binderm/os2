struct osf_result {
	pid_t pid;
	uid_t uid;
	uid_t owner;
	unsigned short permissions;
	char name[64];
	unsigned int fsize;
	unsigned long inode_no;
};
