#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/sizes.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>
#include "../../fs/proc/internal.h"

#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <linux/mutex.h>

#include <linux/jiffies.h>

#include <asm/mach-types.h>
#include <linux/hrtimer.h>
#include <../../../drivers/staging/android/timed_output.h>
#include <linux/workqueue.h>


#define PROC_NAME		"vibrator"
#define MOTOR_PIN            EXYNOS4212_GPM1(6)

//#define MOTOR_INTERVAL_MAX 5000 // ms
//#define MOTOR_INTERVAL_MIN  100   
static unsigned int motor_timer_interval;

struct work_struct motor_enable_work;
struct work_struct motor_disable_work;
struct workqueue_struct *motor_workqueue;

//static void motor_timer_work(unsigned int data);
//static struct timer_list motor_timer =
//		TIMER_INITIALIZER(motor_timer_work, 0, 0);

static int motor_event_enable= 0;
//static int motor_delay = 130;
static int motor_delay = 0;

extern int g_motor_value;

static struct motor{
	struct  hrtimer timer;
	struct mutex motor_mp;
}motor;



/*************************************
*       1 :queue_work for set motor state : 0 --off ; 1 -- on
*	 2: mod the shake time
************************************/
static unsigned int motor_write( int value)
{

	int ret = -1;
	printk("%s , motor_event_enable:%d\n",__func__,motor_event_enable);
#if 1	
	if(value > 0)
		motor_event_enable = 1;
	else
		motor_event_enable = 0;
	
	if(motor_event_enable)
		 gpio_direction_output(MOTOR_PIN,1);
	else
		gpio_direction_output(MOTOR_PIN,0);
#endif

	printk("MOTOR_PIN:%d\n",gpio_get_value(MOTOR_PIN));

	return 0;
}

static unsigned int motor_read(void)
{
	int ret = -1;
	ret = gpio_get_value( MOTOR_PIN);
	
	return ret;	
}

static int motor_proc_write(struct file *file, const char *buffer, 
                           unsigned long count, void *data) 
{ 
	signed int value = 3000;
	sscanf(buffer,"%d",&value);
	//if(strncmp(buffer,"enable",6) == 0)
//	motor_delay = value;
	
	if(value)
		motor_write(1);
	else
		motor_write(0);
	
	//motor_write(3000);
	printk("%s,value:%d\n",__func__,value);
	
    return count; 
} 

static int motor_proc_read(char *page, char **start, off_t off, 
			  int count, int *eof, void *data) 
{
	int len;
	int value;
	value = motor_read();
	len = sprintf(page," %d ;\n usage:\n write enable: echo 1 > %s \n write disable: echo 0 > %s \n",value,PROC_NAME,PROC_NAME);
	//printk("usage:\n write enable: echo 1 > %s \n write disable: echo 0 > %s \n",PROC_NAME,PROC_NAME);
	printk("motor_delay:%d\n",motor_delay);
    return len;
}

static void set_motor(int value)
{
	
	gpio_direction_output(MOTOR_PIN,value);
}

static void motor_work_on(struct work_struct * work)
{
	
	set_motor(1);
}

static void motor_work_off(struct work_struct * work)
{	
	
	set_motor(0);
}


static void timed_vibrator_on(void)
{
	
	schedule_work(&motor_enable_work);
}

static void timed_vibrator_off(void)
{
	
	schedule_work(&motor_disable_work);
}

static enum hrtimer_restart vibrate_timer_func(struct hrtimer *timer)
{
	
	timed_vibrator_off();
	return HRTIMER_NORESTART;
}


static void vibrate_enable(struct timed_output_dev *sdev, int timeout){
	//printk("%s,timeout:%d\n",__func__,timeout);
	mutex_lock(&motor.motor_mp);
	
	hrtimer_cancel(&motor.timer);

	if(timeout ){
		timed_vibrator_on();
		
		if(timeout <= motor_delay)
			timeout = motor_delay;
		
		//mdelay(90);  //must add ;for suppport keyboard
		//mdelay(motor_delay);
		
		//timeout = (timeout > 15000 ? 15000 : timeout);
		//printk("value:%d\n",timeout);
		
		hrtimer_start(&motor.timer,
		ns_to_ktime((u64)timeout * NSEC_PER_MSEC),
		HRTIMER_MODE_REL);

		}	
	else
		{
				timed_vibrator_off();	
		}

	mutex_unlock(&motor.motor_mp);

}


static int vibrate_get_time(struct timed_output_dev *sdev){
	if(hrtimer_active(&motor.timer)){
		ktime_t r = hrtimer_get_remaining(&motor.timer);
		return ktime_to_ms(r);
	}
	return 0;
} 
	
static struct timed_output_dev dev_motor = {
	.name = PROC_NAME,
	.get_time = vibrate_get_time,
	.enable = vibrate_enable,
};

extern char g_selected_motor[32];
static int __init motor_init(void)
{
	struct proc_dir_entry *entry;
	int err = -1;

	if(strcmp(g_selected_motor,"yes")){
		printk("not have motor\n");
		return 0;
	}
	motor_delay = g_motor_value;
	printk("%s motor_delay = %d\n",__func__,motor_delay);

	entry = create_proc_entry(PROC_NAME, 0666, &proc_root);
	if(entry)
	{
		entry->write_proc = motor_proc_write;
		entry->read_proc = motor_proc_read;
	}

	err = gpio_request(MOTOR_PIN, "motor-pin");
	if(err)
	{
		printk("failed request MOTOR_PIN\n");
		//goto err_motor_request;
	}

	mutex_init(&motor.motor_mp);

	INIT_WORK(&motor_enable_work,motor_work_on);
	INIT_WORK(&motor_disable_work,motor_work_off);
	
	hrtimer_init(&motor.timer,CLOCK_MONOTONIC,HRTIMER_MODE_REL);
	motor.timer.function = vibrate_timer_func;

	timed_output_dev_register(&dev_motor);

	printk("urbest motor: add motor proc file\n");
	return 0;

err_motor_request:
	remove_proc_entry(PROC_NAME, &proc_root);
	return  -1;
	
}

module_init(motor_init);

static void __exit motor_exit(void)
{
	if(strcmp(g_selected_motor,"yes")){
		printk("not have motor\n");
		return 0;
	}
	timed_output_dev_unregister(&dev_motor);
	mutex_destroy(&motor.motor_mp);
	gpio_free(MOTOR_PIN);
	remove_proc_entry(PROC_NAME, &proc_root);	
}

module_exit(motor_exit);

MODULE_DESCRIPTION("motor");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("urbest, inc.");
