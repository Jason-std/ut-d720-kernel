/* linux/drivers/video/samsung/s3cfb_s500hdmipilcd.c
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

#include <plat/gpio-cfg.h>
#include <plat/regs-dsim.h>

#include <mach/dsim.h>
#include <mach/mipi_ddi.h>

#include "s5p-dsim.h"
#include "s3cfb.h"


//map SFR region only
#define REGWATCH_PHY_BASE	 0x10000000
#define REGWATCH_PHY_END	 0x1FFFFFFC


extern unsigned int lcd_reg_write(unsigned int reg, unsigned int value);
extern unsigned int lcd_reg_read(unsigned int reg);


typedef struct 
{
	unsigned char add;
	unsigned char data;
}para_1_data;

para_1_data init_data[]={
	///////////////////////////////////////////////////////////////},
	//{CCMON},
	//CMD2 Page 0},
	{0xFF,0x01},
	{0xFB,0x01},
	//VGSW=1(JDW)/NB=1/Res=720x1280},
	{0x00,0x1A},
	//---2013.11.27 JDI new-----},
	{0x02,0x43},
	{0x04,0x43},
	//VGL=2VCL-AVDD,VGH=2AVDD},
	{0x08,0x56},
	//Turn off VGL clamp},
	{0x09,0x00},
	{0x0B,0xB3},
	{0x0C,0xB3},
	{0x0D,0x2F},
	//MUXW_JDI modify_2013/11/27},
	{0x0E,0x1B},
	//MUXG1_JDI modify_2013/11/27},
	{0x0F,0xA2},
	//VCOM Adjust},
	{0x11,0x71},
	//0x11,0x6B},
	{0x12,0x03},
	//VGLO sleep in pull GND},
	{0x37,0x02},
	//ID},
	{0x23,0x00},
	{0x24,0x00},
	{0x25,0x00},
	{0x26,0x00},
	{0x27,0x00},
	{0x28,0x00},
	{0x36,0x00},
	{0x6F,0x00},
	//CMD2 Page 4},
	{0xFF,0x05},
	//RTN 11.27 JDI modifty 0x8D=>0x82},
	{0x02,0x82},
	{0x03,0x82},
	{0x04,0x82},
	//NEW},
	{0x01,0x00},
	{0x05,0x30},
	{0x06,0x33},
	{0x07,0x70},
	{0x08,0x00},
	{0x11,0x00},
	//FS_JDI modify_2013/11/18},
	{0x09,0x00},
	//FW_JDI modify_2013/11/27},
	{0x0A,0x01},
	//SWI_JDI modify_2013/11/27},
	{0x0B,0x70},
	//MUXS_JDI modify_2013/11/27},
	{0x0D,0x04},
	//MUXW_JDI modify_2013/11/27},
	{0x0E,0x1B},
	//MUXG1_JDI modify_2013/11/27},
	{0x0F,0x02},
	//MUXG2_JDI modify_2013/11/27},
	{0x10,0x32},
	//PRECHW_JDI modify_2013/11/27},
	{0x12,0x00},
	//STI_JDI modify_2013/11/27},
	{0x14,0x00},
	//SDT_JDI modify_2013/12/06},
	{0x16,0x04},
	//SOUTE},
	{0x19,0xFF},
	{0x1A,0xFF},
	{0x1B,0xFF},
	//SOUTP},
	{0x1C,0x80},
	{0x1D,0x00},
	{0x1E,0x00},
	{0x1F,0x77},
	//SRGB setting=1~modify2013/12/06},
	{0x21,0x20},
	//sub-pixel column inversion},
	{0x22,0x55},
	{0x23,0x0D},
	//0x24~0x4A_power on/off sequence for JDW},
	{0x24,0x03},
	{0x25,0x53},
	{0x26,0x55},
	{0x27,0x35},
	//CKH power on/off sequence_JDI request_20131120},
	{0x28,0x00},
	{0x29,0x01},
	{0x2A,0x11},
	// power on VCOM},
	{0x2D,0x02},
	//CKH power on/off sequence_JDI request_20131120},
	{0x2F,0x00},
	{0x30,0x11},
	{0x31,0x20},
	{0x32,0x09},
	{0x35,0x0D},
	//STV Rise/Fall},
	{0x36,0x00},
	{0x37,0x09},
	//NVT_command},
	{0x48,0x40},
	{0x49,0x23},
	{0x4A,0xC1},
	////////////////////////////////////////////////////////////////////////},
	//JDI NEW 11.27},
	//SW_OUT=1},
	//{0x6F,0x21},
	{0x6F,0x25},
	{0x7B,0x10},	//0x10:Enable NT50198, 0x00:Disable NT50198
	{0x7C,0x00},
	//LTPS JDC 4 CKV mode},
	{0x7D,0x03},
	//Dummy Line select},
	{0x83,0x00},
	//NVT_command},
	{0x8D,0x00},
	{0xA4,0x8D},
	{0xA6,0x00},
	//BP&FP_JDI modify_2013/11/27},
	{0xBB,0x0A},
	{0xBC,0x06},
	//CIC setngs_JDI modify_2013/11/27},
	{0xD7,0x61},
	{0xD8,0x13},
	//////////////////////////////////////////////////////////////////////},
	//power on CIC_high},
	{0xF5,0x00},
	{0xF6,0x01},
	//////////////////////////////////////////////////////////////////////},
	//2TO6 MUX EQ&SOG OFF},
	{0x89,0xC1},
	//JDI NEW 11.27},
	{0x91,0x91},
	{0xA2,0x10},
	//RELOAD CMD2_P4},
	{0xFB,0x01},
	//page selection cmd start},
	{0xFF, 0x01},
	{0xFB, 0x01},
	//page selection cmd end},
	//R(+) MCR cmd},
	{0x75,0x00},
	{0x76,0x0B},
	{0x77,0x00},
	{0x78,0x4A},
	{0x79,0x00},
	{0x7A,0x78},
	{0x7B,0x00},
	{0x7C,0x95},
	{0x7D,0x00},
	{0x7E,0xAB},
	{0x7F,0x00},
	{0x80,0xBD},
	{0x81,0x00},
	{0x82,0xCD},
	{0x83,0x00},
	{0x84,0xDC},
	{0x85,0x00},
	{0x86,0xE9},
	{0x87,0x01},
	{0x88,0x14},
	{0x89,0x01},
	{0x8A,0x36},
	{0x8B,0x01},
	{0x8C,0x6B},
	{0x8D,0x01},
	{0x8E,0x95},
	{0x8F,0x01},
	{0x90,0xD8},
	{0x91,0x02},
	{0x92,0x0D},
	{0x93,0x02},
	{0x94,0x0F},
	{0x95,0x02},
	{0x96,0x41},
	{0x97,0x02},
	{0x98,0x7A},
	{0x99,0x02},
	{0x9A,0x9F},
	{0x9B,0x02},
	{0x9C,0xD2},
	{0x9D,0x02},
	{0x9E,0xF5},
	{0x9F,0x03},
	{0xA0,0x23},
	{0xA2,0x03},
	{0xA3,0x32},
	{0xA4,0x03},
	{0xA5,0x42},
	{0xA6,0x03},
	{0xA7,0x53},
	{0xA9,0x03},
	{0xAA,0x68},
	{0xAB,0x03},
	{0xAC,0x81},
	{0xAD,0x03},
	{0xAE,0x9F},
	{0xAF,0x03},
	{0xB0,0xD5},
	{0xB1,0x03},
	{0xB2,0xFF},
	//R(-) MCR cmd},
	{0xB3,0x00},
	{0xB4,0x0B},
	{0xB5,0x00},
	{0xB6,0x4A},
	{0xB7,0x00},
	{0xB8,0x78},
	{0xB9,0x00},
	{0xBA,0x95},
	{0xBB,0x00},
	{0xBC,0xAB},
	{0xBD,0x00},
	{0xBE,0xBD},
	{0xBF,0x00},
	{0xC0,0xCD},
	{0xC1,0x00},
	{0xC2,0xDC},
	{0xC3,0x00},
	{0xC4,0xE9},
	{0xC5,0x01},
	{0xC6,0x14},
	{0xC7,0x01},
	{0xC8,0x36},
	{0xC9,0x01},
	{0xCA,0x6B},
	{0xCB,0x01},
	{0xCC,0x95},
	{0xCD,0x01},
	{0xCE,0xD8},
	{0xCF,0x02},
	{0xD0,0x0D},
	{0xD1,0x02},
	{0xD2,0x0F},
	{0xD3,0x02},
	{0xD4,0x41},
	{0xD5,0x02},
	{0xD6,0x7A},
	{0xD7,0x02},
	{0xD8,0x9F},
	{0xD9,0x02},
	{0xDA,0xD2},
	{0xDB,0x02},
	{0xDC,0xF5},
	{0xDD,0x03},
	{0xDE,0x23},
	{0xDF,0x03},
	{0xE0,0x32},
	{0xE1,0x03},
	{0xE2,0x42},
	{0xE3,0x03},
	{0xE4,0x53},
	{0xE5,0x03},
	{0xE6,0x68},
	{0xE7,0x03},
	{0xE8,0x81},
	{0xE9,0x03},
	{0xEA,0x9F},
	{0xEB,0x03},
	{0xEC,0xD5},
	{0xED,0x03},
	{0xEE,0xFF},
	//G(+) MCR cmd},
	{0xEF,0x00},
	{0xF0,0x0B},
	{0xF1,0x00},
	{0xF2,0x4A},
	{0xF3,0x00},
	{0xF4,0x78},
	{0xF5,0x00},
	{0xF6,0x95},
	{0xF7,0x00},
	{0xF8,0xAB},
	{0xF9,0x00},
	{0xFA,0xBD},
	//page selection cmd start},
	{0xFF, 0x02},
	{0xFB, 0x01},
	//page selection cmd end},
	{0x00,0x00},
	{0x01,0xCD},
	{0x02,0x00},
	{0x03,0xDC},
	{0x04,0x00},
	{0x05,0xE9},
	{0x06,0x01},
	{0x07,0x14},
	{0x08,0x01},
	{0x09,0x36},
	{0x0A,0x01},
	{0x0B,0x6B},
	{0x0C,0x01},
	{0x0D,0x95},
	{0x0E,0x01},
	{0x0F,0xD8},
	{0x10,0x02},
	{0x11,0x0D},
	{0x12,0x02},
	{0x13,0x0F},
	{0x14,0x02},
	{0x15,0x41},
	{0x16,0x02},
	{0x17,0x7A},
	{0x18,0x02},
	{0x19,0x9F},
	{0x1A,0x02},
	{0x1B,0xD2},
	{0x1C,0x02},
	{0x1D,0xF5},
	{0x1E,0x03},
	{0x1F,0x23},
	{0x20,0x03},
	{0x21,0x32},
	{0x22,0x03},
	{0x23,0x42},
	{0x24,0x03},
	{0x25,0x53},
	{0x26,0x03},
	{0x27,0x68},
	{0x28,0x03},
	{0x29,0x81},
	{0x2A,0x03},
	{0x2B,0x9F},
	{0x2D,0x03},
	{0x2F,0xD5},
	{0x30,0x03},
	{0x31,0xFF},
	//G(-) MCR cmd},
	{0x32,0x00},
	{0x33,0x0B},
	{0x34,0x00},
	{0x35,0x4A},
	{0x36,0x00},
	{0x37,0x78},
	{0x38,0x00},
	{0x39,0x95},
	{0x3A,0x00},
	{0x3B,0xAB},
	{0x3D,0x00},
	{0x3F,0xBD},
	{0x40,0x00},
	{0x41,0xCD},
	{0x42,0x00},
	{0x43,0xDC},
	{0x44,0x00},
	{0x45,0xE9},
	{0x46,0x01},
	{0x47,0x14},
	{0x48,0x01},
	{0x49,0x36},
	{0x4A,0x01},
	{0x4B,0x6B},
	{0x4C,0x01},
	{0x4D,0x95},
	{0x4E,0x01},
	{0x4F,0xD8},
	{0x50,0x02},
	{0x51,0x0D},
	{0x52,0x02},
	{0x53,0x0F},
	{0x54,0x02},
	{0x55,0x41},
	{0x56,0x02},
	{0x58,0x7A},
	{0x59,0x02},
	{0x5A,0x9F},
	{0x5B,0x02},
	{0x5C,0xD2},
	{0x5D,0x02},
	{0x5E,0xF5},
	{0x5F,0x03},
	{0x60,0x23},
	{0x61,0x03},
	{0x62,0x32},
	{0x63,0x03},
	{0x64,0x42},
	{0x65,0x03},
	{0x66,0x53},
	{0x67,0x03},
	{0x68,0x68},
	{0x69,0x03},
	{0x6A,0x81},
	{0x6B,0x03},
	{0x6C,0x9F},
	{0x6D,0x03},
	{0x6E,0xD5},
	{0x6F,0x03},
	{0x70,0xFF},
	//B(+) MCR cmd},
	{0x71,0x00},
	{0x72,0x0B},
	{0x73,0x00},
	{0x74,0x4A},
	{0x75,0x00},
	{0x76,0x78},
	{0x77,0x00},
	{0x78,0x95},
	{0x79,0x00},
	{0x7A,0xAB},
	{0x7B,0x00},
	{0x7C,0xBD},
	{0x7D,0x00},
	{0x7E,0xCD},
	{0x7F,0x00},
	{0x80,0xDC},
	{0x81,0x00},
	{0x82,0xE9},
	{0x83,0x01},
	{0x84,0x14},
	{0x85,0x01},
	{0x86,0x36},
	{0x87,0x01},
	{0x88,0x6B},
	{0x89,0x01},
	{0x8A,0x95},
	{0x8B,0x01},
	{0x8C,0xD8},
	{0x8D,0x02},
	{0x8E,0x0D},
	{0x8F,0x02},
	{0x90,0x0F},
	{0x91,0x02},
	{0x92,0x41},
	{0x93,0x02},
	{0x94,0x7A},
	{0x95,0x02},
	{0x96,0x9F},
	{0x97,0x02},
	{0x98,0xD2},
	{0x99,0x02},
	{0x9A,0xF5},
	{0x9B,0x03},
	{0x9C,0x23},
	{0x9D,0x03},
	{0x9E,0x32},
	{0x9F,0x03},
	{0xA0,0x42},
	{0xA2,0x03},
	{0xA3,0x53},
	{0xA4,0x03},
	{0xA5,0x68},
	{0xA6,0x03},
	{0xA7,0x81},
	{0xA9,0x03},
	{0xAA,0x9F},
	{0xAB,0x03},
	{0xAC,0xD5},
	{0xAD,0x03},
	{0xAE,0xFF},
	//B(-) MCR cmd},
	{0xAF,0x00},
	{0xB0,0x0B},
	{0xB1,0x00},
	{0xB2,0x4A},
	{0xB3,0x00},
	{0xB4,0x78},
	{0xB5,0x00},
	{0xB6,0x95},
	{0xB7,0x00},
	{0xB8,0xAB},
	{0xB9,0x00},
	{0xBA,0xBD},
	{0xBB,0x00},
	{0xBC,0xCD},
	{0xBD,0x00},
	{0xBE,0xDC},
	{0xBF,0x00},
	{0xC0,0xE9},
	{0xC1,0x01},
	{0xC2,0x14},
	{0xC3,0x01},
	{0xC4,0x36},
	{0xC5,0x01},
	{0xC6,0x6B},
	{0xC7,0x01},
	{0xC8,0x95},
	{0xC9,0x01},
	{0xCA,0xD8},
	{0xCB,0x02},
	{0xCC,0x0D},
	{0xCD,0x02},
	{0xCE,0x0F},
	{0xCF,0x02},
	{0xD0,0x41},
	{0xD1,0x02},
	{0xD2,0x7A},
	{0xD3,0x02},
	{0xD4,0x9F},
	{0xD5,0x02},
	{0xD6,0xD2},
	{0xD7,0x02},
	{0xD8,0xF5},
	{0xD9,0x03},
	{0xDA,0x23},
	{0xDB,0x03},
	{0xDC,0x32},
	{0xDD,0x03},
	{0xDE,0x42},
	{0xDF,0x03},
	{0xE0,0x53},
	{0xE1,0x03},
	{0xE2,0x68},
	{0xE3,0x03},
	{0xE4,0x81},
	{0xE5,0x03},
	{0xE6,0x9F},
	{0xE7,0x03},
	{0xE8,0xD5},
	{0xE9,0x03},
	{0xEA,0xFF},
	//{CCMOFF},
	//{CCMRUN},
	{0xFF,0x00},
	{0xFF,0xFF},		

};

#ifdef CONFIG_FB_S5P_MIPI_S500HD
int colorgain_value = 0x10040100;
char s_selected_lcd_name[32] = {'\0'};
EXPORT_SYMBOL(s_selected_lcd_name);

static int __init select_lcd_name(char *str)
{
	printk("select_lcd_name: str=%s\n", str);
	strcpy(s_selected_lcd_name, str);
	
	return 0;
}

__setup("lcd=", select_lcd_name);


static int __init set_colorgain_reg(char *str)
{
	printk("_+_+_color gain: str=%s\n", str);
	if(*str == '\0')
	{
		printk("no colorgain value_+_+_+\n");
	}
	else
	{
		sscanf(str, "%x", &colorgain_value);
	}
	printk("set color gain = %x _+_+_+\n",colorgain_value);
	return 0;
}

void write_colorgain_reg(void)
{
	int value,reg;

	printk("_+_+_+_+write_colorgain_reg_+_+_+_+\n");	

	if(colorgain_value == 0x10040100)
	{
		printk("the value is no need change\n");
	}
	else
	{
		reg = 0x11c0000c; // color gain control enable reg
		value = lcd_reg_read(reg);
		value |=0x40000;
		lcd_reg_write(reg, value);

		reg = 0x11c001c0; // color gain value reg
		lcd_reg_write(reg, colorgain_value);
	}
}

__setup("lcdRGB=", set_colorgain_reg);

#endif

static struct mipi_ddi_platform_data *ddi_pd;

void s500hd_write_0(unsigned char dcs_cmd)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, dcs_cmd, 0);
}

static void s500hd_write_1(unsigned char dcs_cmd, unsigned char param1)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, dcs_cmd, param1);
}

static void s500hd_write_3(unsigned char dcs_cmd, unsigned char param1, unsigned char param2, unsigned char param3)
{
       unsigned char buf[4];
	buf[0] = dcs_cmd;
	buf[1] = param1;
	buf[2] = param2;
	buf[3] = param3;

	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, 4);
}

static void s500hd_write(void)
{
	unsigned char buf[15] = {0xf8, 0x01, 0x27, 0x27, 0x07, 0x07, 0x54,
							0x9f, 0x63, 0x86, 0x1a,
							0x33, 0x0d, 0x00, 0x00};
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
}

static void s500hd_display_off(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x28, 0x00);
}

void s500hd_sleep_in(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x10, 0);
}

void s500hd_sleep_out(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x11, 0);
}

static void s500hd_display_on(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x29, 0);
}

static void s500hd_turn_on(void)	//ericli add 2014-04-08
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, TURN_ON, 0, 0);
}
void lcd_pannel_on(void)
{
	int i = 0;	
#if 0	
#if 1	
	/* password */
	printk("_+_+lcd_pannel_on_+11111_+\n");
	s500hd_write_1(0xb0, 0x09);
	printk("_+_+lcd_pannel_on_+22222+\n");
	s500hd_write_1(0xb0, 0x09);
	s500hd_write_1(0xd5, 0x64);
	s500hd_write_1(0xb0, 0x09);

	s500hd_write_1(0xd5, 0x84);
	s500hd_write_3(0xf2, 0x02, 0x03, 0x1b);
	s500hd_write();
	s500hd_write_3(0xf6, 0x00, 0x8c, 0x07);

	/* reset */
	s500hd_write_1(0xfa, 0x01);

	/* Exit sleep */
	s500hd_write_0(0x11);
	mdelay(100);
	printk("_+_+lcd_pannel_on_+33333+\n");
	/* Set Display ON */
	s500hd_write_0(0x29);
#else
	printk("_+lcd_pannel_on_+_+\n");
	s500hd_write_0(0x00);
	mdelay(60);
	s500hd_write_0(0x11);
	mdelay(50); 
	s500hd_write_0(0x29);
	mdelay(50);
	s500hd_turn_on( );
#endif	

#else
	printk(KERN_INFO"s3cfb_mipi_dev===%s,start\n",__func__);
	while(1)
	{
		if((init_data[i].add == 0xff &&  init_data[i].data == 0xff))
			break;
		s500hd_write_1(init_data[i].add, init_data[i].data);
		udelay(5);
		i++;
	}
	printk(KERN_INFO"s3cfb_mipi_dev===%s,mid\n",__func__);
	//mdelay(120);
	s500hd_write_0(0x11);
	mdelay(100); 
	s500hd_write_0(0x29);	
	mdelay(20); 
	s500hd_write_0(0x2C);	
	printk(KERN_INFO"s3cfb_mipi_dev===%s,end\n",__func__);
	//s500hd_turn_on();
#endif		

}

void lcd_panel_init(void)
{
	lcd_pannel_on();
}

static int s500hd_panel_init(void)
{
	//mdelay(600);
	lcd_panel_init();

	return 0;
}

static int s500hd_set_link(void *pd, unsigned int dsim_base,
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

static int s500hd_probe(struct device *dev)
{
	return 0;
}

#ifdef CONFIG_PM
static int s500hd_suspend(void)
{
	s500hd_write_0(0x28);
	mdelay(20);
	s500hd_write_0(0x10);
	mdelay(20);

	return 0;
}

static int s500hd_resume(struct device *dev)
{
	return 0;
}
#else
#define s500hd_suspend	NULL
#define s500hd_resume	NULL
#endif

static struct mipi_lcd_driver s500hd_mipi_driver = {
	.name = "mipi_s500hd",
	.init = s500hd_panel_init,
	.display_on = s500hd_display_on,
	.set_link = s500hd_set_link,
	.probe = s500hd_probe,
	.suspend = s500hd_suspend,
	.resume = s500hd_resume,
	.display_off = s500hd_display_off,
};

static int s500hd_init(void)
{
	s5p_dsim_register_lcd_driver(&s500hd_mipi_driver);
	return 0;
}

static void s500hd_exit(void)
{
	return;
}

static struct s3cfb_lcd s500hd_mipi_lcd = {
	.width	= 720,
	.height	= 1280,
	.bpp	= 24,
	.freq	= 65,

	.timing = {
		.h_fp = 10,
		.h_bp = 10,
		.h_sw = 3,
		.v_fp = 10,
		.v_fpe = 1,
		.v_bp = 10,
		.v_bpe = 1,
		.v_sw = 3,
		//.cmd_allow_len = 4,
		.cmd_allow_len = 11,	/*v_fp=stable_vfp + cmd_allow_len */
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};

#ifdef CONFIG_FB_S5P_MIPI_S500HD
/* name should be fixed as 's3cfb_set_lcd_info' */
void s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
	s500hd_mipi_lcd.init_ldi = NULL;
	ctrl->lcd = &s500hd_mipi_lcd;
}
#endif
module_init(s500hd_init);
module_exit(s500hd_exit);

MODULE_LICENSE("GPL");

