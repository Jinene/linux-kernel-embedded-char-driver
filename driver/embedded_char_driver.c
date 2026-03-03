#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include "embedded_char_driver.h"

MODULE_LICENSE("MIT");
MODULE_AUTHOR("Jinene Bensaid");
MODULE_DESCRIPTION("Professional Embedded Linux Character Driver");
MODULE_VERSION("1.0");

static dev_t dev_number;
static struct class *embedded_class;
static struct cdev embedded_cdev;
static struct device *embedded_device;

static char *device_buffer;
static DEFINE_MUTEX(device_mutex);

static int irq_number = 1;  // Example: keyboard IRQ (for demo only)
static int irq_counter = 0;
static spinlock_t irq_lock;

static wait_queue_head_t wait_queue;
static struct task_struct *kernel_thread;

/* ================= File Operations ================= */

static int device_open(struct inode *inode, struct file *file)
{
    pr_info("Device opened\n");
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    pr_info("Device closed\n");
    return 0;
}

static ssize_t device_read(struct file *file, char __user *user_buffer,
                           size_t len, loff_t *offset)
{
    ssize_t bytes_read = 0;

    if (*offset >= BUFFER_SIZE)
        return 0;

    if (mutex_lock_interruptible(&device_mutex))
        return -ERESTARTSYS;

    if (copy_to_user(user_buffer, device_buffer, BUFFER_SIZE))
    {
        mutex_unlock(&device_mutex);
        return -EFAULT;
    }

    bytes_read = BUFFER_SIZE;
    mutex_unlock(&device_mutex);
    return bytes_read;
}

static ssize_t device_write(struct file *file, const char __user *user_buffer,
                            size_t len, loff_t *offset)
{
    if (len > BUFFER_SIZE)
        len = BUFFER_SIZE;

    if (mutex_lock_interruptible(&device_mutex))
        return -ERESTARTSYS;

    if (copy_from_user(device_buffer, user_buffer, len))
    {
        mutex_unlock(&device_mutex);
        return -EFAULT;
    }

    mutex_unlock(&device_mutex);
    return len;
}

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd)
    {
        case IOCTL_RESET_COUNTER:
            spin_lock(&irq_lock);
            irq_counter = 0;
            spin_unlock(&irq_lock);
            break;

        case IOCTL_GET_COUNTER:
            if (copy_to_user((int __user *)arg, &irq_counter, sizeof(int)))
                return -EFAULT;
            break;

        default:
            return -EINVAL;
    }
    return 0;
}

static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open = device_open,
    .read = device_read,
    .write = device_write,
    .release = device_release,
    .unlocked_ioctl = device_ioctl,
};

/* ================= Interrupt Handler ================= */

static irqreturn_t irq_handler(int irq, void *dev_id)
{
    spin_lock(&irq_lock);
    irq_counter++;
    spin_unlock(&irq_lock);

    wake_up_interruptible(&wait_queue);

    return IRQ_HANDLED;
}

/* ================= Kernel Thread ================= */

static int thread_function(void *data)
{
    while (!kthread_should_stop())
    {
        wait_event_interruptible(wait_queue, irq_counter > 0);
        pr_info("Interrupt occurred. Count = %d\n", irq_counter);
        msleep(1000);
    }
    return 0;
}

/* ================= Sysfs Attribute ================= */

static ssize_t irq_show(struct device *dev,
                        struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", irq_counter);
}

static DEVICE_ATTR_RO(irq);

/* ================= Module Init ================= */

static int __init embedded_init(void)
{
    int ret;

    spin_lock_init(&irq_lock);
    init_waitqueue_head(&wait_queue);

    device_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!device_buffer)
        return -ENOMEM;

    mutex_init(&device_mutex);

    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0)
        goto fail_alloc;

    cdev_init(&embedded_cdev, &fops);
    ret = cdev_add(&embedded_cdev, dev_number, 1);
    if (ret)
        goto fail_cdev;

    embedded_class = class_create(CLASS_NAME);
    if (IS_ERR(embedded_class))
        goto fail_class;

    embedded_device = device_create(embedded_class, NULL,
                                    dev_number, NULL, DEVICE_NAME);
    if (IS_ERR(embedded_device))
        goto fail_device;

    device_create_file(embedded_device, &dev_attr_irq);

    ret = request_irq(irq_number, irq_handler,
                      IRQF_SHARED, DEVICE_NAME, &embedded_cdev);
    if (ret)
        goto fail_irq;

    kernel_thread = kthread_run(thread_function, NULL, "embedded_thread");

    pr_info("Embedded driver loaded\n");
    return 0;

fail_irq:
    device_destroy(embedded_class, dev_number);
fail_device:
    class_destroy(embedded_class);
fail_class:
    cdev_del(&embedded_cdev);
fail_cdev:
    unregister_chrdev_region(dev_number, 1);
fail_alloc:
    kfree(device_buffer);
    return -1;
}

/* ================= Module Exit ================= */

static void __exit embedded_exit(void)
{
    kthread_stop(kernel_thread);
    free_irq(irq_number, &embedded_cdev);
    device_remove_file(embedded_device, &dev_attr_irq);
    device_destroy(embedded_class, dev_number);
    class_destroy(embedded_class);
    cdev_del(&embedded_cdev);
    unregister_chrdev_region(dev_number, 1);
    kfree(device_buffer);

    pr_info("Embedded driver unloaded\n");
}

module_init(embedded_init);
module_exit(embedded_exit);
