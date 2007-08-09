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

static int pcimaxfm_major = PCIMAXFM_MAJOR;
static struct class *pcimaxfm_class;

struct pcimaxfm_dev {
	unsigned int dev_num;
	unsigned long base_addr;

	u8 io_ctrl;
	u8 io_data;

	unsigned int freq;
	unsigned int power;

	unsigned int use_count;
	spinlock_t use_lock;
	struct pci_dev *pci_dev;
	//struct semaphore sem;
	struct cdev cdev;
};

static struct pcimaxfm_dev pcimaxfm_devs[PCIMAXFM_MAX_DEVS];

static unsigned int pcimaxfm_num_devs = 0;

static void pcimaxfm_i2c_sda_set(struct pcimaxfm_dev *dev)
{
	dev->io_data |= PCIMAXFM_I2C_SDA;
	outb(dev->io_data, dev->base_addr + PCIMAXFM_OFFSET_VALUE);
}

static void pcimaxfm_i2c_sda_clr(struct pcimaxfm_dev *dev)
{
	dev->io_data &= ~PCIMAXFM_I2C_SDA;
	outb(dev->io_data, dev->base_addr + PCIMAXFM_OFFSET_VALUE);
}

static void pcimaxfm_i2c_scl_set(struct pcimaxfm_dev *dev)
{
	dev->io_data |= PCIMAXFM_I2C_SCL;
	outb(dev->io_data, dev->base_addr + PCIMAXFM_OFFSET_VALUE);
}

static void pcimaxfm_i2c_scl_clr(struct pcimaxfm_dev *dev)
{
	dev->io_data &= ~PCIMAXFM_I2C_SCL;
	outb(dev->io_data, dev->base_addr + PCIMAXFM_OFFSET_VALUE);
}

static void pcimaxfm_i2c_delay(void)
{
	udelay(PCIMAXFM_I2C_DELAY_USECS);
}

static void pcimaxfm_i2c_write_byte(struct pcimaxfm_dev *dev, u8 value)
{
	int i;

	for (i = 0; i < 8; i++) {
		if (value & (1 << (7 - i)))
			pcimaxfm_i2c_sda_set(dev);
		else
			pcimaxfm_i2c_sda_clr(dev);

		pcimaxfm_i2c_delay();
		pcimaxfm_i2c_scl_set(dev);
		pcimaxfm_i2c_delay();
		pcimaxfm_i2c_scl_clr(dev);
		pcimaxfm_i2c_delay();
	}

	pcimaxfm_i2c_sda_set(dev);
	pcimaxfm_i2c_delay();
	pcimaxfm_i2c_scl_set(dev);
	pcimaxfm_i2c_delay();
	pcimaxfm_i2c_scl_clr(dev);
	pcimaxfm_i2c_delay();
}

static void pcimaxfm_i2c_start(struct pcimaxfm_dev *dev, u8 addr)
{
	pcimaxfm_i2c_delay();
	pcimaxfm_i2c_sda_set(dev);
	pcimaxfm_i2c_scl_set(dev);
	pcimaxfm_i2c_delay();
	pcimaxfm_i2c_sda_clr(dev);
	pcimaxfm_i2c_delay();
	pcimaxfm_i2c_scl_clr(dev);
	pcimaxfm_i2c_delay();

	pcimaxfm_i2c_write_byte(dev, addr);
}

static void pcimaxfm_i2c_stop(struct pcimaxfm_dev *dev)
{
	pcimaxfm_i2c_sda_clr(dev);
	pcimaxfm_i2c_delay();
	pcimaxfm_i2c_scl_set(dev);
	pcimaxfm_i2c_delay();
	pcimaxfm_i2c_sda_set(dev);
}

static void pcimaxfm_write_freq_power(struct pcimaxfm_dev *dev,
		int freq, int power)
{
	if (freq == PCIMAXFM_FREQ_NA)
		dev->freq = PCIMAXFM_FREQ_DEFAULT;
	else if (freq < PCIMAXFM_FREQ_MIN)
		dev->freq = PCIMAXFM_FREQ_MIN;
	else if (freq > PCIMAXFM_FREQ_MAX)
		dev->freq = PCIMAXFM_FREQ_MAX;
	else
		dev->freq = freq;

	if (power == PCIMAXFM_POWER_NA)
		dev->power = PCIMAXFM_POWER_MIN;
	else if (power < PCIMAXFM_POWER_MIN)
		dev->power = PCIMAXFM_POWER_MIN;
	else if (power > PCIMAXFM_POWER_MAX)
		dev->power = PCIMAXFM_POWER_MAX;
	else
		dev->power = power;

	pcimaxfm_i2c_start(dev,
			PCIMAXFM_I2C_ADDR_PLL | PCIMAXFM_I2C_ADDR_WRITE_FLAG);
	pcimaxfm_i2c_write_byte(dev, PCIMAXFM_GET_MSB(dev->freq));
	pcimaxfm_i2c_write_byte(dev, PCIMAXFM_GET_LSB(dev->freq));
	pcimaxfm_i2c_write_byte(dev, 192);
	pcimaxfm_i2c_write_byte(dev, dev->power);
	pcimaxfm_i2c_stop(dev);

	printk(KERN_DEBUG PACKAGE ": Frequency: %d Power: %d\n",
			dev->freq, dev->power);
}

static void pcimaxfm_write_rds(struct pcimaxfm_dev *dev,
		char *parameter, char *value)
{
	printk(KERN_DEBUG PACKAGE ": RDS: %s = \"%s\"\n", parameter, value);

	pcimaxfm_i2c_start(dev,
			PCIMAXFM_I2C_ADDR_RDS | PCIMAXFM_I2C_ADDR_WRITE_FLAG);

	pcimaxfm_i2c_write_byte(dev, 0);
	while (*parameter)
		pcimaxfm_i2c_write_byte(dev, *parameter++);

	pcimaxfm_i2c_write_byte(dev, 1);
	while (*value)
		pcimaxfm_i2c_write_byte(dev, *value++);

	pcimaxfm_i2c_write_byte(dev, 2);
	pcimaxfm_i2c_stop(dev);
}

static void pcimaxfm_stereo_set(struct pcimaxfm_dev *dev, int stereo)
{
	if (stereo) {
		dev->io_data &= ~PCIMAXFM_MONO;
	} else {
		dev->io_data |= PCIMAXFM_MONO;
	}

	outb(dev->io_data, dev->base_addr + PCIMAXFM_OFFSET_VALUE);
}

static int pcimaxfm_stereo_get(struct pcimaxfm_dev *dev)
{
	return ((dev->io_data & PCIMAXFM_MONO) != PCIMAXFM_MONO);
}

static int pcimaxfm_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	struct pcimaxfm_dev *dev = container_of(inode->i_cdev,
			struct pcimaxfm_dev, cdev);

	spin_lock(&dev->use_lock);

	if (dev->use_count && !capable(CAP_DAC_OVERRIDE)) {
		ret = -EBUSY;
		goto open_done;
	}

	dev->use_count++;
	filp->private_data = dev;

open_done:
	spin_unlock(&dev->use_lock);

	return ret;
}

static int pcimaxfm_release(struct inode *inode, struct file *filp)
{
	struct pcimaxfm_dev *dev = filp->private_data;

	spin_lock(&dev->use_lock);
	dev->use_count--;
	spin_unlock(&dev->use_lock);

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
	struct pcimaxfm_dev *dev = filp->private_data;

	switch (cmd) {
		case PCIMAXFM_FREQ_SET:
			if (get_user(data, (int __user *)arg))
				return -1;

			pcimaxfm_write_freq_power(dev, data, dev->power);
			break;

		case PCIMAXFM_FREQ_GET:
			if (put_user(dev->freq, (int __user *)arg))
				return -1;
			break;

		case PCIMAXFM_POWER_SET:
			if (get_user(data, (int __user *)arg))
				return -1;
			
			pcimaxfm_write_freq_power(dev, dev->freq, data);
			break;

		case PCIMAXFM_POWER_GET:
			if (put_user(dev->power, (int __user *)arg))
				return -1;

			break;

		case PCIMAXFM_STEREO_SET:
			if (get_user(data, (int __user *)arg))
				return -1;

			pcimaxfm_stereo_set(dev, data);
			break;

		case PCIMAXFM_STEREO_GET:
			if (put_user(pcimaxfm_stereo_get(dev),
						(int __user *)arg))
				return -1;

			break;

		case PCIMAXFM_RDS_SET:
			if(copy_from_user(&rds,
					(struct pcimaxfm_rds_set __user *)arg,
					sizeof(rds)) != 0)
				return -1;

			pcimaxfm_write_rds(dev, rds.param, rds.value);
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

static int __devinit pcimaxfm_probe(struct pci_dev *pci_dev,
		const struct pci_device_id *id)
{
	int ret;
	struct pcimaxfm_dev *dev;
	dev_t dev_t;

	if (pcimaxfm_num_devs >= PCIMAXFM_MAX_DEVS) {
		printk(KERN_ERR PACKAGE
	"%u: Couldn't init card, increase max number of devices (%u).\n",
				pcimaxfm_num_devs, PCIMAXFM_MAX_DEVS);
		return -ENOMEM;
	}

	dev = &pcimaxfm_devs[pcimaxfm_num_devs];
	dev->dev_num = pcimaxfm_num_devs;
	dev->io_ctrl = 0;
	dev->io_data = 0;
	dev->freq    = PCIMAXFM_FREQ_NA;
	dev->power   = PCIMAXFM_POWER_NA;
	dev->pci_dev = pci_dev_get(pci_dev);
	pci_set_drvdata(pci_dev, dev);

	pcimaxfm_num_devs++;

	if ((ret = pci_enable_device(pci_dev))) {
		printk(KERN_INFO PACKAGE "%u: Couldn't enable device.\n",
				dev->dev_num);
		goto err_pci_enable_device;
	}

	dev->base_addr = pci_resource_start(pci_dev, 0);

	if (!request_region(dev->base_addr, PCIMAXFM_REGION_LENGTH, PACKAGE)) {
		printk(KERN_ERR PACKAGE "%u: I/O ports in use.\n",
				dev->dev_num);
		ret = -EBUSY;
		goto err_request_region;
	}

	cdev_init(&dev->cdev, &pcimaxfm_fops);
	dev->cdev.owner = THIS_MODULE;
	dev_t = MKDEV(pcimaxfm_major, dev->dev_num);

	if ((ret = cdev_add(&dev->cdev, dev_t, 1))) {
		printk(KERN_ERR PACKAGE "%u: Couldn't add cdev.\n",
				dev->dev_num);
		goto err_cdev_add;
	}

	if (IS_ERR(class_device_create(pcimaxfm_class, NULL, dev_t, NULL,
					PACKAGE "%d", dev->dev_num))) {
		printk(KERN_ERR PACKAGE "%u: Couldn't create class device.\n",
				dev->dev_num);
		ret = -1;
		goto err_class_device_create;
	}

	printk(KERN_INFO PACKAGE "%u: Found card %s, base address %#lx\n",
			dev->dev_num, pci_name(pci_dev), dev->base_addr);

	dev->io_ctrl |= (PCIMAXFM_MONO | PCIMAXFM_I2C_SDA | PCIMAXFM_I2C_SCL);
	outb(dev->io_ctrl, dev->base_addr + PCIMAXFM_OFFSET_CONTROL);

	return 0;

err_class_device_create:
	cdev_del(&dev->cdev);
err_cdev_add:
	release_region(dev->base_addr, PCIMAXFM_REGION_LENGTH);
err_request_region:
	pci_disable_device(pci_dev);
err_pci_enable_device:
	pci_dev_put(pci_dev);

	return ret;
}

static void __devexit pcimaxfm_remove(struct pci_dev *pci_dev)
{
	struct pcimaxfm_dev *dev = pci_get_drvdata(pci_dev);

	if (dev == NULL) {
		printk(KERN_ERR PACKAGE": Couldn't find PCI driver data.\n");
	} else {
		release_region(dev->base_addr, PCIMAXFM_REGION_LENGTH);
		cdev_del(&dev->cdev);
		class_device_destroy(pcimaxfm_class,
				MKDEV(pcimaxfm_major, dev->dev_num));
	}

	pci_disable_device(pci_dev);
	pci_dev_put(pci_dev);
}

static struct __devinitdata pci_device_id pcimaxfm_id_table[] = {
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
	.probe    = pcimaxfm_probe,
	.remove   = __devexit_p(pcimaxfm_remove)
};

static int __init pcimaxfm_init(void)
{
	int ret;
	dev_t dev = MKDEV(pcimaxfm_major, 0);

	if (pcimaxfm_major) {
		ret = register_chrdev_region(dev, PCIMAXFM_MAX_DEVS, PACKAGE);
	} else {
		ret = alloc_chrdev_region(&dev, 0, PCIMAXFM_MAX_DEVS, PACKAGE);
		pcimaxfm_major = MAJOR(dev);
	}

	if (ret) {
		printk(KERN_ERR PACKAGE
			": Couldn't allocate char devices (major %d).\n",
			pcimaxfm_major);
		return ret;
	}

	if (IS_ERR(pcimaxfm_class = class_create(THIS_MODULE, PACKAGE))) {
		printk(KERN_ERR PACKAGE ": Couldn't create driver class.\n");
		ret = -1;
		goto err_class_create;
	}

	if ((ret = pci_register_driver(&pcimaxfm_driver))) {
		printk(KERN_ERR PACKAGE ": Couldn't register PCI driver.\n");
		goto err_pci_register_driver;
	}

	return 0;

err_pci_register_driver:
	class_destroy(pcimaxfm_class);
err_class_create:
	unregister_chrdev_region(dev, PCIMAXFM_MAX_DEVS);

	return ret;
}

static void __exit pcimaxfm_exit(void)
{
	pci_unregister_driver(&pcimaxfm_driver);

	class_destroy(pcimaxfm_class);

	unregister_chrdev_region(MKDEV(pcimaxfm_major, 0), PCIMAXFM_MAX_DEVS);
}

module_init(pcimaxfm_init);
module_exit(pcimaxfm_exit);
