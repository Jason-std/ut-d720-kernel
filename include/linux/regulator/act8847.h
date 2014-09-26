/*
 * National Semiconductors ACT8847 PMIC chip client interface
 *
 *  Copyright (C) 2009 Samsung Electronics
 *  Author: Marek Szyprowski <m.szyprowski@samsung.com>
 *
 * Based on wm8400.h
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __LINUX_REGULATOR_ACT8847_H
#define __LINUX_REGULATOR_ACT8847_H

#include <linux/regulator/machine.h>


#define ACT8847_DCDC1 	0
#define ACT8847_DCDC2 	1
#define ACT8847_DCDC3 	2
#define ACT8847_DCDC4 	3

#define ACT8847_LDO5    4
#define ACT8847_LDO6    5
#define ACT8847_LDO7    6
#define ACT8847_LDO8    7
#define ACT8847_LDO9  	8
#define ACT8847_LDO10  	9
#define ACT8847_LDO11  	10
#define ACT8847_LDO12  	11
#define ACT8847_LDO13  	12
#define ACT8847_DCDC_DUMMY  13


#define PB_PBAMSK_INDEX		7
#define PB_PBDMSK_INDEX		6
#define PB_WDSRN_INDEX		1
#define PB_WDPCEN_INDEX	1
#define ACT8847_PB_REG	0xC0

#define ACT8847_NUM_REGULATORS 7

struct act8847_regulator_subdev {
	int id;
	struct regulator_init_data *initdata;
};

struct  act8847_platform_data {
	int num_regulators;
	struct  act8847_regulator_subdev *regulators;
};

#endif
