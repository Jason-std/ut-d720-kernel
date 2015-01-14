/*
 *  misc power and gpio proc file for Urbest
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

#include "../../../fs/proc/internal.h"

#include <plat/gpio-cfg.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <mach/regs-pmu.h>

#include "urbetter/power_gpio.h"

#include <urbetter/check.h>

#define GPIO_DEBUG


#ifdef GPIO_DEBUG
#define P_GPIO_DEBUG(x...) printk(KERN_DEBUG "[gpio drv]" x)
#else
#define P_GPIO_DEBUG(x...) do { } while(0)
#endif


#define PROC_NAME                    "power_gpio"



static const char * s_node_names[POWER_ITEM_MAX] =
{
	/* 0*/
	"power_5v_en",
	"power_int_en",
	"power_gps_rst",
	"power_gps_en",
	"power_wifi_en",	//wifi power enable
	"power_wifi_ldo",   //wifi inter ldo enable
	"power_gsm_sw",
	"power_gsm_wup",  //cpu wake up 3g/gsm

	"power_lcd_en",
	"power_lcd_18v_en",
	"power_lvds_pd",
	"power_bl_en",

	"power_hub_rst",
	"power_hub_con",
	"power_hub2_en",
	"power_hub2_rst",
	"power_spk_en",
	"power_usb_sw",
	"power_motor_en",
	"power_ts_rst",

	"power_state_ac",
	"power_state_charge",

	"power_scan_en",
	"power_scan_buz",
	"power_rfid_en",

	"power_fcam_28v",   // front camera
	"power_fcam_18v",
	"power_fcam_pd",
	"power_fcam_rst",

	"power_bcam_28v",  // back camera
	"power_bcam_18v",
	"power_bcam_pd",
	"power_bcam_rst",

	"power_cam_af",

};

static struct power_gpio_node gpio_node_common[] =
{

	{POWER_5V_EN,			EXYNOS4_GPK1(1), 1, 1, NULL},
	{POWER_5V_EN,			EXYNOS4_GPK1(2), 1, 1, NULL},
	{POWER_INT_EN,			EXYNOS4212_GPM3(7), 0, 1, NULL}, // low is  default volt, high is another volt (PMU cc)
 	{POWER_GPS_RST,		EXYNOS4_GPL0(6), 0, 1, NULL},
	{POWER_WIFI_EN,			EXYNOS4212_GPM3(1), 1, 1, NULL},
	{POWER_WIFI_LDO,		EXYNOS4_GPL1(1), 1, 1, NULL},
	{POWER_LCD33_EN,		EXYNOS4212_GPM2(3), 1, 1, NULL},
	{POWER_LVDS_PD,		EXYNOS4212_GPM2(4), 1, 1, NULL},
	{POWER_BL_EN,			EXYNOS4212_GPM4(0), 1, 1, NULL},

	{POWER_HUB_RST,		EXYNOS4212_GPM3(2), 1, 1, NULL},
	{POWER_HUB_CON,		EXYNOS4212_GPM3(3), 1, 1, NULL},
	{POWER_SPK_EN,			EXYNOS4212_GPM1(4), 1, 1, NULL},
	{POWER_USB_SW,			EXYNOS4212_GPM1(5), 1, 1, NULL},
	{POWER_MOTOR_EN,		EXYNOS4212_GPM1(6), 1, 1, NULL},

	{POWER_STATE_AC,		EXYNOS4_GPX0(2), 1, 0, NULL},  //EINT2
	{POWER_STATE_CHARGE,	EXYNOS4_GPX2(7), 1, 0, NULL}, //EINT23

	{POWER_SCAN_EN,		GPIO_SCAN_EN, 1, 1, NULL},
	{POWER_SCAN_BUZ,		GPIO_SCAN_BUZ, 1, 0, NULL},

	{POWER_RFID_EN,			GPIO_RFID_EN, 1, 1, NULL},

	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
};

static int POWER_GPIO_COUNT=ARRAY_SIZE(gpio_node_common);
static struct power_gpio_node * p_gpio_node=gpio_node_common;

int read_power_item_value(int index)
{
	int i;
	for(i = 0; i < POWER_GPIO_COUNT; i++)
	{
		if(p_gpio_node[i].index == index)
		{
			P_GPIO_DEBUG("%s:i=%d,index=%d\n",__func__,i,index);
			if(p_gpio_node[i].read_func){
				return p_gpio_node[i].read_func(p_gpio_node[i]);
			}
		
			if(p_gpio_node[i].polarity == 0)
				return !gpio_get_value(p_gpio_node[i].pin);
			else
				return gpio_get_value(p_gpio_node[i].pin);
		}
	}
	return 0;
}
EXPORT_SYMBOL(read_power_item_value);


int write_power_item_value(int index, int value)
{
	int i;

	for(i = 0; i < POWER_GPIO_COUNT; i++)
	{
		if(p_gpio_node[i].index == index)
		{
			P_GPIO_DEBUG("%s:i=%d,index=%d\n",__func__,i,index);

			if(p_gpio_node[i].write_func){
				p_gpio_node[i].write_func(p_gpio_node[i],value);
				break;
			}
		
			if(p_gpio_node[value].direction == 0)	//input only
				continue;

			s3c_gpio_setpull(p_gpio_node[i].pin, S3C_GPIO_PULL_UP);
			if(p_gpio_node[i].polarity == 0)
				gpio_direction_output(p_gpio_node[i].pin, (value > 0) ? 0 : 1);
			else
				gpio_direction_output(p_gpio_node[i].pin, value);
		}
	}

	return 0;
}
EXPORT_SYMBOL(write_power_item_value);

static void gpio_input_init(void)
{
	int i;
	for(i = 0; i < POWER_GPIO_COUNT; i++)
	{
		if(p_gpio_node[i].direction == 0)	//0 for input only
		{
			s3c_gpio_setpull(p_gpio_node[i].pin, S3C_GPIO_PULL_NONE);
			gpio_direction_input(p_gpio_node[i].pin);
		}
	}
}

static void init_request_gpio(void)
{
	int i;
	for(i = 0; i < POWER_GPIO_COUNT; i++)
	{
		if(gpio_request(p_gpio_node[i].pin, PROC_NAME))
			printk("%s:request gpio failed,i=%d\n",__func__,i);
	}

}

static int stringToIndex(const char * str)
{
	int i;
	if(!str) {
		return POWER_ITEM_INVALID;
	}

	for(i = 0; i < POWER_ITEM_MAX; i++) {
		if(!strcmp(s_node_names[i], str)) {
			return i;
		}
	}
	return POWER_ITEM_INVALID;
}

static int power_gpio_proc_write(struct file *file, const char *buffer,
                           unsigned long count, void *data)
{
	int value;
	value = 0;
	sscanf(buffer, "%d", &value);
	P_GPIO_DEBUG("%s:name=%s\n",__func__,(const char *)data);
	write_power_item_value(stringToIndex((const char*)data), value);
	return count;
}

static int power_gpio_proc_read(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
	int len;
	int value;
	int index = stringToIndex((const char*)data);
	P_GPIO_DEBUG("%s:name=%s\n",__func__,(const char *)data);
	value = read_power_item_value(index);
	len = sprintf( page, "%d\n", value);
	return len;
}

static void ut7gm_pm_power_off(void)
{
	write_power_item_value(POWER_SPK_EN, 0);
	writel(0x5200, S5P_PS_HOLD_CONTROL);
	while(1);
}

static void power_gpio_init_common()
{
	if (is_axp_pmu) {

	} else {
		if(!pm_power_off)
			pm_power_off = ut7gm_pm_power_off;
	}
}

int power_gpio_register(struct power_gpio_oem * p)
{
	if(!(p->name) || !CHECK_UTMODEL(p->name))
		return -1;
	if(p->pcb && !CHECK_PCB(p->pcb))
		return -1;
	P_GPIO_DEBUG("power gpio register %s %d\n",p->name,p->count);

	p_gpio_node = p->pnode;
	POWER_GPIO_COUNT = p->count;

	init_request_gpio();
	gpio_input_init();
	p->power_gpio_boot_init(p);
}

int power_gpio_unregister(struct power_gpio_oem * p)
{
	return 0;
}

static int __init power_gpio_init(struct power_gpio_oem * p)
{
	int i, ret = 0;
	struct proc_dir_entry *root_entry;
	struct proc_dir_entry *entry;

	root_entry = proc_mkdir(PROC_NAME, &proc_root);

	if(root_entry) {
		for(i = 0; i < POWER_ITEM_MAX; i++) {
			entry = create_proc_entry(s_node_names[i], 0666, root_entry);
			if(entry) {
				entry->write_proc = power_gpio_proc_write;
				entry->read_proc = power_gpio_proc_read;
				entry->data = (void*)s_node_names[i];
			}
		}
	} else {
		printk(KERN_ERR "power_gpio_init init err .\n");
		ret = -1;
	}

	power_gpio_init_common();
	return ret;
}

static void __exit power_gpio_exit(void)
{
	remove_proc_entry(PROC_NAME, &proc_root);
}

fs_initcall(power_gpio_init);
module_exit(power_gpio_exit);

MODULE_DESCRIPTION("power_gpio config proc file");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("urbest, inc.");

