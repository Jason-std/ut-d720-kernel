/*
 * VTL CTP driver
 *
 * Copyright (C) 2013 VTL Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <plat/gpio-cfg.h>

#include <mach/gpio.h>


#include "vtl_ts.h"
#include "chip.h"
#include "apk.h"

#define		TS_THREAD_PRIO		90

int		XY_SWAP_ENABLE	=	1;

int		X_REVERSE_ENABLE   =	0;

int		Y_REVERSE_ENABLE   = 1;

static DECLARE_WAIT_QUEUE_HEAD(waiter);
//static struct task_struct *ts_thread = NULL;
static unsigned char thread_syn_flag =0;
static volatile unsigned char thread_running_flag =0;
#define   on                              1

static int SCREEN_MAX_X	   = 2048;
static int SCREEN_MAX_Y	   = 1536;
static int model_tp			   = 9001;

static volatile int screen_on=1;

static struct i2c_client *this_client;

extern int g_touchscreen_init;

extern int TP_I2C_ADDR ;
extern int update_status_flag;
// ****************************************************************************
// Globel or static variables
// ****************************************************************************
static struct ts_driver	g_driver;

struct ts_info	g_ts = {
	.driver = &g_driver,
	.debug  = DEBUG_ENABLE,
};
struct ts_info	*pg_ts = &g_ts;

static struct i2c_device_id vtl_ts_id[] = {
	{ DRIVER_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c,vtl_ts_id);
extern char g_selected_utmodel[];

/*
static struct i2c_board_info i2c_info[] = {
	{
		I2C_BOARD_INFO(DRIVER_NAME, 0x01),
		.platform_data = NULL,
	},
};
*/


// ****************************************************************************
// Function declaration
// ****************************************************************************

static void check_model(void)
{
#if 1
	if(strncmp(g_selected_utmodel, "s9702", strlen("s9702")) == 0) {
		SCREEN_MAX_X = 2048;
		SCREEN_MAX_Y = 1536;
		model_tp = 9001;	
	}
	else if(strncmp(g_selected_utmodel, "s7801", strlen("s7801")) == 0 )
	{
		SCREEN_MAX_X = 768;
		SCREEN_MAX_Y = 1024;
		model_tp =7001;
	}
	else if(strncmp(g_selected_utmodel, "s8001", strlen("s8001")) == 0)
	{
		SCREEN_MAX_X = 800;
		SCREEN_MAX_Y =  1280;
		model_tp = 8001;
	}
	else if(strncmp(g_selected_utmodel, "s100", strlen("s100")) == 0) {

		SCREEN_MAX_X = 1920;
		SCREEN_MAX_Y =  1200;
		model_tp = 1001;
	}

	else if(strncmp(g_selected_utmodel, "p8", strlen("p8")) == 0) {

		SCREEN_MAX_X = 800;
		SCREEN_MAX_Y =  1280;
		model_tp = 8;
	}

	else if(strncmp(g_selected_utmodel, "d816", strlen("d816")) == 0) {
		SCREEN_MAX_X = 1024;
		SCREEN_MAX_Y =  768;
		model_tp = 816;
	}
	else if(strncmp(g_selected_utmodel, "d720", strlen("d720")) == 0) {

		SCREEN_MAX_Y = 800;
		SCREEN_MAX_X =  1280;
		model_tp = 720;
		XY_SWAP_ENABLE=0;
		X_REVERSE_ENABLE=1;
		Y_REVERSE_ENABLE=1;
		
	}else if(strncmp(g_selected_utmodel, "d721", strlen("d721")) == 0) {

		SCREEN_MAX_Y = 800;
		SCREEN_MAX_X =  1280;
		model_tp = 721;
		XY_SWAP_ENABLE=1;
		X_REVERSE_ENABLE=0;
		Y_REVERSE_ENABLE=0;
		
	}else if(strncmp(g_selected_utmodel, "s106", strlen("s106")) == 0) {

		SCREEN_MAX_Y = 800;
		SCREEN_MAX_X =  1280;
		model_tp = 106;
		XY_SWAP_ENABLE=1;
		X_REVERSE_ENABLE=0;
		Y_REVERSE_ENABLE=0;
		
	}
	
	printk("%s(); - ft x=%d, y=%d\n", __func__, SCREEN_MAX_X, SCREEN_MAX_Y);

#endif
}

static int vtl_ts_config(struct ts_info *ts)
{
	struct device *dev;
	struct ts_config_info *ts_config_info;	
	int err;
	
	DEBUG();

	dev = &ts->driver->client->dev;
	/* ts config */
	ts->config_info.touch_point_number = TOUCH_POINT_NUM;
	if(dev->platform_data !=NULL)
	{
		ts_config_info = dev->platform_data;
		//ts->config_info.screen_max_x	   = ts_config_info->screen_max_x;
        	//ts->config_info.screen_max_y	   = ts_config_info->screen_max_y;
		ts->config_info.screen_max_x	   = SCREEN_MAX_X;
        	ts->config_info.screen_max_y	   = SCREEN_MAX_Y;	
		ts->config_info.irq_gpio_number    = ts_config_info->irq_gpio_number;
        	ts->config_info.rst_gpio_number    = ts_config_info->rst_gpio_number;
			
	}
	else
	{
		return -1;	
	}
	
	ts->config_info.irq_number = gpio_to_irq(ts->config_info.irq_gpio_number);/* IRQ config*/
	
	err = gpio_request(ts->config_info.rst_gpio_number, "vtl_ts_rst");
	if ( err ) {
		//return -EIO;
	}

	if (816==model_tp) {
		ts->config_info.b_CtpRestPinOpposite =1; // Opposite for 3.3v
		ts->config_info.b_CtpHaveTouchKey =0; // Opposite for 3.3v
	}else if(model_tp==720){
		ts->config_info.b_CtpRestPinOpposite =1;
	}else if(model_tp==721){
		ts->config_info.b_CtpRestPinOpposite =1;
		ts->config_info.b_CtpHaveTouchKey =1;
	}else if(model_tp==106){
		ts->config_info.b_CtpRestPinOpposite =1;
		ts->config_info.b_CtpHaveTouchKey =0;
	}
	
	gpio_direction_output(ts->config_info.rst_gpio_number, (ts->config_info.b_CtpRestPinOpposite ? 0 : 1));
	//gpio_set_value(ts->config_info.rst_gpio_number, 1);

	return 0;
}

#ifdef HAVE_TOUCH_KEY
static void vtl_ts_vkeys_init(struct ts_info *ts)
{
	int iter;
	
	DEBUG();
	
	ts->vkeys_num = KEY_NUM;
	ts->vkeys = kzalloc(ts->vkeys_num * sizeof(struct struct_vkeys_data), GFP_KERNEL);
	ts->vkeys[0].key_code = KEY_HOME;
	ts->vkeys[0].x0 = ts->vkeys[0].x1 = 1;
	ts->vkeys[0].y0 = ts->vkeys[0].y1 = 4000;
#if 0

	ts->vkeys[1].key_code = KEY_MENU;
	ts->vkeys[1].x0 = ts->vkeys[1].x1 = 4000;
	ts->vkeys[1].y0 = ts->vkeys[1].y1 = 1199;


	ts->vkeys[2].key_code = KEY_HOMEPAGE;
	ts->vkeys[2].x0 = ts->vkeys[2].x1 = 1075;
	ts->vkeys[2].y0 = ts->vkeys[2].y1 = 448;

	ts->vkeys[3].key_code = KEY_SEARCH;
	ts->vkeys[3].x0 = ts->vkeys[3].x1 = 1075;
	ts->vkeys[3].y0 = ts->vkeys[3].y1 = 598;
#endif
	for ( iter = 0; iter < ts->vkeys_num; iter++ ) {
		input_set_capability(ts->driver->input_dev, EV_KEY, ts->vkeys[iter].key_code);
	}

}
#endif

struct ts_info	* vtl_ts_get_object(void)
{
	DEBUG();
	
	return pg_ts;
}
static void vtl_ts_free_gpio(void)
{
	struct ts_info *ts;
	ts =pg_ts;
	DEBUG();
	
	gpio_free(ts->config_info.rst_gpio_number);	
}

void vtl_ts_hw_reset(void)
{
	struct ts_info *ts;	
	ts =pg_ts;
	DEBUG();

	//gpio_set_value(ts->config_info.rst_gpio_number, 1);
	//msleep(10);
	gpio_set_value(ts->config_info.rst_gpio_number, (ts->config_info.b_CtpRestPinOpposite ? 1 : 0));
	msleep(50);
	gpio_set_value(ts->config_info.rst_gpio_number, (ts->config_info.b_CtpRestPinOpposite ? 0 : 1));
	msleep(250);
}

static void vtl_ts_wakeup(void)
{
	struct ts_info *ts;	
	ts =pg_ts;
	DEBUG();

	gpio_set_value(ts->config_info.rst_gpio_number, (ts->config_info.b_CtpRestPinOpposite ? 1 : 0));
	//msleep(50);
	msleep(20);
	gpio_set_value(ts->config_info.rst_gpio_number, (ts->config_info.b_CtpRestPinOpposite ? 0 : 1));
}

static irqreturn_t vtl_ts_irq(int irq, void *dev)
{
	struct ts_info *ts;	
	ts =pg_ts;
		
	DEBUG();
	//printk(">>>eking %s(%d)\n",__func__,__LINE__);
	disable_irq_nosync(ts->config_info.irq_number);// Disable ts interrupt
	thread_syn_flag=1; 
	wake_up_interruptible(&waiter);
	
	return IRQ_HANDLED;
}


static int vtl_ts_read_xy_data(struct ts_info *ts)
{
	struct i2c_client *client;
	struct i2c_msg msgs;
	int ret;
		
	DEBUG();
	client = ts->driver->client;

	msgs.addr = ts->driver->client->addr;
	msgs.flags = 0x01;  // 0x00: write 0x01:read 
	msgs.len = sizeof(ts->xy_data.buf);
	msgs.buf = ts->xy_data.buf;
	//msgs.scl_rate = TS_I2C_SPEED; ///only for rockchip platform
	ret = i2c_transfer( ts->driver->client->adapter, &msgs, 1);
	if(ret != 1){
		printk("___%s:i2c read xy_data err___\n",__func__);
		return -1;
	}
	return 0;
#if 0
	ret = vtl_ts_i2c_read(client,client->addr,ts->xy_data.buf,sizeof(ts->xy_data.buf));
	if(ret){
		printk("___%s:i2c read err___\n",__func__);
		return -1;
	}
	return 0;
#endif
}

static void vtl_ts_report_xy_coord(struct ts_info *ts)
{
	int i;
	int id;
	int vkey_iter;
	int sync;
	int x, y;
	unsigned int press;
	unsigned char touch_point_number;
	static unsigned int release = 0;
	struct input_dev *input_dev;
	union ts_xy_data *xy_data;
	
	DEBUG();

	xy_data = &ts->xy_data;
	input_dev = ts->driver->input_dev;
	touch_point_number = ts->config_info.touch_point_number;
	/* report points */
	//x = (xy_data->point[0].xhi<<4)|(xy_data->point[0].xlo&0xF);
	//y = (xy_data->point[0].yhi<<4)|(xy_data->point[0].ylo&0xF);
	//printk(">>>>id = %d,status = %d,X = %d,Y = %d  touch_point_number =%d\n",xy_data->point[0].id,xy_data->point[0].status,x,y,touch_point_number);
	sync = 0;  press = 0;ts->vkey_press = 0;
	for ( id = 0; id <touch_point_number; id++ ) //down
	{
		if ((xy_data->point[id].xhi != 0xFF) && (xy_data->point[id].yhi != 0xFF) &&
		     ( (xy_data->point[id].status == 1) || (xy_data->point[id].status == 2))) 
		{
		
			if(XY_SWAP_ENABLE){
				x = (xy_data->point[id].yhi<<4)|(xy_data->point[id].ylo&0xF);
				y = (xy_data->point[id].xhi<<4)|(xy_data->point[id].xlo&0xF);
			}else{
				x = (xy_data->point[id].xhi<<4)|(xy_data->point[id].xlo&0xF);
			 	y = (xy_data->point[id].yhi<<4)|(xy_data->point[id].ylo&0xF);
			}
			
			if(X_REVERSE_ENABLE)
				x = ts->config_info.screen_max_x - x;
			
			if(Y_REVERSE_ENABLE)
				y = ts->config_info.screen_max_y - y;
			
		
			//#if(DEBUG_ENABLE)
			//if((ts->debug)||(DEBUG_ENABLE)){
			if(ts->debug){
				printk("id = %d,status = %d,X = %d,Y = %d\n",xy_data->point[id].id,xy_data->point[id].status,x,y);
				//XY_DEBUG(xy_data->point[id].id,xy_data->point[id].status,x,y);
			}
			//#endif	
			
#ifdef HAVE_TOUCH_KEY
			if (ts->config_info.b_CtpHaveTouchKey==1) {
				if(y == 4000)
		    		{
		 
					for ( vkey_iter = 0; vkey_iter < ts->vkeys_num; vkey_iter++ ) {
						if (x >= ts->vkeys[vkey_iter].x0 && x <= ts->vkeys[vkey_iter].x1
						    && y >= ts->vkeys[vkey_iter].y0 && y <= ts->vkeys[vkey_iter].y1)
						{
						       // printk(">>>eking %s(%d) x = %d y=%d \n",__func__,__LINE__,x,y);
							input_report_key(ts->driver->input_dev,ts->vkeys[vkey_iter].key_code, 1);
							//led_on(1);
							sync = 1;
							ts->vkey_press |= (0x01 << vkey_iter);
						}
					}
					continue;
				  }
			}		
#endif

			if(!screen_on)
				continue;

			input_mt_slot(input_dev, xy_data->point[id].id - 1);
			input_report_abs(input_dev, ABS_MT_TRACKING_ID, xy_data->point[id].id-1);
			//input_mt_report_slot_state(input_dev, MT_TOOL_FINGER, true);
			input_report_abs(input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(input_dev, ABS_MT_POSITION_Y, y);
			input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, 1);
			input_report_abs(input_dev, ABS_MT_WIDTH_MAJOR, 1);
	
			press |= 0x01 << (xy_data->point[id].id - 1);
			sync = 1;
		
		}
		
	}
	release &= (release ^ press);//release point flag

	for ( id = 0; id < touch_point_number; id++ ) //up
	{
		if ( release & (0x01<<id) ) 
		{
			input_mt_slot(input_dev, id);
			input_report_abs(input_dev, ABS_MT_TRACKING_ID, -1);
			sync = 1;
		}
	}

	release = press;
	
#ifdef HAVE_TOUCH_KEY
	if (ts->config_info.b_CtpHaveTouchKey==1) {
		/* report virtual keys of up */
		ts->vkey_release &= ts->vkey_release ^ ts->vkey_press;
		for ( vkey_iter = 0; vkey_iter < ts->vkeys_num; vkey_iter++ ) {
			if ( ts->vkey_release & (0x01<<vkey_iter) ) {
				input_report_key(ts->driver->input_dev, ts->vkeys[vkey_iter].key_code, 0);
	                   //     printk(">>>eking %s(%d)  \n",__func__,__LINE__);
				sync = 1;
				//led_on(0);

			}
		}
		ts->vkey_release = ts->vkey_press;
	}
#endif

	if(sync)
	{
		input_sync(input_dev);
	}
}



int vtl_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct ts_info *ts;	
	unsigned char i;
	ts =pg_ts;

	DEBUG();
	if(ts->config_info.ctp_used)
	{
	//	disable_irq(ts->config_info.irq_number);
		if(model_tp==721){
			s3c_gpio_cfgpin(EXYNOS4_GPX0(5),S3C_GPIO_SFN(0xf));
			enable_irq_wake(gpio_to_irq(EXYNOS4_GPX0(5)));
		}else{
			chip_enter_sleep_mode();
			disable_irq(ts->config_info.irq_number);
		}
	
		for(i=0;i<ts->config_info.touch_point_number;i++)
		{
			input_mt_slot(ts->driver->input_dev,i);
			input_report_abs(ts->driver->input_dev, ABS_MT_TRACKING_ID, -1);
			//input_mt_report_slot_state(ts->driver->input_dev, MT_TOOL_FINGER, false);
		}
		input_sync(ts->driver->input_dev);
	}
	update_status_flag =0;
	screen_on=0;
	return 0;
}

int vtl_ts_resume(struct i2c_client *client)
{
	struct ts_info *ts;
	unsigned char i;	
	int ret;
	ts =pg_ts;

	DEBUG();


//	ret = gpio_request(GPIO_EXAXP22(2), "TP_PW_EN");
//	if (ret < 0) {
//		pr_err("failed to request TP_PW_EN \n");
//	}
//	gpio_direction_output(GPIO_EXAXP22(2),  !!on);
//	gpio_set_value(GPIO_EXAXP22(2), !!on);
//	gpio_free(GPIO_EXAXP22(2));
//	mdelay(5);

	
	if(ts->config_info.ctp_used)
	{
		/* Hardware reset */
		//vtl_ts_hw_reset();
		vtl_ts_wakeup();
		for(i=0;i<ts->config_info.touch_point_number;i++)
		{
			input_mt_slot(ts->driver->input_dev,i);
			input_report_abs(ts->driver->input_dev, ABS_MT_TRACKING_ID, -1);
			//input_mt_report_slot_state(ts->driver->input_dev, MT_TOOL_FINGER, false);
		}
		input_sync(ts->driver->input_dev);
		
		if(model_tp!=721)
			enable_irq(ts->config_info.irq_number);
	}
	screen_on=1;
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void vtl_ts_early_suspend(struct early_suspend *handler)
{
	struct ts_info *ts;	
	ts =pg_ts;

	DEBUG();

	vtl_ts_suspend(ts->driver->client, PMSG_SUSPEND);
}

static void vtl_ts_early_resume(struct early_suspend *handler)
{
	struct ts_info *ts;	
	ts =pg_ts;

	DEBUG();

	vtl_ts_resume(ts->driver->client);
}
#endif 
int __devexit vtl_ts_remove(struct i2c_client *client)
{
	struct ts_info *ts;	
	ts =pg_ts;

	DEBUG();

	free_irq(ts->config_info.irq_number, ts);   
	gpio_free(ts->config_info.rst_gpio_number);
	//vtl_ts_free_gpio();

	#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->driver->early_suspend);
	#endif
	if(ts->driver->input_dev != NULL)
	{
		input_unregister_device(ts->driver->input_dev);
		input_free_device(ts->driver->input_dev);
	}

	if ( ts->driver->proc_entry != NULL ){
		remove_proc_entry(DRIVER_NAME, NULL);
	}
	
	if(ts->driver->ts_thread != NULL)
	{
		printk("___kthread stop start___\n");
		thread_syn_flag=1; 
		wake_up_interruptible(&waiter);
		kthread_stop(ts->driver->ts_thread);
		ts->driver->ts_thread = NULL;
		printk("___kthread stop end___\n");
	}
	return 0;
}

static int vtl_ts_init_input_dev(struct ts_info *ts)
{
	struct input_dev *input_dev;
	struct device *dev;	
	int err;

	DEBUG();
	dev = &ts->driver->client->dev;
	
	/* allocate input device */
	ts->driver->input_dev = input_allocate_device();
	if ( ts->driver->input_dev == NULL ) {
		dev_err(dev, "Unable to allocate input device for device %s.\n", DRIVER_NAME);
		return -1;
	}	
	input_dev = ts->driver->input_dev;
	input_dev->name = "ft5x0x_ts";
    	input_dev->id.bustype = BUS_I2C;
    	input_dev->id.vendor  = 0xaaaa;
    	input_dev->id.product = 0x5555;
    	input_dev->id.version = 0x0001; 
	
	/* config input device */
	__set_bit(EV_SYN, input_dev->evbit);
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(EV_ABS, input_dev->evbit);


	set_bit(KEY_HOME, input_dev->keybit);
	set_bit(KEY_HOMEPAGE, input_dev->keybit);

	
	//set_bit(BTN_TOUCH, input_dev->keybit);//20130923
	//set_bit(ABS_MT_POSITION_X, input_dev->absbit);//20130923
    	//set_bit(ABS_MT_POSITION_Y, input_dev->absbit);//20130923

	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
	
	input_mt_init_slots(input_dev, TOUCH_POINT_NUM);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, ts->config_info.screen_max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, ts->config_info.screen_max_y, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0,ts->config_info.touch_point_number, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	
      #ifdef HAVE_TOUCH_KEY
	if (ts->config_info.b_CtpHaveTouchKey==1) {
		vtl_ts_vkeys_init(ts);
	}
      #endif
	/* register input device */
	err = input_register_device(input_dev);
	if ( err ) {
		input_free_device(ts->driver->input_dev);
		ts->driver->input_dev = NULL;
		dev_err(dev, "Unable to register input device for device %s.\n", DRIVER_NAME);
		return -1;
	}
	
	return 0;
}


static int vtl_ts_handler(void *data)
{
	int ret;
	struct device *dev;
	struct ts_info *ts;
	//struct sched_param param = { .sched_priority = TS_THREAD_PRIO};
	
	DEBUG();
	//sched_setscheduler(current, SCHED_RR, &param);

	ts = (struct ts_info *)data;
	dev = &ts->driver->client->dev;
	

	/* Request platform resources (gpio/interrupt pins) */
	ret = vtl_ts_config(ts);
	if(ret){

		dev_err(dev, "VTL touch screen config Failed.\n");
		goto ERR_TS_CONFIG;
	}
	
	vtl_ts_hw_reset();

	ret = chip_init(model_tp);
	if(ret){

		dev_err(dev, "vtl ts chip init failed.\n");
		goto ERR_CHIP_INIT;
	}

	/*init input dev*/
	ret = vtl_ts_init_input_dev(ts);
	if(ret){

		dev_err(dev, "init input dev failed.\n");
		goto ERR_INIT_INPUT;
	}

	/* Create Proc Entry File */
	ts->driver->proc_entry = create_proc_entry(DRIVER_NAME, 0666/*S_IFREG | S_IRUGO | S_IWUSR*/, NULL);
	if ( ts->driver->proc_entry == NULL ) {
		dev_err(dev, "Failed creating proc dir entry file.\n");
		goto ERR_PROC_ENTRY;
	} else {
		ts->driver->proc_entry->proc_fops = &apk_fops;
	}

	/* register early suspend */
#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->driver->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->driver->early_suspend.suspend = vtl_ts_early_suspend;
	ts->driver->early_suspend.resume = vtl_ts_early_resume;
	register_early_suspend(&ts->driver->early_suspend);
#endif

	/* Init irq */
	ret = request_irq(ts->config_info.irq_number, vtl_ts_irq, IRQF_TRIGGER_FALLING, DRIVER_NAME, ts);
	if ( ret ) {
		dev_err(dev, "Unable to request irq for device %s.\n", DRIVER_NAME);
		goto ERR_IRQ_REQ;
	}
	ts->config_info.ctp_used =1;
	while (!kthread_should_stop())//while(1)
	{
		//set_current_state(TASK_INTERRUPTIBLE);	
		wait_event_interruptible(waiter, thread_syn_flag);
		thread_syn_flag = 0;
		//set_current_state(TASK_RUNNING);
		//printk("__state = %d_%d_\n",current->state,ts->driver->ts_thread->state);
		ret = vtl_ts_read_xy_data(ts);

		if(!ret){
			vtl_ts_report_xy_coord(ts);
		}
		else
		{
			printk("____read xy_data error___\n");
		}

		// Enable ts interrupt 
		enable_irq(pg_ts->config_info.irq_number);
	}
	
	printk("vtl_ts_Kthread exit,%s(%d)\n",__func__,__LINE__);
	return 0;




ERR_IRQ_REQ:
	#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->driver->early_suspend);
	#endif
	if ( ts->driver->proc_entry ){
		remove_proc_entry(DRIVER_NAME, NULL); 
		ts->driver->proc_entry = NULL;
	}

ERR_PROC_ENTRY:
	if(ts->driver->input_dev){
		input_unregister_device(ts->driver->input_dev);
		input_free_device(ts->driver->input_dev);
		ts->driver->input_dev = NULL;
	}
ERR_INIT_INPUT:
ERR_CHIP_INIT:	
	gpio_free(ts->config_info.rst_gpio_number);
ERR_TS_CONFIG:
	ts->config_info.ctp_used =0;
	printk("vtl_ts_Kthread exit,%s(%d)\n",__func__,__LINE__);
	//do_exit(0);
	return 0;
}


static int ft5x0x_i2c_txdata(char *txdata, int length)
{
	int ret;

	struct i2c_msg msg[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= length,
			.buf	= txdata,
		},
	};

   	//msleep(1);
	ret = i2c_transfer(this_client->adapter, msg, 1);
	if (ret < 0)
		pr_err("%s i2c write error: %d\n", __func__, ret);

	return ret;
}

int vtl_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = -1;
	unsigned char chip_id = 0xff;
	struct ts_info *ts;
	struct device *dev;
        int ret;
	int retry=0;	

	DEBUG();

	ts = pg_ts;
	ts->driver->client = client;
	dev = &ts->driver->client->dev;
        this_client = client;
     
	/* Check I2C Functionality */
	err = i2c_check_functionality(client->adapter, I2C_FUNC_I2C);
	if ( !err ) {
		dev_err(dev, "Check I2C Functionality Failed.\n");
		return ENODEV;
	}
//	if (BOARD_AXP_VER_1==get_board_version()) {
//		ret = gpio_request(GPIO_EXAXP22(2), "TP_PW_EN");
//		if (ret < 0) {
//			pr_err("failed to request TP_PW_EN \n");
//		}
//		gpio_direction_output(GPIO_EXAXP22(2),  !!on);
//		gpio_set_value(GPIO_EXAXP22(2), !!on);
//		gpio_free(GPIO_EXAXP22(2));
//	}
//	mdelay(5);

#if 1 //FIXME: disable it for testing
	for(retry=0; retry < 4; retry++) {
		err =ft5x0x_i2c_txdata( NULL, 0);
		if (err > 0)
			break;
	}

	if(err <= 0) {
		dev_err(&client->dev, "Warnning: I2C connection might be something wrong!\n");
		return err;
	}
#endif
        TP_I2C_ADDR = client->addr;

	check_model();

	
	//ts->driver->ts_thread = kthread_run(vtl_ts_handler, NULL, DRIVER_NAME);
	ts->driver->ts_thread = kthread_run(vtl_ts_handler, ts, DRIVER_NAME);
	if (IS_ERR(ts->driver->ts_thread)) {
		err = PTR_ERR(ts->driver->ts_thread);
		ts->driver->ts_thread = NULL;
		dev_err(dev, "failed to create kernel thread: %d\n", err);
		return -1;
		//goto ERR_CREATE_TS_THREAD;
	}

	printk("___%s() end____ \n", __func__);
	g_touchscreen_init =1;
	return 0;
	
}

struct i2c_driver vtl_ts_driver  = {
	
	.driver = {
		.owner	= THIS_MODULE,
		.name	= DRIVER_NAME
	},
	.id_table	= vtl_ts_id,
	.probe      	= vtl_ts_probe,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= vtl_ts_suspend,
	.resume	    	= vtl_ts_resume,
#endif
	.remove 	= __devexit_p(vtl_ts_remove),
};


int __init vtl_ts_init(void)
{
	DEBUG();
	 if(g_touchscreen_init)
	 	return 0;
	return i2c_add_driver(&vtl_ts_driver);
}

void __exit vtl_ts_exit(void)
{
	DEBUG();
	i2c_del_driver(&vtl_ts_driver);
}

late_initcall(vtl_ts_init);
module_exit(vtl_ts_exit);

MODULE_AUTHOR("yangdechu@vtl.com.cn");
MODULE_DESCRIPTION("VTL touchscreen driver for rockchip,V1.0");
MODULE_LICENSE("GPL");


