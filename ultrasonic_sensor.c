#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/io.h>
#include <linux/uaccess.h>

#include "ultrasonic_sensor.h"

#define ULTRASONIC_BASE_PHYS  0xFF300000  /* adjust to your Qsys base */
#define ULTRASONIC_REG_SIZE   0x4         /* one 32-bit register */

static void __iomem *us_base;

static long ultrasonic_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    __u32 val;

    switch (cmd) {
    case US_WRITE_CONFIG:
        if (copy_from_user(&val, (void __user *)arg, sizeof(val)))
            return -EFAULT;
        iowrite32(val, us_base);
        return 0;

    case US_READ_STATUS:
        val = ioread32(us_base);
        if (copy_to_user((void __user *)arg, &val, sizeof(val)))
            return -EFAULT;
        return 0;

    default:
        return -ENOTTY;
    }
}

static const struct file_operations ultrasonic_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = ultrasonic_ioctl,
    .compat_ioctl   = ultrasonic_ioctl,
};

static struct miscdevice ultrasonic_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = ULTRASONIC_DEV_NAME,
    .fops  = &ultrasonic_fops,
};

static int __init ultrasonic_init(void)
{
    if (!request_mem_region(ULTRASONIC_BASE_PHYS, ULTRASONIC_REG_SIZE, ULTRASONIC_DEV_NAME)) {
        pr_err("ultrasonic: mem region busy\n");
        return -EBUSY;
    }

    us_base = ioremap(ULTRASONIC_BASE_PHYS, ULTRASONIC_REG_SIZE);
    if (!us_base) {
        release_mem_region(ULTRASONIC_BASE_PHYS, ULTRASONIC_REG_SIZE);
        pr_err("ultrasonic: ioremap failed\n");
        return -ENOMEM;
    }

    return misc_register(&ultrasonic_dev);
}

static void __exit ultrasonic_exit(void)
{
    misc_deregister(&ultrasonic_dev);
    iounmap(us_base);
    release_mem_region(ULTRASONIC_BASE_PHYS, ULTRASONIC_REG_SIZE);
}

module_init(ultrasonic_init);
module_exit(ultrasonic_exit);
