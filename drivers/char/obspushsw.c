#define PUSHSW_VERSION "0.1"

#include <linux/module.h>

#include <linux/types.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>
#include <linux/init.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <linux/errno.h>

#include <linux/obspushsw.h>

#ifdef DEBUG
#define PSW_DEBUG(str...)	printk (KERN_INFO "pushsw: " str)
#else
#define PSW_DEBUG(str...)	/* nothing */
#endif

unsigned int mvGppValueGet(unsigned int group, unsigned int mask);

static long psw_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case PSWIOC_GETSTATUS:
//printk(KERN_INFO "%s %d: %08x\n", __func__, __LINE__, mvGppValueGet(1, 0x000FFF00));
		return mvGppValueGet(1, 0x10000000) >> 28;
	default:
		break;
	}
	return (-ENOIOCTLCMD);
}

static int psw_open(struct inode *inode, struct file *file)
{
	switch (MINOR(inode->i_rdev)) {
	case PUSHSW_MINOR:
		return (0);
	default:
		return (-ENODEV);
	}
}

static int psw_release(struct inode *inode, struct file *file)
{
	return 0;
}

const struct file_operations pushsw_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl =psw_ioctl,
	.open           =psw_open,
	.release        =psw_release,
};

static struct miscdevice pushsw_dev = {
	.minor = PUSHSW_MINOR,
	.name  = "pushsw",
	.fops  = &pushsw_fops,
};

int __init psw_init(void)
{
	printk(KERN_INFO "Push switch driver v%s\n", PUSHSW_VERSION);
	return misc_register(&pushsw_dev);
}

void __exit psw_cleanup(void)
{
	misc_deregister( &pushsw_dev );
}

module_init(psw_init);
module_exit(psw_cleanup);
MODULE_LICENSE("GPL");
