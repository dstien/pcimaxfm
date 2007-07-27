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

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

MODULE_LICENSE("GPL");
MODULE_VERSION(PACKAGE_VERSION);
MODULE_AUTHOR(PACKAGE_AUTHOR);
MODULE_DESCRIPTION(PACKAGE_DESCRIPTION);

u8 pcimaxfm_io_ctrl = 0;
u8 pcimaxfm_io_val  = 0;

int pcimaxfm_major = 0;

int pcimaxfm_freq = PCIMAXFM_FREQ_NA;
int pcimaxfm_power = PCIMAXFM_POWER_NA;

unsigned long pcimaxfm_iobase;

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

static void pcimaxfm_mono_set(void)
{
	pcimaxfm_io_val |= PCIMAXFM_MONO;
	outb(pcimaxfm_io_val, pcimaxfm_iobase + PCIMAXFM_OFFSET_VALUE);
}

static void pcimaxfm_mono_clr(void)
{
	pcimaxfm_io_val &= ~PCIMAXFM_MONO;
	outb(pcimaxfm_io_val, pcimaxfm_iobase + PCIMAXFM_OFFSET_VALUE);
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
			": Couldn't register char device (major %d).\n",
			pcimaxfm_major);
		return ret;
	}

	return 0;
}

static void __exit pcimaxfm_exit(void)
{
	unregister_chrdev_region(MKDEV(pcimaxfm_major, 0), 1);
	pci_unregister_driver(&pcimaxfm_driver);
}

module_init(pcimaxfm_init);
module_exit(pcimaxfm_exit);
