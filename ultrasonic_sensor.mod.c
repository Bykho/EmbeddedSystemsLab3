#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = ultrasonic_init,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = ultrasonic_exit,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("misc:ultrasonic_sensor");

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#include "ultrasonic_sensor.h"

// Placeholder physical base address of the ultrasonic_sensor IP block
#define ULTRASONIC_BASE_PHYS 0xFF200000  // TODO: update to actual QSys base address
#define ULTRASONIC_REG_SIZE  0x0C        // 3 registers × 4 bytes each

// Register offsets
#define REG_CHIRP   0x00  // 16-bit write
#define REG_TIME    0x04  // 16-bit write
#define REG_STATUS  0x08  // 32-bit read

static void __iomem *ultrasonic_virt_base;

// IOCTL handler
static long ultrasonic_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    ultrasonic_chirp_t  ch;
    ultrasonic_time_t   ts;
    ultrasonic_status_t st;

    switch (cmd) {
    case US_WRITE_CHIRP:
        if (copy_from_user(&ch, (void __user *)arg, sizeof(ch)))
            return -EFAULT;
        iowrite16((u16)ch.chirp, ultrasonic_virt_base + REG_CHIRP);
        break;

    case US_WRITE_TIME:
        if (copy_from_user(&ts, (void __user *)arg, sizeof(ts)))
            return -EFAULT;
        iowrite16((u16)ts.time_signal, ultrasonic_virt_base + REG_TIME);
        break;

    case US_READ_STATUS:
        st.status = ioread32(ultrasonic_virt_base + REG_STATUS);
        if (copy_to_user((void __user *)arg, &st, sizeof(st)))
            return -EFAULT;
        break;

    default:
        return -ENOTTY;
    }

    return 0;
}

static const struct file_operations ultrasonic_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = ultrasonic_ioctl,
    .llseek         = no_llseek,
};

static struct miscdevice ultrasonic_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "ultrasonic_sensor",
    .fops  = &ultrasonic_fops,
};

static int __init ultrasonic_init(void)
{
    int ret;

    // Map physical registers into virtual address space
    ultrasonic_virt_base = ioremap(ULTRASONIC_BASE_PHYS, ULTRASONIC_REG_SIZE);
    if (!ultrasonic_virt_base) {
        pr_err("ultrasonic_sensor: failed to ioremap\n");
        return -ENOMEM;
    }

    ret = misc_register(&ultrasonic_dev);
    if (ret) {
        pr_err("ultrasonic_sensor: misc_register failed\n");
        iounmap(ultrasonic_virt_base);
        return ret;
    }

    pr_info("ultrasonic_sensor: module loaded\n");
    return 0;
}

static void __exit ultrasonic_exit(void)
{
    misc_deregister(&ultrasonic_dev);
    iounmap(ultrasonic_virt_base);
    pr_info("ultrasonic_sensor: module unloaded\n");
}

module_init(ultrasonic_init);
module_exit(ultrasonic_exit);

