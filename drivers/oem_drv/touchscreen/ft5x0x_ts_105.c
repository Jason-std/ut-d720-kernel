/* 
 * drivers/input/touchscreen/ft5x0x_ts.c
 *
 * FocalTech ft5x0x TouchScreen driver. 
 *
 * Copyright (c) 2010  Focal tech Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 *	note: only support mulititouch	Wenfs 2010-10-01
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

#include <mach/hardware.h>
#include <plat/gpio-cfg.h>
#include <plat/irqs.h>

#include "ft5x0x_ts.h"


static u8 buf[40] = {0};

#define FT_INT_PORT		GPIO_CTP_INT
//#define FT_WAKE_PORT 	GPIO_TS_WAKE  // orig

#define FT_WAKE_PORT 	      EXYNOS4212_GPM3(4)
//#define FT_POWER_PORT	GPIO_TS_POWER

#define EINT_NUM	IRQ_CTP_INT



#define set_irq_type   irq_set_irq_type		//raymanfeng for kernel 3.0 8
static int touch_key_down;
static int touch_key_count;
static struct ft5x0x_ts_data *s_ft5x0x_ts = NULL;

static struct i2c_client *this_client;
//static struct ft5x0x_ts_platform_data *pdata;

#define CONFIG_FT5X0X_MULTITOUCH 1
#define CONFIG_ANDROID_4_ICS 1                //raymanfeng

//extern int MmFlag;
static  int MmFlag = 1;
//extern spinlock_t lock ;

//extern int g_touchscreen_init;
static  int g_touchscreen_init;

struct ts_event {
	u16	x1;
	u16	y1;
	u16	x2;
	u16	y2;
        u16     x3;
        u16     y3;
        u16     x4;
        u16     y4;
        u16     x5;
        u16     y5;
	u16	pressure;
    u8  touch_point;
};

struct ft5x0x_ts_data {
	struct input_dev	*input_dev;
	struct ts_event		event;
	struct work_struct 	pen_event_work;
	struct workqueue_struct *ts_workqueue;
	struct early_suspend	early_suspend;
};


static int s_model = 0;

//-----
#define TOUCH_TIMER     msecs_to_jiffies(50)
static struct timer_list timer;
static int flag_tp=0;
static int g_x,g_y;
static int first_time = 1;
//------
//extern char g_selected_utmodel[];
static char g_selected_utmodel[] = {'\0'};
static void check_model(void)
{
#if 0
	int value = 703;
	s_model = 712;

	if(sscanf(g_selected_utmodel, "%d", &value) == 1)
	{
		if(value > 0) 
			s_model = value;
		printk("select model %d\n", s_model);
	} 
#endif
}

static int ft5x0x_i2c_rxdata(char *rxdata, int length)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= rxdata,
		},
		{
			.addr	= this_client->addr,
			.flags	= I2C_M_RD,
			.len	= length,
			.buf	= rxdata,
		},
	};

    //msleep(1);
	ret = i2c_transfer(this_client->adapter, msgs, 2);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);
	
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

static unsigned char cmd_write(unsigned char btcmd,unsigned char btPara1,unsigned char btPara2,unsigned char btPara3,unsigned char num)
{
    unsigned char write_cmd[4] = {0};

    write_cmd[0] = btcmd;
    write_cmd[1] = btPara1;
    write_cmd[2] = btPara2;
    write_cmd[3] = btPara3;
    return i2c_master_send(this_client, write_cmd, num);
}

static void ft5x0x_ts_release(void)
{
	struct ft5x0x_ts_data *data = i2c_get_clientdata(this_client);
#ifdef CONFIG_FT5X0X_MULTITOUCH	

	#if 0
	if (touch_key_count > 0)
		{
			switch (touch_key_down)
				{
					case 1:
						input_report_key(data->input_dev, KEY_MENU, 0);
					//	printk("touch key back  up!!\n");
						break;
					case 2:
						input_report_key(data->input_dev, KEY_HOMEPAGE, 0);
					//	printk("touch key menu  up!!\n");
						break;
                                    
					case 3:
                                               if (touch_key_count > 2)
                                               {
					        	input_report_key(data->input_dev, KEY_BACK, 0);
					  //      	printk("touch key home  up!!\n");
					        	break;
                                               }
					case 0:				
						break;	
				}
		}
	#endif
	input_mt_sync(data->input_dev);
	input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
#ifdef CONFIG_ANDROID_4_ICS
	input_report_abs(data->input_dev, ABS_PRESSURE, 0);
	input_report_key(data->input_dev, BTN_TOUCH, 0);
#endif
	//touch_key_down = 0;
	//touch_key_count = 0;
        flag_tp=0;
	//first_time = 1;
#else
	input_report_abs(data->input_dev, ABS_PRESSURE, 0);
	input_report_key(data->input_dev, BTN_TOUCH, 0);
#endif

	input_sync(data->input_dev);
}


static void time_function(void)
{
  int i=0;
  if(flag_tp)
  {
      	i = gpio_get_value(FT_INT_PORT);
  }
  
  if(i == 1)
  {
    ft5x0x_ts_release();
  }
  
}
	
static int ft5x0x_read_data(void)
{
	struct ft5x0x_ts_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;
//	u8 buf[14] = {0};
	u8 buf[32] = {0};
	int ret = -1;
	memset(buf, 0, sizeof(buf));

       buf[0] = 0xf9; 
	ret = ft5x0x_i2c_txdata(buf, 1);
       buf[0] = 0; 


#ifdef CONFIG_FT5X0X_MULTITOUCH
//	ret = ft5x0x_i2c_rxdata(buf, 13);
	ret = ft5x0x_i2c_rxdata(buf, 31);
#else
    ret = ft5x0x_i2c_rxdata(buf, 7);
#endif
    if (ret < 0) {
		printk("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		return ret;
	}
//added by andydeng 2012.5.11 start 	
#if 0
int i=0;	
for(i=0;i<15;i++)
  {
       printk("[%d]=%x  ",i,buf[i]);
  }
printk("\n");
#endif

#if 0
if(s_model == 712)
{
	if(buf[4] == 8)
	{

	       
		input_report_key(data->input_dev, KEY_MENU, 1);
			 #ifdef CONFIG_ANDROID_4_ICS
				input_report_abs(data->input_dev, ABS_PRESSURE, PRESS_MAX);
				input_report_key(data->input_dev, BTN_TOUCH, 1);
			  #endif
		//input_report_key(data->input_dev, KEY_BACK, 1);
		//printk("touch key menu  down!!\n");
		touch_key_down = 1;
		touch_key_count++;
		
	      return 1;
		  
	}
	else if(buf[4] == 4)
	{
		input_report_key(data->input_dev, KEY_HOMEPAGE, 1);
		 #ifdef CONFIG_ANDROID_4_ICS
			input_report_abs(data->input_dev, ABS_PRESSURE, PRESS_MAX);
			input_report_key(data->input_dev, BTN_TOUCH, 1);
		  #endif
		//input_report_key(data->input_dev, KEY_MENU, 1);
		//printk("touch key home  down!!\n");
		touch_key_down = 2;
		touch_key_count++;	
		
	      return 1;
	}
	else if(buf[4] == 2)
	{
		input_report_key(data->input_dev, KEY_BACK, 1);
		 #ifdef CONFIG_ANDROID_4_ICS
			input_report_abs(data->input_dev, ABS_PRESSURE, PRESS_MAX);
			input_report_key(data->input_dev, BTN_TOUCH, 1);
		  #endif
		//input_report_key(data->input_dev, KEY_HOME, 1);
		//printk("touch key  back down!!\n");
		touch_key_down = 3;
		touch_key_count++;	
	     return 1;
	}
	else
	{
	}

}
#endif

	memset(event, 0, sizeof(struct ts_event));
     
      event->touch_point=0;   
      if((buf[2]==0x1a)&&(buf[1]==0xaa))
      {
           event->touch_point = buf[3] & 0xf;
      }
       else
      {
      	   event->touch_point = buf[2] & 0x0F;// 000 0111
      }

    if ((event->touch_point == 0) || (event->touch_point > 5)) {
        ft5x0x_ts_release();
        return 1; 
    }
	
#if 0
   int i=0; 
	//unmasked by andydeng 2012.5.11
    for(i=0;i<10;i++)
    {
         printk("buf[0]=%x  buf[1]=%x  buf[2]=%x buf[3]=%x  buf[4]=%x  buf[5]=%x buf[6]=%x  buf[7]=%x  buf[8]=%x buf[9]=%x   \n",
		 	buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9]);
    }
#endif

#ifdef CONFIG_FT5X0X_MULTITOUCH
    switch (event->touch_point) {

         
                 
		case 5:
                       if((buf[2]==0x1a)&&(buf[1]==0xaa))
                       {
		         	event->x5 = (s16)(buf[21] & 0x0F)<<8 | (s16)buf[22];
 		        	event->y5 = (s16)(buf[23] & 0x0F)<<8 | (s16)buf[24];
                       }
                       else
                       {
                               event->x5 = (s16)(buf[0x1b] & 0x0F)<<8 | (s16)buf[0x1c];
                               event->y5 = (s16)(buf[0x1d] & 0x0F)<<8 | (s16)buf[0x1e];
                       }
		case 4:
                       if((buf[2]==0x1a)&&(buf[1]==0xaa))
                       {
		        	event->x4 = (s16)(buf[17] & 0x0F)<<8 | (s16)buf[18];
		        	event->y4 = (s16)(buf[19] & 0x0F)<<8 | (s16)buf[20];
		       }
                       else
                       {
                                event->x4 = (s16)(buf[0x15] & 0x0F)<<8 | (s16)buf[0x16];
                                event->y4 = (s16)(buf[0x17] & 0x0F)<<8 | (s16)buf[0x18];
                       }
		case 3:
                       if((buf[2]==0x1a)&&(buf[1]==0xaa))
                       {
		        	event->x3 = (s16)(buf[13] & 0x0F)<<8 | (s16)buf[14];
		        	event->y3 = (s16)(buf[15] & 0x0F)<<8 | (s16)buf[16];
                       }
                       else
                       {
                                event->x3 = (s16)(buf[0x0f] & 0x0F)<<8 | (s16)buf[0x10];
                                event->y3 = (s16)(buf[0x11] & 0x0F)<<8 | (s16)buf[0x12];
                       }
		case 2:
                       if((buf[2]==0x1a)&&(buf[1]==0xaa))
                       {
		        	event->x2 = (s16)(buf[9] & 0x0F)<<8 | (s16)buf[10];
		        	event->y2 = (s16)(buf[11] & 0x0F)<<8 | (s16)buf[12];
                       }
                       else
                       {
                                event->x2 = (s16)(buf[9] & 0x0F)<<8 | (s16)buf[10];
                                event->y2 = (s16)(buf[11] & 0x0F)<<8 | (s16)buf[12];
                      
                       }
		case 1:
                       if((buf[2]==0x1a)&&(buf[1]==0xaa))
                       {
                  //              printk("11111111111111111111111111111111 \n");
		        	event->y1 = (s16)(buf[7] & 0x0F)<<8 | (s16)buf[8];
		        	event->x1 = (s16)(buf[5] & 0x0F)<<8 | (s16)buf[6];
                       }
                       else
                       {
                    //            printk("222222222222222222222222222222222222222 \n");
                                event->x1 = (s16)(buf[3] & 0x0F)<<8 | (s16)buf[4];
                                event->y1 = (s16)(buf[5] & 0x0F)<<8 | (s16)buf[6];
                       }
                      
                       break;

		default:
		    //return -1;

		    break;   //denis_wei modify 2010-11-27
        }
	
#else
  //  if (event->touch_point == 1) {
    //	event->x1 = (s16)(buf[3] & 0x0F)<<8 | (s16)buf[4];
	//	event->y1 = (s16)(buf[5] & 0x0F)<<8 | (s16)buf[6];
 //   }
#endif
    event->pressure = 200;

	//dev_dbg(&this_client->dev, "%s: 1:%d %d 2:%d %d \n", __func__,
	//	event->x1, event->y1, event->x2, event->y2);
	//printk(KERN_DEBUG"%d (%d, %d), (%d, %d)\n", event->touch_point, event->x1, event->y1, event->x2, event->y2);

    return 0;
}

static void ft5x0x_report_value(void)
{
	struct ft5x0x_ts_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;
        
        //printk("ttttttttttttttttttttttttttttttttttttttt \n");
#ifdef CONFIG_FT5X0X_MULTITOUCH
         if((buf[5] &0xc0) ==0 ||(buf[5] &0xc0) ==0x80 )
 	  {
 	           // printk(KERN_DEBUG"==TS DOWN =\n");
                     
		     //printk("event->touch_point=%d\n",event->touch_point);	
		     #if 0
		     if (event->x1 > SCREEN_MAX_X +5)
		     	{
				if((event->y1 > 65) && (event->y1 < 80))
					{
						input_report_key(data->input_dev, KEY_MENU, 1);
						//input_report_key(data->input_dev, KEY_BACK, 1);
					//	printk("touch key back  down!!\n");
						touch_key_down = 1;
						touch_key_count++;
			     		}
				else if ((event->y1 > 40) && (event->y1 < 60))
					{
						input_report_key(data->input_dev, KEY_HOME, 1);
						//input_report_key(data->input_dev, KEY_MENU, 1);
					//	printk("touch key menu  down!!\n");
						touch_key_down = 2;
						touch_key_count++;
			     		}
				else if((event->y1 > 0) && (event->y1 < 25))
					{
						input_report_key(data->input_dev, KEY_BACK, 1);
						//input_report_key(data->input_dev, KEY_HOME, 1);
					//	printk("touch key  home down!!\n");
						touch_key_down = 3;
						touch_key_count++;
                                              //  if(touch_key_count > 2)
                                                {
                                               //  input_report_key(data->input_dev, KEY_HOME, 1);
                                                }
			     		}
				else
					{
						touch_key_count = 0;
					}
		     	}
			else
			#endif
			
			 	{
			 		#if 0
			 		if (event->x1 < SCREEN_MAX_X +5)
			 			{
							touch_key_down = 0;
							touch_key_count = 0;
			 			}
					#endif
				     switch(event->touch_point) 
				     {
		                     	
		                        				
					case 5:
						input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
						input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x5);
						input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y5);
						input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
					  #ifdef CONFIG_ANDROID_4_ICS
						input_report_abs(data->input_dev, ABS_PRESSURE, PRESS_MAX);
						input_report_key(data->input_dev, BTN_TOUCH, 1);
					  #endif
						input_mt_sync(data->input_dev);
		                     //           printk("===x5 = %d,y5 = %d ====\n",event->x5,event->y5);
					case 4:
						input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
						input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x4);
						input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y4);
						input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
					  #ifdef CONFIG_ANDROID_4_ICS
						input_report_abs(data->input_dev, ABS_PRESSURE, PRESS_MAX);
						input_report_key(data->input_dev, BTN_TOUCH, 1);
					  #endif
						input_mt_sync(data->input_dev);
		                       //         printk("===x4 = %d,y4 = %d ====\n",event->x4,event->y4);
					case 3:
						input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
						input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x3);
						input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y3);
						input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
					  #ifdef CONFIG_ANDROID_4_ICS
						input_report_abs(data->input_dev, ABS_PRESSURE, PRESS_MAX);
						input_report_key(data->input_dev, BTN_TOUCH, 1);
					  #endif
						input_mt_sync(data->input_dev);
		                         //       printk("===x3 = %d,y3 = %d ====\n",event->x3,event->y3);
					case 2:
						input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
						input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x2);
						input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y2);
						input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
					  #ifdef CONFIG_ANDROID_4_ICS
						input_report_abs(data->input_dev, ABS_PRESSURE, PRESS_MAX);
						input_report_key(data->input_dev, BTN_TOUCH, 1);
					  #endif
						input_mt_sync(data->input_dev);
						//printk("===x2 = %d,y2 = %d ====\n",event->x2,event->y2);
					
					case 1:
						{
						input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
						input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x1);
						input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y1);
						input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
					  #ifdef CONFIG_ANDROID_4_ICS
						input_report_abs(data->input_dev, ABS_PRESSURE, PRESS_MAX);
						input_report_key(data->input_dev, BTN_TOUCH, 1);
					  #endif
						input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, 1);
						input_mt_sync(data->input_dev);
						//printk("===x1 = %d,y1 = %d ====\n",event->x1,event->y1);
						}
					default:
						//printk("==touch_point default =\n");
						break;
				 	}
			 	}
     		} 
		 
		 else if((buf[5] &0xc0)==0x40)
	 	{
/*	 	
	 	       //printk(KERN_DEBUG"==TS UP =b\n");
	 	       if (touch_key_down)
		     	{
				if((event->y1 > 60) && (event->y1 < 100))
					{
						input_report_key(data->input_dev, KEY_BACK, 0);
						printk("touch key back  up!!\n");
					
			     		}
				else if ((event->y1 > 220) && (event->y1 < 260))
					{
						input_report_key(data->input_dev, KEY_MENU, 0);
						printk("touch key menu  up!!\n");
					
			     		}
				else if((event->y1 > 380) && (event->y1 < 420))
					{
						input_report_key(data->input_dev, KEY_HOME, 0);
						printk("touch key home  up!!\n");
					
			     		}
		     	}
			   else
			   	{
			 		input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
					input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x1);
					input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y1);
					input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 0);
			   	}
			input_mt_sync(data->input_dev);
*/			
			#if  0
			switch (touch_key_down)
				{
					case 1:
						input_report_key(data->input_dev, KEY_MENU, 0);
				//		printk("touch key back  up!!\n");
						break;
					case 2:
						input_report_key(data->input_dev, KEY_HOME, 0);
				//		printk("touch key menu  up!!\n");
						break;
					case 3:
						input_report_key(data->input_dev, KEY_BACK, 0);
				//		printk("touch key home  up!!\n");
						break;
					case 0:
				 		input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
						input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x1);
						input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y1);
					  #ifdef CONFIG_ANDROID_4_ICS
						input_report_abs(data->input_dev, ABS_PRESSURE, 0);
						input_report_key(data->input_dev, BTN_TOUCH, 0);
					  #endif
						input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 0);
						break;	
				}
			input_mt_sync(data->input_dev);
			#endif
					
	 	}
	 
#else	/* CONFIG_FT5X0X_MULTITOUCH*/
	if (event->touch_point == 1) {
		input_report_abs(data->input_dev, ABS_X, event->x1);
		input_report_abs(data->input_dev, ABS_Y, event->y1);
		input_report_abs(data->input_dev, ABS_PRESSURE, event->pressure);
	}
	input_report_key(data->input_dev, BTN_TOUCH, 1);
#endif	/* CONFIG_FT5X0X_MULTITOUCH*/
	input_sync(data->input_dev);

	//dev_dbg(&this_client->dev, "%s: 1:%d %d 2:%d %d \n", __func__,
	//	event->x1, event->y1, event->x2, event->y2);
}	/*end ft5x0x_report_value*/

static void ft5x0x_ts_pen_irq_work(struct work_struct *work)
{
	int ret = -1;

   if(MmFlag)	
   {
      //  printk("ft5x0x_ts_pen_irq_work++++++++++++++++++++++++++++++ \n");
	ret = ft5x0x_read_data();	
	if (ret == 0) 
	{	
		ft5x0x_report_value();
	 }
	}

	enable_irq(this_client->irq);


}

static irqreturn_t ft5x0x_ts_interrupt(int irq, void *dev_id)
{
	struct ft5x0x_ts_data *ft5x0x_ts = dev_id;
	
	//printk("==ft5x0x_ts_interrupt=\n");
        flag_tp=1;
        mod_timer(&timer, jiffies + TOUCH_TIMER);

	disable_irq_nosync(this_client->irq);


	if (!work_pending(&ft5x0x_ts->pen_event_work)) 
	{
		queue_work(ft5x0x_ts->ts_workqueue, &ft5x0x_ts->pen_event_work);
	}

	return IRQ_HANDLED;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ft5x0x_ts_suspend(struct early_suspend *handler)
{
	int ret;
	
	struct ft5x0x_ts_data *ts;
	ts =  container_of(handler, struct ft5x0x_ts_data, early_suspend);

	printk(KERN_INFO"==ft5x0x_ts_suspend==\n");
//        gpio_direction_output(FT_POWER_PORT, 0);  
//    	ft5x0x_set_reg(FT5X0X_REG_PMODE, PMODE_HIBERNATE);


	disable_irq(this_client->irq);

//add by jerry
	msleep(200);
//

	ret = cancel_work_sync(&ts->pen_event_work);	
	flush_workqueue(ts->ts_workqueue);

}

static void ft5x0x_ts_resume(struct early_suspend *handler)
{
	int err = -1;
	int count = 0;
	static int irq_counter = 0;
	static char irq_name[32] = "";
	
	printk(KERN_INFO"==ft5x0x_ts_resume==~~~\n");

#if 0
	//int err=-1;
	printk(KERN_INFO"==ft5x0x_ts_resume==~~~\n");
        err = gpio_request(FT_WAKE_PORT, "ft5x0x-ts-wake-en");
	if(err<0) {
		//dev_err(&client->dev, "Error: FT_WAKE_PORT request failed !\n");
		//goto exit_gpio_request_failed;
	}

	gpio_direction_output(FT_WAKE_PORT, 1);	
	mdelay(50);//500 
	gpio_direction_output(FT_WAKE_PORT, 0);

	
        s3c_gpio_cfgpin(FT_INT_PORT, 0x00);
	mdelay(50);	
        gpio_direction_output(FT_INT_PORT, 0);
	
	mdelay(100);


        s3c_gpio_cfgpin(FT_INT_PORT, 0x0f);
        gpio_set_value(FT_INT_PORT,1);
//	s3c_gpio_setpull(S5PV210_GPH1(0), S3C_GPIO_PULL_DOWN);
//	set_irq_type(EINT_NUM, IRQ_TYPE_EDGE_RISING);

        s3c_gpio_setpull(FT_INT_PORT, S3C_GPIO_PULL_UP);
        //set_irq_type(this_client->irq, IRQ_TYPE_LEVEL_LOW);
        set_irq_type(this_client->irq, IRQ_TYPE_EDGE_FALLING);	

        gpio_free(FT_WAKE_PORT);

	sprintf(irq_name, "ft5x0x_ts_%d", irq_counter++);
	printk("%s\n", irq_name);
	disable_irq(this_client->irq);
	free_irq(this_client->irq, s_ft5x0x_ts);
	err = request_irq(this_client->irq, ft5x0x_ts_interrupt, IRQF_TRIGGER_FALLING, irq_name, s_ft5x0x_ts);

#endif
#if 1
        err = gpio_request(FT_WAKE_PORT, "ft5x0x-ts-wake-en");
	if(err<0) {
		//dev_err(&client->dev, "Error: FT_WAKE_PORT request failed !\n");
		//goto exit_gpio_request_failed;
	}
	
	gpio_direction_output(FT_WAKE_PORT, 1);	
	mdelay(10);//500
	gpio_direction_output(FT_WAKE_PORT, 0);
	

	//gpio_free(FT_WAKE_PORT);
#endif
	#if 1 // make sure rst ok
		for(count = 0; count < 5;count++)
		{
			err = i2c_smbus_read_byte_data(this_client, 0xa6);
			msleep(30);
			if(!(err == 0x20 ||err == 0x21 )) // 0x10 -16;  0x20 - 32
			{
				cmd_write(0x07,0x00,0x00,0x00,1);
				msleep(100);
				continue;
			}
			else
				break;
		}

		printk("--resume-err:%x\n",err);
		//msleep(300);

		if (count >= 5)
		{
			gpio_direction_output(FT_WAKE_PORT, 1);			
			mdelay(10);//500 
			//printk("----get FT_WAKE_PORT : %d\n",gpio_get_value(FT_WAKE_PORT));
			gpio_direction_output(FT_WAKE_PORT, 0);
			//printk("++++get FT_WAKE_PORT : %d\n",gpio_get_value(FT_WAKE_PORT));
			gpio_free(FT_WAKE_PORT);
		}
		else
			gpio_free(FT_WAKE_PORT);
		
	#endif
	
	enable_irq(this_client->irq);

}
#endif  //CONFIG_HAS_EARLYSUSPEND


static int 
ft5x0x_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ft5x0x_ts_data *ft5x0x_ts;
	struct input_dev *input_dev;
	int err = 0, retry=0;

        printk("ft 5x06 ++++++++++++++++++++++++++++ \n");	
	printk(KERN_INFO"==ft5x0x_ts_probe=\n");
	if(g_touchscreen_init) {
		printk("skip\n");
		return 0;
	}

	check_model();

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	printk(KERN_DEBUG"==kzalloc=\n");
	ft5x0x_ts = kzalloc(sizeof(*ft5x0x_ts), GFP_KERNEL);
	if (!ft5x0x_ts)	{
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	printk(KERN_DEBUG"==i2c_set_clientdata=\n");
	this_client = client;
	this_client->irq = EINT_NUM;

	err = gpio_request(FT_WAKE_PORT, "ft5x0x-ts-wake-en");
	if(err<0) {
		dev_err(&client->dev, "Error: FT_WAKE_PORT request failed !\n");
		//goto exit_gpio_request_failed;
	}
	
	s_ft5x0x_ts=ft5x0x_ts;
	
	gpio_direction_output(FT_WAKE_PORT, 1);
	msleep(50);
	gpio_direction_output(FT_WAKE_PORT, 0);
	

	msleep(50);
	
	//gpio_free(FT_WAKE_PORT);
	msleep(16);
	
	
	#if 1 //FIXME: disable it for testing
	for(retry=0; retry < 4; retry++) {
		err =ft5x0x_i2c_txdata( NULL, 0);
		if (err > 0)
			break;
	}

	if(err <= 0) {
	//	printk("222222222222222222222222222222222222222222222222 \n");
		dev_err(&client->dev, "Warnning: I2C connection might be something wrong!\n");
		goto err_i2c_failed;
	}
	#endif


	#if 1 // make sure rst ok
		for(retry = 0; retry < 5;retry++)
		{
			err = i2c_smbus_read_byte_data(this_client, 0xa6);
			msleep(30);
			if(!(err == 0x20 ||err == 0x21 )) // 0x10 -16;  0x20 - 32
			{
				cmd_write(0x07,0x00,0x00,0x00,1);
				msleep(100);
				continue;
			}
			else
				break;
		}

		printk("--resume-err:%x\n",err);
		//msleep(300);

		if (retry >= 5)
		{
			gpio_direction_output(FT_WAKE_PORT, 1);			
			mdelay(10);//500 
			gpio_direction_output(FT_WAKE_PORT, 0);
			
			gpio_free(FT_WAKE_PORT);
		}
		else
			gpio_free(FT_WAKE_PORT);
		
	#endif


	i2c_set_clientdata(client, ft5x0x_ts);

	INIT_WORK(&ft5x0x_ts->pen_event_work, ft5x0x_ts_pen_irq_work);
	ft5x0x_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
	if (!ft5x0x_ts->ts_workqueue) {
		err = -ESRCH;
		goto exit_create_singlethread;
	}


	s3c_gpio_setpull(FT_INT_PORT, S3C_GPIO_PULL_UP);
	//set_irq_type(this_client->irq, IRQ_TYPE_LEVEL_LOW);
  	set_irq_type(this_client->irq, IRQ_TYPE_EDGE_FALLING);
	//set_irq_type(this_client->irq, IRQ_TYPE_EDGE_BOTH);


	err = request_irq(this_client->irq, ft5x0x_ts_interrupt, IRQF_TRIGGER_FALLING, "ft5x0x_ts", ft5x0x_ts);
	if (err < 0) {
		dev_err(&client->dev, "ft5x0x_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}

	disable_irq(this_client->irq);

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
	
	ft5x0x_ts->input_dev = input_dev;

#ifdef CONFIG_FT5X0X_MULTITOUCH
	set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);

	input_set_abs_params(input_dev,
			     ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_TOUCH_MAJOR, 0, PRESS_MAX, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);

#ifdef CONFIG_ANDROID_4_ICS
	set_bit(ABS_PRESSURE, input_dev->absbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	input_set_abs_params(input_dev,
			     ABS_PRESSURE, 0, PRESS_MAX, 0 , 0);
#endif
     input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 5, 0, 0);

#else
	set_bit(ABS_X, input_dev->absbit);
	set_bit(ABS_Y, input_dev->absbit);
	set_bit(ABS_PRESSURE, input_dev->absbit);
	set_bit(BTN_TOUCH, input_dev->keybit);

	input_set_abs_params(input_dev, ABS_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_PRESSURE, 0, PRESS_MAX, 0 , 0);
#endif

	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);

	
	
	set_bit(KEY_BACK, input_dev->keybit);
	set_bit(KEY_MENU, input_dev->keybit);
	set_bit(KEY_HOME, input_dev->keybit);
	set_bit(KEY_HOMEPAGE, input_dev->keybit); //wigo 

	input_dev->name		= FT5X0X_NAME;		//dev_name(&client->dev)
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev, "ft5x0x_ts_probe: failed to register input device: %s\n", 
			dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	printk(KERN_DEBUG"==register_early_suspend =\n");
	ft5x0x_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ft5x0x_ts->early_suspend.suspend = ft5x0x_ts_suspend;
	ft5x0x_ts->early_suspend.resume	= ft5x0x_ts_resume;
	register_early_suspend(&ft5x0x_ts->early_suspend);
#endif

	if(ft5x0x_set_reg(0x88, 0x05) <= 0) //5, 6,7,8
        {
          printk("error error \n");
          goto exit_input_register_device_failed;
        }

	if(ft5x0x_set_reg(0x80, 30) <= 0)
        {
           printk("error error \n");
          goto exit_input_register_device_failed;
        }
	
    	enable_irq(this_client->irq);
        
        init_timer(&timer);
        timer.function = (void *)time_function;
        timer.expires = jiffies + TOUCH_TIMER;
        add_timer(&timer);

	dev_warn(&client->dev, "=====probe over =====\n");
	g_touchscreen_init = 1;
	   
    return 0;

exit_input_register_device_failed:
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
	free_irq(this_client->irq, ft5x0x_ts);
exit_irq_request_failed:
//exit_platform_data_null:
	cancel_work_sync(&ft5x0x_ts->pen_event_work);
	destroy_workqueue(ft5x0x_ts->ts_workqueue);
exit_create_singlethread:
	printk(KERN_DEBUG"==singlethread error =\n");
	i2c_set_clientdata(client, NULL);
exit_gpio_request_failed:	
err_i2c_failed:	
	kfree(ft5x0x_ts);
exit_alloc_data_failed:
exit_check_functionality_failed:
    //free_irq(EINT_NUM, ft5x0x_ts);
	return err;
}

static int __devexit ft5x0x_ts_remove(struct i2c_client *client)
{
	struct ft5x0x_ts_data *ft5x0x_ts = i2c_get_clientdata(client);

	printk(KERN_INFO"==ft5x0x_ts_remove=\n");

	unregister_early_suspend(&ft5x0x_ts->early_suspend);
	free_irq(this_client->irq, ft5x0x_ts);
	input_unregister_device(ft5x0x_ts->input_dev);
	kfree(ft5x0x_ts);
	cancel_work_sync(&ft5x0x_ts->pen_event_work);
	destroy_workqueue(ft5x0x_ts->ts_workqueue);
	i2c_set_clientdata(client, NULL);
	return 0;
}

static const struct i2c_device_id ft5x0x_ts_id[] = {
	{ FT5X0X_NAME, 0 },{ }
};
MODULE_DEVICE_TABLE(i2c, ft5x0x_ts_id);

static struct i2c_driver ft5x0x_ts_driver = {
	.probe		= ft5x0x_ts_probe,
	.remove		= __devexit_p(ft5x0x_ts_remove),
	.id_table	= ft5x0x_ts_id,
	.driver	= {
		.name	= FT5X0X_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init ft5x0x_ts_init(void)
{
	u8 ret = 0;
	extern char g_selected_tp[32];

	if(strncmp( g_selected_tp, "ft54", strlen("ft54"))==0) {
		printk(KERN_INFO"## 105 ft5x0x_ts_init= [%s]\n", g_selected_tp);	
		return -1;
	}	
	
	printk(KERN_INFO"==ft5x0x_ts_init=\n");	
	ret = i2c_add_driver(&ft5x0x_ts_driver);
	printk("ret = %d.\n",ret);
	return ret;
	
}

static void __exit ft5x0x_ts_exit(void)
{
	printk(KERN_INFO"==ft5x0x_ts_exit=\n");
	i2c_del_driver(&ft5x0x_ts_driver);
}

late_initcall(ft5x0x_ts_init);
module_exit(ft5x0x_ts_exit);

MODULE_AUTHOR("<wenfs@Focaltech-systems.com>");
MODULE_DESCRIPTION("FocalTech ft5x0x TouchScreen driver");
MODULE_LICENSE("GPL");
