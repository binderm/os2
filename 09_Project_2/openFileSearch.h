#ifndef OFS_H
#define OFS_H

#include "ofs_result.h"

/**
 * ioctl command for findin all a process's open() files.
 *
 * ioctrl argument: unsigned int
 *   id of process
 */
#define OFS_PID 0

/**
 * ioctl command for finding all the files a user has open().
 *
 * ioctl argument: unsigned int
 *   id of user
 */
#define OFS_UID 1

/**
 * ioctl command for finding all open() files that are owned by a user.
 *
 * ioctl argument: unsigned int
 *   id of user
 */
#define OFS_OWNER 3 // DO NOT USE 2 -> does not work for some reason

/**
 * ioctl command for finding all files with a given name.
 *
 * ioctl argument: char* up to 64 bytes (null-terminated)
 *   filename
 */
#define OFS_NAME 4

/**
 * Maximum number of results that can be read.
 */
#define OFS_MAX_RESULTS 256

#endif
