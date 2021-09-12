#include <linux/ioctl.h>
#include <linux/cdev.h>

#define SCULLC_MAJOR 0   /* dynamic major by default */

#define SCULLC_DEVS 4    /* scullc0 through scullc3 */

#define SCULLC_QUANTUM  4000 /* use a quantum size like scull */
#define SCULLC_QSET     500

struct scullc_dev {
	void **data;
	struct scullc_dev *next;  /* next listitem */
	int quantum;              /* the current allocation size */
	int qset;                 /* the current array size */
	size_t size;              /* 32-bit will suffice */
	struct mutex lock;     /* Mutual exclusion */
	struct cdev cdev;
};

extern struct scullc_dev *scullc_devices;

extern struct file_operations scullc_fops;

/*
 * The different configurable parameters
 */
extern int scullc_major;     /* main.c */
extern int scullc_devs;
extern int scullc_order;
extern int scullc_qset;

/*
 * Prototypes for shared functions
 */
int scullc_trim(struct scullc_dev *dev);
struct scullc_dev *scullc_follow(struct scullc_dev *dev, int n);


