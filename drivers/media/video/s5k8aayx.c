/* linux/drivers/media/video/s5k8aayx.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Driver for S5K8AAYX (SXGA camera) from Samsung Electronics
 * 1/6" 1.3Mp CMOS Image Sensor SoC with an Embedded Image Processor
 * supporting MIPI CSI-2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/soc_camera.h>
#include <media/soc_mediabus.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-common.h>
#include <linux/regulator/consumer.h>

//#include <media/v4l2-i2c-drv.h>
#include <media/s5k8aayx_platform.h>
#include <mach/max8997.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/regs-clock.h>

struct regulator *vdd18_cam_regulator = NULL;	
struct regulator *vdd28_cam_regulator = NULL;
struct regulator *vdd12_cam_regulator = NULL;

#ifdef CONFIG_VIDEO_SAMSUNG_V4L2
#include <linux/videodev2_samsung.h>
#endif

#include "s5k8aayx.h"
#define GPIO_CAM_MCLK    EXYNOS4212_GPJ1(3)


#define S5K8AAYX_DRIVER_NAME	"S5K8AAYX"

/* Default resolution & pixelformat. plz ref s5k8aayx_platform.h */
#define DEFAULT_FMT		V4L2_PIX_FMT_UYVY	/* YUV422 */

/*
 * Specification
 * Parallel : ITU-R. 656/601 YUV422, RGB565, RGB888 (Up to VGA), RAW10
 * Serial : MIPI CSI2 (single lane) YUV422, RGB565, RGB888 (Up to VGA), RAW10
 * Resolution : 1600 (H) x 1200 (V)
 * Image control : Brightness, Contrast, Saturation, Sharpness, Glamour
 * Effect : Mono, Negative, Sepia, Aqua, Sketch
 * FPS : 15fps @full resolution, 30fps @VGA, 
 * Max. pixel clock frequency : 85MHz(upto)
 * Internal PLL (6MHz to 54MHz input frequency)
 */

/* Camera functional setting values configured by user concept */
struct s5k8aayx_userset {
	signed int exposure_bias;	/* V4L2_CID_EXPOSURE */
	unsigned int ae_lock;
	unsigned int awb_lock;
	unsigned int auto_wb;	/* V4L2_CID_AUTO_WHITE_BALANCE */
	unsigned int manual_wb;	/* V4L2_CID_WHITE_BALANCE_PRESET */
	unsigned int wb_temp;	/* V4L2_CID_WHITE_BALANCE_TEMPERATURE */
	unsigned int effect;	/* Color FX (AKA Color tone) */
	unsigned int contrast;	/* V4L2_CID_CONTRAST */
	unsigned int saturation;	/* V4L2_CID_SATURATION */
	unsigned int sharpness;		/* V4L2_CID_SHARPNESS */
	unsigned int glamour;
};

struct s5k8aayx_state {
	struct s5k8aayx_platform_data *pdata;
	struct v4l2_subdev sd;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	struct s5k8aayx_userset userset;
	int freq;	/* MCLK in KHz */
	int is_mipi;
	int isize;
	int ver;
	int fps;
};
static int output_format_index = 0;
static int sensor_powered = 0;
static inline struct s5k8aayx_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct s5k8aayx_state, sd);
}
static int s5k8aayx_power(int flag)
{
	int err;
	printk("Robin: sensor is power %s\n",flag == 1 ?"on":"off");
	//Attention: Power On the all the camera module when system turn on
	//Here only control the reset&&powerdown pin
   //xiebin.wang@20110530
	/* Camera A */
	if(flag == 1)
	{	
		//poweron
		//EVT0 power up sequence
		//regulator_enable(vdd12_cam_regulator); 
	    //   udelay(10);
       	regulator_enable(vdd28_cam_regulator); 
	       udelay(10);
		regulator_enable(vdd12_cam_regulator); 
	       udelay(10);
		regulator_enable(vdd18_cam_regulator); 
	       udelay(10);
		
		//MCLK may need to add
		s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(2));//MCLK
		mdelay(1);
		
		//STBYN high
		err = gpio_request(EXYNOS4_GPF0(5), "GPF0");
		if (err)
			printk(KERN_ERR "#### failed to request GPF0_5 ####\n");
		s3c_gpio_setpull(EXYNOS4_GPF0(5), S3C_GPIO_PULL_NONE);
		gpio_direction_output(EXYNOS4_GPF0(5), 1);
		mdelay(1);

		//RSTN high
		err = gpio_request(EXYNOS4_GPF0(4), "GPF0");
		if (err)
			printk(KERN_ERR "#### failed to request GPF0_4 ####\n");
		s3c_gpio_setpull(EXYNOS4_GPF0(4), S3C_GPIO_PULL_NONE);
		gpio_direction_output(EXYNOS4_GPF0(4), 1);	
		mdelay(1);
		
		gpio_free(EXYNOS4_GPF0(4));
		gpio_free(EXYNOS4_GPF0(5));
		msleep(1);
		//VDD_PLL
		//PMIC_Control_Set(PMIC_CONTROL_VDD28_CAM,POWERON);
	
	}
	else
	{
		//PMIC_Control_Set(PMIC_CONTROL_VDD28_CAM,POWEROFF);
		
		
		err = gpio_request(EXYNOS4_GPF0(4), "GPF0");
		if (err)
			printk(KERN_ERR "#### failed to request GPF0_4 ####\n");
		s3c_gpio_setpull(EXYNOS4_GPF0(4), S3C_GPIO_PULL_NONE);
		gpio_direction_output(EXYNOS4_GPF0(4), 0);
		gpio_free(EXYNOS4_GPF0(4));
		mdelay(1);
		//powerdown
		err = gpio_request(EXYNOS4_GPF0(5), "GPF0");
		if (err)
			printk(KERN_ERR "#### failed to request GPE0_5 ####\n");
		s3c_gpio_setpull(EXYNOS4_GPF0(5), S3C_GPIO_PULL_NONE);
		gpio_direction_output(EXYNOS4_GPF0(5), 0);
		gpio_free(EXYNOS4_GPF0(5));
		mdelay(1);

		//bk tests
		//MCLK stop
		s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(0));//MCLK
	//	gpio_direction_output(GPIO_CAM_MCLK, 0);
		mdelay(1);

		regulator_disable(vdd18_cam_regulator); 
	       udelay(10);
		regulator_disable(vdd12_cam_regulator); 
	       udelay(10);
		regulator_disable(vdd28_cam_regulator); 
	       udelay(10);
		
		//PMIC_Control_Set(PMIC_CONTROL_VDD18_CAM,POWEROFF);
	
	}

	return 0;

}
	
/*
 * S5K8AAYX register structure : 2bytes address, 2bytes value
 * retry on write failure up-to 5 times
 */
static inline int s5k8aayx_write(struct v4l2_subdev *sd, u16 addr, u16 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	printk("bk:get i2c client\n");
	struct i2c_msg msg[1];
	unsigned char reg[4];
	int err = 0;
	int retry = 0;


	if (!client->adapter)
		return -ENODEV;

again:

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 4;
	msg->buf = reg;

	reg[0] = addr >> 8;
	reg[1] = addr & 0xff;
	reg[2] = val >> 8;
	reg[3] = val & 0xff;

	err = i2c_transfer(client->adapter, msg, 1);
//	printk("bk err:%d",err);
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

static inline int s5k8aayx_read(struct v4l2_subdev *sd, u16 addr,u16 *val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg[2];
	unsigned char reg[4];
	unsigned char read_reg[4];
	
	int err = 0;
	int retry = 0;


	if (!client->adapter)
		return -ENODEV;

again:

	reg[0] = addr >> 8;
	reg[1] = addr & 0xff;
	
	msg[0].addr  = client->addr;
	msg[0].flags = 0;
	msg[0].len   = 2;
	msg[0].buf   = reg;

	msg[1].addr  = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len   = 2;
	msg[1].buf   = read_reg;

	/*
	 * if return value of this function is < 0,
	 * it mean error.
	 * else, under 16bit is valid data.
	 */
	err = i2c_transfer(client->adapter, msg, 2);
	if (err == 2)
	{
		*val = (read_reg[0]<<8 | read_reg[1])&0xffff;
		return err;
	}
		/* abnormal case: retry 5 times */
	if (retry < 5) {
		dev_err(&client->dev, "%s: address: 0x%02x%02x, " \
			"read error.\n", __func__, \
			reg[0], reg[1]);
		retry++;
		goto again;
	}
	

	return err;
}



static int s5k8aayx_i2c_write(struct v4l2_subdev *sd, unsigned char i2c_data[],
				unsigned char length)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[length], i;
	struct i2c_msg msg = {client->addr, 0, length, buf};

	for (i = 0; i < length; i++)
		buf[i] = i2c_data[i];

	return i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
}

static const char *s5k8aayx_querymenu_wb_preset[] = {
	"WB Tungsten", "WB Fluorescent", "WB sunny", "WB cloudy", NULL
};

static const char *s5k8aayx_querymenu_effect_mode[] = {
	"Effect Sepia", "Effect Aqua", "Effect Monochrome",
	"Effect Negative", "Effect Sketch", NULL
};

static const char *s5k8aayx_querymenu_ev_bias_mode[] = {
	"-3EV",	"-2,1/2EV", "-2EV", "-1,1/2EV",
	"-1EV", "-1/2EV", "0", "1/2EV",
	"1EV", "1,1/2EV", "2EV", "2,1/2EV",
	"3EV", NULL
};

static struct v4l2_queryctrl s5k8aayx_controls[] = {
	{
		/*
		 * For now, we just support in preset type
		 * to be close to generic WB system,
		 * we define color temp range for each preset
		 */
		.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "White balance temperature",
		.minimum = 0,
		.maximum = 10000,
		.step = 1,
		.default_value = 0,	/* FIXME */
		.flags = V4L2_CTRL_FLAG_DISABLED,
	}, {
		.id = V4L2_CID_WHITE_BALANCE_PRESET,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "White balance preset",
		.minimum = 0,
		.maximum = ARRAY_SIZE(s5k8aayx_querymenu_wb_preset) - 2,
		.step = 1,
		.default_value = 0,
		.flags = V4L2_CTRL_FLAG_DISABLED,
	}, {
		.id = V4L2_CID_AUTO_WHITE_BALANCE,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Auto white balance",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
		.flags = V4L2_CTRL_FLAG_DISABLED,
	}, {
		.id = V4L2_CID_EXPOSURE,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Exposure bias",
		.minimum = 0,
		.maximum = ARRAY_SIZE(s5k8aayx_querymenu_ev_bias_mode) - 2,
		.step = 1,
		.default_value = \
		(ARRAY_SIZE(s5k8aayx_querymenu_ev_bias_mode) - 2) / 2,/* 0 EV */
		.flags = V4L2_CTRL_FLAG_DISABLED,
	}, {
		.id = V4L2_CID_COLORFX,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Image Effect",
		.minimum = 0,
		.maximum = ARRAY_SIZE(s5k8aayx_querymenu_effect_mode) - 2,
		.step = 1,
		.default_value = 0,
		.flags = V4L2_CTRL_FLAG_DISABLED,
	}, {
		.id = V4L2_CID_CONTRAST,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Contrast",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
		.flags = V4L2_CTRL_FLAG_DISABLED,
	}, {
		.id = V4L2_CID_SATURATION,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Saturation",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
		.flags = V4L2_CTRL_FLAG_DISABLED,
	}, {
		.id = V4L2_CID_SHARPNESS,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Sharpness",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
		.flags = V4L2_CTRL_FLAG_DISABLED,
	},
};

const char **s5k8aayx_ctrl_get_menu(u32 id)
{
	printk("Robin: %s\n",__func__);

	switch (id) {
	case V4L2_CID_WHITE_BALANCE_PRESET:
		return s5k8aayx_querymenu_wb_preset;

	case V4L2_CID_COLORFX:
		return s5k8aayx_querymenu_effect_mode;

	case V4L2_CID_EXPOSURE:
		return s5k8aayx_querymenu_ev_bias_mode;

	default:
		return v4l2_ctrl_get_menu(id);
	}
}

static inline struct v4l2_queryctrl const *s5k8aayx_find_qctrl(int id)
{
	int i;
	printk("Robin: %s\n",__func__);

	for (i = 0; i < ARRAY_SIZE(s5k8aayx_controls); i++)
		if (s5k8aayx_controls[i].id == id)
			return &s5k8aayx_controls[i];

	return NULL;
}

static int s5k8aayx_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	int i;
	printk("Robin: %s\n",__func__);

	for (i = 0; i < ARRAY_SIZE(s5k8aayx_controls); i++) {
		if (s5k8aayx_controls[i].id == qc->id) {
			memcpy(qc, &s5k8aayx_controls[i], \
				sizeof(struct v4l2_queryctrl));
			return 0;
		}
	}

	return -EINVAL;
}

static int s5k8aayx_querymenu(struct v4l2_subdev *sd, struct v4l2_querymenu *qm)
{
	struct v4l2_queryctrl qctrl;
	printk("Robin: %s\n",__func__);

	qctrl.id = qm->id;
	s5k8aayx_queryctrl(sd, &qctrl);

	return v4l2_ctrl_query_menu(qm, &qctrl, s5k8aayx_ctrl_get_menu(qm->id));
}

/*
 * Clock configuration
 * Configure expected MCLK from host and return EINVAL if not supported clock
 * frequency is expected
 *	freq : in Hz
 *	flag : not supported for now
 */
static int s5k8aayx_s_crystal_freq(struct v4l2_subdev *sd, u32  freq, u32 flags)
{
	int err = -EINVAL;
	printk("Robin: %s\n",__func__);

	return err;
}



static int s5k8aayx_g_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
	int err = 0;
	printk("Robin: %s\n",__func__);

	return err;
}

static int s5k8aayx_s_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
       struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0;
	int i;
	printk("Robin: %s,width = %d,height = %d\n",__func__,fmt->width,fmt->height);

	/*if(fmt->width == 1600 && (fmt->height == 1200))
	{
		output_format_index = 1;  
		
	}
	else 
	{
		output_format_index = 0;
	}

	if(output_format_index == 1)
	{
		for (i = 0; i < S5K8AAYX_UXGA_REGS; i++) {
				if (s5k8aayx_capture_uxga[i][0] == REG_DELAY) {
				msleep(s5k8aayx_capture_uxga[i][1]);
				err = 0;
				} else {
				err = s5k8aayx_write(sd,s5k8aayx_capture_uxga[i][0],s5k8aayx_capture_uxga[i][1]);
				}

				if (err < 0)
					printk( "%s: register set failed\n", \
					__func__);
			}

	}
	else
	{
		for (i = 0; i < S5K8AAYX_VGA_REGS; i++) {
			if (s5k8aayx_preview_vga[i][0] == REG_DELAY) {
				msleep(s5k8aayx_preview_vga[i][1]);
				err = 0;
			} else {
				err = s5k8aayx_write(sd,s5k8aayx_preview_vga[i][0],s5k8aayx_preview_vga[i][1]);
			}

		if (err < 0)
			v4l_info(client, "%s: register set failed\n", \
			__func__);
		}
	}*/
	
	printk("Robin: %s\n",__func__);

	return err;
}
static int s5k8aayx_enum_framesizes(struct v4l2_subdev *sd,
					struct v4l2_frmsizeenum *fsize)
{
	int err = 0;
	printk("Robin: %s\n",__func__);
	/*
	if(output_format_index == 1)
	{
		fsize->discrete.width = 1600;
		fsize->discrete.height = 1200;
	}
	else
	{
		fsize->discrete.width = 640;
		fsize->discrete.height = 480;
	}
	*/
	return err;
}

static int s5k8aayx_enum_frameintervals(struct v4l2_subdev *sd,
					struct v4l2_frmivalenum *fival)
{
	int err = 0;
	printk("Robin: %s\n",__func__);

	return err;
}

static int s5k8aayx_enum_fmt(struct v4l2_subdev *sd,
				struct v4l2_fmtdesc *fmtdesc)
{
	int err = 0;
	printk("Robin: %s\n",__func__);

	return err;
}

static int s5k8aayx_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;
	printk("Robin: %s\n",__func__);

	return err;
}

static int s5k8aayx_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	int err = 0;
	printk("Robin: %s\n",__func__);

	return err;
}

static int s5k8aayx_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	int err = 0;
	printk("Robin: %s\n",__func__);

	return err;
}

static int s5k8aayx_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5k8aayx_state *state = to_state(sd);
	struct s5k8aayx_userset userset = state->userset;
	int err = -EINVAL;
	printk("Robin: %s\n",__func__);

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		ctrl->value = userset.exposure_bias;
		err = 0;
		break;
	case V4L2_CID_AUTO_WHITE_BALANCE:
		ctrl->value = userset.auto_wb;
		err = 0;
		break;
	case V4L2_CID_WHITE_BALANCE_PRESET:
		ctrl->value = userset.manual_wb;
		err = 0;
		break;
	case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
		ctrl->value = userset.wb_temp;
		err = 0;
		break;
	case V4L2_CID_COLORFX:
		ctrl->value = userset.effect;
		err = 0;
		break;
	case V4L2_CID_CONTRAST:
		ctrl->value = userset.contrast;
		err = 0;
		break;
	case V4L2_CID_SATURATION:
		ctrl->value = userset.saturation;
		err = 0;
		break;
	case V4L2_CID_SHARPNESS:
		ctrl->value = userset.saturation;
		err = 0;
		break;
	default:
		dev_err(&client->dev, "%s: no such ctrl\n", __func__);
		break;
	}

	return err;
}

static int s5k8aayx_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	printk("Robin: %s\n",__func__);

	return 0;

}

static int s5k8aayx_init(struct v4l2_subdev *sd, u32 val)
{
	
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL, i;
	u16 chipid;
	printk("Robin :%s: camera initialization start\n", __func__);

	
	if(!sensor_powered)
	{
		s5k8aayx_power(1);
		//printk("bk  power finished\n");
		sensor_powered = 1;
		s5k8aayx_write(sd,0x002C,0x0000);
		s5k8aayx_write(sd,0x002E,0x0040);
		s5k8aayx_read(sd,0x0F12,&chipid);
		switch(chipid)
		{
			case 0x08AA:
				printk("It's S5K8AAYX\n");
				break;
			default:
				printk("!!!!!Front Sensor Id is Error\n");
				break;
		}
		for (i = 0; i < S5K8AAYX_INIT_REGS; i++) {
				if (s5k8aayx_init_reg[i][0] == REG_DELAY) {
					msleep(s5k8aayx_init_reg[i][1]);
					err = 0;
				} else {
					err = s5k8aayx_write(sd,s5k8aayx_init_reg[i][0],s5k8aayx_init_reg[i][1]);
				}

			if (err < 0) // zxz 2012-3-20 for 2M
			{
				v4l_info(client, "%s: register set failed\n", \
				__func__);
			 	 sensor_powered = 0;
			   }
		}

		

	}
	
	/*if(output_format_index == 1)
	{
		for (i = 0; i < S5K8AAYX_UXGA_REGS; i++) {
				if (s5k8aayx_capture_uxga[i][0] == REG_DELAY) {
				msleep(s5k8aayx_capture_uxga[i][1]);
				err = 0;
				} else {
				err = s5k8aayx_write(sd,s5k8aayx_capture_uxga[i][0],s5k8aayx_capture_uxga[i][1]);
				}

				if (err < 0)
					printk( "%s: register set failed\n", \
					__func__);
			}

	}
	else
	{
		for (i = 0; i < S5K8AAYX_VGA_REGS; i++) {
			if (s5k8aayx_preview_vga[i][0] == REG_DELAY) {
				msleep(s5k8aayx_preview_vga[i][1]);
				err = 0;
			} else {
				err = s5k8aayx_write(sd,s5k8aayx_preview_vga[i][0],s5k8aayx_preview_vga[i][1]);
			}

		if (err < 0)
			v4l_info(client, "%s: register set failed\n", \
			__func__);
		}
	}*/	

	if (err < 0) {
		v4l_err(client, "%s: camera initialization failed\n", \
			__func__);
		return -EIO;	/* FIXME */
	}
	//printk("bk : init ok");

	return 0;
}

/*
 * s_config subdev ops
 * With camera device, we need to re-initialize every single opening time
 * therefor, it is not necessary to be initialized on probe time.
 * except for version checking.
 * NOTE: version checking is optional
 */
static int s5k8aayx_s_config(struct v4l2_subdev *sd, int irq, void *platform_data)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5k8aayx_state *state = to_state(sd);
	struct s5k8aayx_platform_data *pdata;
	int i;
	u16 chipid;
	int err = 0;
	
	dev_info(&client->dev, "fetching platform data\n");
	printk("Robin: %s\n",__func__);

	pdata = client->dev.platform_data;

	if (!pdata) {
		dev_err(&client->dev, "%s: no platform data\n", __func__);
		return -ENODEV;
	}

	/*
	 * Assign default format and resolution
	 * Use configured default information in platform data
	 * or without them, use default information in driver
	 */
	if (!(pdata->default_width && pdata->default_height)) {
		/* TODO: assign driver default resolution */
	} else {
		state->pix.width = pdata->default_width;
		state->pix.height = pdata->default_height;
	}

	if (!pdata->pixelformat)
		state->pix.pixelformat = DEFAULT_FMT;
	else
		state->pix.pixelformat = pdata->pixelformat;

	if (!pdata->freq)
		state->freq = 24000000;	/* 24MHz default */
	else
		state->freq = pdata->freq;

	if (!pdata->is_mipi) {
		state->is_mipi = 0;
		dev_info(&client->dev, "parallel mode\n");
	} else
		state->is_mipi = pdata->is_mipi;
	
	return 0;
}

static int s5k8aayx_sleep(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0, i;

	v4l_info(client, "%s: sleep mode\n", __func__);

	for (i = 0; i < S5K8AAYX_SLEEP_REGS; i++) {
		if (s5k8aayx_sleep_reg[i][0] == REG_DELAY) {
			mdelay(s5k8aayx_sleep_reg[i][1]);
			err = 0;
		} else {
			err = s5k8aayx_write(sd, s5k8aayx_sleep_reg[i][0], \
				s5k8aayx_sleep_reg[i][1]);
		}

		if (err < 0)
			v4l_info(client, "%s: register set failed\n", __func__);
	}

	if (err < 0) {
		v4l_err(client, "%s: sleep failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int s5k8aayx_wakeup(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0, i;

	v4l_info(client, "%s: wakeup mode\n", __func__);

	for (i = 0; i < S5K8AAYX_WAKEUP_REGS; i++) {
		if (s5k8aayx_wakeup_reg[i][0] == REG_DELAY) {
			mdelay(s5k8aayx_wakeup_reg[i][1]);
			err = 0;
		} else {
			err = s5k8aayx_write(sd, s5k8aayx_wakeup_reg[i][0], \
				s5k8aayx_wakeup_reg[i][1]);
		}

		if (err < 0)
			v4l_info(client, "%s: register set failed\n", __func__);
	}

	if (err < 0) {
		v4l_err(client, "%s: wake up failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int s5k8aayx_s_stream(struct v4l2_subdev *sd, int enable)
{
	return enable ? s5k8aayx_wakeup(sd) : s5k8aayx_sleep(sd);
}


static const struct v4l2_subdev_core_ops s5k8aayx_core_ops = {
	.init = s5k8aayx_init,	/* initializing API */
	//.s_config = s5k8aayx_s_config,	/* Fetch platform data */
	.queryctrl = s5k8aayx_queryctrl,
	.querymenu = s5k8aayx_querymenu,
	.g_ctrl = s5k8aayx_g_ctrl,
	.s_ctrl = s5k8aayx_s_ctrl,
};

static const struct v4l2_subdev_video_ops s5k8aayx_video_ops = {
	.s_crystal_freq = s5k8aayx_s_crystal_freq,
	.g_mbus_fmt = s5k8aayx_g_fmt,
	.s_mbus_fmt = s5k8aayx_s_fmt,
	.enum_framesizes = s5k8aayx_enum_framesizes,
	.enum_frameintervals = s5k8aayx_enum_frameintervals,
	//.enum_fmt = s5k8aayx_enum_fmt,
	//.try_fmt = s5k8aayx_try_fmt,
	.g_parm = s5k8aayx_g_parm,
	.s_parm = s5k8aayx_s_parm,
	.s_stream = s5k8aayx_s_stream,
};

static const struct v4l2_subdev_ops s5k8aayx_ops = {
	.core = &s5k8aayx_core_ops,
	.video = &s5k8aayx_video_ops,
};

/*
 * s5k8aayx_probe
 * Fetching platform data is being done with s_config subdev call.
 * In probe routine, we just register subdev device
 */
static int s5k8aayx_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct s5k8aayx_state *state;
	struct v4l2_subdev *sd;
	int ret = 0;
	printk("Robin: %s\n",__func__);

	state = kzalloc(sizeof(struct s5k8aayx_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

	//vdda  power on
	//printk("bk: start regulator_get\n");
	vdd28_cam_regulator = regulator_get(NULL, "vdd28_2m");
		
		if (IS_ERR(vdd28_cam_regulator)) {
			printk("%s: failed to get %s\n", __func__, "vdd28_2m");
			ret = -ENODEV;
			goto err_regulator;
		}
	//vddd power on
	vdd12_cam_regulator = regulator_get(NULL, "vdd12_2m");
	
	if (IS_ERR(vdd12_cam_regulator)) {
		printk("%s: failed to get %s\n", __func__, "vdd12_2m");
		ret = -ENODEV;
		goto err_regulator;
	}
	//VDDIO power on
	vdd18_cam_regulator = regulator_get(NULL, "vdd18_2m");
	
	if (IS_ERR(vdd18_cam_regulator)) {
		printk("%s: failed to get %s\n", __func__, "vdd18_2m");
		ret = -ENODEV;
		goto err_regulator;
	}
	//regulator_enable(vdd18_cam_regulator); 	

	
	//regulator_enable(vdd28_cam_regulator); 
	
	//regulator_enable(vdda28_cam_regulator); 

	sd = &state->sd;
	strcpy(sd->name, S5K8AAYX_DRIVER_NAME);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &s5k8aayx_ops);

	dev_info(&client->dev, "s5k8aayx has been probed\n");
	return 0;
err_regulator:
	regulator_put(vdd12_cam_regulator);
	regulator_put(vdd18_cam_regulator);	
	regulator_put(vdd28_cam_regulator);
}


static int s5k8aayx_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));

	s5k8aayx_power(0);
	sensor_powered = 0;
	regulator_put(vdd12_cam_regulator);
	regulator_put(vdd18_cam_regulator);	
	regulator_put(vdd28_cam_regulator);
	return 0;
}



static const struct i2c_device_id s5k8aayx_id[] = {
	{ S5K8AAYX_DRIVER_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, s5k8aayx_id);

static struct i2c_driver s5k8aayx_i2c_driver = {
	.driver = {
		.name	= S5K8AAYX_DRIVER_NAME,
	},
	.probe		= s5k8aayx_probe,
	.remove		= s5k8aayx_remove,
	.id_table	= s5k8aayx_id,
};


static int __init s5k8aayx_mod_init(void)
{
	printk("Robin: %s\n",__func__);
	return i2c_add_driver(&s5k8aayx_i2c_driver);
}

static void __exit s5k8aayx_mod_exit(void)
{
	printk("Robin: %s\n",__func__);
	i2c_del_driver(&s5k8aayx_i2c_driver);
}
module_init(s5k8aayx_mod_init);
module_exit(s5k8aayx_mod_exit);

MODULE_DESCRIPTION("SSCR S5K8AAYX UXGA camera driver");
MODULE_AUTHOR("Robin Wang <xiebin.wang@samsung.com>");
MODULE_LICENSE("GPL");

