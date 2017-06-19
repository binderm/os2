#ifndef OFS_RESULT_H
#define OFS_RESULT_H

/**
 * The maximum length of the name array in struct ofs_result.
 */
#define OFS_RESULT_NAME_MAX_LENGTH 64

/**
 * A single result (open file) of a open file search.
 */
struct ofs_result {
	/* The pid of the process holding the opened file */
	pid_t pid;

	/* The uid of the owner of the process holding the open file */
	uid_t uid;

	/* The uid of the owner of the opened file */
	uid_t owner;

	/* The permission/mode bitmask (umode_t) */
	unsigned short permissions;

	/* The name of the opened file */
	char name[OFS_RESULT_NAME_MAX_LENGTH];

	/* The size of the opened file in bytes (from loff_t) */
	unsigned int fsize;

	/* The inode number of the opened file */
	unsigned long inode_no;
};

#endif
