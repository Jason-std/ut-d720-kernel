#include <linux/i2c.h>
#include <linux/input.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
    #include <linux/pm.h>
    #include <linux/earlysuspend.h>
#endif
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
//#include <mach/sys_config.h>

//#include "ctp_platform_ops.h"

#include <mach/gpio.h>
#include <linux/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/irq.h>
#include <linux/ioc4.h>
#include <linux/io.h>




#define CTP_IRQ_NO			(IRQ_CTP_INT)


#define FT_INT_PORT		GPIO_CTP_INT
#define FT_WAKE_PORT 	      EXYNOS4212_GPM3(4)
#define  INT_CFG    	S3C_GPIO_SFN(0xf)



#define UC_NAME	"uc_ts"
/* -------------- global variable definition -----------*/
static struct i2c_client *this_client;


//#define RESOLUTION

#if defined(RESOLUTION)
/* RESOLUTION 800*480 */	
#define SCREEN_MAX_X			(800)
#define SCREEN_MAX_Y			(480)
#define X_AXIS_MAPPING 0x12 //18=(14333/800) 
#define Y_AXIS_MAPPING 0x11 //17=(8192/480)
#else
/* RESOLUTION 1024*600 */
#define SCREEN_MAX_X			(1024)
#define SCREEN_MAX_Y			(600)
#define X_AXIS_MAPPING 0xE //14=(14333/1024) 
#define Y_AXIS_MAPPING 0xE //13.7=(8192/600)
#endif

#define PRESS_MAX			(1023)
	

/* Addresses to scan */
static union{
	unsigned short dirty_addr_buf[2];
	const unsigned short normal_i2c[2];
}u_i2c_addr = {{0x00},};
static __u32 twi_id = 0;

struct us_info {	
	u16 	x_max;
	u16 	y_max;
	u16	z_max;
	u16	dpi;
	u8 	chip_id;
	u8 	firmware_ver;
	u8	finger_num;
	u8	button_num;
	u32	project_ver;
};


struct us_ts_event {
	u16    x;
	u16    y;
	u16    pressure;
	u8     pen_state;
	u8     button_state;	
};

struct uc_ts_data {
	struct input_dev        *input_dev;
	struct us_ts_event      event;
	struct work_struct      pen_event_work;
	struct workqueue_struct *ts_workqueue;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend	early_suspend;
#endif
};


int uc_ts_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	
	pr_info("%s adapter->nr %d\n",__func__,adapter->nr);
	
	if(twi_id == adapter->nr){
		pr_info("%s: Detected chip %s at adapter %d, address 0x%02x\n",
			 __func__, UC_NAME, i2c_adapter_id(adapter), client->addr);

		strlcpy(info->type, UC_NAME, I2C_NAME_SIZE);
		return 0;
	}else{
	        
		return -ENODEV;
	}
}

static int uc_i2c_rxdata(char *rxdata, int length, u8 cmd)
{
	int ret;
        u8 read_cmd[1] = {cmd};
        
	struct i2c_msg msgs[2] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= read_cmd,
		},
		{
			.addr	= this_client->addr,
			.flags	= I2C_M_RD,
			.len	= length,
			.buf	= rxdata,
		},
	};

        //printk("IIC addr = %x\n",this_client->addr);
	ret = i2c_transfer(this_client->adapter, msgs, 2);
	if (ret < 0)
		printk("msg %s i2c read error: %d\n", __func__, ret);
	
	return ret;
}

static void show_buf_data(char *rxdata,int len)
{
        int i = len;
        for (i = 0; i < 8; i++) {
            printk("DATA[%d] = %#x \n",i,rxdata[i]);
        }    
    
}


static int uc_read_data(void)
{
	struct uc_ts_data *data = i2c_get_clientdata(this_client);
	struct us_ts_event *event = &data->event;
	unsigned char buf[8]={0};
	int ret = 0;
	
	//disable_irq(SW_INT_IRQNO_PIO);
	
	ret = uc_i2c_rxdata(buf, 8, 0x00);
	
	if (ret < 0) {
	    printk("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
        } else {
            printk("%s read_data i2c_rxdata OK: %d\n", __func__, ret);
            //show_buf_data(buf,8);
        }
        
        //paser buffer data
	memset(event, 0, sizeof(struct us_ts_event));
	
	//pr_info("======================uc_read_data=============================\n");        
        
        event->pen_state = (buf[1] & 0x10) > 4;// 0001 0000
        //pr_info("%s pen_state = %#x\n", __func__, event->pen_state);
        
        event->button_state = buf[1] & 0x07;// 0000 0111
        //pr_info("%s button_state = %#x\n", __func__, event->button_state);
        
        //pr_info("X_low=%#x, X_high=%#x \n",buf[2],buf[3]);
        event->x = (u16)( (buf[3] << 8) | buf[2]);
        //pr_info("%s X-axis = %#x\n", __func__, event->x);
        
        
        //pr_info("Y_low=%#x, Y_high=%#x \n",buf[4],buf[5]);
        event->y = (u16)( (buf[5] << 8) | buf[4]);
        //pr_info("%s Y-axis = %#x\n", __func__, event->y);
        
        //pr_info("Z_low=%#x, Z_high=%#x \n",buf[6],buf[7]);
        event->pressure = (u16)( ((buf[7] & 0x07) << 8) | buf[6]);
        //pr_info("%s Z-axis = %#x\n", __func__, event->pressure);
        
        
        if (event->pen_state) {
        	//pr_info("input report\n");
		input_report_abs(data->input_dev, ABS_X, (event->x)/X_AXIS_MAPPING);
		input_report_abs(data->input_dev, ABS_Y, (event->y)/Y_AXIS_MAPPING);
		input_report_abs(data->input_dev, ABS_PRESSURE, event->pressure);
		input_report_key(data->input_dev, BTN_TOUCH, 1);
		input_report_key(data->input_dev, BTN_TOOL_PEN, 1);
	} else {
		//pr_info("input release\n");
		input_report_abs(data->input_dev, ABS_PRESSURE, 0);
		input_report_key(data->input_dev, BTN_TOUCH, 0);
		input_report_key(data->input_dev, BTN_TOOL_PEN, 0);				
	}
		
	
	input_sync(data->input_dev);   
        
        //enable_irq(SW_INT_IRQNO_PIO);

        return ret;
    
}


static int uc_read_digitizer_info(void)
{
	struct us_info info;
	unsigned char buf2[5]={0};
	int ret = 0;
	
	
#if 1	
	unsigned char buf[11]={0};
	u16 tmp;
	
	printk("read 0x1A\n");
        ret = uc_i2c_rxdata(buf, 11, 0x1A);
	if (ret < 0) {
	    printk("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
        } else {
            printk("%s read_data i2c_rxdata OK: %d\n", __func__, ret);
        }
        
        //paser buffer data	
	printk("===================uc_read_digitizer_info================================\n");        
        
        printk("x_max_low=%#x, y_max_high=%#x \n",buf[0],buf[1]);
        info.x_max = (u16)( (buf[1] << 8) | buf[0]);
        printk("%s info.x_max = %#x\n", __func__, info.x_max);        

        printk("y_max_low=%#x, Y_max_high=%#x \n",buf[2],buf[3]);
        info.y_max = (u16)( (buf[3] << 8) | buf[2]);
        printk("%s info.y_max = %#x\n", __func__, info.y_max);        
        
        printk("info.chip_id=%#x\n",buf[4]);
        info.chip_id = buf[4];
        printk("%s info.chip_id = %#x\n", __func__, info.chip_id);
        
        printk("info.firmware_ver=%#x\n",buf[5]);
        info.firmware_ver = buf[5];
        printk("%s info.firmware_ver = %#x\n", __func__, info.firmware_ver);
                
        printk("Proj_low=%#x, Proj_Mid=%#x, Proj_high=%#x \n",buf[8],buf[7],buf[6]);
        tmp = (u16)( (buf[7] << 8) | buf[8]);
        info.project_ver = (u32)((buf[6] << 16) | tmp);
        printk("%s info.project_ver = %#x\n", __func__, info.project_ver);
                
        printk("info.finger_num=%#x\n",buf[10]);
        info.finger_num = buf[10];
        printk("%s info.finger_num = %#x\n", __func__, info.finger_num);
        
#endif
	printk("read 0x40\n");
        
        ret = uc_i2c_rxdata(buf2, 5, 0x40);
	if (ret < 0) {
	    printk("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
        } else {
            printk("%s read_data i2c_rxdata OK: %d\n", __func__, ret);
        }        

        printk("Z_max_low=%#x, Z_max_high=%#x \n",buf2[0],buf2[1]);
        info.z_max = (u16)( (buf2[1] << 8) | buf2[0]);
        printk("%s info.z_max = %#x\n", __func__, info.z_max);   

        printk("dpi_low=%#x, dpi_high=%#x \n",buf2[2],buf2[3]);
        info.dpi = (u16)( (buf2[3] << 8) | buf2[2]);
        printk("%s info.dpi = %#x\n", __func__, info.dpi);
        
        printk("info.button_num=%#x\n",buf2[4]);
        info.button_num = buf2[4];
        printk("%s info.button_num = %#x\n", __func__, info.button_num);           
        
        return ret;
    
}

static int uc_i2c_wxdata(char *txdata, int length)
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

	ret = i2c_transfer(this_client->adapter, msg, 1);
	if (ret < 0)
		printk("%s i2c write error: %d\n", __func__, ret);

	return ret;
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


static int uc_set_reg(u8 addr, u8 para)
{
	u8 buf[3];
	int ret = -1;

	buf[0] = addr;
	buf[1] = para;
	ret = uc_i2c_wxdata(buf, 2);
	if (ret < 0) {
		printk("write reg failed! %#x ret: %d\n", buf[0], ret);
		return -1;
	}

	return 0;
}


static int ft5x0x_set_reg(u8 addr, u8 para)
{
    u8 buf[3];
    int ret = -1;

    buf[0] = addr;
    buf[1] = para;
    ret = ft5x0x_i2c_txdata(buf, 2);
    if (ret < 0) {
        pr_err("write reg failed! %#x ret: %d", buf[0], ret);
        return -1;
    }
    
    return ret;
}


static int uc_digitizer_low_power_mode(int enable)
{
	int ret = 0;

	if (enable) {
		printk("Low Power Mode Enable write 0x38\n");
		ret = uc_set_reg(0x38,0x55);
		mdelay(15);
	}else {		
		printk("Low Power Mode Disable write 0x3E\n");
		printk("first \n");
		ret = uc_set_reg(0x3E,0x55);
		mdelay(15);
		printk("second \n");
		ret = uc_set_reg(0x3E,0x55);	
		ret = uc_set_reg(0x3E,0x55);	
		ret = uc_set_reg(0x3E,0x55);	
		ret = uc_set_reg(0x3E,0x55);	
		ret = uc_set_reg(0x3E,0x55);		
	}
	
	if (ret < 0) {
	    printk("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
        } else {
            printk("%s read_data i2c_rxdata OK: %d\n", __func__, ret);
        }
        return ret;
}




static void uc_work_func(struct work_struct *work)
{
        int ret;
	//printk("%s \n",__func__);
	ret = uc_read_data();
}


static irqreturn_t uc_ts_irq(int irq, void *dev_id)
{
        struct uc_ts_data *uc_ts = dev_id;
		
	printk("*");
	if(1){
		//printk("==IRQ_EINT21=\n");
		if (!work_pending(&uc_ts->pen_event_work)) 
		{
			//printk("Enter work\n");
			queue_work(uc_ts->ts_workqueue, &uc_ts->pen_event_work);
		}
	}else{
		//printk("Other Interrupt\n");
		return IRQ_NONE;
	}

	return IRQ_HANDLED;	

}


#ifdef CONFIG_HAS_EARLYSUSPEND
static void uc_ts_suspend(struct early_suspend *handler)
{
	uc_digitizer_low_power_mode(1);
	printk("==%s=\n",__func__);

}

static void uc_ts_resume(struct early_suspend *handler)
{
	uc_digitizer_low_power_mode(0);	
	printk("==%s=\n",__func__);

}
#endif  //CONFIG_HAS_EARLYSUSPEND


static __devinit uc_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct uc_ts_data *uc_ts;
	struct input_dev *input_dev;
	int err = 0;
	
	printk("\n\n\nuc_ts_probe OK \n\n\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}
	
	printk("i2c_check_functionality OK \n");

	uc_ts = kzalloc(sizeof(struct uc_ts_data), GFP_KERNEL);
	if (!uc_ts) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}
	
	printk("kzalloc OK \n");

	this_client = client;
	i2c_set_clientdata(client, uc_ts);

	INIT_WORK(&uc_ts->pen_event_work, uc_work_func);
	
	printk("INIT_WORK OK \n");

	uc_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
	if (!uc_ts->ts_workqueue) {
		err = -ESRCH;
		goto exit_create_singlethread;
	}

        printk("create_singlethread_workqueue OK \n");

#ifdef CONFIG_HAS_EARLYSUSPEND

	printk("\n [TSP]:register the early suspend \n");
	uc_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	uc_ts->early_suspend.suspend = uc_ts_suspend;
	uc_ts->early_suspend.resume  = uc_ts_resume;
	register_early_suspend(&uc_ts->early_suspend);

#endif


//	gpio_direction_output(FT_WAKE_PORT, 0);
//	msleep(100); 
//	gpio_direction_output(FT_WAKE_PORT, 1);
//	msleep(50); 
	gpio_direction_output(FT_WAKE_PORT, 0);
	msleep(100); 

	
	s3c_gpio_setpull(FT_INT_PORT, S3C_GPIO_PULL_UP);	
//	printk("INT_CFG = %d\n\n",INT_CFG);
//	s3c_gpio_cfgpin(FT_INT_PORT, 0xf);	//Set IO port function	

	this_client->irq = CTP_IRQ_NO;

	
	s3c_gpio_setpull(FT_INT_PORT, S3C_GPIO_PULL_UP);
	//set_irq_type(this_client->irq, IRQ_TYPE_LEVEL_LOW);
  	irq_set_irq_type(this_client->irq, IRQ_TYPE_EDGE_FALLING);


        err = request_irq(this_client->irq, uc_ts_irq, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, UC_NAME, uc_ts);
	if (err < 0) {
		dev_err(&client->dev, "uc_ts_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}

        printk("request_irq OK \n");
		
	disable_irq_nosync(this_client->irq);
	
	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
	
        printk("input_allocate_device OK \n");


	uc_ts->input_dev = input_dev;

	/***setup coordinate area******/
#if defined(RESOLUTION)	
        printk("RESOLUTION 800*480 \n");
#else
        printk("RESOLUTION 1024*600 \n");
#endif        	
	
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_TOOL_PEN, input_dev->keybit);

	set_bit(ABS_X, input_dev->absbit);
	set_bit(ABS_Y, input_dev->absbit);
	set_bit(ABS_PRESSURE, input_dev->absbit);
	

	input_set_abs_params(input_dev, ABS_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_PRESSURE, 0, PRESS_MAX, 0 , 0);

	input_dev->name = "uc-touch";
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
		"fts_ts_probe: failed to register input device: %s\n",
		dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}
	
        printk("input_register_device OK \n");
	
	uc_digitizer_low_power_mode(0);
	
	uc_read_digitizer_info();

	enable_irq(this_client->irq);
	
	printk("[TSP] file(%s), function (%s), -- end\n", __FILE__, __FUNCTION__);
	return 0;


exit_input_register_device_failed:
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
	free_irq(CTP_IRQ_NO, uc_ts);
exit_irq_request_failed:
	cancel_work_sync(&uc_ts->pen_event_work);
	destroy_workqueue(uc_ts->ts_workqueue);
exit_create_singlethread:
	printk("[TSP] ==singlethread error =\n");
	i2c_set_clientdata(client, NULL);
	kfree(uc_ts);
exit_alloc_data_failed:
exit_check_functionality_failed:
	return err;
}



static int __devexit uc_ts_remove(struct i2c_client *client)
{
	struct uc_ts_data *uc_ts;


        printk("==%s=\n",__func__);
        
	uc_ts = (struct uc_ts_data *)i2c_get_clientdata(client);
	free_irq(CTP_IRQ_NO, uc_ts);
	
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&uc_ts->early_suspend);
#endif	

	input_unregister_device(uc_ts->input_dev);
	kfree(uc_ts);
	cancel_work_sync(&uc_ts->pen_event_work);
	destroy_workqueue(uc_ts->ts_workqueue);
	i2c_set_clientdata(client, NULL);
	return 0;
}


static const struct i2c_device_id uc_ts_id[] = {
	{ UC_NAME, 0 },{ }
};

MODULE_DEVICE_TABLE(i2c, uc_ts_id);


static struct i2c_driver uc_ts_driver = {
	.probe        = uc_ts_probe,
	.remove        = __devexit_p(uc_ts_remove),
	.id_table    = uc_ts_id,
	.driver    = {
		.name = UC_NAME,
		.owner	= THIS_MODULE,
	},
};


static int __init uc_ts_init(void)
{
        int ret = -1;
    
        printk("\n\n\nuc_ts_init \n");
        
	
	twi_id = 2;
	
	printk("%s: twi_id is %d. \n", __func__, twi_id);
	ret = i2c_add_driver(&uc_ts_driver);

	printk("%s ret = %d\n\n",__func__,ret);
	    
	return ret;
}

static void __exit uc_ts_exit(void)
{
        printk("uc_ts_exit OK \n");
	i2c_del_driver(&uc_ts_driver);
}


module_init(uc_ts_init);
module_exit(uc_ts_exit);

MODULE_AUTHOR("<uc_logic@UC-Logic-Digitizer.com>");
MODULE_DESCRIPTION("UC-Logic Digitizer driver");
MODULE_LICENSE("GPL");
