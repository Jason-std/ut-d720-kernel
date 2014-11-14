/* linux/arch/arm/plat-samsung/dev-i2c7.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5P series device definition for i2c device 7
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/gfp.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/i2c-gpio.h>

#include <mach/irqs.h>
#include <mach/map.h>

#include <plat/regs-iic.h>
#include <plat/iic.h>
#include <plat/devs.h>
#include <plat/cpu.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>



static struct resource s3c_i2c_resource[] = {
	[0] = {
		.start	= S3C_PA_IIC7,
		.end	= S3C_PA_IIC7 + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_IIC7,
		.end	= IRQ_IIC7,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_i2c7 = {
	.name		= "s3c2440-i2c",
	.id		= 7,
	.num_resources	= ARRAY_SIZE(s3c_i2c_resource),
	.resource	= s3c_i2c_resource,
};


struct s3c2410_platform_i2c i2c7_data __initdata = {
	.flags		= 0,
	.slave_addr	= 0x10,
	.frequency	= 400*1000,
	.sda_delay	= 100,//100,
};

void __init s3c_i2c7_set_platdata(struct s3c2410_platform_i2c *pd)
{
	struct s3c2410_platform_i2c *npd;

	if (!pd) {
		pd = &i2c7_data;
		pd->bus_num = 7;
	}

	npd = s3c_set_platdata(pd, sizeof(struct s3c2410_platform_i2c),
			       &s3c_device_i2c7);

	if (!npd->cfg_gpio)
		npd->cfg_gpio = s3c_i2c7_cfg_gpio;
}


/*

struct i2c_gpio_platform_data i2c7_data  = {
	.sda_pin		= EXYNOS4_GPD0(2),
	.scl_pin	= EXYNOS4_GPD0(3),
	.udelay	= 2,
	.timeout  = 30,
};

struct platform_device s3c_device_i2c7 = {
	.name		= "i2c-gpio",
	.id		= 7,
	.dev       ={
		.platform_data = &i2c7_data,
	}
};
void __init s3c_i2c7_set_platdata(struct s3c2410_platform_i2c *pd)
{
	s3c_gpio_cfgpin(EXYNOS4_GPD0(3), S3C_GPIO_OUTPUT);
}
*/