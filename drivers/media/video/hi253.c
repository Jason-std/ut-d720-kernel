/* linux/drivers/media/video/HI253.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *	         http://www.samsung.com/
 *
 * Driver for HI253 (UXGA camera) from Samsung Electronics
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
//#include <media/v4l2-i2c-drv.h>
#include <media/hi253_platform.h>
#include <mach/gpio.h>
#include <media/v4l2-common.h>
#include <linux/regulator/consumer.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/regs-clock.h>

#ifdef CONFIG_VIDEO_SAMSUNG_V4L2
#include <linux/videodev2_samsung.h>
#endif

#include "hi253.h"
static int output_format_index = 0;
#define HI253_DRIVER_NAME	"HI253"

/* Default resolution & pixelformat. plz ref HI253_platform.h */
#define DEFAULT_FMT		V4L2_PIX_FMT_UYVY	/* YUV422 */

/*
 * Specification
 * Parallel : ITU-R. 656/601 YUV422, RGB565, RGB888 (Up to VGA), RAW10
 * Serial : MIPI CSI2 (single lane) YUV422, RGB565, RGB888 (Up to VGA), RAW10
 * Resolution : 1280 (H) x 1024 (V)
 * Image control : Brightness, Contrast, Saturation, Sharpness, Glamour
 * Effect : Mono, Negative, Sepia, Aqua, Sketch
 * FPS : 15fps @full resolution, 30fps @VGA, 24fps @720p
 * Max. pixel clock frequency : 48MHz(upto)
 * Internal PLL (6MHz to 27MHz input frequency)
 */

static int HI253_init(struct v4l2_subdev *sd, u32 val);

/* Camera functional setting values configured by user concept */
struct HI253_userset
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

struct HI253_state
{
	struct HI253_platform_data *pdata;
	struct v4l2_subdev sd;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	struct HI253_userset userset;
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
	HI253_PREVIEW_XGA,
};

enum HI253_capture_frame_size {
	
	HI253_CAPTURE_QVGA=0, /*320x240*/
	HI253_CAPTURE_VGA ,	/* 640x480 */
	HI253_CAPTURE_SVGA,    /*800X600*/
	HI253_CAPTURE_1MP,	/* 1280x720 */
	HI253_CAPTURE_2MP,	/* UXGA  - 1600x1200 */
	
};


struct HI253_enum_framesize
{
	unsigned int index;
	unsigned int width;
	unsigned int height;
};
#if 0 //modify.lzy now sensor did't distinguish preview or capture mode
struct HI253_enum_framesize HI253_framesize_list[] = {
	{ HI253_PREVIEW_XGA, 800, 600 }
};
#endif
struct HI253_enum_framesize HI253_capture_framesize_list[] = {
	
	   {HI253_CAPTURE_QVGA, 320,  240 },
	   {HI253_CAPTURE_VGA,   640,  480 },
	   {HI253_CAPTURE_SVGA, 800,  600 },
	   { HI253_CAPTURE_1MP, 1280, 720 },
	   { HI253_CAPTURE_2MP, 1600, 1200 },	 
 };

//clock gpio config
#define GPIO_CAM_MCLK    EXYNOS4212_GPJ1(3)
#define GPIO_CAM_PCLK     EXYNOS4212_GPJ0(0)
#define GPIO_CAM_EN         EXYNOS4_GPF0(5)
#define GPIO_CAM_nRST     EXYNOS4_GPF0(4)

struct regulator *vdd28_cam_regulator = NULL;
struct regulator *vdd18_cam_regulator = NULL;
struct regulator *vdd12_cam_regulator = NULL;


static int HI253_power( int on)
{
	//struct csi_dev *dev=(struct csi_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
	//struct i2c_client *client = v4l2_get_subdevdata(sd);
	printk("HI253_power: %s\n",on?"on":"off");
	if(on)
	{	
		printk("HI253 power on start\n");
		//pull down RESETB
		if (gpio_request(GPIO_CAM_nRST, "GPF0_4") < 0)
			pr_err("failed gpio_request(GPF0_4) for camera control\n");
		gpio_direction_output(GPIO_CAM_nRST, 0);
		s3c_gpio_setpull(GPIO_CAM_nRST, S3C_GPIO_PULL_NONE);

		//pull up CHIP_ ENABLEB
		if (gpio_request(GPIO_CAM_EN, "GPF0_5") < 0)
			pr_err("failed gpio_request(GPF0_5) for camera control\n");
		gpio_direction_output(GPIO_CAM_EN, 1);
		s3c_gpio_setpull(GPIO_CAM_EN, S3C_GPIO_PULL_NONE);

		gpio_free(GPIO_CAM_nRST);
		gpio_free(GPIO_CAM_EN);

		//open vdd
		regulator_enable(vdd28_cam_regulator); 
		udelay(10);
		regulator_enable(vdd12_cam_regulator);
		udelay(10);
		regulator_enable(vdd18_cam_regulator); 
		udelay(10);


		s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(2));//MCLK

		//pull down CHIP_ ENABLEB
		if (gpio_request(GPIO_CAM_EN, "GPF0_5") < 0)
		pr_err("failed gpio_request(GPF0_5) for Hi253 camera control\n");
		gpio_direction_output(GPIO_CAM_EN, 0);
		s3c_gpio_setpull(GPIO_CAM_EN, S3C_GPIO_PULL_NONE);

		

		mdelay(110);

		//pull up RESETB
		if (gpio_request(GPIO_CAM_nRST, "GPF0_4") < 0)
		pr_err("failed gpio_request(GPF0_4) for Hi253 camera control\n");
		gpio_direction_output(GPIO_CAM_nRST, 1);
		s3c_gpio_setpull(GPIO_CAM_nRST, S3C_GPIO_PULL_NONE);

		// PCLK high
		s3c_gpio_cfgpin(GPIO_CAM_PCLK, S3C_GPIO_SFN(2));//PCLK
		
		gpio_free(GPIO_CAM_nRST);
		gpio_free(GPIO_CAM_EN);
		printk("HI253 power on end\n");
	}
	else
	{	printk("HI253 power down start\n");
		//pull down RESETB
		if (gpio_request(GPIO_CAM_nRST, "GPF0_4") < 0)
		pr_err("failed gpio_request(GPF0_4) for camera control\n");
		gpio_direction_output(GPIO_CAM_nRST, 0);
		s3c_gpio_setpull(GPIO_CAM_nRST, S3C_GPIO_PULL_NONE);
		mdelay(700);
		//pull up CHIP_ ENABLEB
		if (gpio_request(GPIO_CAM_EN, "GPF0_5") < 0)
		pr_err("failed gpio_request(GPF0_5) for camera control\n");
		gpio_direction_output(GPIO_CAM_EN, 1);
		s3c_gpio_setpull(GPIO_CAM_EN, S3C_GPIO_PULL_NONE);

		gpio_free(GPIO_CAM_nRST);
		gpio_free(GPIO_CAM_EN);
		//MCLK stop
		s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(0));//MCLK
		//	gpio_direction_output(GPIO_CAM_MCLK, 0);
		mdelay(1);
		//open vdd
		regulator_disable(vdd28_cam_regulator); 
		regulator_disable(vdd18_cam_regulator); 
		regulator_disable(vdd12_cam_regulator); 
		udelay(10);
		printk("HI253 power down end\n");
	}


	return 0;
}


static inline struct HI253_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct HI253_state, sd);
}

static int HI253_reset(struct v4l2_subdev *sd)
{
	return HI253_init(sd, 0);
}

int DDI_I2C_Read(struct v4l2_subdev *sd, unsigned short reg,unsigned char reg_bytes, 
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
 * HI253 register structure : 2bytes address, 2bytes value
 * retry on write failure up-to 5 times
 */
static inline int HI253_write(struct v4l2_subdev *sd, unsigned short addr, u8 val)
{
	/*struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg[1];
	unsigned char reg[3];

	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 3;
	msg->buf = reg;

	reg[0] = addr >> 8;
	reg[1] = addr & 0xFF;
	reg[2] = val & 0xFF;

	if (i2c_transfer(client->adapter, msg, 1) != 1)
	{
		printk("HI253 i2c_transfer failed\n");
		return -EIO;
	}*/
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
static int __HI253_init_2bytes(struct v4l2_subdev *sd, unsigned short *reg[], int total)
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
		/*else if (item[0] == REG_ID)
		 {
		 DDI_I2C_Read(sd, 0x0001, 2, &bytes, 1); // 1byte read
		 printk("===== cam sensor HI253 ID : 0x%x ", bytes);
		 DDI_I2C_Read(sd, 0x0002, 2, &bytes, 1); // 1byte read
		 }*/
		else
		{	
			ret = HI253_write(sd, item[0], item[1]);
		}

		
	}
	if (ret < 0)
	v4l_info(client, "%s: register set failed\n", __func__);

	return ret;
}

/*
modify.lzy 
follow function which modify from  __HI253_init_2bytes
is used to write regs for setformat  
2013/3/22
*/
static int HI253_real_write_reg(struct v4l2_subdev *sd, unsigned short *reg[], int total)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = -EINVAL, i;
	unsigned short *item;

	for (i = 0; i < total; i++)
	{
		item = (unsigned short *) &reg[i];
		ret = HI253_write(sd, item[0], item[1]);
	}

	if (ret < 0)
	printk( "%s: register set failed\n", __func__);
	return ret;
}


static int HI253_i2c_write(struct v4l2_subdev *sd, unsigned char i2c_data[], unsigned char length)
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
static int HI253_write_regs(struct v4l2_subdev *sd, unsigned char regs[], int size)
{
#if 0
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int i, err;

	for (i = 0; i < size; i++)
	{
		err = HI253_i2c_write(sd, &regs[i], sizeof(regs[i]));
		if (err < 0)
			v4l_info(client, "%s: register set failed\n", __func__);
	}

	return 0;
#else
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int i, err;
	struct HI253_reg *pReg = (struct HI253_reg *)regs;

	for (i = 0; i < size/sizeof(struct HI253_reg); i++)
	{
		err = HI253_i2c_write(sd, pReg+i, sizeof(struct HI253_reg));
		if (err < 0)
			v4l_info(client, "%s: register set failed\n", __func__);
	}

	return 0;
#endif
}

static const char *HI253_querymenu_wb_preset[] =
{ "WB Tungsten", "WB Fluorescent", "WB sunny", "WB cloudy", NULL };

static const char *HI253_querymenu_effect_mode[] =
{ "Effect Sepia", "Effect Aqua", "Effect Monochrome", "Effect Negative", "Effect Sketch", NULL };

static const char *HI253_querymenu_ev_bias_mode[] =
{ "-3EV", "-2,1/2EV", "-2EV", "-1,1/2EV", "-1EV", "-1/2EV", "0", "1/2EV", "1EV", "1,1/2EV", "2EV", "2,1/2EV", "3EV", NULL };

static struct v4l2_queryctrl HI253_controls[] =
{
	{
		/*
		 * For now, we just support in preset type
		 * to be close to generic WB system,
		 * we define color temp range for each preset
		 */
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
		.maximum = ARRAY_SIZE(HI253_querymenu_wb_preset) - 2,
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
		.maximum = ARRAY_SIZE(HI253_querymenu_ev_bias_mode) - 2,
		.step = 1,
		.default_value =
		(ARRAY_SIZE(HI253_querymenu_ev_bias_mode) - 2) / 2,
		/* 0 EV */
	},
	{
		.id = V4L2_CID_CAMERA_EFFECT,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Image Effect",
		.minimum = 0,
		.maximum = ARRAY_SIZE(HI253_querymenu_effect_mode) - 2,
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

const char **HI253_ctrl_get_menu(u32 id)
{
	switch (id)
	{
	case V4L2_CID_WHITE_BALANCE_PRESET:
		return HI253_querymenu_wb_preset;

	case V4L2_CID_CAMERA_EFFECT:
		return HI253_querymenu_effect_mode;

	case V4L2_CID_EXPOSURE:
		return HI253_querymenu_ev_bias_mode;

	default:
		return v4l2_ctrl_get_menu(id);
	}
}

static inline struct v4l2_queryctrl const *HI253_find_qctrl(int id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(HI253_controls); i++)
		if (HI253_controls[i].id == id)
			return &HI253_controls[i];

	return NULL;
}

static int HI253_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(HI253_controls); i++)
	{
		if (HI253_controls[i].id == qc->id)
		{
			memcpy(qc, &HI253_controls[i], sizeof(struct v4l2_queryctrl));
			return 0;
		}
	}

	return -EINVAL;
}

static int HI253_querymenu(struct v4l2_subdev *sd, struct v4l2_querymenu *qm)
{
	struct v4l2_queryctrl qctrl;

	qctrl.id = qm->id;
	HI253_queryctrl(sd, &qctrl);

	return v4l2_ctrl_query_menu(qm, &qctrl, HI253_ctrl_get_menu(qm->id));
}

static int HI253_g_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;

	return err;
}

static int HI253_s_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
	
	int ret = -EINVAL;
	int i, err=0;
	unsigned short *item;
	struct HI253_state *state =to_state(sd);; 
	printk("lzy: %s,width = %d,height = %d\n",__func__,fmt->width,fmt->height);
	state->pix.width = fmt->width;
	state->pix.height = fmt->height;
	#if 0
	if(1600==fmt->width&&(1200==fmt->height))
	{	state->framesize_index=4;	
 		err = HI253_real_write_reg(sd, (unsigned short **) HI253_init_reg_1600_1200,HI253_init_reg_1600_1200S);	
	}
	else if(1280==fmt->width&&(720==fmt->height))		
	{	state->framesize_index=3;	
		err = HI253_real_write_reg(sd, (unsigned short **) HI253_init_reg_1280_720,HI253_init_reg_1280_720S);
	}
	else if(800==fmt->width&&(600==fmt->height))		
	{	state->framesize_index=2;	
		err = HI253_real_write_reg(sd, (unsigned short **) HI253_init_reg_800_600,HI253_init_reg_800_600S);
	}
	else if(640==fmt->width&&(480==fmt->height))		
	{	state->framesize_index=1;	
		err = HI253_real_write_reg(sd, (unsigned short **) HI253_init_reg_640_480,HI253_init_reg_640_480S);
	}
	else 		
	{	//for size 320x240 or other mess size
		err = HI253_real_write_reg(sd, (unsigned short **) HI253_init_reg_800_600,HI253_init_reg_800_600S);
	}

	if (err < 0)
	printk("ooh,God: %s, set regs failed: width = %d,height = %d\n",__func__,fmt->width,fmt->height);			
	printk("lzy: %s end \n",__func__);
	#endif
	#if 0
	if(fmt->width == 1600 && (fmt->height == 1200))
	{
		output_format_index = 1;  	
	}
	else 
	{
		output_format_index = 0;
	}

	if(output_format_index == 1)
	{
 		err = HI253_real_write_reg(sd, (unsigned short **) HI253_init_reg_1600_1200,HI253_init_reg_1600_1200S);	
	}
	else
	{
		err = HI253_real_write_reg(sd, (unsigned short **) HI253_init_reg_800_600,HI253_init_reg_800_600S);
	}
	if (err < 0)
	printk("ooh,God: %s, set regs failed: width = %d,height = %d\n",__func__,fmt->width,fmt->height);
	printk("lzy: %s end \n",__func__);
	return err;
	#endif
	return 0;

}
	


static int HI253_enum_framesizes(struct v4l2_subdev *sd, struct v4l2_frmsizeenum *fsize)
{
	struct HI253_state *state = to_state(sd);
	int num_entries = sizeof(HI253_capture_framesize_list) / sizeof(struct HI253_enum_framesize);
	struct HI253_enum_framesize *elem;
	int index = 0;
	int i = 0;

	/* The camera interface should read this value, this is the resolution
	 * at which the sensor would provide framedata to the camera i/f
	 *
	 * In case of image capture,
	 * this returns the default camera resolution (WVGA)
	 */
	 #if 0
	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;

	index = state->framesize_index;
	
	printk("%s : state->framesize_index=%d\n",__func__,state->framesize_index);
	
	for (i = 0; i < num_entries; i++)
	{
		elem = &HI253_capture_framesize_list[i];
		if (elem->index == index)
		{
			fsize->discrete.width = HI253_capture_framesize_list[index].width;
			fsize->discrete.height = HI253_capture_framesize_list[index].height;
			printk("%s : fsize->discrete.width= %d, fsize->discrete.height=%d \n ",__func__,fsize->discrete.width,fsize->discrete.height);
			return 0;
		}
	}
	#endif 
	#if 0
	if(output_format_index == 1)
	{
		fsize->discrete.width = 1600;
		fsize->discrete.height = 1200;
	}
	else
	{
		fsize->discrete.width = 800;
		fsize->discrete.height = 600;
	}
	printk("%s : fsize->discrete.width= %d, fsize->discrete.height=%d \n ",__func__,fsize->discrete.width,fsize->discrete.height);
	#endif
	return 0;
}

static int HI253_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_frmivalenum *fival)
{
	int err = 0;

	return err;
}

static int HI253_enum_fmt(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmtdesc)
{
	int err = 0;

	return err;
}

static int HI253_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;

	return err;
}

static int HI253_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0;

	dev_dbg(&client->dev, "%s\n", __func__);

	return err;
}

static int HI253_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0;

	dev_dbg(&client->dev, "%s: numerator %d, denominator: %d\n", __func__,
			param->parm.capture.timeperframe.numerator,
			param->parm.capture.timeperframe.denominator);

	return err;
}

static int HI253_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct HI253_state *state = to_state(sd);
	struct HI253_userset userset = state->userset;
	int err = 0;

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

static int HI253_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct HI253_state *state = to_state(sd);
	int err = 0;
	int value = ctrl->value;

	switch (ctrl->id)
	{

	case V4L2_CID_CAMERA_FLASH_MODE:
	case V4L2_CID_CAMERA_BRIGHTNESS:
		break;
	case V4L2_CID_CAMERA_WHITE_BALANCE:
		dev_dbg(&client->dev, "%s: V4L2_CID_CAMERA_WHITE_BALANCE\n", __func__);

		if (value <= WHITE_BALANCE_AUTO)
		{
			err = HI253_write_regs(sd, (unsigned char *) HI253_regs_awb_enable[value],
							sizeof(HI253_regs_awb_enable[value]));
		}
		else
		{
			err = HI253_write_regs(sd, (unsigned char *) HI253_regs_wb_preset[value - 2],
							sizeof(HI253_regs_wb_preset[value - 2]));
		}
		break;
	case V4L2_CID_CAMERA_EFFECT:
		dev_dbg(&client->dev, "%s: V4L2_CID_CAMERA_EFFECT\n", __func__);
		err = HI253_write_regs(sd, (unsigned char *) HI253_regs_color_effect[value - 1],
						sizeof(HI253_regs_color_effect[value - 1]));
		break;
	case V4L2_CID_CAMERA_ISO:
	case V4L2_CID_CAMERA_METERING:
		break;
	case V4L2_CID_CAMERA_CONTRAST: //do not support now
		dev_dbg(&client->dev, "%s: V4L2_CID_CAMERA_CONTRAST\n", __func__);
		err = HI253_write_regs(sd, (unsigned char *) HI253_regs_contrast_bias[value],
						sizeof(HI253_regs_contrast_bias[value]));
		break;
	case V4L2_CID_CAMERA_SATURATION: //do not support now
		dev_dbg(&client->dev, "%s: V4L2_CID_CAMERA_SATURATION\n", __func__);
		err = HI253_write_regs(sd, (unsigned char *) HI253_regs_saturation_bias[value],
						sizeof(HI253_regs_saturation_bias[value]));
		break;
	case V4L2_CID_CAMERA_SHARPNESS: //do not support now
		dev_dbg(&client->dev, "%s: V4L2_CID_CAMERA_SHARPNESS\n", __func__);
		err = HI253_write_regs(sd, (unsigned char *) HI253_regs_sharpness_bias[value],
						sizeof(HI253_regs_sharpness_bias[value]));
		break;
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
		err = HI253_reset(sd);
		break;
	case V4L2_CID_EXPOSURE:
		dev_dbg(&client->dev, "%s: V4L2_CID_EXPOSURE\n", __func__);
		err = HI253_write_regs(sd, (unsigned char *) HI253_regs_ev_bias[value],
						sizeof(HI253_regs_ev_bias[value]));
		break;
	default:
		err=0;
		dev_err(&client->dev, "%s: no such control\n", __func__);
		/* err = -EINVAL; */
		break;
	}

	if (err < 0)
		dev_dbg(&client->dev, "%s: vidioc_s_ctrl failed\n", __func__);

	return err;
}

static int HI253_init(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct HI253_state *state = to_state(sd);
	int err = -EINVAL, i;
	unsigned char sensorID[2] = {0};
	 state->framesize_index=2;
	printk("%s: camera initialization start\n", __func__);
	HI253_power( 1);
	
	//for(i =0;i<100;i++)
	//{
		DDI_I2C_Read(sd, 0x04, 1, sensorID, 1);
		//DDI_I2C_Read(sd, 0x0004, 2, sensorID+1, 1);
		HI253_write(sd, 0x0003, 0x10);
		DDI_I2C_Read(sd, 0x03, 1, sensorID+1, 1);
		printk("++++check HI253 sensor ID : %x++++%x\r\n", sensorID[0],sensorID[1]);
	//}
	err = __HI253_init_2bytes(sd, (unsigned short **) HI253_init_reg,
			HI253_INIT_REGS);

	if (err < 0)
	{
		/* This is preview fail */
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

/*
 * s_config subdev ops
 * With camera device, we need to re-initialize every single opening time
 * therefor, it is not necessary to be initialized on probe time.
 * except for version checking.
 * NOTE: version checking is optional
 */
static int HI253_s_config(struct v4l2_subdev *sd, int irq, void *platform_data)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct HI253_state *state = to_state(sd);
	struct hi253_platform_data *pdata;

	dev_info(&client->dev, "fetching platform data\n");

	pdata = client->dev.platform_data;

	if (!pdata)
	{
		dev_err(&client->dev, "%s: no platform data\n", __func__);
		return -ENODEV;
	}

	/*
	 * Assign default format and resolution
	 * Use configured default information in platform data
	 * or without them, use default information in driver
	 */
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
static int HI253_s_stream(struct v4l2_subdev *sd, int enable)
{
	return 0;
}

static const struct v4l2_subdev_core_ops HI253_core_ops =
{
	.init = HI253_init, /* initializing API */
	//.s_config = HI253_s_config, /* Fetch platform data */
	.queryctrl = HI253_queryctrl,
	.querymenu = HI253_querymenu,
	//.s_power = HI253_power,
	.g_ctrl = HI253_g_ctrl,
	.s_ctrl = HI253_s_ctrl,
};

static const struct v4l2_subdev_video_ops HI253_video_ops =
{
	.g_mbus_fmt = HI253_g_fmt,
	.s_mbus_fmt = HI253_s_fmt,
	.enum_framesizes = HI253_enum_framesizes,
	.enum_frameintervals = HI253_enum_frameintervals,
	//.enum_fmt = HI253_enum_fmt,
	//.try_fmt = HI253_try_fmt,
	.g_parm = HI253_g_parm,
	.s_parm = HI253_s_parm,
	.s_stream = HI253_s_stream,
};

static const struct v4l2_subdev_ops HI253_ops =
{
	.core = &HI253_core_ops,
	.video = &HI253_video_ops,
};

/*
 * HI253_probe
 * Fetching platform data is being done with s_config subdev call.
 * In probe routine, we just register subdev device
 */
static int HI253_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct HI253_state *state;
	struct v4l2_subdev *sd;
	int ret =0;
	printk("HI253_probe\n");
	state = kzalloc(sizeof(struct HI253_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;
	
	vdd18_cam_regulator = regulator_get(NULL, "vdd18_2m");
	if (IS_ERR(vdd18_cam_regulator)) {
		printk("%s: failed to get %s\n", __func__, "vdd18_2m");
		ret = -ENODEV;
		goto err_regulator;
	}
	
	vdd12_cam_regulator = regulator_get(NULL, "vdd12_2m");
	if (IS_ERR(vdd12_cam_regulator)) {
		printk("%s: failed to get %s\n", __func__, "vdd12_2m");
		ret = -ENODEV;
		goto err_regulator;
	}

	vdd28_cam_regulator = regulator_get(NULL, "vdd28_2m");
	if (IS_ERR(vdd28_cam_regulator)) {
		printk("%s: failed to get %s\n", __func__, "vdd28_2m");
		ret = -ENODEV;
		goto err_regulator;
	}

	sd = &state->sd;
	strcpy(sd->name, HI253_DRIVER_NAME);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &HI253_ops);
	dev_info(&client->dev, "HI253 has been probed\n");

	
	return 0;

err_regulator:
	regulator_put(vdd18_cam_regulator);	
	regulator_put(vdd12_cam_regulator);
	regulator_put(vdd28_cam_regulator);
	return ret;
}

static int HI253_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	printk("HI253_remove\n");
	HI253_power( 0);
	
	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id HI253_id[] = {
	{ HI253_DRIVER_NAME, 0 },
	{ }, 
};
MODULE_DEVICE_TABLE(i2c, HI253_id);

static struct i2c_driver hi253_i2c_driver =
{
	.driver = {
		.name = HI253_DRIVER_NAME,
	},
	.probe = HI253_probe,
	.remove = __devexit_p(HI253_remove),
	.id_table = HI253_id,
};

static int __init hi253_mod_init(void)
{
	printk("%s\n",__func__);
	return i2c_add_driver(&hi253_i2c_driver);
}

static void __exit hi253_mod_exit(void)
{
	printk("%s\n",__func__);
	i2c_del_driver(&hi253_i2c_driver);
}

module_init(hi253_mod_init);
module_exit(hi253_mod_exit);

MODULE_DESCRIPTION("Samsung Electronics HI253 UXGA camera driver");
MODULE_AUTHOR("Jinsung Yang <jsgood.yang@samsung.com>");
MODULE_LICENSE("GPL");



