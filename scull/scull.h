#include <linux/ioctl.h>
#include <linux/cdev.h>



#define SCULLC_MAJOR 0
#define SCULLC_DEVS 4

#define SCULLC_QUANTUM 4000
#define SCULLC_QSET 500


struct scull_dev {
    void **data;
    struct scull_dev *next;
    int quantum;
    int qset;
    unsigned long size;
    struct mutex lock;
    struct cdev cdev;
};

extern struct scull_dev *scull_devices;
extern struct file_operations scull_fops;
extern int scull_major;
extern int scull_devs;
extern int scull_order;
extern int scull_qset;

int scull_trim(struct scull_dev *dev);
struct scull_dev* scull_follow(struct scull_dev *dev, int n);
