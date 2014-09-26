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


static struct gpio_node d105_gpio[]={	
	{
		.name="d105_eth",
		.pin=EXYNOS4X12_GPM3(1),
		.out_value=1,
	} ,
	{
		.name="d105_usb_power",
		.pin=EXYNOS4_GPK1(1),
		.out_value=1,
	},
} ;

static struct oem_gpio_nodes d105_gpio_nodes={
	.nodes=d105_gpio,
	.count=ARRAY_SIZE(d105_gpio),
	.ut_model="D105",
	.dir_name="D105_power",
};

static int __init d105_gpio_cfg_modinit(void)
{	

	if(strcmp(g_selected_utmodel,"D105"))
		return 0;
	printk("***D105 gpio init\n");

	register_oem_gpio_cfg(&d105_gpio_nodes);

	return 0;
}

static void __exit d105_gpio_cfg_exit(void)
{
	if(strcmp(g_selected_utmodel,"D105"))
		return ;
	unregister_oem_gpio_cfg(&d105_gpio_nodes);
}

late_initcall(d105_gpio_cfg_modinit);
module_exit(d105_gpio_cfg_exit);

MODULE_LICENSE("GPL");

