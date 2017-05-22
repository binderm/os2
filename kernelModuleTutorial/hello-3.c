#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_AUTHOR("Marcel Binder <binder4@hm.edu>");
MODULE_LICENSE("GPL");

static int a_number __initdata = 42;

static int __init set_up(void) {
	printk(KERN_INFO "Hello world!\na_number=%d\n", a_number);
	return 0;
}

static void __exit tear_down(void) {
	printk(KERN_INFO "Goodbye world!\n");
}

module_init(set_up);
module_exit(tear_down);
