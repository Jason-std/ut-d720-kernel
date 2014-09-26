/* linux/drivers/video/samsung/s3cfb_ut7gm.c
 *
 * Innulux 7 inch (800x480) Display Panel Support
 *
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/platform_device.h>
#include <linux/leds.h>
//#include <linux/gpio.h>
//#include <mach/gpio.h>
//#include <plat/gpio-cfg.h>
#include "../../oem_drv/power_gpio.h"

#include "s3cfb.h"


static struct s3cfb_lcd cfg_ut7gm = {
	.name = "ut7gm",
	.width = 800,
	.height = 480,
	.bpp = 32/*24*/,
	.freq = 60,

	.timing = {
		.h_fp = 10,
		.h_bp = 78,
		.h_sw = 10,
		.v_fp = 30,
		.v_fpe = 1,
		.v_bp = 30,
		.v_bpe = 1,
		.v_sw = 2,
	},

	.polarity = {
		.rise_vclk = 0,
		.inv_hsync = 1,
		.inv_vsync = 1,
		.inv_vden = 0,
	},
	
};

static int s_init = 0;
static void s3cfb_ut7gm_init(void)
{
	printk("LCD %s init.", cfg_ut7gm.name);
	if(!s_init)
	{
		write_power_item_value(POWER_LCD, 1);
		write_power_item_value(POWER_BACKLIGHT, 1);
		s_init = 1;
	}
}

extern int ut_register_lcd(struct s3cfb_lcd *lcd);
 
static int __init regiser_lcd(void)
{
	cfg_ut7gm.init_ldi = s3cfb_ut7gm_init;
	ut_register_lcd(&cfg_ut7gm);
	return 0;
}

early_initcall(regiser_lcd);

