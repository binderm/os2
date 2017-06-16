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
static int search_performed_;
static unsigned int result_count_;
static int read_position_;

static int ofs_open(struct inode *inode, struct file *flip) {
	// do not allow concurrent use (static members would cause race
	// conditions!)
	if (device_opened_) {
		printk(KERN_INFO "openFileSearch: Driver already in use\n");
		return -EBUSY;
	}
	device_opened_ = 1;

	if (!(results_ = kmalloc(OFS_MAX_RESULTS * sizeof(struct ofs_result),
					GFP_KERNEL))) {
		printk(KERN_ERR "openFileSearch: Failed to allocate memory " \
				"for results\n");
		return -1;
	}

	// discard search performed by prior caller
	search_performed_ = 0;

	printk(KERN_INFO "openFileSearch: Opened\n");
	return 0;
}

static int ofs_release(struct inode *inode, struct file *flip) {
	device_opened_ = 0;
	kfree(results_);

	printk(KERN_INFO "openFileSearch: Released\n");
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

static void ofs_found(pid_t pid, uid_t uid, struct file *open_fd,
ofs_result_filter filter, void *filter_arg) {
	struct ofs_result *result = &results_[result_count_];
	char *result_name_buffer;
	char *result_name;
	struct inode *inode;
	char *result_name_ending;

	result->pid = pid;
	result->uid = uid;
	
	if (!(result_name_buffer = kmalloc(OFS_RESULT_NAME_MAX_LENGTH * sizeof(char), GFP_KERNEL))) {
		printk(KERN_ERR "openFileSearch: Failed to allocate memory for result name\n");
		return;
	}
	// TODO is atomic_long_inc_not_zero required on file->f_count?
	result_name = d_path(&(open_fd->f_path), result_name_buffer,
			OFS_RESULT_NAME_MAX_LENGTH);	
	if (IS_ERR(result_name) && PTR_ERR(result_name) == -ENAMETOOLONG) {
		result_name = result_name_buffer;
		result_name_ending = open_fd->f_path.dentry->d_iname;
		strcpy(result_name, ".../");
		strlcpy(&result_name[4], result_name_ending,
				OFS_RESULT_NAME_MAX_LENGTH);
		printk(KERN_INFO "openFileSearch: filename too long: .../%s\n",
				result_name_ending);
	} else if (IS_ERR(result_name)) {
		result_name = "(error)";
		printk(KERN_WARNING "openFileSearch: Failed to construct " \
				"filename (%ld)", PTR_ERR(result_name));
	}
	strlcpy(result->name, result_name, OFS_RESULT_NAME_MAX_LENGTH);
	kfree(result_name_buffer);

	inode = open_fd->f_inode;
	result->permissions = inode->i_mode;
	result->owner = (inode->i_uid).val;
	result->fsize = inode->i_size;
	result->inode_no = inode->i_ino;

	if (filter(result, filter_arg)) {
		result_count_++;
	}
}

static void ofs_search(struct task_struct *task,
		ofs_result_filter filter,
		void *filter_arg) {
	struct files_struct *files = task->files;
	struct fdtable *fdt;
	pid_t pid = task->pid;
	uid_t uid = task->cred->uid.val;
	unsigned int fd;
	unsigned int max_fds;
	struct file *open_fd;

	rcu_read_lock();
	uid = rcu_dereference(task->cred)->uid.val;
	rcu_read_unlock();

	rcu_read_lock();
	// reference to file_struct::fdt MUST be done via files_fdtable macro
	// --> ensures lock-free dereference (see <kernel>/Documentation/filesystems/files.txt)
	// (files_struct::fdt initially points to files_struct::fdtab but elsewhere after possible expansion of fdtable)
	fdt = files_fdtable(files);

	max_fds = fdt->max_fds;
	for (fd = 0; result_count_ < OFS_MAX_RESULTS && fd < max_fds; fd++) {
		if (fd_is_open(fd, fdt)) {
			rcu_read_lock();
			open_fd = rcu_dereference_raw(fdt->fd[fd]);
			ofs_found(pid, uid, open_fd, filter, filter_arg);
			rcu_read_unlock();
		}
	}
	rcu_read_unlock();
}

static void ofs_search_r(struct task_struct *task,
		ofs_result_filter filter, void *filter_arg) {
	struct list_head *cursor;
	struct task_struct *child;

	ofs_search(task, filter, filter_arg);
	list_for_each(cursor, &(task->children)) {
		child = list_entry(cursor, struct task_struct, sibling);
		ofs_search_r(child, filter, filter_arg);
	}
}

static inline void new_search(void) {
	search_performed_ = 0;
	result_count_ = 0;
	read_position_ = 0;
}

static long ofs_find_files_opened_by_process(pid_t requested_pid) {
	struct pid *pid;
	struct task_struct *task;

	printk(KERN_INFO "openFileSearch: Searching for open files of task %d\n",
			requested_pid);

	if (!(pid = find_vpid(requested_pid))
			|| !(task = pid_task(pid, PIDTYPE_PID))) {
		printk(KERN_INFO "openFileSearch: PID %d not found\n",
				requested_pid);
		return -EINVAL;
	}

	new_search();
	ofs_search(task, &ofs_no_filter, NULL);
	search_performed_ = 1;

	printk(KERN_INFO "openFileSearch: %d results found\n", result_count_);
	return 0;
}

static long ofs_find_files_opened_by_user(unsigned int uid) {
	printk(KERN_INFO "openFileSearch: Searching for open files of user %u\n",
			uid);

	// init_task is actually the process with pid 0 (the scheduler/swapper
	// process)
	// it is scheduled when a processor is idle... and its the father of all
	// tasks --> use it as entry point for traversing all tasks
	new_search();
	ofs_search_r(&init_task, &ofs_filter_by_uid, &uid);
	search_performed_ = 1;

	printk(KERN_INFO "openFileSearch: %d results found\n", result_count_);
	return 0;
}

static long ofs_find_open_files_owned_by_user(unsigned int owner) {
	printk(KERN_INFO "openFileSearch: Searching for open files owned by " \
			"user %u\n", owner);

	new_search();
	ofs_search_r(&init_task, &ofs_filter_by_owner, &owner);
	search_performed_ = 1;

	printk(KERN_INFO "openFileSearch: %d results found\n", result_count_); 
	return 0;
}

static long ofs_find_open_files_with_name(char *filename) {
	printk(KERN_INFO "openFileSearch: Searching for open files named %s\n",
			filename);

	new_search();
	ofs_search_r(&init_task, &ofs_filter_by_name, filename);
	search_performed_ = 1;

	printk(KERN_INFO "openFileSearch: %d results found\n", result_count_);
	return 0;
}

static long ofs_ioctl(struct file *flip, unsigned int ioctl_cmd,
		unsigned long ioctl_arg) {
	switch (ioctl_cmd) {
		case OFS_PID:
			return ofs_find_files_opened_by_process(
					*(pid_t *) ioctl_arg);
		case OFS_UID:
			return ofs_find_files_opened_by_user(
					*(unsigned int *) ioctl_arg);
		case OFS_OWNER:
			return ofs_find_open_files_owned_by_user(
					*(unsigned int *) ioctl_arg);
		case OFS_NAME:
			return ofs_find_open_files_with_name((char *) ioctl_arg);
		default:
			printk(KERN_INFO "openFileSearch: Unknown search " \
					"command %u\n", ioctl_cmd);
			return -EINVAL;
	}
}

static inline unsigned int ofs_min(unsigned int a, unsigned int b) {
	return a <= b ? a : b;
}

// ssize_t = long int, size_t = unsigned long, loff_t = long long 
static ssize_t ofs_read(struct file *flip, char __user *buffer,
		size_t requested_results, loff_t *offset) {
	unsigned int available_results;
	unsigned int read_results;

	printk(KERN_INFO "openFileSearch: Read request for %lu results\n",
			requested_results);

	// require a prior call of ioctl
	if (!search_performed_) {
		printk(KERN_INFO "openFileSearch: Search has not been " \
				"performed yet or all results have already " \
				"been read\n");
		return -ESRCH;
	}

	// truncate count to number of available results
	available_results = result_count_ - read_position_;	
	read_results = ofs_min(requested_results, available_results);

	if (available_results) {
		copy_to_user(buffer, &results_[read_position_],
				read_results * sizeof(struct ofs_result));
		read_position_ += read_results;
	} else {	
		// close search if no (more) results are available
		search_performed_ = 0;
	}

	printk(KERN_INFO "openFileSearch: %d/%d results read\n", read_results,
			available_results);
	return read_results;
}

static struct file_operations fops = {
	.owner = THIS_MODULE, // see https://stackoverflow.com/a/6079839/1948906
	.open = ofs_open,
	.release = ofs_release,
	.unlocked_ioctl = ofs_ioctl,
	.read = ofs_read
};

static int __init ofs_init(void) {
	// major = 0 --> use a dynamically created major number
	// name --> module name in /proc/devices
	// fops --> supported file operations
	major_num_ = register_chrdev(0, MODULE_NAME, &fops);
	if (major_num_ < 0) {
		printk(KERN_ERR "openFileSearch: Failed to register as \
				character device (%d)\n", major_num_);
		return -1;
	}

	printk(KERN_INFO "openFileSearch: Registered as character device " \
			"under major number %d.\nCreate a device file via\n" \
			"mknod /dev/openFileSearchDev c %d 0\n",
			major_num_, major_num_);
	return 0;
}

static void __exit ofs_exit(void) {
	unregister_chrdev(major_num_, MODULE_NAME);

	printk(KERN_INFO "openFileSearch: Unregistered character device with " \
			"major number %d\n", major_num_);
}

module_init(ofs_init);
module_exit(ofs_exit);

