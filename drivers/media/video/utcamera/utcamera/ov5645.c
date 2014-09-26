

/* linux/drivers/media/video/ov5645.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *	         http://www.samsung.com/
 *
 * Driver for ov5645 (UXGA camera) from Samsung Electronics
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
#include "ov5645.h"

#ifdef CONFIG_VIDEO_SAMSUNG_V4L2
#include <linux/videodev2_samsung.h>
#endif


static int output_format_index = 0;
#define OV5645_DRIVER_NAME	"OV5645"

/* Default resolution & pixelformat. plz ref ov5645_platform.h */
#define DEFAULT_FMT		V4L2_PIX_FMT_UYVY	/* YUV422 */


#define dbg_fun     printk("*** func:%s\n",__FUNCTION__)
#define dbg_int(i)  printk("*** func:%s,i = %d\n",__FUNCTION__,i)
#define dbg_str(i)  printk("*** func:%s,i = %s\n",__FUNCTION__,i)



static int ov5645_init(struct v4l2_subdev *sd, u32 val);

struct ov5645_userset
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

struct ov5645_state
{
	struct ov5645_platform_data *pdata;
	struct v4l2_subdev sd;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	struct ov5645_userset userset;
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
	ov5645_PREVIEW_XGA,
};

enum ov5645_capture_frame_size {
	
//	ov5645_CAPTURE_QVGA=0, /*320x240*/
	ov5645_CAPTURE_VGA =0,	/* 640x480 */
	ov5645_CAPTURE_SVGA,    /*800X600*/
	ov5645_CAPTURE_1MP,	/* 1280x720 */
	ov5645_CAPTURE_2MP,	/* UXGA  - 1600x1200 */
	ov5645_CAPTURE_1080P,	
	ov5645_CAPTURE_5MP, /*2592x1944*/
};


struct ov5645_enum_framesize
{
	unsigned int index;
	unsigned int width;
	unsigned int height;
};
struct ov5645_enum_framesize ov5645_capture_framesize_list[] = {
	
	   {ov5645_CAPTURE_VGA,   640,  480 },
	   {ov5645_CAPTURE_1MP,   1280,  720 },
	   {ov5645_CAPTURE_2MP, 1600, 1200 },	
	   {ov5645_CAPTURE_1080P, 1920, 1080}, 
	   {ov5645_CAPTURE_5MP, 2592, 1944 },	 
 };

//#define GPIO_CAMERA_POWER  EXYNOS4212_GPM3(0)
#define GPIO_CAMERA_POWER  GPIO_EXAXP22(1)
//#define GPIO_CAMERA_PD1    EXYNOS4_GPL0(1)
#define GPIO_CAMERA_PD0    EXYNOS4_GPL0(0)
#define GPIO_CAMERA_FLASH  EXYNOS4_GPL2(1)
#define GPIO_CAMERA_RESET  EXYNOS4212_GPJ1(4)



static int ov5645_power11( int enable)
{	
	int ret=0;
	struct regulator *vdd28_cam_regulator = NULL;
	struct regulator *vddaf_cam_regulator = NULL;
	printk("**1* fun: ov5645_sensor_power ,enable = %d\n",enable);
	if (enable) {
		gpio_direction_output(GPIO_CAMERA_POWER, 1);

		vdd28_cam_regulator=regulator_get(NULL, "dldo2_cam_avdd_2v8");
		regulator_enable(vdd28_cam_regulator);
		regulator_put(vdd28_cam_regulator);

		vddaf_cam_regulator=regulator_get(NULL, "vdd_eldo3_18");
		regulator_enable(vddaf_cam_regulator);
		regulator_put(vddaf_cam_regulator);

		
		gpio_direction_output(GPIO_CAMERA_PD0, 0);
		gpio_direction_output(GPIO_CAMERA_RESET, 1);
		usleep_range(5000, 5000);  // must
		gpio_direction_output(GPIO_CAMERA_PD0, 1);
		usleep_range(2000, 3000);  // must
		gpio_direction_output(GPIO_CAMERA_RESET, 0);
		usleep_range(4500, 5000);  // must
		gpio_direction_output(GPIO_CAMERA_RESET, 1);
		usleep_range(1500, 1500);  // must
	} else {
		gpio_direction_output(GPIO_CAMERA_POWER, 0);
		vdd28_cam_regulator=regulator_get(NULL, "dldo2_cam_avdd_2v8");
		regulator_disable(vdd28_cam_regulator);
		regulator_put(vdd28_cam_regulator);

		vddaf_cam_regulator=regulator_get(NULL, "vdd_eldo3_18");
		regulator_disable(vddaf_cam_regulator);
		regulator_put(vddaf_cam_regulator);
		gpio_direction_output(GPIO_CAMERA_PD0, 0);
	}

	return ret;
}

static int ov5645_power( int enable)
{
	return 0;
}

static inline struct ov5645_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct ov5645_state, sd);
}

static int ov5645_reset(struct v4l2_subdev *sd)
{
	dbg_fun;

	return ov5645_init(sd, 0);
}

static int reg_read_16(struct i2c_client *client, u16 reg, u8 *val)
{
	int ret;
	/* We have 16-bit i2c addresses - care for endianess */
	unsigned char data[2] = { reg >> 8, reg & 0xff };

	ret = i2c_master_send(client, data, 2);
	if (ret < 2) {
		dev_err(&client->dev, "%s: i2c read error, reg: %x\n",
			__func__, reg);
		return ret < 0 ? ret : -EIO;
	}

	ret = i2c_master_recv(client, val, 1);
	if (ret < 1) {
		dev_err(&client->dev, "%s: i2c read error, reg: %x\n",
				__func__, reg);
		return ret < 0 ? ret : -EIO;
	}
	return 0;
}

static int reg_read_8(struct i2c_client *client, u16 reg, u8 *val)
{
	int ret;
	/* We have 16-bit i2c addresses - care for endianess */
	unsigned char data[1] = { reg & 0xff };

	ret = i2c_master_send(client, data, 1);
	if (ret < 1) {
		dev_err(&client->dev, "%s: i2c read error, reg: %x\n",
			__func__, reg);
		return ret < 0 ? ret : -EIO;
	}

	ret = i2c_master_recv(client, val, 1);
	if (ret < 1) {
		dev_err(&client->dev, "%s: i2c read error, reg: %x\n",
				__func__, reg);
		return ret < 0 ? ret : -EIO;
	}
	return 0;
}

static int reg_write_16(struct i2c_client *client, u16 reg, u8 val)
{
	int ret;
	unsigned char data[3] = { reg >> 8, reg & 0xff, val };

	ret = i2c_master_send(client, data, 3);
	if (ret < 3) {
		dev_err(&client->dev, "%s: i2c write error, reg: %x\n",
			__func__, reg);
		return ret < 0 ? ret : -EIO;
	}

	return 0;
}

static int reg_write_8(struct i2c_client *client, u16 reg, u16 val16)
{
	int ret;
	unsigned char data[2] = { reg & 0xff, val16 & 0xff };

	ret = i2c_master_send(client, data, 2);
	if (ret < 2) {
		dev_err(&client->dev, "%s: i2c write error, reg: %x\n",
			__func__, reg);
		return ret < 0 ? ret : -EIO;
	}
	return 0;
}

static int i2c_write_array_8(struct i2c_client *client,
				struct hm2056_reg  *vals)
{
	while(vals->u16RegAddr != HM5056_TABLE_END) {

		int ret = reg_write_8(client, vals->u16RegAddr, vals->u8Val);

		if (ret < 0)
			return ret;

		#if 0 // for test write sensor reg
			//mdelay(50);
			uint8_t value = -1;;
	
			ret = reg_read_8(client, vals->u16RegAddr, &value);
			if(vals->u8Val != (value & 0xff))
				printk("[addr]%x, [R] reg[0x:%x] [W]:value:[0x%x]  [R]value[:0x%x]\n",client->addr, vals->u16RegAddr,vals->u8Val,value);
		#endif
		vals++;
	}
	return 0;
}

static int i2c_write_array_16(struct i2c_client *client,
				struct hm2056_reg  *vals)
{
	uint i = 0;

	while(vals->u16RegAddr != HM5056_TABLE_END) {

		int ret = reg_write_16(client, vals->u16RegAddr, vals->u8Val);

		if (ret < 0)
			return ret;

		#if 0 // for test write sensor reg
			//mdelay(50);
			uint8_t value = -1;;

			ret = reg_read_16(client, vals->u16RegAddr, &value);
			if(vals->u8Val != (value & 0xff))
				printk("[addr]%x, [R] reg[0x:%x] [W]:value:[0x%x]  [R]value[:0x%x]\n",client->addr, vals->u16RegAddr,vals->u8Val,value);
		#endif
		vals++;
	}
	dev_dbg(&client->dev, "Register list loaded\n");

	return 0;
}

#if 0
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
 * ov5645 register structure : 2bytes address, 2bytes value
 * retry on write failure up-to 5 times
 */
static inline int ov5645_write(struct v4l2_subdev *sd, unsigned short addr, u8 val)
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
static int __ov5645_init_2bytes(struct v4l2_subdev *sd, unsigned short *reg[], int total)
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
			ret = ov5645_write(sd, item[0], item[1]);
		}

		
	}
	if (ret < 0)
	v4l_info(client, "%s: register set failed\n", __func__);

	return ret;
}

static int ov5645_read(struct v4l2_subdev *sd, unsigned char addr)
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

	printk( "ov5645: read 0x%02x = 0x%02x\n", addr, buffer[0]);

	return (buffer[0]);
}


static int ov5645_real_write_reg(struct v4l2_subdev *sd, unsigned short *reg[], int total)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = -EINVAL, i;
	unsigned short *item;

	for (i = 0; i < total; i++)
	{
		item = (unsigned short *) &reg[i];
		ret = ov5645_write(sd, item[0], item[1]);
	}

	if (ret < 0)
	printk( "%s: register set failed\n", __func__);
	return ret;
}


static int ov5645_i2c_write(struct v4l2_subdev *sd, unsigned char i2c_data[], unsigned char length)
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
static int ov5645_write_regs(struct v4l2_subdev *sd, unsigned char regs[], int size)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int i, err;
	struct ov5645_reg *pReg = (struct ov5645_reg *)regs;

	for (i = 0; i < size/sizeof(struct ov5645_reg); i++)
	{
		err = ov5645_i2c_write(sd, pReg+i, sizeof(struct ov5645_reg));
		if (err < 0)
			v4l_info(client, "%s: register set failed\n", __func__);
	}

	return 0;
}
#endif
static const char *ov5645_querymenu_wb_preset[] =
{ "WB Tungsten", "WB Fluorescent", "WB sunny", "WB cloudy", NULL };

static const char *ov5645_querymenu_effect_mode[] =
{ "Effect Sepia", "Effect Aqua", "Effect Monochrome", "Effect Negative", "Effect Sketch", NULL };

static const char *ov5645_querymenu_ev_bias_mode[] =
{ "-3EV", "-2,1/2EV", "-2EV", "-1,1/2EV", "-1EV", "-1/2EV", "0", "1/2EV", "1EV", "1,1/2EV", "2EV", "2,1/2EV", "3EV", NULL };

static struct v4l2_queryctrl ov5645_controls[] =
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
		.maximum = ARRAY_SIZE(ov5645_querymenu_wb_preset) - 2,//5个元素，5-2 =3 从0-3 ,4个挡
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
		.maximum = ARRAY_SIZE(ov5645_querymenu_ev_bias_mode) - 2,
		.step = 1,
		.default_value =
		(ARRAY_SIZE(ov5645_querymenu_ev_bias_mode) - 2) / 2,//默认取中间值
		/* 0 EV */
	},
	{
		.id = V4L2_CID_CAMERA_EFFECT,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Image Effect",
		.minimum = 0,
		.maximum = ARRAY_SIZE(ov5645_querymenu_effect_mode) - 2,
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

const char **ov5645_ctrl_get_menu(u32 id)
{
	dbg_int(id);

	switch (id)
	{
	case V4L2_CID_WHITE_BALANCE_PRESET:
		return ov5645_querymenu_wb_preset;

	case V4L2_CID_CAMERA_EFFECT:
		return ov5645_querymenu_effect_mode;

	case V4L2_CID_EXPOSURE:
		return ov5645_querymenu_ev_bias_mode;

	default:
		return v4l2_ctrl_get_menu(id);
	}
}

static inline struct v4l2_queryctrl const *ov5645_find_qctrl(int id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ov5645_controls); i++)
		if (ov5645_controls[i].id == id)
			return &ov5645_controls[i];

	return NULL;
}

static int ov5645_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	int i;
	dbg_fun;

	for (i = 0; i < ARRAY_SIZE(ov5645_controls); i++)
	{
		if (ov5645_controls[i].id == qc->id)
		{
			memcpy(qc, &ov5645_controls[i], sizeof(struct v4l2_queryctrl));
			return 0;
		}
	}

	return -EINVAL;
}

static int ov5645_querymenu(struct v4l2_subdev *sd, struct v4l2_querymenu *qm)
{
	struct v4l2_queryctrl qctrl;

	qctrl.id = qm->id;
	ov5645_queryctrl(sd, &qctrl);

	return v4l2_ctrl_query_menu(qm, &qctrl, ov5645_ctrl_get_menu(qm->id));
}

static int ov5645_g_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;
	dbg_fun;

	return err;
}

static int ov5645_s_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
	
	int ret = -EINVAL;
	int i, err=0;
	unsigned short *item;
	struct ov5645_state *state =to_state(sd);
	
	printk("lzy: %s,width = %d,height = %d\n",__func__,fmt->width,fmt->height);
	state->pix.width = fmt->width;
	state->pix.height = fmt->height;


		//printk("ov5645 500w set\n");
		state->framesize_index=ov5645_CAPTURE_5MP;		
//		i2c_write_array_16(v4l2_get_subdevdata(sd), ov5645_reset_regs);
//		i2c_write_array_16(v4l2_get_subdevdata(sd), ov5645_2LANE_500W_regs);
		//i2c_write_array_16(v4l2_get_subdevdata(sd), ov5645_2LANE_500W_regs_new);
		
 		//err = ov5645_real_write_reg(sd, (unsigned short **) ov5645_init_reg_1600_1200,ov5645_init_reg_1600_1200S);	

	if(1920==fmt->width&&(1080==fmt->height))
	{	
		printk("ov5645 200w set\n");
		state->framesize_index=ov5645_CAPTURE_1080P;		
//		i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_res_1920x1080);
	}

	if(1600==fmt->width&&(1200==fmt->height))
	{	
		printk("ov5645 200w set\n");
		state->framesize_index=ov5645_CAPTURE_2MP;		
//		i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_res_1600x1200);
	}
	else if(1280==fmt->width&&(720==fmt->height))		
	{	
		printk("ov5645 200w set\n");
		state->framesize_index=ov5645_CAPTURE_1MP;		
//		i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_res_1280x720);
	}
	else if(640==fmt->width&&(480==fmt->height))		
	{	
		printk("ov5645 200w set\n");
		state->framesize_index=ov5645_CAPTURE_VGA;		
//		i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_res_640x480);
	}
	if(2592==fmt->width&&(1944==fmt->height))
	{
		 state->framesize_index=ov5645_CAPTURE_5MP;
			/*
		printk("ov5645 500w set\n");
		state->framesize_index=ov5645_CAPTURE_5MP;		
		i2c_write_array_16(v4l2_get_subdevdata(sd), ov5645_reset_regs);
		usleep_range(45000, 50000);  // must
		i2c_write_array_16(v4l2_get_subdevdata(sd), ov5645_2LANE_500W_regs);
		//i2c_write_array_16(v4l2_get_subdevdata(sd), ov5645_2LANE_500W_regs_new);
		
 		//err = ov5645_real_write_reg(sd, (unsigned short **) ov5645_init_reg_1600_1200,ov5645_init_reg_1600_1200S);	
 		*/
	}

	return 0;

}
	


static int ov5645_enum_framesizes(struct v4l2_subdev *sd, struct v4l2_frmsizeenum *fsize)
{
	struct ov5645_state *state = to_state(sd);
	int num_entries = sizeof(ov5645_capture_framesize_list) / sizeof(struct ov5645_enum_framesize);
	struct ov5645_enum_framesize *elem;
	int index = 0;
	int i = 0;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;

	index = state->framesize_index;
	
	printk("%s : state->framesize_index=%d\n",__func__,state->framesize_index);
	
	for (i = 0; i < num_entries; i++)
	{
		elem = &ov5645_capture_framesize_list[i];
		printk("%s:i=%d,index=%d,w=%d,h=%d\n",__func__,i,elem->index,elem->width,elem->height);
		if (elem->index == index)
		{
			fsize->discrete.width = ov5645_capture_framesize_list[i].width;
			fsize->discrete.height = ov5645_capture_framesize_list[i].height;
			printk("%s : fsize->discrete.width= %d, fsize->discrete.height=%d \n ",__func__,fsize->discrete.width,fsize->discrete.height);
			return 0;
		}
	}
	return -EINVAL;
}

static int ov5645_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_frmivalenum *fival)
{
	int err = 0;
	dbg_fun;
	return err;
}

static int ov5645_enum_fmt(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmtdesc)
{
	int err = 0;
	
	dbg_fun;

	return err;
}

static int ov5645_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;
	dbg_fun;

	return err;
}

static int ov5645_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0;

	dbg_fun;

	return err;
}

static int ov5645_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0;
	dbg_fun;
	dev_dbg(&client->dev, "%s: numerator %d, denominator: %d\n", __func__,
			param->parm.capture.timeperframe.numerator,
			param->parm.capture.timeperframe.denominator);

	return err;
}

static int ov5645_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov5645_state *state = to_state(sd);
	struct ov5645_userset userset = state->userset;
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

static int ov5645_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov5645_state *state = to_state(sd);
	int err = 0;
	int value = ctrl->value;
	
	dbg_int(ctrl->id);

	switch (ctrl->id)
	{

	case V4L2_CID_CAMERA_FLASH_MODE:
	case V4L2_CID_CAMERA_BRIGHTNESS:
		break;
	case V4L2_CID_CAMERA_WHITE_BALANCE:
		dbg_str("*** V4L2_CID_CAMERA_WHITE_BALANCE");
		if (value <= WHITE_BALANCE_AUTO)
	/*	{
			err = ov5645_write_regs(sd, (unsigned char *) ov5645_regs_awb_enable[value],
							sizeof(ov5645_regs_awb_enable[value]));
		}
		else
		{
			err = ov5645_write_regs(sd, (unsigned char *) ov5645_regs_wb_preset[value - 2],
							sizeof(ov5645_regs_wb_preset[value - 2]));
		} */
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
		err = ov5645_reset(sd);
		break;
	case V4L2_CID_EXPOSURE:
		dbg_str("*** V4L2_CID_EXPOSURE");
//		err = ov5645_write_regs(sd, (unsigned char *) ov5645_regs_ev_bias[value],
//						sizeof(ov5645_regs_ev_bias[value]));
		break;
	default:
		err=0;
	dbg_str("V4L2_CID_EXPOSURE");
		/* err = -EINVAL; */
		break;
	}

	if (err < 0)
		dev_dbg(&client->dev, "%s: vidioc_s_ctrl failed\n", __func__);

	return err;
}

static int ov5645_init(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov5645_state *state = to_state(sd);
	
	int err = -EINVAL, i;	
	printk("enter %s\n",__func__);
//	unsigned char test_ov5645[2];
	 state->framesize_index=0;
	dbg_fun;
	ov5645_power( 1);

/*	test_ov5645[0]=ov5645_read(sd,0xFC);
	test_ov5645[1]=ov5645_read(sd,0xFD);


	if((test_ov5645[0]==0x39)&&(test_ov5645[1]==0x20)){
		
		printk("ov5645	exit\n");
	}else
		printk("ov5645 not exit\n");


	err = __ov5645_init_2bytes(sd, (unsigned short **) ov5645_init_reg,ov5645_INIT_REGS);

	if (err < 0)
	{
		state->check_previewdata = 100;
		v4l_err(client, "%s: camera initialization failed. err(%d)\n",
				__func__, state->check_previewdata);
		return -1;
	}
*/
	i2c_write_array_16(v4l2_get_subdevdata(sd), ov5645_reset_regs);
	usleep_range(4500, 5000);  // must
	i2c_write_array_16(v4l2_get_subdevdata(sd), ov5645_reset_regs);
	usleep_range(4500, 5000);  // must
	/* This is preview success */

	uint8_t value = -1;
	 reg_read_16(client, 0x300a, &value);
	printk("enter %s,reg0x300a=0x%02x\n",__func__,value);
	 reg_read_16(client, 0x300b, &value);
	printk("enter %s,reg0x300b=0x%02x\n",__func__,value);
	
	state->check_previewdata = 0;
	printk("%s: camera initialization end\n", __func__);
	return 0;
}

static int ov5645_s_config(struct v4l2_subdev *sd, int irq, void *platform_data)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov5645_state *state = to_state(sd);
	struct ov5645_platform_data *pdata;

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

static int ov5645_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct ov5645_state *state =to_state(sd);
	int w,h;
	
	w=state->pix.width ;
	h=state->pix.height ;

	printk("lzy: %s,width = %d,height = %d\n",__func__,w,h);

	if(enable){
		ov5645_power11( 1);
	//	usleep_range(4500, 5000); 
	//	state->framesize_index=ov5645_CAPTURE_5MP;		
		i2c_write_array_16(v4l2_get_subdevdata(sd), ov5645_reset_regs);
		usleep_range(45000, 50000);
		i2c_write_array_16(v4l2_get_subdevdata(sd), ov5645_2LANE_500W_regs);
		state->framesize_index=ov5645_CAPTURE_5MP;	
		
		if(1920==w&&(1080==h))
		{	
			printk("ov5645 200w set\n");
			state->framesize_index=ov5645_CAPTURE_1080P;		
			i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_res_1920x1080);
		}

		if(1600==w&&(1200==h))
		{	
			printk("ov5645 200w set\n");
			state->framesize_index=ov5645_CAPTURE_2MP;		
			i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_res_1600x1200);
		}
		else if(1280==w&&(720==h))		
		{	
			printk("ov5645 200w set\n");
			state->framesize_index=ov5645_CAPTURE_1MP;		
			i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_res_1280x720);
		}
		else if(640==w&&(480==h))		
		{	
			printk("ov5645 200w set\n");
			state->framesize_index=ov5645_CAPTURE_VGA;		
			i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_res_640x480);
		}
		//i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_auto_focus_init);
	}else{
		ov5645_power11( 0);
	}
	
	
	return 0;
}

static const struct v4l2_subdev_core_ops ov5645_core_ops =
{
	.init = ov5645_init, /* initializing API */
	//.s_config = ov5645_s_config, /* Fetch platform data */
	.queryctrl = ov5645_queryctrl,
	.querymenu = ov5645_querymenu,
      .s_power = ov5645_power,
	.g_ctrl = ov5645_g_ctrl,
	.s_ctrl = ov5645_s_ctrl,
};

static const struct v4l2_subdev_video_ops ov5645_video_ops =
{
	.g_mbus_fmt = ov5645_g_fmt,
	.s_mbus_fmt = ov5645_s_fmt,
	.enum_framesizes = ov5645_enum_framesizes,
	.enum_frameintervals = ov5645_enum_frameintervals,
	//.enum_fmt = ov5645_enum_fmt,//
	//.try_fmt = ov5645_try_fmt,
	.g_parm = ov5645_g_parm,
	.s_parm = ov5645_s_parm,
	.s_stream = ov5645_s_stream,
};

static const struct v4l2_subdev_ops ov5645_ops =
{
	.core = &ov5645_core_ops,
	.video = &ov5645_video_ops,
};

static int ov5645_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct ov5645_state *state;
	struct v4l2_subdev *sd;
	int ret =0;
	dbg_fun;
	//ov5645_init();
	state = kzalloc(sizeof(struct ov5645_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

	sd = &state->sd;
	strcpy(sd->name, OV5645_DRIVER_NAME);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &ov5645_ops);
	dev_info(&client->dev, "*** ov5645 has been probed\n");

	
	return 0;

}

static int ov5645_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	printk("ov5645_remove\n");
//	ov5645_power11( 0);
	
	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id ov5645_id[] = {
	{ OV5645_DRIVER_NAME, 0 },
	{ }, 
};
MODULE_DEVICE_TABLE(i2c, ov5645_id);

static struct i2c_driver ov5645_i2c_driver =
{
	.driver = {
		.name = OV5645_DRIVER_NAME,
	},
	.probe = ov5645_probe,
	.remove = __devexit_p(ov5645_remove),
	.id_table = ov5645_id,
};

static int __init ov5645_mod_init(void)
{
	printk("%s\n",__func__);
	return i2c_add_driver(&ov5645_i2c_driver);
}

static void __exit ov5645_mod_exit(void)
{
	printk("%s\n",__func__);
	i2c_del_driver(&ov5645_i2c_driver);
}

module_init(ov5645_mod_init);
module_exit(ov5645_mod_exit);

MODULE_DESCRIPTION("Samsung Electronics ov5645 UXGA camera driver");
MODULE_AUTHOR("Jinsung Yang <jsgood.yang@samsung.com>");
MODULE_LICENSE("GPL");



