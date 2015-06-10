#include <linux/module.h>
#include <linux/init.h>
#include <linux/buffer_head.h>
#include <linux/blkdev.h>

#define DRIVER_AUTHOR "Zespre Schmidt <starbops@gmail.com>"
#define DRIVER_DESC "SSTF Validator"

static int set_size = 512;
static struct block_device *bdev;

static int __init init_read(void)
{
	printk("init!!!\n");
	bdev = open_by_devnum(MKDEV(8, 17), 0x08000);
	if(IS_ERR(bdev)) {
		return -EIO;
	}
	if(set_blocksize(bdev, set_size)) {
		printk("set block size error\n");
		return -EIO;
	}
	// Design your write pattern here!!
	__breadahead(bdev, 1 * 4096, set_size);
	__breadahead(bdev, 11 * 4096, set_size);
	__breadahead(bdev, 2 * 4096, set_size);
	__breadahead(bdev, 12 * 4096, set_size);
	__breadahead(bdev, 3 * 4096, set_size);
	__breadahead(bdev, 13 * 4096, set_size);
	__breadahead(bdev, 4 * 4096, set_size);
	__breadahead(bdev, 14 * 4096, set_size);
	__breadahead(bdev, 5 * 4096, set_size);
	__breadahead(bdev, 15 * 4096, set_size);
	return 0;
}

static void __exit exit_read(void)
{
	printk("exit!!!\n");
	if(bdev) {
		blkdev_put(bdev, 0x08000);
		bdev = NULL;
	}
}

module_init(init_read);
module_exit(exit_read);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
