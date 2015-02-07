

/* linux/drivers/media/video/bf3a20.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *	         http://www.samsung.com/
 *
 * Driver for bf3a20 (UXGA camera) from Samsung Electronics
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
#include "bf3a20.h"

#ifdef CONFIG_VIDEO_SAMSUNG_V4L2
#include <linux/videodev2_samsung.h>
#endif
#include "urbetter/power_gpio.h"

static int output_format_index = 0;
#define BF3A20_DRIVER_NAME	"BF3A20"
extern char g_selected_utmodel[];


/* Default resolution & pixelformat. plz ref bf3a20_platform.h */
#define DEFAULT_FMT		V4L2_PIX_FMT_UYVY	/* YUV422 */


#define dbg_fun     printk("*** func:%s\n",__FUNCTION__)
#define dbg_int(i)  printk("*** func:%s,i = %d\n",__FUNCTION__,i)
#define dbg_str(i)  printk("*** func:%s,i = %s\n",__FUNCTION__,i)



static int bf3a20_init(struct v4l2_subdev *sd, u32 val);

struct bf3a20_userset
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

struct bf3a20_state
{
	struct bf3a20_platform_data *pdata;
	struct v4l2_subdev sd;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	struct bf3a20_userset userset;
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
	bf3a20_PREVIEW_XGA,
};

enum bf3a20_capture_frame_size {
	
//	bf3a20_CAPTURE_QVGA=0, /*320x240*/
	bf3a20_CAPTURE_VGA =0,	/* 640x480 */
//	bf3a20_CAPTURE_SVGA,    /*800X600*/
	bf3a20_CAPTURE_1MP,	/* 1280x720 */
	bf3a20_CAPTURE_2MP,	/* UXGA  - 1600x1200 */
	
};


struct bf3a20_enum_framesize
{
	unsigned int index;
	unsigned int width;
	unsigned int height;
};
struct bf3a20_enum_framesize bf3a20_capture_framesize_list[] = {
	
	   {bf3a20_CAPTURE_VGA,   640,  480 },
	   {bf3a20_CAPTURE_1MP,   1280,  720 },
	   { bf3a20_CAPTURE_2MP, 1600, 1200 },	 
 };

//#define GPIO_CAMERA_POWER  EXYNOS4212_GPM3(0)  //D501
#define GPIO_CAMERA_POWER  GPIO_EXAXP22(1)  //D720
#define GPIO_CAMERA_PD0    EXYNOS4_GPL0(1)  //D720
//#define GPIO_CAMERA_PD0    EXYNOS4_GPL0(0)  //D501
#define GPIO_CAMERA_RESET  EXYNOS4212_GPJ1(4)



static void d1011_gpio_cfg(void)//for d1011
{
	int err = 0;

	gpio_free(EXYNOS4212_GPM3(0));
	gpio_free(EXYNOS4_GPL0(0));
	gpio_free(EXYNOS4212_GPJ1(4));
	
	err = gpio_request(EXYNOS4212_GPM3(0), "cam_power");
	if (err){
		printk( "#### failed to request EXYNOS4212_GPM3(0) ####\n");
	}else{
		s3c_gpio_cfgpin(EXYNOS4212_GPM3(0), S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(EXYNOS4212_GPM3(0), S3C_GPIO_PULL_NONE);
	}
	err = gpio_request(EXYNOS4_GPL0(0), "cam_pd");
	if (err){
		printk( "#### failed to request EXYNOS4_GPL0(0) ####\n");
	}else{
		s3c_gpio_cfgpin(EXYNOS4_GPL0(0), S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(EXYNOS4_GPL0(0), S3C_GPIO_PULL_NONE);
	}
	
	err = gpio_request(EXYNOS4212_GPJ1(4), "cam_rst");
	if (err){
		printk( "#### failed to request EXYNOS4212_GPJ1(4) ####\n");
	}else{
		s3c_gpio_cfgpin(EXYNOS4212_GPJ1(4), S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(EXYNOS4212_GPJ1(4), S3C_GPIO_PULL_NONE);
	}
	

}




struct regulator *vdd28_cam_regulator = NULL;
struct regulator *vdd18_cam_regulator = NULL;
struct regulator *vdd12_cam_regulator = NULL;

static int bf3a20_power( int enable)
{
	int ret=0;
	
	if(strncmp(g_selected_utmodel, "d1011", strlen("d1011")) == 0){
		 d1011_gpio_cfg();
		 if(enable){
			 gpio_direction_output(EXYNOS4212_GPM3(0),1);//cam_1.8/cam_2.8/cam_af
			 gpio_direction_output(EXYNOS4_GPL0(0),0);//cam_pd
			 gpio_direction_output(EXYNOS4212_GPJ1(4),1);//cam_rst
			 msleep(5);
			 gpio_direction_output(EXYNOS4_GPL0(0),1);//cam_pd
			 msleep(2);
			 gpio_direction_output(EXYNOS4212_GPJ1(4),0);//cam_rst
			 msleep(5);
			 gpio_direction_output(EXYNOS4212_GPJ1(4),1);//cam_rst
			 msleep(2);
	 
	 }else{
		 write_power_item_value(EXYNOS4212_GPM0(3),0);
		 write_power_item_value(EXYNOS4_GPL0(0),0);
		 write_power_item_value(EXYNOS4_GPL0(0),0);//cam_pd
	 }
		 
 }
else{
		if(enable){
			write_power_item_value(POWER_FCAM_18V,1);
			write_power_item_value(POWER_FCAM_28V,1);

			write_power_item_value(POWER_FCAM_PD,1);
			write_power_item_value(POWER_FCAM_RST,1);
			msleep(50);
			write_power_item_value(POWER_FCAM_PD,0);
			msleep(20);
			write_power_item_value(POWER_FCAM_RST,0);
			msleep(45);
			write_power_item_value(POWER_FCAM_RST,1);
			msleep(15);
		}else{
			write_power_item_value(POWER_FCAM_18V,0);
			write_power_item_value(POWER_FCAM_28V,0);
			write_power_item_value(POWER_FCAM_PD,1);
		}
	}
	return ret;
	
/*	
	if (enable) {
		vdd28_cam_regulator=regulator_get(NULL, "dldo2_cam_avdd_2v8");
		regulator_enable(vdd28_cam_regulator);
		regulator_put(vdd28_cam_regulator);
		
		gpio_direction_output(GPIO_CAMERA_POWER, 1);
		gpio_direction_output(GPIO_CAMERA_PD0, 1);
		gpio_direction_output(GPIO_CAMERA_RESET, 1);
		usleep_range(50000, 50000);  // must
		gpio_direction_output(GPIO_CAMERA_PD0, 0);
		usleep_range(20000, 30000);  // must
		gpio_direction_output(GPIO_CAMERA_RESET, 0);
		usleep_range(45000, 50000);  // must
		gpio_direction_output(GPIO_CAMERA_RESET, 1);
		usleep_range(15000, 15000);  // must
	} else {
		gpio_direction_output(GPIO_CAMERA_POWER, 0);
		
		vdd28_cam_regulator=regulator_get(NULL, "dldo2_cam_avdd_2v8");
		regulator_disable(vdd28_cam_regulator);
		regulator_put(vdd28_cam_regulator);
		
		gpio_direction_output(GPIO_CAMERA_PD0, 1);
	}
*/
}


static inline struct bf3a20_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct bf3a20_state, sd);
}

static int bf3a20_reset(struct v4l2_subdev *sd)
{
	dbg_fun;

	return bf3a20_init(sd, 0);
}

static int DDI_I2C_Read(struct v4l2_subdev *sd, unsigned short reg,unsigned char reg_bytes, 
					unsigned char *val, unsigned char val_bytes)
{
	unsigned char data[2];
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (reg_bytes == 2)
	{
		data[0] = reg >> 8;
		data[1] = (u8) reg & 0xff;
	}
	else
	{
		data[0] = (u8) reg & 0xff;
	}

	if (i2c_master_send(client, data, reg_bytes) != reg_bytes)
	{
		printk("write error for read!!!! \n");
		return -EIO;
	}

	if (i2c_master_recv(client, val, val_bytes) != val_bytes)
	{
		printk("read error!!!! \n");
		return -EIO;
	}

	return 0;
}

/*
 * bf3a20 register structure : 2bytes address, 2bytes value
 * retry on write failure up-to 5 times
 */
static inline int bf3a20_write(struct v4l2_subdev *sd, unsigned short addr, u8 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg;
	unsigned char data[2] = { addr, val};
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
static int __bf3a20_init_2bytes(struct v4l2_subdev *sd, unsigned short *reg[], int total)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = -EINVAL, i;
	unsigned short *item;
	unsigned char bytes;

	for (i = 0; i < total; i++)
	{
		item = (unsigned short *) &reg[i];

		if (item[0] == REG_DELAY)
		{	
			mdelay(item[1]);
			ret = 0;
		}
		else
		{	
			ret = bf3a20_write(sd, item[0], item[1]);
		}

		
	}
	if (ret < 0)
	v4l_info(client, "%s: register set failed\n", __func__);

	return ret;
}

static int bf3a20_read(struct v4l2_subdev *sd, unsigned char addr)
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

	printk( "bf3a20: read 0x%02x = 0x%02x\n", addr, buffer[0]);

	return (buffer[0]);
}


static int bf3a20_real_write_reg(struct v4l2_subdev *sd, unsigned short *reg[], int total)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = -EINVAL, i;
	unsigned short *item;

	for (i = 0; i < total; i++)
	{
		item = (unsigned short *) &reg[i];
		ret = bf3a20_write(sd, item[0], item[1]);
	//	msleep(20);
	}

	if (ret < 0)
	printk( "%s: register set failed\n", __func__);
	return ret;
}


static int bf3a20_i2c_write(struct v4l2_subdev *sd, unsigned char i2c_data[], unsigned char length)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[length], i;
	struct i2c_msg msg = { client->addr, 0, length, buf };

	for (i = 0; i < length; i++)
	{
		
		buf[i] = i2c_data[i];
	}
	return i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
}

//This function is used to set effection or AWB etc.
static int bf3a20_write_regs(struct v4l2_subdev *sd, unsigned char regs[], int size)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int i, err;
	struct bf3a20_reg *pReg = (struct bf3a20_reg *)regs;

	for (i = 0; i < size/sizeof(struct bf3a20_reg); i++)
	{
		err = bf3a20_i2c_write(sd, pReg+i, sizeof(struct bf3a20_reg));
		if (err < 0)
			v4l_info(client, "%s: register set failed\n", __func__);
	}

	return 0;
}

static const char *bf3a20_querymenu_wb_preset[] =
{ "WB Tungsten", "WB Fluorescent", "WB sunny", "WB cloudy", NULL };

static const char *bf3a20_querymenu_effect_mode[] =
{ "Effect Sepia", "Effect Aqua", "Effect Monochrome", "Effect Negative", "Effect Sketch", NULL };

static const char *bf3a20_querymenu_ev_bias_mode[] =
{ "-3EV", "-2,1/2EV", "-2EV", "-1,1/2EV", "-1EV", "-1/2EV", "0", "1/2EV", "1EV", "1,1/2EV", "2EV", "2,1/2EV", "3EV", NULL };

static struct v4l2_queryctrl bf3a20_controls[] =
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
		.maximum = ARRAY_SIZE(bf3a20_querymenu_wb_preset) - 2,//5个元素，5-2 =3 从0-3 ,4个挡
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
		.maximum = ARRAY_SIZE(bf3a20_querymenu_ev_bias_mode) - 2,
		.step = 1,
		.default_value =
		(ARRAY_SIZE(bf3a20_querymenu_ev_bias_mode) - 2) / 2,//默认取中间值
		/* 0 EV */
	},
	{
		.id = V4L2_CID_CAMERA_EFFECT,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Image Effect",
		.minimum = 0,
		.maximum = ARRAY_SIZE(bf3a20_querymenu_effect_mode) - 2,
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

const char **bf3a20_ctrl_get_menu(u32 id)
{
	dbg_int(id);

	switch (id)
	{
	case V4L2_CID_WHITE_BALANCE_PRESET:
		return bf3a20_querymenu_wb_preset;

	case V4L2_CID_CAMERA_EFFECT:
		return bf3a20_querymenu_effect_mode;

	case V4L2_CID_EXPOSURE:
		return bf3a20_querymenu_ev_bias_mode;

	default:
		return v4l2_ctrl_get_menu(id);
	}
}

static inline struct v4l2_queryctrl const *bf3a20_find_qctrl(int id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bf3a20_controls); i++)
		if (bf3a20_controls[i].id == id)
			return &bf3a20_controls[i];

	return NULL;
}

static int bf3a20_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	int i;
	dbg_fun;

	for (i = 0; i < ARRAY_SIZE(bf3a20_controls); i++)
	{
		if (bf3a20_controls[i].id == qc->id)
		{
			memcpy(qc, &bf3a20_controls[i], sizeof(struct v4l2_queryctrl));
			return 0;
		}
	}

	return -EINVAL;
}

static int bf3a20_querymenu(struct v4l2_subdev *sd, struct v4l2_querymenu *qm)
{
	struct v4l2_queryctrl qctrl;

	qctrl.id = qm->id;
	bf3a20_queryctrl(sd, &qctrl);

	return v4l2_ctrl_query_menu(qm, &qctrl, bf3a20_ctrl_get_menu(qm->id));
}

static int bf3a20_g_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;
	dbg_fun;

	return err;
}

static int bf3a20_s_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
	
	int ret = -EINVAL;
	int i, err=0;
	unsigned short *item;
	struct bf3a20_state *state =to_state(sd);
	
	printk("lzy: %s,width = %d,height = %d\n",__func__,fmt->width,fmt->height);
	state->pix.width = fmt->width;
	state->pix.height = fmt->height;

	if(1600==fmt->width&&(1200==fmt->height))
	{	state->framesize_index=bf3a20_CAPTURE_2MP;
 		err = bf3a20_real_write_reg(sd, (unsigned short **) bf3a20_init_reg_1600_1200,bf3a20_init_reg_1600_1200S);	
	}
	else if(1280==fmt->width&&(720==fmt->height))		
	{	state->framesize_index=bf3a20_CAPTURE_1MP;	
		dbg_str("bf3a20 720P set\n");
		err = bf3a20_real_write_reg(sd, (unsigned short **) bf3a20_init_reg_1280_720,bf3a20_init_reg_1280_720S);
	}
	else if(640==fmt->width&&(480==fmt->height))		
	{	state->framesize_index=bf3a20_CAPTURE_VGA;	
		dbg_str("bf3a20 30w set\n");
		err = bf3a20_real_write_reg(sd, (unsigned short **) bf3a20_init_reg_640_480,bf3a20_init_reg_640_480S);
	}
	if (err < 0)
		return -EINVAL;
	else
		return 0;

}
	


static int bf3a20_enum_framesizes(struct v4l2_subdev *sd, struct v4l2_frmsizeenum *fsize)
{
	struct bf3a20_state *state = to_state(sd);
	int num_entries = sizeof(bf3a20_capture_framesize_list) / sizeof(struct bf3a20_enum_framesize);
	struct bf3a20_enum_framesize *elem;
	int index = 0;
	int i = 0;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;

	index = state->framesize_index;
	
	printk("%s : state->framesize_index=%d\n",__func__,state->framesize_index);
	
	for (i = 0; i < num_entries; i++)
	{
		elem = &bf3a20_capture_framesize_list[i];
		printk("%s:i=%d,index=%d,w=%d,h=%d\n",__func__,i,elem->index,elem->width,elem->height);
		if (elem->index == index)
		{
			fsize->discrete.width = bf3a20_capture_framesize_list[i].width;
			fsize->discrete.height = bf3a20_capture_framesize_list[i].height;
			printk("%s : fsize->discrete.width= %d, fsize->discrete.height=%d \n ",__func__,fsize->discrete.width,fsize->discrete.height);
			return 0;
		}
	}
	return -EINVAL;
}

static int bf3a20_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_frmivalenum *fival)
{
	int err = 0;
	dbg_fun;
	return err;
}

static int bf3a20_enum_fmt(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmtdesc)
{
	int err = 0;
	
	dbg_fun;

	return err;
}

static int bf3a20_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;
	dbg_fun;

	return err;
}

static int bf3a20_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0;

	dbg_fun;

	return err;
}

static int bf3a20_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0;
	dbg_fun;
	dev_dbg(&client->dev, "%s: numerator %d, denominator: %d\n", __func__,
			param->parm.capture.timeperframe.numerator,
			param->parm.capture.timeperframe.denominator);

	return err;
}

static int bf3a20_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct bf3a20_state *state = to_state(sd);
	struct bf3a20_userset userset = state->userset;
	int err = 0;
	
	//dbg_int(ctrl->id);

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
#define ARRY_SIZE(A) (sizeof(A)/sizeof(A[0]))
static int bf3a20_set_wb(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct bf3a20_state *state = to_state(sd);
	int value=ctrl->value,ret=0;
	u8 awb=bf3a20_read(sd,0x13);
	switch(value){
		case WHITE_BALANCE_AUTO:
			bf3a20_write(sd, 0x13, awb|0x02);
			ret=bf3a20_real_write_reg(sd, bf3a20_wb_auto,ARRY_SIZE(bf3a20_wb_auto));
			break;
		case WHITE_BALANCE_SUNNY:
			bf3a20_write(sd, 0x13, awb&~0x02);
			ret=bf3a20_real_write_reg(sd, bf3a20_wb_sunny,ARRY_SIZE(bf3a20_wb_sunny));
			break;
		case WHITE_BALANCE_CLOUDY:
			bf3a20_write(sd, 0x13, awb&~0x02);
			ret=bf3a20_real_write_reg(sd, bf3a20_wb_cloudy,ARRY_SIZE(bf3a20_wb_cloudy));
			break;
		case WHITE_BALANCE_TUNGSTEN:
			bf3a20_write(sd, 0x13, awb&~0x02);
			ret=bf3a20_real_write_reg(sd, bf3a20_wb_tungsten,ARRY_SIZE(bf3a20_wb_tungsten));
			break;
		case WHITE_BALANCE_FLUORESCENT:
			bf3a20_write(sd, 0x13, awb&~0x02);
			ret=bf3a20_real_write_reg(sd, bf3a20_wb_fluorescent,ARRY_SIZE(bf3a20_wb_fluorescent));
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

static int bf3a20_set_exposure(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct bf3a20_state *state = to_state(sd);
	int value=ctrl->value,ret=0;
	switch(value){
		case 3:
			printk("bf3a20_set_exposure 3\n");
			ret=bf3a20_real_write_reg(sd, bf3a20_ev_m3,ARRY_SIZE(bf3a20_ev_m3));
			break;
		case 2:
			printk("bf3a20_set_exposure 2\n");
			ret=bf3a20_real_write_reg(sd, bf3a20_ev_m2,ARRY_SIZE(bf3a20_ev_m2));
			break;
		case 1:
			printk("bf3a20_set_exposure 1\n");
			ret=bf3a20_real_write_reg(sd, bf3a20_ev_m1,ARRY_SIZE(bf3a20_ev_m1));
			break;
		case 0:
			printk("bf3a20_set_exposure 0\n");
			ret=bf3a20_real_write_reg(sd, bf3a20_ev_default,ARRY_SIZE(bf3a20_ev_default));
			break;
		case -1:
			printk("bf3a20_set_exposure -1\n");
			ret=bf3a20_real_write_reg(sd, bf3a20_ev_p1,ARRY_SIZE(bf3a20_ev_p1));
			break;
		case -2:
			printk("bf3a20_set_exposure -2\n");
			ret=bf3a20_real_write_reg(sd, bf3a20_ev_p2,ARRY_SIZE(bf3a20_ev_p2));
			break;
		case -3:
			printk("bf3a20_set_exposure -3\n");
			ret=bf3a20_real_write_reg(sd, bf3a20_ev_p3,ARRY_SIZE(bf3a20_ev_p3));
			break;
		default:
			printk("%s:default \n",__func__);
			ret = -EINVAL	;		
	}
	if(ret==0){
		state->userset.exposure_bias=value;
	}
	printk("%s:reg[56]=0x%02x,reg[55]=0x%02x\n",__func__,bf3a20_read(sd,0x56),bf3a20_read(sd,0x55));
	return ret;
}

static int bf3a20_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct bf3a20_state *state = to_state(sd);
	int err = 0;
	int value = ctrl->value;
	
	//dbg_int(ctrl->id);

	printk("enter %s,id=%ul\n",__func__,ctrl->id);

	switch (ctrl->id)
	{

	case V4L2_CID_CAMERA_FLASH_MODE:
	case V4L2_CID_CAMERA_BRIGHTNESS:
		break;
	case V4L2_CID_CAMERA_WHITE_BALANCE:
		printk("*** V4L2_CID_CAMERA_WHITE_BALANCE value=%d\n",ctrl->value);
		bf3a20_set_wb(sd,ctrl);
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
		err = bf3a20_reset(sd);
		break;
	case V4L2_CID_CAMERA_EXPOSURE:
		printk("*** V4L2_CID_EXPOSURE value=%d\n",ctrl->value);
		bf3a20_set_exposure(sd,ctrl);
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

static int bf3a20_init(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct bf3a20_state *state = to_state(sd);
	
	int err = -EINVAL, i;	
	unsigned char test_bf3a20[2];
	 state->framesize_index=2;
	dbg_fun;
	bf3a20_power( 1);

	test_bf3a20[0]=bf3a20_read(sd,0xFC);
	test_bf3a20[1]=bf3a20_read(sd,0xFD);


	if((test_bf3a20[0]==0x39)&&(test_bf3a20[1]==0x20)){		
		printk("bf3a20	exit\n");
	}else{
		printk("bf3a20 not exit\n");
		return -ENODEV;
	}

	err = __bf3a20_init_2bytes(sd, (unsigned short **) bf3a20_init_reg,bf3a20_INIT_REGS);

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

static int bf3a20_s_config(struct v4l2_subdev *sd, int irq, void *platform_data)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct bf3a20_state *state = to_state(sd);
	struct bf3a20_platform_data *pdata;

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

static int bf3a20_s_stream(struct v4l2_subdev *sd, int enable)
{
	printk("====bf3a20_s_stream on=%d ======== \n",__func__,enable);
	return 0;
}

static const struct v4l2_subdev_core_ops bf3a20_core_ops =
{
	.init = bf3a20_init, /* initializing API */
	//.s_config = bf3a20_s_config, /* Fetch platform data */
	.queryctrl = bf3a20_queryctrl,
	.querymenu = bf3a20_querymenu,
      .s_power = bf3a20_power,
	.g_ctrl = bf3a20_g_ctrl,
	.s_ctrl = bf3a20_s_ctrl,
};

static const struct v4l2_subdev_video_ops bf3a20_video_ops =
{
	.g_mbus_fmt = bf3a20_g_fmt,
	.s_mbus_fmt = bf3a20_s_fmt,
	.enum_framesizes = bf3a20_enum_framesizes,
	.enum_frameintervals = bf3a20_enum_frameintervals,
	//.enum_fmt = bf3a20_enum_fmt,//
	//.try_fmt = bf3a20_try_fmt,
	.g_parm = bf3a20_g_parm,
	.s_parm = bf3a20_s_parm,
	.s_stream = bf3a20_s_stream,
};

static const struct v4l2_subdev_ops bf3a20_ops =
{
	.core = &bf3a20_core_ops,
	.video = &bf3a20_video_ops,
};

static int bf3a20_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct bf3a20_state *state;
	struct v4l2_subdev *sd;
	int ret =0;
	dbg_fun;
	//bf3a20_init();
	state = kzalloc(sizeof(struct bf3a20_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

	sd = &state->sd;
	strcpy(sd->name, BF3A20_DRIVER_NAME);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &bf3a20_ops);
	dev_info(&client->dev, "*** bf3a20 has been probed\n");

	
	return 0;

}

static int bf3a20_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	printk("bf3a20_remove\n");
	bf3a20_power(0);
	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id bf3a20_id[] = {
	{ BF3A20_DRIVER_NAME, 0 },
	{ }, 
};
MODULE_DEVICE_TABLE(i2c, bf3a20_id);

static struct i2c_driver bf3a20_i2c_driver =
{
	.driver = {
		.name = BF3A20_DRIVER_NAME,
	},
	.probe = bf3a20_probe,
	.remove = __devexit_p(bf3a20_remove),
	.id_table = bf3a20_id,
};

static int __init bf3a20_mod_init(void)
{
	printk("%s\n",__func__);
	return i2c_add_driver(&bf3a20_i2c_driver);
}

static void __exit bf3a20_mod_exit(void)
{
	printk("%s\n",__func__);
	i2c_del_driver(&bf3a20_i2c_driver);
}

module_init(bf3a20_mod_init);
module_exit(bf3a20_mod_exit);

MODULE_DESCRIPTION("Samsung Electronics bf3a20 UXGA camera driver");
MODULE_AUTHOR("Jinsung Yang <jsgood.yang@samsung.com>");
MODULE_LICENSE("GPL");



