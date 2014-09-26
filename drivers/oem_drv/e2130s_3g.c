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
#include <mach/io.h>
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

#include <plat/sdhci.h>
#include <plat/devs.h>

static unsigned int GSM_POWER_EN;

static unsigned int GSM_PRE_STATE;

extern  int gsm_power_switch;
extern void sdhci_s3c_notify_change(struct platform_device *dev, int state);

struct work_struct gsm_power;
static struct timer_list gsm_timer;
static void gsm_power_on(void)
{	
	gpio_direction_output(GSM_POWER_EN,  1);
	msleep(100);
	sdhci_s3c_notify_change(&s3c_device_hsmmc1, 1);
	msleep(7000);
	sdhci_s3c_notify_change(&s3c_device_hsmmc1, 1);
	return;
}

static void gsm_power_off(void)
{
	gpio_direction_output(GSM_POWER_EN,0);
	msleep(1000);
	sdhci_s3c_notify_change(&s3c_device_hsmmc1, 0);
	return;
}


static void  handle_gsm_power(struct work_struct* work)
{
	if(GSM_PRE_STATE)
		gsm_power_on();
	else
		gsm_power_off();
}

static void detect_gsm_loop(unsigned long data)
{
	int all_gsm_state=0;

	if(gsm_power_switch!=GSM_PRE_STATE){
		GSM_PRE_STATE=gsm_power_switch;
		schedule_work(&gsm_power);
		mod_timer(&gsm_timer, jiffies + msecs_to_jiffies(8000));
	}
	mod_timer(&gsm_timer, jiffies + msecs_to_jiffies(1000));
}



static int gsm_probe(struct platform_device *dev)
{
	printk("%s(); + \n", __func__);
	GSM_PRE_STATE=gsm_power_switch;
	INIT_WORK(&gsm_power,handle_gsm_power);	
	gsm_timer.function=detect_gsm_loop;
	gsm_timer.expires=jiffies+msecs_to_jiffies(1000);
	init_timer(&gsm_timer);
	add_timer(&gsm_timer);
	
	printk("%s(); - \n", __func__);

	return 0;

}

static void gsm_shutdown(struct platform_device *dev)
{
	return;
}

static void gsm_suspend(struct device *dev)
{
	printk("%s(); + \n", __func__);
	
	printk("%s(); - \n", __func__);
}

static void gsm_resume(struct device *dev)
{
	printk("%s(); + \n", __func__);
	
	printk("%s(); - \n", __func__);
}

static const struct dev_pm_ops gsm_pm_ops = {
	.suspend_noirq = gsm_suspend,
	.resume_noirq = gsm_resume,
};


static struct platform_driver gsm_driver = {
	.probe = gsm_probe,
	.shutdown = gsm_shutdown,	
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "gsm-module",
	},
};



static bool check_gsm_modules(void)
{
	extern char g_selected_gsmd[32];

	printk("%s(); %s \n",__func__, g_selected_gsmd);

	GSM_POWER_EN=EXYNOS4212_GPM1(5);

	if( !strcmp(g_selected_gsmd, "e2130s")) {
		return true;
	}

	return false;
}

static int __init gsm_init(void)
{
	if(check_gsm_modules())
		return platform_driver_register(&gsm_driver);
	else 
		return -1;
}


static void __exit gsm_exit(void)
{
	platform_driver_unregister(&gsm_driver);
}

module_init(gsm_init);
module_exit(gsm_exit);

MODULE_AUTHOR("urbest system team");
MODULE_DESCRIPTION("GSM detect driver");
MODULE_LICENSE("GPL");
