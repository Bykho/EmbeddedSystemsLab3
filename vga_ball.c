/* * Device driver for the VGA video generator
 *
 * A Platform device implemented using the misc subsystem
 *
 * Stephen A. Edwards
 * Columbia University
 *
 * References:
 * Linux source: Documentation/driver-model/platform.txt
 *               drivers/misc/arm-charlcd.c
 * http://www.linuxforu.com/tag/linux-device-drivers/
 * http://free-electrons.com/docs/
 *
 * "make" to build
 * insmod vga_ball.ko
 *
 * Check code style with
 * checkpatch.pl --file --no-tree vga_ball.c
 */

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
#include "vga_ball.h"

#define DRIVER_NAME "vga_ball"

/* Device registers */
#define BALL_X(x) (x)
#define BALL_Y(x) ((x)+4)
#define LINE_MATRIX_BASE(x) ((x) + 8)
/*
 * Information about our device
 */
struct vga_ball_dev {
	struct resource res; /* Resource: our registers */
	void __iomem *virtbase; /* Where registers can be accessed in memory */
    vga_ball_position_t position;
	int LineMatrix[256][2];
} dev;


static void read_line_matrix(int out[256][2])
{
  int i;
  for (i = 0; i < 256; i++) {
    /* each entry is two 32-bit words, at offsets 8..2055 */
    out[i][0] = ioread32(LINE_MATRIX_BASE(dev.virtbase) + (i * 8)    );
    out[i][1] = ioread32(LINE_MATRIX_BASE(dev.virtbase) + (i * 8) + 4);
  }
}

/*
 * Write segments of a single digit
 * Assumes digit is in range and the device information has been set up
 */
static void write_position(vga_ball_position_t *position)
{
	iowrite32(position->x, BALL_X(dev.virtbase)); // dev.virtbase gets the location of the registers in hardware.
	iowrite32(position->y, BALL_Y(dev.virtbase));
	
	dev.position = *position;
}

// Add a new function to write the line matrix
static void write_line_matrix(int LineMatrix[256][2])
{
    // You need to define a register offset for the LineMatrix data
    // For example, if your hardware has a register at offset 8 for line data:
    
    // Write each value to hardware
	int i;
    for (i = 0; i < 256; i++) {
        // Assuming each line entry takes 8 bytes (4 for each value)
        iowrite32(LineMatrix[i][0], LINE_MATRIX_BASE(dev.virtbase) + (i * 8));
        iowrite32(LineMatrix[i][1], LINE_MATRIX_BASE(dev.virtbase) + (i * 8) + 4);
    }
    // Store the line matrix in our device struct
    memcpy(dev.LineMatrix, LineMatrix, sizeof(dev.LineMatrix));
}

/*
 * Handle ioctl() calls from userspace:
 * Read or write the segments on single digits.
 * Note extensive error checking of arguments
 */
static long vga_ball_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	vga_ball_arg_t vla;
	vga_ball_line_t vla_line;
	
	switch (cmd) {
	case VGA_BALL_WRITE_POSITION:
		if (copy_from_user(&vla, (vga_ball_arg_t *) arg, sizeof(vga_ball_arg_t)))
			return -EACCES;
		write_position(&vla.position);
		break;

	case VGA_BALL_READ_POSITION:
	  	vla.position = dev.position;
        if (copy_to_user((vga_ball_arg_t *) arg, &vla, sizeof(vga_ball_arg_t)))
			return -EACCES;
		break;

	case VGA_BALL_WRITE_LINE:
		if (copy_from_user(&vla_line, (vga_ball_line_t *) arg, sizeof(vga_ball_line_t)))
			return -EACCES;
		write_line_matrix(vla_line.LineMatrix);
		break;

    case VGA_BALL_READ_LINE:  /* new */
      /* pull live values out of the hardware into vla_line.LineMatrix */
      read_line_matrix(vla_line.LineMatrix);
      if (copy_to_user((vga_ball_line_t *)arg, &vla_line, sizeof(vla_line)))
        return -EACCES;
      break;

	default:
		return -EINVAL;
	}

	return 0;
}

/* The operations our device knows how to do */
static const struct file_operations vga_ball_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl = vga_ball_ioctl,
};

/* Information about our device for the "misc" framework -- like a char dev */
static struct miscdevice vga_ball_misc_device = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= DRIVER_NAME,
	.fops		= &vga_ball_fops,
};

/*
 * Initialization code: get resources (registers) and display
 * a welcome message
 */
static int __init vga_ball_probe(struct platform_device *pdev)
{
    vga_ball_position_t initial_position = { 50, 50 };
    int ret;

	/* Register ourselves as a misc device: creates /dev/vga_ball */
	ret = misc_register(&vga_ball_misc_device);

	/* Get the address of our registers from the device tree */
	ret = of_address_to_resource(pdev->dev.of_node, 0, &dev.res);
	if (ret) {
		ret = -ENOENT;
		goto out_deregister;
	}

	/* Make sure we can use these registers */
	printk(KERN_INFO "vga dev.res.start: %d, resource size: %d...\n", (int)dev.res.start, (int)resource_size(&dev.res));
	if (request_mem_region(dev.res.start, resource_size(&dev.res),
			       DRIVER_NAME) == NULL) {
		ret = -EBUSY;
		goto out_deregister;
	}

	/* Arrange access to our registers */
	dev.virtbase = of_iomap(pdev->dev.of_node, 0);
	if (dev.virtbase == NULL) {
		ret = -ENOMEM;
		goto out_release_mem_region;
	}
        
	/* Set an initial color */
    write_position(&initial_position);

	return 0;

out_release_mem_region:
	release_mem_region(dev.res.start, resource_size(&dev.res));
out_deregister:
	misc_deregister(&vga_ball_misc_device);
	return ret;
}

/* Clean-up code: release resources */
static int vga_ball_remove(struct platform_device *pdev)
{
	iounmap(dev.virtbase);
	release_mem_region(dev.res.start, resource_size(&dev.res));
	misc_deregister(&vga_ball_misc_device);
	return 0;
}

/* Which "compatible" string(s) to search for in the Device Tree */
#ifdef CONFIG_OF
static const struct of_device_id vga_ball_of_match[] = {
	{ .compatible = "csee4840,vga_ball-1.0" },
	{},
};
MODULE_DEVICE_TABLE(of, vga_ball_of_match);
#endif

/* Information for registering ourselves as a "platform" driver */
static struct platform_driver vga_ball_driver = {
	.driver	= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(vga_ball_of_match),
	},
	.remove	= __exit_p(vga_ball_remove),
};

/* Called when the module is loaded: set things up */
static int __init vga_ball_init(void)
{
	pr_info(DRIVER_NAME ": init\n");
	return platform_driver_probe(&vga_ball_driver, vga_ball_probe);
}

/* Calball when the module is unloaded: release resources */
static void __exit vga_ball_exit(void)
{
	platform_driver_unregister(&vga_ball_driver);
	pr_info(DRIVER_NAME ": exit\n");
}

module_init(vga_ball_init);
module_exit(vga_ball_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stephen A. Edwards, Columbia University");
MODULE_DESCRIPTION("VGA ball driver");
