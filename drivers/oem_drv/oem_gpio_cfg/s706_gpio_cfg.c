#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/switch.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <mach/hardware.h>
#include <asm/io.h>
//#include <mach/io.h>
#include <mach/gpio.h>
#include <plat/adc.h>

#include <linux/delay.h>

#include <linux/io.h>
#include <plat/map-base.h>
#include <linux/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-clock.h>
#include <linux/input.h>
#include <linux/irq.h>


#include <plat/gpio-cfg.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>

#include <linux/proc_fs.h>

#include <linux/gpio.h>
#include <linux/serial_core.h>
#include <plat/devs.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-serial.h>
#include <mach/gpio-ut4x12.h>
#include <plat/cpu.h>

#include "oem_gpio_cfg.h"

extern char g_selected_utmodel[];

#define S706_POWER_NAME_SFZ "s706_power_sfz"
#define S706_POWER_NAME_ZW "s706_power_zw"
#define S706_POWER_NAME_GSM "s706_power_gsm"
#define S706_PROC_NAME "s706_power"

#define S706_POWER_SFZ1 EXYNOS4_GPK1(0)  // for new board
#define S706_POWER_ZW1 EXYNOS4_GPK1(2)  // for new board

#define S706_POWER_SFZ EXYNOS4_GPL2(3)
#define S706_POWER_ZW EXYNOS4_GPL2(4)
#define S706_POWER_GSM EXYNOS4X12_GPM1(5)

static int s706_sfz_writeproc(struct file *file,const char *buffer,
                           unsigned long count, void *data)
{
	int value; 
	value = 0; 
	sscanf(buffer, "%d", &value);
	gpio_direction_output(S706_POWER_SFZ, value);
	gpio_set_value(S706_POWER_SFZ, value);
	s5p_gpio_set_drvstr(S706_POWER_SFZ, S5P_GPIO_DRVSTR_LV4);

	gpio_direction_output(S706_POWER_SFZ1, value);
	gpio_set_value(S706_POWER_SFZ1, value);
	s5p_gpio_set_drvstr(S706_POWER_SFZ1, S5P_GPIO_DRVSTR_LV4);

	return count;
}

static int s706_zw_writeproc(struct file *file,const char *buffer,
                           unsigned long count, void *data)
{
	int value; 
	value = 0; 
	sscanf(buffer, "%d", &value);
	gpio_direction_output(S706_POWER_ZW, value);
	gpio_set_value(S706_POWER_ZW, value);
	s5p_gpio_set_drvstr(S706_POWER_ZW, S5P_GPIO_DRVSTR_LV4);

	gpio_direction_output(S706_POWER_ZW1, value);
	gpio_set_value(S706_POWER_ZW1, value);
	s5p_gpio_set_drvstr(S706_POWER_ZW1, S5P_GPIO_DRVSTR_LV4);

	return count;
}

static int s706_gsm_writeproc(struct file *file,const char *buffer,
                           unsigned long count, void *data)
{
	int value; 
	value = 0; 
	sscanf(buffer, "%d", &value);
	gpio_direction_output(S706_POWER_GSM, value);
	gpio_set_value(S706_POWER_GSM, value);
	s5p_gpio_set_drvstr(S706_POWER_GSM, S5P_GPIO_DRVSTR_LV4);

	gpio_direction_output(EXYNOS4X12_GPM1(2), 1);//AP_wakeup_3g
	gpio_direction_output(EXYNOS4X12_GPM1(3), 1);//  3g_RST

/*	if(value){
		printk("reset this gsm\n");
		gpio_direction_output(EXYNOS4X12_GPM1(3), 1);
		gpio_set_value(EXYNOS4X12_GPM1(3), 1);
		s5p_gpio_set_drvstr(EXYNOS4X12_GPM1(3), 1);
		msleep(200);
		s5p_gpio_set_drvstr(EXYNOS4X12_GPM1(3), 0);
	} */

	return count;
}



static struct gpio_node s706_gpio[]={
	{
		.name=S706_POWER_NAME_SFZ,
		.pin=S706_POWER_SFZ1,
		.out_value=0,
		.writeproc=s706_sfz_writeproc,
	},
	{
		.name=S706_POWER_NAME_ZW,
		.pin=S706_POWER_ZW1,
		.out_value=0,
		.writeproc=s706_zw_writeproc,
	},
	{
		.name=S706_POWER_NAME_GSM,
		.pin=S706_POWER_GSM,
		.out_value=0,
		.writeproc=s706_gsm_writeproc,
	} 
} ;

static struct oem_gpio_nodes s706_gpio_nodes={
	.nodes=s706_gpio,
	.count=ARRAY_SIZE(s706_gpio),
	.ut_model="s706",
	.dir_name=S706_PROC_NAME,
};
static int __init s706_gpio_cfg_modinit(void)
{	

	if(!strstr(g_selected_utmodel,"s706"))
		return 0;
	printk("***s706 gpio init\n");

	gpio_direction_output(EXYNOS4_GPK1(2), 1); // usb host vdd_5v
	gpio_direction_output(EXYNOS4_GPL2(2), 1);//HUB_RST2/GPL2_2
	msleep(100);
	gpio_direction_output(EXYNOS4_GPL2(2), 0);//HUB_RST2/GPL2_2
	msleep(5);	
	gpio_direction_output(EXYNOS4X12_GPM1(2), 1);//AP_wakeup_3g
	gpio_direction_output(EXYNOS4X12_GPM1(3), 1);//  3g_RST
//	msleep(200);
//	gpio_direction_output(EXYNOS4X12_GPM1(3), 0);//  3g_RST

	gpio_direction_output(S706_POWER_SFZ1, 0);
	gpio_set_value(S706_POWER_SFZ1, 0);
	s5p_gpio_set_drvstr(S706_POWER_SFZ1, S5P_GPIO_DRVSTR_LV4);

	gpio_direction_output(S706_POWER_ZW1, 0);
	gpio_set_value(S706_POWER_ZW1, 0);
	s5p_gpio_set_drvstr(S706_POWER_ZW1, S5P_GPIO_DRVSTR_LV4);

	gpio_direction_output(S706_POWER_SFZ, 0);
	gpio_set_value(S706_POWER_SFZ, 0);
	s5p_gpio_set_drvstr(S706_POWER_SFZ, S5P_GPIO_DRVSTR_LV4);

	gpio_direction_output(S706_POWER_ZW, 0);
	gpio_set_value(S706_POWER_ZW, 0);
	s5p_gpio_set_drvstr(S706_POWER_ZW, S5P_GPIO_DRVSTR_LV4);

	register_oem_gpio_cfg(&s706_gpio_nodes);
	return 0;
}

static void __exit s706_gpio_cfg_exit(void)
{
	if(!strstr(g_selected_utmodel,"s706"))
		return ;
	unregister_oem_gpio_cfg(&s706_gpio_nodes);
}

late_initcall(s706_gpio_cfg_modinit);
module_exit(s706_gpio_cfg_exit);

MODULE_LICENSE("GPL");

