/*
 * Driver for keys on GPIO lines.
 * Urbest
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <asm/gpio.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

#include <linux/io.h>
#include <mach/regs-clock.h>
#include <plat/pm.h>
#include <plat/adc.h>



extern char g_selected_utmodel[];

static struct timer_list timer;
static struct input_dev * input;


struct st_s3c_key{
	int key_code;
	uint key_pin;
	int key_history_flg;
};

static int key_count=0;
static struct st_s3c_key * s3c_key_para;
static int * s3c_Keycode;

static int s3c_Keycode_s502[] = {
	KEY_F7, 
	KEY_F8, 
};
static struct st_s3c_key s3c_key_para_s502[] = {
		{ KEY_F7, EXYNOS4_GPX2(4), 0},
		{ KEY_F8, EXYNOS4_GPX2(6), 0},	
};


static int s3c_Keycode_d105[] = {
	KEY_F8, 
	KEY_HOME,
};
static struct st_s3c_key s3c_key_para_d105[] = {
		{ KEY_F8, EXYNOS4_GPX1(7), 1},
		{ KEY_HOME, EXYNOS4_GPX1(6), 1},
};

static int s3c_Keycode_d720[] = {
	KEY_F1, 
	KEY_F2,
	KEY_MENU,
	KEY_BACK,
};
static struct st_s3c_key s3c_key_para_d720[] = {
		{ KEY_F1, EXYNOS4_GPX1(0), 1},
		{ KEY_MENU, EXYNOS4_GPX0(7), 1},
		{ KEY_F2, EXYNOS4_GPX0(6), 1},
		{ KEY_BACK, EXYNOS4_GPX0(5), 1},
};

static void s3cbutton_timer_handler(unsigned long data)
{
	int flag;
	int i;
	for(i=0; i<key_count; i++) 
	{
		flag = gpio_get_value(s3c_key_para[i].key_pin);
		if(flag != s3c_key_para[i].key_history_flg) 
		{
			if(flag) 
			{
			    input_report_key(input, s3c_key_para[i].key_code, 0);
			    printk("s3c-button  key up!i=%d\n",i);
 			}  
			else  
			{
			     input_report_key(input, s3c_key_para[i].key_code, 1);
			     printk("s3c-button  key down!! i=%d\n",i);
			}
			s3c_key_para[i].key_history_flg= flag;

			input_sync(input);
		}
	}
	mod_timer(&timer,jiffies + HZ/10);
}


static int checkboard()
{
	if(strstr(g_selected_utmodel,"s502")){
		s3c_key_para=s3c_key_para_s502;
		s3c_Keycode=s3c_Keycode_s502;
		key_count=ARRAY_SIZE(s3c_key_para_s502);
	}else if(strstr(g_selected_utmodel,"D105")){
		s3c_key_para=s3c_key_para_d105;
		s3c_Keycode=s3c_Keycode_d105;
		key_count=ARRAY_SIZE(s3c_key_para_d105);
	}else if(strstr(g_selected_utmodel,"d720")){
		s3c_key_para=s3c_key_para_d720;
		s3c_Keycode=s3c_Keycode_d720;
		key_count=ARRAY_SIZE(s3c_key_para_d720);
	}else{
		s3c_key_para=NULL;
		s3c_Keycode=NULL;
		key_count=0;
	}
	return key_count;
}

static int s3c_button_probe(struct platform_device *pdev)
{
	int i;

	checkboard();
	if(key_count==0){
		printk("not need this driver,return\n");
		return 0;
	}

	for(i=0; i<key_count; i++)  {
		gpio_request(s3c_key_para[i].key_pin, "oem-button");
		s3c_gpio_setpull(s3c_key_para[i].key_pin, S3C_GPIO_PULL_UP);
		gpio_direction_input(s3c_key_para[i].key_pin);
	}

	input = input_allocate_device();
	if(!input) 
		return -ENOMEM;

	set_bit(EV_KEY, input->evbit);
	//set_bit(EV_REP, input->evbit);	/* Repeat Key */

	for(i = 0; i < key_count; i++)
		set_bit(s3c_key_para[i].key_code, input->keybit);

	input->name = "oem-button";
	input->phys = "oem-button/input0";

	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;

	input->keycode = s3c_Keycode;
	input->keycodemax  = key_count;
	input->keycodesize = sizeof(s3c_Keycode[0]);

	if(input_register_device(input) != 0)
	{
		printk(KERN_ERR"oem-button input register device fail!!\n");
		input_free_device(input);
		return -ENODEV;
	}

	/* Scan timer init */
	init_timer(&timer);
	timer.function = s3cbutton_timer_handler;

	timer.expires = jiffies + (HZ/20);
	add_timer(&timer);

	printk(KERN_INFO"oem button Initialized!!\n");

	return 0;
}

static int s3c_button_remove(struct platform_device *pdev)
{
	int i;
	if(key_count==0){
		return 0;
	}

	input_unregister_device(input);
	del_timer(&timer);
	for(i=0; i<key_count; i++)  {
		gpio_free(s3c_key_para[i].key_pin);
	}

	return  0;
}
/*
static int s3c_button_suspend(struct platform_device *pdev, pm_message_t state)
{
	
}

static int s3c_button_resume(struct platform_device *pdev)
{

}
*/
static struct platform_driver s3c_button_device_driver = {
	.probe		= s3c_button_probe,
	.remove		= s3c_button_remove,
//	.suspend	= s3c_button_suspend,
//	.resume		= s3c_button_resume,
	.driver		= {
		.name	= "oem-btn",
		.owner	= THIS_MODULE,
	}
};


static struct platform_device s3c_device_button = {
	.name	= "oem-btn",
	.id		= -1,
};

static int __init s3c_button_init(void)
{
	platform_device_register(&s3c_device_button); 
	return platform_driver_register(&s3c_button_device_driver);
}

static void __exit s3c_button_exit(void)
{
	platform_driver_unregister(&s3c_button_device_driver);
	platform_device_unregister(&s3c_device_button);
}

module_init(s3c_button_init);
module_exit(s3c_button_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("raymanfeng <raymanfeng@hotmail.com>");
MODULE_DESCRIPTION("Keyboard driver for s3c button.");
MODULE_ALIAS("platform:s3c-button");
