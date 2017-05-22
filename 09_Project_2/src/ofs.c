#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include "ofs.h"

MODULE_AUTHOR("Marcel Binder <binder4@hm.edu");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("...");

static int ofs_open(struct inode *inode, struct file *flip) {
	return 0;
}

static long ofs_find_files_opened_by_process(unsigned int pid) {
	return 0;
}

static long ofs_find_files_opened_by_user(unsigned int uid) {
	return 0;
}

static long ofs_find_open_files_owned_by_user(unsigned int uid) {
	return 0;
}

static long ofs_find_open_files_with_name(char *filename) {
	return 0;
}

static long ofs_ioctl(struct file *flip, unsigned int ioctl_cmd, unsigned long ioctl_arg) {
	switch (ioctl_cmd) {
		case OFS_PID:
			return ofs_find_files_opened_by_process(*(unsigned int*) ioctl_arg);
		case OFS_UID:
			return ofs_find_files_opened_by_user(*(unsigned int *) ioctl_arg);
		case OFS_OWNER:
			return ofs_find_open_files_owned_by_user(*(unsigned int *) ioctl_arg);
		case OFS_NAME:
			return ofs_find_open_files_with_name((char *) ioctl_arg);
		default:
			printk(KERN_WARNING "Unknown ioctl command %ud\n", ioctl_cmd);
			return EINVAL;
	}
	return 0;
}

static ssize_t ofs_read(struct file *flip, char __user *buffer, size_t length, loff_t *offset) {
	return 0;
}

static int ofs_release(struct inode *inode, struct file *flip) {
	return 0;
}

static struct file_operations fops = {
	.open = ofs_open,
	.unlocked_ioctl = ofs_ioctl,
	.read = ofs_read,
	.release = ofs_release
};

static int __init ofs_init(void) {
	// major = 0 --> dynamically allocate major number should be returned
	// name --> name of device in /proc/devices
	// fops --> supported file operations
	int major_num = register_chrdev(0, "ofs", &fops);
	if (major_num < 0) {
		printk(KERN_ERR "ofs: Failed to register as character device\n");
		return -1;
	}

	printk(KERN_INFO "ofs: Registered as character device under major number %d.\n" \
			"Create special file via\n" \
			"mknod /dev/openFileSearchDev c %d\n", major_num, major_num);
	return 0;
}

static void __exit ofs_exit(void) {
	printk(KERN_ERR "ofs: Cleaned up");
}

module_init(ofs_init);
module_exit(ofs_exit);
