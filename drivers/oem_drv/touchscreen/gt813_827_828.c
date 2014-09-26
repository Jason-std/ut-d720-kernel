/* drivers/input/touchscreen/gt813_827_828.c
 * 
 * 2010 - 2012 Goodix Technology.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be a reference 
 * to you, when you are integrating the GOODiX's CTP IC into your system, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details.
 * 
 * Version:1.0
 * Author:scott@goodix.com
 * Release Date:2012/05/01
 * Revision record:
 *      V1.0:2012/05/01,create file,by scott
 */

#include <linux/irq.h>
#include "gt813_827_828.h"

static const char *goodix_ts_name = "goodix_touch";
static struct workqueue_struct *goodix_wq;
static struct i2c_client * i2c_connect_client = NULL; 
static u8 config[GTP_CONFIG_LENGTH + GTP_ADDR_LENGTH]
                = {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};

#if GTP_HAVE_TOUCH_KEY
	static const u16 touch_key_array[] = GTP_KEY_TAB;
	#define GTP_MAX_KEY_NUM	 (sizeof(touch_key_array)/sizeof(touch_key_array[0]))
#endif

static int gtp_i2c_test(struct i2c_client *client);	
#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h);
static void goodix_ts_late_resume(struct early_suspend *h);
#endif
 
#ifdef GTP_CREATE_WR_NODE
//extern int init_wr_node(struct i2c_client*);
//extern void uninit_wr_node(void);
#endif

#if GTP_AUTO_UPDATE
extern u8 gup_init_update_proc(struct goodix_ts_data *);
#endif

#if GTP_ESD_PROTECT
static struct delayed_work gtp_esd_check_work;
static struct workqueue_struct * gtp_esd_check_workqueue = NULL;
static void gtp_esd_check_func(struct work_struct *);
#endif


char ut_tp_type[] = "gt828_9";  // TODO
extern int g_touchscreen_init;

/*******************************************************	
Function:
	Read data from the i2c slave device.

Input:
	client:	i2c device.
	buf[0]:operate address.
	buf[1]~buf[len]:read data buffer.
	len:operate length.
	
Output:
	numbers of i2c_msgs to transfer
*********************************************************/
int gtp_i2c_read(struct i2c_client *client, uint8_t *buf, int len)
{
    struct i2c_msg msgs[2];
    int ret=-1;
    int retries = 0;

    GTP_DEBUG_FUNC();

    msgs[0].flags = !I2C_M_RD;
    msgs[0].addr  = client->addr;
    msgs[0].len   = GTP_ADDR_LENGTH;
    msgs[0].buf   = &buf[0];
   // msgs[0].scl_rate = 200 * 1000;

    msgs[1].flags = I2C_M_RD;
    msgs[1].addr  = client->addr;
    msgs[1].len   = len - GTP_ADDR_LENGTH;
    msgs[1].buf   = &buf[GTP_ADDR_LENGTH];
 //   msgs[1].scl_rate = 200 * 1000;

    while(retries<5)
    {
        ret=i2c_transfer(client->adapter, msgs, 2);
        if (ret == 2)break;
        retries++;
    }
  return ret;
}

/*******************************************************	
Function:
	write data to the i2c slave device.

Input:
	client:	i2c device.
	buf[0]:operate address.
	buf[1]~buf[len]:write data buffer.
	len:operate length.
	
Output:
	numbers of i2c_msgs to transfer.
*********************************************************/
int gtp_i2c_write(struct i2c_client *client,uint8_t *data,int len)
{
    struct i2c_msg msg;
    int ret=-1;
    int retries = 0;

    GTP_DEBUG_FUNC();
  
    msg.flags = !I2C_M_RD;
    msg.addr  = client->addr;
    msg.len   = len;
    msg.buf   = data;
    //msg.scl_rate = 200 * 1000;
	
    while(retries<5)
    {
        ret=i2c_transfer(client->adapter,&msg, 1);
        if (ret == 1)break;
        retries++;
    }
    return ret;
}

/*******************************************************	
Function:
	write i2c end cmd.

Input:
	client:	i2c device.
	
Output:
	numbers of i2c_msgs to transfer.
*********************************************************/
int gtp_i2c_end_cmd(struct i2c_client *client)
{
    int ret=-1;
    u8 end_cmd_data[2]={0x80, 0x00}; 
    
    GTP_DEBUG_FUNC();

    ret = gtp_i2c_write(client, end_cmd_data, 2);

    return ret;
}

/*******************************************************
Function:
	Send config Function.

Input:
	client:	i2c client.
	
Output:
	Executive outcomes.0！！success,non-0！！fail.
*******************************************************/
int gtp_send_cfg(struct i2c_client *client)
{
#if GTP_DRIVER_SEND_CFG
    int retry = 0;
    int ret = -1;
    
    for (retry = 0; retry < 5; retry++)
    {
        ret = gtp_i2c_write(client, config , GTP_CONFIG_LENGTH + GTP_ADDR_LENGTH);        
        gtp_i2c_end_cmd(client);

        if (ret > 0)
        {
            return 0;
        }
    }

    return retry;
#else
    return 0;
#endif
}

/*******************************************************
Function:
	Enable IRQ Function.

Input:
	ts:	i2c client private struct.
	
Output:
	None.
*******************************************************/
void gtp_irq_disable(struct goodix_ts_data *ts)
{	
    unsigned long irqflags;
	
    GTP_DEBUG_FUNC();
	
    spin_lock_irqsave(&ts->irq_lock, irqflags);
    if (!ts->irq_is_disable)
    {
        ts->irq_is_disable = 1; 
        disable_irq_nosync(ts->client->irq);
    }
    spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}

/*******************************************************
Function:
	Disable IRQ Function.

Input:
	ts:	i2c client private struct.
	
Output:
	None.
*******************************************************/
void gtp_irq_enable(struct goodix_ts_data *ts)
{	
    unsigned long irqflags;
	
    GTP_DEBUG_FUNC();
		
    spin_lock_irqsave(&ts->irq_lock, irqflags);
    if (ts->irq_is_disable) 
    {		
        enable_irq(ts->client->irq);		
        ts->irq_is_disable = 0;	
    }	
    spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}

/*******************************************************
Function:
	Touch down report function.

Input:
	ts:private data.
	id:tracking id.
	x:input x.
	y:input y.
	w:input weight.
	
Output:
	None.
*******************************************************/
static void gtp_touch_down(struct goodix_ts_data* ts,int id,int x,int y,int w)
{
#if GTP_CHANGE_X2Y
    GTP_SWAP(x,y);
#endif

    input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
    input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
    input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, w);
    input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, w);
    input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
    input_mt_sync(ts->input_dev);

    GTP_DEBUG("ID:%d, X:%d, Y:%d, W:%d", id, x, y, w);
	//printk("ID:%d, X:%d, Y:%d, W:%d\n", id, x, y, w);
}

/*******************************************************
Function:
	Touch up report function.

Input:
	ts:private data.
	
Output:
	None.
*******************************************************/
static void gtp_touch_up(struct goodix_ts_data* ts,int i)
{
	input_mt_slot(ts->input_dev, i);
	input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);

    input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
    input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
    input_mt_sync(ts->input_dev);
	//printk("%s\n",__func__);
}

/*******************************************************
Function:
	Goodix touchscreen work function.

Input:
	ts:	i2c client private struct.
	
Output:
	None.
*******************************************************/
static void goodix_ts_work_func(struct work_struct *work)
{
    u8* coor_data = NULL;
    u8  point_data[2 + 2 + 5 * GTP_MAX_TOUCH + 1]={GTP_READ_COOR_ADDR >> 8, GTP_READ_COOR_ADDR & 0xFF};
    u8  check_sum = 0;
    u8  touch_num = 0;
    u8  finger = 0;
    u8  key_value = 0;
    //int input_x = 0;
    //int input_y = 0;	
    //int input_w = 0;
    int idx = 0;
    int  i = 0;
    int ret = -1;
   struct tp_event  event ;
   struct tp_event  current_events[GTP_MAX_TOUCH];
   static u8 points_last_flag[GTP_MAX_TOUCH]={0};
  
    struct goodix_ts_data *ts = NULL;

    GTP_DEBUG_FUNC();
    
    ts = container_of(work, struct goodix_ts_data, work);
    if (ts->enter_update)
    {
        return;
    }

    ret = gtp_i2c_read(ts->client, point_data, 10); 
    finger = point_data[2];
    touch_num = (finger & 0x01) + !!(finger & 0x02) + !!(finger & 0x04) + !!(finger & 0x08) + !!(finger & 0x10);
    if (touch_num > 1)
    {
        u8 buf[25];
        buf[0] = GTP_READ_COOR_ADDR >> 8;
        buf[1] = (GTP_READ_COOR_ADDR & 0xff) + 8;
        ret = gtp_i2c_read(ts->client, buf, 5 * (touch_num - 1) + 2); 
        memcpy(&point_data[10], &buf[2], 5 * (touch_num - 1));
    }
    gtp_i2c_end_cmd(ts->client);

    if (ret <= 0)
    {
        GTP_INFO("I2C read error!");
        goto exit_work_func;
    }

    if((finger & 0xC0) != 0x80)
    {
        GTP_INFO("Data not ready!");
        goto exit_work_func;
    }

    key_value = point_data[3]&0x0f; // 1, 2, 4, 8
    if ((key_value & 0x0f) == 0x0f)
    {
        if (gtp_send_cfg(ts->client))
        {
            GTP_DEBUG("Reload config failed!\n");
        }
        goto exit_work_func;
    }

    coor_data = &point_data[4];
    check_sum = 0;
    for ( idx = 0; idx < 5 * touch_num; idx++)
    {
        check_sum += coor_data[idx];
    }
    if (check_sum != coor_data[5 * touch_num])
    {
        GTP_INFO("Check sum error!");
        goto exit_work_func;
    }
#if 1
    if(touch_num == 0)
    	{

    		for(i=0;i<GTP_MAX_TOUCH;i++)
		{
			if(points_last_flag[i]!=0)
			{
				//FTprintk("Point UP event.id=%d\n",i);
				input_mt_slot(ts->input_dev, i);
				input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);					
			}
		}
		memset(points_last_flag, 0, sizeof(points_last_flag));
		//input_mt_sync(data->input_dev);

		input_report_abs(ts->input_dev, ABS_PRESSURE, 0);
		input_report_key(ts->input_dev, BTN_TOUCH, 0);

		input_sync(ts->input_dev);
		 gtp_irq_enable(ts);
		return; 
    	}
#endif 
    if (touch_num)
    {
        int pos = 0;
        
        for (idx = 0; idx < GTP_MAX_TOUCH; idx++)
        {
            if (!(finger & (0x01 << idx)))
            {
                continue;
            }
 #if 1

            event.x  = coor_data[pos] << 8;
            event.x |= coor_data[pos + 1];
            
            event.y= coor_data[pos + 2] << 8;
            event.y |= coor_data[pos + 3];
            event.w   = coor_data[pos + 4];
            event.id  = idx;
            pos += 5;
            memcpy(&current_events[event.id], &event, sizeof(event));
 #endif
			
	#if 0	
            input_x  = coor_data[pos] << 8;
            input_x |= coor_data[pos + 1];

            input_y  = coor_data[pos + 2] << 8;
            input_y |= coor_data[pos + 3];

            input_w  = coor_data[pos + 4];

            pos += 5;

            gtp_touch_down(ts, idx, input_x, input_y, input_w);
	#endif	   
        }
	#if 1	
	for(i = 0;i < touch_num;i++)
		{
			#if GTP_CHANGE_X2Y
  			  GTP_SWAP(current_events[i].x,current_events[i].y);
			#endif
			//printk("x= %d--y =%d-------\n", current_events[event.id].x, current_events[event.id].y);	
			  input_mt_slot(ts->input_dev, i);
                          input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);
  			  input_report_abs(ts->input_dev, ABS_MT_POSITION_X, current_events[i].x);
   			  input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, current_events[i].y);
     			  input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, current_events[i].w);
   			  input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, current_events[i].w);
 			  input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, current_events[i].id);
   			  input_mt_sync(ts->input_dev);
			  points_last_flag[i] = i;
		}
   #endif
    }
#if 0
    else
    {
        GTP_DEBUG("Touch Release!");
               gtp_touch_up(ts,i);
    }
#endif
#if GTP_HAVE_TOUCH_KEY
    for (idx= 0; idx < GTP_MAX_KEY_NUM; idx++)
    {
        input_report_key(ts->input_dev, touch_key_array[idx], key_value & (0x01<<idx));   
    }
#endif
    input_report_key(ts->input_dev, BTN_TOUCH, (touch_num || key_value));
    input_sync(ts->input_dev);

exit_work_func:
    if (ts->use_irq)
    {
        gtp_irq_enable(ts);
    }
}

/*******************************************************
Function:
	Timer interrupt service routine.

Input:
	timer:	timer struct pointer.
	
Output:
	Timer work mode. HRTIMER_NORESTART---not restart mode
*******************************************************/
static enum hrtimer_restart goodix_ts_timer_handler(struct hrtimer *timer)
{
    struct goodix_ts_data *ts = container_of(timer, struct goodix_ts_data, timer);
	
    GTP_DEBUG_FUNC();
	
    queue_work(goodix_wq, &ts->work);
    hrtimer_start(&ts->timer, ktime_set(0, (GTP_POLL_TIME+6)*1000000), HRTIMER_MODE_REL);
    return HRTIMER_NORESTART;
}

/*******************************************************
Function:
	External interrupt service routine.

Input:
	irq:	interrupt number.
	dev_id: private data pointer.
	
Output:
	irq execute status.
*******************************************************/
static irqreturn_t goodix_ts_irq_handler(int irq, void *dev_id)
{
    struct goodix_ts_data *ts = dev_id;

	//printk("--%s\n",__func__);

    GTP_DEBUG_FUNC();
  
    gtp_irq_disable(ts);
    queue_work(goodix_wq, &ts->work);
	
    return IRQ_HANDLED;
}

/*******************************************************
Function:
	Reset chip Function.

Input:
	ms:reset time.
	
Output:
	None.
*******************************************************/
void gtp_reset_guitar(int ms)
{
    GTP_DEBUG_FUNC();
    GTP_GPIO_OUTPUT(GTP_RST_PORT, 1); // 0 
    msleep(ms);

    GTP_GPIO_AS_INPUT(GTP_RST_PORT);
    msleep(20);

    return;
}

/*******************************************************
Function:
	Eter sleep function.

Input:
	ts:private data.
	
Output:
	Executive outcomes.0！！success,non-0！！fail.
*******************************************************/
static int gtp_enter_sleep(struct goodix_ts_data * ts)
{
    int ret = -1;
    int retry = 0;
    u8 i2c_control_buf[3] = {GTP_REG_SLEEP >> 8, GTP_REG_SLEEP & 0xff, 0xc0};
	
    GTP_DEBUG_FUNC();
	
    while(retry++ < 5)
    {
        ret = gtp_i2c_write(ts->client, i2c_control_buf, 2);
        if (ret == 1)
        {
            GTP_DEBUG("GTP enter sleep!");
            return 0;
        }
        msleep(10);
    }
    GTP_ERROR("GTP send sleep cmd failed.");
    return retry;
}

/*******************************************************
Function:
	Wakeup from sleep mode Function.

Input:
	ts:	private data.
	
Output:
	Executive outcomes.0！！success,non-0！！fail.
*******************************************************/
static int gtp_wakeup_sleep(struct goodix_ts_data * ts)
{
    u8 retry = 0;
    int ret = -1;
     int irq = 0;
    while(retry++ < 5)
    {
#if GTP_POWER_CTRL_SLEEP
        gtp_reset_guitar(10);
        ret = gtp_send_cfg(ts->client);
#else
GTP_GPIO_OUTPUT(GTP_INT_PORT, 0);
        msleep(5);
        GTP_GPIO_OUTPUT(GTP_INT_PORT, 1);
        msleep(5);
        //GTP_GPIO_AS_INT(GTP_INT_PORT);
        GTP_GPIO_AS_INT(GTP_INT_PORT, irq);	
        ts->client->irq = irq;
        ret = gtp_i2c_test(ts->client);

#endif
        if (!ret)
        {
            GTP_DEBUG("GTP wakeup sleep.");
            return 0;
        }
        msleep(10);
    }
    GTP_ERROR("GTP wakeup sleep failed.");
    return retry;
}

/*******************************************************
Function:
	GTP initialize function.

Input:
	ts:	i2c client private struct.
	
Output:
	Executive outcomes.0---succeed.
*******************************************************/
//extern char ut_tp_type[];
static int gtp_init_panel(struct goodix_ts_data *ts)
{
    int ret = -1;
  //  u8 *cfg_info_group1 = NULL;
  
    u8 cfg_info_group_1648[] = CTP_CFG_GROUP1_1648;
    u8 cfg_info_group_1697[] = CTP_CFG_GROUP1_1697;

      u8 *cfg_info_group1 = cfg_info_group_1648;
	
#if GTP_DRIVER_SEND_CFG
    u8 rd_cfg_buf[16];

#if 0  // for debug
    if (!strncmp("gt828_10", ut_tp_type, strlen(ut_tp_type)))
    {
        cfg_info_group1 = cfg_info_group_1648;
      printk("[touch 828] type CTP_CFG_GROUP1_1648\n");
   }else{
        cfg_info_group1 = cfg_info_group_1697;
        printk("[touch 828] type CTP_CFG_GROUP1_1679\n");
    }
#endif

   u8 cfg_info_group2[] = CTP_CFG_GROUP2;
    u8 cfg_info_group3[]= CTP_CFG_GROUP3;


    u8 *send_cfg_buf[3] = {cfg_info_group1, cfg_info_group2, cfg_info_group3};
    u8 cfg_info_len[3] = {sizeof(cfg_info_group1)/sizeof(cfg_info_group1[0]), 
                          sizeof(cfg_info_group2)/sizeof(cfg_info_group2[0]),
                          sizeof(cfg_info_group3)/sizeof(cfg_info_group3[0])};
    printk("len1=%d,len2=%d,len3=%d\n",cfg_info_len[0],cfg_info_len[1],cfg_info_len[2]);
    if ((!cfg_info_len[1])&&(!cfg_info_len[2]))
    {
        rd_cfg_buf[2] = 0; 
    }
    else
    {
        rd_cfg_buf[0] = GTP_REG_SENSOR_ID >> 8;
        rd_cfg_buf[1] = GTP_REG_SENSOR_ID & 0xff;
        ret=gtp_i2c_read(ts->client, rd_cfg_buf, 2);
        gtp_i2c_end_cmd(ts->client);
        if (ret <= 0)
        {
            GTP_ERROR("Read SENSOR ID failed,default use group1 config!");
            rd_cfg_buf[2] = 0;
        }
        rd_cfg_buf[2] &= 0x07;
    }

    printk("SENSOR ID:%d\n", rd_cfg_buf[2]);
    memcpy(&config[2], send_cfg_buf[rd_cfg_buf[2]], GTP_CONFIG_LENGTH);

printk("111\n");

#if GTP_CUSTOM_CFG
    config[RESOLUTION_LOC]     = (u8)(GTP_MAX_WIDTH>>8);
    config[RESOLUTION_LOC + 1] = (u8)GTP_MAX_WIDTH;
    config[RESOLUTION_LOC + 2] = (u8)(GTP_MAX_HEIGHT>>8);
    config[RESOLUTION_LOC + 3] = (u8)GTP_MAX_HEIGHT;
#endif  //endif GTP_CUSTOM_CFG
    
    if (GTP_INT_TRIGGER == 0)  //FALLING
    {
        config[TRIGGER_LOC] &= 0xf7; 
    }
    else if (GTP_INT_TRIGGER == 1)  //RISING
    {
        config[TRIGGER_LOC] |= 0x08;
    }

#else //else DRIVER NEED NOT SEND CONFIG

    ret=gtp_i2c_read(ts->client, config, GTP_CONFIG_LENGTH + GTP_ADDR_LENGTH);
    gtp_i2c_end_cmd(ts->client);
    if (ret <= 0)
    {
        GTP_ERROR("GTP read resolution & max_touch_num failed, use default value!");
        ts->abs_x_max = GTP_MAX_WIDTH;
        ts->abs_y_max = GTP_MAX_HEIGHT;
        ts->int_trigger_type = GTP_INT_TRIGGER;
    }
#endif //endif GTP_DRIVER_SEND_CFG

printk("222\n");

    GTP_DEBUG_FUNC();

    ts->abs_x_max = (config[RESOLUTION_LOC] << 8) + config[RESOLUTION_LOC + 1];
    ts->abs_y_max = (config[RESOLUTION_LOC + 2] << 8) + config[RESOLUTION_LOC + 3];
    ts->int_trigger_type = (config[TRIGGER_LOC] >> 3) & 0x01;
    if ((!ts->abs_x_max)||(!ts->abs_y_max))
    {
        GTP_ERROR("GTP resolution & max_touch_num invalid, use default value!");
        ts->abs_x_max = GTP_MAX_WIDTH;
        ts->abs_y_max = GTP_MAX_HEIGHT;
    }

    ret = gtp_send_cfg(ts->client);
    if (ret)
    {
        GTP_ERROR("Send config error.");
    }
    
    GTP_DEBUG("X_MAX = %d,Y_MAX = %d,TRIGGER = 0x%02x",ts->abs_x_max,ts->abs_y_max,ts->int_trigger_type);

    msleep(10);

    return 0;
}

/*******************************************************
Function:
	Read goodix touchscreen version function.

Input:
	client:	i2c client struct.
	version:address to store version info
	
Output:
	Executive outcomes.0---succeed.
*******************************************************/
int  gtp_read_version(struct i2c_client *client, u8 *version)
{
    int ret = -1;
    u8 buf[8];
	
    GTP_DEBUG_FUNC();
	
    buf[0]=GTP_REG_VERSION >> 8;
    buf[1]=GTP_REG_VERSION & 0xff;
    
    ret=gtp_i2c_read(client,buf, 5);
    gtp_i2c_end_cmd(client);
    if (ret <= 0)
    {
        return ret;
    }

    sprintf(version, "%02x_%02x%02x", buf[2], buf[3], buf[4]);

    return 0;
}

/*******************************************************
Function:
	I2c test Function.

Input:
	client:i2c client.
	
Output:
	Executive outcomes.0！！success,non-0！！fail.
*******************************************************/
int gtp_i2c_test(struct i2c_client *client)
{
    u8 retry = 0;
    int ret = -1;
  
    GTP_DEBUG_FUNC();
  
    while(retry++ < 5)
    {
        ret = gtp_i2c_end_cmd(client);
        if (ret > 0)
        {
            return 0;
        }
        GTP_ERROR("GTP i2c test failed time %d.",retry);
        msleep(10);
    }
    return retry;
}

/*******************************************************
Function:
	Request gpio Function.

Input:
	ts:private data.
	
Output:
	Executive outcomes.0！！success,non-0！！fail.
*******************************************************/
int gtp_request_io_port(struct goodix_ts_data *ts)
{
    int ret = 0;
    int irq = 0;

    ret = GTP_GPIO_REQUEST(GTP_INT_PORT, "GTP_INT_IRQ");
    if (ret < 0) 
    {
        GTP_ERROR("--Failed to request GPIO:%d, ERRNO:%d", (int)GTP_INT_PORT, ret);
  //      ret = -ENODEV;
    }
//    else
    {
        GTP_GPIO_AS_INT(GTP_INT_PORT, irq);	
        ts->client->irq = irq;
    }

    ret = GTP_GPIO_REQUEST(GTP_RST_PORT, "GTP_RST_PORT");
    if (ret < 0) 
    {
        GTP_ERROR("++Failed to request GPIO:%d, ERRNO:%d",(int)GTP_RST_PORT,ret);
        ret = -ENODEV;
    }

    GTP_GPIO_AS_INPUT(GTP_RST_PORT);
    gtp_reset_guitar(20);

	#if 0  // masked andydeng 20121012
    if(ret < 0)
    {
        GTP_GPIO_FREE(GTP_RST_PORT);
        GTP_GPIO_FREE(GTP_INT_PORT);
    }
	#endif
    return ret;
}

/*******************************************************
Function:
	Request irq Function.

Input:
	ts:private data.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
int gtp_request_irq(struct goodix_ts_data *ts)
{
    int ret = -1;
    const u8 irq_table[2] = GTP_IRQ_TAB;

    GTP_DEBUG("INT trigger type:%x\n", ts->int_trigger_type);

    ret  = request_irq(ts->client->irq, 
                       goodix_ts_irq_handler,
                       irq_table[ts->int_trigger_type],
                       ts->client->name,
                       ts);
    if (ret)
    {
        GTP_ERROR("Request IRQ failed!ERRNO:%d.", ret);
        GTP_GPIO_AS_INPUT(GTP_INT_PORT);
        GTP_GPIO_FREE(GTP_INT_PORT);

        hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        ts->timer.function = goodix_ts_timer_handler;
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);

        return 1;
    }
    else 
    {	
        gtp_irq_disable(ts);
        ts->use_irq = 1;
        return 0;
    }
}

/*******************************************************
Function:
	Request input device Function.

Input:
	ts:private data.
	
Output:
	Executive outcomes.0！！success,non-0！！fail.
*******************************************************/
int gtp_request_input_dev(struct goodix_ts_data *ts)
{
    int ret = -1;
    char phys[32];
#if GTP_HAVE_TOUCH_KEY
    u8 index = 0;
#endif
  
    GTP_DEBUG_FUNC();
  
    ts->input_dev = input_allocate_device();
    if (ts->input_dev == NULL)
    {
        GTP_ERROR("Failed to allocate input device.");
        return -ENOMEM;
    }

    ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
    ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
    ts->input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);

#if GTP_HAVE_TOUCH_KEY
    for (index = 0; index < GTP_MAX_KEY_NUM; index++)
    {
        input_set_capability(ts->input_dev,EV_KEY,touch_key_array[index]);	
    }
#endif

#if GTP_CHANGE_X2Y
    GTP_SWAP(ts->abs_x_max, ts->abs_y_max);
#endif

    input_set_abs_params(ts->input_dev, ABS_X, 0, ts->abs_x_max, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_Y, 0, ts->abs_y_max, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, ts->abs_x_max, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, ts->abs_y_max, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);	
    input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, 255, 0, 0);

    sprintf(phys, "input/ts");
    ts->input_dev->name = goodix_ts_name;
    ts->input_dev->phys = phys;
    ts->input_dev->id.bustype = BUS_I2C;
    ts->input_dev->id.vendor = 0xDEAD;
    ts->input_dev->id.product = 0xBEEF;
    ts->input_dev->id.version = 10427;
	
    ret = input_register_device(ts->input_dev);
    if (ret)
    {
        GTP_ERROR("Register %s input device failed", ts->input_dev->name);
        return -ENODEV;
    }
    
#ifdef CONFIG_HAS_EARLYSUSPEND
    ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    ts->early_suspend.suspend = goodix_ts_early_suspend;
    ts->early_suspend.resume = goodix_ts_late_resume;
    register_early_suspend(&ts->early_suspend);
#endif

    return 0;
}

/*******************************************************
Function:
	Goodix touchscreen probe function.

Input:
	client:	i2c device struct.
	id:device id.
	
Output:
	Executive outcomes. 0---succeed.
*******************************************************/
int goodix_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = -1;
    struct goodix_ts_data *ts;
    u8 version_info[16];

	 //   extern char g_selected_utmodel[];
   
    GTP_DEBUG_FUNC();
    printk("<<-GTP-FUNC->> Func:%s@Line:%d\n",__func__,__LINE__);

	if(g_touchscreen_init) {
		printk("skip\n");
		return 0;
	}
	  goodix_wq = create_singlethread_workqueue("goodix_wq");
        if (!goodix_wq)
        {
            GTP_ERROR("Creat workqueue failed.");
            return -ENOMEM;
        }

    printk("GTP I2C addr:%x", client->addr);
    i2c_connect_client = client;

	//if(client->addr == 0x55)
		//client->addr = 0x5D;
	printk("----<%s>,client->addr:0x%x\n",__func__,client->addr);
	
    //do NOT remove these output log
    printk("GTP Driver Version:%s\n",GTP_DRIVER_VERSION);
    printk("GTP Driver build@%s,%s\n", __TIME__,__DATE__);

#if 0

    if (!strncmp(g_selected_utmodel, "s101", strlen("s101")) )
    {
       GTP_MAX_HEIGHT  = 1280;    //   10'
        GTP_MAX_WIDTH  = 800;     // 
    }//else is 9', not change
#endif

	printk("X:%d,Y:%d\n",GTP_MAX_HEIGHT,GTP_MAX_WIDTH);


    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
    {
        printk("GTP I2C check functionality failed.");
        return -ENODEV;
    }
    ts = kzalloc(sizeof(*ts), GFP_KERNEL);
    if (ts == NULL)
    {
        printk("GTP Alloc GFP_KERNEL memory failed.");
        return -ENOMEM;
    }
    
    memset(ts, 0, sizeof(*ts));
    INIT_WORK(&ts->work, goodix_ts_work_func);
    ts->client = client;
    i2c_set_clientdata(client, ts);

    ret = gtp_request_io_port(ts);

#if 0 // masked andydeng 20121012	
    if (ret)
    {
        printk("GTP request IO port failed");
        kfree(ts);
        return ret;
    }
#endif
#if 0
	ret = gpio_request(GTP_RST_PORT, "GTP_RST_PORT");
	if(ret<0) {
		dev_err(&client->dev, "Error: GTP_RST_PORT request failed !\n");
		//goto exit_gpio_request_failed;
	}	
	 gtp_reset_guitar(20);
#endif

    ret = gtp_i2c_test(client);
    if (ret)
    {
        printk("GTP I2C communication ERROR!");
	goto I2C_FAILED;
    }

#if GTP_AUTO_UPDATE
    ret = gup_init_update_proc(ts);
    if (ret)
    {
        printk("GTP Create update thread error.");
    }
#endif

    ret = gtp_init_panel(ts);
    if (ret)
    {
        printk("GTP init panel failed.");
    }

    ret = gtp_request_input_dev(ts);
    if (ret)
    {
        printk("GTP request input dev failed");
    }
    
    ret = gtp_request_irq(ts); 
    if (ret)
    {
        printk("GTP works in polling mode.");
    }
    else
    {
        printk("GTP works in interrupt mode.");
    }

    ret = gtp_read_version(client, version_info);
    if (ret)
    {
        printk("GTP Read version failed.");
    }
    else
    {
       printk("GTP Chip Version:%s", version_info);
    }
    gtp_irq_enable(ts);

#ifdef GTP_CREATE_WR_NODE
    //init_wr_node(client);
#endif

#if GTP_ESD_PROTECT
    //gtp_esd_check_workqueue = create_workqueue("gtp_esd_check"); // masked andydeng 
    gtp_esd_check_workqueue = create_workqueue(dev_name(&client->dev)); 
    INIT_DELAYED_WORK(&gtp_esd_check_work, gtp_esd_check_func);
    ret = queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE); 
    if (ret)
    {
        printk("GTP Create ESD check thread failed!");
    }
#endif

	g_touchscreen_init = 1;

	    return 0;

I2C_FAILED: 
	kfree(ts);
	GTP_GPIO_FREE(GTP_RST_PORT);
        GTP_GPIO_FREE(GTP_INT_PORT);
//	cancel_work_sync(&ts->work);
//	kfree(ts);
	destroy_workqueue(goodix_wq); 
		
    return 0;
}


/*******************************************************
Function:
	Goodix touchscreen driver release function.

Input:
	client:	i2c device struct.
	
Output:
	Executive outcomes. 0---succeed.
*******************************************************/
int goodix_ts_remove(struct i2c_client *client)
{
    struct goodix_ts_data *ts = i2c_get_clientdata(client);
	
    GTP_DEBUG_FUNC();

	g_touchscreen_init = 0; 
	
#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&ts->early_suspend);
#endif

#ifdef GTP_CREATE_WR_NODE
    //uninit_wr_node();
#endif

#if GTP_ESD_PROTECT
    destroy_workqueue(gtp_esd_check_workqueue);
#endif

    if (ts) 
    {
        if (ts->use_irq)
        {
            GTP_GPIO_AS_INPUT(GTP_INT_PORT);
            GTP_GPIO_FREE(GTP_INT_PORT);
            free_irq(client->irq, ts);
        }
        else
        {
            hrtimer_cancel(&ts->timer);
        }
    }	
	
    GTP_INFO("GTP driver is removing...");
    i2c_set_clientdata(client, NULL);
    input_unregister_device(ts->input_dev);
    kfree(ts);

    return 0;
}


/*******************************************************
Function:
	Early suspend function.

Input:
	h:early_suspend struct.
	
Output:
	None.
*******************************************************/
#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h)
{
    struct goodix_ts_data *ts;
    int ret = -1;	
    ts = container_of(h, struct goodix_ts_data, early_suspend);
	
    GTP_DEBUG_FUNC();

#if GTP_ESD_PROTECT
    ts->gtp_is_suspend = 1;
    cancel_delayed_work_sync(&gtp_esd_check_work);
#endif

    if (ts->use_irq)
    {
        gtp_irq_disable(ts);
    }
    else
    {
        hrtimer_cancel(&ts->timer);
    }
    ret = gtp_enter_sleep(ts);
    if (ret)
    {
        GTP_ERROR("GTP early suspend failed.");
    }
	printk("--%s end--\n",__func__);
}

/*******************************************************
Function:
	Late resume function.

Input:
	h:early_suspend struct.
	
Output:
	None.
*******************************************************/
static void goodix_ts_late_resume(struct early_suspend *h)
{
	
    struct goodix_ts_data *ts;
    int ret = -1;
    ts = container_of(h, struct goodix_ts_data, early_suspend);
	printk("%s,irq:%d\n",__func__, ts->client->irq );
	
    GTP_DEBUG_FUNC();
	
    ret = gtp_wakeup_sleep(ts);
    if (ret)
    {
        GTP_ERROR("GTP later resume failed.");
    }

    if (ts->use_irq)
    {
        gtp_irq_enable(ts);
    }
    else
    {
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
    }

#if GTP_ESD_PROTECT
    ts->gtp_is_suspend = 0;
    queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE);
#endif

	printk("--%s end--\n",__func__);
}
#endif

#if GTP_ESD_PROTECT
static void gtp_esd_check_func(struct work_struct *work)
{
    int i = 0;
    int ret = -1;
    struct goodix_ts_data *ts = NULL;
//    u8 buf[2] = {GTP_REG_CONFIG_DATA >> 8 & 0xff, GTP_REG_CONFIG_DATA & 0xff};

    GTP_DEBUG_FUNC();
    
    ts = i2c_get_clientdata(i2c_connect_client);

    for (i = 0; ts->irq_is_disable; i++)
    {
        msleep(1);

        if (i >= 5)
        {
            return;
        }
    }

    for (i = 0; i < 3; i++)
    {
        ret = gtp_i2c_end_cmd(i2c_connect_client);
	    if (ret >= 0)
	    {
	        break;
	    }
	}
	
    if (i >= 3)
    {
        gtp_reset_guitar(50);
    }

    if(!ts->gtp_is_suspend)
    {
        queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE);
    }

    return;
}
#endif


static const struct i2c_device_id goodix_ts_id[] = {
    { GTP_I2C_NAME, 0 },
    { }
};

struct i2c_driver goodix_ts_driver = {
    .probe      = goodix_ts_probe,
    .remove     = goodix_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
    .suspend    = goodix_ts_early_suspend,
    .resume     = goodix_ts_later_resume,
#endif
    .id_table   = goodix_ts_id,
    .driver = {
        .name     = GTP_I2C_NAME,
        .owner    = THIS_MODULE,
    },
};

/*******************************************************	
Function:
	Driver Install function.
Input:
  None.
Output:
	Executive Outcomes. 0---succeed.
********************************************************/
static int __devinit goodix_ts_init(void)
{
    int ret = 0;
	extern char g_selected_tp[32];

    GTP_DEBUG_FUNC();	
//    if ((!strncmp("gt828_10", ut_tp_type, strlen(ut_tp_type))) || (!strncmp("gt828_9", ut_tp_type, strlen(ut_tp_type))))

//if(!strncmp(g_selected_tp,"gt101",strlen("gt101")))
    {
        GTP_INFO("GTP driver install.");
	#if 0
        goodix_wq = create_singlethread_workqueue("goodix_wq");
        if (!goodix_wq)
        {
            GTP_ERROR("Creat workqueue failed.");
            return -ENOMEM;
        }
	#endif
        ret = i2c_add_driver(&goodix_ts_driver);
    }
    
    return ret; 
}

/*******************************************************	
Function:
	Driver uninstall function.
Input:
  None.
Output:
	Executive Outcomes. 0---succeed.
********************************************************/
static void __exit goodix_ts_exit(void)
{
    GTP_DEBUG_FUNC();
    GTP_INFO("GTP driver exited.");
    i2c_del_driver(&goodix_ts_driver);
    if (goodix_wq)
    {
        destroy_workqueue(goodix_wq);
    }
}

late_initcall(goodix_ts_init);
module_exit(goodix_ts_exit);

//MODULE_AUTHOR("Gabor Juhos <juhosg@openwrt.org>");
//MODULE_AUTHOR("Miguel Gaio <miguel.gaio@efixo.com>");
MODULE_DESCRIPTION("GTP Series Driver");
MODULE_LICENSE("GPL");