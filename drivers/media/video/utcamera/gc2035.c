

/* linux/drivers/media/video/gc2035.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *	         http://www.samsung.com/
 *
 * Driver for gc2035 (UXGA camera) from Samsung Electronics
 * 1/4" 2.0Mp CMOS Image Sensor SoC with an Embedded Image Processor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include <mach/gpio.h>
#include <media/v4l2-common.h>
#include <linux/regulator/consumer.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/regs-clock.h>
#include "gc2035.h"
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#ifdef CONFIG_VIDEO_SAMSUNG_V4L2
#include <linux/videodev2_samsung.h>
#endif
#include "../../../oem_drv/power_gpio.h"

static int output_format_index = 0;
#define GC2035_DRIVER_NAME	"GC2035"

/* Default resolution & pixelformat. plz ref gc2035_platform.h */
#define DEFAULT_FMT		V4L2_PIX_FMT_UYVY	/* YUV422 */


#define dbg_fun     printk("*** func:%s\n",__FUNCTION__)
#define dbg_int(i)  printk("*** func:%s,i = %d\n",__FUNCTION__,i)
#define dbg_str(i)  printk("*** func:%s,i = %s\n",__FUNCTION__,i)



static int gc2035_init(struct v4l2_subdev *sd, u32 val);

struct gc2035_userset
{
	signed int exposure_bias; /* V4L2_CID_EXPOSURE */
	unsigned int ae_lock;
	unsigned int awb_lock;
	unsigned int auto_wb; /* V4L2_CID_CAMERA_WHITE_BALANCE */
	unsigned int manual_wb; /* V4L2_CID_WHITE_BALANCE_PRESET */
	unsigned int wb_temp; /* V4L2_CID_WHITE_BALANCE_TEMPERATURE */
	unsigned int effect; /* Color FX (AKA Color tone) */
	unsigned int contrast; /* V4L2_CID_CAMERA_CONTRAST */
	unsigned int saturation; /* V4L2_CID_CAMERA_SATURATION */
	unsigned int sharpness; /* V4L2_CID_CAMERA_SHARPNESS */
	unsigned int glamour;
};

struct gc2035_state
{
	struct gc2035_platform_data *pdata;
	struct v4l2_subdev sd;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	struct gc2035_userset userset;
	int framesize_index;
	int freq; /* MCLK in KHz */
	int is_mipi;
	int isize;
	int ver;
	int fps;
	int check_previewdata;
};

enum
{
	gc2035_PREVIEW_XGA,
};

enum gc2035_capture_frame_size {
	
//	gc2035_CAPTURE_QVGA=0, /*320x240*/
	gc2035_CAPTURE_VGA =0,	/* 640x480 */
//	gc2035_CAPTURE_SVGA,    /*800X600*/
	gc2035_CAPTURE_1MP,	/* 1280x720 */
	gc2035_CAPTURE_2MP,	/* UXGA  - 1600x1200 */
	
};


struct gc2035_enum_framesize
{
	unsigned int index;
	unsigned int width;
	unsigned int height;
};
struct gc2035_enum_framesize gc2035_capture_framesize_list[] = {
	
	   {gc2035_CAPTURE_VGA,   640,  480 },
	   {gc2035_CAPTURE_1MP,   1280,  720 },
	   { gc2035_CAPTURE_2MP, 1600, 1200 },	 
 };

static int gc2035_power( int enable)
{
	int ret=0;
	printk("**1* fun: gc2035_sensor_power ,enable = %d\n",enable);

	if (enable) {
		write_power_item_value(POWER_FCAM_18V,1);
		write_power_item_value(POWER_FCAM_28V,1);
		msleep(80);		
		write_power_item_value(POWER_FCAM_PD,0);
		msleep(25);
		write_power_item_value(POWER_FCAM_RST,0);
		msleep(50);
		write_power_item_value(POWER_FCAM_RST,1);
		msleep(80);
	}else{
		write_power_item_value(POWER_FCAM_28V,0);
		write_power_item_value(POWER_FCAM_18V,0);
		write_power_item_value(POWER_FCAM_PD,1);
	}

	return ret;
}



static inline struct gc2035_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct gc2035_state, sd);
}

static int gc2035_reset(struct v4l2_subdev *sd)
{
	dbg_fun;

	return gc2035_init(sd, 0);
}

/*
 * gc2035 register structure : 2bytes address, 2bytes value
 * retry on write failure up-to 5 times
 */
static inline int gc2035_write(struct v4l2_subdev *sd, u8 addr, u8 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg;
	u8 data[2] = { addr, val};
	int ret;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 2;
	msg.buf = data;
	ret = i2c_transfer(client->adapter, &msg, 1);
	
	if (ret > 0)
		ret = 0;

	return ret;
}

// This functin is used to init sensor.
static int __gc2035_init_2bytes(struct v4l2_subdev *sd, const struct gc2035_reg *regs, int total)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = -EINVAL, i;

	for (i = 0; i < total; i++)
		ret = gc2035_write(sd, regs[i].addr, regs[i].val);		
	if (ret < 0)
	v4l_info(client, "%s: register set failed\n", __func__);

	return ret;
}

static int gc2035_read(struct v4l2_subdev *sd, unsigned char addr)
{
	struct i2c_client *c = v4l2_get_subdevdata(sd);
	unsigned char buffer[1];
	int rc;

	buffer[0] = addr;
	if (1 != (rc = i2c_master_send(c, buffer, 1)))
		printk( "i2c i/o error: rc == %d (should be 1)\n", rc);

	msleep(10);

	if (1 != (rc = i2c_master_recv(c, buffer, 1)))
		printk("i2c i/o error: rc == %d (should be 1)\n", rc);

	printk( "gc2035: read 0x%02x = 0x%02x\n", addr, buffer[0]);

	return (buffer[0]);
}


static int gc2035_real_write_reg(struct v4l2_subdev *sd, const struct gc2035_reg * regs, int total)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = -EINVAL, i;

	for (i = 0; i < total; i++)
	{
		ret = gc2035_write(sd, regs[i].addr, regs[i].val);
	//	msleep(20);
	}

	if (ret < 0)
	printk( "%s: register set failed\n", __func__);
	return ret;
}


static int gc2035_i2c_write(struct v4l2_subdev *sd, uint8_t i2c_data[], unsigned char length)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	uint8_t buf[length], i;
	struct i2c_msg msg = { client->addr, 0, length, buf };

	for (i = 0; i < length; i++)
	{
		
		buf[i] = i2c_data[i];
	}
	return i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
}


static const char *gc2035_querymenu_wb_preset[] =
{ "WB Tungsten", "WB Fluorescent", "WB sunny", "WB cloudy", NULL };

static const char *gc2035_querymenu_effect_mode[] =
{ "Effect Sepia", "Effect Aqua", "Effect Monochrome", "Effect Negative", "Effect Sketch", NULL };

static const char *gc2035_querymenu_ev_bias_mode[] =
{ "-3EV", "-2,1/2EV", "-2EV", "-1,1/2EV", "-1EV", "-1/2EV", "0", "1/2EV", "1EV", "1,1/2EV", "2EV", "2,1/2EV", "3EV", NULL };

static struct v4l2_queryctrl gc2035_controls[] =
{
	{
		.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "White balance in kelvin",
		.minimum = 0,
		.maximum = 10000,
		.step = 1,
		.default_value = 0, /* FIXME */
	},
	{
		.id = V4L2_CID_WHITE_BALANCE_PRESET,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "White balance preset",
		.minimum = 0,
		.maximum = ARRAY_SIZE(gc2035_querymenu_wb_preset) - 2,//5个元素，5-2 =3 从0-3 ,4个挡
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_CAMERA_WHITE_BALANCE,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Auto white balance",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_EXPOSURE,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Exposure bias",
		.minimum = 0,
		.maximum = ARRAY_SIZE(gc2035_querymenu_ev_bias_mode) - 2,
		.step = 1,
		.default_value =
		(ARRAY_SIZE(gc2035_querymenu_ev_bias_mode) - 2) / 2,//默认取中间值
		/* 0 EV */
	},
	{
		.id = V4L2_CID_CAMERA_EFFECT,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Image Effect",
		.minimum = 0,
		.maximum = ARRAY_SIZE(gc2035_querymenu_effect_mode) - 2,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_CAMERA_CONTRAST,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Contrast",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
	},
	{
		.id = V4L2_CID_CAMERA_SATURATION,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Saturation",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
	},
	{
		.id = V4L2_CID_CAMERA_SHARPNESS,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Sharpness",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
	},
};

const char **gc2035_ctrl_get_menu(u32 id)
{
	dbg_int(id);

	switch (id)
	{
	case V4L2_CID_WHITE_BALANCE_PRESET:
		return gc2035_querymenu_wb_preset;

	case V4L2_CID_CAMERA_EFFECT:
		return gc2035_querymenu_effect_mode;

	case V4L2_CID_EXPOSURE:
		return gc2035_querymenu_ev_bias_mode;

	default:
		return v4l2_ctrl_get_menu(id);
	}
}

static inline struct v4l2_queryctrl const *gc2035_find_qctrl(int id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(gc2035_controls); i++)
		if (gc2035_controls[i].id == id)
			return &gc2035_controls[i];

	return NULL;
}

static int gc2035_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	int i;
	dbg_fun;

	for (i = 0; i < ARRAY_SIZE(gc2035_controls); i++)
	{
		if (gc2035_controls[i].id == qc->id)
		{
			memcpy(qc, &gc2035_controls[i], sizeof(struct v4l2_queryctrl));
			return 0;
		}
	}

	return -EINVAL;
}

static int gc2035_querymenu(struct v4l2_subdev *sd, struct v4l2_querymenu *qm)
{
	struct v4l2_queryctrl qctrl;

	qctrl.id = qm->id;
	gc2035_queryctrl(sd, &qctrl);

	return v4l2_ctrl_query_menu(qm, &qctrl, gc2035_ctrl_get_menu(qm->id));
}

static int gc2035_g_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;
	dbg_fun;

	return err;
}

static int gc2035_s_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
	
	int ret = -EINVAL;
	int i, err=0;
	unsigned short *item;
	struct gc2035_state *state =to_state(sd);
	
	printk("lzy: %s,width = %d,height = %d\n",__func__,fmt->width,fmt->height);
	state->pix.width = fmt->width;
	state->pix.height = fmt->height;

	if(1600==fmt->width&&(1200==fmt->height))
	{	state->framesize_index=gc2035_CAPTURE_2MP;
		err = gc2035_real_write_reg(sd,  gc2035_init_reg_1600_1200,
		                                   ARRY_SIZE(gc2035_init_reg_1600_1200));	
	}
	else if(1280==fmt->width&&(720==fmt->height))		
	{	state->framesize_index=gc2035_CAPTURE_1MP;	
		dbg_str("gc2035 720P set\n");
		err = gc2035_real_write_reg(sd,  gc2035_init_reg_1280_720,
			                              ARRY_SIZE(gc2035_init_reg_1280_720));
	} 
	else if(640==fmt->width&&(480==fmt->height))		
	{	state->framesize_index=gc2035_CAPTURE_VGA;	
		dbg_str("gc2035 30w set\n");
		err = gc2035_real_write_reg(sd, gc2035_init_reg_640_480,
			                               ARRY_SIZE(gc2035_init_reg_640_480));
	}
	if (err < 0)
		return -EINVAL;
	else
		return 0;

}
	


static int gc2035_enum_framesizes(struct v4l2_subdev *sd, struct v4l2_frmsizeenum *fsize)
{
	struct gc2035_state *state = to_state(sd);
	int num_entries = sizeof(gc2035_capture_framesize_list) / sizeof(struct gc2035_enum_framesize);
	struct gc2035_enum_framesize *elem;
	int index = 0;
	int i = 0;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;

	index = state->framesize_index;
	
	printk("%s : state->framesize_index=%d\n",__func__,state->framesize_index);
	
	for (i = 0; i < num_entries; i++)
	{
		elem = &gc2035_capture_framesize_list[i];
		printk("%s:i=%d,index=%d,w=%d,h=%d\n",__func__,i,elem->index,elem->width,elem->height);
		if (elem->index == index)
		{
			fsize->discrete.width = gc2035_capture_framesize_list[i].width;
			fsize->discrete.height = gc2035_capture_framesize_list[i].height;
			printk("%s : fsize->discrete.width= %d, fsize->discrete.height=%d \n ",__func__,fsize->discrete.width,fsize->discrete.height);
			return 0;
		}
	}
	return -EINVAL;
}

static int gc2035_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_frmivalenum *fival)
{
	int err = 0;
	dbg_fun;
	return err;
}

static int gc2035_enum_fmt(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmtdesc)
{
	int err = 0;
	
	dbg_fun;

	return err;
}

static int gc2035_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;
	dbg_fun;

	return err;
}

static int gc2035_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0;

	dbg_fun;

	return err;
}

static int gc2035_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0;
	dbg_fun;
	dev_dbg(&client->dev, "%s: numerator %d, denominator: %d\n", __func__,
			param->parm.capture.timeperframe.numerator,
			param->parm.capture.timeperframe.denominator);

	return err;
}

static int gc2035_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct gc2035_state *state = to_state(sd);
	struct gc2035_userset userset = state->userset;
	int err = 0;
	
	dbg_int(ctrl->id);

	switch (ctrl->id)
	{
	case V4L2_CID_CAMERA_WHITE_BALANCE:
		ctrl->value = userset.auto_wb;
		break;
	case V4L2_CID_WHITE_BALANCE_PRESET:
		ctrl->value = userset.manual_wb;
		break;
	case V4L2_CID_CAMERA_EFFECT:
		ctrl->value = userset.effect;
		break;
	case V4L2_CID_CAMERA_CONTRAST:
		ctrl->value = userset.contrast;
		break;
	case V4L2_CID_CAMERA_SATURATION:
		ctrl->value = userset.saturation;
		break;
	case V4L2_CID_CAMERA_SHARPNESS:
		ctrl->value = userset.saturation;
		break;
	case V4L2_CID_CAM_JPEG_MAIN_SIZE:
	case V4L2_CID_CAM_JPEG_MAIN_OFFSET:
	case V4L2_CID_CAM_JPEG_THUMB_SIZE:
	case V4L2_CID_CAM_JPEG_THUMB_OFFSET:
	case V4L2_CID_CAM_JPEG_POSTVIEW_OFFSET:
	case V4L2_CID_CAM_JPEG_MEMSIZE:
	case V4L2_CID_CAM_JPEG_QUALITY:
	case V4L2_CID_CAMERA_AUTO_FOCUS_RESULT:
	case V4L2_CID_CAM_DATE_INFO_YEAR:
	case V4L2_CID_CAM_DATE_INFO_MONTH:
	case V4L2_CID_CAM_DATE_INFO_DATE:
	case V4L2_CID_CAM_SENSOR_VER:
	case V4L2_CID_CAM_FW_MINOR_VER:
	case V4L2_CID_CAM_FW_MAJOR_VER:
	case V4L2_CID_CAM_PRM_MINOR_VER:
	case V4L2_CID_CAM_PRM_MAJOR_VER:
	//case V4L2_CID_ESD_INT:
	//case V4L2_CID_CAMERA_GET_ISO:
	//case V4L2_CID_CAMERA_GET_SHT_TIME:
	case V4L2_CID_CAMERA_OBJ_TRACKING_STATUS:
	case V4L2_CID_CAMERA_SMART_AUTO_STATUS:
		ctrl->value = 0;
		break;
	case V4L2_CID_EXPOSURE:
		ctrl->value = userset.exposure_bias;
		break;
	default:
		dev_err(&client->dev, "%s: no such ctrl\n", __func__);
		/* err = -EINVAL; */
		break;
	}

	return err;
}

static int gc2035_set_wb(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct gc2035_state *state = to_state(sd);
	int value=ctrl->value,ret=0;
	switch(value){
		case WHITE_BALANCE_AUTO:
			ret=gc2035_real_write_reg(sd, gc2035_wb_auto,ARRY_SIZE(gc2035_wb_auto));
			break;
		case WHITE_BALANCE_SUNNY:
			ret=gc2035_real_write_reg(sd, gc2035_wb_sunny,ARRY_SIZE(gc2035_wb_sunny));
			break;
		case WHITE_BALANCE_CLOUDY:
			ret=gc2035_real_write_reg(sd, gc2035_wb_cloudy,ARRY_SIZE(gc2035_wb_cloudy));
			break;
		case WHITE_BALANCE_TUNGSTEN:
			ret=gc2035_real_write_reg(sd, gc2035_wb_tungsten,ARRY_SIZE(gc2035_wb_tungsten));
			break;
		case WHITE_BALANCE_FLUORESCENT:
			ret=gc2035_real_write_reg(sd, gc2035_wb_fluorescent,ARRY_SIZE(gc2035_wb_fluorescent));
			break;
		default:
			printk("%s:default \n",__func__);
			ret = -EINVAL	;		
	}
	if(ret==0){
		state->userset.auto_wb=value;
	} 
	return ret;
}

static int gc2035_set_exposure(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct gc2035_state *state = to_state(sd);
	int value=ctrl->value,ret=0;
	switch(value){
		case 3:
			printk("gc2035_set_exposure 3\n");
			ret=gc2035_real_write_reg(sd, gc2035_ev_p3,ARRY_SIZE(gc2035_ev_p3));
			break;
		case 2:
			printk("gc2035_set_exposure 2\n");
			ret=gc2035_real_write_reg(sd, gc2035_ev_p2,ARRY_SIZE(gc2035_ev_p2));
			break;
		case 1:
			printk("gc2035_set_exposure 1\n");
			ret=gc2035_real_write_reg(sd, gc2035_ev_p1,ARRY_SIZE(gc2035_ev_p1));
			break;
		case 0:
			printk("gc2035_set_exposure 0\n");
			ret=gc2035_real_write_reg(sd, gc2035_ev_default,ARRY_SIZE(gc2035_ev_default));
			break;
		case -1:
			printk("gc2035_set_exposure -1\n");
			ret=gc2035_real_write_reg(sd, gc2035_ev_m1,ARRY_SIZE(gc2035_ev_m1));
			break;
		case -2:
			printk("gc2035_set_exposure -2\n");
			ret=gc2035_real_write_reg(sd, gc2035_ev_m2,ARRY_SIZE(gc2035_ev_m2));
			break;
		case -3:
			printk("gc2035_set_exposure -3\n");
			ret=gc2035_real_write_reg(sd, gc2035_ev_m3,ARRY_SIZE(gc2035_ev_m3));
			break;
		default:
			printk("%s:default \n",__func__);
			ret = -EINVAL	;		
	}
	if(ret==0){
		state->userset.exposure_bias=value;
	}
	return ret;
}

static int gc2035_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct gc2035_state *state = to_state(sd);
	int err = 0;
	int value = ctrl->value;
	
	dbg_int(ctrl->id);

	printk("enter %s,id=%ul\n",__func__,ctrl->id);

	switch (ctrl->id)
	{

	case V4L2_CID_CAMERA_FLASH_MODE:
	case V4L2_CID_CAMERA_BRIGHTNESS:
		break;
	case V4L2_CID_CAMERA_WHITE_BALANCE:
		printk("*** V4L2_CID_CAMERA_WHITE_BALANCE value=%d\n",ctrl->value);
		gc2035_set_wb(sd,ctrl);
		break;
	case V4L2_CID_CAMERA_EFFECT:
	case V4L2_CID_CAMERA_ISO:
	case V4L2_CID_CAMERA_METERING:
		break;
	case V4L2_CID_CAMERA_CONTRAST: //do not support now
	case V4L2_CID_CAMERA_SATURATION: //do not support now  
	case V4L2_CID_CAMERA_SHARPNESS: //do not support now
	case V4L2_CID_CAMERA_WDR:
	case V4L2_CID_CAMERA_FACE_DETECTION:
	case V4L2_CID_CAMERA_FOCUS_MODE:
	case V4L2_CID_CAM_JPEG_QUALITY:
	case V4L2_CID_CAMERA_SCENE_MODE:
	case V4L2_CID_CAMERA_GPS_LATITUDE:
	case V4L2_CID_CAMERA_GPS_LONGITUDE:
	case V4L2_CID_CAMERA_GPS_TIMESTAMP:
	case V4L2_CID_CAMERA_GPS_ALTITUDE:
	case V4L2_CID_CAMERA_OBJECT_POSITION_X:
	case V4L2_CID_CAMERA_OBJECT_POSITION_Y:
	case V4L2_CID_CAMERA_SET_AUTO_FOCUS:
	case V4L2_CID_CAMERA_FRAME_RATE:
		break;
	case V4L2_CID_CAM_PREVIEW_ONOFF:
		if (state->check_previewdata == 0)
			err = 0;
		else
			err = -EIO;
		break;
	case V4L2_CID_CAMERA_CHECK_DATALINE:
	case V4L2_CID_CAMERA_CHECK_DATALINE_STOP:
		break;
	case V4L2_CID_CAMERA_RESET:
		dev_dbg(&client->dev, "%s: V4L2_CID_CAMERA_RESET\n", __func__);
		err = gc2035_reset(sd);
		break;
	case V4L2_CID_CAMERA_EXPOSURE:
		printk("*** V4L2_CID_EXPOSURE value=%d\n",ctrl->value);
		gc2035_set_exposure(sd,ctrl);
		break;
	default:
		err=0;
		printk("default\n");
		/* err = -EINVAL; */
		break;
	}

	if (err < 0)
		dev_dbg(&client->dev, "%s: vidioc_s_ctrl failed\n", __func__);

	return err;
}

static int gc2035_init(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct gc2035_state *state = to_state(sd);
	
	int err = -EINVAL, i;	
	unsigned char test_gc2035[2];
	 state->framesize_index=2;
	dbg_fun;
	gc2035_power( 1);

	test_gc2035[0]=gc2035_read(sd,0xF0);
	test_gc2035[1]=gc2035_read(sd,0xF1);


	if((test_gc2035[0]==0x20)&&(test_gc2035[1]==0x35)){		
		printk("gc2035	exit\n");
	}else{
		printk("gc2035 not exit\n");
		return -ENODEV;
	}

	err = __gc2035_init_2bytes(sd, gc2035_init_reg,ARRY_SIZE(gc2035_init_reg));

	if (err < 0)
	{		
		state->check_previewdata = 100;
		v4l_err(client, "%s: camera initialization failed. err(%d)\n",
				__func__, state->check_previewdata);
		return -1;
	}

	/* This is preview success */
	state->check_previewdata = 0;
	printk("%s: camera initialization end\n", __func__);
	return 0;
}

static int gc2035_s_config(struct v4l2_subdev *sd, int irq, void *platform_data)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct gc2035_state *state = to_state(sd);
	struct gc2035_platform_data *pdata;

	dbg_fun;

	pdata = client->dev.platform_data;

	if (!pdata)
	{
		dev_err(&client->dev, "%s: no platform data\n", __func__);
		return -ENODEV;
	}

	if (!(pdata->default_width && pdata->default_height))
	{
		/* TODO: assign driver default resolution */
	}
	else
	{
		state->pix.width = pdata->default_width;
		state->pix.height = pdata->default_height;
	}

	if (!pdata->pixelformat)
		state->pix.pixelformat = DEFAULT_FMT;
	else
		state->pix.pixelformat = pdata->pixelformat;

	if (!pdata->freq)
		state->freq = 24000000; /* 24MHz default */
	else
		state->freq = pdata->freq;

	if (!pdata->is_mipi)
	{
		state->is_mipi = 0;
		dev_info(&client->dev, "parallel mode\n");
	}
	else
		state->is_mipi = pdata->is_mipi;

	return 0;
}

static int gc2035_s_stream(struct v4l2_subdev *sd, int enable)
{
	printk("====%s on=%d ======== \n",__func__,enable);
	return 0;
}

static const struct v4l2_subdev_core_ops gc2035_core_ops =
{
	.init = gc2035_init, /* initializing API */
	//.s_config = gc2035_s_config, /* Fetch platform data */
	.queryctrl = gc2035_queryctrl,
	.querymenu = gc2035_querymenu,
      .s_power = gc2035_power,
	.g_ctrl = gc2035_g_ctrl,
	.s_ctrl = gc2035_s_ctrl,
};

static const struct v4l2_subdev_video_ops gc2035_video_ops =
{
	.g_mbus_fmt = gc2035_g_fmt,
	.s_mbus_fmt = gc2035_s_fmt,
	.enum_framesizes = gc2035_enum_framesizes,
	.enum_frameintervals = gc2035_enum_frameintervals,
	//.enum_fmt = gc2035_enum_fmt,//
	//.try_fmt = gc2035_try_fmt,
	.g_parm = gc2035_g_parm,
	.s_parm = gc2035_s_parm,
	.s_stream = gc2035_s_stream,
};

static const struct v4l2_subdev_ops gc2035_ops =
{
	.core = &gc2035_core_ops,
	.video = &gc2035_video_ops,
};
static struct v4l2_subdev *SD;
static int i2c_proc_write(struct file *file, const char *buffer,
                           unsigned long count, void *data)
{
	u8 value=0;
	u8 reg=0; 
	u8 wd[2];

	if(!SD) {
		printk("[OV5645] Please open camera first\n");
		return count;
	}

	if(sscanf(buffer, "%x=%x", &reg, &value) == 2)
	{
		//reg_write_16(s_client, reg, value);
		wd[0]=reg;
		wd[1]=value;
		gc2035_i2c_write(SD,wd,2);
	}
	else if(sscanf(buffer, "%x", &reg) == 1)
	{
		value=gc2035_read(SD, reg);
	}
	printk("reg=0x%08x, value=0x%08x\n", reg, value);

	return count;
}

static int i2c_proc_read(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
	int len;
	len = sprintf( page, "usage(hex):\n  write: echo \"3008=0001\" > /proc/%s\n  read:  echo \"3008\" > /proc/%s\n", 
				   "gc2035_reg", "gc2035_reg");
    return len;
}

static void gc2035_init_proc_node(void)
{
	struct proc_dir_entry *root_entry;
	struct proc_dir_entry *entry;

	static int is_init=0;

	if(is_init)
		return;

	printk("%s\n", __FUNCTION__);
	root_entry = proc_mkdir("gc2035", NULL);
	entry = create_proc_entry("gc2035_reg", 0666, root_entry);

	if(entry) {
		entry->write_proc = i2c_proc_write;
		entry->read_proc = i2c_proc_read;
	}
	is_init=1;
}
static int gc2035_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct gc2035_state *state;
	struct v4l2_subdev *sd;
	int ret =0;
	dbg_fun;
	//gc2035_init();
	state = kzalloc(sizeof(struct gc2035_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

	sd = &state->sd;
	strcpy(sd->name, GC2035_DRIVER_NAME);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &gc2035_ops);
	dev_info(&client->dev, "*** gc2035 has been probed\n");
	
	SD=sd;
	gc2035_init_proc_node();
	return 0;

}

static int gc2035_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	printk("gc2035_remove\n");
	gc2035_power(0);
	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id gc2035_id[] = {
	{ GC2035_DRIVER_NAME, 0 },
	{ }, 
};
MODULE_DEVICE_TABLE(i2c, gc2035_id);

static struct i2c_driver gc2035_i2c_driver =
{
	.driver = {
		.name = GC2035_DRIVER_NAME,
	},
	.probe = gc2035_probe,
	.remove = __devexit_p(gc2035_remove),
	.id_table = gc2035_id,
};

static int __init gc2035_mod_init(void)
{
	printk("%s\n",__func__);
	return i2c_add_driver(&gc2035_i2c_driver);
}

static void __exit gc2035_mod_exit(void)
{
	printk("%s\n",__func__);
	i2c_del_driver(&gc2035_i2c_driver);
}

module_init(gc2035_mod_init);
module_exit(gc2035_mod_exit);

MODULE_DESCRIPTION("Samsung Electronics gc2035 UXGA camera driver");
MODULE_AUTHOR("Jinsung Yang <jsgood.yang@samsung.com>");
MODULE_LICENSE("GPL");



