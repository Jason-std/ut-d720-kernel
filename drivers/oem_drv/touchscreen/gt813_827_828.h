/* drivers/input/touchscreen/gt813_827_828.h
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
 *
 */

#ifndef _LINUX_GOODIX_TOUCH_H
#define	_LINUX_GOODIX_TOUCH_H

#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <mach/gpio.h>
#include <linux/earlysuspend.h>

#define GTP_CREATE_WR_NODE 0
//extern s32 gtp_update_proc(void*);
//extern struct i2c_client * i2c_connect_client;
//extern s32 gtp_read_version(struct i2c_client *clien, u8 **version);

struct goodix_ts_data {
  spinlock_t irq_lock;
  struct i2c_client *client;
  struct input_dev  *input_dev;
  struct hrtimer timer;
  struct work_struct  work;
  struct early_suspend early_suspend;
  int irq_is_disable;
  int use_irq;
  int abs_x_max;
  int abs_y_max;
  int  max_touch_num;
  int  int_trigger_type;
  int  green_wake_mode;
  int  chip_type;
  int  enter_update;
  int  gtp_is_suspend;
};

extern u16 show_len;
extern u16 total_len;

//***************************PART1:ON/OFF define*******************************
#define GTP_CUSTOM_CFG        1
#define GTP_DRIVER_SEND_CFG   1
#define GTP_HAVE_TOUCH_KEY    0
#define GTP_POWER_CTRL_SLEEP 1 //0   // modefy andydeng
#define GTP_AUTO_UPDATE       0
#define GTP_CHANGE_X2Y        1
#define GTP_ESD_PROTECT        0 //0

#define GUP_USE_HEADER_FILE   0
#if GUP_USE_HEADER_FILE
    #define GT827       //Choose you IC type.GT813,GT827,GT828
#endif

#define GTP_DEBUG_ON          0
#define GTP_DEBUG_ARRAY_ON    0
#define GTP_DEBUG_FUNC_ON     0

//***************************PART2:TODO define**********************************
//STEP_1(REQUIRED):Change config table.
/*TODO: puts the config info corresponded to your TP here, the following is just 
a sample config, send this config should cause the chip cannot work normally*/
//default or float
//1648
#define CTP_CFG_GROUP1_1648 {0x1D,0x0E,0x1C,0x0D,0x1B,0x0C,0x1A,0x0B,0x19,0x0A,0x18,0x09,0x17,0x08,0x16,0x07,0x15,0x06,0x14,0x05,0x13,0x04,0x12,0x03,0x11,0x02,0x10,0x01,0x0F,0x00,0x13,0x09,0x12,0x08,0x11,0x07,0x10,0x06,0x0F,0x05,0x0E,0x04,0x0D,0x03,0x0C,0x02,0x0B,0x01,0x0A,0x00,0x1F,0x03,0x60,0x00,0x00,0x10,0x00,0x00,0x09,0x00,0x00,0x02,0x3F,0x37,0x34,0x03,0x00,0x05,0x00,0x05,0x00,0x03,0x20,0x4A,0x46,0x45,0x41,0x07,0x00,0x03,0x19,0x05,0x14,0x10,0x00,0x04,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2A,0x2A,0x00,0x37,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01}
//1697
#define CTP_CFG_GROUP1_1697 {0x1D,0x0E,0x1C,0x0D,0x1B,0x0C,0x1A,0x0B,0x19,0x0A,0x18,0x09,0x17,0x08,0x16,0x07,0x00,0x0F,0x01,0x10,0x02,0x11,0x03,0x12,0x04,0x13,0x05,0x14,0x06,0x15,0x00,0x0A,0x01,0x0B,0x02,0x0C,0x03,0x0D,0x04,0x0E,0x05,0x0F,0x06,0x10,0x07,0x11,0x08,0x12,0x09,0x13,0x1F,0x03,0x08,0x00,0x00,0x12,0x00,0x00,0x08,0x00,0x00,0x02,0x41,0x31,0x30,0x03,0x00,0x05,0x00,0x04,0x00,0x03,0x00,0x45,0x43,0x41,0x3F,0x05,0x00,0x03,0x19,0x05,0x14,0x10,0x00,0x04,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2A,0x2A,0x00,0x37,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01}
//TODO puts your group2 config info here,if need.
//VDD
#define CTP_CFG_GROUP2 {\
  }
//TODO puts your group3 config info here,if need.
//GND
#define CTP_CFG_GROUP3 {\
  }

//STEP_2(REQUIRED):Change I/O define & I/O operation mode.
#define TOUCH_RESET_PIN  EXYNOS4212_GPM3(4)
#define GTP_RST_PORT TOUCH_RESET_PIN
//#define GTP_INT_PORT    S3C64XX_GPN(15) //S3C64XX_GPL(10) //S3C64XX_GPN(15)
#define GTP_INT_PORT    EXYNOS4_GPX0(4)
#define GTP_INT_IRQ     gpio_to_irq(EXYNOS4_GPX0(4))
#define GTP_INT_CFG     S3C_GPIO_SFN(2) //S3C_GPIO_SFN(3)  //S3C_GPIO_SFN(2)


#define GTP_GPIO_AS_INPUT(pin)          do{\
                                            gpio_direction_input(pin);\
                                           gpio_set_value(pin,0);\
                                        }while(0)
#define GTP_GPIO_AS_INT(pin,irq)          do{\
						GTP_GPIO_AS_INPUT(pin);\
                                            	irq = gpio_to_irq(pin);\
					}while(0)
                                          
                                     
#define GTP_GPIO_GET_VALUE(pin)         gpio_get_value(pin)
#define GTP_GPIO_OUTPUT(pin,level)      gpio_direction_output(pin,level)
#define GTP_GPIO_REQUEST(pin, label)    gpio_request(pin, label)
#define GTP_GPIO_FREE(pin)              gpio_free(pin)
#define GTP_IRQ_TAB                     {IRQ_TYPE_EDGE_FALLING,IRQ_TYPE_EDGE_RISING}

//STEP_3(optional):Custom set some config by themself,if need.
#if GTP_CUSTOM_CFG
  //#define GTP_MAX_HEIGHT   800			
  //#define GTP_MAX_WIDTH    480
  //static unsigned  GTP_MAX_HEIGHT  =1024;    //   97'
  //static unsigned  GTP_MAX_WIDTH   = 768;    // 
 #define GTP_MAX_HEIGHT   1280			
#define GTP_MAX_WIDTH    800
  #define GTP_INT_TRIGGER  1    //0:Falling 1:Rising
#else
  //#define GTP_MAX_HEIGHT   4096
  //#define GTP_MAX_WIDTH    4096
  static unsigned  GTP_MAX_HEIGHT  = 4096; 
  static unsigned  GTP_MAX_WIDTH   = 4096;
  #define GTP_INT_TRIGGER  1
#endif
#define GTP_MAX_TOUCH      5
#define GTP_ESD_CHECK_CIRCLE  2000

//STEP_4(optional):If this project have touch key,Set touch key config.                                    
#if GTP_HAVE_TOUCH_KEY
  #define GTP_KEY_TAB	 {KEY_MENU, KEY_HOME, KEY_SEND}
#endif

//***************************PART3:OTHER define*********************************
#define GTP_DRIVER_VERSION    "V1.0<2012/05/01>"
#define GTP_I2C_NAME          "Goodix-TS"
#define GTP_POLL_TIME		  10
#define GTP_ADDR_LENGTH       2
#define GTP_CONFIG_LENGTH     112
#define FAIL                  0
#define SUCCESS               1

//Register define
#define GTP_READ_COOR_ADDR    0x0F40
#define GTP_REG_SLEEP         0x0F72
#define GTP_REG_SENSOR_ID     0x0FF5
#define GTP_REG_CONFIG_DATA   0x0F80
#define GTP_REG_VERSION       0x0F7D

#define RESOLUTION_LOC        71
#define TRIGGER_LOC           66

//Log define
#define GTP_INFO(fmt,arg...)           printk("<<-GTP-INFO->> "fmt"\n",##arg)
#define GTP_ERROR(fmt,arg...)          printk("<<-GTP-ERROR->> "fmt"\n",##arg)
#define GTP_DEBUG(fmt,arg...)          do{\
                                         if(GTP_DEBUG_ON)\
                                         printk("<<-GTP-DEBUG->> [%d]"fmt"\n",__LINE__, ##arg);\
                                       }while(0)
#define GTP_DEBUG_ARRAY(array, num)    do{\
                                         s32 i;\
                                         u8* a = array;\
                                         if(GTP_DEBUG_ARRAY_ON)\
                                         {\
                                            printk("<<-GTP-DEBUG-ARRAY->>\n");\
                                            for (i = 0; i < (num); i++)\
                                            {\
                                                printk("%02x   ", (a)[i]);\
                                                if ((i + 1 ) %10 == 0)\
                                                {\
                                                    printk("\n");\
                                                }\
                                            }\
                                            printk("\n");\
                                        }\
                                       }while(0)
#define GTP_DEBUG_FUNC()               do{\
                                         if(GTP_DEBUG_FUNC_ON)\
                                         printk("<<-GTP-FUNC->> Func:%s@Line:%d\n",__func__,__LINE__);\
                                       }while(0)
#define GTP_SWAP(x, y)                 do{\
                                         typeof(x) z = x;\
                                         x = y;\
                                         y = z;\
                                       }while (0)

//****************************PART4:UPDATE define*******************************
#define PACK_SIZE               64
#define MAX_TIMEOUT             60000
#define MAX_I2C_RETRIES         200

//I2C buf address
#define ADDR_CMD                80
#define ADDR_STA                81
#ifdef UPDATE_NEW_PROTOCOL
	#define ADDR_DAT              0
#else
	#define ADDR_DAT              82
#endif

//Moudle state
#define NEW_UPDATE_START        0x01
#define UPDATE_START            0x02
#define SLAVE_READY             0x08
#define UNKNOWN_ERROR           0x00
#define FRAME_ERROR             0x10
#define CHECKSUM_ERROR          0x20
#define TRANSLATE_ERROR         0x40
#define FLASH_ERROR             0x80

//Error no
#define ERROR_NO_FILE           2   //ENOENT
#define ERROR_FILE_READ         23  //ENFILE
#define ERROR_FILE_TYPE         21  //EISDIR
#define ERROR_GPIO_REQUEST      4   //EINTR
#define ERROR_I2C_TRANSFER      5   //EIO
#define ERROR_NO_RESPONSE       16  //EBUSY
#define ERROR_TIMEOUT           110 //ETIMEDOUT


  struct tp_event {
	u16	x;
	u16	y;
	u16	w;
	u16	id;
	
};
#include <linux/input/mt.h>
//*****************************End of Part III********************************


#endif /* _LINUX_GOODIX_TOUCH_H */

