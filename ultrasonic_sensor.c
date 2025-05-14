#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "ultrasonic_sensor.h"

#define ULTRASONIC_BASE_PHYS  0xFF300000  /* adjust to your Qsys base */
#define ULTRASONIC_REG_SIZE   0x4         /* one 32-bit register */
#define DRIVER_NAME "ultrasonic_sensor"
#define STATUS(x) (x + 4)

//static void __iomem *us_base;

struct ultrasonic_sensor_dev {
    struct resource res; /* Resource: our registers */
	void __iomem *virtbase; /* Where registers can be accessed in memory */
} dev;

static void read_status(uint32_t *status)
{
    /* each entry is two 32-bit words, at offsets 8..2055 */
    // maybe add an offset? since we are working with 2 registers?
    *status = ioread32(STATUS(dev.virtbase)); // takes in an address
    printk(KERN_INFO "status%d...\n", *status);
}

static long ultrasonic_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    __u32 val;

    uint32_t value;

    switch (cmd) {
    case US_WRITE_CONFIG:
        if (copy_from_user(&val, (void __user *)arg, sizeof(val)))
            return -EFAULT;
        iowrite32(val, STATUS(dev.virtbase));
        break;
    case US_READ_STATUS:
        read_status(&value);
        //val = ioread32(us_base);
        if (copy_to_user(( uint32_t *)arg, &value, sizeof(value)))
            return -EACCES;
        break;

    default:
        return -ENOTTY;
    }
}

/* The operations our device knows how to do */
static const struct file_operations ultrasonic_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = ultrasonic_ioctl,
    .compat_ioctl   = ultrasonic_ioctl,
};

/* Information about our device for the "misc" framework -- like a char dev */
static struct miscdevice ultrasonic_sensor_misc_device = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= DRIVER_NAME,
	.fops		= &ultrasonic_fops,
};

/* Initialization code */
static int __init ultrasonic_sensor_probe(struct platform_device *pdev)
{
    int ret;
    /* Register ourselves as a misc device: creates /dev/ultrasonic_sensor */
	ret = misc_register(&ultrasonic_sensor_misc_device);

    /* Get the address of our registers from the device tree */
	ret = of_address_to_resource(pdev->dev.of_node, 0, &dev.res); // okay the index is going to have to be different for status vs for the thing we write to i think.
	if (ret) {
		ret = -ENOENT;
		goto out_deregister;
	}

    /* Make sure we can use these registers */
    printk(KERN_INFO "sensor: dev.res.start: %d, resource size: %d...\n", (int)dev.res.start, (int)resource_size(&dev.res));
    if (!request_mem_region(dev.res.start, resource_size(&dev.res), ULTRASONIC_DEV_NAME)) {
        pr_err("ultrasonic: mem region busy\n");
        return -EBUSY;
        goto out_deregister;
    }

    /* Arrange access to our registers */
    dev.virtbase = of_iomap(pdev->dev.of_node, 0); // index of the i/o range? i don't know 
    if (dev.virtbase == NULL) {
		ret = -ENOMEM;
		goto out_release_mem_region;
	}

    return 0;
out_release_mem_region:
	release_mem_region(dev.res.start, resource_size(&dev.res));
out_deregister:
	misc_deregister(&ultrasonic_sensor_misc_device);
	return ret;
}

/* Clean-up code: release resources */
static int ultrasonic_sensor_remove(struct platform_device *pdev)
{
	iounmap(dev.virtbase);
	release_mem_region(dev.res.start, resource_size(&dev.res));
	misc_deregister(&ultrasonic_sensor_misc_device);
	return 0;
}

/* Which "compatible" string(s) to search for in the Device Tree */
#ifdef CONFIG_OF
static const struct of_device_id ultrasonic_sensor_of_match[] = {
	{ .compatible = "csee4840,ultrasonic_sensor-1.0" },
	{},
};
MODULE_DEVICE_TABLE(of, ultrasonic_sensor_of_match);
#endif

/* Information for registering ourselves as a "platform" driver */
static struct platform_driver ultrasonic_sensor_driver = {
	.driver	= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(ultrasonic_sensor_of_match),
	},
	.remove	= __exit_p(ultrasonic_sensor_remove),
};

static int __init ultrasonic_init(void)
{   
    pr_info(DRIVER_NAME ": init\n");
    return platform_driver_probe(&ultrasonic_sensor_driver, ultrasonic_sensor_probe);
}

static void __exit ultrasonic_exit(void)
{
    platform_driver_unregister(&ultrasonic_sensor_driver);
	pr_info(DRIVER_NAME ": exit\n");
}

module_init(ultrasonic_init);
module_exit(ultrasonic_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("als2408 nb3227, Columbia University");
MODULE_DESCRIPTION("ultrasonic driver");
