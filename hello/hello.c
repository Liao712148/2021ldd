#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/moduleparam.h>
MODULE_LICENSE("Dual BDE/GPL");
static char *whom = "world";
static int howmany = 1;
module_param(howmany, int, S_IRUGO);
module_param(whom, charp, S_IRUGO);
static int hello_init(void) {
    printk(KERN_ALERT "Hello world\n");
    printk(KERN_ALERT "The init process is \"%s\" (pid %i)\n", current->comm, current->pid);
    int i;
    for(i = 0; i < howmany; i++) {
	    printk(KERN_ALERT "%s %d times\n", whom, howmany);
    }
    return 0;
}
static void hello_exit(void) {
    printk(KERN_ALERT "Goodbye, curel world\n");
    printk(KERN_ALERT "The exit process is \"%s\" (pid %i)\n", current->comm, current->pid);
}

module_init(hello_init);
module_exit(hello_exit);
