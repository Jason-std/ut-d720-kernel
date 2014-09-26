/*
 * driver for Fusitju UT2055 LS 8MP camera
 *
 * Copyright (c) 2010, Samsung Electronics. All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include <linux/i2c.h>
#include <linux/init.h>
#include <media/v4l2-device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/vmalloc.h>
#include <linux/firmware.h>
#include <linux/videodev2.h>
#include <linux/io.h>

#ifdef CONFIG_VIDEO_SAMSUNG_V4L2
#include <linux/videodev2_samsung.h>
#endif

#include <linux/regulator/machine.h>

#include <media/ut2055_platform.h>

//
//#include "ut2055.h"
#include "ut2055_good_in_mid.h"
//#include "ut2055_good_in_pc.h"
//


#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

extern char g_selected_came[];
extern char g_selected_utmodel[];

#define CONTINUOUS_FOCUS
#define UT2055_DRIVER_NAME	"UT2055"
#define SDCARD_FW
#ifdef SDCARD_FW
#define UT2055_FW_PATH		"/sdcard/RS_M5LS.bin"
#endif /* SDCARD_FW */
#define UT2055T_FW_REQUEST_PATH	"ut2055/RS_M5LS_T.bin"	/* Techwin */
#define UT2055O_FW_REQUEST_PATH	"ut2055/RS_M5LS_O.bin"	/* Optical communication */
#define UT2055_FW_DUMP_PATH	"/data/RS_M5LS_dump.bin"
#define UT2055_FW_VER_LEN		24
#define UT2055_FW_VER_FILE_CUR	0x16FF00

#define UT2055_FLASH_BASE_ADDR	0x10000000
#define UT2055_INT_RAM_BASE_ADDR	0x68000000

#define UT2055_I2C_RETRY		5
#define UT2055_I2C_VERIFY		100
#define UT2055_ISP_TIMEOUT	3000
#define UT2055_ISP_AFB_TIMEOUT	15000 /* FIXME */
#define UT2055_ISP_ESD_TIMEOUT	1000

#define UT2055_JPEG_MAXSIZE	0x3A0000
#define UT2055_THUMB_MAXSIZE	0xFC00
#define UT2055_POST_MAXSIZE	0xBB800

#define UT2055_DEF_APEX_DEN	100

#define ut2055_readb(sd, g, b, v)		ut2055_read(sd, 1, g, b, v)
#define ut2055_readw(sd, g, b, v)		ut2055_read(sd, 2, g, b, v)
#define ut2055_readl(sd, g, b, v)		ut2055_read(sd, 4, g, b, v)

#define ut2055_writeb(sd, g, b, v)	ut2055_write(sd, 1, g, b, v)
#define ut2055_writew(sd, g, b, v)	ut2055_write(sd, 2, g, b, v)
#define ut2055_writel(sd, g, b, v)	ut2055_write(sd, 4, g, b, v)

#define CHECK_ERR(x)	if ((x) < 0) { \
				cam_err("i2c failed, err %d\n", x); \
				return x; \
			}


static struct i2c_client *gclient;

static int gc0329 = 0;
static int ghm2055 = 0;
static int ggc0308 = 0;
static int ghi253 = 0;
static int ghi704 = 0;
static int gOv56540 = 0;
static int ghm5065 = 0;
static int first_check = 1;
static int cam_count = 0;
static int front_reverse = 0;
static int gCameraId = 0;
static int camera_Ycbcr_type = 0;

int WhiteBanlanceValue;
static int gIsHm2055 = 1;
static int gFirstCheck = 1;
static int gIs_30W_capture = 1;
static int gIs_720P_capture = 0;
static int gIs_200W_capture = 0;
static int gIs_500W_capture = 0;
static int g_cam_model=0;
static int cam_sw_720p=0;
static int cam_sw_vga=0;
static int cam_sw_flag=0;
#ifdef CONTINUOUS_FOCUS
static int g_focus_mode=0;
#endif

static const struct ut2055_frmsizeenum preview_frmsizes[] = {
	{ UT2055_PREVIEW_QCIF,	176,	144,	0x05 },	/* 176 x 144 */
	{ UT2055_PREVIEW_QCIF2,	528,	432,	0x2C },	/* 176 x 144 */
	{ UT2055_PREVIEW_QVGA,	320,	240,	0x09 },
	{ UT2055_PREVIEW_VGA,	640,	480,	0x17 },
	{ UT2055_PREVIEW_D1,	720,	480,	0x18 },
	{ UT2055_PREVIEW_WVGA,	800,	480,	0x1A },
	{ UT2055_PREVIEW_720P,	1280,	720,	0x21 },
	{ UT2055_PREVIEW_1080P,	1920,	1080,	0x28 },
	{ UT2055_PREVIEW_HDR,	3264,	2448,	0x27 },
};

static const struct ut2055_frmsizeenum capture_frmsizes[] = {
	{ UT2055_CAPTURE_VGA,	640,	480,	0x09 },
	{ UT2055_CAPTURE_WVGA,	800,	480,	0x0A },
	{ UT2055_CAPTURE_W1MP,	1600,	960,	0x2C },
	{ UT2055_CAPTURE_2MP,	1600,	1200,	0x2C },//jerry
	{ UT2055_CAPTURE_W2MP,	2048,	1232,	0x2C },
	{ UT2055_CAPTURE_3MP,	2048,	1536,	0x1B },
	{ UT2055_CAPTURE_W4MP,	2560,	1536,	0x1B },
	{ UT2055_CAPTURE_5MP,	2592,	1944,	0x1B },
	{ UT2055_CAPTURE_W6MP,	3072,	1856,	0x1B },	
	{ UT2055_CAPTURE_7MP,	3072,	2304,	0x2D },
	{ UT2055_CAPTURE_W7MP,	2560,	1536,	0x2D },
	{ UT2055_CAPTURE_8MP,	3264,	2448,	0x25 },
};

static struct ut2055_control ut2055_ctrls[] = {
	{
		.id = V4L2_CID_CAMERA_ISO,
		.minimum = ISO_AUTO,
		.maximum = ISO_800,
		.step = 1,
		.value = ISO_AUTO,
		.default_value = ISO_AUTO,
	}, {
		.id = V4L2_CID_CAMERA_BRIGHTNESS,
		.minimum = EV_MINUS_4,
		.maximum = EV_MAX - 1,
		.step = 1,
		.value = EV_DEFAULT,
		.default_value = EV_DEFAULT,
	}, {
		.id = V4L2_CID_CAMERA_SATURATION,
		.minimum = SATURATION_MINUS_2,
		.maximum = SATURATION_MAX - 1,
		.step = 1,
		.value = SATURATION_DEFAULT,
		.default_value = SATURATION_DEFAULT,
	}, {
		.id = V4L2_CID_CAMERA_SHARPNESS,
		.minimum = SHARPNESS_MINUS_2,
		.maximum = SHARPNESS_MAX - 1,
		.step = 1,
		.value = SHARPNESS_DEFAULT,
		.default_value = SHARPNESS_DEFAULT,
	}, {
		.id = V4L2_CID_CAMERA_ZOOM,
		.minimum = ZOOM_LEVEL_0,
		.maximum = ZOOM_LEVEL_MAX - 1,
		.step = 1,
		.value = ZOOM_LEVEL_0,
		.default_value = ZOOM_LEVEL_0,
	}, {
		.id = V4L2_CID_CAM_JPEG_QUALITY,
		.minimum = 1,
		.maximum = 100,
		.step = 1,
		.value = 100,
		.default_value = 100,
	},
};

static inline struct ut2055_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct ut2055_state, sd);
}


#if 1
static int ut2055_read(struct v4l2_subdev *sd,
	u8 len, u8 category, u8 byte, int *val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg;
	unsigned char data[5];
	unsigned char recv_data[len + 1];
	int i, err = 0;

	if (!client->adapter)
		return -ENODEV;

	if (len != 0x01 && len != 0x02 && len != 0x04)
		return -EINVAL;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = sizeof(data);
	msg.buf = data;

	/* high byte goes out first */
	data[0] = msg.len;
	data[1] = 0x01;			/* Read category parameters */
	data[2] = category;
	data[3] = byte;
	data[4] = len;

	for (i = UT2055_I2C_RETRY; i; i--) {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			break;
		mdelay(20);
	}

	if (err != 1) {
		cam_err("category %#x, byte %#x\n", category, byte);
		return err;
	}

	msg.flags = I2C_M_RD;
	msg.len = sizeof(recv_data);
	msg.buf = recv_data;
	for (i = UT2055_I2C_RETRY; i; i--) {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			break;
		mdelay(20);
	}

	if (err != 1) {
		cam_err("category %#x, byte %#x\n", category, byte);
		return err;
	}

	if (recv_data[0] != sizeof(recv_data))
		cam_i2c_dbg("expected length %d, but return length %d\n",
				 sizeof(recv_data), recv_data[0]);

	if (len == 0x01)
		*val = recv_data[1];
	else if (len == 0x02)
		*val = recv_data[1] << 8 | recv_data[2];
	else
		*val = recv_data[1] << 24 | recv_data[2] << 16 |
				recv_data[3] << 8 | recv_data[4];

	cam_i2c_dbg("category %#02x, byte %#x, value %#x\n", category, byte, *val);
	return err;
}

static int ut2055_write(struct v4l2_subdev *sd,
	u8 len, u8 category, u8 byte, int val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg;
	unsigned char data[len + 4];
	int i, err;

	if (!client->adapter)
		return -ENODEV;

	if (len != 0x01 && len != 0x02 && len != 0x04)
		return -EINVAL;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = sizeof(data);
	msg.buf = data;

	data[0] = msg.len;
	data[1] = 0x02;			/* Write category parameters */
	data[2] = category;
	data[3] = byte;
	if (len == 0x01) {
		data[4] = val & 0xFF;
	} else if (len == 0x02) {
		data[4] = (val >> 8) & 0xFF;
		data[5] = val & 0xFF;
	} else {
		data[4] = (val >> 24) & 0xFF;
		data[5] = (val >> 16) & 0xFF;
		data[6] = (val >> 8) & 0xFF;
		data[7] = val & 0xFF;
	}

	cam_i2c_dbg("category %#x, byte %#x, value %#x\n", category, byte, val);

	for (i = UT2055_I2C_RETRY; i; i--) {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			break;
		mdelay(20);
	}

	return err;
}

static int ut2055_mem_read(struct v4l2_subdev *sd, u16 len, u32 addr, u8 *val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg;
	unsigned char data[8];
	unsigned char recv_data[len + 3];
	int i, err = 0;

	if (!client->adapter)
		return -ENODEV;

	if (len <= 0)
		return -EINVAL;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = sizeof(data);
	msg.buf = data;

	/* high byte goes out first */
	data[0] = 0x00;
	data[1] = 0x03;
	data[2] = (addr >> 24) & 0xFF;
	data[3] = (addr >> 16) & 0xFF;
	data[4] = (addr >> 8) & 0xFF;
	data[5] = addr & 0xFF;
	data[6] = (len >> 8) & 0xFF;
	data[7] = len & 0xFF;

	for (i = UT2055_I2C_RETRY; i; i--) {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			break;
		mdelay(20);
	}

	if (err != 1)
		return err;

	msg.flags = I2C_M_RD;
	msg.len = sizeof(recv_data);
	msg.buf = recv_data;
	for (i = UT2055_I2C_RETRY; i; i--) {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			break;
		mdelay(20);
	}

	if (err != 1)
		return err;

	if (len != (recv_data[1] << 8 | recv_data[2]))
		cam_i2c_dbg("expected length %d, but return length %d\n",
			len, recv_data[1] << 8 | recv_data[2]);

	memcpy(val, recv_data + 3, len);

	cam_i2c_dbg("address %#x, length %d\n", addr, len);
	return err;
}

#endif

#if 1

static int ut2055_i2c_write(struct v4l2_subdev *sd, unsigned char i2c_data[], unsigned char length);


static int ut2055_i2c_read(struct v4l2_subdev *sd, unsigned char reg)
{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
#if 0	
	int rc;

	unsigned char data[1]={0};
	data[0] = reg;
	i2c_master_recv(client, data ,1);
	return data[0];
#endif 

//	return i2c_smbus_read_byte_data(client, reg);
	return i2c_smbus_read_byte_data(gclient, reg);
}

//extern s32 i2c_smbus_write_byte_data(struct i2c_client *client, u8 command, u8 value);


static int ut2055_i2c_write(struct v4l2_subdev *sd, unsigned char i2c_data[],
				unsigned char length)
{
#if 0 //FIXME: urbetter skip I2C
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg = {client->addr, 0, length, i2c_data};
//	struct i2c_msg msg = {client->addr, I2C_M_IGNORE_NAK, length, buf};

	return i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;  
#else
/*
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	if (gCameraId)
		client->addr = 0x42>>1;
	else
		client->addr = 0x60>>1;
*/
//	return i2c_smbus_write_byte_data(client, i2c_data[0], i2c_data[1]);	
	int ret;
	ret = i2c_smbus_write_byte_data(gclient, i2c_data[0], i2c_data[1]);
	return ret;
#endif
}

static int hm2055_i2c_write(struct v4l2_subdev *sd, unsigned char i2c_data[],
				unsigned char length);


static int hm2055_i2c_read(struct v4l2_subdev *sd, unsigned short reg)
{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
#if 1	
	int rc;

	unsigned char data[2]={0};
	data[0]= reg>>8;
	data[1]= (unsigned char)(reg & 0x00ff);
	printk("liao test read data %x %x\n",data[0],data[1]);
	i2c_master_recv(gclient, data ,1);
	printk("liao test read data %x %x\n",data[0],data[1]);
	return data[0];
#endif 

	return i2c_smbus_read_byte_data(gclient, reg);
}


//extern s32 i2c_smbus_write_byte_data(struct i2c_client *client, u8 command, u8 value);


static int hm2055_i2c_write(struct v4l2_subdev *sd, unsigned char i2c_data[],
				unsigned char length)
{
#if 1 //FIXME: urbetter skip I2C
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg = {gclient->addr, 0, length, i2c_data};
//	struct i2c_msg msg = {client->addr, I2C_M_IGNORE_NAK, length, buf};

	return i2c_transfer(gclient->adapter, &msg, 1) == 1 ? 0 : -EIO;  
#else

	struct i2c_client *client = v4l2_get_subdevdata(sd);
	return i2c_smbus_write_byte_data(client, i2c_data[0], i2c_data[1]);
#endif
}


//#if 1
/*
 * OV7675 register structure : 2bytes address, 2bytes value
 * retry on write failure up-to 5 times
 */
static inline int hm2055_write(struct v4l2_subdev *sd, u8 addr, u8 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg[1];
	unsigned char reg[2];
	int err = 0;
	int retry = 0;


	if (!client->adapter)
		return -ENODEV;

again:
	msg->addr = gclient->addr;
	msg->flags = 0;
	msg->len = 2;
	msg->buf = reg;

	reg[0] = addr & 0xff;
	reg[1] = val & 0xff;

	err = i2c_transfer(gclient->adapter, msg, 1);
	if (err >= 0)
		return err;	/* Returns here on success */

	/* abnormal case: retry 5 times */
	if (retry < 5) {
		dev_err(&client->dev, "%s: address: 0x%02x%02x, " \
			"value: 0x%02x%02x\n", __func__, \
			reg[0], reg[1], reg[2], reg[3]);
		retry++;
		goto again;
	}

	return err;
}




#if 1
#define OV5640_CMD_ACK 0x3023
#define OV5640_CMD_MAIN 0x3022
#define OV5640_CMD_FW_STATUS 0x3029

static u8 ov5640_i2c_buf[4];
static u8 ov5640_counter = 0;



static int ov5640_i2c_rxdata(unsigned short saddr,
        unsigned char *rxdata, int length)	
{
        struct i2c_msg msgs[] = {
        {
                .addr   = saddr,
                .flags = 0,
                .len   = 2,
                .buf   = rxdata,
        },
        {
                .addr   = saddr,
                .flags = I2C_M_RD,
                .len   = length,
                .buf   = rxdata,
        },
        };
		
        if (i2c_transfer(gclient->adapter, msgs, 2) < 0) {
                printk("ov5640_i2c_rxdata failed!\n");
                printk(KERN_ERR "ov5640_i2c_rxdata failed!\n");
                return -EIO;
        }
        return 0;
}

static int32_t ov5640_i2c_read(unsigned short   saddr,
        unsigned int raddr, unsigned int *rdata){
        int rc = 0;
        unsigned char buf[2];
        #ifdef CAM_DBG
        printk(KERN_ERR "+ov5640_i2c_read\n");
        #endif
        memset(buf, 0, sizeof(buf));
        buf[0] = (raddr & 0xFF00)>>8;
        buf[1] = (raddr & 0x00FF);
        rc = ov5640_i2c_rxdata(saddr, buf, 2);
        if (rc < 0)
        {
        	printk("ov5640_i2c_read  reg 0x%x failed!\n",raddr);
			return rc;
        }
        *rdata = buf[0] << 8 | buf[1];
/*		
        if (rc < 0)        
		printk("ov5640_i2c_read failed!\n");
        #ifdef CAM_DBG
        printk(KERN_ERR "-ov5640_i2c_read\n");
        #endif
*/        
        return rc;
}

static int32_t ov5640_i2c_read_byte(unsigned short   saddr,
    unsigned int raddr, unsigned int *rdata){
    int rc = 0;
    unsigned char buf[2];
    #ifdef CAM_DBG
    printk(KERN_ERR "+ov5640_i2c_read\n");
    #endif
    memset(buf, 0, sizeof(buf));
    buf[0] = (raddr & 0xFF00)>>8;
    buf[1] = (raddr & 0x00FF);
    rc = ov5640_i2c_rxdata(saddr, buf, 1);
    if (rc < 0) 
        	{
        		printk("ov5640_i2c_read_byte  reg 0x%x failed!\n",raddr);
			return rc;
        	}
    *rdata = buf[0];
    if (rc < 0) 
		printk("ov5640_i2c_read failed!\n");
    #ifdef CAM_DBG
    printk(KERN_ERR "-ov5640_i2c_read\n");
    #endif
    return rc;
}

static int ov5640_i2c_txdata(u16 saddr,u8 *txdata,int length)
{
        struct i2c_msg msg[] = {
                {
                        .addr  = saddr,
                        .flags = 0,
                        .len = length,
                        .buf = txdata,
                },
        };
        if (i2c_transfer(gclient->adapter, msg, 1) < 0)        
			return -EIO;
        else 
			return 0;
}

static int ov5640_i2c_write(unsigned short saddr, unsigned int waddr,
        unsigned short bdata,u8 trytimes)
{
   int rc = -EIO;
   ov5640_counter = 0;
   ov5640_i2c_buf[0] = (waddr & 0xFF00)>>8;
   ov5640_i2c_buf[1] = (waddr & 0x00FF);
   //ov5640_i2c_buf[2] = (bdata & 0xFF00)>>8;
   ov5640_i2c_buf[2] = (bdata & 0x00FF);
   
   while ( (ov5640_counter<trytimes) &&(rc != 0) )	
   {
      rc = ov5640_i2c_txdata(saddr, ov5640_i2c_buf, 3);
      if (rc < 0)
      {
              ov5640_counter++;	
              printk(KERN_ERR "--CAMERA-- i2c_write_w failed,i2c addr=0x%x, command addr = 0x%x, val = 0x%x, s=%d, rc=%d!\n",saddr,waddr, bdata,ov5640_counter,rc);
              mdelay(4);
      }
      else 
          {
                //printk(KERN_ERR "--CAMERA--i2c_write_w ok!\n");
          }
   }
   return rc;
}


static void ov5640_whitebalance_setting(struct v4l2_subdev *sd, int value)
{
	int err, i;
	int num;
	
	printk("  ov5640_whitebalance_setting!!!\n");

	switch (value) {
	case 1:		
		num = ((sizeof(ov5640_whitebalance_auto)/sizeof(ov5640_whitebalance_auto[0])));			
		for (i = 0; i < num ; i++) {
			err =hm2055_i2c_write(sd, ov5640_whitebalance_auto[i], 3);
		}
	break;
	case 2:		
		num = ((sizeof(ov5640_whitebalance_incandescence)/sizeof(ov5640_whitebalance_incandescence[0])));			
		for (i = 0; i < num ; i++) {
			err =hm2055_i2c_write(sd, ov5640_whitebalance_incandescence[i], 3);
		}
	break;
	case 3:		
		num = ((sizeof(ov5640_whitebalance_daylight)/sizeof(ov5640_whitebalance_daylight[0])));			
		for (i = 0; i < num ; i++) {
			err =hm2055_i2c_write(sd, ov5640_whitebalance_daylight[i], 3);
		}
	break;
	case 4:		
		num = ((sizeof(ov5640_whitebalance_fluorescent)/sizeof(ov5640_whitebalance_fluorescent[0])));			
		for (i = 0; i < num ; i++) {
			err =hm2055_i2c_write(sd, ov5640_whitebalance_fluorescent[i], 3);
		}
	break;
	case 5:	
		num = ((sizeof(ov5640_whitebalance_cloud)/sizeof(ov5640_whitebalance_cloud[0])));			
		for (i = 0; i < num ; i++) {
			err =hm2055_i2c_write(sd, ov5640_whitebalance_cloud[i], 3);
		}
	break;
	}
	
	//mdelay(200);
	mdelay(50);
	
}

static void ov5640_exposure(struct v4l2_subdev *sd, int which)
{
	int err, i;
	int num;
	printk("%s  %d\n",__func__,which);
	
	switch (which) {
		case 0:
		num = ((sizeof(ov5640_exposure_0)/sizeof(ov5640_exposure_0[0])));			
		for (i = 0; i < num ; i++) {
			err =hm2055_i2c_write(sd, ov5640_exposure_0[i], 3);
		}
		break;
		case 2:
		num = ((sizeof(ov5640_exposure_2)/sizeof(ov5640_exposure_2[0])));			
		for (i = 0; i < num ; i++) {
			err =hm2055_i2c_write(sd, ov5640_exposure_2[i], 3);
		}
		break;
		case 4:
		num = ((sizeof(ov5640_exposure_4)/sizeof(ov5640_exposure_4[0])));			
		for (i = 0; i < num ; i++) {
			err =hm2055_i2c_write(sd, ov5640_exposure_4[i], 3);
		}
		break;
		case 6:
		num = ((sizeof(ov5640_exposure_6)/sizeof(ov5640_exposure_6[0])));			
		for (i = 0; i < num ; i++) {
			err =hm2055_i2c_write(sd, ov5640_exposure_6[i], 3);
		}
		break;
		case 8:
		num = ((sizeof(ov5640_exposure_8)/sizeof(ov5640_exposure_8[0])));			
		for (i = 0; i < num ; i++) {
			err =hm2055_i2c_write(sd, ov5640_exposure_8[i], 3);
		}
		break;
	}
}


static void ov5640_auto_focus(struct v4l2_subdev *sd)
	{
//		unsigned char ctrlreg[1][3] = {0x30, 0x22, 0x03};
//		unsigned char readreg[1][2] = {0x30, 0x22};
		int data;
		printk("%s\n",__func__);
//		hm2055_i2c_write(sd, ctrlreg[0], 3);

		ov5640_i2c_write(0x3c, 0x3022, 0x03, 10);
		ov5640_i2c_write(0x3c, 0x3022, 0x03, 10);
/*
		ov5640_i2c_read(0x3c, 0x3808, &data);
		printk("0x3808 = 0x%x\n",data);
		ov5640_i2c_read(0x3c, 0x3029, &data);
		printk("0x3029 = 0x%x\n",data);
		ov5640_i2c_read_byte(0x3c, 0x3808, &data);
		printk("data = 0x%x\n",data);
*/
		
		mdelay(200);
	}


#endif

#endif

static int ut2055_mem_write(struct v4l2_subdev *sd, u8 cmd, u16 len, u32 addr, u8 *val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg;
	unsigned char data[len + 8];
	int i, err = 0;

	if (!client->adapter)
		return -ENODEV;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = sizeof(data);
	msg.buf = data;

	/* high byte goes out first */
	data[0] = 0x00;
	data[1] = cmd;
	data[2] = (addr >> 24) & 0xFF;
	data[3] = (addr >> 16) & 0xFF;
	data[4] = (addr >> 8) & 0xFF;
	data[5] = addr & 0xFF;
	data[6] = (len >> 8) & 0xFF;
	data[7] = len & 0xFF;
	memcpy(data + 2 + sizeof(addr) + sizeof(len), val, len);

	cam_i2c_dbg("address %#x, length %d\n", addr, len);

	for (i = UT2055_I2C_RETRY; i; i--) {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			break;
		mdelay(20);
	}

	return err;
}

static irqreturn_t ut2055_isp_isr(int irq, void *dev_id)
{
	struct v4l2_subdev *sd = (struct v4l2_subdev *)dev_id;
	struct ut2055_state *state = to_state(sd);

	cam_dbg("**************** interrupt ****************\n");
	state->isp.issued = 1;
	wake_up_interruptible(&state->isp.wait);

	return IRQ_HANDLED;
}

static u32 ut2055_wait_interrupt(struct v4l2_subdev *sd,
	unsigned int timeout)
{
#if 0
	struct ut2055_state *state = to_state(sd);
	cam_trace("E\n");

	if (wait_event_interruptible_timeout(state->isp.wait,
		state->isp.issued == 1,
		msecs_to_jiffies(timeout)) == 0) {
		cam_err("timeout\n");
		return 0;
	}

	state->isp.issued = 0;

	ut2055_readb(sd, UT2055_CATEGORY_SYS,
		UT2055_SYS_INT_FACTOR, &state->isp.int_factor);

	cam_trace("X\n");
	return state->isp.int_factor;
#endif
	return 1;
}

static int ut2055_set_mode(struct v4l2_subdev *sd, u32 mode)
{
	int i, err;
	u32 old_mode, val;
	cam_trace("E\n");

/*
	if (old_mode == mode) {
		cam_dbg("%#x -> %#x\n", old_mode, mode);
		return old_mode;
	}
*/
//	cam_dbg("%#x -> %#x\n", old_mode, mode);
	
	switch (/*old_mode*/1) {
	case UT2055_SYSINIT_MODE:
		cam_warn("sensor is initializing\n");
		err = -EBUSY;
		break;

	case UT2055_PARMSET_MODE:
	case UT2055_MONITOR_MODE:
	case UT2055_STILLCAP_MODE:
		break;

	default:
//		cam_warn("current mode is unknown, %d\n", old_mode);
		err = -EINVAL;
	}



	cam_trace("X\n");
	return /*old_mode*/0;
}

/*
 * v4l2_subdev_core_ops
 */
static int ut2055_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ut2055_ctrls); i++) {
		if (qc->id == ut2055_ctrls[i].id) {
			qc->maximum = ut2055_ctrls[i].maximum;
			qc->minimum = ut2055_ctrls[i].minimum;
			qc->step = ut2055_ctrls[i].step;
			qc->default_value = ut2055_ctrls[i].default_value;
			return 0;
		}
	}

	return -EINVAL;
}

static int ut2055_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct ut2055_state *state = to_state(sd);
	int err = 0;
	#ifdef CONTINUOUS_FOCUS
	int data,af_pos_h,af_pos_l;
	#endif
	switch (ctrl->id) {
	case V4L2_CID_CAMERA_AUTO_FOCUS_RESULT:
		if (ghm5065)
			{
				#ifdef CONTINUOUS_FOCUS
				ov5640_i2c_write(0x1f,0x070a, 0x01, 2);	
				mdelay(70);	
				ov5640_i2c_read_byte(0x1f, 0x07ae, &data);
				if(data==1){	
					ctrl->value = 2;
					ov5640_i2c_read_byte(0x1f, 0x06F0, &af_pos_h);
					ov5640_i2c_read_byte(0x1f, 0x06F1, &af_pos_l);
			  		ov5640_i2c_write(0x1f,0x0700, af_pos_h & 0xff, 10); 
			  		ov5640_i2c_write(0x1f,0x0701, af_pos_l & 0xff, 10); 
					ov5640_i2c_write(0x1f,0x070a, 0x00, 2);
					printk("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
				}else
					ctrl->value = 0;
				#else
				ctrl->value = state->focus.status;
				#endif
			}
		else
			{
				ctrl->value = 2;
			}
		
		break;

	case V4L2_CID_CAM_JPEG_MEMSIZE:
		ctrl->value = UT2055_JPEG_MAXSIZE +
			UT2055_THUMB_MAXSIZE + UT2055_POST_MAXSIZE;
		break;

	case V4L2_CID_CAM_JPEG_MAIN_SIZE:
		ctrl->value = state->jpeg.main_size;
		break;

	case V4L2_CID_CAM_JPEG_MAIN_OFFSET:
		ctrl->value = state->jpeg.main_offset;
		break;

	case V4L2_CID_CAM_JPEG_THUMB_SIZE:
		ctrl->value = state->jpeg.thumb_size;
		break;

	case V4L2_CID_CAM_JPEG_THUMB_OFFSET:
		ctrl->value = state->jpeg.thumb_offset;
		break;

	case V4L2_CID_CAM_JPEG_POSTVIEW_OFFSET:
		ctrl->value = state->jpeg.postview_offset;
		break;

	case V4L2_CID_CAMERA_EXIF_FLASH:
		ctrl->value = state->exif.flash;
		break;

	case V4L2_CID_CAMERA_EXIF_ISO:
		ctrl->value = state->exif.iso;
		break;

	case V4L2_CID_CAMERA_EXIF_TV:
		ctrl->value = state->exif.tv;
		break;

	case V4L2_CID_CAMERA_EXIF_BV:
		ctrl->value = state->exif.bv;
		break;

	case V4L2_CID_CAMERA_EXIF_EBV:
		ctrl->value = state->exif.ebv;
		break;	

	case V4L2_CID_CAMERA_MODEL:
		ctrl->value = g_cam_model;
		break;

	default:
		cam_err("no such control id %d\n",
				ctrl->id - V4L2_CID_PRIVATE_BASE);
		/*err = -ENOIOCTLCMD*/
		err = 0;
		break;
	}

	if (err < 0 && err != -ENOIOCTLCMD)
		cam_err("failed, id %d\n", ctrl->id - V4L2_CID_PRIVATE_BASE);

	return err;
}

#ifdef CONFIG_TARGET_LOCALE_KOR
static int ut2055_set_antibanding(struct v4l2_subdev *sd, int val)
{
	int antibanding = 0x02;	/* Fix 60Hz for domastic */
	int err = 0;

	cam_dbg("E, value %d\n", val);

	antibanding = val;

	cam_trace("X\n");
	return err;
}
#endif

static int ut2055_set_af_softlanding(struct v4l2_subdev *sd)
{
	struct ut2055_state *state = to_state(sd);
	u32 status = 0;
	int i, err = 0;

	cam_trace("E\n");
#if 0
	if (unlikely(state->isp.bad_fw)) {
		cam_err("\"Unknown\" state, please update F/W");
		return -ENOSYS;
	}

	err = ut2055_set_mode(sd, UT2055_MONITOR_MODE);
	if (err <= 0) {
		cam_err("failed to set mode\n");
		return err;
	}


	for (i = UT2055_I2C_VERIFY; i; i--) {
		mdelay(10);

		if ((status & 0x01) == 0x00)
			break;
	}

	if ((status & 0x01) != 0x00) {
		cam_err("failed\n");
		return -ETIMEDOUT;
	}

	cam_trace("X\n");
	return err;
#endif
	return 0;
}

static int ut2055_dump_fw(struct v4l2_subdev *sd)
{
	struct file *fp;
	mm_segment_t old_fs;
	u8 *buf, val;
	u32 addr, unit, count, intram_unit = 0x1000;
	int i, j, err;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(UT2055_FW_DUMP_PATH,
		O_WRONLY|O_CREAT|O_TRUNC, S_IRUGO|S_IWUGO|S_IXUSR);
	if (IS_ERR(fp)) {
		cam_err("failed to open %s, err %ld\n",
			UT2055_FW_DUMP_PATH, PTR_ERR(fp));
		err = -ENOENT;
		goto out0;
	}

	buf = kmalloc(intram_unit, GFP_KERNEL);
	if (!buf) {
		cam_err("failed to allocate memory\n");
		err = -ENOMEM;
		goto out0;
	}

	cam_dbg("start, file path %s\n", UT2055_FW_DUMP_PATH);

	/* set pin */
	val = 0x7E;
	err = ut2055_mem_write(sd, 0x04, sizeof(val), 0x50000308, &val);
	if (err < 0) {
		cam_err("i2c falied, err %d\n", err);
		goto out1;
	}

	addr = UT2055_FLASH_BASE_ADDR;
	unit = SZ_64K;
	count = 31;
	for (i = 0; i < count; i++) {
		for (j = 0; j < unit; j += intram_unit) {
			err = ut2055_mem_read(sd,
				intram_unit, addr + (i * unit) + j, buf);
			if (err < 0) {
				cam_err("i2c falied, err %d\n", err);
				goto out1;
			}
			vfs_write(fp, buf, intram_unit, &fp->f_pos);
		}
	}

	addr = UT2055_FLASH_BASE_ADDR + SZ_64K * count;
	unit = SZ_8K;
	count = 4;
	for (i = 0; i < count; i++) {
		for (j = 0; j < unit; j += intram_unit) {
			err = ut2055_mem_read(sd,
				intram_unit, addr + (i * unit) + j, buf);
			if (err < 0) {
				cam_err("i2c falied, err %d\n", err);
				goto out1;
			}
			vfs_write(fp, buf, intram_unit, &fp->f_pos);
		}
	}

	cam_dbg("end\n");
out1:
	kfree(buf);
out0:
	if (!IS_ERR(fp))
		filp_close(fp, current->files);
	set_fs(old_fs);

	return err;
}

static int ut2055_get_sensor_fw_version(struct v4l2_subdev *sd,
	char *buf)
{
	u8 val;
	int err;

	/* set pin */
	val = 0x7E;
	err = ut2055_mem_write(sd, 0x04, sizeof(val), 0x50000308, &val);
	CHECK_ERR(err);

	err = ut2055_mem_read(sd, UT2055_FW_VER_LEN,
		UT2055_FLASH_BASE_ADDR + UT2055_FW_VER_FILE_CUR, buf);

	cam_dbg("%s\n", buf);
	return 0;
}

static int ut2055_get_phone_fw_version(struct v4l2_subdev *sd,
	char *buf)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct device *dev = &client->adapter->dev;
	u8 sensor_ver[UT2055_FW_VER_LEN] = {0, };
	const struct firmware *fentry;
	int err;

#ifdef SDCARD_FW
	struct file *fp;
	mm_segment_t old_fs;
	long nread;
	int fw_requested = 1;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(UT2055_FW_PATH, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		cam_trace("failed to open %s, err %ld\n", UT2055_FW_PATH, PTR_ERR(fp));
		goto request_fw;
	}

	fw_requested = 0;
	err = vfs_llseek(fp, UT2055_FW_VER_FILE_CUR, SEEK_SET);
	if (err < 0) {
		cam_warn("failed to fseek, %d\n", err);
		goto out;
	}

	nread = vfs_read(fp, (char __user *)buf, UT2055_FW_VER_LEN, &fp->f_pos);
	if (nread != UT2055_FW_VER_LEN) {
		cam_err("failed to read firmware file, %ld Bytes\n", nread);
		err = -EIO;
		goto out;
	}

request_fw:
	if (fw_requested) {
		set_fs(old_fs);
#endif /* SDCARD_FW */
	ut2055_get_sensor_fw_version(sd, sensor_ver);

	if (sensor_ver[0] == 'T')
		err = request_firmware(&fentry, UT2055T_FW_REQUEST_PATH, dev);
	else
		err = request_firmware(&fentry, UT2055O_FW_REQUEST_PATH, dev);

	if (err != 0) {
		cam_err("request_firmware falied\n");
		err = -EINVAL;
		goto out;
	}

	memcpy(buf, (u8 *)&fentry->data[UT2055_FW_VER_FILE_CUR], UT2055_FW_VER_LEN);
#ifdef SDCARD_FW
	}
#endif /* SDCARD_FW */

out:
#ifdef SDCARD_FW
	if (!fw_requested) {
		filp_close(fp, current->files);
		set_fs(old_fs);
	}
#endif  /* SDCARD_FW */

	cam_dbg("%s\n", buf);
	return 0;
}

static int ut2055_check_fw(struct v4l2_subdev *sd)
{
	struct ut2055_state *state = to_state(sd);
//	u8 sensor_ver[UT2055_FW_VER_LEN] = "FAILED Fujitsu UT2055LS";
//	u8 phone_ver[UT2055_FW_VER_LEN] = "FAILED Fujitsu UT2055LS";
	unsigned char sensor_ver[UT2055_FW_VER_LEN] = "FAILED Fujitsu UT2055LS";
	unsigned char phone_ver[UT2055_FW_VER_LEN] = "FAILED Fujitsu UT2055LS";
	int af_cal_h = 0, af_cal_l = 0;
	int rg_cal_h = 0, rg_cal_l = 0;
	int bg_cal_h = 0, bg_cal_l = 0;
	int update_count = 0;
	u32 int_factor;
	int err;

	cam_trace("E\n");

	/* F/W version */
	ut2055_get_phone_fw_version(sd, phone_ver);

	if (state->isp.bad_fw)
		goto out;

	ut2055_get_sensor_fw_version(sd, sensor_ver);

#if 0
	int_factor = ut2055_wait_interrupt(sd, UT2055_ISP_TIMEOUT);
	if (!(int_factor & UT2055_INT_MODE)) {
		cam_err("firmware was erased?\n");
		return -ETIMEDOUT;
	}
#endif

out:
	if (!state->fw_version) {
		state->fw_version = kzalloc(50, GFP_KERNEL);
		if (!state->fw_version) {
			cam_err("no memory for F/W version\n");
			return -ENOMEM;
		}
	}

	sprintf(state->fw_version, "%s %s %d %x %x %x %x %x %x",
		sensor_ver, phone_ver, update_count,
		af_cal_h, af_cal_l, rg_cal_h, rg_cal_l, bg_cal_h, bg_cal_l);

	cam_trace("X\n");
	return 0;
}

static int ut2055_set_sensor_mode(struct v4l2_subdev *sd, int val)
{
	struct ut2055_state *state = to_state(sd);
	int err;
	cam_dbg("E, value %d\n", val);

	err = ut2055_set_mode(sd, UT2055_PARMSET_MODE);
	CHECK_ERR(err);


	state->sensor_mode = val;

	cam_trace("X\n");
	return 0;
}

static int ut2055_set_flash(struct v4l2_subdev *sd, int val, int recording)
{
	struct ut2055_state *state = to_state(sd);
	int light, flash;
	int err;
	cam_dbg("E, value %d\n", val);

	//return 0;

	if (!recording)
		state->flash_mode = val;
	return 0;

	/* movie flash mode should be set when recording is started */
	if (state->sensor_mode == SENSOR_MOVIE && !recording)
		return 0;

retry:
	switch (val) {
	case FLASH_MODE_OFF:
		light = 0x00;
		flash = (state->sensor_mode == SENSOR_CAMERA) ? 0x00 : -1;
		break;

	case FLASH_MODE_AUTO:
		light = (state->sensor_mode == SENSOR_CAMERA) ? 0x02 : 0x04;
		flash = (state->sensor_mode == SENSOR_CAMERA) ? 0x02 : -1;
		break;

	case FLASH_MODE_ON:
		light = (state->sensor_mode == SENSOR_CAMERA) ? 0x01 : 0x03;
		flash = (state->sensor_mode == SENSOR_CAMERA) ? 0x01 : -1;
		break;

	case FLASH_MODE_TORCH:
		light = 0x03;
		flash = -1;
		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = FLASH_MODE_OFF;
		goto retry;
	}

	if (light >= 0) {
	}

	if (flash >= 0) {
	}

	cam_trace("X\n");
	return 0;
}

static int ut2055_set_iso(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct v4l2_queryctrl qc = {0,};
	int val = ctrl->value, err;
	u32 iso[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
	cam_dbg("E, value %d\n", val);

	qc.id = ctrl->id;
	ut2055_queryctrl(sd, &qc);

	if (val < qc.minimum || val > qc.maximum) {
		cam_warn("invalied value, %d\n", val);
		val = qc.default_value;
	}

	val -= qc.minimum;


	cam_trace("X\n");
	return 0;
}

static int ut2055_set_metering(struct v4l2_subdev *sd, int val)
{
	int err;
	cam_dbg("E, value %d\n", val);

retry:
	switch (val) {
	case METERING_CENTER:
		break;
	case METERING_SPOT:
		break;
	case METERING_MATRIX:
		break;
	default:
		cam_warn("invalid value, %d\n", val);
		val = METERING_CENTER;
		goto retry;
	}

	cam_trace("X\n");
	return 0;
}

static int ut2055_set_exposure(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	int err, i;
	int num;
	int val = ctrl->value;
	gclient = v4l2_get_subdevdata(sd);
	if(gclient->addr==0x24){
		printk("  ut2057_set_exposure val=%d gclient->addr=%x\n",val,gclient->addr);	
		switch (val) {
			case -4:
			num = ((sizeof(hm2057_exposure_0)/sizeof(hm2057_exposure_0[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, hm2057_exposure_0[i], 3);
			}
			break;
			case -2:
			num = ((sizeof(hm2057_exposure_2)/sizeof(hm2057_exposure_2[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, hm2057_exposure_2[i], 3);
			}
			break;
			case 0:
			num = ((sizeof(hm2057_exposure_4)/sizeof(hm2057_exposure_4[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, hm2057_exposure_4[i], 3);
			}
			break;
			case 2:
			num = ((sizeof(hm2057_exposure_6)/sizeof(hm2057_exposure_6[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, hm2057_exposure_6[i], 3);
			}
			break;
			case 4:
			num = ((sizeof(hm2057_exposure_8)/sizeof(hm2057_exposure_8[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, hm2057_exposure_8[i], 3);
			}
			break;
		}
	}
	else if(gclient->addr==0x31){
		printk("  gc0329_set_exposure val=%d gclient->addr=%x\n",val,gclient->addr);	
		switch (val) {
			case -4:
			num = ((sizeof(gc0329_ev_m2)/sizeof(gc0329_ev_m2[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, gc0329_ev_m2[i], 2);
			}
			break;
			case -2:
			num = ((sizeof(gc0329_ev_m1)/sizeof(gc0329_ev_m1[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, gc0329_ev_m1[i], 2);
			}
			break;
			case 0:
			num = ((sizeof(gc0329_ev_default)/sizeof(gc0329_ev_default[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, gc0329_ev_default[i], 2);
			}
			break;
			case 2:
			num = ((sizeof(gc0329_ev_p1)/sizeof(gc0329_ev_p1[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, gc0329_ev_p1[i], 2);
			}
			break;
			case 4:
			num = ((sizeof(gc0329_ev_p2)/sizeof(gc0329_ev_p2[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, gc0329_ev_p2[i], 2);
			}
			break;
		}
	}
	else if(gclient->addr==0x1f){
		printk("  hm5065_set_exposure val=%d gclient->addr=%x\n",val,gclient->addr);	
		switch (val) {
			case -4:
			num = ((sizeof(hm5065_exposure_0)/sizeof(hm5065_exposure_0[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, hm5065_exposure_0[i], 3);
			}
			break;
			case -2:
			num = ((sizeof(hm5065_exposure_2)/sizeof(hm5065_exposure_2[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, hm5065_exposure_2[i], 3);
			}
			break;
			case 0:
			num = ((sizeof(hm5065_exposure_4)/sizeof(hm5065_exposure_4[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, hm5065_exposure_4[i], 3);
			}
			break;
			case 2:
			num = ((sizeof(hm5065_exposure_6)/sizeof(hm5065_exposure_6[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, hm5065_exposure_6[i], 3);
			}
			break;
			case 4:
			num = ((sizeof(hm5065_exposure_8)/sizeof(hm5065_exposure_8[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, hm5065_exposure_8[i], 3);
			}
			break;
		}
	}	
	return 0;
}

static int ut2055_set_whitebalance(struct v4l2_subdev *sd, int val)
{
	int err, i;
	int num;
	unsigned char ctrlreg[1][2];
	gclient = v4l2_get_subdevdata(sd);

	if(gclient->addr==0x24){
		printk("  hm2055_white_balance_preset val=%d gclient->addr=%x\n",val,gclient->addr);
		// normal,grayscale,sepia,sepia_green,sepia_blue,color_inv,gray_inv,embossment,sketch

		switch (val) {
		case 1: 	
			num = ((sizeof(hm2057_white_balance_auto)/sizeof(hm2057_white_balance_auto[0])));			
			for (i = 0; i < num ; i++) {
				err=hm2055_i2c_write(sd, hm2057_white_balance_auto[i], 3);
			}
			
		break;
		case 4: 	

			num = ((sizeof(hm2057_white_balance_incandescence)/
							sizeof(hm2057_white_balance_incandescence[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, hm2057_white_balance_incandescence[i], 3);
			}
		break;
		case 2: 	
			num = ((sizeof(hm2057_white_balance_daylight)/sizeof(hm2057_white_balance_daylight[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, hm2057_white_balance_daylight[i], 3);
			}
		break;
		case 5: 	
			
			num = ((sizeof(hm2057_white_balance_fluorescent)/sizeof(hm2057_white_balance_fluorescent[0]))); 		
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, hm2057_white_balance_fluorescent[i], 3);
			}
		break;
		case 3: 
			num = ((sizeof(hm2057_white_balance_cloud)/sizeof(hm2057_white_balance_cloud[0]))); 		
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, hm2057_white_balance_cloud[i], 3);
			}
		break;
		}
	}
	else if(gclient->addr==0x31){
		printk("  gc0329_white_balance_preset val=%d gclient->addr=%x\n",val,gclient->addr);
		// normal,grayscale,sepia,sepia_green,sepia_blue,color_inv,gray_inv,embossment,sketch

		num = ((sizeof(gc0329_ev_m1)/sizeof(gc0329_ev_m1[0]))); 		
		for (i = 0; i < num ; i++) {
			err =hm2055_i2c_write(sd, gc0329_ev_m1[i], 2);
		}

		num = ((sizeof(gc0329_ev_default)/sizeof(gc0329_ev_default[0])));			
		for (i = 0; i < num ; i++) {
			err =hm2055_i2c_write(sd, gc0329_ev_default[i], 2);
		}

		switch (val) {
		case 1: 	
			num = ((sizeof(gc0329_awb_on)/sizeof(gc0329_awb_on[0])));			
			for (i = 0; i < num ; i++) {
				err=hm2055_i2c_write(sd, gc0329_awb_on[i], 2);
			}
			
		break;
		case 4: 	

			num = ((sizeof(gc0329_wb_tungsten)/
							sizeof(gc0329_wb_tungsten[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, gc0329_wb_tungsten[i], 2);
			}
		break;
		case 2: 	
			num = ((sizeof(gc0329_wb_sunny)/sizeof(gc0329_wb_sunny[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, gc0329_wb_sunny[i], 2);
			}
		break;
		case 5: 	
			
			num = ((sizeof(gc0329_wb_fluorescent)/sizeof(gc0329_wb_fluorescent[0]))); 		
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, gc0329_wb_fluorescent[i], 2);
			}
		break;
		case 3: 
			num = ((sizeof(gc0329_wb_cloudy)/sizeof(gc0329_wb_cloudy[0]))); 		
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, gc0329_wb_cloudy[i], 2);
			}
		break;
		}
	}
	else if(gclient->addr==0x1f){
		printk("  hm5065_white_balance_preset val=%d gclient->addr=%x\n",val,gclient->addr);
		// normal,grayscale,sepia,sepia_green,sepia_blue,color_inv,gray_inv,embossment,sketch

		switch (val) {
		case 1: 	
			num = ((sizeof(hm5065_whitebalance_auto)/sizeof(hm5065_whitebalance_auto[0])));			
			for (i = 0; i < num ; i++) {
				err=hm2055_i2c_write(sd, hm5065_whitebalance_auto[i], 3);
			}
			
		break;
		case 4: 	

			num = ((sizeof(hm5065_whitebalance_incandescence)/
							sizeof(hm5065_whitebalance_incandescence[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, hm5065_whitebalance_incandescence[i], 3);
			}
		break;
		case 2: 	
			num = ((sizeof(hm5065_whitebalance_daylight)/sizeof(hm5065_whitebalance_daylight[0])));			
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, hm5065_whitebalance_daylight[i], 3);
			}
		break;
		case 5: 	
			
			num = ((sizeof(hm5065_whitebalance_fluorescent)/sizeof(hm5065_whitebalance_fluorescent[0]))); 		
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, hm5065_whitebalance_fluorescent[i], 3);
			}
		break;
		case 3: 
			num = ((sizeof(hm5065_whitebalance_cloud)/sizeof(hm5065_whitebalance_cloud[0]))); 		
			for (i = 0; i < num ; i++) {
				err =hm2055_i2c_write(sd, hm5065_whitebalance_cloud[i], 3);
			}
		break;
		}
	}	
	mdelay(200);
	return 0;
}

static int ut2055_set_sharpness(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct v4l2_queryctrl qc = {0,};
	int val = ctrl->value, err;
	u32 sharpness[] = {0x03, 0x04, 0x05, 0x06, 0x07};
	cam_dbg("E, value %d\n", val);

	qc.id = ctrl->id;
	ut2055_queryctrl(sd, &qc);

	if (val < qc.minimum || val > qc.maximum) {
		cam_warn("invalied value, %d\n", val);
		val = qc.default_value;
	}

	val -= qc.minimum;


	cam_trace("X\n");
	return 0;
}

static int ut2055_set_saturation(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	struct v4l2_queryctrl qc = {0,};
	int val = ctrl->value, err;
	u32 saturation[] = {0x01, 0x02, 0x03, 0x04, 0x05};
	cam_dbg("E, value %d\n", val);

	qc.id = ctrl->id;
	ut2055_queryctrl(sd, &qc);

	if (val < qc.minimum || val > qc.maximum) {
		cam_warn("invalied value, %d\n", val);
		val = qc.default_value;
	}

	val -= qc.minimum;


	cam_trace("X\n");
	return 0;
}

static int ut2055_set_scene_mode(struct v4l2_subdev *sd, int val)
{
	struct v4l2_control ctrl;
	int evp, iso, brightness, whitebalance, sharpness, saturation;
	int err;
	cam_dbg("E, value %d\n", val);

	iso = ISO_AUTO;
	brightness = EV_DEFAULT;
	whitebalance = WHITE_BALANCE_AUTO;
	sharpness = SHARPNESS_DEFAULT;
	saturation = CONTRAST_DEFAULT;

retry:
	switch (val) {
	case SCENE_MODE_NONE:
		evp = 0x00;
		break;

	case SCENE_MODE_PORTRAIT:
		evp = 0x01;
		sharpness = SHARPNESS_MINUS_1;
		break;

	case SCENE_MODE_LANDSCAPE:
		evp = 0x02;
		sharpness = SHARPNESS_PLUS_1;
		saturation = SATURATION_PLUS_1;
		break;

	case SCENE_MODE_SPORTS:
		evp = 0x03;
		break;

	case SCENE_MODE_PARTY_INDOOR:
		evp = 0x04;
		/*iso = ISO_200; sensor will set internally */
		saturation = SATURATION_PLUS_1;
		break;

	case SCENE_MODE_BEACH_SNOW:
		evp = 0x05;
		/*iso = ISO_50; sensor will set internally */
		brightness = EV_PLUS_2;
		saturation = SATURATION_PLUS_1;
		break;

	case SCENE_MODE_SUNSET:
		evp = 0x06;
		whitebalance = WHITE_BALANCE_SUNNY;
		break;

	case SCENE_MODE_DUSK_DAWN:
		evp = 0x07;
		whitebalance = WHITE_BALANCE_FLUORESCENT;
		break;

	case SCENE_MODE_FALL_COLOR:
		evp = 0x08;
		saturation = SATURATION_PLUS_2;
		break;

	case SCENE_MODE_NIGHTSHOT:
		evp = 0x09;
		break;

	case SCENE_MODE_BACK_LIGHT:
		evp = 0x0A;
		break;

	case SCENE_MODE_FIREWORKS:
		evp = 0x0B;
		/*iso = ISO_50; sensor will set internally */
		break;

	case SCENE_MODE_TEXT:
		evp = 0x0C;
		sharpness = SHARPNESS_PLUS_2;
		break;

	case SCENE_MODE_CANDLE_LIGHT:
		evp = 0x0D;
		whitebalance = WHITE_BALANCE_SUNNY;
		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = SCENE_MODE_NONE;
		goto retry;
	}

	/* EV-P */

	/* ISO */
	ctrl.id = V4L2_CID_CAMERA_ISO;
	ctrl.value = iso;
	ut2055_set_iso(sd, &ctrl);

	/* EV Bias */
	ctrl.id = V4L2_CID_CAMERA_BRIGHTNESS;
	ctrl.value = brightness;
	ut2055_set_exposure(sd, &ctrl);

	/* AWB */
	ut2055_set_whitebalance(sd, whitebalance);

	/* Chroma Saturation */
	ctrl.id = V4L2_CID_CAMERA_SATURATION;
	ctrl.value = saturation;
	ut2055_set_saturation(sd, &ctrl);

	/* Sharpness */
	ctrl.id = V4L2_CID_CAMERA_SHARPNESS;
	ctrl.value = sharpness;
	ut2055_set_sharpness(sd, &ctrl);

	/* Emotional Color */

	cam_trace("X\n");
	return 0;
}

static int ut2055_set_effect_color(struct v4l2_subdev *sd, int val)
{
	u32 int_factor;
	int on, old_mode, cb, cr;
	int err;

	if (/*on*/1)	{
		old_mode = ut2055_set_mode(sd, UT2055_PARMSET_MODE);
		CHECK_ERR(old_mode);


		if (old_mode == UT2055_MONITOR_MODE) {
			err = ut2055_set_mode(sd, old_mode);
			CHECK_ERR(err);
#if 0
			int_factor = ut2055_wait_interrupt(sd, UT2055_ISP_TIMEOUT);
			if (!(int_factor & UT2055_INT_MODE)) {
				cam_err("UT2055_INT_MODE isn't issued, %#x\n",
					int_factor);
				return -ETIMEDOUT;
			}
#endif
			CHECK_ERR(err);
		}
	}

	switch (val) {
	case IMAGE_EFFECT_NONE:
		break;

	case IMAGE_EFFECT_SEPIA:
		cb = 0xD8;
		cr = 0x18;
		break;

	case IMAGE_EFFECT_BNW:
		cb = 0x00;
		cr = 0x00;
		break;
	}


	if (val != IMAGE_EFFECT_NONE) {
	}

	return 0;
}

static int ut2055_set_effect_gamma(struct v4l2_subdev *sd, s32 val)
{
	u32 int_factor;
	int on, effect, old_mode;
	int err;

	if (on) {
	}

	switch (val) {
	case IMAGE_EFFECT_NEGATIVE:
		effect = 0x01;
		break;

	case IMAGE_EFFECT_AQUA:
		effect = 0x08;
		break;
	}

	old_mode = ut2055_set_mode(sd, UT2055_PARMSET_MODE);
	CHECK_ERR(old_mode);


	if (old_mode == UT2055_MONITOR_MODE) {
		err = ut2055_set_mode(sd, old_mode);
		CHECK_ERR(err);
#if 0
		int_factor = ut2055_wait_interrupt(sd, UT2055_ISP_TIMEOUT);
		if (!(int_factor & UT2055_INT_MODE)) {
			cam_err("UT2055_INT_MODE isn't issued, %#x\n",
				int_factor);
			return -ETIMEDOUT;
		}
#endif		
		CHECK_ERR(err);
	}

	return err;
}

static int ut2055_set_effect(struct v4l2_subdev *sd, int val)
{
	int err;
	cam_dbg("E, value %d\n", val);

retry:
	switch (val) {
	case IMAGE_EFFECT_NONE:
	case IMAGE_EFFECT_BNW:
	case IMAGE_EFFECT_SEPIA:
		err = ut2055_set_effect_color(sd, val);
		CHECK_ERR(err);
		break;

	case IMAGE_EFFECT_AQUA:
	case IMAGE_EFFECT_NEGATIVE:
		err = ut2055_set_effect_gamma(sd, val);
		CHECK_ERR(err);
		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = IMAGE_EFFECT_NONE;
		goto retry;
	}

	cam_trace("X\n");
	return 0;
}

static int ut2055_set_wdr(struct v4l2_subdev *sd, int val)
{
	int contrast, wdr, err;

	cam_dbg("%s\n", val ? "on" : "off");

	contrast = (val == 1 ? 0x09 : 0x05);
	wdr = (val == 1 ? 0x01 : 0x00);


	cam_trace("X\n");
	return 0;
}

static int ut2055_set_antishake(struct v4l2_subdev *sd, int val)
{
	int ahs, err;

	cam_dbg("%s\n", val ? "on" : "off");

	ahs = (val == 1 ? 0x0E : 0x00);


	cam_trace("X\n");
	return 0;
}

static int ut2055_set_face_beauty(struct v4l2_subdev *sd, int val)
{
	struct ut2055_state *state = to_state(sd);
	int err;

	cam_dbg("%s\n", val ? "on" : "off");


	state->beauty_mode = val;

	cam_trace("X\n");
	return 0;
}

static int ut2055_set_lock(struct v4l2_subdev *sd, int val)
{
	struct ut2055_state *state = to_state(sd);

	cam_trace("%s\n", val ? "on" : "off");


	cam_trace("X\n");
	return 0;
}

static int ut2055_set_af(struct v4l2_subdev *sd, int val)
{
	struct ut2055_state *state = to_state(sd);
	int i, status, err=0;

	cam_info("[vincent]%s, mode %#x\n", val ? "start" : "stop", state->focus.mode);

	state->focus.status = 0;
#if 0
	if (state->focus.mode != FOCUS_MODE_CONTINOUS) {

		if (!(state->focus.touch &&
			state->focus.mode == FOCUS_MODE_TOUCH)) {
			if (val && state->focus.lock) {
				ut2055_set_lock(sd, 0);
				mdelay(100);
			}
			ut2055_set_lock(sd, val);
		}
		printk("==========>ut2055_set_af state=%x\n",state->focus.status);

		/* check AF status for 6 sec */
		for (i = 600; i && err; i--) {
			mdelay(10);

			if (!(status & 0x01))
				err = 0;
		}

//		state->focus.status = status;
	} else {
		printk("==========>ut2055_set_af 11111\n");

		err = -EBUSY;
		//for (i = UT2055_I2C_VERIFY; i && err; i--) {
		//	mdelay(10);

//			if ((val && status == 0x05) || (!val && status != 0x05))
//				err = 0;
		//}
	}
	printk("==========>ut2055_set_af state  end\n");
#endif
	cam_dbg("X\n");
	return err;
}

static int ut2055_set_af_mode(struct v4l2_subdev *sd, int val)
{
	struct ut2055_state *state = to_state(sd);
	struct regulator *movie = regulator_get(NULL, "led_movie");
	u32 cancel, mode, status = 0;
	int i, err;
	printk("======ut2055_set_af_mode=======>val=%d\n",val);
	cancel = val & FOCUS_MODE_DEFAULT;
	val &= 0xFF;
	#ifdef CONTINUOUS_FOCUS
	g_focus_mode=val;
	if(g_focus_mode==3){
		ov5640_i2c_write(0x1f,0x0700, 0x01, 2); 
		ov5640_i2c_write(0x1f,0x0701, 0xFD, 2); 
		ov5640_i2c_write(0x1f,0x070a, 0x00, 2);
	}
	if(g_focus_mode==0){
		ov5640_i2c_write(0x1f,0x070a, 0x00, 2);
		mdelay(50);	
		ov5640_i2c_write(0x1f,0x070a, 0x01, 2);
	}
	#endif
#if 0
retry:
	switch (val) {
	case FOCUS_MODE_AUTO:
		mode = 0x00;
		break;

	case FOCUS_MODE_MACRO:
		mode = 0x01;
		break;

	case FOCUS_MODE_CONTINOUS:
		mode = 0x02;
		cancel = 0;
		break;

	case FOCUS_MODE_FACEDETECT:
		mode = 0x03;
		break;

	case FOCUS_MODE_TOUCH:
		mode = 0x04;
		cancel = 0;
		break;

	case FOCUS_MODE_INFINITY:
		mode = 0x06;
		cancel = 0;
		break;

	default:
		cam_warn("invalid value, %d", val);
		val = FOCUS_MODE_AUTO;
		goto retry;
	}

	if (cancel) {
		ut2055_set_af(sd, 0);
		ut2055_set_lock(sd, 0);
	} else {
		if (state->focus.mode == val)
			return 0;
	}

	cam_dbg("E, value %d\n", val);

	if (val == FOCUS_MODE_FACEDETECT) {
		/* enable face detection */
		mdelay(10);
	} else if (state->focus.mode == FOCUS_MODE_FACEDETECT) {
		/* disable face detection */
	}

	//if (val == FOCUS_MODE_MACRO)
	//	regulator_set_current_limit(movie, 15000, 17000);
	//else if (state->focus.mode == FOCUS_MODE_MACRO)
	//	regulator_set_current_limit(movie, 90000, 110000);

	state->focus.mode = val;

	for (i = UT2055_I2C_VERIFY; i; i--) {
		mdelay(10);

		if (!(status & 0x01))
			break;
	}

	if ((status & 0x01) != 0x00) {
		cam_err("failed\n");
		return -ETIMEDOUT;
	}
#endif
	cam_trace("X\n");
	return 0;
}

static int ut2055_set_touch_auto_focus(struct v4l2_subdev *sd, int val)
{
	struct ut2055_state *state = to_state(sd);
	int err=0;
	cam_info("%s\n", val ? "start" : "stop");

	state->focus.touch = val;
#if 0
	if (val) {
		err = ut2055_set_af_mode(sd, FOCUS_MODE_TOUCH);
		if (err < 0) {
			cam_err("ut2055_set_af_mode failed\n");
			return err;
		}
	}
#endif
	cam_trace("X\n");
	return err;
}

static int ut2055_set_zoom(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct ut2055_state *state = to_state(sd);
	struct v4l2_queryctrl qc = {0,};
	int val = ctrl->value, err;
	int zoom[] = { 1, 2, 3, 5, 6, 7, 9, 10, 11, 13, 14, 15, 17, 18, 19,
		20, 21, 22, 24, 25, 26, 28, 29, 30, 31, 32, 34, 35, 36, 38, 39};
	cam_dbg("E, value %d\n", val);

	qc.id = ctrl->id;
	ut2055_queryctrl(sd, &qc);

	if (val < qc.minimum || val > qc.maximum) {
		cam_warn("invalied value, %d\n", val);
		val = qc.default_value;
	}


	state->zoom = val;

	cam_trace("X\n");
	return 0;
}

static int ut2055_set_jpeg_quality(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	struct v4l2_queryctrl qc = {0,};
	int val = ctrl->value, ratio, err;
	cam_dbg("E, value %d\n", val);

	qc.id = ctrl->id;
	ut2055_queryctrl(sd, &qc);

	if (val < qc.minimum || val > qc.maximum) {
		cam_warn("invalied value, %d\n", val);
		val = qc.default_value;
	}


	if (val <= 65)		/* Normal */
		ratio = 0x0A;
	else if (val <= 75)	/* Fine */
		ratio = 0x05;
	else			/* Superfine */
		ratio = 0x00;


	cam_trace("X\n");
	return 0;
}

static int ut2055_get_exif(struct v4l2_subdev *sd)
{
	struct ut2055_state *state = to_state(sd);
	/* standard values */
	u16 iso_std_values[] = { 10, 12, 16, 20, 25, 32, 40, 50, 64, 80,
		100, 125, 160, 200, 250, 320, 400, 500, 640, 800,
		1000, 1250, 1600, 2000, 2500, 3200, 4000, 5000, 6400, 8000};
	/* quantization table */
	u16 iso_qtable[] = { 11, 14, 17, 22, 28, 35, 44, 56, 71, 89,
		112, 141, 178, 224, 282, 356, 449, 565, 712, 890,
		1122, 1414, 1782, 2245, 2828, 3564, 4490, 5657, 7127, 8909};
	int num, den, i, err;


	return 0;
}

static int ut2055_start_capture(struct v4l2_subdev *sd, int val)
{
	struct ut2055_state *state = to_state(sd);
	int err, int_factor;
	cam_trace("E\n");

	if (!(state->isp.int_factor & UT2055_INT_CAPTURE)) {
#if 0		
		int_factor = ut2055_wait_interrupt(sd,
			state->beauty_mode ? UT2055_ISP_AFB_TIMEOUT : UT2055_ISP_TIMEOUT);
		if (!(int_factor & UT2055_INT_CAPTURE)) {
			cam_warn("UT2055_INT_CAPTURE isn't issued, %#x\n", int_factor);
			return -ETIMEDOUT;
		}
#endif		
	}

#if 0	
	int_factor = ut2055_wait_interrupt(sd, UT2055_ISP_TIMEOUT);
	if (!(int_factor & UT2055_INT_CAPTURE)) {
		cam_warn("UT2055_INT_CAPTURE isn't issued on transfer, %#x\n", int_factor);
		return -ETIMEDOUT;
	}
#endif

	//state->jpeg.main_size = 200;		//vincent
	state->jpeg.main_offset = 0;
	state->jpeg.thumb_offset = UT2055_JPEG_MAXSIZE;
	state->jpeg.postview_offset = UT2055_JPEG_MAXSIZE + UT2055_THUMB_MAXSIZE;

	ut2055_get_exif(sd);

	cam_trace("X\n");
	return /*err*/0;
}

static int ut2055_set_hdr(struct v4l2_subdev *sd, int val)
{
	u32 int_factor;
	int err;
	cam_trace("E\n");

	switch (val) {
	case 0:
		err = ut2055_set_mode(sd, UT2055_MONITOR_MODE);
		CHECK_ERR(err);
#if 0		
		int_factor = ut2055_wait_interrupt(sd, UT2055_ISP_TIMEOUT);
		if (!(int_factor & UT2055_INT_MODE)) {
			cam_err("UT2055_INT_MODE isn't issued, %#x\n",
				int_factor);
			return -ETIMEDOUT;
		}
#endif		
		break;
	case 1:
	case 2:
#if 0		
		int_factor = ut2055_wait_interrupt(sd, UT2055_ISP_TIMEOUT);
#endif		
		break;
	default:
		cam_err("invalid HDR count\n");
	}

	cam_trace("X\n");
	return 0;
}

static int ut2055_check_dataline(struct v4l2_subdev *sd, int val)
{
	int err = 0;

	cam_dbg("E, value %d\n", val);


	cam_trace("X\n");
	return 0;
}

static int ut2055_check_esd(struct v4l2_subdev *sd)
{
	s32 val = 0;
	int err = 0;

	/* check ISP */
	cam_dbg("progress %#x\n", val);

	if (val != 0x80) {
		goto esd_occur;
	} else {
#if 0	
		ut2055_wait_interrupt(sd, UT2055_ISP_ESD_TIMEOUT);
#endif

		if (val & UT2055_INT_ESD)
			goto esd_occur;
	}

	cam_warn("ESD is not detected\n");
	return 0;

esd_occur:
	cam_warn("ESD shock is detected\n");
	return -EIO;
}

static int ut2055_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct ut2055_state *state = to_state(sd);
	int err = 0;

	printk(KERN_INFO "[vincent]id %d [%d], value %d\n",
		ctrl->id - V4L2_CID_PRIVATE_BASE,ctrl->id, ctrl->value);

	if (unlikely(state->isp.bad_fw && ctrl->id != V4L2_CID_CAM_UPDATE_FW)) {
		cam_err("\"Unknown\" state, please update F/W");
		return -ENOSYS;
	}

	switch (ctrl->id) {
	case V4L2_CID_CAM_UPDATE_FW:
		if (ctrl->value == FW_MODE_DUMP)
			err = ut2055_dump_fw(sd);
		else
			err = ut2055_check_fw(sd);
		break;

	case V4L2_CID_CAMERA_SENSOR_MODE:
		err = ut2055_set_sensor_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FLASH_MODE:
		err = ut2055_set_flash(sd, ctrl->value, 0);
		break;

	case V4L2_CID_CAMERA_ISO:
		err = ut2055_set_iso(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_METERING:
		if (state->sensor_mode == SENSOR_CAMERA)
			err = ut2055_set_metering(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_BRIGHTNESS:
		err = ut2055_set_exposure(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_WHITE_BALANCE:
		err = ut2055_set_whitebalance(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_SCENE_MODE:
		err = ut2055_set_scene_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_EFFECT:
		err = ut2055_set_effect(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_WDR:
		err = ut2055_set_wdr(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_ANTI_SHAKE:
		err = ut2055_set_antishake(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_BEAUTY_SHOT:
		err = ut2055_set_face_beauty(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FOCUS_MODE:
		err = ut2055_set_af_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_SET_AUTO_FOCUS:
		printk(KERN_INFO "[ut2055_s_ctrl]id %d, value %d\n",ctrl->id - V4L2_CID_PRIVATE_BASE, ctrl->value);
		err = ut2055_set_af(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_OBJECT_POSITION_X:
		state->focus.pos_x = ctrl->value;
		break;

	case V4L2_CID_CAMERA_OBJECT_POSITION_Y:
		state->focus.pos_y = ctrl->value;
		break;

	case V4L2_CID_CAMERA_TOUCH_AF_START_STOP:
		err = ut2055_set_touch_auto_focus(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_ZOOM:
		err = ut2055_set_zoom(sd, ctrl);
		break;

	case V4L2_CID_CAM_JPEG_QUALITY:
		err = ut2055_set_jpeg_quality(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_CAPTURE:
		err = ut2055_start_capture(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_HDR:
		err = ut2055_set_hdr(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_VT_MODE:
		state->vt_mode = ctrl->value;
		break;

	case V4L2_CID_CAMERA_CHECK_DATALINE:
		state->check_dataline = ctrl->value;
		break;

	case V4L2_CID_CAMERA_CHECK_ESD:
		err = ut2055_check_esd(sd);
		break;

	default:
		cam_err("no such control id %d, value %d\n",
				ctrl->id - V4L2_CID_PRIVATE_BASE, ctrl->value);
		/*err = -ENOIOCTLCMD;*/
		err = 0;
		break;
	}

	if (err < 0 && err != -ENOIOCTLCMD)
		cam_err("failed, id %d, value %d\n",
				ctrl->id - V4L2_CID_PRIVATE_BASE, ctrl->value);
	return err;
}

static int ut2055_g_ext_ctrl(struct v4l2_subdev *sd, struct v4l2_ext_control *ctrl)
{
	struct ut2055_state *state = to_state(sd);
	int err = 0;
	printk("********[ut2055_g_ext_ctrl][ctrl->id=%d] [ctrl->value=%d]\n",ctrl->id,ctrl->value);

	switch (ctrl->id) {
	case V4L2_CID_CAM_SENSOR_FW_VER:
		strcpy(ctrl->string, state->exif.unique_id);
		break;

	default:
		cam_err("no such control id %d\n", ctrl->id - V4L2_CID_CAMERA_CLASS_BASE);
		/*err = -ENOIOCTLCMD*/
		err = 0;
		break;
	}

	if (err < 0 && err != -ENOIOCTLCMD)
		cam_err("failed, id %d\n", ctrl->id - V4L2_CID_CAMERA_CLASS_BASE);

	return err;
}

static int ut2055_g_ext_ctrls(struct v4l2_subdev *sd, struct v4l2_ext_controls *ctrls)
{
	struct v4l2_ext_control *ctrl = ctrls->controls;
	int i, err = 0;

	for (i = 0; i < ctrls->count; i++, ctrl++) {
		err = ut2055_g_ext_ctrl(sd, ctrl);
		printk("########[ut2055_g_ext_ctrls][count=%d][ctrl->value=%d]\n",ctrls->count,ctrl->value);
		if (err) {
			ctrls->error_idx = i;
			break;
		}
	}
	return err;
}

static int ut2055_check_manufacturer_id(struct v4l2_subdev *sd)
{
	int i, err;
	u8 id;
	u32 addr[] = {0x1000AAAA, 0x10005554, 0x1000AAAA};
	u8 val[3][2] = {
		[0] = {0x00, 0xAA},
		[1] = {0x00, 0x55},
		[2] = {0x00, 0x90},
	};
	u8 reset[] = {0x00, 0xF0};

	/* set manufacturer's ID read-mode */
	for (i = 0; i < 3; i++) {
		err = ut2055_mem_write(sd, 0x06, 2, addr[i], val[i]);
		CHECK_ERR(err);
	}

	/* read manufacturer's ID */
	err = ut2055_mem_read(sd, sizeof(id), 0x10000001, &id);
	CHECK_ERR(err);

	/* reset manufacturer's ID read-mode */
	err = ut2055_mem_write(sd, 0x06, sizeof(reset), 0x10000000, reset);
	CHECK_ERR(err);

	cam_dbg("%#x\n", id);

	return id;
}

static int ut2055_program_fw(struct v4l2_subdev *sd,
	u8 *buf, u32 addr, u32 unit, u32 count, u8 id)
{
	u32 val;
	u32 intram_unit = SZ_4K;
	int i, j, retries, err = 0;
	int erase = 0x01;
	if (unit == SZ_64K && id != 0x01)
		erase = 0x04;

	for (i = 0; i < unit*count; i += unit) {
		/* Set Flash ROM memory address */
		
		/* Erase FLASH ROM entire memory */
		/* Response while sector-erase is operating */
		retries = 0;
/*		
		do {
			mdelay(50);
		} while (val == erase && retries++ < UT2055_I2C_VERIFY);

		if (val != 0) {
			cam_err("failed to erase sector\n");
			return -1;
		}
*/
		/* Set FLASH ROM programming size */

		/* Clear M-5MoLS internal RAM */

		/* Set Flash ROM programming address */

		/* Send programmed firmware */
		for (j = 0; j < unit; j += intram_unit) {
			err = ut2055_mem_write(sd, 0x04, intram_unit,
				UT2055_INT_RAM_BASE_ADDR + j, buf + i + j);
			CHECK_ERR(err);
			mdelay(10);
		}

		/* Start Programming */

		/* Confirm programming has been completed */
		retries = 0;
/*		
		do {
			mdelay(50);
		} while (val && retries++ < UT2055_I2C_VERIFY);
*/
//		if (val != 0) {
//			cam_err("failed to program\n");
//			return -1;
//		}
	}

	return 0;
}

static int ut2055_load_fw(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct device *dev = &client->adapter->dev;
	const struct firmware *fentry;
	u8 sensor_ver[UT2055_FW_VER_LEN] = {0, };
	u8 *buf = NULL, val, id;
	int offset, err;

#ifdef SDCARD_FW
	struct file *fp;
	mm_segment_t old_fs;
	long fsize, nread;
	int fw_requested = 1;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(UT2055_FW_PATH, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		cam_trace("failed to open %s, err %ld\n",
			UT2055_FW_PATH, PTR_ERR(fp));
		goto request_fw;
	}

	fw_requested = 0;
	fsize = fp->f_path.dentry->d_inode->i_size;

	cam_dbg("start, file path %s, size %ld Bytes\n", UT2055_FW_PATH, fsize);

	buf = vmalloc(fsize);
	if (!buf) {
		cam_err("failed to allocate memory\n");
		err = -ENOMEM;
		goto out;
	}

	nread = vfs_read(fp, (char __user *)buf, fsize, &fp->f_pos);
	if (nread != fsize) {
		cam_err("failed to read firmware file, %ld Bytes\n", nread);
		err = -EIO;
		goto out;
	}

request_fw:
	if (fw_requested) {
		set_fs(old_fs);
#endif /* SDCARD_FW */
	ut2055_get_sensor_fw_version(sd, sensor_ver);

	if (sensor_ver[0] == 'T')
		err = request_firmware(&fentry, UT2055T_FW_REQUEST_PATH, dev);
	else
		err = request_firmware(&fentry, UT2055O_FW_REQUEST_PATH, dev);

	if (err != 0) {
		cam_err("request_firmware falied\n");
			err = -EINVAL;
			goto out;
	}

	cam_dbg("start, size %d Bytes\n", fentry->size);
	buf = (u8 *)fentry->data;

#ifdef SDCARD_FW
	}
#endif /* SDCARD_FW */

	/* set pin */
	val = 0x7E;
	err = ut2055_mem_write(sd, 0x04, sizeof(val), 0x50000308, &val);
	if (err < 0) {
		cam_err("i2c falied, err %d\n", err);
		goto out;
	}

	id = ut2055_check_manufacturer_id(sd);
	if (id < 0) {
		cam_err("i2c falied, err %d\n", id);
		goto out;
	}

	/* select flash memory */

	/* program FLSH ROM */
	err = ut2055_program_fw(sd, buf, UT2055_FLASH_BASE_ADDR, SZ_64K, 31, id);
	if (err < 0)
		goto out;

	offset = SZ_64K * 31;
	if (id == 0x01) {
		err = ut2055_program_fw(sd,
			buf + offset, UT2055_FLASH_BASE_ADDR + offset, SZ_8K, 4, id);
	} else {
		err = ut2055_program_fw(sd,
			buf + offset, UT2055_FLASH_BASE_ADDR + offset, SZ_4K, 8, id);
	}

	cam_dbg("end\n");

out:
#ifdef SDCARD_FW
	if (!fw_requested) {
		vfree(buf);
		filp_close(fp, current->files);
		set_fs(old_fs);
	}
#endif  /* SDCARD_FW */
	return err;
}

/*
 * v4l2_subdev_video_ops
 */
static const struct ut2055_frmsizeenum *ut2055_get_frmsize
	(const struct ut2055_frmsizeenum *frmsizes, int num_entries, int index)
{
	int i;

	for (i = 0; i < num_entries; i++) {
		if (frmsizes[i].index == index)
			return &frmsizes[i];
	}

	return NULL;
}

static int ut2055_set_frmsize(struct v4l2_subdev *sd)
{
	struct ut2055_state *state = to_state(sd);
	struct v4l2_control ctrl;
	int err;
	u32 old_mode;
	cam_trace("E\n");

	if (state->format_mode != V4L2_PIX_FMT_MODE_CAPTURE) {
		err = ut2055_set_mode(sd, UT2055_PARMSET_MODE);

		CHECK_ERR(err);


		if (state->zoom) {
			/* Zoom position returns to 1 when the monitor size is changed. */
			ctrl.id = V4L2_CID_CAMERA_ZOOM;
			ctrl.value = state->zoom;
			ut2055_set_zoom(sd, &ctrl);
		}

		cam_info("preview frame size %dx%d\n",
			state->preview->width, state->preview->height);
	} else {
		cam_info("capture frame size %dx%d\n",
			state->capture->width, state->capture->height);
	}

	cam_trace("X\n");
	return 0;
}
static int ut2055_enum_framesizes(struct v4l2_subdev *sd,
	struct v4l2_frmsizeenum *fsize);
static void hm5065_single_focus(struct v4l2_subdev *sd);

static int ut2055_s_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *ffmt)
{
	struct ut2055_state *state = to_state(sd);
	const struct ut2055_frmsizeenum **frmsize;
	struct v4l2_frmsizeenum cam_frmsize;

//add vincent
#if 1
	int err = 0,data,data1,data2;
	unsigned int ret = 0;
	u32 width = ffmt->width;
	u32 height = ffmt->height;
	u32 tmp_width;
	u32 old_index;
	int i, num_entries;
	cam_frmsize.discrete.width = 0;
	cam_frmsize.discrete.height = 0;
	cam_trace("E\n");
	printk("=========================== ut2055_s_fmt %d =================================\n",ffmt->field);
	
	if(width==1280 && cam_sw_flag==0)	
	{
		cam_sw_flag=1;
		cam_sw_720p=1;
	}
	if(width==640 && cam_sw_flag==1)	
	{
		cam_sw_vga=1;
		cam_sw_flag=0;
	}
	printk("%s(): line %d: (%d*%d) cam_sw_vga=%d?\n",__FUNCTION__, __LINE__,ffmt->width,ffmt->height,cam_sw_vga);
	if (ffmt->field==2)
	{
		if (ffmt->width == 640 && ffmt->height == 480)
		{
			gIs_30W_capture = 1;
			gIs_720P_capture = 0;
			gIs_200W_capture = 0;
			gIs_500W_capture = 0;
			ut2055_enum_framesizes(sd,&cam_frmsize);
			mdelay(50);

			if( strstr(g_selected_utmodel, "s702") )
			{
				ov5640_i2c_read_byte(0x1f, 0x0118, &data1);
				ov5640_i2c_read_byte(0x1f, 0x0119, &data2);
				if(((state->flash_mode== FLASH_MODE_AUTO && data1<74) || (state->flash_mode== FLASH_MODE_ON)) )
					gpio_direction_output(EXYNOS4_GPL2(1), 1);	
			}

			if (gclient->addr == 0x24) // hm2055
			{
					if (gIsHm2055 == 0)
					{
						printk("+++++++++++++++++++++++++++++++++++ hm2057_VGA_Preview %d ++++++++++++++++++++++++++++++++\n",gIsHm2055);	
							for (i = 0; i < hm2057_VGA_Preview_SIZE; i++) {
								err = hm2055_i2c_write(sd, hm2057_VGA_Preview[i],
										sizeof(hm2057_VGA_Preview[i]));
								if (err < 0)
									printk("%s: %d register set failed\n",
											__func__, i);
							}
							//mdelay(400);
					}
			}
			if (gclient->addr == 0x31) // gc0329
			{
					if (gc0329 == 1)
						{
							//printk("+++++++++++++++++++++++++++++++++++ GC0329_Preview %d ++++++++++++++++++++++++++++++++\n",gc0329);	
							mdelay(200);//continue takepicture will be fail
						}
			}

			if (gclient->addr == 0x1f) // hm5065
			{
				if (ghm5065 == 1)
				{
					printk("+++++++++++++++++++++++++++++++++++ hm5065_vga_parameter %d ++++++++++++++++++++++++++++++++\n",ghm5065);
											
					//check 	focus							
					/*
					err =ov5640_i2c_write(0x1f,0x0010, 0x02, 2);
					if (err <= 0)
						printk("###ERR ov5640_i2c_write\n");
					mdelay(50);
					for (i = 0; i < HM5065_VGA_REGS; i++) 
					{
						err = hm2055_i2c_write(sd, hm5065_vga_reg[i],
						sizeof(hm5065_vga_reg[i]));
						if (err < 0)
							printk("%s: %d register set failed\n",
									__func__, i);
					}
					*/
					#ifdef CONTINUOUS_FOCUS
					mdelay(300);//effect focus quality  
					if(g_focus_mode==3)
					{
					#endif	
						hm5065_single_focus(sd);	
						mdelay(50);
						for (i = 0; i < 20; i++)
						{
							ov5640_i2c_read_byte(0x1f, 0x07ae, &data);
							if(data == 1)
							{
								printk("+++++++++++++++++++++++++++++++++++ vga count=%d ++++++++++++++++++++++++++++++++\n",i);
								ov5640_i2c_read_byte(0x1f, 0x06F0, &data1);
								ov5640_i2c_read_byte(0x1f, 0x06F1, &data2);
								break;
							}
							else
							{
								//printk("+++++++++++++++++++++++++++++++++++ hm5065_single_focus++++++++++++++++++++++++++++++++\n");
								mdelay(40);
							}															
						}
					#ifdef CONTINUOUS_FOCUS	
					}
					#endif
				}
			}
		}
		else if (ffmt->width == 1600 && ffmt->height == 1200)
		{
			gIs_30W_capture =  0;
			gIs_720P_capture = 0;
			gIs_200W_capture = 1;
			gIs_500W_capture = 0;
			ut2055_enum_framesizes(sd,&cam_frmsize);
			mdelay(50);

			if( strstr(g_selected_utmodel, "s702") )
			{
				ov5640_i2c_read_byte(0x1f, 0x0118, &data1);
				ov5640_i2c_read_byte(0x1f, 0x0119, &data2);
				if(((state->flash_mode== FLASH_MODE_AUTO && data1<74) || (state->flash_mode== FLASH_MODE_ON)) )
					gpio_direction_output(EXYNOS4_GPL2(1), 1);	
			}			
			if (gclient->addr == 0x24)
			{		
				if (gIsHm2055 == 0)
				{
					printk("+++++++++++++++++++++++++++++++++++ hm2057_200w_parameter ++++++++++++++++++++++++++++++++\n");
					for (i = 0; i < hm2057_200W_REGS_SIZE; i++) {
					err = hm2055_i2c_write(sd, hm2057_200w_parameter[i],
					sizeof(hm2057_200w_parameter[i]));
					if (err < 0)
						printk("%s: %d register set failed\n",__func__, i);
					}
				mdelay(200);
				}
			}
			if (gclient->addr == 0x1f) // hm5065
			{
				if (ghm5065 == 1)
				{				
					printk("+++++++++++++++++++++++++++++++++++ hm5065_200w_parameter %d ++++++++++++++++++++++++++++++++\n",ghm5065);								
					//check 	focus												
					err =ov5640_i2c_write(0x1f,0x0010, 0x00, 2);
					if (err <= 0)
						printk("###ERR ov5640_i2c_write\n");
					mdelay(50);

					for (i = 0; i < HM5065_2M_REGS; i++) 
					{
						err = hm2055_i2c_write(sd, hm5065_2M_reg[i],
						sizeof(hm5065_2M_reg[i]));
						if (err < 0)
							printk("%s: %d register set failed\n",
									__func__, i);
					}	
					
					#ifdef CONTINUOUS_FOCUS
					mdelay(300);//effect focus quality  
					#if 1
					ov5640_i2c_write(0x1f,0x00b2, 0x4f, 2);//30M 20121205
					ov5640_i2c_write(0x1f,0x00b3, 0xc0, 2);
					mdelay(50);
					ov5640_i2c_write(0x1f,0x0010, 0x02, 2);
					mdelay(50);
					ov5640_i2c_write(0x1f,0x0010, 0x01, 2);
					mdelay(20);	
					#endif
					if(g_focus_mode==3)
					{
					#endif
						ov5640_i2c_write(0x1f,0x0010, 0x02, 2);//20121205
						mdelay(50);
						ov5640_i2c_write(0x1f,0x0010, 0x01, 2);
						mdelay(50);	
						
						hm5065_single_focus(sd);	
						for (i = 0; i < 20; i++)
						{
							ov5640_i2c_read_byte(0x1f, 0x07ae, &data);
							if(data == 1)
							{
								printk("+++++++++++++++++++++++++++++++++++ 200w count=%d ++++++++++++++++++++++++++++++++\n",i);
								ov5640_i2c_read_byte(0x1f, 0x06F0, &data1);
								ov5640_i2c_read_byte(0x1f, 0x06F1, &data2);
								//g_af_state=0x2;
								break;
							}
							else
							{
								//printk("+++++++++++++++++++++++++++++++++++ hm5065_single_focus++++++++++++++++++++++++++++++++\n");
								mdelay(40);
							}															
						}
					#ifdef CONTINUOUS_FOCUS	
					}
					#endif					
				}
			}
		}
		else if (ffmt->width == 2592 && ffmt->height == 1944)
		{
			gIs_30W_capture =  0;
			gIs_720P_capture = 0;
			gIs_200W_capture = 0;
			gIs_500W_capture = 1;			
			ut2055_enum_framesizes(sd,&cam_frmsize);
			mdelay(50);
			if( strstr(g_selected_utmodel, "s702") )
			{
				ov5640_i2c_read_byte(0x1f, 0x0118, &data1);
				ov5640_i2c_read_byte(0x1f, 0x0119, &data2);
				if(((state->flash_mode== FLASH_MODE_AUTO && data1<74) || (state->flash_mode== FLASH_MODE_ON)) )
					gpio_direction_output(EXYNOS4_GPL2(1), 1);	
			}			
			if (gclient->addr == 0x1f) // hm5065
			{
				if (ghm5065 == 1)
				{
					printk("+++++++++++++++++++++++++++++++++++ hm5065_500w_parameter %d (%d %d %d)++++++++++++++++++++++++++++++++\n",ghm5065,data1,data2,state->flash_mode);
					//check 	focus												
					err =ov5640_i2c_write(0x1f,0x0010, 0x00, 2);
					if (err <= 0)
						printk("###ERR ov5640_i2c_write\n");
					mdelay(50);

					for (i = 0; i < HM5065_5M_REGS; i++) 
					{
						err = hm2055_i2c_write(sd, hm5065_5M_reg[i],
						sizeof(hm5065_5M_reg[i]));
						if (err < 0)
							printk("%s: %d register set failed\n",
									__func__, i);
					}	
					
					#ifdef CONTINUOUS_FOCUS
					mdelay(300);//effect focus quality  
					#if 1
					ov5640_i2c_write(0x1f,0x00b2, 0x4f, 2);//30M 20121205
					ov5640_i2c_write(0x1f,0x00b3, 0xc0, 2);
					mdelay(50);
					ov5640_i2c_write(0x1f,0x0010, 0x02, 2);
					mdelay(50);
					ov5640_i2c_write(0x1f,0x0010, 0x01, 2);
					mdelay(20);	
					#endif
					if(g_focus_mode==3)
					{
					#endif
						ov5640_i2c_write(0x1f,0x0010, 0x02, 2);
						mdelay(50);
						ov5640_i2c_write(0x1f,0x0010, 0x01, 2);
						mdelay(50);	
						
						hm5065_single_focus(sd);	
						for (i = 0; i < 20; i++)
						{
							ov5640_i2c_read_byte(0x1f, 0x07ae, &data);
							if(data == 1)
							{
								printk("+++++++++++++++++++++++++++++++++++ 500w count=%d ++++++++++++++++++++++++++++++++\n",i);
								ov5640_i2c_read_byte(0x1f, 0x06F0, &data1);
								ov5640_i2c_read_byte(0x1f, 0x06F1, &data2);
								//g_af_state=0x2;
								break;
							}
							else
							{
								//printk("+++++++++++++++++++++++++++++++++++ hm5065_single_focus++++++++++++++++++++++++++++++++\n");
								mdelay(40);
							}															
						}
					#ifdef CONTINUOUS_FOCUS	
					}
					#endif
				}
			}
		}
	}
	if (ffmt->field==4) // hm5065
	{
		if( strstr(g_selected_utmodel, "s702") )					
			gpio_direction_output(EXYNOS4_GPL2(1), 0);
		
		if (gclient->addr == 0x24) // hm2055
		{
			if (ffmt->width == 640 && ffmt->height == 480)
			{
				if (gIsHm2055 == 0)
				{
					printk("+++++++++++++++++++++++++++++++++++ hm2057_VGA_Preview %d ++++++++++++++++++++++++++++++++\n",gIsHm2055);	
					gIs_30W_capture = 1;
					gIs_720P_capture = 0;
					gIs_200W_capture = 0;
					gIs_500W_capture = 0;
					ut2055_enum_framesizes(sd,&cam_frmsize);
					mdelay(50);	
					for (i = 0; i < hm2057_VGA_Preview_SIZE; i++) {
						err = hm2055_i2c_write(sd, hm2057_VGA_Preview[i],
								sizeof(hm2057_VGA_Preview[i]));
						if (err < 0)
							printk("%s: %d register set failed\n",
									__func__, i);
					}
					//mdelay(400);
				}
			}
		}
		if (gclient->addr == 0x1f) // hm5065
		{
			if (ffmt->width == 640 && ffmt->height == 480)
			{
				gIs_30W_capture = 1;
				gIs_720P_capture = 0;
				gIs_200W_capture = 0;
				gIs_500W_capture = 0;
				ut2055_enum_framesizes(sd,&cam_frmsize);
				mdelay(50);	
				if (ghm5065 == 1)
				{
					printk("+++++++++++++++++++++++++++++++++++ hm5065_VGA_Preview %d ++++++++++++++++++++++++++++++++\n",ghm5065);

					err =ov5640_i2c_write(0x1f,0x0010, 0x02, 2);
					if (err <= 0)
						printk("###ERR ov5640_i2c_write\n");
					mdelay(50);
				
					for (i = 0; i < HM5065_VGA_PREVIEW; i++) 
					{
						err = hm2055_i2c_write(sd, hm5065_vga_preview[i],
						sizeof(hm5065_vga_preview[i]));
						if (err < 0)
							printk("%s: %d register set failed\n",
									__func__, i);
					}
					ov5640_i2c_write(0x1f,0x0010, 0x02, 2);	
					mdelay(50);
					ov5640_i2c_write(0x1f,0x0010, 0x01, 2);					
					mdelay(50);
					#if 1
					ov5640_i2c_write(0x1f,0x00b2, 0x50, 2);
					ov5640_i2c_write(0x1f,0x00b3, 0x58, 2);//37.5M 20121205
					mdelay(50);
					ov5640_i2c_write(0x1f,0x0010, 0x02, 2);
					mdelay(50);
					ov5640_i2c_write(0x1f,0x0010, 0x01, 2);
					mdelay(20);	
					#endif
				}
			}	
		}

	}		

	if (cam_sw_vga) // hm5065
	{
		if (gclient->addr == 0x24) // hm2055
		{
			if (ffmt->width == 640 && ffmt->height == 480)
			{
				if (gIsHm2055 == 0)
				{
					printk("+++++++++++++++++++++++++++++++++++ hm2057_VGA_Preview video %d ++++++++++++++++++++++++++++++++\n",gIsHm2055);	
					gIs_30W_capture = 1;
					gIs_720P_capture = 0;
					gIs_200W_capture = 0;
					gIs_500W_capture = 0;
					ut2055_enum_framesizes(sd,&cam_frmsize);
					mdelay(50);	
					for (i = 0; i < hm2057_VGA_Preview_SIZE; i++) {
						err = hm2055_i2c_write(sd, hm2057_VGA_Preview[i],
								sizeof(hm2057_VGA_Preview[i]));
						if (err < 0)
							printk("%s: %d register set failed\n",
									__func__, i);
					}
					//mdelay(400);
				}
			}
		}	
		if (gclient->addr == 0x1f) // hm5065
		{
			if (ffmt->width == 640 && ffmt->height == 480)
			{
				gIs_30W_capture = 1;
				gIs_720P_capture = 0;
				gIs_200W_capture = 0;
				gIs_500W_capture = 0;
				cam_sw_vga=0;
				ut2055_enum_framesizes(sd,&cam_frmsize);
				mdelay(50); 
				if (ghm5065 == 1)
				{
					printk("+++++++++++++++++++++++++++++++++++ cflag hm5065_VGA_Preview video %d ++++++++++++++++++++++++++++++++\n",ghm5065);
		
					err =ov5640_i2c_write(0x1f,0x0010, 0x02, 2);
					if (err <= 0)
						printk("###ERR ov5640_i2c_write\n");
					mdelay(50);
					/*for (i = 0; i < HM5065_INIT_REG3S; i++) 
						{
							err = hm2055_i2c_write(sd, hm5065_init_reg3[i],
									sizeof(hm5065_init_reg3[i]));
							if (err < 0)
								printk("%s: %d register set failed\n",
									__func__, i);
						}
					//mdelay(200);
					//ov5640_i2c_write(0x1f,0x0010, 0x02, 2);
					mdelay(400);	*/				
					for (i = 0; i < HM5065_VGA_PREVIEW; i++) 
					{
						err = hm2055_i2c_write(sd, hm5065_vga_preview[i],
						sizeof(hm5065_vga_preview[i]));
						if (err < 0)
							printk("%s: %d register set failed\n",
									__func__, i);
					}
					mdelay(50);
				}
			}		
		}
	}
	//if(cam_sw_720p)
	//{
		if (gclient->addr == 0x24) // hm2055
		{
			if (ffmt->width == 1280 && ffmt->height == 720)
			{
				if (gIsHm2055 == 0)
				{
					printk("+++++++++++++++++++++++++++++++++++ hm2057_720p_Preview video %d ++++++++++++++++++++++++++++++++\n",gIsHm2055);	
					gIs_30W_capture = 0;
					gIs_720P_capture = 1;
					gIs_200W_capture = 0;
					gIs_500W_capture = 0;
					cam_sw_720p=0;
					ut2055_enum_framesizes(sd,&cam_frmsize);
					mdelay(50);	
					for (i = 0; i < hm2057_720P_REGS_SIZE; i++) {
						err = hm2055_i2c_write(sd, hm2057_720p_parameter[i],
								sizeof(hm2057_720p_parameter[i]));
						if (err < 0)
							printk("%s: %d register set failed\n",
									__func__, i);
					}
					//mdelay(400);
				}
			}		
		}		
		if (gclient->addr == 0x1f) // hm5065
		{
			if (ffmt->width == 1280 && ffmt->height == 720)
			{
				gIs_30W_capture = 0;
				gIs_720P_capture = 1;
				gIs_200W_capture = 0;
				gIs_500W_capture = 0;
				cam_sw_720p=0;
				ut2055_enum_framesizes(sd,&cam_frmsize);
				mdelay(50);				
				if (ghm5065 == 1)
				{
					//gpio_direction_output(EXYNOS4_GPL0(0), 0); 
					//gpio_direction_output(EXYNOS4_GPL0(1), 1); 
					//mdelay(50);
					printk("+++++++++++++++++++++++++++++++++++ hm5065_720P video %d ++++++++++++++++++++++++++++++++\n",ghm5065);
						
					err =ov5640_i2c_write(0x1f,0x0010, 0x02, 2);
					if (err <= 0)
						printk("###ERR ov5640_i2c_write\n");
					mdelay(50);				
					for (i = 0; i < HM5065_INIT_REG3S; i++) 
						{
							err = hm2055_i2c_write(sd, hm5065_init_reg3[i],
									sizeof(hm5065_init_reg3[i]));
							if (err < 0)
								printk("%s: %d register set failed\n",
									__func__, i);
						}
					//mdelay(200);
					//ov5640_i2c_write(0x1f,0x0010, 0x02, 2);
					mdelay(200);	
					for (i = 0; i < HM5065_720P_REGS; i++) 
					{
						err = hm2055_i2c_write(sd, hm5065_720p_reg[i],
						sizeof(hm5065_720p_reg[i]));
						if (err < 0)
							printk("%s: %d register set failed\n",
									__func__, i);
					}
					//mdelay(50);
				}
			}		
		}
	//}
	if(gclient->addr == 0x1f && ffmt->field!=2){
		ov5640_i2c_write(0x1f,0x070a, 0x00, 2);
		mdelay(50);	
		ov5640_i2c_write(0x1f,0x070a, 0x01, 2);
	}
#endif
//vincent end
	#if 1
	if (unlikely(state->isp.bad_fw)) {
		cam_err("\"Unknown\" state, please update F/W");
		return -ENOSYS;
	}
	if (ffmt->width < ffmt->height) {
		tmp_width = ffmt->height;
		height = ffmt->width;
		width = tmp_width;
	}

	if (ffmt->colorspace == V4L2_COLORSPACE_JPEG) {
		state->format_mode = V4L2_PIX_FMT_MODE_CAPTURE;
		frmsize = &state->capture;
	} else {
		state->format_mode = V4L2_PIX_FMT_MODE_PREVIEW;
		frmsize = &state->preview;
	}
	
	old_index = *frmsize ? (*frmsize)->index : -1;
	*frmsize = NULL;

	if (state->format_mode != V4L2_PIX_FMT_MODE_CAPTURE) {
	//if (ffmt->field != 2) {		
		num_entries = ARRAY_SIZE(preview_frmsizes);
		for (i = 0; i < num_entries; i++) {
			if (width == preview_frmsizes[i].width &&
				height == preview_frmsizes[i].height) {
				*frmsize = &preview_frmsizes[i];
				break;
			}
		}
		printk("======== preview_frmsizes =========>i=%d width=%d height=%d\n",i,preview_frmsizes[i].width,preview_frmsizes[i].height);
	} else {
		num_entries = ARRAY_SIZE(capture_frmsizes);
		for (i = 0; i < num_entries; i++) {
			if (width == capture_frmsizes[i].width &&
				height == capture_frmsizes[i].height) {
				*frmsize = &capture_frmsizes[i];
				break;
			}
		}
		printk("======== capture_frmsizes =========>i=%d width=%d height=%d\n",i,capture_frmsizes[i].width,capture_frmsizes[i].height);
	}

	if (*frmsize == NULL) {
		cam_warn("invalid frame size %dx%d\n", width, height);
		*frmsize = state->format_mode != V4L2_PIX_FMT_MODE_CAPTURE ?
			ut2055_get_frmsize(preview_frmsizes, num_entries,
				UT2055_PREVIEW_VGA) :
			ut2055_get_frmsize(capture_frmsizes, num_entries,
				UT2055_CAPTURE_8MP);
	}

	cam_dbg("%dx%d\n", (*frmsize)->width, (*frmsize)->height);
	ut2055_set_frmsize(sd);
	//ut2055_enum_framesizes(sd,&cam_frmsize);

	cam_trace("X\n");
	#endif
	return 0;
}

static int ut2055_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *a)
{
	struct ut2055_state *state = to_state(sd);

	a->parm.capture.timeperframe.numerator = 1;
	a->parm.capture.timeperframe.denominator = state->fps;

	return 0;
}

static int ut2055_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *a)
{
	struct ut2055_state *state = to_state(sd);
	int err;

	u32 fps = a->parm.capture.timeperframe.denominator /
					a->parm.capture.timeperframe.numerator;

	if (unlikely(state->isp.bad_fw)) {
		cam_err("\"Unknown\" state, please update F/W");
		return -ENOSYS;
	}

	if (fps != state->fps) {
		if (fps <= 0 || fps > 30) {
			cam_err("invalid frame rate %d\n", fps);
			fps = 30;
		}
		state->fps = fps;
	}

	err = ut2055_set_mode(sd, UT2055_PARMSET_MODE);
	CHECK_ERR(err);

	cam_dbg("fixed fps %d\n", state->fps);

	return 0;
}

static int ut2055_enum_framesizes(struct v4l2_subdev *sd,
	struct v4l2_frmsizeenum *fsize)
{

	struct ut2055_state *state = to_state(sd);
	u32 err, old_mode;
	printk("%s\n",__func__);

	/*
	* The camera interface should read this value, this is the resolution
	* at which the sensor would provide framedata to the camera i/f
	* In case of image capture,
	* this returns the default camera resolution (VGA)
	*/
	#if 0
	if (state->format_mode != V4L2_PIX_FMT_MODE_CAPTURE) {
		if (state->preview == NULL || state->preview->index < 0)
			return -EINVAL;

		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = state->preview->width;
		fsize->discrete.height = state->preview->height;
	} else {
		if (state->capture == NULL || state->capture->index < 0)
			return -EINVAL;

		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = state->capture->width;
		fsize->discrete.height = state->capture->height;
	}

	return 0;
	#endif
		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	if (gclient->addr == 0x24 || gclient->addr == 0x20)
		{
			if (gIs_30W_capture)
			{
					fsize->discrete.width = 800;	
					fsize->discrete.height = 600;
			}
			else if(gIs_720P_capture)
			{
					fsize->discrete.width = 1280;	
					fsize->discrete.height = 720;
			}
			else if(gIs_200W_capture)
			{
					fsize->discrete.width = 1600;	
					fsize->discrete.height = 1200;
			}			
		}
	else if (gclient->addr == 0x3c)
		{
			if (!gIs_200W_capture)
			{
					fsize->discrete.width = 640;	
					fsize->discrete.height = 480;
			}
			else
			{
					fsize->discrete.width = 2592;	
					fsize->discrete.height = 1944;
			}
		}
	else if (gclient->addr == 0x1f)
		{
			if (gIs_30W_capture)
			{
					fsize->discrete.width = 640;	
					fsize->discrete.height = 480;
			}
			else if(gIs_720P_capture)
			{
					fsize->discrete.width = 1280;	
					fsize->discrete.height = 720;
			}			
			else if(gIs_200W_capture)
			{
					fsize->discrete.width = 1600;	
					fsize->discrete.height = 1200;
			}
			else if(gIs_500W_capture)
			{
					fsize->discrete.width = 2592;
					fsize->discrete.height = 1944;
//					fsize->discrete.height = 1900;
			}
		}
	else
		{
			fsize->discrete.width = 640;	
			fsize->discrete.height = 480;
		}
	
	//int err = 0;
	printk("fsize->discrete.width=%d fsize->discrete.height=%d\n" ,fsize->discrete.width,fsize->discrete.height);

	return 0;
}

static void hm5065_single_focus(struct v4l2_subdev *sd)
{
	//ov5640_i2c_write(0x1f, 0x070a, 0x00, 2);//close continue focus
#if 0
	mdelay(20);
	ov5640_i2c_write(0x1f, 0x331e, 0x03, 2);
	mdelay(20);
	ov5640_i2c_write(0x1f, 0x070a, 0x00, 2);
	ov5640_i2c_write(0x1f, 0x070c, 0x00, 2);
	mdelay(20);
	ov5640_i2c_write(0x1f, 0x070c, 0x03, 2);
	mdelay(20);
#endif
	
	
	ov5640_i2c_write(0x1f, 0x070a, 0x03, 2);
	mdelay(50);
	ov5640_i2c_write(0x1f, 0x070b, 0x01, 2);
	mdelay(50);
	ov5640_i2c_write(0x1f, 0x070b, 0x02, 2);
	mdelay(50);

}

static int ut2055_s_stream_preview(struct v4l2_subdev *sd, int enable)
{
	struct ut2055_state *state = to_state(sd);
	u32 old_mode, int_factor;
	int err,i,data,data1,data2;
	printk("%s\n",__func__);

#if 0
	if (enable) {
		ut2055_set_lock(sd, 0);

		if (state->vt_mode) {
			printk("vt mode\n");
		}

		old_mode = ut2055_set_mode(sd, UT2055_MONITOR_MODE);
		if (old_mode <= 0) {
			cam_err("failed to set mode\n");
			return old_mode;
		}

		if (old_mode != UT2055_MONITOR_MODE) {
#if 0			
			int_factor = ut2055_wait_interrupt(sd, UT2055_ISP_TIMEOUT);
			if (!(int_factor & UT2055_INT_MODE)) {
				cam_err("UT2055_INT_MODE isn't issued, %#x\n",
					int_factor);
				return -ETIMEDOUT;
			}
#endif
		}

		if (state->check_dataline) {
			err = ut2055_check_dataline(sd, state->check_dataline);
			CHECK_ERR(err);
		}
	} else {
		
	}
#endif
#if 0
	if(!enable)
	{						
		if (gclient->addr == 0x24) // hm2055
		{
			if (gIsHm2055 == 0)
			{
				printk("+++++++++++++++++++++++++++++++++++ hm2057_VGA_Preview %d ++++++++++++++++++++++++++++++++\n",gIsHm2055);	
				for (i = 0; i < hm2057_VGA_Preview_SIZE; i++) {
					err = hm2055_i2c_write(sd, hm2057_VGA_Preview[i],
					sizeof(hm2057_VGA_Preview[i]));
					if (err < 0)
						printk("%s: %d register set failed\n",
								__func__, i);
				}
				//mdelay(300);
			}
		gIs_200W_capture = 0;	
		}
		if (gclient->addr == 0x1f) // hm5065
		{
			if (ghm5065 == 1)
			{
				printk("+++++++++++++++++++++++++++++++++++ hm5065_VGA_Preview %d ++++++++++++++++++++++++++++++++\n",ghm5065);

				err =ov5640_i2c_write(0x1f,0x0010, 0x00, 2);
				ov5640_i2c_read_byte(0x1f, 0x0010, &data);
				printk("###ERR ov5640_i2c_read_byte %d\n",data);
				if (err >= 0)
					printk("###ERR ov5640_i2c_write\n");
				mdelay(200);
				for (i = 0; i < HM5065_VGA_REGS; i++) 
				{
					err = hm2055_i2c_write(sd, hm5065_vga_reg[i],
					sizeof(hm5065_vga_reg[i]));
					if (err < 0)
						printk("%s: %d register set failed\n",
								__func__, i);
				}
				mdelay(300);
			}
			gIs_500W_capture = 0;
		}
	}
	else
	{
		if (gclient->addr == 0x24) // hm2055
		{
			if (gIsHm2055 == 0)
			{
				printk("+++++++++++++++++++++++++++++++++++ hm2057_200w_parameter %d ++++++++++++++++++++++++++++++++\n",gIsHm2055);	
				for (i = 0; i < hm2057_200W_REGS_SIZE; i++) {
					err = hm2055_i2c_write(sd, hm2057_200w_parameter[i],
					sizeof(hm2057_200w_parameter[i]));
					if (err < 0)
						printk("%s: %d register set failed\n",
								__func__, i);
				}
				mdelay(400);
			}
			gIs_200W_capture = 1;
		}
		if (gclient->addr == 0x1f) // hm5065
		{
			/*if (ghm5065 == 1)
			{
				printk("+++++++++++++++++++++++++++++++++++ hm5065_500w_parameter %d %d++++++++++++++++++++++++++++++++\n",ghm5065,sizeof(hm5065_5M_reg[0]));	
				err =hm2055_i2c_write(sd, reg_hm5065[0], sizeof(reg_hm5065[0]));
				ov5640_i2c_read_byte(0x1f, 0x0010, &data);
				printk("###ERR ov5640_i2c_read_byte %d\n",data);
				if (err < 0)
					printk("###ERR ov5640_i2c_write\n");
				mdelay(200);

				for (i = 0; i < HM5065_2M_REGS; i++) 
				{
					err = hm2055_i2c_write(sd, hm5065_2M_reg[i],
					sizeof(hm5065_2M_reg[i]));
					if (err < 0)
						printk("%s: %d register set failed\n",
								__func__, i);
				}
				mdelay(300);
				ov5640_i2c_read_byte(0x1f, 0x0010, &data);
				printk("###ERR ov5640_i2c_read_byte %d\n",data);
			}*/
			printk("+++++++++++++++++++++++++++++++++++ hm5065_500w_parameter ++++++++++++++++++++++++++++++++\n");	
#if 0
			hm5065_single_focus(sd);							
			//check 	focus							

			for (i = 0; i < 10; i++)
				{
					ov5640_i2c_read_byte(0x1f, 0x07ae, &data);
					if(data == 1)
						{
							ov5640_i2c_read_byte(0x1f, 0x06F0, &data1);
							ov5640_i2c_read_byte(0x1f, 0x06F1, &data2);
							break;
						}
					else
						{
							mdelay(200);
						}
	
				}
			//check end 				
#endif				
															
			ov5640_i2c_write(0x1f,0x0010, 0x02, 2);
			mdelay(200);
#if 1	
			ov5640_i2c_write(0x1f,0x0085, 0x01, 2); //Full size 
			ov5640_i2c_write(0x1f,0x0040, 0x00, 2); //Full size

			ov5640_i2c_write(0x1f,0x0041, 0x00, 2); //00:full size	 
			mdelay(600);
			ov5640_i2c_read_byte(0x1f, 0x0085, &data);
			printk("###ERR ov5640_i2c_read_byte %d\n",data);
			//5640_i2c_write(0x1f,0x00e8, 0x01, 2);
			//5640_i2c_write(0x1f,0x00ed, 0x05, 2); 
			//5640_i2c_write(0x1f,0x00ee, 0x1e, 2); 	 
#endif
			
			//5640_i2c_write(0x1f,0x0010, 0x02, 2);
			//leep(200);	
			ov5640_i2c_write(0x1f,0x0010, 0x01, 2); 
			mdelay(200);

			
#if 0
			//focus
						
			ov5640_i2c_write(0x1f,0x070a, 0x00, 2);
			if (data == 1)
				{
					ov5640_i2c_write(0x1f,0x0734, data1&0xFF, 2);
					ov5640_i2c_write(0x1f,0x0735, data2&0xFF, 2);
				}
		
			ov5640_i2c_write(0x1f,0x070c, 0x00, 2);
			mdelay(50);
			ov5640_i2c_write(0x1f,0x070c, 0x05, 2); 												
			mdelay(50);
			ov5640_i2c_write(0x1f,0x070a, 0x00, 2);
			mdelay(100);
			ov5640_i2c_write(0x1f,0x070c, 0x00, 2);
			mdelay(100);
			ov5640_i2c_write(0x1f,0x070c, 0x03, 2);
			mdelay(100);
			ov5640_i2c_write(0x1f,0x070a, 0x03, 2);
			mdelay(100);
			ov5640_i2c_write(0x1f,0x070b, 0x01, 2);
			mdelay(100);
			ov5640_i2c_write(0x1f,0x070b, 0x02, 2);
			mdelay(100);
			//end		
#endif
			gIs_500W_capture = 1;
		}	
	}
#endif
	return 0;
}

static int ut2055_s_stream_capture(struct v4l2_subdev *sd, int enable)
{
	u32 int_factor;
	int err,i,data,data1,data2,cap_index=0;
	struct ut2055_state *state = to_state(sd);
	struct v4l2_frmsizeenum cam_frmsize;
	printk("==========================>%s index=%d enable=%d\n",__func__,state->capture->index,enable);
	cap_index=state->capture->index;
	cam_frmsize.discrete.width = 0;
	cam_frmsize.discrete.height = 0;
	/*if(enable){
	ov5640_i2c_write(0x1f,0x0046, 0x09, 2);
	ov5640_i2c_read_byte(0x1f, 0x0046, &data);
	printk("###ERR ov5640_i2c_read_byte %d\n",data);
		}else{
		ov5640_i2c_write(0x1f,0x0046, 0x00, 2);
		ov5640_i2c_read_byte(0x1f, 0x0046, &data);
		printk("###ERR ov5640_i2c_read_byte %d\n",data);

		}*/

	if (enable) {
#if 0		
		err = ut2055_set_mode(sd, UT2055_STILLCAP_MODE);
		if (err <= 0) {
			cam_err("failed to set mode\n");
			return err;
		}

		int_factor = ut2055_wait_interrupt(sd, UT2055_ISP_TIMEOUT);
		if (!(int_factor & UT2055_INT_SOUND)) {
			cam_err("UT2055_INT_SOUND isn't issued, %#x\n",
				int_factor);
			return -ETIMEDOUT;
		}
#endif		
#if 1
		if(state->capture->index == 0)
		{
			gIs_30W_capture = 1;
			gIs_200W_capture = 0;
			gIs_500W_capture = 0;
			
			if (gclient->addr == 0x1f) // hm5065
			{
			#if 1																			
				ov5640_i2c_write(0x1f,0x0010, 0x02, 2);
				mdelay(200);	
				ov5640_i2c_write(0x1f,0x0040, 0x00, 2); //Full size
				ov5640_i2c_write(0x1f,0x0041, 0x04, 2); //00:full size	 
				ov5640_i2c_write(0x1f,0x0046, 0x09, 2);
				mdelay(200);
				ov5640_i2c_read_byte(0x1f, 0x0046, &data);
				printk("###ERR ov5640_i2c_read_byte %d\n",data);
				ov5640_i2c_write(0x1f,0x0010, 0x01, 2); 
				mdelay(200);
			#endif
				if (ghm5065 == 0)
				{
					printk("111+++++++++++++++++++++++++++++++++++ hm5065_VGA %d ++++++++++++++++++++++++++++++++\n",ghm5065);
					ut2055_enum_framesizes(sd,&cam_frmsize);
					mdelay(50);

					err =ov5640_i2c_write(0x1f,0x0010, 0x02, 2);
					ov5640_i2c_write(0x1f,0x0046, 0x09, 2);
					if (err >= 0)
						printk("###ERR ov5640_i2c_write\n");
					mdelay(200);
					for (i = 0; i < HM5065_VGA_REGS; i++) 
					{
						err = hm2055_i2c_write(sd, hm5065_vga_reg[i],sizeof(hm5065_vga_reg[i]));
						if (err < 0)
							printk("%s: %d register set failed\n",__func__, i);
					}
					mdelay(300);
				}
			}
		}
		else if(state->capture->index == 3){
			gIs_30W_capture = 0;
			gIs_200W_capture = 1;
			gIs_500W_capture = 0;
			
			if (gclient->addr == 0x1f) // hm5065
			{				
				if (ghm5065 == 1)
				{
					printk("111+++++++++++++++++++++++++++++++++++ hm5065_200W %d ++++++++++++++++++++++++++++++++\n",ghm5065);
					ut2055_enum_framesizes(sd,&cam_frmsize);
					mdelay(50);
					err =ov5640_i2c_write(0x1f,0x0010, 0x02, 2);
					if (err >= 0)
						printk("###ERR ov5640_i2c_write\n");
					mdelay(200);
					for (i = 0; i < HM5065_2M_REGS; i++) 
					{
						err = hm2055_i2c_write(sd, hm5065_2M_reg[i],sizeof(hm5065_2M_reg[i]));
						//printk("###ov5640_i2c_read_byte hm5065_2M_reg %d\n",i);
						if (err < 0)
							printk("%s: %d register set failed\n",__func__, i);
					}
					mdelay(300);
				}
			}
		}
		else if(state->capture->index == 7){
			gIs_30W_capture = 0;
			gIs_200W_capture = 0;
			gIs_500W_capture = 1;
			
			if (gclient->addr == 0x1f) // hm5065
			{
				if (ghm5065 == 1)
				{
					printk("111+++++++++++++++++++++++++++++++++++ hm5065_500W %d ++++++++++++++++++++++++++++++++\n",ghm5065);
					ut2055_enum_framesizes(sd,&cam_frmsize);
					mdelay(50);

					err =ov5640_i2c_write(0x1f,0x0010, 0x02, 2);
					if (err >= 0)
						printk("###ERR ov5640_i2c_write\n");
					mdelay(200);
					for (i = 0; i < HM5065_5M_REGS; i++) 
					{
						err = hm2055_i2c_write(sd, hm5065_5M_reg[i],sizeof(hm5065_5M_reg[i]));
						//printk("###ov5640_i2c_read_byte hm5065_5M_reg %d\n",i);
						if (err < 0)
							printk("%s: %d register set failed\n",__func__, i);
					}
					mdelay(300);
				}
			}
		}
#if 0
		if (gclient->addr == 0x1f) // hm5065
			{
				printk("+++++++++++++++++++++++++++++++++++ hm5065_single_focus start++++++++++++++++++++++++++++++++\n");	

				hm5065_single_focus(sd);							
				//check 	focus							

				for (i = 0; i < 10; i++)
					{
						ov5640_i2c_read_byte(0x1f, 0x07ae, &data);
						if(data == 1)
							{
								ov5640_i2c_read_byte(0x1f, 0x06F0, &data1);
								ov5640_i2c_read_byte(0x1f, 0x06F1, &data2);
								break;
							}
						else
							{
								//printk("+++++++++++++++++++++++++++++++++++ hm5065_single_focus++++++++++++++++++++++++++++++++\n");
								mdelay(20);
							}
		
					}
							
				ov5640_i2c_write(0x1f,0x070a, 0x00, 2);
				if (data == 1)
					{
						ov5640_i2c_write(0x1f,0x0734, data1&0xFF, 2);
						ov5640_i2c_write(0x1f,0x0735, data2&0xFF, 2);
					}
			
				ov5640_i2c_write(0x1f,0x070c, 0x00, 2);
				mdelay(10);
				ov5640_i2c_write(0x1f,0x070c, 0x05, 2); 												
				mdelay(10);
				ov5640_i2c_write(0x1f,0x070a, 0x00, 2);
				mdelay(10);
				ov5640_i2c_write(0x1f,0x070c, 0x00, 2);
				mdelay(10);
				ov5640_i2c_write(0x1f,0x070c, 0x03, 2);
				mdelay(10);
				ov5640_i2c_write(0x1f,0x070a, 0x03, 2);
				mdelay(10);
				ov5640_i2c_write(0x1f,0x070b, 0x01, 2);
				mdelay(10);
				ov5640_i2c_write(0x1f,0x070b, 0x02, 2);
				mdelay(10);
				//end		
				//printk("+++++++++++++++++++++++++++++++++++ hm5065_single_focus end++++++++++++++++++++++++++++++++\n");

			}		
#endif

#endif		
	} 
	else {
#if 1
		gIs_30W_capture = 1;
		gIs_200W_capture = 0;
		gIs_500W_capture = 0;

		if (gclient->addr == 0x1f) // hm5065
		{
			if (ghm5065 == 1)
			{
				printk("111+++++++++++++++++++++++++++++++++++ hm5065_VGA_Preview %d ++++++++++++++++++++++++++++++++\n",ghm5065);
				ut2055_enum_framesizes(sd,&cam_frmsize);
				mdelay(50);
				err =ov5640_i2c_write(0x1f,0x0010, 0x02, 2);
				if (err >= 0)
					printk("###ERR ov5640_i2c_write\n");
				mdelay(200);
				for (i = 0; i < HM5065_VGA_PREVIEW; i++) 
				{
					err = hm2055_i2c_write(sd, hm5065_vga_preview[i],sizeof(hm5065_vga_preview[i]));
					if (err < 0)
						printk("%s: %d register set failed\n",__func__, i);
				}
				//mdelay(300);
			}
		}
#endif
	}

#if 0
	if (gclient->addr == 0x24) // hm2055
	{
		if (gIsHm2055 == 0)
		{
			printk("+++++++++++++++++++++++++++++++++++ hm2057_200w_parameter %d ++++++++++++++++++++++++++++++++\n",gIsHm2055);	
			for (i = 0; i < hm2057_VGA_Preview_SIZE; i++) {
				err = hm2055_i2c_write(sd, hm2057_200w_parameter[i],
				sizeof(hm2057_200w_parameter[i]));
				if (err < 0)
					printk("%s: %d register set failed\n",
							__func__, i);
			}
		}
	}
	gIs_200W_capture = 0;
#endif
	return 0;
}

static int ut2055_s_stream_hdr(struct v4l2_subdev *sd, int enable)
{
	int err;

	err = ut2055_set_mode(sd, UT2055_PARMSET_MODE);
	CHECK_ERR(err);

	if (enable) {
	} else {
	}
	return 0;
}

static int ut2055_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct ut2055_state *state = to_state(sd);
	int err;
	printk("=================>ut2055_s_stream [enable=%d] [state->format_mode=%d]\n",enable,state->format_mode);
	cam_trace("E\n");

	if (unlikely(state->isp.bad_fw)) {
		cam_err("\"Unknown\" state, please update F/W");
		return -ENOSYS;
	}

	switch (enable) {
	case STREAM_MODE_CAM_ON:
	case STREAM_MODE_CAM_OFF:
		switch (state->format_mode) {
		case V4L2_PIX_FMT_MODE_CAPTURE:
			printk("capture %s",
				enable == STREAM_MODE_CAM_ON ? "on" : "off");
			err = ut2055_s_stream_capture(sd, enable == STREAM_MODE_CAM_ON);
			break;
		case V4L2_PIX_FMT_MODE_HDR:
			err = ut2055_s_stream_hdr(sd, enable == STREAM_MODE_CAM_ON);
			break;
		default:
			printk("preview %s",
				enable == STREAM_MODE_CAM_ON ? "on" : "off");
			err = ut2055_s_stream_preview(sd, enable == STREAM_MODE_CAM_ON);
			break;
		}
		break;
 
	case STREAM_MODE_MOVIE_ON:
		if (state->flash_mode != FLASH_MODE_OFF)
			err = ut2055_set_flash(sd, state->flash_mode, 1);

		if (state->preview->index == UT2055_PREVIEW_720P ||
				state->preview->index == UT2055_PREVIEW_1080P)
			err = ut2055_set_af(sd, 1);
		break;

	case STREAM_MODE_MOVIE_OFF:
		if (state->preview->index == UT2055_PREVIEW_720P ||
				state->preview->index == UT2055_PREVIEW_1080P)
			err = ut2055_set_af(sd, 0);

		ut2055_set_flash(sd, FLASH_MODE_OFF, 1);
		break;

	default:
		cam_err("invalid stream option, %d\n", enable);
		break;
	}

	cam_trace("X\n");
	return 0;
}

static int ut2055_check_version(struct v4l2_subdev *sd)
{
	struct ut2055_state *state = to_state(sd);
	int i, val;


	cam_info("*************************************\n");
	cam_info("F/W Version: %s\n", state->exif.unique_id);
	cam_dbg("Binary Released: %s %s\n", __DATE__, __TIME__);
	cam_info("*************************************\n");

	return 0;
}

static int ut2055_init_param(struct v4l2_subdev *sd)
{
	int err;
	cam_trace("E\n");


#ifdef CONFIG_TARGET_LOCALE_KOR
	err = ut2055_set_antibanding(sd, 0x02);
	CHECK_ERR(err);
#endif
	cam_trace("X\n");
	return 0;
}

static int ut2055_init(struct v4l2_subdev *sd, u32 val)
{

	struct ut2055_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u32 int_factor;
	int err;

	unsigned char test_gc0308[2];
	unsigned char test_hm2055[3];
	unsigned char initreg_704[4];
	unsigned char test_hi704[2];
	unsigned char test1_hi704[2];
	unsigned char test_hm5065[2];
	unsigned char test_gc0329[2];
	int i;
	int retry;
	int count=0;
	//takepicture exceptionally foce quit
	struct v4l2_frmsizeenum cam_frmsize;
	cam_frmsize.discrete.width = 0;
	cam_frmsize.discrete.height = 0;
/*	
	#if 0

	unsigned int * base,* base1,* base2,* base3,* base4;
	#endif
*/	
	err = 0;
	
	gclient = v4l2_get_subdevdata(sd);
/*
	#if 0
	base = (unsigned int *)ioremap(0x114000cc, 0x100);
	if(!base)
	{
		printk("base ioremap failed\n");	
		return 0;
	}
	*base = 0xffffffff;

	iounmap(base);

	base1 = (unsigned int *)ioremap(0x114000AC, 0x100);//GPD0
	if(!base1)
	{
		printk("base ioremap failed\n");	
		return 0;
	}
	*base1 = 0xffffffff;

	iounmap(base1);

	base2 = (unsigned int *)ioremap(0x1140026c, 0x100);//GPJ1
	if(!base2)
	{
		printk("base ioremap failed\n");	
		return 0;
	}
	*base2 = 0xffffffff;

	iounmap(base2);

	base3 = (unsigned int *)ioremap(0x1140024c, 0x100);//GPJ0
	if(!base3)
	{
		printk("base ioremap failed\n");	
		return 0;
	}
	*base3 = 0xffffffff;

	iounmap(base3);

	base4 = (unsigned int *)ioremap(0x110000cc, 0x100);
	if(!base4)
	{
		printk("base ioremap failed\n");	
		return 0;
	}
	*base4 = 0xffffffff;

	iounmap(base4);
	#endif
*/	
	/* Default state values */
	state->isp.bad_fw = 0;

	state->preview = NULL;
	state->capture = NULL;

	state->format_mode = V4L2_PIX_FMT_MODE_PREVIEW;
	state->sensor_mode = SENSOR_CAMERA;
	state->flash_mode = FLASH_MODE_OFF;
	state->beauty_mode = 0;

	state->fps = 0;			/* auto */

	memset(&state->focus, 0, sizeof(state->focus));
	gCameraId=val;
	printk("*************************************************************%s+++++[value=%d]\n",__func__,val);

#if 0
	/* start camera program(parallel FLASH ROM) */
	err = ut2055_writeb(sd, UT2055_CATEGORY_FLASH,
		UT2055_FLASH_CAM_START, 0x01);
	CHECK_ERR(err);

#if 0
	int_factor = ut2055_wait_interrupt(sd, UT2055_ISP_TIMEOUT);
	if (!(int_factor & UT2055_INT_MODE)) {
		cam_err("firmware was erased?\n");
		state->isp.bad_fw = 1;
		return -ENOSYS;
	}
#endif
	/* check up F/W version */
	err = ut2055_check_version(sd);
	CHECK_ERR(err);

	ut2055_init_param(sd);
#endif

#if 1

	s3c_gpio_cfgall_range(EXYNOS4_GPD0(2), 2, S3C_GPIO_SFN(3), S3C_GPIO_PULL_UP);

	
	//gpio_request(EXYNOS4_GPK1(1), "camera_pw_en");
	gpio_request(EXYNOS4212_GPM3(0), "camera_pw");
	gpio_request(EXYNOS4_GPL0(0), "camera_pd0");
	gpio_request(EXYNOS4_GPL0(1), "camera_pd1");
	gpio_request(EXYNOS4_GPL2(1), "camera_flash");

	gpio_direction_output(EXYNOS4_GPL2(1), 0);

	//gpio_direction_output(EXYNOS4_GPK1(1), 1);
	
	gpio_direction_output(EXYNOS4212_GPM3(0), 0);
	mdelay(20);
	gpio_direction_output(EXYNOS4212_GPM3(0), 1);
	mdelay(100);//300
			
	gpio_direction_output(EXYNOS4_GPL0(0), 0); 			
	gpio_direction_output(EXYNOS4_GPL0(1), 0);

	mdelay(10);
	gpio_direction_output(EXYNOS4212_GPJ1(4), 0);	
	mdelay(20);//30
	gpio_direction_output(EXYNOS4212_GPJ1(4), 1);
	mdelay(50);//100

	printk("start check camera!!!!!!!!!!\n");


	if(first_check)
		{
//2 hm2055

			gclient->addr = 0x48>>1;
			
			test_hm2055[0] = 0x00;
			test_hm2055[1] = 0x22;
			test_hm2055[2] = 0x00;
			for(retry=0; retry < 4; retry++) {
//				err = ov5640_i2c_write(0x24 , 0x0022, 0x00, 2);
				err = hm2055_i2c_write(sd, test_hm2055, 3);
				if (err >= 0)
					break;
			}
			
			if(err < 0) 
				{
					printk("err = %d, hm2055 not exist\n",err);
					ghm2055 = 0;
				}
			else
				{
					printk("err = %d, hm2055/hm2057 exist\n",err);
					ghm2055 = 1;
					cam_count++;
				}

#if 1
			 if(err >= 0)
			 {			  
				unsigned int data = 0;
			 	//data=hm2055_i2c_read(sd, 0x0001);
				ov5640_i2c_read_byte(0x24, 0x0002, &data);

			   	printk("data = 0x%x\n",data);
				if (data == 0x55)
					{
						gIsHm2055 = 1;
						g_cam_model=2055;
						printk("---------------hm2055----------------\n");
					}
				else if ((data == 0x56) || (data == 0x57))
					{	
						gIsHm2055 = 0;
						g_cam_model=2057;
						printk("---------------hm2057----------------\n");
					}
				else
					{
						gIsHm2055 = 1;
						printk("---------------no----------------\n");
					}
					
			 }
#endif			 

			
//2 gc0308

//			unsigned char test_gc0308[2];
			test_gc0308[0] = 0x12;
			test_gc0308[1] = 0x80;			
			gclient->addr = 0x42>>1;
			
			
			for(retry=0; retry < 4; retry++) {
				err =ut2055_i2c_write(sd , test_gc0308, 2);
				if (err >= 0)
					break;
			}

			if(err < 0) 
				{
					printk("err = %d, gc0308 not exist\n",err);
					ggc0308 = 0;
				}
			else
				{
					printk("err = %d, gc0308 exist\n",err);
					ggc0308 = 1;
					cam_count++;
				}


//2 hi704

			gclient->addr = 0x60>>1;
			
			test_hi704[0] = 0x00;
			test_hi704[1] = 0x00;
			for(retry=0; retry < 4; retry++) {
				err =ut2055_i2c_write(sd , test_hi704, 2);
				if (err >= 0)
					break;
			}

			if(err < 0) 
				{
					printk("err = %d, hi704 not exist\n",err);
					ghi704 = 0;
				}
			else
				{
					printk("err = %d, hi704 exist\n",err);
					ghi704 = 1;
					cam_count++;
				}

//0329

			gclient->addr = 0x62>>1;
			
			test_gc0329[0] = 0x00;
			test_gc0329[1] = 0x00;
			//i2c_write_byte(0x2);
       		//delay_us(1000);
			for(retry=0; retry < 4; retry++) {
				err =ut2055_i2c_write(sd , test_gc0329, 2);
				if (err >= 0)
					break;
			}

			if(err < 0) 
				{
					printk("err = %d, gc0329 not exist\n",err);
					gc0329 = 0;
				}
			else
				{
					printk("err = %d, gc0329 exist\n",err);
					gc0329 = 1;
					cam_count++;
				}


//2  ov5640	

		
			gclient->addr = 0x78>>1;
			
//			unsigned char test_ov5640[3] = {0x31, 0x03, 0x11};
			for(retry=0; retry < 4; retry++) {
				err =ov5640_i2c_write(0x3c , 0x3103, 0x11,2);
				if (err >= 0)
					break;
			}

			if(err < 0) 
				{
					printk("err = %d, ov5640 not exist\n",err);
					gOv56540= 0;
				}
			else
				{
					printk("err = %d, ov5640 exist\n",err);
					gOv56540 = 1;
					cam_count++;
				}
	
//2 hm5065

			gclient->addr = 0x3e>>1;
			test_hm5065[0] = 0x00;
			test_hm5065[1] = 0x22;
			test_hm5065[2] = 0x00;

			gpio_direction_output(EXYNOS4_GPL0(0), 1);
			gpio_direction_output(EXYNOS4_GPL0(1), 1);
			mdelay(300);
			
//			unsigned char test_hm5065[3] = {0xff, 0xff, 0x1};
			for(retry=0; retry < 4; retry++) {
				//simulate_iic_write(0x1f,0x0001,0x01,16);
				err =hm2055_i2c_write(sd ,test_hm5065, 3);
				if (err >= 0)
					break;
			}	

			if(err < 0) 
				{
					printk("err = %d, hm5065 not exist\n",err);
					ghm5065= 0;
				}
			else
				{
					printk("err = %d, hm5065 exist\n",err);
					ghm5065 = 1;
					g_cam_model=5065;
					cam_count++;
				}

		first_check = 0;	

		}
//3 end 


//	gUt2055Alive = 1;

	printk("cam_count = %d\n",cam_count);
	if(cam_count == 1)
		{
			if (ghm2055)
				gclient->addr = 0x24;
			else if (ggc0308)
				gclient->addr = 0x21;
			else if (gc0329)
				gclient->addr = 0x31;
			else if (ghi253)
				gclient->addr = 0x20;
			else if (gOv56540)
				gclient->addr = 0x3c;
			else if (ghm5065)
				gclient->addr = 0x1f;
			else
				gclient->addr = 0x30;
		}
	else if(cam_count == 2)
		{
			if (ghm2055 == 1 && ggc0308 == 1)
				{
					if (gCameraId)
						gclient->addr = 0x21;//gc0308
					else
						gclient->addr = 0x24;//hm2057
				}
			else if (gIsHm2055 == 0 && gc0329 == 1)
				{
					if (gCameraId)
						gclient->addr = 0x31;//gc0329
					else
						gclient->addr = 0x24;//hm2057
				}
			else if (ghm5065 == 1 && ghi704 == 1)
				{
					if (gCameraId)
						gclient->addr = 0x30;//hi704
					else
						gclient->addr = 0x1f;//hm5065
				}
			else if (ghm5065 == 1 && gc0329 == 1)
				{
					if (gCameraId)
						gclient->addr = 0x31;//gc0329
					else
						gclient->addr = 0x1f;//hm5065
				}
			else if (gIsHm2055 == 0 && ghi704 == 1)
				{
					if (gCameraId)
						gclient->addr = 0x30;//hi704
					else
						gclient->addr = 0x24;//hm2057
				}
			else if (ghi253 == 1 && ghi704 == 1)
				{
					if (gCameraId)
						gclient->addr = 0x30;//hi704
					else
						gclient->addr = 0x20;//hi253
				}
			else if (gOv56540 == 1 && ggc0308 == 1)
				{
					if (gCameraId)
						gclient->addr = 0x21;//gc0308
					else
						gclient->addr = 0x3c;//ov5640
				}
			else if (ghm5065 == 1 && ggc0308 == 1)
				{
					if (gCameraId)
						gclient->addr = 0x21;//gc0308
					else
						gclient->addr = 0x1f;//hm5065
				}
			else if (gIsHm2055 == 0 && ghm5065 == 1)
				{
					if (gCameraId)
						gclient->addr = 0x24;//hm2057
					else
						gclient->addr = 0x1f;//hm5065
				}			
		}
	else
		return -1;

	/*if(cam_count==1){
			gpio_direction_output(EXYNOS4_GPL0(0), 0); 
			gpio_direction_output(EXYNOS4_GPL0(1), 1);
		}else{	
	if (!gCameraId)
		{
			gpio_direction_output(EXYNOS4_GPL0(0), 1);
			gpio_direction_output(EXYNOS4_GPL0(1), 0);
			mdelay(300);
			//gpio_direction_output(EXYNOS4212_GPJ1(4), 0); 	
			//mdelay(30);
			//gpio_direction_output(EXYNOS4212_GPJ1(4), 1);
		}
	else
		{
			gpio_direction_output(EXYNOS4_GPL0(0), 0); 
			gpio_direction_output(EXYNOS4_GPL0(1), 1);
		}
			}*/
	printk("init gCameraId = %d,gclient->addr = 0x%x\n",gCameraId,gclient->addr);

	//takepicture exceptionally foce quit
	gIs_30W_capture = 1;
	ut2055_enum_framesizes(sd,&cam_frmsize);

  	if (gclient->addr == 0x21)	 //2 gc0308
  		{
			unsigned char initreg_0308[2] = {0x12, 0x80};
			printk("%s: gc0308 camera initialization start\n", __func__);
			camera_Ycbcr_type = 0;
			g_cam_model=308;
//		      if(gCameraId)
//		      	{
				err = ut2055_i2c_write(sd, initreg_0308, 2);
				if (err < 0)
					v4l_info(client, "%s: init register set failed\n", __func__);
//		      	}
			mdelay(150);
			
			for (i = 0; i < gc0308_INIT_REGS; i++) {
				err = ut2055_i2c_write(sd, gc0308_init_reg[i],
						sizeof(gc0308_init_reg[i]));
				if (err < 0)
					v4l_info(client, "%s: %d register set failed\n",
							__func__, i);
			}
			
			
//			gc0308_white_balance_preset(sd, WhiteBanlanceValue);
  		}
	else if(gclient->addr == 0x24)  //2   //2 hm2055
		{
#if 1		
			if ( !strstr(g_selected_came, "V01") ){
				gpio_direction_output(EXYNOS4_GPL0(0), 0); 
				gpio_direction_output(EXYNOS4_GPL0(1), 1);
			}else{
				gpio_direction_output(EXYNOS4_GPL0(0), 1); 
				gpio_direction_output(EXYNOS4_GPL0(1), 0);
			}

			//added andydeng for s901-- camera 1 to 2
			printk("--cam_count:%d g_selected_utmodel:%s\n",cam_count,g_selected_utmodel);
			if(cam_count == 1  )
			{
				if(gCameraId == 0)
					{
						gpio_direction_output(EXYNOS4_GPL0(0), 0); 
						gpio_direction_output(EXYNOS4_GPL0(1), 1);
				}
				else if(gCameraId == 1)
					{
						gpio_direction_output(EXYNOS4_GPL0(0), 1); 
						gpio_direction_output(EXYNOS4_GPL0(1), 0);
				}
			}else if(cam_count == 2 && gCameraId == 1)	//added vincent for s101 hm2057+hm5065
			{
				gpio_direction_output(EXYNOS4_GPL0(0), 0); 
				gpio_direction_output(EXYNOS4_GPL0(1), 1);
			}
			//added end 				

			mdelay(50);

			v4l_info(client, "%s: hm2055 camera initialization start\n", __func__);
			camera_Ycbcr_type = 0;		
			
			if (gIsHm2055 == 0)
				{
					printk("+++++++++++++++++++++++++++++++++++ ut2055_init ++++++++++++++++++++++++++++++++\n");
					for (i = 0; i < hm2057_INIT_REGS; i++) 
						{
							err = hm2055_i2c_write(sd, hm2057_init_reg[i],
									sizeof(hm2057_init_reg[i]));
							if (err < 0)
								v4l_info(client, "%s: %d register [hm2057]set failed\n",
										__func__, i);
						}
//					hm2057_white_balance_preset(sd, WhiteBanlanceValue);
				g_cam_model=2057;
				}
			else
				{
					for (i = 0; i < hm2055_INIT_REGS; i++) 
						{
							err = hm2055_i2c_write(sd, hm2055_init_reg[i],
									sizeof(hm2055_init_reg[i]));
							if (err < 0)
								v4l_info(client, "%s: %d register [hm2055]set failed\n",
										__func__, i);
						}
//					hm2055_white_balance_preset(sd, WhiteBanlanceValue);
				g_cam_model=2055;
				}
#endif
			
		}
	else if(gclient->addr == 0x30)//2  hi704
		{
		#if 1
			if ( !strstr(g_selected_came, "V01") ){
				gpio_direction_output(EXYNOS4_GPL0(0), 1); 
				gpio_direction_output(EXYNOS4_GPL0(1), 0);
			}else{
				gpio_direction_output(EXYNOS4_GPL0(0), 0); 
				gpio_direction_output(EXYNOS4_GPL0(1), 1);
			}
			mdelay(50);
			//unsigned char initreg_704[4] = {0x00, 0x00, 0x00, 0x00};
			initreg_704[0] = 0x00;
			initreg_704[1] = 0x00;
			initreg_704[2] = 0x00;
			initreg_704[3] = 0x00;

			v4l_info(client, "%s: hi704 camera initialization start\n", __func__);
//			mdelay(150);
			camera_Ycbcr_type = 0;
			g_cam_model=704;
			err = ut2055_i2c_write(sd, initreg_704, 4);
			
			err = ut2055_i2c_write(sd, initreg_704, 2);
			if (err < 0)
				v4l_info(client, "[initreg_704]%s: register set failed\n",__func__);
					
			mdelay(150);
			
			for (i = 0; i < HI704_INIT_REGS; i++) 
				{
					err =hm2055_i2c_write(sd, hi704_init_reg[i], sizeof(hi704_init_reg[i]));
					if (err < 0)
						v4l_info(client, "[vincent]%s: %d register set failed\n",
								__func__, i);
				}

			//Hi704_white_balance_preset(sd, WhiteBanlanceValue);
		#endif
		}	
		else if(gclient->addr == 0x31)//2  gc0329
		{
			if ( !strstr(g_selected_came, "V01") ){
				gpio_direction_output(EXYNOS4_GPL0(0), 1); 
				gpio_direction_output(EXYNOS4_GPL0(1), 0);
			}else {
				if (ghm5065 ){
				gpio_direction_output(EXYNOS4_GPL0(0), 0); 
				gpio_direction_output(EXYNOS4_GPL0(1), 0);
				}else{
					gpio_direction_output(EXYNOS4_GPL0(0), 0); 
					gpio_direction_output(EXYNOS4_GPL0(1), 1);
				}
			}
			g_cam_model=329;
			mdelay(50);
			for (i = 0; i < GC0329_INIT_REGS; i++) {
				err = hm2055_i2c_write(sd, gc0329_init_reg[i], sizeof(gc0329_init_reg[i]));
				if (err < 0)
					v4l_info(client, "%s: %d [gc0329]register set failed\n",
							__func__, i);
			}
			#if 0
			if( strstr(g_selected_utmodel, "s702") ){
				mdelay(50);	
				for (i = 0; i < GC0329_MIRROR ; i++) {
					err =hm2055_i2c_write(sd, gc0329_mirror[i], sizeof(gc0329_mirror[i]));
					if (err < 0)
					v4l_info(client, "%s: [gc0329]mirror register set failed\n", __func__);	
				}
			}
			#endif
			mdelay(300);//switch to front camera will be blank screen
			//if (err < 0) {
			//	v4l_err(client, "%s: camera initialization failed\n",
			//			__func__);
			//	return -EIO;	/* FIXME */
			//}
		}	
	else if (gclient->addr == 0x3c)  //2 ov5640
		{

			gpio_direction_output(EXYNOS4_GPL0(0), 1); 
			gpio_direction_output(EXYNOS4_GPL0(1), 0); 
			
			camera_Ycbcr_type = 1;

					
			for (i = 0; i < 2; i++) 
				{
					err = hm2055_i2c_write(sd, ov5640_init_reg[i],
							sizeof(ov5640_init_reg[i]));
					if (err < 0)
						v4l_info(client, "%s: %d register [ov5640]set failed\n",
								__func__, i);
				}
			mdelay(100);
			
			for (i = 2; i < OV5640_INIT_REG_SIZE; i++) 
				{
					err = hm2055_i2c_write(sd, ov5640_init_reg[i],
							sizeof(ov5640_init_reg[i]));
					if (err < 0)
						v4l_info(client, "%s: %d register [ov5640]set failed\n",
								__func__, i);
				}

			
			
			for (i = 0; i < OV5640_FIRMWARE_REG_SIZE; i++) 
				{
					err = hm2055_i2c_write(sd, ov5640_firmware_reg[i],
							sizeof(ov5640_firmware_reg[i]));
					if (err < 0)
						v4l_info(client, "%s: %d register [ov5640]set failed\n",
								__func__, i);
				}
			
			mdelay(20);
			ov5640_auto_focus(sd);
			
							
		}
	else if (gclient->addr == 0x1f)  //2 hm5065
		{


			gpio_direction_output(EXYNOS4_GPL0(0), 1); 
			gpio_direction_output(EXYNOS4_GPL0(1), 1); 
			mdelay(20);//50

			camera_Ycbcr_type = 1;
			g_cam_model=5065;

			for (i = 0; i < HM5065_INIT_REG1S; i++) 
				{
					err = hm2055_i2c_write(sd, hm5065_init_reg1[i],
							sizeof(hm5065_init_reg1[i]));
					if (err < 0)
					{
						v4l_info(client, "[1]%s: %d  init register [hm5065] 0x%x%x set failed\n",
								__func__, i, hm5065_init_reg1[i][0], hm5065_init_reg1[i][1]);
						do{
							err = hm2055_i2c_write(sd, hm5065_init_reg1[i],
														sizeof(hm5065_init_reg1[i]));
							count++;
							printk("%s--------1111[%d]\n",__func__,count);							
						}while(err<0 && count<5);
						count=0;
						
					}
				}
			mdelay(100);//200
			for (i = 0; i < HM5065_INIT_REG2S; i++) 
				{
					err = hm2055_i2c_write(sd, hm5065_init_reg2[i],
							sizeof(hm5065_init_reg2[i]));
					if (err < 0)
					{
						v4l_info(client, "[2]%s: %d  init register [hm5065] 0x%x%x set failed\n",
								__func__, i, hm5065_init_reg2[i][0], hm5065_init_reg2[i][1]);

						do{
							err = hm2055_i2c_write(sd, hm5065_init_reg1[i],
														sizeof(hm5065_init_reg1[i]));
							count++;
							printk("%s--------222[%d]\n",__func__,count);							
						}while(err<0 && count<5);
						count=0;
					}

				}
			mdelay(100);//200
			for (i = 0; i < HM5065_INIT_REG3S; i++) 
				{
					err = hm2055_i2c_write(sd, hm5065_init_reg3[i],
							sizeof(hm5065_init_reg3[i]));
					if (err < 0)
					{
						v4l_info(client, "[3]%s: %d  init register [hm5065] 0x%x%x set failed\n",
								__func__, i, hm5065_init_reg3[i][0], hm5065_init_reg3[i][1]);

						do{
							err = hm2055_i2c_write(sd, hm5065_init_reg1[i],
														sizeof(hm5065_init_reg1[i]));
							count++;
							printk("%s--------333[%d]\n",__func__,count);
						}while(err<0 && count<5);
						count=0;
					}

				}
//add by jerry for 702
			if(!strncmp(g_selected_utmodel, "s702", strlen("s702")))
				{
					ov5640_i2c_write(0x1f,0x0083, 0x00, 2);
					ov5640_i2c_write(0x1f,0x0084, 0x00, 2);
				}
//end
//add by jerry for 501
#if 1
			if(!strncmp(g_selected_utmodel, "s501", strlen("s501")))
				{
					ov5640_i2c_write(0x1f,0x0083, 0x00, 2);
					ov5640_i2c_write(0x1f,0x0084, 0x00, 2);
				}
#endif
//end
//add by jerry for 703
			if(!strncmp(g_selected_utmodel, "s703", strlen("s703")))
				{
					ov5640_i2c_write(0x1f,0x0083, 0x00, 2);
					ov5640_i2c_write(0x1f,0x0084, 0x00, 2);
				}
//end
			
			mdelay(100);//200

 			ov5640_i2c_write(0x1f,0x01a0, 0x01, 2);//jerry auto white blance
			ov5640_i2c_write(0x1f,0x00b2, 0x50, 2);
			ov5640_i2c_write(0x1f,0x00b3, 0x58, 2);//37.5M
			mdelay(50);
			ov5640_i2c_write(0x1f,0x0010, 0x02, 2);
			mdelay(50);
			ov5640_i2c_write(0x1f,0x0010, 0x01, 2);
			mdelay(20);

		}
	
	if (err < 0) {
		v4l_err(client, "%s: camera initialization failed\n",
				__func__);
		//return -EIO;	/* FIXME */
		return 0;
	}
#endif

	printk("%s--------\n",__func__);

	return 0;
}

static const struct v4l2_subdev_core_ops ut2055_core_ops = {
	.init = ut2055_init,		/* initializing API */
	.load_fw = ut2055_load_fw,
	.queryctrl = ut2055_queryctrl,
	.g_ctrl = ut2055_g_ctrl,
	.s_ctrl = ut2055_s_ctrl,
	.g_ext_ctrls = ut2055_g_ext_ctrls,
};

static const struct v4l2_subdev_video_ops ut2055_video_ops = {
	.s_mbus_fmt = ut2055_s_fmt,
	.g_parm = ut2055_g_parm,
	.s_parm = ut2055_s_parm,
	.enum_framesizes = ut2055_enum_framesizes,
	.s_stream = ut2055_s_stream,
};

static const struct v4l2_subdev_ops ut2055_ops = {
	.core = &ut2055_core_ops,
	.video = &ut2055_video_ops,
};

static ssize_t ut2055_camera_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char type[] = "SONY_UT2055_NONE";

	return sprintf(buf, "%s\n", type);
}

static ssize_t ut2055_camera_fw_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ut2055_state *state = to_state(sd);

	return sprintf(buf, "%s\n", state->fw_version);
}

static DEVICE_ATTR(camera_type, S_IRUGO, ut2055_camera_type_show, NULL);
static DEVICE_ATTR(camera_fw, S_IRUGO, ut2055_camera_fw_show, NULL);

/*
 * ut2055_probe
 * Fetching platform data is being done with s_config subdev call.
 * In probe routine, we just register subdev device
 */
extern unsigned int lcd_reg_write(unsigned int reg, unsigned int value);

static int __devinit ut2055_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct ut2055_state *state;
	struct v4l2_subdev *sd;

	const struct ut2055_platform_data *pdata =
		client->dev.platform_data;
	int err = 0;
	unsigned int * base;
	
	printk("%s+++++++++\n", __func__);
	lcd_reg_write(0x114000ac, 0x100);  //todo
	lcd_reg_write(0x1140026c, 0xc0);  //todo	

	state = kzalloc(sizeof(struct ut2055_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

	sd = &state->sd;
	strcpy(sd->name, UT2055_DRIVER_NAME);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &ut2055_ops);

#ifdef CAM_DEBUG
	state->dbg_level = CAM_DEBUG; /*| CAM_TRACE | CAM_I2C;*/
#endif

	if (device_create_file(&client->dev, &dev_attr_camera_type) < 0) {
		cam_warn("failed to create device file, %s\n",
			dev_attr_camera_type.attr.name);
	}

	if (device_create_file(&client->dev, &dev_attr_camera_fw) < 0) {
		cam_warn("failed to create device file, %s\n",
			dev_attr_camera_fw.attr.name);
	}

	/* wait queue initialize */
	init_waitqueue_head(&state->isp.wait);

	if (pdata->config_isp_irq)
		pdata->config_isp_irq();
#if 0
	err = request_irq(pdata->irq,
		ut2055_isp_isr, IRQF_TRIGGER_RISING, "ut2055 isp", sd);
	if (err) {
		cam_err("failed to request irq\n");
		return err;
	}
#endif	
	state->isp.irq = pdata->irq;
	state->isp.issued = 0;

	printk("%s\n", __func__);

	return 0;
}

static int __devexit ut2055_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ut2055_state *state = to_state(sd);

	if (ut2055_set_af_softlanding(sd) < 0)
		cam_err("failed to set soft landing\n");

	device_remove_file(&client->dev, &dev_attr_camera_type);
	device_remove_file(&client->dev, &dev_attr_camera_fw);

	if (state->isp.irq > 0)
		free_irq(state->isp.irq, sd);

	v4l2_device_unregister_subdev(sd);

	kfree(state->fw_version);
	kfree(state);

	return 0;
}

static const struct i2c_device_id ut2055_id[] = {
	{ UT2055_DRIVER_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ut2055_id);

static struct i2c_driver ut2055_i2c_driver = {
	.driver = {
		.name	= UT2055_DRIVER_NAME,
	},
	.probe		= ut2055_probe,
	.remove		= __devexit_p(ut2055_remove),
	.id_table	= ut2055_id,
};

static int __init ut2055_mod_init(void)
{
	return i2c_add_driver(&ut2055_i2c_driver);
}

static void __exit ut2055_mod_exit(void)
{
	i2c_del_driver(&ut2055_i2c_driver);
}
module_init(ut2055_mod_init);
module_exit(ut2055_mod_exit);


MODULE_AUTHOR("Goeun Lee <ge.lee@samsung.com>");
MODULE_DESCRIPTION("driver for Fusitju UT2055 LS 8MP camera");
MODULE_LICENSE("GPL");
