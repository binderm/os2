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
	pid_t pid; /* The pid of the process holding the opened file */
	uid_t uid; /* TODO unclear? */
	uid_t owner; /* The uid of the owner of the opened file */
	unsigned short permissions; /* The permission/mode bitmask */
	char name[OFS_RESULT_NAME_MAX_LENGTH]; /* The name of the opened file */
	unsigned int fsize; /* The size of the opened file in bytes */
	unsigned long inode_no; /* The inode number of the opened file */
};

#endif
