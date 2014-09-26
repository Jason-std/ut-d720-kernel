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
#include <linux/wakelock.h>

#define DOCK_SEC_HEADSET_4POLE (0x01 << 0)
#define SEC_HEADSET_3POLE (0x01 << 1)
#define set_irq_type   irq_set_irq_type		 
#define	DOCK_SEC_JACK_NO_DEVICE		0x0
#define	DOCK_SEC_HEADSET_3POLE		0x01 << 1
#ifdef CONFIG_SWITCH_4212_3GMicSw
#define DOCK_HP_DETECT_PIN EXYNOS4_GPX1(3)//EXYNOS4_GPX2(2)
#define PLUG_IN 0
#define PLUG_OUT 1
#endif

#ifdef CONFIG_SWITCH_4212_HEADSET
#define DOCK_HP_DETECT_PIN EXYNOS4_GPX2(2)
#endif
extern char g_selected_utmodel[];
//extern int heset_dev_seted;
int DOCK_KEYBOARD_FLAG=0;
EXPORT_SYMBOL(DOCK_KEYBOARD_FLAG);

static struct work_struct dock_insert_work;
static struct work_struct adc_work;
#define keybd_suspend
#ifdef keybd_suspend
static struct wake_lock	keybd_wake_lock;
#endif
static void detect_battery_cap(unsigned long data);
static struct timer_list adc_read_timer = TIMER_INITIALIZER(detect_battery_cap, 0, 0);

struct switch_dev *headset_sdev_key = NULL;
extern struct switch_dev * get_headset_sdev(void);
static struct platform_device dock_keyboard;
static struct s3c_adc_client	*dock_adc_client = NULL;

void set_keystate_boot_keyboard(void){
    schedule_work(&dock_insert_work);
}
EXPORT_SYMBOL(set_keystate_boot_keyboard);

static void dock_insert_handler(struct work_struct* work)
{
	  int state_flag;
    int head_level = 0;
    printk("=========%s============\n",__FUNCTION__);
    
	state_flag=gpio_get_value(EXYNOS4_GPX0(3));
    head_level=gpio_get_value(DOCK_HP_DETECT_PIN);
    headset_sdev_key = get_headset_sdev();

    if(0 == state_flag)//insert
    {
		    printk("++++++++++++++dock keyboard On++++++++++++++++\n");
    		DOCK_KEYBOARD_FLAG=1;
			
			// hub2 reset
			gpio_direction_output(GPIO_USB_HUB2_RESET, 1);
			udelay(10);
			gpio_direction_output(GPIO_USB_HUB2_RESET, 0);
			
    		gpio_direction_output(EXYNOS4212_GPM4(1),1);
    		gpio_direction_output(EXYNOS4_GPL2(3),1);
    	 	
#ifdef CONFIG_SWITCH_4212_HEADSET
    	      if(0 == head_level)//no headset
#else
    	      if(PLUG_OUT == head_level)//no headset
#endif
    	      {
    	        switch_set_state(headset_sdev_key, DOCK_SEC_HEADSET_4POLE);
    	        gpio_direction_output(EXYNOS4212_GPM1(4),0);//spk off
    	        gpio_direction_output(EXYNOS4_GPL2(4),1);
    	      }
       #ifdef keybd_suspend
         wake_lock(&keybd_wake_lock);
	  #endif
    }else{//out
		    printk("++++++++++++++dock keyboard Off++++++++++++++++\n");
		    DOCK_KEYBOARD_FLAG=0;
			
			gpio_direction_output(GPIO_USB_HUB2_RESET, 1);
			udelay(10);
			
		    gpio_direction_output(EXYNOS4212_GPM4(1),0);
		    gpio_direction_output(EXYNOS4_GPL2(3),0);
#ifdef CONFIG_SWITCH_4212_HEADSET
    	      if(0 == head_level)//no headset
#else
    	      if(PLUG_OUT == head_level)//no headset
#endif     
    	      {
    	           switch_set_state(headset_sdev_key, 0);
    	           gpio_direction_output(EXYNOS4212_GPM1(4),1);
    	           gpio_direction_output(EXYNOS4_GPL2(4),0);
    	      }
        #ifdef keybd_suspend
	    wake_lock_timeout(&keybd_wake_lock, 1000 * 2);
	    #endif
    }
}

static void adc_read_fun(struct work_struct* work)
{
    int adc_val = 0;

    adc_val = s3c_adc_read(dock_adc_client, 2);
    printk("========%s=======adc_val[%d]=========\n",__FUNCTION__, adc_val);
    //when the battery voltage lower than 3.1V, keyboard get power from MID
    if(3100 > adc_val){
        //do something
    }
}

static void detect_battery_cap(unsigned long data)//read 
{
    printk("--------%s----------\n",__FUNCTION__);
    schedule_work(&adc_work);
    mod_timer(&adc_read_timer, jiffies + 60*HZ);
}

static irqreturn_t dock_keyboard_irq_handler(int irq, void *dev_id)
{
	printk("%s ,irq:%d\n",__FUNCTION__,irq);
#if 1 //raymanfeng
	//gpio_direction_output(GPIO_USB_HUB2_RESET, 0);
	schedule_work(&dock_insert_work);
#endif
	return IRQ_HANDLED;
}


static int dock_keyboard_suspend(struct platform_device *pdev, pm_message_t state)
{
	printk("liao ++++++++++%s++++++++++\n",__func__);
	//gpio_direction_output(GPIO_USB_HUB2_RESET, 1);
	if (DOCK_KEYBOARD_FLAG){
	 //   gpio_direction_output(GPIO_USB_HUB2_RESET, 1); //raymanfeng
		gpio_direction_output(EXYNOS4_GPL2(4),0);
		gpio_direction_output(EXYNOS4_GPL2(3),0);
	}
	return 0;
}

static int dock_keyboard_resume(struct platform_device *pdev)
{
	printk("liao ++++++++++%s++++++++++\n",__func__);
	
	if (DOCK_KEYBOARD_FLAG){
	//	gpio_direction_output(GPIO_USB_HUB2_RESET, 0); //raymanfeng
		gpio_direction_output(EXYNOS4_GPL2(4),1);
		gpio_direction_output(EXYNOS4_GPL2(3),1);
	}
	return 0;
}

static int  dock_keyboard_probe(struct platform_device *pdev)
{
	  int ret;
	  int state_flag;
	  
	  printk("%s\n",__FUNCTION__);

	  INIT_WORK(&dock_insert_work, dock_insert_handler);
	  INIT_WORK(&adc_work, adc_read_fun);
	  
	  dock_adc_client = s3c_adc_register(&dock_keyboard, NULL, NULL, 0);
  	adc_read_timer.expires =  jiffies + 10*HZ;
	  add_timer(&adc_read_timer);
	
		s3c_gpio_cfgpin(EXYNOS4_GPX0(3), S3C_GPIO_SPECIAL(0x0f)); //Eint3
		s3c_gpio_setpull(EXYNOS4_GPX0(3), S3C_GPIO_PULL_NONE);
		gpio_set_debounce(EXYNOS4_GPX0(3), 140);
		ret = request_irq(IRQ_EINT(3),dock_keyboard_irq_handler,IRQF_TRIGGER_RISING |IRQF_TRIGGER_FALLING,"dock_keyboard",NULL);
		if(ret<0){
			  printk("IRQ_EINT3 request irq ERROR!!!");
			  return -1;
		}
			  
	  state_flag=gpio_get_value(EXYNOS4_GPX0(3));
	  if(state_flag==0){
		    DOCK_KEYBOARD_FLAG=1;
		    gpio_direction_output(EXYNOS4212_GPM4(1),0);
		    gpio_direction_output(EXYNOS4_GPL2(3),1);
	 	    gpio_direction_output(EXYNOS4_GPL2(4),1);
		    gpio_direction_output(EXYNOS4212_GPM1(4),0);
		    set_keystate_boot_keyboard();
      }

		return 0;
}

static int dock_keyboard_remove(struct platform_device *pdev)
{
	return  0;
}

static struct platform_driver dock_keyboard_device_driver = {
	.probe		= dock_keyboard_probe,
	.remove		= dock_keyboard_remove,
	.suspend		= dock_keyboard_suspend,
	.resume		= dock_keyboard_resume,
	.driver		= {
		.name	= "dock_keyboard",
		.owner	= THIS_MODULE,
	}
};

static struct platform_device dock_keyboard = {
	.name	= "dock_keyboard",
	.id		= -1,
};

extern int g_dock_keypad_type;
static int __init dock_keyboard_modinit(void)
{
	if (!g_dock_keypad_type)
		{
			return 0;
		}
#ifdef keybd_suspend	
	wake_lock_init(&keybd_wake_lock, WAKE_LOCK_SUSPEND,"keybd wake lock");
#endif
	if((strncmp(g_selected_utmodel, "s101", strlen("s101")) == 0)|| (strncmp(g_selected_utmodel, "s901", strlen("s901")) == 0))
	{
	    platform_device_register(&dock_keyboard);
	    return platform_driver_register(&dock_keyboard_device_driver);
	}
	
	return 0;
}

static void __exit dock_keyboard_exit(void)
{
	platform_driver_unregister(&dock_keyboard_device_driver);
	platform_device_unregister(&dock_keyboard);
	#ifdef keybd_suspend
	wake_lock_destroy(&keybd_wake_lock);
	#endif
}

//module_init(dock_keyboard_modinit);
late_initcall_sync(dock_keyboard_modinit);
module_exit(dock_keyboard_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Urbest");
MODULE_DESCRIPTION("Urbest dock_keyboard");
MODULE_ALIAS("platform:dock_keyboard");


/*
late_initcall(dock_keyboard_modinit);

static void __exit dock_keyboard_exit(void)
{
	gpio_free(DOCK_KEYBOARD_DETECT_PIN);
}
module_exit(dock_keyboard_exit);
*/

