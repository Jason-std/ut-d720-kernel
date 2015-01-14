#include <linux/interrupt.h>
#include <linux/i2c.h>
//#include <linux/i2c-id.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <linux/synclink.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include "../../fs/proc/internal.h"

#include <urbetter/check.h>
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#include <linux/suspend.h>
#endif

#define BH1721_DEBUG
#undef BH1721_DEBUG

#ifdef BH1721_DEBUG
	#define dprintk(x...) printk(x) 
#else
	#define dprintk(x...)  
#endif

#define  PROC_NAME  "LS_BH1721"

//==================================================================
#define POWER_DOWN 0x00//0000 0000
#define POWER_ON 0x01//0000 0001
#define AUTIO_MODE_BYTE_1 //0001_0000 0010_0000
#define AUTIO_MODE_BYTE_2
#define HIGHT_MODE //0001_0010 0010_0010
#define LOW_MODE   //0001_0011 0001_0110 0010_0011 0010_0110
#define CHANGE_MESURE_TIME_HIGH//010_MT[98765]
#define CHANGE_MESURE_TIME_LOW//011_MT[4]_XXXX
//==================================================================
static struct input_dev* bh1721_input=NULL;
static struct class *bh1721_class  = NULL;
struct device* bh1721_dev = NULL;
static int bh1721_major = 0;

#define GSENSOR_PROC_NAME "bh1721"
struct bh1770_chip {
};
static struct i2c_client * bh1721_i2c_client = NULL;
static struct work_struct read_work;
static void read_data(struct work_struct* work);
//static void lux_read_fun(unsigned long data);
static enum hrtimer_restart lux_read_fun(struct hrtimer *timer);
//static struct timer_list lux_read_timer = TIMER_INITIALIZER(lux_read_fun, 0, 0);
static int value = 0;

static struct input_event bh1721_event;
static char lux_value [2] = {10,20};
struct hrtimer timer;
ktime_t light_poll_delay;
 
static int bh1721_i2c_txdata(char *txdata, int length)
{
	int ret;

	struct i2c_msg msg[] = {
		{
			.addr	= bh1721_i2c_client->addr,
			.flags	= 0,
			.len	= length,
			.buf	= txdata,
		},
	};

   	//msleep(1);
	ret = i2c_transfer(bh1721_i2c_client->adapter, msg, 1);
	if (ret < 0)
		pr_err("%s i2c write error: %d\n", __func__, ret);

	return ret;
}



static enum hrtimer_restart lux_read_fun(struct hrtimer *timer)//read 
{
    //printk("--------%s----------\n",__FUNCTION__);
    schedule_work(&read_work);
	 //   mod_timer(&lux_read_timer, jiffies + 2*HZ);
      hrtimer_forward_now(timer,light_poll_delay);

	return HRTIMER_RESTART;
}


static void read_data(struct work_struct* work){
	int ret;
	struct i2c_msg msg[1];
	uint8_t buf[2];

	msleep(180);

	dprintk("before read : buf[0]:%d buf[1]:%d \n",buf[0],buf[1]);
	ret = i2c_master_recv(bh1721_i2c_client,  buf,2);
	 dprintk("ret :%d======buf[0]={%d} buf[1]={%d} buf = {%d}===================\n",ret,buf[0],buf[1],(buf[0]<<8) |buf[1] );

	bh1721_event.type =EV_MSC ;
	bh1721_event.code =MSC_RAW ;
	bh1721_event.value = (buf[0] << 8) + buf[1];

	lux_value[0] = buf[0] ;
	lux_value[1] = buf[1];

	input_event(bh1721_input,EV_MSC,MSC_RAW,(buf[0] << 8) + buf[1]);
	input_sync(bh1721_input);
	
 	//printk("======buf[0]{%d} buf[1]{%d} buf{%d}===================\n",buf[0],buf[1],(buf[0]<<8) |buf[1] );
}

#if 1
static int bh1721_open(struct inode *inode, struct file *file)
{
	printk("---%s---\n",__func__);
	return 0;
}
 
ssize_t bh1721_read(struct file *file, char __user * buf, size_t count, loff_t * ppos)
{
//printk("%s,lux_value[0]:%d lux_value[1]:%d\n",__func__,lux_value[0],lux_value[1]);
	if(count > 2)
		count = 2 ;
	if(copy_to_user(buf, lux_value, count))
		return -EFAULT;
	return sizeof(lux_value);

}

struct file_operations bh1721_fops = {	
	.owner = THIS_MODULE,
	.open = bh1721_open,		
	.read = bh1721_read,	
	};
#endif

static int __devinit bh1721_i2c_probe(struct i2c_client * client, const struct i2c_device_id * id)
{
	  int ret = 0,retry = 0;
	  int err = 0;
	  struct bh1770_chip *chip;
	  chip = kzalloc(sizeof *chip, GFP_KERNEL);
	 if (!chip)
	      return -ENOMEM;

	if(CHECK_UTMODEL("d1011") && (client->adapter->nr==5))
		return -1;
	 
	i2c_set_clientdata(client, chip);
	
	  bh1721_i2c_client = client;
	  
	  printk("========================bh1721_i2c_probe============================\n");
	 printk("%s: addr=0x%x @ IIC%d, irq=%d\n",client->name,client->addr,client->adapter->nr,client->irq);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
	{
		printk(KERN_ERR "Rohm_ls_probe: need I2C_FUNC_I2C\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

   
	ret = i2c_smbus_write_byte(client, 0x01);	//0000 0001,	Power on
//	i2c_smbus_write_byte(client, 0x49);	//0100 1001,	
//	i2c_smbus_write_byte(client, 0x6C);	//0110 1100,	transimission rate is 100%
//	i2c_smbus_write_byte(client, 0x10);	//0001 0000,	1lx/step H-Res Continuously
	ret = i2c_smbus_write_byte(client, 0x22);  // 0010 0010  continuously H-resolution Mode
	msleep(100);

	
#if 1 //FIXME: disable it for testing
	for(retry=0; retry < 4; retry++) {
		err =bh1721_i2c_txdata( NULL, 0);
		if (err > 0)
			break;
	}

	if(err <= 0) {
		printk("222222222222222222222222222222222222222222222222 \n");
		dev_err(&client->dev, "Warnning: I2C connection might be something wrong!\n");
		return -1;
	}
#endif



	bh1721_input = input_allocate_device();
	
	if(!bh1721_input)
	 	printk("bh1721_input alloc Error");

 	 set_bit(EV_MSC, bh1721_input->evbit);
	set_bit(MSC_RAW, bh1721_input->mscbit);
	set_bit(MSC_SCAN, bh1721_input->mscbit);
	
	bh1721_input->name = GSENSOR_PROC_NAME;
  
 	 if(input_register_device(bh1721_input) != 0){
			printk("s3c-button input register device fail!!\n");
			input_free_device(bh1721_input);
	}

#if 1
	bh1721_major = register_chrdev(0, GSENSOR_PROC_NAME, &bh1721_fops);
	if(bh1721_major < 0){		
		printk("=======bh1721 register chrdev failed=======\n");		
		ret = bh1721_major;		
		return -1;	
		}

	bh1721_class = class_create(THIS_MODULE, GSENSOR_PROC_NAME);
	if (IS_ERR(bh1721_class)) 
		{		
		printk(KERN_ERR "%s Error creating lignt_sensor class\n", "<bh1721 light sensor>");		
		ret = -EIO;		
		return -1;	
	}		
	/* Register char device */	
	bh1721_dev = device_create(bh1721_class, NULL, MKDEV(bh1721_major, 0), NULL, GSENSOR_PROC_NAME);	
	if (IS_ERR(bh1721_dev)) {		
		printk(KERN_ERR "%s Error creating light_sensor class device\n", "<bh1721 light sensor>");		
		ret = -EIO;		
		return -1;	
		}
#endif


	hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	light_poll_delay = ns_to_ktime(200 * NSEC_PER_MSEC);
	timer.function = lux_read_fun;

	hrtimer_start(&timer, light_poll_delay,HRTIMER_MODE_REL);

//	lux_read_timer.expires =  jiffies + 10*HZ;
//	add_timer(&lux_read_timer);
	INIT_WORK(&read_work, read_data);

	printk("-----%s----over\n",__func__); 
err_check_functionality_failed:
  return ret;
}

static int bh1721fvc_suspend(struct device *dev)
{
	int err = 0;
	struct i2c_client *client = to_i2c_client(dev);

	hrtimer_cancel(&timer);
	cancel_work_sync(&read_work);
	 i2c_smbus_write_byte(client, 0x00);	//0000 0000,	Power down

	return err;
}

static int bh1721fvc_resume(struct device *dev)
{
	int err = 0;
	struct i2c_client *client = to_i2c_client(dev);

	 i2c_smbus_write_byte(client, 0x01);	//0000 0001,	Power on
	 i2c_smbus_write_byte(client, 0x22);  // 0010 0010  continuously H-resolution Mode
	 mdelay(150);

	hrtimer_start(&timer, light_poll_delay,HRTIMER_MODE_REL);

	return err;
}


static struct i2c_device_id bh1721_idtable[] = {
	{"bh1721", 0},
};

static const struct dev_pm_ops bh1721fvc_pm_ops = {
	.suspend	= bh1721fvc_suspend,
	.resume   = bh1721fvc_resume,
};

static struct i2c_driver bh1721_i2c_driver = 
{
	.driver = 
	{
		.name = "bh1721_i2c",
		.owner = THIS_MODULE,
		.pm =  &bh1721fvc_pm_ops,
	},
	.id_table = bh1721_idtable,
	.probe = bh1721_i2c_probe,

};

static int __init bh1721_init(void)
{	
	return i2c_add_driver(&bh1721_i2c_driver);
}

static void __exit bh1721_exit(void)
{
	i2c_del_driver(&bh1721_i2c_driver);
}
late_initcall(bh1721_init);
module_exit(bh1721_exit);
MODULE_AUTHOR("urbetter inc.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("urbetter bh1721 G-sensor");
