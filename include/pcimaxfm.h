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

#ifndef _PCIMAXFM_H
#define _PCIMAXFM_H

#define PCIMAXFM_VENDOR			0xe159
#define PCIMAXFM_DEVICE			0x0001
#define PCIMAXFM_SUBVENDOR		0x4001
#define PCIMAXFM_SUBDEVICE		0x0001

#define PCIMAXFM_OFFSET_CONTROL		2
#define PCIMAXFM_OFFSET_VALUE		3
#define PCIMAXFM_REGION_LENGTH		(PCIMAXFM_OFFSET_VALUE + 1)

#define PCIMAXFM_MONO			(1 << 1)
#define PCIMAXFM_I2C_SDA		(1 << 2)
#define PCIMAXFM_I2C_SCL		(1 << 3)

#define PCIMAXFM_I2C_ADDR_PLL		0x40
#define PCIMAXFM_I2C_ADDR_RDS		0x2c
#define PCIMAXFM_I2C_ADDR_WRITE_FLAG	(1 << 7)

#define PCIMAXFM_I2C_DELAY_USECS	100

#define PCIMAXFM_GET_MSB(value)		((value & 0xff00) >> 8)
#define PCIMAXFM_GET_LSB(value)		(value & 0x00ff)

#define PCIMAXFM_FREQ_MIN		1720 /*  86.00 MHz */
#define PCIMAXFM_FREQ_MAX		2160 /* 108.00 MHz */
#define PCIMAXFM_FREQ_DEFAULT		2000 /* 100.00 MHz */
#define PCIMAXFM_FREQ_NA		0

#define PCIMAXFM_POWER_MIN		0x00
#define PCIMAXFM_POWER_MAX		0x0F
#define PCIMAXFM_POWER_NA		0x10

#define PCIMAXFM_IOC_MAGIC		'+'
#define PCIMAXFM_POWER_SET		_IOR(PCIMAXFM_IOC_MAGIC, 4, int)
#define PCIMAXFM_POWER_GET		_IOW(PCIMAXFM_IOC_MAGIC, 5, int)

#endif /* _PCIMAXFM_H */
