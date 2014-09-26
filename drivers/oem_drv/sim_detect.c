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


static unsigned int SIM_DET;
static unsigned int SIM_DET_IRQ;

static unsigned int GSM_POWER_EN;
static unsigned int GSM_PWD;
static unsigned int GSM_WAKE_MODULE;

static unsigned int GSM_WAKE_HOST;
static unsigned int GSM_WAKE_HOST_IRQ;


//#define GSM_DETECT_INTERVAL    (msecs_to_jiffies(10000))
#define GSM_DETECT_INTERVAL    (msecs_to_jiffies(1000))


//add by albert_lee 2013-01-14 start
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
static struct early_suspend    gsm_det_early_suspend;
static DEFINE_MUTEX(gsm_mutex); //  maybe redundancy, todo by albert
static void gsm_early_suspend(struct early_suspend *h);
static void gsm_late_resume(struct early_suspend *h);
#endif
//add by albert_lee 2013-01-14 end

#define GSM_LOOP_CHECK 1
#if GSM_LOOP_CHECK
extern int gsm_tty_flag;
#else
static int gsm_tty_flag = 0;
#endif
extern  int gsm_power_switch;

struct work_struct sim_detect_handle,gsm_power_on,gsm_power_off;
volatile int sim_state;
static struct timer_list gsm_timer;

static int g_gsm_suspend_flag = 0;

static int g_gsm_power_state = 0;

extern char g_selected_utmodel[32];
unsigned char g_gsm_poweroff_state=0;
static void gsm_sleep_in(void)
{
	gpio_direction_output(GSM_WAKE_MODULE,	GPIO_LEVEL_HIGH);
}

static void gsm_wake_out(void)
{
	gpio_direction_output(GSM_WAKE_MODULE,	GPIO_LEVEL_LOW);
}

static void mu509_power_on(void)
{	
#if !GSM_LOOP_CHECK
	gsm_tty_flag = 1;
#endif	
	gpio_direction_output(GSM_POWER_EN,  GPIO_LEVEL_HIGH);


	msleep(100);
	gpio_direction_output(GSM_PWD,	GPIO_LEVEL_LOW);
	gpio_direction_output(GSM_PWD,	GPIO_LEVEL_HIGH);
	msleep(200);
	gpio_direction_output(GSM_PWD,	GPIO_LEVEL_LOW);

	gsm_wake_out();

	g_gsm_power_state = 1;
	
	return;
}

static void mu509_power_off(void)
{
#if !GSM_LOOP_CHECK
	gsm_tty_flag = 0;
#endif	

	gpio_direction_output(GSM_POWER_EN,	GPIO_LEVEL_LOW);

	gpio_direction_output(GSM_PWD,	GPIO_LEVEL_LOW);
	gpio_direction_output(GSM_PWD,	GPIO_LEVEL_HIGH);
	msleep(1000);
	gpio_direction_output(GSM_PWD,	GPIO_LEVEL_LOW);
	msleep(600);
	g_gsm_power_state = 0;
	g_gsm_poweroff_state=1;
	return;
}

static void  handle_mu509_power_on(struct work_struct* work)
{
	mu509_power_on();
}

static void  handle_mu509_power_off(struct work_struct* work)
{
	mu509_power_off();
}

static int g_all_gsm_state = 0;
static void detect_gsm_loop(unsigned long data)
{
	int all_gsm_state=0;
	int delay_time=GSM_DETECT_INTERVAL;
 	all_gsm_state=(gsm_power_switch<<2)|(gsm_tty_flag<<1)|(sim_state<<0);
	

	if (g_gsm_suspend_flag)
		{
 			mod_timer(&gsm_timer, jiffies + delay_time);
			return;
		}

	if (g_all_gsm_state == all_gsm_state)
		{
			mod_timer(&gsm_timer, jiffies + delay_time);
			return;
		}
	else
		{
			g_all_gsm_state = all_gsm_state;
		}
	
	printk("%s,all_gsm_state:0x%x\n",__FUNCTION__,all_gsm_state);
	
	switch(all_gsm_state){
		case 0x0:				
			break;
		case 0x1:
			break;
		case 0x2:
			printk("turn off gsm module\n");
			schedule_work(&gsm_power_off);
			break;
		case 0x3:			
			printk("turn off gsm module\n");
			schedule_work(&gsm_power_off);
			break;
		case 0x4:			
			break;
			
		case 0x5:		
			printk("turn on gsm module\n");
			schedule_work(&gsm_power_on);
			break;
			
		case 0x6:
			printk("turn off gsm module\n");
			schedule_work(&gsm_power_off);
			break;
		case 0x7:
			break;
		default:
			break;
		
		}
	
	mod_timer(&gsm_timer, jiffies + delay_time);
}

static void handle_sim_change(struct work_struct* work)
{
	int val;
	val=gpio_get_value(SIM_DET);// 1  in 0 out
	
	printk("%s,val:%d,gsm_tty_flag:%d\n",__FUNCTION__,val,gsm_tty_flag);
	if (val) {
		sim_state=1;
		if (gsm_tty_flag==1) {			
			mu509_power_off();
			mod_timer(&gsm_timer, jiffies + GSM_DETECT_INTERVAL);
		}
	} else {
		sim_state=0;
		mu509_power_off();
	}
		
	return;
}

static irqreturn_t hand_sim_state_irq(int irq, void *dev_id)
{
	schedule_work(&sim_detect_handle);
    
	return IRQ_HANDLED;
}

static irqreturn_t hand_gsm_wakup_irq(int irq, void *dev_id)
{
	gsm_wake_out();
	return IRQ_HANDLED;
}

static int gsm_probe(struct platform_device *dev)
{
	int ret=0;

	printk("%s(); + \n", __func__);
	
	INIT_WORK(&sim_detect_handle,handle_sim_change);
	INIT_WORK(&gsm_power_on,handle_mu509_power_on);	
	INIT_WORK(&gsm_power_off,handle_mu509_power_off);
	gsm_timer.function=detect_gsm_loop;
	gsm_timer.expires=jiffies+GSM_DETECT_INTERVAL;
	init_timer(&gsm_timer);
	add_timer(&gsm_timer);
	
	//SIM init
	if(!strcmp(g_selected_utmodel,"d720")){
		sim_state=1;
		}else{
	s3c_gpio_cfgpin(SIM_DET, S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(SIM_DET, S3C_GPIO_PULL_UP);
	ret=request_irq(SIM_DET_IRQ,hand_sim_state_irq,IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING,"SIM_DETECT", NULL);
	if(ret<0) {
		printk("request IRQ_SIM_PLUG irq error ret:%d\n",ret);
		}
	}
	// GSM wake up EINT handle
	s3c_gpio_cfgpin(GSM_WAKE_HOST, S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(GSM_WAKE_HOST, S3C_GPIO_PULL_UP);
	ret=request_irq(GSM_WAKE_HOST_IRQ, hand_gsm_wakup_irq, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, "3G_WAKE", NULL);
	if(ret<0) {
		printk("request IRQ_3G_WUP irq error ret:%d\n",ret);
		goto irq1_err;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	gsm_det_early_suspend.suspend = gsm_early_suspend;
	gsm_det_early_suspend.resume = gsm_late_resume;
	gsm_det_early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 5;
	register_early_suspend(&gsm_det_early_suspend);
#endif

	printk("%s(); - \n", __func__);

	return 0;

irq1_err:
	free_irq(IRQ_SIM_PLUG, NULL);
	return ret;
}

static void gsm_shutdown(struct platform_device *dev)
{
	printk("%s(); + \n", __func__);

	if(gsm_tty_flag) {
		mu509_power_off();
	}
	printk("%s(); - \n", __func__);
	
//	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
/*
	add by albert 2013-01-17

	only for these MID which have dock's keypad.

	MU509 sleep before CPU suspend
*/
static bool check_soft_suspend_usb(void)
{
	extern char g_selected_utmodel[32];

	if (!strncmp(g_selected_utmodel, "s901", strlen("s901"))
		|| !strncmp(g_selected_utmodel, "s101", strlen("s101"))
	) {
		return true;
	} 

	return false;
}


static void gsm_early_suspend(struct early_suspend *h)
{

	printk("%s(); + \n", __func__);
	g_gsm_suspend_flag = 1;
	del_timer(&gsm_timer);
	printk("g_gsm_power_state = %d\n",g_gsm_power_state);
	if(!strcmp(g_selected_utmodel,"s501"))
	{
			gsm_sleep_in();	
	}else{
	if (!g_gsm_power_state)
		gsm_sleep_in();
	}
/*	
	if((gsm_power_switch>0)&& (sim_state>0) && check_soft_suspend_usb()) {
		mutex_lock(&gsm_mutex);
		gsm_sleep_in();
		mutex_unlock(&gsm_mutex);
	}
*/	
    printk("liao test g_gsm_poweroff_state=%d\r\n",g_gsm_poweroff_state);

	if(g_gsm_poweroff_state)
	{
	  msleep(4000);
	  g_gsm_poweroff_state=0;
	}
	

	printk("%s(); - \n", __func__);

}

static void gsm_late_resume(struct early_suspend *h)
{
	printk("%s(); + \n", __func__);
	g_gsm_suspend_flag = 0;
/*	
	if((gsm_power_switch>0)&& (sim_state>0) && check_soft_suspend_usb()) {
		mutex_lock(&gsm_mutex);
		gsm_wake_out();
		mutex_unlock(&gsm_mutex);
	}
*/	
	mod_timer(&gsm_timer, jiffies + 200);
	printk("%s(); - \n", __func__);
}
#else

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
#endif

static struct platform_driver gsm_driver = {
	.probe = gsm_probe,
	.shutdown = gsm_shutdown,	
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "gsm-module",
#ifndef CONFIG_HAS_EARLYSUSPEND 
		   .pm = &gsm_pm_ops,
#endif
	},
};



static bool check_gsm_modules(void)
{
	extern char g_selected_gsmd[32];

	printk("%s(); %s \n",__func__, g_selected_gsmd);

	if (!strncmp(g_selected_utmodel, "d720", strlen("d720"))){
		SIM_DET=EXYNOS4_GPK1(0);
		SIM_DET_IRQ=gpio_to_irq(SIM_DET);

		GSM_POWER_EN=EXYNOS4212_GPM1(5);
		GSM_PWD=EXYNOS4212_GPM3(6);
		GSM_WAKE_MODULE=EXYNOS4212_GPM3(5);

		GSM_WAKE_HOST=EXYNOS4_GPX3(6);
		GSM_WAKE_HOST_IRQ=gpio_to_irq(GSM_WAKE_HOST);
	}else{
		SIM_DET=GPIO_SIM_PLUG;
		SIM_DET_IRQ=IRQ_SIM_PLUG;

		GSM_POWER_EN=GPIO_GSM_POWER_EN;
		GSM_PWD=GPIO_GSM_POWER_ON_OFF;
		GSM_WAKE_MODULE=GPIO_GSM_WAKE_IN;

		GSM_WAKE_HOST=GPIO_3G_WUP;
		GSM_WAKE_HOST_IRQ=IRQ_3G_WUP;
	}

	if( (strncmp(g_selected_gsmd, "mu509", strlen("mu509"))==0
		|| strncmp(g_selected_gsmd, "x9", strlen("x9"))==0
		|| strncmp(g_selected_gsmd, "mc509", strlen("mc509"))==0
		)) {
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
#ifdef CONFIG_HAS_EARLYSUSPEND
	mutex_destroy(&gsm_mutex);
#endif	
	platform_driver_unregister(&gsm_driver);

}

module_init(gsm_init);
module_exit(gsm_exit);

MODULE_AUTHOR("urbest system team");
MODULE_DESCRIPTION("GSM detect driver");
MODULE_LICENSE("GPL");
