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

//UT7GM keys define:
//key "back"		-- EINT4 GPH0(4)
//key "power"		-- EINT1 GPH0(1)
//key "menu"		-- EINT28 GPH3(4)
//key "home"		-- EINT29 GPH3(5)
//key "vol up"		-- EINT16 GPH2(0)
//key "vol down"	-- EINT17 GPH2(1)

extern char g_selected_utmodel[];
static struct timer_list timer;
static struct input_dev * input;
static struct s3c_adc_client	*button_ut7gm_client = NULL;
//enum  {
//	BUTTON_END=0,
//	BUTTON_VOLUMEUP,
//	BUTTON_VOLUMEDOWN,
//	BUTTON_KPEQUAL,
//	BUTTON_HOME,
//	BUTTONE_MAX
//};

struct st_s3c_key{
	int key_code;
	uint key_pin;
	int key_history_flg;
};

#define MAX_BUTTON_CNT 		10

static int s_max_button_cnt = MAX_BUTTON_CNT;


static int s3c_Keycode[MAX_BUTTON_CNT] = {
	KEY_RESERVED,
	KEY_RESERVED,
	KEY_RESERVED,
	KEY_RESERVED,
	KEY_RESERVED, // 5

	KEY_RESERVED,
	KEY_RESERVED,
	KEY_RESERVED,
	KEY_RESERVED,
	KEY_RESERVED,		
};

static struct st_s3c_key s3c_key_para[MAX_BUTTON_CNT] = {
	{ KEY_RESERVED, 0, 0},
	{ KEY_RESERVED, 0, 0},
	{ KEY_RESERVED, 0, 0},
	{ KEY_RESERVED, 0, 0},
	{ KEY_RESERVED, 0, 0}, //5

	{ KEY_RESERVED, 0, 0},
	{ KEY_RESERVED, 0, 0},
	{ KEY_RESERVED, 0, 0},
	{ KEY_RESERVED, 0, 0},	
	{ KEY_RESERVED, 0, 0}, //10
};

static const int s3c_Keycode_common[4] = {
	KEY_POWER,
	KEY_VOLUMEUP,
	KEY_VOLUMEDOWN,
	KEY_BACK,
};

static const struct st_s3c_key s3c_key_para_common[4] = {
		{ KEY_POWER, EXYNOS4_GPX0(1), 0},
		{ KEY_VOLUMEUP, EXYNOS4_GPX2(0), 0},
		{ KEY_VOLUMEDOWN, EXYNOS4_GPX2(1), 0},	
		{ KEY_MENU, EXYNOS4_GPX2(4), 0},	//S702
};

static const int s3c_Keycode_d816[2] = {
	KEY_VOLUMEUP,
	KEY_VOLUMEDOWN,
//	KEY_BACK,
};

static const struct st_s3c_key s3c_key_para_d816[2] = {
	{ KEY_VOLUMEUP, EXYNOS4_GPX2(1), 0},
	{ KEY_VOLUMEDOWN, EXYNOS4_GPX2(0), 0},
//	{ KEY_BACK, EXYNOS4_GPX2(4), 0},
};

static const int s3c_Keycode_d107[6] = {
	KEY_END,
	KEY_VOLUMEUP,
	KEY_VOLUMEDOWN,
	KEY_KPEQUAL,
	KEY_HOME,
	KEY_WWW,
};

static const struct st_s3c_key s3c_key_para_d107[6] = {
	{ KEY_END, GPIO_ON_OUT, 0},
	{ KEY_VOLUMEUP, GPIO_KP_VOLUMEUP, 0},
	{ KEY_VOLUMEDOWN, GPIO_KP_VOLUMEDOWN, 0},
	{ KEY_KPEQUAL, GPIO_KEY_SHUTTER, 0},
	{ KEY_HOME, GPIO_KEY_HOME, 0},
	{ KEY_WWW, GPIO_KEY_MENU, 0},
};


static const int s3c_Keycode_d1011[8] = {
	KEY_POWER,
	KEY_BACK,
	KEY_VOLUMEUP,
	KEY_VOLUMEDOWN,
	KEY_KPEQUAL,
	
	KEY_HOME,
	KEY_MENU,	
	KEY_WWW,
//	KEY_MAIL,
};

static const struct st_s3c_key s3c_key_para_d1011[8] = {
	{ KEY_POWER, EXYNOS4_GPX0(1), 0},
	{ KEY_BACK, EXYNOS4_GPX0(5), 0},
	{ KEY_VOLUMEUP, GPIO_KP_VOLUMEUP, 0},
	{ KEY_VOLUMEDOWN, GPIO_KP_VOLUMEDOWN, 0},
	{ KEY_KPEQUAL, EXYNOS4_GPX0(6), 0},
	
	{ KEY_HOME, EXYNOS4_GPX0(7), 0},
	{ KEY_MENU, EXYNOS4_GPX1(0), 0}, /*F1*/
	{ KEY_WWW, EXYNOS4_GPX2(4), 0}, /*F2*/
//	{ KEY_MAIL, EXYNOS4_GPX2(6), 0},	/*F3*/
};

static void s3cbutton_timer_handler(unsigned long data)
{
	int flag;
	int i;
	for(i=0; i<s_max_button_cnt; i++)
	{
		flag = gpio_get_value(s3c_key_para[i].key_pin);
		if(flag != s3c_key_para[i].key_history_flg) 
		{
			if(flag) 
			{
			    input_report_key(input, s3c_key_para[i].key_code, 0);
			    printk(KERN_DEBUG"s3c-button  key up!\n");
 			}  
			else  
			{
			     input_report_key(input, s3c_key_para[i].key_code, 1);
			     printk(KERN_DEBUG"s3c-button  key down!!\n");
			}
			s3c_key_para[i].key_history_flg= flag;

			input_sync(input);
		}
	}

	/* Kernel Timer restart */
	mod_timer(&timer,jiffies + HZ/10);
}


#define CONFIG_WAKELOCK_KEEP

#ifdef CONFIG_WAKELOCK_KEEP
static void keep_wakeup_timeout(u8 sec);

#include <linux/wakelock.h>

static struct wake_lock s_ut_wake_lock2;
#endif
static int s3c_button_probe(struct platform_device *pdev)
{
	int i;

	for(i=0; i<s_max_button_cnt; i++)  {
		gpio_request(s3c_key_para[i].key_pin, "s3c-button");
		s3c_gpio_setpull(s3c_key_para[i].key_pin, S3C_GPIO_PULL_UP);
		gpio_direction_input(s3c_key_para[i].key_pin);
	}

	input = input_allocate_device();
	if(!input)
		return -ENOMEM;

	set_bit(EV_KEY, input->evbit);
	//set_bit(EV_REP, input->evbit);	/* Repeat Key */

	for(i = 0; i < s_max_button_cnt; i++)
		set_bit(s3c_key_para[i].key_code, input->keybit);

	input->name = "s3c-button";
	input->phys = "s3c-button/input0";

	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;

	input->keycode = s3c_Keycode;
	input->keycodemax  = s_max_button_cnt;
	input->keycodesize = sizeof(s3c_Keycode[0]);

	if(input_register_device(input) != 0)
	{
		printk(KERN_ERR"s3c-button input register device fail!!\n");

		input_free_device(input);
		return -ENODEV;
	}

	/* Scan timer init */
	init_timer(&timer);
	timer.function = s3cbutton_timer_handler;

	timer.expires = jiffies + (HZ/20);
	add_timer(&timer);



#ifdef CONFIG_WAKELOCK_KEEP
	wake_lock_init(&s_ut_wake_lock2,  WAKE_LOCK_SUSPEND, "hold wake buttom");
#endif

	printk(KERN_INFO"s3c button Initialized!!\n");

	return 0;
}

static int s3c_button_remove(struct platform_device *pdev)
{
	int i;

	input_unregister_device(input);
	del_timer(&timer);
	for(i=0; i<s_max_button_cnt; i++)  {
		gpio_free(s3c_key_para[i].key_pin);
	}

#ifdef CONFIG_WAKELOCK_KEEP
	wake_lock_destroy(&s_ut_wake_lock2);
#endif

	return  0;
}


#ifdef CONFIG_PM
static inline unsigned int ms_to_jiffies(unsigned int ms)
{
        unsigned int j;
        j = (ms * HZ + 500) / 1000;
        return (j > 0) ? j : 1;
}

static int s3c_button_suspend(struct platform_device *pdev, pm_message_t state)
{
	printk("%s(); +\n",__func__);
	mod_timer(&timer, jiffies + ms_to_jiffies(800)); 

	printk("%s(); -\n",__func__);

	return 0;
}
#ifdef CONFIG_WAKELOCK_KEEP
static void keep_wakeup_timeout(u8 sec)
{
	printk("%s(); +\n",__func__);

	wake_lock(&s_ut_wake_lock2);
	wake_lock_timeout(&s_ut_wake_lock2,  sec * HZ);//jerry

	printk("%s(); -\n",__func__);

}
#endif

//andydeng added 20130109 start

 void s3c_send_wakeup_key(void)
{
	printk("%s(); +\n",__func__);
	if(!input)
		return ;
	input_report_key(input,KEY_END,1);
	input_sync(input);
	mdelay(1);
	input_report_key(input,KEY_END,0);
	input_sync(input);
	printk("%s(); -\n",__func__);
}
EXPORT_SYMBOL(s3c_send_wakeup_key);

static  int button_ut7gm_get_adc_data(int channel)
{
	int adc_value = 0;
	int retry_cnt = 0;

	if (IS_ERR(button_ut7gm_client)) {
		return -1;  
	}
	
	do {
		adc_value = s3c_adc_read(button_ut7gm_client, channel);
		if (adc_value < 0) {
			pr_info("%s: adc read(%d), retry(%d)", __func__, adc_value, retry_cnt++);
			msleep(200);
		}
	} while (((adc_value < 0) && (retry_cnt <= 5)));

	if(retry_cnt > 5 ) 
		return -1;

	return adc_value;
}

static int button_ut7gm_readbat_mv(void)
{
	int i = 0,count = 0,sum = 0,adc_value = 0, mv_value = 0;
	int buffer[16] = {0};

	button_ut7gm_get_adc_data(0);	
	
	 for(i = 0; i < 16; i++)
   	 {
	        buffer[i] = button_ut7gm_get_adc_data(0);
			if(sum += buffer[i] != 0)
			{
		    		sum += buffer[i];
				count++;
			}
   	 }
	if (count == 0)
	{
		return -1;  
	}
	adc_value = sum/count;
	
	mv_value  =  (adc_value * 1800 / 4095) * 3;
	printk("--button--adc_value:%d , mv_value:%d\n",adc_value,mv_value);
	return mv_value;
}
static int s3c_check_battery_adc(void)
{

	int i = 0,count = 0,sum = 0,adc_value = 0, mv_value = 0;
	int cycle_count = 0;
	//int buffer[16] = {0};
	
	printk("---%s---\n",__func__);	
	
	//button_ut7gm_get_adc_data(0);	
	
	for(cycle_count = 0 ; cycle_count < 3; cycle_count++)
	{
		mv_value = button_ut7gm_readbat_mv();

		mdelay(200);
	}

	if(mv_value < 3530)
		s3c_send_wakeup_key();
	
	printk("+++%s+++\n",__func__);	
	return 0;
}
//andydeng added 20130109
static int s3c_button_resume(struct platform_device *pdev)
{
	u32 stat_val = 0;

#ifdef CONFIG_WAKELOCK_KEEP
	keep_wakeup_timeout(10);//jerry
#endif
	
	mod_timer(&timer, jiffies + ms_to_jiffies(1800)); 
	#if 1
	 stat_val = (__raw_readl(S5P_WAKEUP_STAT));
	printk("\nS5P_WAKEUP_STAT = %08x!!\n\n", stat_val);


	if( stat_val & 1) {
		if(s3c_pm_is_key_wakeup()) {
		//	input_report_key(input, KEY_END, 1);
			input_report_key(input, KEY_POWER, 1);
			input_sync(input);
			mdelay(1);
			printk(KERN_DEBUG"s3c_button_resume by eint!!\n");
			printk("\ns3c_button_resume by key!!\n\n");
		//	input_report_key(input, KEY_END, 0);
			input_report_key(input, KEY_POWER, 0);
			input_sync(input);
			//s3c_check_battery_adc();  //  add fot test when key INT
		}
		else {
				printk("\ns3c_button_resume by eint but is not key!!\n\n");
			#ifdef CONFIG_WAKELOCK_KEEP
				keep_wakeup_timeout(6);//add by jerry
			#endif	
				s3c_check_battery_adc();
			}
	} else {
#ifdef CONFIG_WAKELOCK_KEEP
		printk("\ns3c_button_resume by other!!\n\n");
		keep_wakeup_timeout(6);//add by jerry
		s3c_check_battery_adc();
#endif
	}
	#endif
	
	printk("s3c_button_resume!! [%08x]\n", stat_val);
	return 0;
}
#else
#define s3c_button_suspend	NULL
#define s3c_button_resume	NULL
#endif

static struct platform_driver s3c_button_device_driver = {
	.probe		= s3c_button_probe,
	.remove		= s3c_button_remove,
	.suspend	= s3c_button_suspend,
	.resume		= s3c_button_resume,
	.driver		= {
		.name	= "s3c-button",
		.owner	= THIS_MODULE,
	}
};


static struct platform_device s3c_device_button = {
	.name	= "s3c-button",
	.id		= -1,
};

extern char g_selected_utmodel[32];
static int __init s3c_button_init(void)
{
	if(strstr(g_selected_utmodel, "d107")) {
		memcpy(s3c_Keycode, s3c_Keycode_d107, sizeof(s3c_Keycode_d107));
		memcpy(s3c_key_para, s3c_key_para_d107, sizeof(s3c_key_para_d107));

		s_max_button_cnt = ARRAY_SIZE(s3c_Keycode_d107);
	}

	else if(strstr(g_selected_utmodel, "d816")) {
		memcpy(s3c_Keycode, s3c_Keycode_d816, sizeof(s3c_Keycode_d816));
		memcpy(s3c_key_para, s3c_key_para_d816, sizeof(s3c_key_para_d816));

		s_max_button_cnt = ARRAY_SIZE(s3c_Keycode_d816);
	}

	else if(strstr(g_selected_utmodel, "d1011")) {
		memcpy(s3c_Keycode, s3c_Keycode_d1011, sizeof(s3c_Keycode_d1011));
		memcpy(s3c_key_para, s3c_key_para_d1011, sizeof(s3c_key_para_d1011));

		s_max_button_cnt = ARRAY_SIZE(s3c_Keycode_d1011);
	}

	else {
		memcpy(s3c_Keycode, s3c_Keycode_common, sizeof(s3c_Keycode_common));
		memcpy(s3c_key_para, s3c_key_para_common, sizeof(s3c_key_para_common));

		s_max_button_cnt = ARRAY_SIZE(s3c_Keycode_common);
	}
	printk("%s(); + s_max_button_cnt = %d\n", __func__, s_max_button_cnt);
	platform_device_register(&s3c_device_button);
	
	button_ut7gm_client = s3c_adc_register(&s3c_device_button, NULL, NULL, 0);
	if (IS_ERR(button_ut7gm_client)) {
		printk("ERROR register button_ut7gm_client!");
	}
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
