/*
 * drivers/input/touchscreen/gslX680.c
 *
 * Copyright (c) 2012 Shanghai Basewin
 *	Guan Yuwei<guanyuwei@basewin.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */



#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/earlysuspend.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/ioc4.h>
#include <linux/io.h>

#include <linux/proc_fs.h>

#include <mach/gpio.h>
//#include <mach/gpio-smdkv210.h>
#include <mach/hardware.h>
#include <plat/gpio-cfg.h>
#include <plat/irqs.h>

#include <linux/input/mt.h>
#include "gslx680.h"

#include <linux/kthread.h>

//#define  pr_err  printk
//#define GSL_DEBUG
//#define GSL_TIMER

//#define HAVE_TOUCH_KEY
#define GSLX680_I2C_NAME 	"gslx680"
#define GSLX680_I2C_ADDR 	0x40
#define IRQ_PORT			IRQ_CTP_INT//IRQ_EINT(8)

#define GSL_DATA_REG		0x80
#define GSL_STATUS_REG		0xe0
#define GSL_PAGE_REG		0xf0

#define PRESS_MAX    		255
#define MAX_FINGERS 		5
#define MAX_CONTACTS 		10
#define DMA_TRANS_LEN		0x20
#define GPIO_TS_WAKE             EXYNOS4212_GPM3(4)//GPIO_CTP_INT
#define GPIO_TS_EINT              IRQ_CTP_INT

//#define FILTER_POINT
#ifdef FILTER_POINT
#define FILTER_MAX	6
#endif

#ifdef HAVE_TOUCH_KEY
static u16 key = 0;
static int key_state_flag = 0;
struct key_data {
	u16 key;
	u16 x_min;
	u16 x_max;
	u16 y_min;
	u16 y_max;	
};

const u16 key_array[]={
                                      KEY_BACK,
                                      KEY_HOME,
                                      KEY_MENU,
                                      KEY_SEARCH,
                                     }; 
#define MAX_KEY_NUM     (sizeof(key_array)/sizeof(key_array[0]))

struct key_data gsl_key_data[MAX_KEY_NUM] = {
	{KEY_BACK, 2048, 2048, 2048, 2048},
	{KEY_HOME, 2048, 2048, 2048, 2048},	
	{KEY_MENU, 2048, 2048, 2048, 2048},
	{KEY_SEARCH, 2048, 2048, 2048, 2048},
};
#endif

struct gsl_ts_data {
	u8 x_index;
	u8 y_index;
	u8 z_index;
	u8 id_index;
	u8 touch_index;
	u8 data_reg;
	u8 status_reg;
	u8 data_size;
	u8 touch_bytes;
	u8 update_data;
	u8 touch_meta_data;
	u8 finger_size;
};

static struct gsl_ts_data devices[] = {
	{
		.x_index = 6,
		.y_index = 4,
		.z_index = 5,
		.id_index = 7,
		.data_reg = GSL_DATA_REG,
		.status_reg = GSL_STATUS_REG,
		.update_data = 0x4,
		.touch_bytes = 4,
		.touch_meta_data = 4,
		.finger_size = 70,
	},
};

struct gsl_ts {
	struct i2c_client *client;
	struct input_dev *input;
	struct work_struct work;
	struct workqueue_struct *wq;
	struct gsl_ts_data *dd;
	u8 *touch_data;
	u8 device_id;
	u8 prev_touches;
	bool is_suspended;
	bool int_pending;
	struct mutex sus_lock;
//	uint32_t gpio_irq;
	int irq;
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
#ifdef GSL_TIMER
	struct timer_list gsl_timer;
#endif

};
//#define GSL_DEBUG               //////////////
#ifdef GSL_DEBUG 
#define print_info(fmt, args...)   \
        do{                              \
                printk(fmt, ##args);     \
        }while(0)
#else
#define print_info(fmt, args...)
#endif


static u32 id_sign[MAX_CONTACTS+1] = {0};
static u8 id_state_flag[MAX_CONTACTS+1] = {0};
static u8 id_state_old_flag[MAX_CONTACTS+1] = {0};
static u16 x_old[MAX_CONTACTS+1] = {0};
static u16 y_old[MAX_CONTACTS+1] = {0};
static u16 x_new = 0;
static u16 y_new = 0;
static int s_kthread_run = 0;

static struct i2c_client *this_client;
extern s32 i2c_smbus_write_byte_data(const struct i2c_client *client, u8 command,u8 value);

struct delayed_work gsl_pen_event_work;
struct workqueue_struct *gsl_workqueue;

//add 

static int i2c_write_bytes(struct i2c_client *client,uint8_t *data,int len)
{
	struct i2c_msg msg;
	int ret=-1;
	int retries = 0;

	msg.flags=!I2C_M_RD;
	msg.addr=client->addr;
	msg.len=len;
	msg.buf=data;		
	
	while(retries<5)
	{
		ret=i2c_transfer(client->adapter,&msg, 1);
		if(ret == 1)break;
		retries++;
	}
	return ret;
}



static int i2c_txdata(char *txdata, int length)
{
	int ret;

	struct i2c_msg msg[] = {
		{
			.addr	=this_client->addr,
			.flags	= 0,
			.len	= length,
			.buf	= txdata,
		},
	};

   	//msleep(1);
	ret = i2c_transfer(this_client->adapter, msg, 1);
	if (ret < 0)
		printk("%s i2c write error: %d\n", __func__, ret);

	return ret;
}
//
static int set_reg(u8 addr, u8 para)
{
    u8 buf[3];
    int ret = -1;

    buf[0] = addr;
    buf[1] = para;
    ret = i2c_txdata(buf, 2);
    if (ret < 0) {
        printk("write reg failed! %#x ret: %d", buf[0], ret);
        return -1;
    }
    
    return ret;
}




static int gslX680_chip_init(void)
{
	gpio_request(GPIO_TS_WAKE, "ts-wake-en");
	//gpio_request(GPIO_TS_POWER, "ts-power-en");
	mdelay(50);
	//gpio_direction_output(GPIO_TS_WAKE, 1);
	gpio_direction_output(GPIO_TS_WAKE, 0);
	//gpio_direction_output(GPIO_TS_POWER, 1);  
	mdelay(50);
	s3c_gpio_setpull(GPIO_TS_EINT, S3C_GPIO_PULL_UP);
  	irq_set_irq_type(IRQ_PORT, IRQ_TYPE_EDGE_RISING);
	return 0;
}

static int gslX680_shutdown_low(void)
{
	//gpio_direction_output(GPIO_TS_WAKE, 0);
	gpio_direction_output(GPIO_TS_WAKE, 1);
	//mdelay(10);
	return 0;
}

static int gslX680_shutdown_high(void)
{
	//gpio_direction_output(GPIO_TS_WAKE, 1);
	gpio_direction_output(GPIO_TS_WAKE, 0);
	//mdelay(10);
	return 0;
}

static inline u16 join_bytes(u8 a, u8 b)
{
	u16 ab = 0;
	ab = ab | a;
	ab = ab << 8 | b;
	return ab;
}

static u32 gsl_read_interface(struct i2c_client *client, u8 reg, u8 *buf, u32 num)
{
	struct i2c_msg xfer_msg[2];

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = 1;
	xfer_msg[0].flags = client->flags & I2C_M_TEN;
	xfer_msg[0].buf = &reg;

	xfer_msg[1].addr = client->addr;
	xfer_msg[1].len = num;
	xfer_msg[1].flags |= I2C_M_RD;
	xfer_msg[1].buf = buf;

	if (reg < 0x80) {
		i2c_transfer(client->adapter, xfer_msg, ARRAY_SIZE(xfer_msg));
		msleep(5);
	}

	return i2c_transfer(client->adapter, xfer_msg, ARRAY_SIZE(xfer_msg)) == ARRAY_SIZE(xfer_msg) ? 0 : -EFAULT;
}

static u32 gsl_write_interface(struct i2c_client *client, const u8 reg, u8 *buf, u32 num)
{
	struct i2c_msg xfer_msg[1];

	buf[0] = reg;

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = num + 1;
	xfer_msg[0].flags = client->flags & I2C_M_TEN;
	xfer_msg[0].buf = buf;

	return i2c_transfer(client->adapter, xfer_msg, 1) == 1 ? 0 : -EFAULT;
}

static __inline__ void fw2buf(u8 *buf, const u32 *fw)
{
	u32 *u32_buf = (int *)buf;
	*u32_buf = *fw;
}

static void gsl_load_fw(struct i2c_client *client)
{
	u8 buf[DMA_TRANS_LEN*4 + 1] = {0};
	u8 send_flag = 1;
	u8 *cur = buf + 1;
	u32 source_line = 0;
	u32 source_len;
	u8 read_buf[4] = {0};
	struct fw_data *ptr_fw;
	
	printk("=============gsl_load_fw start==============\n");

#ifdef GSL1680E_COMPATIBLE
	msleep(100);
	gsl_read_interface(client, 0xfc, read_buf, 4);
	//printk("read 0xfc = %x %x %x %x\n", read_buf[3], read_buf[2], read_buf[1], read_buf[0]);

	if(read_buf[2] != 0x82 && read_buf[2] != 0x88)
	{
		msleep(100);
		gsl_read_interface(client, 0xfc, read_buf, 4);
		//printk("read 0xfc = %x %x %x %x\n", read_buf[3], read_buf[2], read_buf[1], read_buf[0]);		
	}
	
	if(read_buf[2] == 0x82)
	{
		ptr_fw = GSL1680E_FW;
		source_len = ARRAY_SIZE(GSL1680E_FW);	
	}
	else
#endif
	{
		ptr_fw = GSLX680_FW;
		source_len = ARRAY_SIZE(GSLX680_FW);
	}

	for (source_line = 0; source_line < source_len; source_line++) 
	{
		/* init page trans, set the page val */
		if (GSL_PAGE_REG == ptr_fw[source_line].offset)
		{
			fw2buf(cur, &ptr_fw[source_line].val);
			gsl_write_interface(client, GSL_PAGE_REG, buf, 4);
			send_flag = 1;
		}
		else 
		{
			if (1 == send_flag % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20))
	    			buf[0] = (u8)ptr_fw[source_line].offset;

			fw2buf(cur, &ptr_fw[source_line].val);
			cur += 4;

			if (0 == send_flag % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20)) 
			{
	    			gsl_write_interface(client, buf[0], buf, cur - buf - 1);
	    			cur = buf + 1;
			}

			send_flag++;
		}
	}

	printk("=============gsl_load_fw end==============\n");

}
struct mutex i2c_lock;


static int gsl_ts_write(struct i2c_client *client, u8 addr, u8 *pdata, int datalen)
{
	int ret = 0;
	u8 tmp_buf[128];
	//u8 tmp_buf[2]={addr,};
	unsigned int bytelen = 0;
	if (datalen > 125)
	{
		printk("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}
	
	tmp_buf[0] = addr;
	bytelen++;
//	mutex_lock(&i2c_lock);
	
#if 1
	if (datalen != 0 && pdata != NULL)
	{   
		memcpy(&tmp_buf[bytelen], pdata, datalen);
		bytelen += datalen;

	}
#endif
       
	ret = i2c_master_send(client, tmp_buf, bytelen);
     
//	mutex_unlock(&i2c_lock);

	return ret;
}

static int gsl_ts_read(struct i2c_client *client, u8 addr, u8 *pdata, unsigned int datalen)
{
	int ret = 0;

	if (datalen > 126)
	{
		printk("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}

	ret = gsl_ts_write(client, addr, NULL, 0);
	//ret =i2c_smbus_write_byte_data(client, addr, NULL);
	if (ret < 0)
	{
		printk("%s set data address fail!\n", __func__);
		return ret;
	}
	
	return i2c_master_recv(client, pdata, datalen);
}

static void test_i2c(struct i2c_client *client)
{
	u8 read_buf = 0;
	u8 write_buf = 0x12;
	int ret;
	ret = gsl_ts_read( client, 0xf0, &read_buf, sizeof(read_buf) );
	if  (ret  < 0)  
	{
		pr_info("I2C transfer error!\n");
	}
	else
	{
		pr_info("I read reg 0xf0 is %x\n", read_buf);
	}
	msleep(10);

	ret = gsl_ts_write(client, 0xf0, &write_buf, sizeof(write_buf));
	//ret = i2c_smbus_write_byte_data(client, 0xf0, 0x12);
	if  (ret  < 0)  
	{
		pr_info("I2C transfer error!\n");
	}
	else
	{
		pr_info("I write reg 0xf0 0x12\n");
	}
	msleep(10);

	ret = gsl_ts_read( client, 0xf0, &read_buf, sizeof(read_buf) );
	if  (ret  <  0 )
	{
		pr_info("I2C transfer error!\n");
	}
	else
	{
		pr_info("I read reg 0xf0 is 0x%x\n", read_buf);
	}
	msleep(10);

}


static void startup_chip(struct i2c_client *client)
{
	u8 tmp = 0x00;
	printk("---%s---\n",__func__);
	gsl_ts_write(client, 0xe0, &tmp, 1);
	//i2c_smbus_write_byte_data(client, 0xe0, tmp);
	//set_reg(0xe0,tmp);
	mdelay(5);	
	
}


static void reset_chip(struct i2c_client *client)
{

	
	u8 buf[4] = {0x00};
	u8 tmp = 0x88;
	//printk("--1-%s---\n",__func__);
	gsl_ts_write(client, 0xe0, &tmp, sizeof(tmp));
	//i2c_smbus_write_byte_data(client, 0xe0, tmp);
	//set_reg(0xe0,tmp);
	mdelay(5);
        //printk("--2-%s---\n",__func__);
	tmp = 0x04;
	gsl_ts_write(client, 0xe4, &tmp, sizeof(tmp));
	//i2c_smbus_write_byte_data(client, 0xe4, tmp);
	//set_reg(0xe4,tmp);
	mdelay(5);
       // printk("--3-%s---\n",__func__);
	gsl_ts_write(client, 0xbc, buf, sizeof(buf));
	//i2c_smbus_write_byte_data(client, 0xbc, 0x00);
	//set_reg(0xe0,0x00);
	mdelay(5);
}

static void init_chip(struct i2c_client *client)
{
	gslX680_shutdown_low();	
	msleep(20); 	
	gslX680_shutdown_high();	
	msleep(10); 		
	//test_i2c(client);
	reset_chip(client);
	gsl_load_fw(client);			
	startup_chip(client);
	reset_chip(client);
	startup_chip(client);	
}

static void check_mem_data(struct i2c_client *client)
{
	char write_buf;
	char read_buf[4]  = {0};
	int ret;
	mdelay(10);
	write_buf = 0x00;
	gsl_ts_write(client,0xf0, &write_buf, sizeof(write_buf));
	mdelay(5);
	//ret=i2c_smbus_write_byte_data(client, 0xf0, write_buf);
	//set_reg(0xf0,write_buf);
	//printk("***********ret = %d\n",ret);
	gsl_ts_read(client,0x00, read_buf, sizeof(read_buf));
	mdelay(5);
	gsl_ts_read(client,0x00, read_buf, sizeof(read_buf));
	if (read_buf[3] != 0x1 || read_buf[2] != 0 || read_buf[1] != 0 || read_buf[0] != 0)
	{
		printk("!!!!!!!!!!!page: %x offset: %x val: %x %x %x %x\n",0x0, 0x0, read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		init_chip(client);
	}
}

#ifdef FILTER_POINT
static void filter_point(u16 x, u16 y , u8 id)
{
	u16 x_err =0;
	u16 y_err =0;
	u16 filter_step_x = 0, filter_step_y = 0;
	
	id_sign[id] = id_sign[id] + 1;
	if(id_sign[id] == 1)
	{
		x_old[id] = x;
		y_old[id] = y;
	}
	
	x_err = x > x_old[id] ? (x -x_old[id]) : (x_old[id] - x);
	y_err = y > y_old[id] ? (y -y_old[id]) : (y_old[id] - y);

	if( (x_err > FILTER_MAX && y_err > FILTER_MAX/3) || (x_err > FILTER_MAX/3 && y_err > FILTER_MAX) )
	{
		filter_step_x = x_err;
		filter_step_y = y_err;
	}
	else
	{
		if(x_err > FILTER_MAX)
			filter_step_x = x_err; 
		if(y_err> FILTER_MAX)
			filter_step_y = y_err;
	}

	if(x_err <= 2*FILTER_MAX && y_err <= 2*FILTER_MAX)
	{
		filter_step_x >>= 2; 
		filter_step_y >>= 2;
	}
	else if(x_err <= 3*FILTER_MAX && y_err <= 3*FILTER_MAX)
	{
		filter_step_x >>= 1; 
		filter_step_y >>= 1;
	}	

	x_new = x > x_old[id] ? (x_old[id] + filter_step_x) : (x_old[id] - filter_step_x);
	y_new = y > y_old[id] ? (y_old[id] + filter_step_y) : (y_old[id] - filter_step_y);

	x_old[id] = x_new;
	y_old[id] = y_new;
}
#else

static void record_point(u16 x, u16 y , u8 id)
{
	u16 x_err =0;
	u16 y_err =0;

	id_sign[id]=id_sign[id]+1;
	
	if(id_sign[id]==1){
		x_old[id]=x;
		y_old[id]=y;
	}

	x = (x_old[id] + x)/2;
	y = (y_old[id] + y)/2;
		
	if(x>x_old[id]){
		x_err=x -x_old[id];
	}
	else{
		x_err=x_old[id]-x;
	}

	if(y>y_old[id]){
		y_err=y -y_old[id];
	}
	else{
		y_err=y_old[id]-y;
	}

	if( (x_err > 3 && y_err > 1) || (x_err > 1 && y_err > 3) ){
		x_new = x;     x_old[id] = x;
		y_new = y;     y_old[id] = y;
	}
	else{
		if(x_err > 3){
			x_new = x;     x_old[id] = x;
		}
		else
			x_new = x_old[id];
		if(y_err> 3){
			y_new = y;     y_old[id] = y;
		}
		else
			y_new = y_old[id];
	}

	if(id_sign[id]==1){
		x_new= x_old[id];
		y_new= y_old[id];
	}
	
}
#endif

#ifdef HAVE_TOUCH_KEY
static void report_key(struct gsl_ts *ts, u16 x, u16 y)
{
	u16 i = 0;

	for(i = 0; i < MAX_KEY_NUM; i++) 
	{
		if((gsl_key_data[i].x_min < x) && (x < gsl_key_data[i].x_max)&&(gsl_key_data[i].y_min < y) && (y < gsl_key_data[i].y_max))
		{
			key = gsl_key_data[i].key;	
			input_report_key(ts->input, key, 1);
			input_sync(ts->input); 		
			key_state_flag = 1;
			break;
		}
	}
}
#endif

static void report_data(struct gsl_ts *ts, u16 x, u16 y, u8 pressure, u8 id)
{
	swap(x, y);

	print_info("#####id=%d,x=%d,y=%d######\n",id,x,y);

	if(x>=SCREEN_MAX_X||y>=SCREEN_MAX_Y)
	{
	#ifdef HAVE_TOUCH_KEY
		report_key(ts,x,y);
	#endif
		return;
	}

	input_mt_slot(ts->input, id);
	input_report_abs(ts->input, ABS_MT_TRACKING_ID, id);
	input_report_abs(ts->input, ABS_MT_POSITION_X, x);
	input_report_abs(ts->input, ABS_MT_POSITION_Y, y);
	input_report_abs(ts->input, ABS_MT_PRESSURE, pressure); 
	input_report_key(ts->input, BTN_TOUCH, 1);
}

static void process_gslX680_data(struct gsl_ts *ts)
{
	u8 id, touches;
	u16 x, y;
	int i = 0;
      // printk("---%s--\n",__func__);
	touches = ts->touch_data[ts->dd->touch_index];
	for(i=1;i<=MAX_CONTACTS;i++)
	{
		if(touches == 0)
			id_sign[i] = 0;	
		id_state_flag[i] = 0;
	}
	for(i= 0;i < (touches > MAX_FINGERS ? MAX_FINGERS : touches);i ++)
	{
		x = join_bytes( ( ts->touch_data[ts->dd->x_index  + 4 * i + 1] & 0xf),
				ts->touch_data[ts->dd->x_index + 4 * i]);
		y = join_bytes(ts->touch_data[ts->dd->y_index + 4 * i + 1],
				ts->touch_data[ts->dd->y_index + 4 * i ]);
		id = ts->touch_data[ts->dd->id_index + 4 * i] >> 4;

		if(1 <=id && id <= MAX_CONTACTS)
		{
		#ifdef FILTER_POINT
			filter_point(x, y ,id);
		#else
			record_point(x, y , id);
		#endif
			//report_data(ts, x_new, y_new, 10, id);
			report_data(ts, y_new, x_new, 10, id);
			id_state_flag[id] = 1;
		}
	}
	for(i=1;i<=MAX_CONTACTS;i++)
	{	
		if( (0 == touches) || ((0 != id_state_old_flag[i]) && (0 == id_state_flag[i])) )
		{
			input_mt_slot(ts->input, i);
			input_report_abs(ts->input, ABS_MT_TRACKING_ID, -1);
			input_report_key(ts->input, BTN_TOUCH,  0);		
			id_sign[i]=0;
		}
		id_state_old_flag[i] = id_state_flag[i];
	}
	if(0 == touches)
	{	
	#ifdef HAVE_TOUCH_KEY
		if(key_state_flag)
		{
        		input_report_key(ts->input, key, 0);
			input_sync(ts->input);
			key_state_flag = 0;
		}
	#endif			
	}
	input_sync(ts->input);
	ts->prev_touches = touches;
}


static void gsl_ts_xy_worker(struct work_struct *work)
{
	int rc;
	u8 read_buf[4] = {0};

	struct gsl_ts *ts = container_of(work, struct gsl_ts,work);

	print_info("---gsl_ts_xy_worker---\n");				 

	/* read data from DATA_REG */
	rc = gsl_ts_read(ts->client, 0x80, ts->touch_data, ts->dd->data_size);
	print_info("---touches: %d ---\n",ts->touch_data[0]);		
		
	if (rc < 0) 
	{
		dev_err(&ts->client->dev, "read failed\n");
		goto schedule;
	}

	if (ts->touch_data[ts->dd->touch_index] == 0xff) {
		goto schedule;
	}

	rc = gsl_ts_read( ts->client, 0xbc, read_buf, sizeof(read_buf));
	if (rc < 0) 
	{
		dev_err(&ts->client->dev, "read 0xbc failed\n");
		goto schedule;
	}
	print_info("xxxxxxxx reg %x : %x %x %x %x\n",0xbc, read_buf[3], read_buf[2], read_buf[1], read_buf[0]);

	if (read_buf[3] == 0 && read_buf[2] == 0 && read_buf[1] == 0 && read_buf[0] == 0)
	{
		process_gslX680_data(ts);
	}
	else
	{
		reset_chip(ts->client);
		startup_chip(ts->client);
		//printk("*************************\n");
	}
	
schedule:
	enable_irq(ts->irq);
		
}

static irqreturn_t gsl_ts_irq(int irq, void *dev_id)
{	
	struct gsl_ts *ts = dev_id;

	//print_info("==========GSLX680 Interrupt============\n");				 
        //printk("==========GSLX680 Interrupt============\n");		
	disable_irq_nosync(ts->irq);

	if (!work_pending(&ts->work)) 
	{
		queue_work(ts->wq, &ts->work);
	}
	
	return IRQ_HANDLED;

}

#ifdef GSL_TIMER
static void gsl_timer_handle(unsigned long data)
{
	struct gsl_ts *ts = (struct gsl_ts *)data;

#ifdef GSL_DEBUG	
	printk("----------------gsl_timer_handle-----------------\n");	
#endif

	disable_irq_nosync(ts->irq);	
	check_mem_data(ts->client);
	ts->gsl_timer.expires = jiffies + 3 * HZ;
	add_timer(&ts->gsl_timer);
	enable_irq(ts->irq);
	
}
#endif
static int gsl_ts_init_ts(struct i2c_client *client, struct gsl_ts *ts)
{
	struct input_dev *input_device;
	int i, rc = 0;
	
	printk("[GSLX680] Enter %s\n", __func__);

	
	ts->dd = &devices[ts->device_id];

	if (ts->device_id == 0) {
		ts->dd->data_size = MAX_FINGERS * ts->dd->touch_bytes + ts->dd->touch_meta_data;
		ts->dd->touch_index = 0;
	}

	ts->touch_data = kzalloc(ts->dd->data_size, GFP_KERNEL);
	if (!ts->touch_data) {
		pr_err("%s: Unable to allocate memory\n", __func__);
		return -ENOMEM;
	}

	ts->prev_touches = 0;

	input_device = input_allocate_device();
	if (!input_device) {
		rc = -ENOMEM;
		goto error_alloc_dev;
	}

	ts->input = input_device;
	input_device->name = "ft5x0x_ts";//GSLX680_I2C_NAME;
	input_device->id.bustype = BUS_I2C;
	input_device->dev.parent = &client->dev;
	input_set_drvdata(input_device, ts);

	set_bit(ABS_MT_POSITION_X, input_device->absbit);
	set_bit(ABS_MT_POSITION_Y, input_device->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, input_device->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_device->absbit);
	set_bit(ABS_PRESSURE, input_device->absbit);
	set_bit(BTN_TOUCH, input_device->keybit);
	set_bit(EV_ABS, input_device->evbit);
	set_bit(EV_KEY, input_device->evbit);
	set_bit(EV_SYN,input_device->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_device->propbit);
	input_mt_init_slots(input_device, (MAX_CONTACTS + 1));
	 
	input_set_abs_params(input_device,ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_device,ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_device,ABS_MT_TOUCH_MAJOR, 0, PRESS_MAX, 0, 0);
	input_set_abs_params(input_device,ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);
       input_set_abs_params(input_device, ABS_MT_TRACKING_ID, 0, 5, 0, 0);
	input_set_abs_params(input_device,ABS_PRESSURE, 0, PRESS_MAX, 0 , 0);
	input_set_abs_params(input_device, ABS_MT_PRESSURE, 0, PRESS_MAX, 0, 0);

#ifdef HAVE_TOUCH_KEY
	input_device->evbit[0] = BIT_MASK(EV_KEY);
	//input_device->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	for (i = 0; i < MAX_KEY_NUM; i++)
		set_bit(key_array[i], input_device->keybit);
#endif
	
	client->irq = IRQ_PORT,
	ts->irq = client->irq;

	ts->wq = create_singlethread_workqueue("kworkqueue_ts");
	if (!ts->wq) {
		dev_err(&client->dev, "Could not create workqueue\n");
		goto error_wq_create;
	}
	flush_workqueue(ts->wq);	

	INIT_WORK(&ts->work, gsl_ts_xy_worker);

	rc = input_register_device(input_device);
	if (rc)
		goto error_unreg_device;

	return 0;

error_unreg_device:
	destroy_workqueue(ts->wq);
error_wq_create:
	input_free_device(input_device);
error_alloc_dev:
	kfree(ts->touch_data);
	return rc;
}

static int gsl_ts_suspend(struct device *dev)
{
	struct gsl_ts *ts = dev_get_drvdata(dev);
	int rc = 0;
        //return 0;
  	printk("I'am in gsl_ts_suspend() start\n");
	
#ifdef GSL_TIMER
	printk( "gsl_ts_suspend () : delete gsl_timer\n");

	del_timer(&ts->gsl_timer);
#endif
	disable_irq_nosync(ts->irq);	
		   
	gslX680_shutdown_low();
	printk("----%s---\n",__func__);

	return 0;
}

static inline unsigned int ms_to_jiffies(unsigned int ms)
{
        unsigned int j;
        j = (ms * HZ + 500) / 1000;
        return (j > 0) ? j : 1;
}

static int gsl_ts_resume(struct device *dev)
{
	struct gsl_ts *ts = dev_get_drvdata(dev);
	int rc = 0;
	
  	printk("I'am in gsl_ts_resume() start\n");
       //  return 0;
	gslX680_shutdown_high();
	mdelay(20); 

	//add 
	//gslX680_shutdown_low();	
	//mdelay(50); 	
	//gslX680_shutdown_high();	
	//mdelay(30); 	

	//queue_delayed_work(gsl_workqueue, &gsl_pen_event_work, ms_to_jiffies(80));

#if 1
	//
	reset_chip(ts->client);
	//printk("--3-\n");
	startup_chip(ts->client);
	//printk("--1-\n");
        check_mem_data(ts->client);
	//printk("--2-\n");
#ifdef GSL_TIMER
	printk( "gsl_ts_resume () : add gsl_timer\n");
	init_timer(&ts->gsl_timer);
	ts->gsl_timer.expires = jiffies + 3 * HZ;
	ts->gsl_timer.function = &gsl_timer_handle;
	ts->gsl_timer.data = (unsigned long)ts;
	add_timer(&ts->gsl_timer);
#endif
       // mdelay(50);
	enable_irq(ts->irq);
      //  printk("----%s---\n",__func__);
   #endif  
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void gsl_ts_early_suspend(struct early_suspend *h)
{
	struct gsl_ts *ts = container_of(h, struct gsl_ts, early_suspend);
	printk("[GSL1680] Enter %s\n", __func__);
	gsl_ts_suspend(&ts->client->dev);
}

static void gsl_ts_late_resume(struct early_suspend *h)
{
	struct gsl_ts *ts = container_of(h, struct gsl_ts, early_suspend);
	printk("[GSL1680] Enter %s\n", __func__);
	gsl_ts_resume(&ts->client->dev);
}
#endif
static int __devinit gsl_ts_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct gsl_ts *ts;
	int rc;
        
	printk("GSLX680 Enter %s\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C functionality not supported\n");
		return -ENODEV;
	}
 
	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;
	printk("==kzalloc success=\n");

	ts->client = client;
	this_client = client;
	i2c_set_clientdata(client, ts);
	ts->device_id = id->driver_data;

	ts->is_suspended = false;
	ts->int_pending = false;
	mutex_init(&ts->sus_lock);
	
	rc = gsl_ts_init_ts(client, ts);
	if (rc < 0) {
		dev_err(&client->dev, "GSLX680 init failed\n");
		goto error_mutex_destroy;
	}	

	gslX680_chip_init();    
	init_chip(ts->client);
	check_mem_data(ts->client);
	
	rc=  request_irq(client->irq, gsl_ts_irq, IRQF_TRIGGER_RISING, client->name, ts);
	if (rc < 0) {
		printk( "gsl_probe: request irq failed\n");
		goto error_req_irq_fail;
	}
      //  mutex_init(&i2c_lock); ////////////
      

      
#ifdef GSL_TIMER
	printk( "gsl_ts_probe () : add gsl_timer\n");

	init_timer(&ts->gsl_timer);
	ts->gsl_timer.expires = jiffies + 3 * HZ;	//��ʱ3  ����
	ts->gsl_timer.function = &gsl_timer_handle;
	ts->gsl_timer.data = (unsigned long)ts;
	add_timer(&ts->gsl_timer);
#endif

	/* create debug attribute */
	//rc = device_create_file(&ts->input->dev, &dev_attr_debug_enable);

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN +20;
	//ts->early_suspend.level =EARLY_SUSPEND_LEVEL_STOP_DRAWING+1;

	//ts->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
	ts->early_suspend.suspend = gsl_ts_early_suspend;
	ts->early_suspend.resume = gsl_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	printk("[GSLX680] End %s\n", __func__);

	return 0;

//exit_set_irq_mode:	
error_req_irq_fail:
    free_irq(ts->irq, ts);	

error_mutex_destroy:
	mutex_destroy(&ts->sus_lock);
	input_free_device(ts->input);
	kfree(ts);
	return rc;
}

static int __devexit gsl_ts_remove(struct i2c_client *client)
{
	struct gsl_ts *ts = i2c_get_clientdata(client);
	printk("==gsl_ts_remove=\n");

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif

	device_init_wakeup(&client->dev, 0);
	cancel_work_sync(&ts->work);
	free_irq(ts->irq, ts);
	destroy_workqueue(ts->wq);
	input_unregister_device(ts->input);
	mutex_destroy(&ts->sus_lock);

	//device_remove_file(&ts->input->dev, &dev_attr_debug_enable);
	
	kfree(ts->touch_data);
	kfree(ts);

	return 0;
}

static const struct i2c_device_id gsl_ts_id[] = {
	{GSLX680_I2C_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, gsl_ts_id);


static struct i2c_driver gsl_ts_driver = {
	.driver = {
		.name = GSLX680_I2C_NAME,
		.owner = THIS_MODULE,
	},
#ifndef CONFIG_HAS_EARLYSUSPEND
	//.suspend	= gsl_ts_suspend,
	//.resume	= gsl_ts_resume,
#endif
	.probe		= gsl_ts_probe,
	.remove		= __devexit_p(gsl_ts_remove),
	.id_table	= gsl_ts_id,
	
};

static int __init gsl_ts_init(void)
{
    int ret;
	
	printk("==gsl_ts_init==\n");
	ret = i2c_add_driver(&gsl_ts_driver);
	printk("ret=%d\n",ret);
	return ret;
}
static void __exit gsl_ts_exit(void)
{
	printk("==gsl_ts_exit==\n");
	i2c_del_driver(&gsl_ts_driver);
	return;
}

late_initcall(gsl_ts_init);
module_exit(gsl_ts_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GSLX680 touchscreen controller driver");
MODULE_AUTHOR("Guan Yuwei, guanyuwei@basewin.com");
MODULE_ALIAS("platform:gsl_ts");
