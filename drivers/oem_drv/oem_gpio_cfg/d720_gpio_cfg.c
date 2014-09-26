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
#include <urbetter/check.h>
#include "oem_gpio_cfg.h"


extern char g_selected_utmodel[];

static struct gpio_node d720_gpio[]={
	{
		.name="rf_en",
		.pin=EXYNOS4212_GPM3(5),
		.out_value=1,
	}	
} ;

static struct oem_gpio_nodes d720_gpio_nodes={
	.nodes=d720_gpio,
	.count=ARRAY_SIZE(d720_gpio),
	.ut_model="d720",
	.dir_name="d720_power",
};
static int __init d720_gpio_cfg_modinit(void)
{	

	if(!CHECK_UTMODEL("d720"))
		return 0;
	printk("***d720 gpio init\n");

	register_oem_gpio_cfg(&d720_gpio_nodes);
	return 0;
}

static void __exit d720_gpio_cfg_exit(void)
{
	if(!CHECK_UTMODEL("d720"))
		return ;
	unregister_oem_gpio_cfg(&d720_gpio_nodes);
}

late_initcall(d720_gpio_cfg_modinit);
module_exit(d720_gpio_cfg_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Leslie_Xiao");
MODULE_DESCRIPTION("Urbest d720 RF POWER driver");

