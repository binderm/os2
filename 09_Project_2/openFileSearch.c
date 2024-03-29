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
static bool device_opened_ = 0;
static struct ofs_result *results_;
static bool search_performed_;
static unsigned int result_count_;
static int read_position_;

static int ofs_open(struct inode *inode, struct file *flip) {
	// do not allow concurrent use (static members would cause race
	// conditions!)
	if (device_opened_) {
		printk(KERN_WARNING "openFileSearch: Driver already in use\n");
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

static void ofs_path_to_result(struct ofs_result *result, struct path *path) {
	char *name = d_path(path, result->name, OFS_RESULT_NAME_MAX_LENGTH);
	char *short_name;

	if (IS_ERR(name)) {
		if (PTR_ERR(name) == -ENAMETOOLONG) {
			short_name = path->dentry->d_iname;
			printk(KERN_WARNING "openFileSearch: filename too " \
					" long, using short version: %s\n",
					short_name);
			strlcpy(result->name, short_name,
					OFS_RESULT_NAME_MAX_LENGTH);
		} else {
			printk(KERN_ERR "Failed to build filename\n");
			strcpy(result->name, "(error)");
		}
	} else {
		strlcpy(result->name, name, OFS_RESULT_NAME_MAX_LENGTH);
	}
}

static void ofs_inode_to_result(struct ofs_result *result, struct inode *inode) {
	inode_lock_shared(inode);
	result->permissions = inode->i_mode;
	result->owner = (inode->i_uid).val;
	result->fsize = inode->i_size;
	result->inode_no = inode->i_ino;
	inode_unlock_shared(inode);
}

static void ofs_build_result(pid_t pid, uid_t uid, struct file *open_fd) {
	struct ofs_result *result = &results_[result_count_];

	result->pid = pid;
	result->uid = uid;

	ofs_path_to_result(result, &open_fd->f_path);
	ofs_inode_to_result(result, open_fd->f_inode);
}

static void ofs_filter_result(ofs_result_filter filter, void *filter_arg) {
	if (filter(&results_[result_count_], filter_arg)) {
		// include this open file in the results
		result_count_++;
	}
	// else the result will be overwritten by the next result
}

static void ofs_search(struct task_struct *task, ofs_result_filter filter,
		void *filter_arg) {
	pid_t pid = task->pid;
	uid_t uid;
	struct files_struct *files;
	struct fdtable *fdt;
	unsigned int fd;
	unsigned int max_fds;
	struct file *open_fd;

	rcu_read_lock();
	uid = rcu_dereference(task->cred)->uid.val;
	rcu_read_unlock();

	task_lock(task);
	files = task->files;

	rcu_read_lock();
	// (see <kernel>/Documentation/filesystems/files.txt)
	// (files_struct::fdt initially points to files_struct::fdtab but
	// elsewhere after possible expansion of fdtable)
	fdt = files_fdtable(files);

	max_fds = fdt->max_fds;
	for (fd = 0; result_count_ < OFS_MAX_RESULTS && fd < max_fds; fd++) {
		if (fd_is_open(fd, fdt)) {
			rcu_read_lock();
			// it's recommended to use fcheck_files() here
			// but it would check if fd < max_fds on every call
			open_fd = rcu_dereference_raw(fdt->fd[fd]);
			ofs_build_result(pid, uid, open_fd);
			ofs_filter_result(filter, filter_arg);
			rcu_read_unlock();
		}
	}
	rcu_read_unlock();
	task_unlock(task);
}

static void ofs_search_all(ofs_result_filter filter, void *filter_arg) {
	struct task_struct *task;

	for_each_process(task) {
		ofs_search(task, filter, filter_arg);
		if (result_count_ >= OFS_MAX_RESULTS) {
			return;
		}
	}
}

static void new_search(void) {
	search_performed_ = 0;
	result_count_ = 0;
	read_position_ = 0;
}

static long ofs_search_open_files_by_pid(pid_t requested_pid) {
	struct pid *pid;
	struct task_struct *task;

	printk(KERN_INFO "openFileSearch: Searching for open files of " \
			"process %d\n", requested_pid);

	if (!(pid = find_vpid(requested_pid))
			|| !(task = pid_task(pid, PIDTYPE_PID))) {
		printk(KERN_WARNING "openFileSearch: PID %d not found\n",
				requested_pid);
		return -EINVAL;
	}

	new_search();
	ofs_search(task, &ofs_no_filter, NULL);
	search_performed_ = 1;
	return 0;
}

static long ofs_search_open_files_by_uid(unsigned int uid) {
	printk(KERN_INFO "openFileSearch: Searching for open files of user %u\n",
			uid);

	new_search();
	ofs_search_all(&ofs_filter_by_uid, &uid);
	search_performed_ = 1;
	return 0;
}

static long ofs_search_open_files_by_owner(unsigned int owner) {
	printk(KERN_INFO "openFileSearch: Searching for open files owned by " \
			"user %u\n", owner);

	new_search();
	ofs_search_all(&ofs_filter_by_owner, &owner);
	search_performed_ = 1;
	return 0;
}

static long ofs_search_open_files_by_name(__user char *name) {
	char *filename;
	size_t length;

	if (!(filename = kmalloc(OFS_RESULT_NAME_MAX_LENGTH * sizeof(char),
					GFP_KERNEL))) {
		printk(KERN_ERR "openFileSearch: Failed to allocate memory " \
				" for filename\n");
		return -EIO;
	}

	if (copy_from_user(filename, name, OFS_RESULT_NAME_MAX_LENGTH
				* sizeof(char))) {
		printk(KERN_ERR "openFileSearch: Failed to copy filename to " \
				"kernel memory\n");
		return -EIO;
	}

	length = strnlen(filename, OFS_RESULT_NAME_MAX_LENGTH);
	if (length == OFS_RESULT_NAME_MAX_LENGTH) {
		// the length must not be higher than max - 1 in order to be
		// properly NULL-terminated!
		printk(KERN_WARNING "openFileSearch: Searched filename must " \
				"not be longer than %u characters " \
				"(including '\\0').\n",
				OFS_RESULT_NAME_MAX_LENGTH);
		return -EINVAL;
	}

	printk(KERN_INFO "openFileSearch: Searching for open files named %s\n",
			filename);

	new_search();
	ofs_search_all(&ofs_filter_by_name, filename);
	search_performed_ = 1;
	return 0;
}

static long ofs_ioctl(struct file *flip, unsigned int ioctl_cmd,
		unsigned long ioctl_arg) {
	long err;

	switch (ioctl_cmd) {
		case OFS_PID:
			err = ofs_search_open_files_by_pid(
					*(pid_t *) ioctl_arg);
			break;
		case OFS_UID:
			err = ofs_search_open_files_by_uid(
					*(unsigned int *) ioctl_arg);
			break;
		case OFS_OWNER:
			err = ofs_search_open_files_by_owner(
					*(unsigned int *) ioctl_arg);
			break;
		case OFS_NAME:
			err = ofs_search_open_files_by_name((char *) ioctl_arg);
			break;
		default:
			printk(KERN_WARNING "openFileSearch: Unknown search " \
					"command %u\n", ioctl_cmd);
			return -EINVAL;
	}

	if (!err) {
		printk(KERN_INFO "openFileSearch: %d results found\n",
				result_count_);
	}
	return err;
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
		printk(KERN_WARNING "openFileSearch: Search has not been " \
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

