/*------------------------------------------------------------------------------
--
--  Abstract : vc0882 headset detect driver (kernel module)
--
----------------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: vc0882_switch_headset.c,v $
--  $Date: 2011/01/21 $
--  $Revision: 0.1 $
--
Version 			   Description				 					Date				Author
  0.1				   Created			        					2011-01-21	   	Zhang Jing
----------------------------------------------------------------------------------------*/


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
//#include <linux/v210_switch_headset.h>
#define set_irq_type   irq_set_irq_type		//raymanfeng for kernel 3.0 

#define HP_DETECT_PIN EXYNOS4_GPX2(2)


#define EAR_DETECT_PIN	EXYNOS4_GPX1(3)

#define ENIT_NUM  IRQ_EINT(18)//IRQ_EINT16_31  
#define EAR_ENIT_NUM  IRQ_EINT11

extern char g_selected_utmodel[];
extern void set_keystate_boot_keyboard(void);

static struct work_struct earphone_work;
#define EARPHONE_KEY_DETECT_INTERVAL                 (msecs_to_jiffies(100))
int earphone_inset_flag=0;
EXPORT_SYMBOL(earphone_inset_flag);
static struct work_struct earphone_report_work;
static struct work_struct delay_open_micbias;
struct v210_headset_switch_data *SwitchData;
#define EARPHON_KEY_MAX 2
 #define EARPHONE_KEY_PRESS_AVAILIABLE

static int earphone_Keycode[EARPHON_KEY_MAX] = {KEY_MEDIA,KEY_BACK};

static struct input_dev* earphon_key_input=NULL;
#define EAR_PRESSKEY_ENIT_NUM  IRQ_EINT3
#define EARPHONE_KEY EXYNOS4_GPX1(3)
static struct s3c_adc_client	*headset_client = NULL;

#ifdef CONFIG_SND_SOC_SAMSUNG_SMDK_WM8978
extern void wm8978_set_bias(int flag);
#endif
 extern char g_selected_codec[32];

#define HP_DETEST_IRQ	"HP_DETECT_IRQ"
//#define DOUBLE_IRQ_CHECK

struct switch_dev *headset_sdev_for_dock = NULL;
struct switch_dev * get_headset_sdev(void)
{
    return headset_sdev_for_dock;
}
EXPORT_SYMBOL(get_headset_sdev);
enum {
	SEC_JACK_NO_DEVICE		= 0x0,
	SEC_HEADSET_4POLE		= 0x01 << 0,
	SEC_HEADSET_3POLE		= 0x01 << 1,
	SEC_TTY_DEVICE			= 0x01 << 2,
	SEC_FM_HEADSET			= 0x01 << 3,
	SEC_FM_SPEAKER			= 0x01 << 4,
	SEC_TVOUT_DEVICE		= 0x01 << 5,
	SEC_EXTRA_DOCK_SPEAKER		= 0x01 << 6,
	SEC_EXTRA_CAR_DOCK_SPEAKER	= 0x01 << 7,
	SEC_UNKNOWN_DEVICE		= 0x01 << 8,
};

struct v210_headset_switch_data {
	void	(*set_micbias_state) (bool);
	struct switch_dev sdev;
	unsigned gpio;
	unsigned ear_jack_gpio;
	const char *name_on;
	const char *name_off;
	const char *state_on;
	const char *state_off;
	int irq;
	int ear_jack_irq;
	struct work_struct work;
};

// extern char g_selected_utmodel[32];
extern int DOCK_KEYBOARD_FLAG;
void sec_jack_set_micbias_state(bool on)
{}
//Jodon add for hdmi plugout
void headset_irq_for_hdmi(void)
{
	printk("%s\n",__FUNCTION__);
	schedule_work(&SwitchData->work);
}
EXPORT_SYMBOL(headset_irq_for_hdmi);

//extern int s3c_adc_get_adc_data(int channel);
static void delay_micbias_open(struct work_struct* work)
{
//	s3c_adc_get_adc_data(0);
  //	msleep(1000);
  int handset_state = 0;
  printk("g_selected_codec:%s\n",g_selected_codec);
   if (!strcmp(g_selected_codec,"wm8978"))
  	 {  	 
    		msleep(500);
    		handset_state = gpio_get_value(HP_DETECT_PIN);
    		//printk("========delay_micbias_open=========wm8978_set_bias(%d)=========\n", handset_state);
      	if (handset_state == 1){ //plug in
    	    wm8978_set_bias(1);
    	  }else{
    	    wm8978_set_bias(0);
    	  }
    	  
  	 }
//   schedule_work(&delay_open_micbias);
}

static int earphone_pole_flag;//  2:4pole,1: 3pole, 0: pole
static void report_4pole_earphone(struct work_struct* work)
{
	printk("earphone_pole_flag:%d\n",earphone_pole_flag);
	switch (earphone_pole_flag){
	case 2:
		switch_set_state(&(SwitchData->sdev), SEC_HEADSET_4POLE);
		break;
	case 1:
		switch_set_state(&(SwitchData->sdev), SEC_HEADSET_3POLE);
		break;
	case 0:
		
//add by jerry		
		if(DOCK_KEYBOARD_FLAG)
			{
				gpio_direction_output(EXYNOS4212_GPM1(4),0);//spk off
	 			gpio_direction_output(EXYNOS4_GPL2(4),1);//keyboard spk on
			}else{
			  switch_set_state(&(SwitchData->sdev), SEC_JACK_NO_DEVICE);
		    msleep(300);
		    gpio_direction_output(EXYNOS4212_GPM1(4),1);//spk on
		    gpio_direction_output(EXYNOS4_GPL2(4),0);//keyboard spk off
			}
//end			
	 	break;
	default:
		 break;
		}
	return;
}

int cur_pin_val, old_pin_val;
int scan_pin_cnt;
int press_donw_flg=0;
 int headset_get_adc_data(int channel)
 {
	int adc_value = 0;
	int retry_cnt = 0;

	if (IS_ERR(headset_client)) {
		return -1;  
	}
	
	do {
		adc_value = s3c_adc_read(headset_client, channel);
		if (adc_value < 0) {
			pr_info("%s: adc read(%d), retry(%d)", __func__, adc_value, retry_cnt++);
			msleep(200);
		}
	} while (((adc_value < 0) && (retry_cnt <= 5)));

	if(retry_cnt > 5 ) 
		return -1;

	return adc_value;
 }
 extern int suspend_flag;
 int resume_detect_cnt;
static void earphone_key_scan(struct work_struct* work)
{
#if 1

	if(suspend_flag==1 && resume_detect_cnt<100){
		resume_detect_cnt++;
		goto scan_out;
	}else{
			resume_detect_cnt=0;
		}
	 cur_pin_val=gpio_get_value(EARPHONE_KEY);

	if(1==cur_pin_val)
	{
		if(cur_pin_val == old_pin_val) {
			if(scan_pin_cnt<1000)
				scan_pin_cnt ++;
		} else {
			scan_pin_cnt = 0;
		}
	//	printk("scan_pin_cnt:%d,press_donw_flg:%d\n",scan_pin_cnt,press_donw_flg);
		if(scan_pin_cnt==3 && press_donw_flg==1 ) {
			// up
			input_report_key(earphon_key_input,earphone_Keycode[0],0);
			input_sync(earphon_key_input);
			
			printk("Report key 0,press_donw_flg:%d\n",press_donw_flg);
			press_donw_flg=0;
		}


		if(scan_pin_cnt==30 && earphone_inset_flag==1 ) {
			// 4  pole
			earphone_pole_flag = 2;
			schedule_work(&earphone_report_work);
		}
	}
	else
	{
		if(cur_pin_val == old_pin_val) {
			if(scan_pin_cnt<1000)
				scan_pin_cnt ++;
		} else {
			scan_pin_cnt = 0;
		}

		if(scan_pin_cnt==2 && earphone_pole_flag==2 &&press_donw_flg==0) {
			//down
			press_donw_flg = 1;
			#ifdef EARPHONE_KEY_PRESS_AVAILIABLE
			input_report_key(earphon_key_input,earphone_Keycode[0],1);
			input_sync(earphon_key_input);
			printk("Report key 1\n");
			#endif

		}

		if((  scan_pin_cnt==30 && earphone_inset_flag==1 )
//			||(  scan_pin_cnt==40 && earphone_inset_flag==1 )
		) {
			// 3 pole
			if(press_donw_flg ==0) {
				earphone_pole_flag =1;
				schedule_work(&earphone_report_work);
			}
			else { //press_donw_flg=1
			#ifdef EARPHONE_KEY_PRESS_AVAILIABLE
				input_report_key(earphon_key_input,earphone_Keycode[0],0);
				input_sync(earphon_key_input);
				
				printk("Report key 0\n");
			#endif
				press_donw_flg=0;
			}
		
		}
	}

	old_pin_val = cur_pin_val;
scan_out:
msleep(EARPHONE_KEY_DETECT_INTERVAL);
schedule_work(&earphone_work);

#else

	static int input_key_flag=0;
	int adc_value;
	adc_value=headset_get_adc_data(1);
//    printk("adc1:%d\n",adc_value);

	if(earphone_inset_flag==1){
		if(adc_value>0 && adc_value<20){
				switch_set_state(&(SwitchData->sdev), SEC_HEADSET_3POLE);
			}else if(adc_value>4000){
				earphone_inset_flag=2;
				switch_set_state(&(SwitchData->sdev), SEC_HEADSET_4POLE);
				goto scan_out;
			}
		}
	if(earphone_inset_flag==2){
		if(adc_value>=100 && adc_value<=200){
			printk("Report Key 0\n");
			input_report_key(earphon_key_input,earphone_Keycode[0],0);
			
			input_sync(earphon_key_input);
			input_key_flag=1;
			}
		else if(input_key_flag==1 && adc_value>4000){
			printk("Report Key 1\n");
			input_report_key(earphon_key_input,earphone_Keycode[0],1);
			
			input_sync(earphon_key_input);
			input_key_flag=0;
			}
		}

scan_out:	
	msleep(EARPHONE_KEY_DETECT_INTERVAL);
	schedule_work(&earphone_work);
	//add_timer(&earphone_timer);
	 


#endif
return ;
			
}
static void headset_switch_work(struct work_struct *work)
{
	int handset_state;
	int ear_jack_state;
	int report_state = SEC_JACK_NO_DEVICE;
	
	static int report_old_state = SEC_JACK_NO_DEVICE;
	static int handset_old_state = 0xff;
	
	struct v210_headset_switch_data	*data =
		container_of(work, struct v210_headset_switch_data, work);

	handset_state = gpio_get_value(HP_DETECT_PIN);
	printk("%s,handset_state:%d,%d\n",__FUNCTION__,handset_state,headset_get_adc_data(1));

	
	if (handset_state==1){ //plug in
		printk(KERN_DEBUG "Headset plug in.\n");
		printk("Headset plug in.\n");
		gpio_direction_output(EXYNOS4212_GPM1(4),0);//spk off
		report_state = SEC_HEADSET_3POLE;
		if (!strcmp(g_selected_codec,"wm8976"))
			{
			//	msleep(500);
				wm8978_set_bias(1);
			}
	//	enable_irq(EAR_PRESSKEY_ENIT_NUM);
	} else {//plug out
		printk("Headset plug out.\n");
		report_state =0;
	//	disable_irq(EAR_PRESSKEY_ENIT_NUM);
	}
	
	
	if(handset_old_state^handset_state) {
		data->set_micbias_state(!!handset_state);
		handset_old_state = handset_state;
	}


	/*
	ear_jack_state = gpio_get_value(EAR_DETECT_PIN);
	#ifndef DOUBLE_IRQ_CHECK
	 msleep(msecs_to_jiffies(100));
	#endif
	ear_jack_state = gpio_get_value(EAR_DETECT_PIN);
	ear_jack_state = gpio_get_value(EAR_DETECT_PIN);

	
 	if(handset_state==1) 
		{
			if (ear_jack_state==1)
				{
					printk("Headset 4pole plug in.\n");
				}
			else
				{
					printk("Headset 3pole plug in.\n");
						
				}
		} 

	
	if (ear_jack_state==1) { // 4 pole plug in or plug out
 		if(handset_state==1) {
//			printk("Headset 4pole plug in.\n");
			report_state = SEC_HEADSET_4POLE;
		} else {
			report_state &= ~(SEC_HEADSET_4POLE);
		}
	}

	
	
	
	printk("#Headset plug  handset_state=%d, ear_jack_state=%d, report_state=%d, report_old_state=%d.\n", handset_state, ear_jack_state, report_state, report_old_state);
	*/

//	if(report_old_state ^ report_state) {
///		switch_set_state(&data->sdev, report_state);
//		report_old_state = report_state;
//	}
	
//	if (!strcmp(g_selected_codec,"wm8976"))
//		{
 //	 	  wm8978_set_bias(0);
//		}
		
	schedule_work(&delay_open_micbias);


	scan_pin_cnt = 0;
	 if(handset_state==1)		
		earphone_inset_flag=1;
	else{		
		earphone_inset_flag=0;
		earphone_pole_flag=0;
		schedule_work(&earphone_report_work);
		}

}

static irqreturn_t headset_irq_handler(int irq, void *dev_id)
{
	struct v210_headset_switch_data *switch_data =
	    (struct v210_headset_switch_data *)dev_id;
	printk("%s\n",__FUNCTION__);
	schedule_work(&switch_data->work);
	return IRQ_HANDLED;
}

static ssize_t headset_switch_print_state(struct switch_dev *sdev, char *buf)
{
	struct v210_headset_switch_data	*switch_data =
		container_of(sdev, struct v210_headset_switch_data, sdev);
	const char *state;
	if (switch_get_state(sdev))
		state = switch_data->state_on;
	else
		state = switch_data->state_off;

	if (state)
		return sprintf(buf, "%s\n", state);
	return -1;
}

static int headset_switch_probe(struct platform_device *pdev)
{
	struct gpio_switch_platform_data *pdata = pdev->dev.platform_data;
	struct v210_headset_switch_data *switch_data;
	int ret = 0;
	//unsigned int value = 0;

	if (!pdata)
		return -EBUSY;

	switch_data = kzalloc(sizeof(struct v210_headset_switch_data), GFP_KERNEL);
	if (!switch_data)
		return -ENOMEM;

	switch_data->sdev.name = pdata->name;
	switch_data->gpio = HP_DETECT_PIN;
	switch_data->ear_jack_gpio = EAR_DETECT_PIN;
	switch_data->name_on = pdata->name_on;
	switch_data->name_off = pdata->name_off;
	switch_data->state_on = pdata->state_on;
	switch_data->state_off = pdata->state_off;
	switch_data->sdev.print_state = headset_switch_print_state;
	switch_data->set_micbias_state=sec_jack_set_micbias_state;
	
    	ret = switch_dev_register(&switch_data->sdev);
    	headset_sdev_for_dock = (&switch_data->sdev);

	if (ret < 0)
		goto err_switch_dev_register;

	ret = gpio_request(switch_data->gpio, HP_DETEST_IRQ);
	if (ret)
	    printk(KERN_ERR "#### failed to request GPH0-6 ");

	ret = gpio_request(switch_data->ear_jack_gpio, "EAR_DET_GPIO");
	if (ret)
	    printk(KERN_ERR "#### failed to request GPH0-3 ");
#if 0	
	gpio_direction_input(switch_data->gpio);
	switch_data->irq = gpio_to_irq(switch_data->gpio);
#else
	#ifndef DOUBLE_IRQ_CHECK	
	 s3c_gpio_cfgpin(switch_data->ear_jack_gpio, S3C_GPIO_INPUT);
	 s3c_gpio_setpull(switch_data->ear_jack_gpio, S3C_GPIO_PULL_UP);
	#else
	s3c_gpio_cfgpin(switch_data->ear_jack_gpio, S3C_GPIO_SPECIAL(0x0f)); //Eint6
	s3c_gpio_setpull(switch_data->ear_jack_gpio, S3C_GPIO_PULL_UP);
	set_irq_type(EAR_ENIT_NUM, IRQF_TRIGGER_RISING |IRQF_TRIGGER_FALLING);	
	#endif
//	s3c_gpio_cfgpin(switch_data->gpio, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(switch_data->gpio, S3C_GPIO_SPECIAL(0xf)); //Eint6
	s3c_gpio_setpull(switch_data->gpio, S3C_GPIO_PULL_NONE);
	set_irq_type(ENIT_NUM, IRQF_TRIGGER_RISING |IRQF_TRIGGER_FALLING);

	switch_data->irq = ENIT_NUM;
	switch_data->ear_jack_irq = EAR_ENIT_NUM;
#endif
	gpio_set_debounce(switch_data->gpio, 100);
	gpio_set_debounce(switch_data->ear_jack_gpio, 143);

	SwitchData=switch_data;

	INIT_WORK(&switch_data->work, headset_switch_work);

//	init_timer(&earphone_timer);
	//earphone_timer.function = earphone_key_scan;
//	 earphone_timer.expires = jiffies + EARPHONE_KEY_DETECT_INTERVAL;
//	add_timer(&earphone_timer);

	INIT_WORK(&earphone_work, earphone_key_scan);

	INIT_WORK(&earphone_report_work, report_4pole_earphone);
	
		
	INIT_WORK(&delay_open_micbias, delay_micbias_open);
	
	schedule_work(&delay_open_micbias);
	
	schedule_work(&earphone_work);
	
#if 0
//added by zj on 2011/03/09
	value = __raw_readl( IO_ADDRESS(V8REG_GPIO_INT_SYS0_TYPE) );
	value |= 0x00000010;
	__raw_writel( value, IO_ADDRESS(V8REG_GPIO_INT_SYS0_TYPE) );
	__raw_writel( 0x112A880, IO_ADDRESS(V8REG_GPIO_INT_SYS_WIDTH1) );		//250ms
//end
#endif

	ret = request_irq(switch_data->irq ,headset_irq_handler, IRQF_SHARED /*| IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING*/, switch_data->sdev.name, switch_data);
	if(ret < 0)
	{
		printk(KERN_ERR "%s(), IRQ%d not available", __func__, switch_data->irq);
		goto err_request_gpio;
	}

	#ifdef DOUBLE_IRQ_CHECK
	ret = request_irq(switch_data->ear_jack_irq ,headset_irq_handler, IRQF_SHARED, switch_data->sdev.name, switch_data);
	if(ret < 0)
	{
		printk(KERN_ERR "%s(), IRQ%d not available", __func__, switch_data->ear_jack_irq);
		goto err_request_gpio;
	}
	//enable_irq_wake(switch_data->irq);
	#endif
	
	/* Perform initial detection */
	headset_switch_work(&switch_data->work);

	return 0;

err_request_gpio:
    switch_dev_unregister(&switch_data->sdev);
err_switch_dev_register:
	kfree(switch_data);

	return ret;
}

static int __devexit headset_switch_remove(struct platform_device *pdev)
{
	struct v210_headset_switch_data *switch_data = platform_get_drvdata(pdev);

	cancel_work_sync(&switch_data->work);
	gpio_free(switch_data->gpio);
    	switch_dev_unregister(&switch_data->sdev);
	kfree(switch_data);

	return 0;
}

static struct platform_driver headset_switch_driver = {
	.probe		= headset_switch_probe,
	.remove		= __devexit_p(headset_switch_remove),
	.driver		= {
		.name	= "switch-headset",
		.owner	= THIS_MODULE,
	},
};

static struct gpio_switch_platform_data v210_headset_switch_data = {
       .name = "h2w", 
//	.gpio = HP_DETECT_PIN,
};
 
static struct platform_device headset_switch_device = {
       .name             = "switch-headset",
       .dev = {
               .platform_data    = &v210_headset_switch_data,
       }
};


static irqreturn_t earphone_press_key_handle(int irq, char* utmodule)
{
	static int up_or_down=1;
	printk("%s,up_or_down:%d\n",__FUNCTION__,up_or_down);
	if(earphon_key_input)  {
	input_report_key(earphon_key_input,earphone_Keycode[0],up_or_down);
	input_sync(earphon_key_input);
	   }
	if(up_or_down==1)
		up_or_down=0;
	else 
		up_or_down=1;
	return IRQ_HANDLED;
}


static int __init headset_switch_init(void)
{
//	extern char g_selected_utmodel[];

	int ret,i; 
#if 0

	ret = gpio_request(EARPHONE_KEY, "earphone_key");
	if (ret)
	    printk(KERN_ERR "#### failed to request GPH0-3 ");

	
 	s3c_gpio_cfgpin(EARPHONE_KEY, S3C_GPIO_SFN(0xf));
 	s3c_gpio_setpull(EARPHONE_KEY, S3C_GPIO_PULL_UP);
 	gpio_set_debounce(EARPHONE_KEY, 143);
	set_irq_type(EAR_PRESSKEY_ENIT_NUM, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING);
	ret= request_irq(EAR_PRESSKEY_ENIT_NUM,earphone_press_key_handle,IRQF_SHARED,NULL,g_selected_utmodel);
	if(ret<0)
		{
			printk("request_irq ERROR");		
		}

	
#endif

	earphon_key_input = input_allocate_device();
	 if(!earphon_key_input){
	 	printk("earphon_key_input alloc Error");
	 	}
	set_bit(EV_KEY, earphon_key_input->evbit);
	for(i = 0; i < EARPHON_KEY_MAX; i++)
		set_bit(earphone_Keycode[i], earphon_key_input->keybit);
//	earphon_key_input->name = "wm8978";
//	earphon_key_input->phys = "wm8978/input0";

	earphon_key_input->keycode = earphone_Keycode;
	
	earphon_key_input->evbit[0] = BIT(EV_KEY);
//	earphon_key_input->keybit[BTN_0] = BIT(BTN_0);
	if(input_register_device(earphon_key_input) != 0)
		{
			printk("s3c-button input register device fail!!\n");
			input_free_device(earphon_key_input);
		}
 //	disable_irq(EAR_PRESSKEY_ENIT_NUM);
//	if(strncmp(g_selected_utmodel, "7305", strlen("7305")))
//		return 0;

#if 0	
	if(!strncmp(g_selected_utmodel, "7305", strlen("7305")) || !strncmp(g_selected_utmodel, "812pe", strlen("812pe")))
		;
	else
		return 0;
#endif

	platform_device_register(&headset_switch_device);
	headset_client = s3c_adc_register(&headset_switch_device, NULL, NULL, 0);
	if (IS_ERR(headset_client)) {
		printk("ERROR register headset_client!");
	}

	return platform_driver_register(&headset_switch_driver);
}

static void __exit headset_switch_exit(void)
{
	platform_device_unregister(&headset_switch_device);
	platform_driver_unregister(&headset_switch_driver);
	
	input_unregister_device(earphon_key_input);
}

late_initcall(headset_switch_init);
module_exit(headset_switch_exit);

MODULE_AUTHOR("Zhang Jing <zhangjing@vimicro.com>");
MODULE_DESCRIPTION("VC0882 Switch Headset driver");
MODULE_LICENSE("GPL");

