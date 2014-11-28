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


#define RF_POWER_NAME "rf_power"
#define RF_POWER_ON_NAME "rf_power_on"
#define RF_PPT_NAME "rf_ptt"
#define RF_PROC_NAME "s502_rf"

#define RF_POWER_GPIO EXYNOS4X12_GPM2(0)
#define RF_POWER_ON_GPIO EXYNOS4X12_GPM2(1)
#define RF_PPT_GPIO EXYNOS4X12_GPM2(2)

static struct gpio_node s502_gpio[]={
	{
		.name=RF_POWER_NAME,
		.pin=RF_POWER_GPIO,
		.out_value=1,
	},
	{
		.name=RF_POWER_ON_NAME,
		.pin=RF_POWER_ON_GPIO,
		.out_value=0,
	},
	{
		.name=RF_PPT_NAME,
		.pin=RF_PPT_GPIO,
		.out_value=0,
	} 
} ;

static struct oem_gpio_nodes s502_gpio_nodes={
	.nodes=s502_gpio,
	.count=ARRAY_SIZE(s502_gpio),
	.ut_model="s502",
	.dir_name=RF_PROC_NAME,
};
static int __init s502_gpio_cfg_modinit(void)
{	

	if(!strstr(g_selected_utmodel,"s502"))
		return 0;
	printk("***s502 gpio init\n");

	register_oem_gpio_cfg(&s502_gpio_nodes);
	return 0;
}

static void __exit s502_gpio_cfg_exit(void)
{
	if(!strstr(g_selected_utmodel,"s502"))
		return ;
	unregister_oem_gpio_cfg(&s502_gpio_nodes);
}

late_initcall(s502_gpio_cfg_modinit);
module_exit(s502_gpio_cfg_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Leslie_Xiao");
MODULE_DESCRIPTION("Urbetter S502 RF POWER driver");

