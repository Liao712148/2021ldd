static void scull_create_proc(void)
{
	proc_create_data("scullmem", 0 /* default mode */,
			NULL /* parent dir */, proc_ops_wrapper(&scullmem_proc_ops, scullmem_pops),
			NULL /* client data */);
	proc_create("scullseq", 0, NULL, proc_ops_wrapper(&scullseq_proc_ops, scullseq_pops));
}

static void scull_remove_proc(void)
{
	/* no problem if it was not registered */
	remove_proc_entry("scullmem", NULL /* parent dir */);
	remove_proc_entry("scullseq", NULL);
}

int scull_trim(struct scull_dev *dev) {
    struct scull_qset *next, *dptr;
    int qset = dev->qset;
    int i;
    for(dptr = dev->data; dptr != NULL; dptr = next) {
        if(dptr->data) {
            for(i = 0; i < qset; i++) {
                kfree(dptr->data[i]);
            }
            kfree(dptr->data);
            dptr->data = NULL;
        }
        next = dptr->next;
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
    struct scull_qset *qs = dev->data;
    
    if(!qs) {
        dev->data = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
        qs = dev->data;
        if(qs == NULL) {
            return NULL;
        }
        memset(qs, 0, sizeof(struct scull_qset));
    }
    while(n--) {
        if(!qs->next) {
            qs->next = kamlloc(sizeof(struct scull_qset), GFP_KERNEL);
            if(qs->next == NULL) {
                return NULL;
            }
            memset(qs, 0, sizeof(struct scull_qset));
        }
        qs = qs->next;
        continue;
    }
    return qs;
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
            goto out;
        }
        memset(dptr->data, 0, qset * sizeof(char*));
    }
    if(!dptr->data[s_pos]) {
        dptr->data[s_pos] = kmalloc(quantum * sizeof(char), GFP_KERNEL);
        if(!dptr->data[s_pos]) {
            goto out;
        }
    }
    if(count > quantum - q_pos) {
        count = quantum - q_pos;
    }
    if(copy_from_user(dptr->data[s_pos] + q_pos, buf, count)) {
        retval = -EFAULT;
        goto out;
    }

    *f_pos += count;
    retval = count;
    if(dev->size < *f_pos) {
        dev->size = *f_pos;
    }
out:
    mutex_unlock(&dev->lock);
    return retval;
}

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
#ifdef SCULL_DEBUG
    scull_remove_proc();
#endif
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

#ifdef SCULL_DEBUG
    scull_create_proc();
#endif
    
    return 0;
fail:
    scull_cleanup_module();
    return result;

}
module_init(scull_init_module);
module_exit(scull_cleanup_module);
