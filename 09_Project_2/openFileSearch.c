#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/fdtable.h>
#include <linux/dcache.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/err.h>
#include <linux/init_task.h>
#include "openFileSearch.h"
#define MODULE_NAME "openFileSearch"

MODULE_AUTHOR("Marcel Binder <binder4@hm.edu>");
MODULE_LICENSE("GPL");

static int major_num_;
static int device_opened_ = 0;
static struct ofs_result *results_;
static int search_performed_ = 0;
static unsigned int result_count_ = 0;

static int ofs_open(struct inode *inode, struct file *flip) {
	if (device_opened_) {
		printk(KERN_WARNING "ofs: Driver already in use\n");
		return -EBUSY;
	}
	device_opened_ = 1;
	printk(KERN_INFO "ofs: Opened\n");
	return 0;
}

typedef int (*ofs_result_filter)(struct ofs_result *, void *);

int ofs_no_filter(struct ofs_result *result, void *filter_arg) {
	return 1;
}

int ofs_filter_by_uid(struct ofs_result *result, void *filter_arg) {
	unsigned int uid = *(unsigned int *) filter_arg;
	return result->uid == uid;
}

int ofs_filter_by_owner(struct ofs_result *result, void *filter_arg) {
	unsigned int owner = *(unsigned int *) filter_arg;
	return result->owner == owner;
}

int ofs_filter_by_name(struct ofs_result *result, void *filter_arg) {
	char * name = (char *) filter_arg;
	return strncmp(result->name, name, OFS_RESULT_NAME_MAX_LENGTH) == 0;
}

static void ofs_find_open_files_of_task(struct task_struct *task, ofs_result_filter filter, void *filter_arg) {
	pid_t pid;
	uid_t uid;
	struct files_struct *files;
	struct fdtable *fdt;
	unsigned int fd_capacity;
	unsigned int fds_length;
	int limit_exerceeded;
	struct ofs_result *result;
	unsigned int fds_section_index;
	unsigned long open_fds_section;
	unsigned int fds_bit_index;
	unsigned int open_fd_index;
	struct file *open_file;
	char *result_name_buffer;
	char *result_name;
	struct inode *inode;

	if (!(result_name_buffer = kmalloc(OFS_RESULT_NAME_MAX_LENGTH * sizeof(char), GFP_KERNEL))) {
		printk(KERN_ERR "ofs: Failed to allocate memory for result name\n");
		return;
	}

	pid = task->pid;
	uid = task->cred->uid.val;
	files = task->files;

	// Reading the fdtable must be protected with RCU read lock
	rcu_read_lock();

	// reference to file_struct::fdt MUST be done via files_fdtable macro
	// --> ensures lock-free dereference (see <kernel>/Documentation/filesystems/files.txt)
	// (files_struct::fdt initially points to files_struct::fdtab but elsewhere after possible expansion of fdtable)
	fdt = files_fdtable(files);
	// now one CAN safely read fdt

	// fdtable primarily is a wrapper around a array of file descriptors (fdtable::fd of type struct file *)
	// max_fds contains the capacity (not number of elements) of this array
	// initially the capacity MUST be at least BITS_PER_LONG
	// the fill state of the array CAN be determined via so called file descriptor sets (fdtable::xx_fds) = array of long values
	// --> chained bit mask first bit of first long is for the first fd in the array, second bit for the second, ..., first bit of second long is for the (BITS_PER_LONG + 1)-th fd in the array
	// length of the fds arrays is therefore max_fds / BITS_PER_LONG
	fd_capacity = fdt->max_fds;
	fds_length = fd_capacity / BITS_PER_LONG;
	printk(KERN_DEBUG "ofs: fd_capacity=%u, fds_length=%u\n", fd_capacity, fds_length);

	// e.g. opening a owned file in vim and write
	// open_fds bits 1-3 (stdin, stdout, stderr) and 5 (.swp file for permanent backup) set = opened
	// open_fds bits 4 (original file) not opened (only for a few moments when originally loading the files content)
	// open_fds bits 6 (original file) used for writing (only opened for write operation)
	// close_on_exec bit 3 set (.swp file) should be closed when changing process image but stdin, stdout, stderr should remain open of course
	// TODO whats full_fds_bits for? -> always 0? length?
	limit_exerceeded = 0;
	for (fds_section_index = 0; !limit_exerceeded && fds_section_index < fds_length; fds_section_index++) {
		open_fds_section = fdt->open_fds[fds_section_index];
		printk(KERN_DEBUG "ofs: open_fds[%u]=%lu\n", fds_section_index, open_fds_section);

		for (fds_bit_index = 0; !limit_exerceeded && fds_bit_index < BITS_PER_LONG; fds_bit_index++) {
			if (open_fds_section & (1UL << fds_bit_index)) {
				if (result_count_ > OFS_MAX_RESULTS) {
					printk(KERN_WARNING "Found more than %u results\n", OFS_MAX_RESULTS);
					limit_exerceeded = 1;
					break; // TODO remove break
				}
				result = &results_[result_count_];

				open_fd_index = fds_section_index + fds_bit_index;

				// for getting a struct file from the fd array fcheck_files MUST be used holding the RCU read lock (see implementation: the last parameter is the index for the fd array)
				open_file = fcheck_files(files, open_fd_index);
				if (open_file) {
					result->pid = pid;
					// TODO is atomic_long_inc_not_zero required on file->f_count?
					result_name = d_path(&(open_file->f_path), result_name_buffer, OFS_RESULT_NAME_MAX_LENGTH);	

					if (IS_ERR(result_name)) {
						printk(KERN_WARNING "ofs: Failed to get name for open file. Error code: %ld\n", PTR_ERR(result_name));
						continue;
					}
					strncpy(result->name, result_name, OFS_RESULT_NAME_MAX_LENGTH);

					inode = open_file->f_inode;
					result->permissions = inode->i_mode;
					result->uid = uid;
					result->owner = (inode->i_uid).val; // TODO use uid_t from_kuid(user_namespace, kuid_t) instead
					result->fsize = inode->i_size;
					result->inode_no = inode->i_ino;
					if (filter(result, filter_arg)) {
						printk(KERN_DEBUG "ofs: Result#%u: name=%s\npermissions=%u, owner=%u, fsize=%u, inode_no=%lu\n",
								result_count_, result->name, result->permissions, result->owner, result->fsize, result->inode_no);
						result_count_++;
					}
				} else {
					printk(KERN_WARNING "ofs: Failed to query struct file with index %u\n", open_fd_index);
				}
			}
		}
	}

	// Unlock read access to fdtable
	rcu_read_unlock();

	kfree(result_name_buffer);
}

static long ofs_find_files_opened_by_process(pid_t requested_pid) {
	struct pid *pid;
	struct task_struct *task;

	printk(KERN_INFO "ofs: Searching for open files of task %d\n", requested_pid);

	if (!(pid = find_vpid(requested_pid))
			|| !(task = pid_task(pid, PIDTYPE_PID))) {
		printk(KERN_INFO "ofs: PID %d not found\n", requested_pid);
		return -EINVAL;
	}

	result_count_ = 0;
	ofs_find_open_files_of_task(task, &ofs_no_filter, NULL);
	search_performed_ = 1;

	printk(KERN_INFO "ofs: %d results found\n", result_count_);
	return 0;
}

static void ofs_find_open_files_of_tasks(struct task_struct *task, ofs_result_filter filter, void *filter_arg) {
	struct list_head *cursor;
	struct task_struct *child;

	printk(KERN_DEBUG "ofs: open files for pid %d\n", task->pid);
	ofs_find_open_files_of_task(task, filter, filter_arg);
	list_for_each(cursor, &(task->children)) {
		child = list_entry(cursor, struct task_struct, sibling);
		ofs_find_open_files_of_tasks(child, filter, filter_arg);
	}
}

static long ofs_find_files_opened_by_user(unsigned int uid) {
	printk(KERN_INFO "ofs: Searching for open files of user %u\n", uid);

	// init_task is actually the process with pid 0 (the scheduler/swapper process)
	// it is scheduled when a processor is idle... and its the father of all tasks
	// --> use it as entry point for traversing all tasks
	result_count_ = 0;
	ofs_find_open_files_of_tasks(&init_task, &ofs_filter_by_uid, (void *) &uid);
	search_performed_ = 1;

	printk(KERN_INFO "ofs: %d results found\n", result_count_);
	return 0;
}

static long ofs_find_open_files_owned_by_user(unsigned int owner) {
	printk(KERN_INFO "ofs: Searching for open files owned by user %u\n", owner);

	result_count_ = 0;
	ofs_find_open_files_of_tasks(&init_task, &ofs_filter_by_owner, (void *) &owner);
	search_performed_ = 1;

	printk(KERN_INFO "ofs: %d results found\n", result_count_); 
	return 0;
}

static long ofs_find_open_files_with_name(char *filename) {
	printk(KERN_INFO "ofs: Searching for open files named %s\n", filename);

	result_count_ = 0;
	ofs_find_open_files_of_tasks(&init_task, &ofs_filter_by_name, (void *) filename);
	search_performed_ = 1;

	printk(KERN_INFO "ofs: %d results found\n", result_count_);
	return 0;
}

static long ofs_ioctl(struct file *flip, unsigned int ioctl_cmd, unsigned long ioctl_arg) {
	switch (ioctl_cmd) {
		case OFS_PID:
			return ofs_find_files_opened_by_process(*(pid_t *) ioctl_arg);
		case OFS_UID:
			return ofs_find_files_opened_by_user(*(unsigned int *) ioctl_arg);
		case OFS_OWNER:
			return ofs_find_open_files_owned_by_user(*(unsigned int *) ioctl_arg);
		case OFS_NAME:
			return ofs_find_open_files_with_name((char *) ioctl_arg);
		default:
			printk(KERN_INFO "ofs: Unknown search command %u\n", ioctl_cmd);
			return -EINVAL;
	}
}

static inline unsigned int ofs_min(unsigned int a, unsigned int b) {
	return a <= b ? a : b;
}

// ssize_t = long int, size_t = unsigned long, loff_t = long long 
static ssize_t ofs_read(struct file *flip, char __user *buffer, size_t length, loff_t *offset) {
	unsigned int result_count;

	printk(KERN_INFO "ofs: Read request for %lu results\n", length);
	// require a prior call of ioctl
	if (!search_performed_) {
		printk(KERN_INFO "ofs: Search has not been performed yet or all results have already been read\n");
		return -ESRCH;
	}

	// truncate request to a maximum of 256 results
	// TODO use offset
	result_count = ofs_min(length, ofs_min(result_count_, OFS_MAX_RESULTS));

	copy_to_user(buffer, results_, result_count * sizeof(struct ofs_result));

	// clear search flag and result counter
	search_performed_ = 0;
	result_count_ = 0;

	printk(KERN_INFO "ofs: %d results have been available\n", result_count);
	return result_count;
}

static int ofs_release(struct inode *inode, struct file *flip) {
	device_opened_ = 0;
	printk(KERN_INFO "ofs: Released\n");
	return 0;
}

static struct file_operations fops = {
	.owner = THIS_MODULE, // https://stackoverflow.com/a/6079839/1948906
	.open = ofs_open,
	.unlocked_ioctl = ofs_ioctl,
	.read = ofs_read,
	.release = ofs_release
};

static int __init ofs_init(void) {
	// allocate normal kernel ram for storing the results in
	if (!(results_ = kmalloc(OFS_MAX_RESULTS * sizeof(struct ofs_result), GFP_KERNEL))) {
		printk(KERN_ERR "ofs: Failed to allocate memory for results\n");
		return -1;
	}

	// major = 0 --> dynamically allocate major number should be returned
	// name --> name of device in /proc/devices
	// fops --> supported file operations
	major_num_ = register_chrdev(0, MODULE_NAME, &fops);
	if (major_num_ < 0) {
		printk(KERN_ERR "ofs: Failed to register as character device: error %d\n", major_num_);
		return -1;
	}

	printk(KERN_INFO "ofs: Registered as character device under major number %d.\n" \
			"     Create special file via\n" \
			"     mknod /dev/openFileSearchDev c %d 0\n", major_num_, major_num_);
	return 0;
}

static void __exit ofs_exit(void) {
	kfree(results_);

	// return type of unregister_chrdev was changed to void as it always returned 0 (always succeeds)
	unregister_chrdev(major_num_, MODULE_NAME);
	printk(KERN_INFO "ofs: Unregistered character device with major number %d\n", major_num_);
}

module_init(ofs_init);
module_exit(ofs_exit);

