/*
 *  urbest register watcher, read/write physical register for debug.
 *  Copyright (c) 2009 urbest Technologies, Inc.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/sizes.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>
#include "../../fs/proc/internal.h"

#define PROC_NAME "display_controller"
#define COLOR_GAIN_PROC_NAME "color_gain"
#define GAMMA_PROC_NAME "gamma"
#define HUE_PROC_NAME "hue"
#define VSYNC_PROC_NAME "vsync"		//raymanfeng

//map SFR region only
#define REGWATCH_PHY_BASE	 0x10000000
#define REGWATCH_PHY_END	 0x1FFFFFFC

struct display_controller_ret
{
	unsigned int reg;
	unsigned int value;
}read_ret;

int gamma_reg_array[]={
	0x11c0037c,
	0x11c00380,
	0x11c00384,
	0x11c00388,
	0x11c0038c,
	0x11c00390,
	0x11c00394,
	0x11c00398,
	0x11c0039c,
	0x11c003a0,
	0x11c003a4,
	0x11c003a8,
	0x11c003ac,
	0x11c003b0,
	0x11c003b4,
	0x11c003b8,
	0x11c003bc,
	0x11c003c0,
	0x11c003c4,
	0x11c003c8,
	0x11c003cc,
	0x11c003d0,
	0x11c003d4,
	0x11c003d8,
	0x11c003dc,
	0x11c003e0,
	0x11c003e4,
	0x11c003e8,
	0x11c003ec,
	0x11c003f0,
	0x11c003f4,
	0x11c003f8,
	0x11c003fc,
};

int hue_reg_array[]={
	0x11c001ec,
	0x11c003f0,
	0x11c003f4,
	0x11c003f8,
	0x11c003fc,
	0x11c00200,
	0x11c00204,
	0x11c00208,
	0x11c0020c,
};


enum gamma_item
{
	GAMMA_ENABLE=0,
	GAMMA_DISABLE,
	GAMMA00,
	GAMMA01,
	GAMMA02,
	GAMMA03,
	GAMMA04,
	GAMMA05,
	GAMMA06,
	GAMMA07,
	GAMMA08,
	GAMMA09,
	GAMMA10,
	GAMMA11,
	GAMMA12,
	GAMMA13,
	GAMMA14,
	GAMMA15,
	GAMMA16,
	GAMMA17,
	GAMMA18,
	GAMMA19,
	GAMMA20,
	GAMMA21,
	GAMMA22,
	GAMMA23,
	GAMMA24,
	GAMMA25,
	GAMMA26,
	GAMMA27,
	GAMMA28,
	GAMMA29,
	GAMMA30,
	GAMMA31,
	GAMMA32,	
	GAMMA33,
	GAMMA34,
	GAMMA35,
	GAMMA36,
	GAMMA37,
	GAMMA38,
	GAMMA39,
	GAMMA40,
	GAMMA41,
	GAMMA42,
	GAMMA43,
	GAMMA44,
	GAMMA45,
	GAMMA46,
	GAMMA47,
	GAMMA48,
	GAMMA49,
	GAMMA50,
	GAMMA51,
	GAMMA52,
	GAMMA53,
	GAMMA54,
	GAMMA55,
	GAMMA56,
	GAMMA57,
	GAMMA58,
	GAMMA59,
	GAMMA60,
	GAMMA61,
	GAMMA62,
	GAMMA63,
	GAMMA64,
	GAMMA_PRINT,
	GAMMA_ITEM_MAX,
};

enum hue_item
{
	HUE_ENABLE=0,
	HUE_DISABLE,
	HUECOEF_CR_1,
	HUECOEF_CR_2,
	HUECOEF_CR_3,
	HUECOEF_CR_4,
	HUECOEF_CB_1,
	HUECOEF_CB_2,
	HUECOEF_CB_3,
	HUECOEF_CB_4,
	HUEOFFSET,
	HUE_PRINT,
	HUE_ITEM_MAX,
};

enum color_gain_item
{
	COLOR_GAIN_ENABLE=0,
	COLOR_GAIN_DISABLE,
	COLOR_GAIN_RGB,
	COLOR_GAIN_ITEM_MAX,
};


static int gamma_post_value[GAMMA_ITEM_MAX] = {-1};
static int hue_post_value[HUE_ITEM_MAX] = {-1};
static int color_gain_post_value[COLOR_GAIN_ITEM_MAX] = {-1};

static const char * gamma_node_names[GAMMA_ITEM_MAX] = 
{
	"gamma_enable",	
	"gamma_disable",	
	"gamma00",
	"gamma01",
	"gamma02",
	"gamma03",
	"gamma04",
	"gamma05",
	"gamma06",
	"gamma07",
	"gamma08",
	"gamma09",
	"gamma10",
	"gamma11",
	"gamma12",
	"gamma13",
	"gamma14",
	"gamma15",
	"gamma16",
	"gamma17",
	"gamma18",
	"gamma19",
	"gamma20",
	"gamma21",
	"gamma22",
	"gamma23",
	"gamma24",
	"gamma25",
	"gamma26",
	"gamma27",
	"gamma28",
	"gamma29",
	"gamma30",
	"gamma31",
	"gamma32",	
	"gamma33",
	"gamma34",
	"gamma35",
	"gamma36",
	"gamma37",
	"gamma38",
	"gamma39",
	"gamma40",
	"gamma41",
	"gamma42",
	"gamma43",
	"gamma44",
	"gamma45",
	"gamma46",
	"gamma47",
	"gamma48",
	"gamma49",
	"gamma50",
	"gamma51",
	"gamma52",
	"gamma53",
	"gamma54",
	"gamma55",
	"gamma56",
	"gamma57",
	"gamma58",
	"gamma59",
	"gamma60",
	"gamma61",
	"gamma62",
	"gamma63",
	"gamma64",
	"gamma_print",
};

static const char * hue_node_names[HUE_ITEM_MAX] = 
{
		"hue_enable",	
		"hue_disable",	
		"huecoef_cr_1",
		"huecoef_cr_2",
		"huecoef_cr_3",
		"huecoef_cr_4",
		"huecoef_cb_1",
		"huecoef_cb_2",
		"huecoef_cb_3",
		"huecoef_cb_4",
		"hueoffset",
		"hue_print",
};

static const char * color_gain_node_names[HUE_ITEM_MAX] = 
{
		"color_gain_enable",	
		"color_gain_disable",	
		"color_gain_rgb",
};

static int gamma_string_to_Index(const char * str)
{
	int i;
	if(!str) {
		return 0;
	}	

	for(i = 0; i < GAMMA_ITEM_MAX; i++) {
		if(!strncmp(gamma_node_names[i], str, strlen(gamma_node_names[i]))) {
			return i;
		}
	}
	
	return 0;
}

static int hue_string_to_Index(const char * str)
{
	int i;
	if(!str) {
		return 0;
	}
	
	for(i = 0; i < HUE_ITEM_MAX; i++) {
		if(!strncmp(hue_node_names[i], str, strlen(hue_node_names[i]))) {
			return i;
		}
	}
	return 0;
}

static int color_gain_string_to_Index(const char * str)
{
	int i;
	if(!str) {
		return 0;
	}
	
	for(i = 0; i < COLOR_GAIN_ITEM_MAX; i++) {
		if(!strncmp(color_gain_node_names[i], str, strlen(color_gain_node_names[i]))) {
			return i;
		}
	}
	return 0;
}

static int is_ignored_item(int index, int value)
{

	return 0;
}

unsigned int lcd_reg_write(unsigned int reg, unsigned int value)
{
	unsigned int * base;

	if((reg < REGWATCH_PHY_BASE) || (reg > REGWATCH_PHY_END))
	{
		printk("invalid address:0x%08x, please input 0x10000000~0xFFFFFFFC\n", reg);	
		return -1;
	}

	base = (unsigned int *) ioremap(reg, 0x100);
	if(!base)
	{
		printk("base ioremap failed\n");	
		return 0;
	}
	*base = value;
	iounmap(base);
	return 0;
}
EXPORT_SYMBOL(lcd_reg_write);

unsigned int lcd_reg_read(unsigned int reg)
{
	unsigned int * base, value;

	if((reg < REGWATCH_PHY_BASE) || (reg > REGWATCH_PHY_END))
	{
		printk("invalid address:0x%08x, please input 0x10000000~0xFFFFFFFC\n", reg);	
		return 0;
	}

	base = (unsigned int *) ioremap(reg, 0x100);
	if(!base)
	{
		printk("base ioremap failed\n");	
		return 0;
	}
	value = *base;
	iounmap(base);
	return value;	
}
EXPORT_SYMBOL(lcd_reg_read);

int write_gamma_item_value(int index, int value, const char *buffer)
{
	int i,temp;
	unsigned int reg=0; 
	printk("write_gamma_item_value %s index=%d value=%d\n", gamma_node_names[index],index,value);

	switch(index)
	{
		case GAMMA_ENABLE:
			reg=0x11c0000c;
			break;
		case GAMMA_DISABLE:
			reg=0x11c0000c;
			break;
		case GAMMA00:
			reg=0x11c0037c;
			break;
		case GAMMA01:
			reg=0x11c0037c;
			break;
		case GAMMA02:
			reg=0x11c00380;
			break;
		case GAMMA03:
			reg=0x11c00380;
			break;
		case GAMMA04:
			reg=0x11c00384;
			break;
		case GAMMA05:
			reg=0x11c00384;
			break;
		case GAMMA06:
			reg=0x11c00388;
			break;
		case GAMMA07:
			reg=0x11c00388;
			break;	
		case GAMMA08:
			reg=0x11c0038c;
			break;
		case GAMMA09:
			reg=0x11c0038c;
			break;
		case GAMMA10:
			reg=0x11c00390;
			break;
		case GAMMA11:
			reg=0x11c00390;
			break;
		case GAMMA12:
			reg=0x11c00394;
			break;
		case GAMMA13:
			reg=0x11c00394;
			break;
		case GAMMA14:
			reg=0x11c00398;
			break;
		case GAMMA15:
			reg=0x11c00398;
			break;		
		case GAMMA16:
			reg=0x11c0039c;
			break;
		case GAMMA17:
			reg=0x11c0039c;
			break;
		case GAMMA18:
			reg=0x11c003a0;
			break;
		case GAMMA19:
			reg=0x11c003a0;
			break;
		case GAMMA20:
			reg=0x11c003a4;
			break;
		case GAMMA21:
			reg=0x11c003a4;
			break;
		case GAMMA22:
			reg=0x11c003a8;
			break;
		case GAMMA23:
			reg=0x11c003a8;
			break;	
		case GAMMA24:
			reg=0x11c003ac;
			break;
		case GAMMA25:
			reg=0x11c003ac;
			break;
		case GAMMA26:
			reg=0x11c003b0;
			break;
		case GAMMA27:
			reg=0x11c003b0;
			break;
		case GAMMA28:
			reg=0x11c003b4;
			break;
		case GAMMA29:
			reg=0x11c003b4;
			break;
		case GAMMA30:
			reg=0x11c003b8;
			break;
		case GAMMA31:
			reg=0x11c003b8;
			break;		
		case GAMMA32:
			reg=0x11c003bc;
			break;
		case GAMMA33:
			reg=0x11c003bc;
			break;
		case GAMMA34:
			reg=0x11c003c0;
			break;
		case GAMMA35:
			reg=0x11c003c0;
			break;
		case GAMMA36:
			reg=0x11c003c4;
			break;
		case GAMMA37:
			reg=0x11c003c4;
			break;
		case GAMMA38:
			reg=0x11c003c8;
			break;
		case GAMMA39:
			reg=0x11c003c8;
			break;
		case GAMMA40:
			reg=0x11c003cc;
			break;	
		case GAMMA41:
			reg=0x11c003cc;
			break;
		case GAMMA42:
			reg=0x11c003d0;
			break;
		case GAMMA43:
			reg=0x11c003d0;
			break;
		case GAMMA44:
			reg=0x11c003d4;
			break;
		case GAMMA45:
			reg=0x11c003d4;
			break;
		case GAMMA46:
			reg=0x11c003d8;
			break;
		case GAMMA47:
			reg=0x11c003d8;
			break;
		case GAMMA48:
			reg=0x11c003dc;
			break;		
		case GAMMA49:
			reg=0x11c003dc;
			break;
		case GAMMA50:
			reg=0x11c003e0;
			break;
		case GAMMA51:
			reg=0x11c003e0;
			break;
		case GAMMA52:
			reg=0x11c003e4;
			break;
		case GAMMA53:
			reg=0x11c003e4;
			break;
		case GAMMA54:
			reg=0x11c003e8;
			break;
		case GAMMA55:
			reg=0x11c003e8;
			break;
		case GAMMA56:
			reg=0x11c003ec;
			break;	
		case GAMMA57:
			reg=0x11c003ec;
			break;
		case GAMMA58:
			reg=0x11c003f0;
			break;
		case GAMMA59:
			reg=0x11c003f0;
			break;
		case GAMMA60:
			reg=0x11c003f4;
			break;
		case GAMMA61:
			reg=0x11c003f4;
			break;
		case GAMMA62:
			reg=0x11c003f8;
			break;
		case GAMMA63:
			reg=0x11c003f8;
			break;		
		case GAMMA64:
			reg=0x11c003fc;
			break;
		default:
			reg=0x11c00000;
			break;
	}			

	if(index == GAMMA_ENABLE){
		if(sscanf(buffer, "%x", &value) == 1)
			if(value == 1){
				value = lcd_reg_read(reg);
				value |=0x10000;
				lcd_reg_write(reg, value);
			}
	}
	else if(index == GAMMA_DISABLE){
		if(sscanf(buffer, "%x", &value) == 1)
			if(value == 0){
				value = lcd_reg_read(reg);
				value &=0xfffeffff;
				lcd_reg_write(reg, value);
			}
	}
	else if(index == GAMMA_PRINT){
		for(i=0;i<ARRAY_SIZE(gamma_reg_array);i++)
		{
			value = lcd_reg_read(gamma_reg_array[i]);
			printk("%s: reg=%x value=%x\n",gamma_node_names[i+2],gamma_reg_array[i],value);
		}
	}
	else if(!(index % 2)){
		if(sscanf(buffer, "%x", &value) == 1)
		{
			value &=0xffff;
			temp = lcd_reg_read(reg);
			temp =(temp >> 16) << 16;
			value =temp | value;
			lcd_reg_write(reg, value);
		}
	}
	else if(index % 2){
		if(sscanf(buffer, "%x", &value) == 1)
		{
			value &=0xffff;
			value= value << 16;
			temp = lcd_reg_read(reg);
			temp &=0xffff;
			value =temp | value;
			lcd_reg_write(reg, value);
		}
	}
	
	value = lcd_reg_read(reg);
	printk("write:reg=0x%08x, value=0x%08x\n", reg, value);

	return 0;
}
EXPORT_SYMBOL(write_gamma_item_value);

struct display_controller_ret read_gamma_item_value(int index)
{
	int i,value=0;
	unsigned int reg=0; 

	switch(index)
	{
		case GAMMA_ENABLE:
			reg=0x11c0000c;
			break;
		case GAMMA_DISABLE:
			reg=0x11c0000c;
			break;		
		case GAMMA00:
			reg=0x11c0037c;
			break;
		case GAMMA01:
			reg=0x11c0037c;
			break;
		case GAMMA02:
			reg=0x11c00380;
			break;
		case GAMMA03:
			reg=0x11c00380;
			break;
		case GAMMA04:
			reg=0x11c00384;
			break;
		case GAMMA05:
			reg=0x11c00384;
			break;
		case GAMMA06:
			reg=0x11c00388;
			break;
		case GAMMA07:
			reg=0x11c00388;
			break;	
		case GAMMA08:
			reg=0x11c0038c;
			break;
		case GAMMA09:
			reg=0x11c0038c;
			break;
		case GAMMA10:
			reg=0x11c00390;
			break;
		case GAMMA11:
			reg=0x11c00390;
			break;
		case GAMMA12:
			reg=0x11c00394;
			break;
		case GAMMA13:
			reg=0x11c00394;
			break;
		case GAMMA14:
			reg=0x11c00398;
			break;
		case GAMMA15:
			reg=0x11c00398;
			break;		
		case GAMMA16:
			reg=0x11c0039c;
			break;
		case GAMMA17:
			reg=0x11c0039c;
			break;
		case GAMMA18:
			reg=0x11c003a0;
			break;
		case GAMMA19:
			reg=0x11c003a0;
			break;
		case GAMMA20:
			reg=0x11c003a4;
			break;
		case GAMMA21:
			reg=0x11c003a4;
			break;
		case GAMMA22:
			reg=0x11c003a8;
			break;
		case GAMMA23:
			reg=0x11c003a8;
			break;	
		case GAMMA24:
			reg=0x11c003ac;
			break;
		case GAMMA25:
			reg=0x11c003ac;
			break;
		case GAMMA26:
			reg=0x11c003b0;
			break;
		case GAMMA27:
			reg=0x11c003b0;
			break;
		case GAMMA28:
			reg=0x11c003b4;
			break;
		case GAMMA29:
			reg=0x11c003b4;
			break;
		case GAMMA30:
			reg=0x11c003b8;
			break;
		case GAMMA31:
			reg=0x11c003b8;
			break;		
		case GAMMA32:
			reg=0x11c003bc;
			break;
		case GAMMA33:
			reg=0x11c003bc;
			break;
		case GAMMA34:
			reg=0x11c003c0;
			break;
		case GAMMA35:
			reg=0x11c003c0;
			break;
		case GAMMA36:
			reg=0x11c003c4;
			break;
		case GAMMA37:
			reg=0x11c003c4;
			break;
		case GAMMA38:
			reg=0x11c003c8;
			break;
		case GAMMA39:
			reg=0x11c003c8;
			break;
		case GAMMA40:
			reg=0x11c003cc;
			break;	
		case GAMMA41:
			reg=0x11c003cc;
			break;
		case GAMMA42:
			reg=0x11c003d0;
			break;
		case GAMMA43:
			reg=0x11c003d0;
			break;
		case GAMMA44:
			reg=0x11c003d4;
			break;
		case GAMMA45:
			reg=0x11c003d4;
			break;
		case GAMMA46:
			reg=0x11c003d8;
			break;
		case GAMMA47:
			reg=0x11c003d8;
			break;
		case GAMMA48:
			reg=0x11c003dc;
			break;		
		case GAMMA49:
			reg=0x11c003dc;
			break;
		case GAMMA50:
			reg=0x11c003e0;
			break;
		case GAMMA51:
			reg=0x11c003e0;
			break;
		case GAMMA52:
			reg=0x11c003e4;
			break;
		case GAMMA53:
			reg=0x11c003e4;
			break;
		case GAMMA54:
			reg=0x11c003e8;
			break;
		case GAMMA55:
			reg=0x11c003e8;
			break;
		case GAMMA56:
			reg=0x11c003ec;
			break;	
		case GAMMA57:
			reg=0x11c003ec;
			break;
		case GAMMA58:
			reg=0x11c003f0;
			break;
		case GAMMA59:
			reg=0x11c003f0;
			break;
		case GAMMA60:
			reg=0x11c003f4;
			break;
		case GAMMA61:
			reg=0x11c003f4;
			break;
		case GAMMA62:
			reg=0x11c003f8;
			break;
		case GAMMA63:
			reg=0x11c003f8;
			break;		
		case GAMMA64:
			reg=0x11c003fc;
			break;			
		default:
			reg=0x11c00000;
			break;			
	}			

	read_ret.value = lcd_reg_read(reg);
	read_ret.reg = reg;

	return read_ret;
}
EXPORT_SYMBOL(read_gamma_item_value);

int write_hue_item_value(int index, int value, const char *buffer)
{
	int i;
	unsigned int reg=0; 
	printk("write_hue_item_value %s  index=%d value=%d\n", hue_node_names[index],index,value);

	switch(index)
	{
		case HUE_ENABLE:
			reg=0x11c0000c;
			break;
		case HUE_DISABLE:
			reg=0x11c0000c;
			break;
		case HUECOEF_CR_1:
			reg=0x11c001ec;
			break;
		case HUECOEF_CR_2:
			reg=0x11c001f0;
			break;
		case HUECOEF_CR_3:
			reg=0x11c001f4;
			break;
		case HUECOEF_CR_4:
			reg=0x11c001f8;
			break;
		case HUECOEF_CB_1:
			reg=0x11c001fc;
			break;
		case HUECOEF_CB_2:
			reg=0x11c00200;
			break;
		case HUECOEF_CB_3:
			reg=0x11c00204;
			break;
		case HUECOEF_CB_4:
			reg=0x11c00208;
			break;	
		case HUEOFFSET:
			reg=0x11c0020c;
			break;
		default:
			reg=0x11c00000;
			break;
	}			

	if(index == HUE_ENABLE){
		if(sscanf(buffer, "%x", &value) == 1)
			if(value == 1){
				value = lcd_reg_read(reg);
				value |=0x7780;
				lcd_reg_write(reg, value);
			}
	}
	else if(index == HUE_DISABLE){
		if(sscanf(buffer, "%x", &value) == 1)
			if(value == 0){
				value = lcd_reg_read(reg);
				value = value & 0xffff887F;
				lcd_reg_write(reg, value);
			}
	}
	else if(index == HUE_PRINT){
		for(i=0;i<ARRAY_SIZE(hue_reg_array);i++)
		{
			value = lcd_reg_read(hue_reg_array[i]);
			printk("%s: reg=%x value=%x\n",hue_node_names[i+2],hue_reg_array[i],value);
		}
	}
	else{
		if(sscanf(buffer, "%x", &value) == 1)
		{
			lcd_reg_write(reg, value);
		}
	}
	value = lcd_reg_read(reg);
	printk("write:reg=0x%08x, value=0x%08x\n", reg, value);

	return 0;
}
EXPORT_SYMBOL(write_hue_item_value);

struct display_controller_ret read_hue_item_value(int index)
{
	int i,value=0;
	unsigned int reg=0; 

	switch(index)
	{
		case HUE_ENABLE:
			reg=0x11c0000c;
			break;
		case HUE_DISABLE:
			reg=0x11c0000c;
			break;
		case HUECOEF_CR_1:
			reg=0x11c001ec;
			break;
		case HUECOEF_CR_2:
			reg=0x11c001f0;
			break;
		case HUECOEF_CR_3:
			reg=0x11c001f4;
			break;
		case HUECOEF_CR_4:
			reg=0x11c001f8;
			break;
		case HUECOEF_CB_1:
			reg=0x11c001fc;
			break;
		case HUECOEF_CB_2:
			reg=0x11c00200;
			break;
		case HUECOEF_CB_3:
			reg=0x11c00204;
			break;
		case HUECOEF_CB_4:
			reg=0x11c00208;
			break;	
		case HUEOFFSET:
			reg=0x11c0020c;
			break;		
		default:
			reg=0x11c00000;
			break;
	}			

	read_ret.value = lcd_reg_read(reg);
	read_ret.reg = reg;

	return read_ret;
}
EXPORT_SYMBOL(read_hue_item_value);

int write_color_gain_item_value(int index, int value, const char *buffer)
{
	int i;
	unsigned int reg=0; 
	printk("write_color_gain_item_value %s  index=%d value=%d\n", color_gain_node_names[index],index,value);

	switch(index)
	{
		case COLOR_GAIN_ENABLE:
			reg=0x11c0000c;
			break;
		case COLOR_GAIN_DISABLE:
			reg=0x11c0000c;
			break;
		case COLOR_GAIN_RGB:
			reg=0x11c001c0;
			break;		
		default:
			reg=0x11c00000;
			break;
	}			

	if(index == COLOR_GAIN_ENABLE){
		if(sscanf(buffer, "%x", &value) == 1)
			if(value == 1){
				value = lcd_reg_read(reg);
				value |=0x40000;
				lcd_reg_write(reg, value);
			}
	}
	else if(index == COLOR_GAIN_DISABLE){
		if(sscanf(buffer, "%x", &value) == 1)
			if(value == 0){
				value = lcd_reg_read(reg);
				value = value & 0xfffbffff;
				lcd_reg_write(reg, value);
			}
	}
	else{
		if(sscanf(buffer, "%x", &value) == 1)
		{
			lcd_reg_write(reg, value);
		}
	}
	value = lcd_reg_read(reg);
	printk("write:reg=0x%08x, value=0x%08x\n", reg, value);

	return 0;
}
EXPORT_SYMBOL(write_color_gain_item_value);

struct display_controller_ret read_color_gain_item_value(int index)
{
	int i,value=0;
	unsigned int reg=0; 

	switch(index)
	{
		case COLOR_GAIN_ENABLE:
			reg=0x11c0000c;
			break;
		case COLOR_GAIN_DISABLE:
			reg=0x11c0000c;
			break;
		case COLOR_GAIN_RGB:
			reg=0x11c001c0;
			break;		
		default:
			reg=0x11c00000;
			break;
	}		

	read_ret.value = lcd_reg_read(reg);
	read_ret.reg = reg;

	return read_ret;
}
EXPORT_SYMBOL(read_color_gain_item_value);

static int display_controller_gamma_write(struct file *file, const char *buffer, 
                           unsigned long count, void *data) 
{ 
	int value; 
	value = 0; 
	sscanf(buffer, "%d", &value);
	write_gamma_item_value(gamma_string_to_Index((const char*)data), value, buffer);
	return count; 
} 

static int display_controller_gamma_read(char *page, char **start, off_t off, 
			  int count, int *eof, void *data) 
{
	unsigned int len; 
	read_gamma_item_value(gamma_string_to_Index((const char*)data));
	len = sprintf( page, "read:reg=0x%08x, value=0x%08x\n", read_ret.reg, read_ret.value);
    return len;
}

/* display controller hue write/read */
static int display_controller_hue_write(struct file *file, const char *buffer, 
                           unsigned long count, void *data) 
{ 
	int value; 
	value = 0; 
	sscanf(buffer, "%d", &value);
	write_hue_item_value(hue_string_to_Index((const char*)data), value, buffer);
	return count; 
} 

static int display_controller_hue_read(char *page, char **start, off_t off, 
			  int count, int *eof, void *data) 
{
	unsigned int len; 
	read_hue_item_value(hue_string_to_Index((const char*)data));
	len = sprintf( page, "read:reg=0x%08x, value=0x%08x\n", read_ret.reg, read_ret.value);
    return len;
}

/* display controller color gain write/read */
static int display_controller_color_gain_write(struct file *file, const char *buffer, 
                           unsigned long count, void *data) 
{ 
	int value; 
	value = 0; 
	sscanf(buffer, "%d", &value);
	write_color_gain_item_value(color_gain_string_to_Index((const char*)data), value, buffer);
	return count; 
} 

static int display_controller_color_gain_read(char *page, char **start, off_t off, 
			  int count, int *eof, void *data) 
{
	unsigned int len; 
	read_color_gain_item_value(color_gain_string_to_Index((const char*)data));
	len = sprintf( page, "read:reg=0x%08x, value=0x%08x\n", read_ret.reg, read_ret.value);
    return len;
}


//=================== raymanfeng for vsync ====================
unsigned int g_vsync_toggle = 1;
EXPORT_SYMBOL(g_vsync_toggle);

static int vsync_writeproc(struct file *file,const char *buffer,
                           unsigned long count, void *data)
{
	if(buffer[0] == '0') 
		g_vsync_toggle = 0;
	else
		g_vsync_toggle = 1;

	return count;
}


static int vsync_readproc(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
	int len = 0;
	len = sprintf(page, "vsync=%d\n", g_vsync_toggle);
	return len;
}

static void init_vsync_proc(struct proc_dir_entry * root_entry)
{
	struct proc_dir_entry * entry = create_proc_entry(VSYNC_PROC_NAME, 0666, root_entry);
	if(entry) {
		entry->write_proc = vsync_writeproc;
		entry->read_proc = vsync_readproc;
	}
}
//===========================================================

static int __init display_controller_init(void)
{
	int i,j,k;
	struct proc_dir_entry *root_entry;
	struct proc_dir_entry *gamma_entry,*gamma,*hue_entry,*hue,*color_gain_entry,*color_gain;
	root_entry = proc_mkdir(PROC_NAME, &proc_root);
	gamma_entry = proc_mkdir(GAMMA_PROC_NAME, root_entry);
	hue_entry = proc_mkdir(HUE_PROC_NAME, root_entry);
	color_gain_entry = proc_mkdir(COLOR_GAIN_PROC_NAME, root_entry);

	for(i = 0; i < GAMMA_ITEM_MAX; i++) 
	{
		gamma = create_proc_entry(gamma_node_names[i], 0666, gamma_entry);
		if(gamma) {
			gamma->write_proc = display_controller_gamma_write;
			gamma->read_proc = display_controller_gamma_read;
			gamma->data = (void*)gamma_node_names[i];	
		}
	}

	for(j = 0; j < HUE_ITEM_MAX; j++) 
	{
		hue = create_proc_entry(hue_node_names[j], 0666, hue_entry);
		if(hue) {
			hue->write_proc = display_controller_hue_write;
			hue->read_proc = display_controller_hue_read;
			hue->data = (void*)hue_node_names[j];	
		}
	}

	for(k = 0; k < COLOR_GAIN_ITEM_MAX; k++) 
	{
		color_gain = create_proc_entry(color_gain_node_names[k], 0666, color_gain_entry);
		if(color_gain) {
			color_gain->write_proc = display_controller_color_gain_write;
			color_gain->read_proc = display_controller_color_gain_read;
			color_gain->data = (void*)color_gain_node_names[k];	
		}
	}

	init_vsync_proc(root_entry);	//raymanfeng for vsync.

	printk("urbest display controller(Gamma/Hue/Color gain)\n");
	return 0;
}

module_init(display_controller_init);

static void __exit display_controller_exit(void)
{
	remove_proc_entry(PROC_NAME, &proc_root);
}

module_exit(display_controller_exit);

MODULE_DESCRIPTION("display controller proc file");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("urbest, inc.");

