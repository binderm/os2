#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/fdtable.h>
#include <linux/dcache.h>
#include "ofs.h"
#define MODULE_NAME "ofs"

MODULE_AUTHOR("Marcel Binder <binder4@hm.edu");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("...");

static int major_num;
static int device_opened = 0;
//static struct ofs_result results[OFS_MAX_RESULTS];
static int search_performed = 1;

static int ofs_open(struct inode *inode, struct file *flip) {
	if (device_opened) {
		printk(KERN_WARNING "ofs: Driver already in use\n");
		return -EBUSY;
	}
	device_opened = 1;
	printk(KERN_INFO "ofs: Opened\n");
	return 0;
}

static long ofs_find_files_opened_by_process(pid_t requested_pid) {
	struct pid *p;
	pid_t pid;
	struct task_struct *task;
	struct files_struct *files;
	struct fdtable *fdt;
	unsigned int fd_capacity;
	unsigned int fds_length;
	unsigned int result_index;
	unsigned int fds_section_index;
	unsigned long open_fds_section;
	unsigned int fds_bit_index;
	unsigned int open_fd_index;
	struct file *open_file;
	char result_name_buffer[OFS_RESULT_NAME_MAX_LENGTH];
	char *result_name = result_name_buffer;
	struct inode *inode;
	umode_t result_permissions; // umode_t = unsigned int
	uid_t result_owner; // uid_t = unsigned short
	loff_t result_fsize; // loff_t = long long int (TODO convert to unsigned int for struct ofs_result)
	unsigned long result_inode_no;
	
	printk(KERN_INFO "ofs: Find open files of process %u\n", requsted_pid);
	// wrap numberic pid to struct pid --> new struct pid is allocated when pid is reused (wrap around) --> safer reference
	p = find_get_pid(requested_pid);

	task = get_pid_task(p, PIDTYPE_PID);
	if (!task) {
		printk(KERN_WARNING "ofs: Failed to get task_stuct for pid %u. Might not exist.\n", pid);
		return -EINVAL;
	}
	pid = task->pid;
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
	result_index = 0;
	for (fds_section_index = 0; fds_section_index < fds_length; fds_section_index++) {
		open_fds_section = fdt->open_fds[fds_section_index];
		printk(KERN_DEBUG "ofs: open_fds[%u]=%lu\n", fds_section_index, open_fds_section);
		
		for (fds_bit_index = 0; fds_bit_index < BITS_PER_LONG; fds_bit_index++) {
			if (open_fds_section & (1UL << fds_bit_index)) {
				open_fd_index = fds_section_index + fds_bit_index;
				
				// for getting a struct file from the fd array fcheck_files MUST be used holding the RCU read lock (see implementation: the last parameter is the index for the fd array)
				open_file = fcheck_files(files, open_fd_index);
				if (open_file) {
					// TODO is atomic_long_inc_not_zero required on file->f_count?
					result_name = d_path(&(open_file->f_path), result_name_buffer, OFS_RESULT_NAME_MAX_LENGTH);
					// TODO check if error occurred (name too long)
					inode = open_file->f_inode;
					result_permissions = inode->i_mode;
					result_owner = (inode->i_uid).val; // TODO use uid_t from_kuid(user_namespace, kuid_t) instead
					result_fsize = inode->i_size;
					result_inode_no = inode->i_ino;
					printk(KERN_DEBUG "ofs: Result#%u: name=%s\npermissions=%u, owner=%u, fsize=%lld, inode_no=%lu\n",
						result_index, result_name, result_permissions, result_owner, result_fsize, result_inode_no);
					result_index++;
				} else {
					printk(KERN_WARNING "ofs: Failed to query struct file with index %u\n", open_fd_index);
				}
			}
		}
	}

	// Unlock read access to fdtable
	rcu_read_unlock();

	return 0;
}

static long ofs_find_files_opened_by_user(unsigned int uid) {
	printk(KERN_INFO "ofs: Find all open files of user %u\n", uid);
	return 0;
}

static long ofs_find_open_files_owned_by_user(unsigned int uid) {
	printk(KERN_INFO "ofs: Find all open files owned by user %u\n", uid);
	return 0;
}

static long ofs_find_open_files_with_name(char *filename) {
	printk(KERN_INFO "ofs: Find all open files with name %s\n", filename);
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
			printk(KERN_WARNING "Unknown ioctl command %u\n", ioctl_cmd);
			return -EINVAL;
	}
	return 0;
}

static ssize_t ofs_read(struct file *flip, char __user *buffer, size_t length, loff_t *offset) {
	// truncate request to a maximum of 256 results
	ssize_t requested_result_count = length > OFS_MAX_RESULTS
		? OFS_MAX_RESULTS
		: length;
	
	printk(KERN_INFO "ofs: Read %ld results\n", requested_result_count);

	// require a prior call of ioctl
	if (!search_performed) {
		return -ESRCH;
	}

	return requested_result_count;
}

static int ofs_release(struct inode *inode, struct file *flip) {
	device_opened = 0;
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
	// major = 0 --> dynamically allocate major number should be returned
	// name --> name of device in /proc/devices
	// fops --> supported file operations
	major_num = register_chrdev(0, MODULE_NAME, &fops);
	if (major_num < 0) {
		printk(KERN_ERR "ofs: Failed to register as character device: error %d\n", major_num);
		return -1;
	}

	printk(KERN_INFO "ofs: Registered as character device under major number %d.\n" \
			"Create special file via\n" \
			"mknod /dev/openFileSearchDev c %d 0\n", major_num, major_num);
	return 0;
}

static void __exit ofs_exit(void) {
	// return type of unregister_chrdev was changed to void as it always returned 0 (always succeeds)
	unregister_chrdev(major_num, MODULE_NAME);
	printk(KERN_INFO "ofs: Unregistered character device with major number %d\n", major_num);
}

module_init(ofs_init);
module_exit(ofs_exit);