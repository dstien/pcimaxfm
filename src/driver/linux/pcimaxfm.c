/*
 * pcimaxfm - PCI MAX FM transmitter driver and tools
 * Copyright (C) 2007 Daniel Stien <daniel@stien.org>
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <pcimaxfm.h>

#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/types.h>

MODULE_LICENSE("GPL");
MODULE_VERSION(PACKAGE_VERSION);
MODULE_AUTHOR(PACKAGE_BUGREPORT_AUTHOR);
MODULE_DESCRIPTION(PACKAGE_DESCRIPTION);

u8 pcimaxfm_io_ctrl = 0;
u8 pcimaxfm_io_val  = 0;

int pcimaxfm_freq = PCIMAXFM_FREQ_NA;
int pcimaxfm_power = PCIMAXFM_POWER_NA;

int pcimaxfm_major = 0;
unsigned long pcimaxfm_iobase;
struct cdev *pcimaxfm_cdev;
static struct class *pcimaxfm_class;

static void pcimaxfm_i2c_sda_set(void)
{
	pcimaxfm_io_val |= PCIMAXFM_I2C_SDA;
	outb(pcimaxfm_io_val, pcimaxfm_iobase + PCIMAXFM_OFFSET_VALUE);
}

static void pcimaxfm_i2c_sda_clr(void)
{
	pcimaxfm_io_val &= ~PCIMAXFM_I2C_SDA;
	outb(pcimaxfm_io_val, pcimaxfm_iobase + PCIMAXFM_OFFSET_VALUE);
}

static void pcimaxfm_i2c_scl_set(void)
{
	pcimaxfm_io_val |= PCIMAXFM_I2C_SCL;
	outb(pcimaxfm_io_val, pcimaxfm_iobase + PCIMAXFM_OFFSET_VALUE);
}

static void pcimaxfm_i2c_scl_clr(void)
{
	pcimaxfm_io_val &= ~PCIMAXFM_I2C_SCL;
	outb(pcimaxfm_io_val, pcimaxfm_iobase + PCIMAXFM_OFFSET_VALUE);
}

static void pcimaxfm_i2c_delay(void)
{
	udelay(PCIMAXFM_I2C_DELAY_USECS);
}

static void pcimaxfm_i2c_start(void)
{
	pcimaxfm_i2c_delay();
	pcimaxfm_i2c_sda_set();
	pcimaxfm_i2c_scl_set();
	pcimaxfm_i2c_delay();
	pcimaxfm_i2c_sda_clr();
	pcimaxfm_i2c_delay();
	pcimaxfm_i2c_scl_clr();
	pcimaxfm_i2c_delay();
}

static void pcimaxfm_i2c_stop(void)
{
	pcimaxfm_i2c_sda_clr();
	pcimaxfm_i2c_delay();
	pcimaxfm_i2c_scl_set();
	pcimaxfm_i2c_delay();
	pcimaxfm_i2c_sda_set();
}

static void pcimaxfm_i2c_write_byte(u8 value)
{
	int i;

	for (i = 0; i < 8; i++) {
		if (value & (1 << (7 - i)))
			pcimaxfm_i2c_sda_set();
		else
			pcimaxfm_i2c_sda_clr();

		pcimaxfm_i2c_delay();
		pcimaxfm_i2c_scl_set();
		pcimaxfm_i2c_delay();
		pcimaxfm_i2c_scl_clr();
		pcimaxfm_i2c_delay();
	}

	pcimaxfm_i2c_sda_set();
	pcimaxfm_i2c_delay();
	pcimaxfm_i2c_scl_set();
	pcimaxfm_i2c_delay();
	pcimaxfm_i2c_scl_clr();
	pcimaxfm_i2c_delay();
}

static void pcimaxfm_write_freq_power(int freq, int power)
{
	if (freq == PCIMAXFM_FREQ_NA)
		pcimaxfm_freq = PCIMAXFM_FREQ_DEFAULT;
	else if (freq < PCIMAXFM_FREQ_MIN)
		pcimaxfm_freq = PCIMAXFM_FREQ_MIN;
	else if (freq > PCIMAXFM_FREQ_MAX)
		pcimaxfm_freq = PCIMAXFM_FREQ_MAX;
	else
		pcimaxfm_freq = freq;

	if (power == PCIMAXFM_POWER_NA)
		pcimaxfm_power = PCIMAXFM_POWER_MIN;
	else if (power < PCIMAXFM_POWER_MIN)
		pcimaxfm_power = PCIMAXFM_POWER_MIN;
	else if (power > PCIMAXFM_POWER_MAX)
		pcimaxfm_power = PCIMAXFM_POWER_MAX;
	else
		pcimaxfm_power = power;

	pcimaxfm_i2c_start();
	pcimaxfm_i2c_write_byte(
			PCIMAXFM_I2C_ADDR_PLL | PCIMAXFM_I2C_ADDR_WRITE_FLAG);
	pcimaxfm_i2c_write_byte(PCIMAXFM_GET_MSB(pcimaxfm_freq));
	pcimaxfm_i2c_write_byte(PCIMAXFM_GET_LSB(pcimaxfm_freq));
	pcimaxfm_i2c_write_byte(192);
	pcimaxfm_i2c_write_byte(pcimaxfm_power);
	pcimaxfm_i2c_stop();

	printk(KERN_DEBUG PACKAGE ": Frequency: %d Power: %d\n",
			pcimaxfm_freq, pcimaxfm_power);
}

static void pcimaxfm_write_rds(char *parameter, char *value)
{
	printk(KERN_DEBUG PACKAGE ": RDS: %s = \"%s\"\n", parameter, value);

	pcimaxfm_i2c_start();
	pcimaxfm_i2c_write_byte(
			PCIMAXFM_I2C_ADDR_RDS | PCIMAXFM_I2C_ADDR_WRITE_FLAG);

	pcimaxfm_i2c_write_byte(0);
	while (*parameter)
		pcimaxfm_i2c_write_byte(*parameter++);

	pcimaxfm_i2c_write_byte(1);
	while (*value)
		pcimaxfm_i2c_write_byte(*value++);

	pcimaxfm_i2c_write_byte(2);
	pcimaxfm_i2c_stop();
}

static void pcimaxfm_stereo_set(int stereo)
{
	if (stereo) {
		pcimaxfm_io_val &= ~PCIMAXFM_MONO;
	} else {
		pcimaxfm_io_val |= PCIMAXFM_MONO;
	}

	outb(pcimaxfm_io_val, pcimaxfm_iobase + PCIMAXFM_OFFSET_VALUE);
}

static int pcimaxfm_stereo_get(void)
{
	return ((pcimaxfm_io_val & PCIMAXFM_MONO) != PCIMAXFM_MONO);
}

static int pcimaxfm_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	int i, ret;
	char rdsbuf[5];

	if ((ret = pci_enable_device(dev))) {
		return ret;
	}

	pcimaxfm_iobase = pci_resource_start(dev, 0);

	printk(KERN_INFO PACKAGE ": pcimaxfm_iobase: %#lx\n", pcimaxfm_iobase);

	if (!request_region(pcimaxfm_iobase, PCIMAXFM_REGION_LENGTH, PACKAGE)) {
		printk(KERN_ERR PACKAGE ": I/O ports in use.\n");
		return -EBUSY;
	}

	pcimaxfm_io_ctrl |=
		(PCIMAXFM_MONO | PCIMAXFM_I2C_SDA | PCIMAXFM_I2C_SCL);
	outb(pcimaxfm_io_ctrl, pcimaxfm_iobase + PCIMAXFM_OFFSET_CONTROL);

	pcimaxfm_write_freq_power(PCIMAXFM_FREQ_DEFAULT, 8);

	pcimaxfm_write_rds("PS00", "PCIMAXFM");

	for (i = 1; i < 40; i++) {
		sprintf(rdsbuf, "PS%02d", i);
		pcimaxfm_write_rds(rdsbuf, "        ");
	}

	pcimaxfm_write_rds("PD00", "10");

	for (i = 1; i < 40; i++) {
		sprintf(rdsbuf, "PD%02d", i);
		pcimaxfm_write_rds(rdsbuf, "0");
	}

	return 0;
}

static void pcimaxfm_remove(struct pci_dev *dev)
{
	release_region(pcimaxfm_iobase, PCIMAXFM_REGION_LENGTH);
}

static struct pci_device_id pcimaxfm_id_table[] = {
	{
		.vendor    = PCIMAXFM_VENDOR,
		.device    = PCIMAXFM_DEVICE,
		.subvendor = PCIMAXFM_SUBVENDOR,
		.subdevice = PCIMAXFM_SUBDEVICE
	}
};

static struct pci_driver pcimaxfm_driver = {
	.name     = PACKAGE,
	.id_table = pcimaxfm_id_table,
	.probe	  = pcimaxfm_probe,
	.remove   = pcimaxfm_remove
};

static int pcimaxfm_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int pcimaxfm_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t pcimaxfm_read(struct file *filp, char __user *buf, size_t count,
		loff_t *f_pos)
{
	return 0;
}

static int pcimaxfm_ioctl(struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	int data;
	struct pcimaxfm_rds_set rds;

	switch (cmd) {
		case PCIMAXFM_FREQ_SET:
			if (get_user(data, (int __user *)arg))
				return -1;

			pcimaxfm_write_freq_power(data, pcimaxfm_power);
			break;

		case PCIMAXFM_FREQ_GET:
			if (put_user(pcimaxfm_freq, (int __user *)arg))
				return -1;
			break;

		case PCIMAXFM_POWER_SET:
			if (get_user(data, (int __user *)arg))
				return -1;
			
			pcimaxfm_write_freq_power(pcimaxfm_freq, data);
			break;

		case PCIMAXFM_POWER_GET:
			if (put_user(pcimaxfm_power, (int __user *)arg))
				return -1;

			break;

		case PCIMAXFM_STEREO_SET:
			if (get_user(data, (int __user *)arg))
				return -1;

			pcimaxfm_stereo_set(data);
			break;

		case PCIMAXFM_STEREO_GET:
			if (put_user(pcimaxfm_stereo_get(), (int __user *)arg))
				return -1;

			break;

		case PCIMAXFM_RDS_SET:
			if(copy_from_user(&rds,
					(struct pcimaxfm_rds_set __user *)arg,
					sizeof(rds)) != 0)
				return -1;

			pcimaxfm_write_rds(rds.param, rds.value);
			break;

		default:
			return -ENOTTY;
	}

	return 0;
}

static struct file_operations pcimaxfm_fops = {
	.owner   = THIS_MODULE,
	.read    = pcimaxfm_read,
	.ioctl   = pcimaxfm_ioctl,
	.open    = pcimaxfm_open,
	.release = pcimaxfm_release
};

static int __init pcimaxfm_init(void)
{
	int ret;
	dev_t dev = MKDEV(pcimaxfm_major, 0);

	if ((ret = pci_register_driver(&pcimaxfm_driver))) {
		printk(KERN_ERR PACKAGE ": Couldn't register driver.\n");
		return ret;
	}

	ret = alloc_chrdev_region(&dev, 0, 1, PACKAGE);
	pcimaxfm_major = MAJOR(dev);

	if (ret) {
		printk(KERN_ERR PACKAGE
			": Couldn't allocate char device (major %d).\n",
			pcimaxfm_major);
		goto fail_alloc_chrdev_region;
	}

	if (!(pcimaxfm_cdev = cdev_alloc())) {
		printk(KERN_ERR PACKAGE ": Couldn't allocate cdev.\n");
		ret = -1;
		goto fail_cdev;
	}
	pcimaxfm_cdev->owner = THIS_MODULE;
	pcimaxfm_cdev->ops = &pcimaxfm_fops;

	if ((ret = cdev_add(pcimaxfm_cdev, dev, 1))) {
		printk(KERN_ERR PACKAGE ": Couldn't add cdev pcimaxfm0.\n");
		goto fail_cdev;
	}

	if (IS_ERR(pcimaxfm_class = class_create(THIS_MODULE, PACKAGE))) {
		printk(KERN_ERR PACKAGE ": Couldn't create class.\n");
		ret = -1;
		goto fail_class_create;
	}

	if (IS_ERR(class_device_create(pcimaxfm_class, NULL, dev, NULL,
					PACKAGE "%d", 0))) {
		printk(KERN_ERR PACKAGE ": Couldn't create class device.\n");
		ret = -1;
		goto fail_class_device_create;
	}

	return 0;

fail_class_device_create:
	class_destroy(pcimaxfm_class);
fail_class_create:
	cdev_del(pcimaxfm_cdev);
fail_cdev:
	unregister_chrdev_region(dev, 1);
fail_alloc_chrdev_region:
	pci_unregister_driver(&pcimaxfm_driver);

	return ret;
}

static void __exit pcimaxfm_exit(void)
{
	dev_t dev = MKDEV(pcimaxfm_major, 0);

	class_device_destroy(pcimaxfm_class, dev);
	class_destroy(pcimaxfm_class);

	cdev_del(pcimaxfm_cdev);
	unregister_chrdev_region(dev, 1);

	pci_unregister_driver(&pcimaxfm_driver);
}

module_init(pcimaxfm_init);
module_exit(pcimaxfm_exit);
