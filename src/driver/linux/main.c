/*
 * pcimaxfm - PCI MAX FM transmitter driver and tools
 * Copyright (C) 2007-2013 Daniel Stien <daniel@stien.org>
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

#if PCIMAXFM_ENABLE_RDS
#include "../../common/rds.h"
#endif /* PCIMAXFM_ENABLE_RDS */

#define KMSG(lvl, fmt, ...) \
	printk(lvl PACKAGE ": " fmt "\n", ## __VA_ARGS__)
#define KMSGN(lvl, fmt, ...) \
	printk(lvl PACKAGE "%u: " fmt "\n", dev->dev_num, ## __VA_ARGS__)

#define KMSG_ERR(fmt, ...)    KMSG(KERN_ERR, fmt, ## __VA_ARGS__)
#define KMSG_ERRN(fmt, ...)   KMSGN(KERN_ERR, fmt, ## __VA_ARGS__)
#define KMSG_INFO(fmt, ...)   KMSG(KERN_INFO, fmt, ## __VA_ARGS__)
#define KMSG_INFON(fmt, ...)  KMSGN(KERN_INFO, fmt, ## __VA_ARGS__)
#define KMSG_DEBUG(fmt, ...)  KMSG(KERN_DEBUG, fmt, ## __VA_ARGS__)
#define KMSG_DEBUGN(fmt, ...) KMSGN(KERN_DEBUG, fmt, ## __VA_ARGS__)

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
#if PCIMAXFM_ENABLE_RDS_TOGGLE
	unsigned int rdssignal;
#endif

	unsigned int use_count;
	spinlock_t use_lock;
	struct pci_dev *pci_dev;
	struct cdev cdev;
};

static struct pcimaxfm_dev pcimaxfm_devs[PCIMAXFM_MAX_DEVS];

static unsigned int pcimaxfm_num_devs = 0;

static void pcimaxfm_i2c_sda_set(struct pcimaxfm_dev *dev)
{
	dev->io_data |= PCIMAXFM_I2C_SDA;
	outb(dev->io_data, dev->base_addr + PCIMAXFM_OFFSET_DATA);
}

static void pcimaxfm_i2c_sda_clr(struct pcimaxfm_dev *dev)
{
	dev->io_data &= ~PCIMAXFM_I2C_SDA;
	outb(dev->io_data, dev->base_addr + PCIMAXFM_OFFSET_DATA);
}

static void pcimaxfm_i2c_scl_set(struct pcimaxfm_dev *dev)
{
	dev->io_data |= PCIMAXFM_I2C_SCL;
	outb(dev->io_data, dev->base_addr + PCIMAXFM_OFFSET_DATA);
}

static void pcimaxfm_i2c_scl_clr(struct pcimaxfm_dev *dev)
{
	dev->io_data &= ~PCIMAXFM_I2C_SCL;
	outb(dev->io_data, dev->base_addr + PCIMAXFM_OFFSET_DATA);
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

	KMSG_DEBUGN("Frequency: %d Power: %d", dev->freq, dev->power);
}

#if PCIMAXFM_ENABLE_RDS
static void pcimaxfm_write_rds(struct pcimaxfm_dev *dev,
		const char *parameter, const char *value)
{
	char *parm = (char*)parameter, *val = (char*)value;

	pcimaxfm_i2c_start(dev,
			PCIMAXFM_I2C_ADDR_RDS | PCIMAXFM_I2C_ADDR_WRITE_FLAG);

	pcimaxfm_i2c_write_byte(dev, 0);
	while (*parm)
		pcimaxfm_i2c_write_byte(dev, *parm++);

	pcimaxfm_i2c_write_byte(dev, 1);
	while (*val)
		pcimaxfm_i2c_write_byte(dev, *val++);

	pcimaxfm_i2c_write_byte(dev, 2);
	pcimaxfm_i2c_stop(dev);

	KMSG_DEBUGN("RDS: %s = \"%s\"", parameter, value);
}

#if PCIMAXFM_ENABLE_RDS_TOGGLE
static void pcimaxfm_rdssignal_set(struct pcimaxfm_dev *dev, int signal)
{
	char valstr[2];

	dev->rdssignal = signal ? 1 : 0;

	valstr[0] = '0' + dev->rdssignal;
	valstr[1] = '\0';

	pcimaxfm_write_rds(dev, "PWR", valstr);
}
#endif /* PCIMAXFM_ENABLE_RDS_TOGGLE */
#endif /* PCIMAXFM_ENABLE_RDS */

#if PCIMAXFM_ENABLE_TX_TOGGLE
static void pcimaxfm_tx_set(struct pcimaxfm_dev *dev, int tx)
{
	if (tx) {
		dev->io_data |= PCIMAXFM_TX;
	} else {
		dev->io_data &= ~PCIMAXFM_TX;
	}

	outb(dev->io_data, dev->base_addr + PCIMAXFM_OFFSET_DATA);
}

static int pcimaxfm_tx_get(struct pcimaxfm_dev *dev)
{
	return (dev->io_data & PCIMAXFM_TX) == PCIMAXFM_TX;
}
#endif /* PCIMAXFM_ENABLE_TX_TOGGLE */

static void pcimaxfm_stereo_set(struct pcimaxfm_dev *dev, int stereo)
{
#if PCIMAXFM_INVERT_STEREO
	stereo = !stereo;
#endif

	if (stereo) {
		dev->io_data &= ~PCIMAXFM_MONO;
	} else {
		dev->io_data |= PCIMAXFM_MONO;
	}

	outb(dev->io_data, dev->base_addr + PCIMAXFM_OFFSET_DATA);
}

static int pcimaxfm_stereo_get(struct pcimaxfm_dev *dev)
{
	int stereo = ((dev->io_data & PCIMAXFM_MONO) != PCIMAXFM_MONO);

#if PCIMAXFM_INVERT_STEREO
	return !stereo;
#else
	return stereo;
#endif
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
	struct pcimaxfm_dev *dev = filp->private_data;
	int len;
	static char str[0xff], str_freq[0x20], str_power[0x6];

	if (*f_pos != 0) {
		return 0;
	}

	if (dev->freq == PCIMAXFM_FREQ_NA) {
		snprintf(str_freq, sizeof(str_freq), "NA");
	} else {
		snprintf(str_freq, sizeof(str_freq),
				"%u.%u%u MHz (%u 50 KHz steps)",
				dev->freq / 20,
				(dev->freq % 20) / 2,
				(dev->freq % 2 == 0 ? 0 : 5),
				dev->freq);
	}

	if (dev->power == PCIMAXFM_POWER_NA) {
		snprintf(str_power, sizeof(str_power), "NA/%u",
				PCIMAXFM_POWER_MAX);
	} else {
		snprintf(str_power, sizeof(str_power), "%u/%u",
				dev->power, PCIMAXFM_POWER_MAX);
	}

	snprintf(str, sizeof(str),
#if PCIMAXFM_ENABLE_TX_TOGGLE
			"TX      : %s\n"
#endif /* PCIMAXFM_ENABLE_TX_TOGGLE */
			"Freq    : %s\n"
			"Power   : %s\n"
			"Stereo  : %s\n"
#if PCIMAXFM_ENABLE_RDS_TOGGLE
			"RDS     : %s\n"
#endif /* PCIMAXFM_ENABLE_RDS_TOGGLE */
			"\n"
			"Address : %#lx\n"
			"Control : %#x\n"
			"Data    : %#x\n",
#if PCIMAXFM_ENABLE_TX_TOGGLE
			 PCIMAXFM_STR_BOOL(pcimaxfm_tx_get(dev)),
#endif /* PCIMAXFM_ENABLE_TX_TOGGLE */
			str_freq, str_power,
			PCIMAXFM_STR_BOOL(pcimaxfm_stereo_get(dev)),
#if PCIMAXFM_ENABLE_RDS_TOGGLE
			PCIMAXFM_STR_BOOL(dev->rdssignal),
#endif /* PCIMAXFM_ENABLE_RDS_TOGGLE */
			dev->base_addr, dev->io_ctrl, dev->io_data);

	len = strlen(str);

	if ((count < len) || copy_to_user(buf, str, len)) {
		return -EFAULT;
	}

	*f_pos = len;

	return len;
}

static long pcimaxfm_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	int data;
	struct pcimaxfm_dev *dev = filp->private_data;
#if PCIMAXFM_ENABLE_RDS
	struct pcimaxfm_rds_set rds;
#endif /* PCIMAXFM_ENABLE_RDS_TOGGLE */

	switch (cmd) {
#if PCIMAXFM_ENABLE_TX_TOGGLE
		case PCIMAXFM_TX_SET:
			if (get_user(data, (int __user *)arg))
				return -1;

			pcimaxfm_tx_set(dev, data);
			break;

		case PCIMAXFM_TX_GET:
			if (put_user(pcimaxfm_tx_get(dev),
						(int __user *)arg))
				return -1;

			break;
#endif /* PCIMAXFM_ENABLE_TX_TOGGLE */

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

#if PCIMAXFM_ENABLE_RDS
#if PCIMAXFM_ENABLE_RDS_TOGGLE
		case PCIMAXFM_RDSSIGNAL_SET:
			if (get_user(data, (int __user *)arg))
				return -1;

			pcimaxfm_rdssignal_set(dev, data);
			break;

		case PCIMAXFM_RDSSIGNAL_GET:
			if (put_user(dev->rdssignal, (int __user *)arg))
				return -1;

			break;
#endif /* PCIMAXFM_ENABLE_RDS_TOGGLE */

		case PCIMAXFM_RDS_SET:
			if(copy_from_user(&rds,
					(struct pcimaxfm_rds_set __user *)arg,
					sizeof(rds)) != 0)
				return -1;

			if (validate_rds(rds.param, rds.value, 0, NULL)) {
				KMSG_ERRN("Invalid RDS set received.");
				return -1;
			}

			pcimaxfm_write_rds(dev,
					rds_params_name[rds.param], rds.value);
			break;
#endif /* PCIMAXFM_ENABLE_RDS */

		default:
			return -ENOTTY;
	}

	return 0;
}

static struct file_operations pcimaxfm_fops = {
	.owner          = THIS_MODULE,
	.read           = pcimaxfm_read,
	.unlocked_ioctl = pcimaxfm_ioctl,
	.open           = pcimaxfm_open,
	.release        = pcimaxfm_release
};

static int pcimaxfm_probe(struct pci_dev *pci_dev,
		const struct pci_device_id *id)
{
	int ret;
	struct pcimaxfm_dev *dev;
	dev_t dev_t;

	if (pcimaxfm_num_devs >= PCIMAXFM_MAX_DEVS) {
		KMSG_ERR("Couldn't init card %u, increase max number of "
				"devices (%u).",
				pcimaxfm_num_devs, PCIMAXFM_MAX_DEVS);
		return -ENOMEM;
	}

	dev = &pcimaxfm_devs[pcimaxfm_num_devs];
	dev->dev_num   = pcimaxfm_num_devs;
	dev->freq      = PCIMAXFM_FREQ_NA;
	dev->power     = PCIMAXFM_POWER_NA;
#if PCIMAXFM_ENABLE_RDS_TOGGLE
	dev->rdssignal = PCIMAXFM_BOOL_NA;
#endif /* PCIMAXFM_ENABLE_RDS_TOGGLE*/
	dev->use_count = 0;
	spin_lock_init(&dev->use_lock);

	dev->pci_dev   = pci_dev_get(pci_dev);
	pci_set_drvdata(pci_dev, dev);

	pcimaxfm_num_devs++;

	if ((ret = pci_enable_device(pci_dev))) {
		KMSG_ERRN("Couldn't enable device.");
		goto err_pci_enable_device;
	}

	dev->base_addr = pci_resource_start(pci_dev, 0);

	if (!request_region(dev->base_addr, PCIMAXFM_REGION_LENGTH, PACKAGE)) {
		KMSG_ERRN("Couldn't request I/O ports, region already in use.");
		ret = -EBUSY;
		goto err_request_region;
	}

	cdev_init(&dev->cdev, &pcimaxfm_fops);
	dev->cdev.owner = THIS_MODULE;
	dev_t = MKDEV(pcimaxfm_major, dev->dev_num);

	if ((ret = cdev_add(&dev->cdev, dev_t, 1))) {
		KMSG_ERRN("Couldn't add cdev.");
		goto err_cdev_add;
	}

	if (IS_ERR(device_create(pcimaxfm_class, NULL, dev_t, NULL,
				PACKAGE "%d", dev->dev_num))) {
		KMSG_ERRN("Couldn't create class device.");
		ret = -1;
		goto err_device_create;
	}

	KMSG_INFON("Found card %s, base address %#lx",
			pci_name(pci_dev), dev->base_addr);

	/* Get TX and stereo encoder state if their control lines are
	 * already enabled. */
	dev->io_ctrl =
		inb(dev->base_addr + PCIMAXFM_OFFSET_CTRL) & (
#if PCIMAXFM_ENABLE_TX_TOGGLE
				PCIMAXFM_TX |
#endif /* PCIMAXFM_ENABLE_TX_TOGGLE */
				PCIMAXFM_MONO);

	if ((dev->io_ctrl & PCIMAXFM_MONO) == PCIMAXFM_MONO) {
		dev->io_data = inb(dev->base_addr + PCIMAXFM_OFFSET_DATA)
			& PCIMAXFM_MONO;
	} else {
		dev->io_data = 0;
	}

#if PCIMAXFM_ENABLE_TX_TOGGLE
	if ((dev->io_ctrl & PCIMAXFM_TX) == PCIMAXFM_TX) {
		dev->io_data |= inb(dev->base_addr + PCIMAXFM_OFFSET_DATA)
			& PCIMAXFM_TX;
	}
#endif /* PCIMAXFM_ENABLE_TX_TOGGLE */

	/* Enable TX, stereo encoder control and I2C. */
	dev->io_ctrl |= (
#if PCIMAXFM_ENABLE_TX_TOGGLE
			PCIMAXFM_TX |
#endif /* PCIMAXFM_ENABLE_TX_TOGGLE */
			PCIMAXFM_MONO | PCIMAXFM_I2C_SDA | PCIMAXFM_I2C_SCL);
	outb(dev->io_ctrl, dev->base_addr + PCIMAXFM_OFFSET_CTRL);

	return 0;

err_device_create:
	cdev_del(&dev->cdev);
err_cdev_add:
	release_region(dev->base_addr, PCIMAXFM_REGION_LENGTH);
err_request_region:
	pci_disable_device(pci_dev);
err_pci_enable_device:
	pci_dev_put(pci_dev);

	return ret;
}

static void pcimaxfm_remove(struct pci_dev *pci_dev)
{
	struct pcimaxfm_dev *dev = pci_get_drvdata(pci_dev);

	if (dev == NULL) {
		KMSG_ERR("Couldn't find PCI driver data for removal.");
	} else {
		/* Disable everything but TX and stereo encoder state. */
		dev->io_ctrl &= (
#if PCIMAXFM_ENABLE_TX_TOGGLE
				PCIMAXFM_TX |
#endif /* PCIMAXFM_ENABLE_TX_TOGGLE */
				PCIMAXFM_MONO);
		outb(dev->io_ctrl, dev->base_addr + PCIMAXFM_OFFSET_CTRL);

		dev->io_data &= (
#if PCIMAXFM_ENABLE_TX_TOGGLE
				PCIMAXFM_TX |
#endif /* PCIMAXFM_ENABLE_TX_TOGGLE */
				PCIMAXFM_MONO);
		outb(dev->io_data, dev->base_addr + PCIMAXFM_OFFSET_DATA);

		release_region(dev->base_addr, PCIMAXFM_REGION_LENGTH);
		cdev_del(&dev->cdev);
		device_destroy(pcimaxfm_class,
				MKDEV(pcimaxfm_major, dev->dev_num));
	}

	pci_disable_device(pci_dev);
	pci_dev_put(pci_dev);
}

static DEFINE_PCI_DEVICE_TABLE(pcimaxfm_id_table) = {
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
	.remove   = pcimaxfm_remove
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
		KMSG_ERR("Couldn't allocate char devices (major %d, devs %d).",
				pcimaxfm_major, PCIMAXFM_MAX_DEVS);
		return ret;
	}

	if (IS_ERR(pcimaxfm_class = class_create(THIS_MODULE, PACKAGE))) {
		KMSG_ERR("Couldn't create driver class.");
		ret = -1;
		goto err_class_create;
	}

	if ((ret = pci_register_driver(&pcimaxfm_driver))) {
		KMSG_ERR("Couldn't register PCI driver.");
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
