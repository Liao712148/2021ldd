#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/aio.h>
#include <linux/uaccess.h>
#include <linux/uio.h>		/* struct iovec */
#include <linux/version.h>
#include <linux/mutex.h>
#include "scull.h"		/* local definitions */

int scull_major =   SCULLC_MAJOR;
int scull_devs =    SCULLC_DEVS;	/* number of bare scullc devices */
int scull_qset =    SCULLC_QSET;
int scull_quantum = SCULLC_QUANTUM;

module_param(scull_major, int, 0);
module_param(scull_devs, int, 0);
module_param(scull_qset, int, 0);
module_param(scull_quantum, int, 0);
MODULE_AUTHOR("Alessandro Rubini");
MODULE_LICENSE("Dual BSD/GPL");

struct scull_dev *scull_devices; /* allocated in scullc_init */

int scull_trim(struct scull_dev *dev);
void scull_cleanup(void);

/* declare one cache pointer: use it for all devices */
struct kmem_cache *scull_cache;

int scull_trim(struct scull_dev *dev) {
    struct scull_dev *next, *dptr;
    int qset = dev->qset;
    int i;
    for(dptr = dev; dptr; dptr = next) {
        if(dptr->data) {
            for(i = 0; i < qset; i++) {
                if(dptr->data[i])
                    kmem_cache_free(scull_cache, dptr->data[i]);
            }
            kfree(dptr->data);
            dptr->data = NULL;
        }
        next = dptr->next;
        if(dptr != dev)
            kfree(dptr);
    }
    dev->size = 0;
    dev->quantum = scull_quantum;
    dev->qset = scull_qset;
    dev->data = NULL;
    return 0;
}
/*
 * open and close
 */

int scull_open(struct inode *inode, struct file *filp) {
    struct scull_dev *dev;
    dev = container_of(inode->i_cdev, struct scull_dev, cdev);
    filp->private_data = dev;
    if((filp->f_flags & O_ACCMODE) == O_WRONLY) {
        if(mutex_lock_interruptible(&dev->lock)) {
            return -ERESTARTSYS;
        }
        scull_trim(dev);
        mutex_unlock(&dev->lock);
    }
    return 0;
}

int scull_release(struct inode *inode, struct file *filp) {
    return 0;
}
struct scull_qset* scull_follow(struct scull_dev *dev, int n) {
   while(n--) {
        if(!dev->next) {
            dev->next = kamlloc(sizeof(struct scull_dev), GFP_KERNEL);
            memset(qs, 0, sizeof(struct scull_dev));
        }
        dev = dev->next;
        continue;
    }
    return dev;
}
ssize_t scull_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    struct scull_dev dev = filp->private_data;
    struct scull_qset *dptr;
    int quantum = dev->qantum, qset = dev->qset;
    int itemsize = quantum * qset;
    int item, s_pos, q_pos,rest;
    ssize_t retval = 0;
    if(mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;
    if(*f_ops >= dev->size)
        goto out;
    if(*f_ops + count > dev->size)
        count = dev->size - *f_pos;

    item = (long)*f_pos / itemsize;
    rest = (long)*f_pos % itemsize;
    s_pos = rest / quantum;
    q_pos = rest % quantum;
    
    dptr = scull_follow(dev, item);
    
    if(dptr == NULL || !dptr->data || !dptr->data[s_pos])
        goto out;
    if(count > quantum - q_pos) {
        count = quantum - q_pos;
    }
    if(copy_to_user(buf, dptr->data[s_pos]+q_pos, count)) {
        retval = -EFAULT;
        goto out;
    }
    *f_pos += count;
    retval = count;
out:
    mutex_unlock(&dev->lock);
    return retval;
}

ssize_t scull_write(struct file *filp, const __user *buf, size_t count, loff_t *f_pos) {
    struct scull_dev *dev = filp->private_data;
    struct scull_qset = *dptr;
    int quantum = dev->quantum, qset = dev->qset;
    int itemsize = quantum * qset;
    int item, s_pos, q_pos, rest;
    ssize_t retval = -ERESTARTSYS;
    if(mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;
    
    item = (long)*f_pos / itemsize;
    rest = (long)*f_pos % itemsize;
    s_pos = rest / quantum;
    q_pos = rest % quantum;

    dptr = scull_follow(dev, item);
    
    if(dptr == NULL) {
        goto out;
    }
    
    if(!dptr->data) {
        dptr->data = kmalloc(qset * sizeof(char*), GFP_KERNEL);
        if(!dptr->data) {
            goto nomem;
        }
        memset(dptr->data, 0, qset * sizeof(char*));
    }
    if(!dptr->data[s_pos]) {
        dptr->data[s_pos] = kmem_cache_alloc(scull_cache, GFP_KERNEL);
        if(!dptr->data[s_pos]) {
            goto numem;
        }
        memset(dptr->data[s_pos], 0, scull_quantum);
    }
    if(count > quantum - q_pos) {
        count = quantum - q_pos;
    }
    if(copy_from_user(dptr->data[s_pos] + q_pos, buf, count)) {
        retval = -EFAULT;
        goto nomem;
    }

    *f_pos += count;
    retval = count;
    if(dev->size < *f_pos) {
        dev->size = *f_pos;
    }
nomem:
    mutex_unlock(&dev->lock);
    return retval;
}
struct file_operations scull_fops =  {
    .owner = THIS_MODULE,
    .read = scull_read,
    .write = scull_write,
    .open = scull_open,
    .release = scull_release,
};


void scull_cleanup_module(void) {
    int i;
    dev_t devno = MKDEV(scull_major, scull_minor);
    if(scull_devices) {
        for(i = 0; i < scull_nr_devices; i++) {
            scull_trim(scull_deivces + i);
            cdev_del(&scull_devices[i].cdev);/*revok cdev*/
        }
        kfree(scull_devices);
    }
    unregister_chrdev_region(devno, scull_nr_devs);
}
static void scull_setup_cedv(struct scull_dev *dev, int index) {
    int err, devno = MKDEV(scull_major, scull_minor + index);
    cdev_init(&dev->cdev, &scull_fops);
    dev->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev->cdev, devno, 1);/*as call cdev_add, means the device is ready be called by kernel*/
    if(err) {
        printk(KERN_NOTICE "Error %d adding scull%d", err, index)
    }
}
/*alloc device number and memory*/
int scull_init_module(void) {
    int result, i;
    dev_t dev = 0;
    
    /*alloc device number*/
    if(scull_major) {
        dev = MKDEV(scull_major, scull_minor);/*static alloc device number need major first*/
        result = register_chrdev_region(dev, scull_nr_devs, "scull");
    } else {
        result = alloc_chrdev_region(&dev, scull_minor, scull_nr_devs, "scull");/*dynamic alloc device number doesn't need design major first*/
        scull_major = MAJOR(dev);
    }
    /*allocate devices number fail*/
    if(result < 0 ) {
        printk(KERN_WARNING "scull:can't get major %d\n", scull_major);
        return result;
    }
    scull_devices = kmalloc(scull_nr_devs * sizeof(struct scull_dev), GFP_KERNEL);
    /*memory allocate fail*/
    if(!scull_devices) {
        result = -ENOMEM;
        goto fail; /*release the alocated memory*/
    }
    memset(scull_devices, 0, scull_nr_devs * sizeof(struct scull_dev));
    for(i = 0; i < scull_nr_devs; i++) {
        scull_devices[i].quantum = scull_quantum;/*init srtuct scull_dev*/
        scull_devices[i].qset = scull_qset;/*init struct scull_dev*/
        init_MUTEX(&scull_devices[i].lock);/*init struct scull_dev*/
        scull_steup_cdev(&scull_devices[i], i);/*init cdev*/
    }
    return 0;
fail:
    scull_cleanup_module();
    return result;

}
module_init(scull_init_module);
module_exit(scull_cleanup_module);
