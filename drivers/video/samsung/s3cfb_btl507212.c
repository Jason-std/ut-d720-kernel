/* linux/drivers/video/samsung/s3cfb_dummymipilcd.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Modified by Samsung Electronics (UK) on May 2010
 *
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/backlight.h>
#include <linux/lcd.h>
#include <linux/regulator/driver.h>

#include <plat/gpio-cfg.h>
#include <plat/regs-dsim.h>

#include <mach/dsim.h>
#include <mach/mipi_ddi.h>

#include "s5p-dsim.h"
#include "s3cfb.h"
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend btl_early_suspend;
#endif

struct regulator *lcd_regulator = NULL; 
struct regulator *lcd_vdd18_regulator = NULL;
EXPORT_SYMBOL(lcd_regulator);
EXPORT_SYMBOL(lcd_vdd18_regulator);

static struct mipi_ddi_platform_data *ddi_pd;
unsigned char set_password[] = {0xB9,0xFF,0x83,0x94};   
unsigned char set_mipi[] = {0xBA,0x13,0x83,0x00,0x16,0xA6,0x50,
			    0x08,0xFF,0x0F,0x24,0x03,0x21,0x24,
			    0x25,0x20,0x02,0x10};
unsigned char set_other1[] ={0xC1,0x01,0x00,0x07,0x10,0x16,0x1B,0x25,0x2C,
			     0x34,0x38,0x44,0x4B,0x52,0x5B,0x61,0x6A,0x71,
			     0x79,0x82,0x89,0x91,0x9B,0xA4,0xAC,0xB5,0xBD,
			     0xC6,0xCD,0xD6,0xDD,0xE4,0xEC,0xF4,0xFB,0x05,
			     0xEA,0xC9,0xB0,0x02,0x5F,0xFD,0x73,0xC0,0x00,
			     0x03,0x0E,0x14,0x19,0x20,0x28,0x2F,0x33,0x3D,
			     0x43,0x4A,0x52,0x57,0x60,0x66,0x6E,0x75,0x7C,
			     0x83,0x8B,0x93,0x9B,0xA3,0xAA,0xB3,0xBA,0xC2,
			     0xC9,0xCF,0xD7,0xDE,0xE5,0x05,0xEA,0xC9,0xB1,
			     0x02,0x5F,0xFD,0x73,0xC0,0x00,0x01,0x0F,0x16,
			     0x1C,0x26,0x2E,0x36,0x3B,0x47,0x4E,0x56,0x5F,
			     0x65,0x6F,0x76,0x7F,0x88,0x90,0x99,0xA2,0xAC,
			     0xB4,0xBD,0xC5,0xCD,0xD5,0xDE,0xE5,0xEB,0xF4,
			     0xFA,0xFF,0x05,0xEA,0xC9,0xB1,0x02,0x5F,0xFD,
			     0x73,0xC0};
unsigned char set_power[] = {0xB1,0x7C,0x00,0x24,0x09,0x01,0x11,
			     0x11,0x36,0x3E,0x26,0x26,0x57,0x0A,
			     0x01,0xE6};

unsigned char set_other[] = {0xB2,0x0F,0xC8,0x04,0x04,0x00,0x81};
unsigned char increase_driving_abilty[] = {0xBF,0x06,0x10};

unsigned char set_cyc[] = {0xB4,0x00,0x00,0x00,0x05,0x06,0x41,0x42,0x02,
			   0x41,0x42,0x43,0x47,0x19,0x58,0x60,0x08,0x85,0x10};

unsigned char set_gip[] = {0xD5,0x4C,0x01,0x07,0x01,0xCD,0x23,0xEF,0x45,
			   0x67,0x89,0xAB,0x11,0x00,0xDC,0x10,0xFE,0x32,
			   0xBA,0x98,0x76,0x54,0x00,0x11,0x40};

unsigned char gamma_rb[] = {0xE0,0x24,0x33,0x36,0x3F,0x3f,0x3f,0x3c,0x56,
			   0x05,0x0c,0x0e,0x11,0x13,0x12,0x14,0x12,0x1e,
			   0x24,0x33,0x36,0x3f,0x3f,0x3f,0x3c,0x56,0x05,
			   0x0c,0x0e,0x11,0x13,0x12,0x14,0x12,0x1e};

unsigned char set_other2[] = {0xC1,0x01,0x00,0x07,0x10,0x16,0x1B,0x25,0x2C,
			      0x34,0x38,0x44,0x4B,0x52,0x5B,0x61,0x6A,0x71,
			      0x79,0x82,0x89,0x91,0x9B,0xA4,0xAC,0xB5,0xBD,
			      0xC6,0xCD,0xD6,0xDD,0xE4,0xEC,0xF4,0xFB,0x05,
			      0xEA,0xC9,0xB0,0x02,0x5F,0xFD,0x73,0xC0,0x00,
			      0x03,0x0E,0x14,0x19,0x20,0x28,0x2F,0x33,0x3D,
			      0x43,0x4A,0x52,0x57,0x60,0x66,0x6E,0x75,0x7C,
			      0x83,0x8B,0x93,0x9B,0xA3,0xAA,0xB3,0xBA,0xC2,
			      0xC9,0xCF,0xD7,0xDE,0xE5,0x05,0xEA,0xC9,0xB1,
			      0x02,0x5F,0xFD,0x73,0xC0,0x00,0x01,0x0F,0x16,
			      0x1C,0x26,0x2E,0x36,0x3B,0x47,0x4E,0x56,0x5F,
			      0x65,0x6F,0x76,0x7F,0x88,0x90,0x99,0xA2,0xAC,
			      0xB4,0xBD,0xC5,0xCD,0xD5,0xDE,0xE5,0xEB,0xF4,
			      0xFA,0xFF,0x05,0xEA,0xC9,0xB1,0x02,0x5F,0xFD,
			      0x73,0xC0};

unsigned char ce_parameter[] = {0xE5,0x00,0x00,0x04,0x04,0x02,
				0x00,0x80,0x20,0x00,0x20,0x00,
				0x00,0x08,0x06,0x04,0x00,0x80,0x0E};
unsigned char set_tcon_opt[] = {0xC7,0x00,0x20}; 

static void btl507212_write_0(unsigned char dcs_cmd)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, /*GEN_SHORT_WR_1_PARA*/DCS_WR_NO_PARA, dcs_cmd, 0);
}

static void btl507212_write_1(unsigned char dcs_cmd, unsigned char param1)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, /*GEN_SHORT_WR_2_PARA*/DCS_WR_1_PARA, dcs_cmd, param1);
}


static void btl507212_write(unsigned char *buf,unsigned int data_size)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base,/*GEN_LONG_WR*/DCS_LONG_WR,(unsigned int)buf,data_size);
}

static void btl507212_display_off(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x28, 0x00);
}

static void s90379_sleep_in(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x10, 0);
}

static void s90379_sleep_out(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x11, 0);
}

static void btl507212_display_on(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x29, 0);
}

static void lcd_pannel_on(void)
{
    static int init_once_done = 0;


    if (!init_once_done) {
        printk("btl507212 init code...\n");

	btl507212_write_0(0x11);   //sleep out
	mdelay(200);

	btl507212_write(set_password,sizeof(set_password));
        mdelay(100);
	btl507212_write(set_mipi,sizeof(set_mipi));
        mdelay(100);
	btl507212_write_1(0xCC,0x05); //set panel
        mdelay(100);
	btl507212_write(set_other1,sizeof(set_other1));
	mdelay(100);
	btl507212_write(set_power,sizeof(set_power));
        mdelay(100);
	btl507212_write(set_other,sizeof(set_other));
        mdelay(100);
	btl507212_write(increase_driving_abilty,sizeof(increase_driving_abilty));
        mdelay(100);
	btl507212_write(set_cyc,sizeof(set_cyc));
        mdelay(100);
	btl507212_write(set_gip,sizeof(set_gip));
        mdelay(100);
	btl507212_write(gamma_rb,sizeof(gamma_rb));
        mdelay(100);
	btl507212_write(set_other2,sizeof(set_other2));
	mdelay(100);
	btl507212_write_1(0xE3,0x01); //enable ce
        mdelay(100);
	btl507212_write(ce_parameter,sizeof(ce_parameter));
        mdelay(100);
	btl507212_write(set_tcon_opt,sizeof(set_tcon_opt));
        mdelay(100);
	btl507212_write_1(0xB6,0x2A); //set vcom
        mdelay(100);

    }

    //display on
    btl507212_write_0(0x29);
    mdelay(20);
    btl507212_write_1(0x51,0xFF);
    mdelay(10);
    init_once_done = 1;
}

static void lcd_panel_init(void)
{
	lcd_pannel_on();
}

static int btl507212_panel_init(void)
{
    static int init_once_done = 0;
    if (!init_once_done) {
        init_once_done = 1;
        mdelay(10);
    }
	lcd_panel_init();

	return 0;
}

static int btl507212_set_link(void *pd, unsigned int dsim_base,
	unsigned char (*cmd_write) (unsigned int dsim_base, unsigned int data0,
	    unsigned int data1, unsigned int data2),
	unsigned char (*cmd_read) (unsigned int dsim_base, unsigned int data0,
	    unsigned int data1, unsigned int data2))
{
	struct mipi_ddi_platform_data *temp_pd = NULL;

	temp_pd = (struct mipi_ddi_platform_data *) pd;
	if (temp_pd == NULL) {
		printk(KERN_ERR "mipi_ddi_platform_data is null.\n");
		return -1;
	}

	ddi_pd = temp_pd;

	ddi_pd->dsim_base = dsim_base;

	if (cmd_write)
		ddi_pd->cmd_write = cmd_write;
	else
		printk(KERN_WARNING "cmd_write function is null.\n");

	if (cmd_read)
		ddi_pd->cmd_read = cmd_read;
	else
		printk(KERN_WARNING "cmd_read function is null.\n");

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
void btl507212_early_suspend(struct early_suspend *h)
{
	printk("[btl507212_early_suspend]\n");
	btl507212_write_0(0x10);
	mdelay(200);
	btl507212_write_0(0x28);
	mdelay(20);
}
void btl507212_late_resume(struct early_suspend *h)
{
	printk("[btl507212_late_resume]\n");
	btl507212_write_1(0x51,0xFF);
	mdelay(100);
	btl507212_write_1(0x53,0x24);
	mdelay(20);
}
#else
#ifdef CONFIG_PM
static int btl507212_suspend(void)
{
	btl507212_write_0(0x28);
	mdelay(20);
	btl507212_write_0(0x10);
	mdelay(20);

	return 0;
}

static int btl507212_resume(struct device *dev)
{
	return 0;
}
#else
#define btl507212_suspend	NULL
#define btl507212_resume	NULL
#endif
#endif

#define LCD_POWER EXYNOS4_GPD0(1)
static int btl507212_probe(struct device *dev)
{
	int err = 0;

	printk("============BTL507212 PROBE===============\n");

        lcd_regulator = regulator_get(NULL, "vdd33_lcd");
        if (IS_ERR(lcd_regulator)) {
                printk("%s: failed to get %s\n", __func__, "vdd33_lcd");
                err = -ENODEV;
                goto err_regulator;
        }
        regulator_enable(lcd_regulator);        //yulu

        lcd_vdd18_regulator = regulator_get(NULL, "lcd_iovdd18");
        if (IS_ERR(lcd_vdd18_regulator)) {
                printk("%s: failed to get %s\n", __func__, "lcd_iovdd18");
                err = -ENODEV;
                goto err_regulator;
        }
        regulator_enable(lcd_vdd18_regulator);
#ifdef CONFIG_HAS_EARLYSUSPEND
	btl_early_suspend.suspend = btl507212_early_suspend;
	btl_early_suspend.resume = btl507212_late_resume;
	btl_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	register_early_suspend(&btl_early_suspend);
#endif
	return 0;

err_regulator:
        regulator_put(lcd_regulator);
        regulator_put(lcd_vdd18_regulator);

	return err;
}

struct s3cfb_lcd btl507212_mipi_lcd = {
	.width	= 720,
	.height	= 1280,
	.bpp	= 24,
	.freq	= 60,

	.timing = {
		.h_fp = 0x20,
		.h_bp = 0x3b,
		.h_sw = 0x09,
		.v_fp = 0x0b,
		.v_fpe = 1,
		.v_bp = 0x0b,
		.v_bpe = 1,
		.v_sw = 0x02,
		.cmd_allow_len = 0xf,
	},

	.polarity = {
		.rise_vclk = 0,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};

static struct mipi_lcd_driver btl507212_mipi_driver = {
	.name = "btl507212_mipi_lcd",
	.init = btl507212_panel_init,
	.display_on = btl507212_display_on,
	.set_link = btl507212_set_link,
	.probe = btl507212_probe,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend = btl507212_suspend,
	.resume = btl507212_resume,
#endif
	.display_off = btl507212_display_off,
	.lcd_panel_info = &btl507212_mipi_lcd,
};

static int btl507212_init(void)
{
	s5p_dsim_register_lcd_driver(&btl507212_mipi_driver);
	return 0;
}

static void btl507212_exit(void)
{
	return;
}

/* name should be fixed as 's3cfb_set_lcd_info' */
void s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
	btl507212_mipi_lcd.init_ldi = NULL;
	ctrl->lcd = &btl507212_mipi_lcd;
}

module_init(btl507212_init);
module_exit(btl507212_exit);

MODULE_LICENSE("GPL");
