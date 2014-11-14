

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
#include <linux/proc_fs.h>
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
#include "../../../oem_drv/power_gpio.h"


static int output_format_index = 0;
#define OV5645_DRIVER_NAME	"OV5645"

/* Default resolution & pixelformat. plz ref ov5645_platform.h */
#define DEFAULT_FMT		V4L2_PIX_FMT_UYVY	/* YUV422 */


#define dbg_fun     printk("*** func:%s\n",__FUNCTION__)
#define dbg_int(i)  printk("*** func:%s,i = %d\n",__FUNCTION__,i)
#define dbg_str(i)  printk("*** func:%s,i = %s\n",__FUNCTION__,i)

static struct i2c_client * s_client = NULL;

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
	unsigned int flash_mode;
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
	int focus_mode;
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
	ov5645_CAPTURE_960,    /*1280x960*/
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
	   {ov5645_CAPTURE_960,    1280,  960},
	   {ov5645_CAPTURE_2MP, 1600, 1200 },	
	   {ov5645_CAPTURE_1080P, 1920, 1080}, 
	   {ov5645_CAPTURE_5MP, 2592, 1944 },	 
 };

//#define GPIO_CAMERA_POWER  EXYNOS4212_GPM3(0)
#define GPIO_CAMERA_POWER  GPIO_EXAXP22(1)
//#define GPIO_CAMERA_PD1    EXYNOS4_GPL0(1)
#define GPIO_CAMERA_PD0    EXYNOS4_GPL0(0)
#define GPIO_CAMERA_FLASH_EN  EXYNOS4_GPL2(1)
#define GPIO_CAMERA_RESET  EXYNOS4212_GPJ1(4)

#define GPIO_CAMERA_FLASH_MODE EXYNOS4212_GPM2(1)


static int ov5645_power( int enable)
{	
	int ret=0;
/*	struct regulator *vdd28_cam_regulator = NULL;
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
*/

	if(enable){
		write_power_item_value(POWER_BCAM_18V,1);
		write_power_item_value(POWER_BCAM_28V,1);
		write_power_item_value(POWER_CAM_AF,1);

		write_power_item_value(POWER_BCAM_PD,0);
		write_power_item_value(POWER_BCAM_RST,1);
		msleep(5);
		write_power_item_value(POWER_BCAM_PD,1);
		msleep(2);
		write_power_item_value(POWER_BCAM_RST,0);
		msleep(5);
		write_power_item_value(POWER_BCAM_RST,1);
		msleep(2);
	}else{
		write_power_item_value(POWER_BCAM_18V,0);
		write_power_item_value(POWER_BCAM_28V,0);
		write_power_item_value(POWER_CAM_AF,0);
		write_power_item_value(POWER_BCAM_PD,0);
	}
	return ret;
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
			msleep(10);
			uint8_t value = -1;;

			ret = reg_read_16(client, vals->u16RegAddr, &value);
			if(vals->u8Val != (value & 0xff))
				printk("[addr]%x, [R] reg[0x:%x] [W]:value:[0x%x]  [R]value[:0x%x]\n",client->addr, vals->u16RegAddr,vals->u8Val,value);
		#endif
		if(vals->u32Delay_ms > 0)
			msleep(vals->u32Delay_ms);
		vals++;
	}
	dev_dbg(&client->dev, "Register list loaded\n");

	return 0;
}


static int i2c_write_array_16_sleep(struct i2c_client *client,
				struct hm2056_reg  *vals)
{
	uint i = 0;

	while(vals->u16RegAddr != HM5056_TABLE_END) {

		int ret = reg_write_16(client, vals->u16RegAddr, vals->u8Val);

		if (ret < 0)
			return ret;

		#if 1 // for test write sensor reg
			//mdelay(50);
		//	msleep(10);
			uint8_t value = -1;;

			ret = reg_read_16(client, vals->u16RegAddr, &value);
			if(vals->u8Val != (value & 0xff))
				printk("[addr]%x, [R] reg[0x:%x] [W]:value:[0x%x]  [R]value[:0x%x]\n",client->addr, vals->u16RegAddr,vals->u8Val,value);
		#endif
		if(vals->u32Delay_ms > 0)
			msleep(vals->u32Delay_ms);
		vals++;
	}
	dev_dbg(&client->dev, "Register list loaded\n");

	return 0;
}

static int i2c_check_array_af(struct i2c_client *client,
				unsigned char * pv)
{
	uint i = 0,ret;
	u8 val;
	u16 start=0x8000,end=0x9572,reg;
	pv+=2;
	for(reg=start,i=0;reg<end+1;reg++,i++){
		reg_read_16(client, reg, &val);
		if(val!=pv[i]){
			printk("%s:reg[0x%04x],R[0x%02x],W[0x%02x]\n",__func__,reg,val,pv[i]);
		}
	}
}

static int i2c_check_array_16(struct i2c_client *client,
				struct hm2056_reg  *vals)
{
	uint i = 0,ret;

	while(vals->u16RegAddr != HM5056_TABLE_END) {

		//int ret = reg_write_16(client, vals->u16RegAddr, vals->u8Val);

		//if (ret < 0)
		//	return ret;

		#if 1 // for test write sensor reg
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
	u8 r;
	unsigned short *item;
	struct ov5645_state *state =to_state(sd);
	
	printk("*** lzy: %s,width = %d,height = %d\n",__func__,fmt->width,fmt->height);
	state->pix.width = fmt->width;
	state->pix.height = fmt->height;


	state->framesize_index=ov5645_CAPTURE_5MP;
	//i2c_write_array_16(v4l2_get_subdevdata(sd), ov5645_reset_regs);
//	i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_reg_stop_stream);

	if(2592==fmt->width&&(1944==fmt->height))
	{
		printk("ov5645 5M set\n");
		state->framesize_index=ov5645_CAPTURE_5MP;		
		i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_res_2592x1944); 		
	}
	else if(1920==fmt->width&&(1080==fmt->height))
	{	
		printk("ov5645 1080P set\n");
		state->framesize_index=ov5645_CAPTURE_1080P;		
		i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_res_1920x1080);
	}
	else if(1600==fmt->width&&(1200==fmt->height))
	{	
		printk("ov5645 2M set\n");
		state->framesize_index=ov5645_CAPTURE_2MP;		
		i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_res_1600x1200);
	}
	else if(1280==fmt->width&&(960==fmt->height))
	{	
		printk("ov5645 1.3M set\n");
		state->framesize_index=ov5645_CAPTURE_960;		
		i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_res_1280x960);
	}
	else if(1280==fmt->width&&(720==fmt->height))		
	{	
		printk("ov5645 1M set\n");
		state->framesize_index=ov5645_CAPTURE_1MP;		
		i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_res_1280x720);
	}
	else if(640==fmt->width&&(480==fmt->height))		
	{	
		printk("ov5645 VGA set\n");
		state->framesize_index=ov5645_CAPTURE_VGA;		
		i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_res_640x480);
	} 

//	reg_read_16(v4l2_get_subdevdata(sd), 0x3000, &r);
//	printk("%s;reg[0x3000] = 0x%02x\n",__func__,r);
//	i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_auto_focus_init);
//	i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_focus_constant);
//	reg_read_16(v4l2_get_subdevdata(sd), 0x3000, &r);
//	printk("%s;11reg[0x3000] = 0x%02x\n",__func__,r);

//	i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_auto_focus_init_0723);

	i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_reg_stop_stream);

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

static void ov5645_enable_torch(struct v4l2_subdev *sd)
{
	gpio_direction_output(GPIO_CAMERA_FLASH_EN,1);
	gpio_direction_output(GPIO_CAMERA_FLASH_MODE,0);
}

static void ov5645_enable_flash(struct v4l2_subdev *sd)
{
	gpio_direction_output(GPIO_CAMERA_FLASH_EN,1);
	gpio_direction_output(GPIO_CAMERA_FLASH_MODE,1);
}

static void ov5645_disable_flash(struct v4l2_subdev *sd)
{
	gpio_direction_output(GPIO_CAMERA_FLASH_EN,0);
	gpio_direction_output(GPIO_CAMERA_FLASH_MODE,0);
}
static int ov5645_set_flash_mode(struct v4l2_subdev *sd, struct v4l2_control * ctrl)
{
	struct ov5645_state *state = to_state(sd);
	struct ov5645_userset * userset = &state->userset;
	int value=ctrl->value;
	if ((value >= FLASH_MODE_OFF) && (value <= FLASH_MODE_TORCH)) {
		printk("%s: setting flash mode to %d\n",__func__, value);
		if (value == FLASH_MODE_TORCH)
			ov5645_enable_torch(sd);
		else if(value == FLASH_MODE_ON)
			ov5645_enable_flash(sd);
		else if(value == FLASH_MODE_OFF)
			ov5645_disable_flash(sd);
		else if(value == FLASH_MODE_AUTO){
			printk("set flash auto\n");
		}
		userset->flash_mode=value;
		return 0;
	}
	printk("%s: trying to set invalid flash mode %d\n",__func__, value);
	return -EINVAL;
}

static int ov5645_set_exposure(struct v4l2_subdev *sd, struct v4l2_control * ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov5645_state *state = to_state(sd);
	struct ov5645_userset * userset = &state->userset;
	int value=ctrl->value;
	int ret = -EINVAL;
	switch(value){
		case -3:
			ret=i2c_write_array_16(client, OV5645_reg_exposure_n3);
			break;
		case -2:
			ret=i2c_write_array_16(client, OV5645_reg_exposure_n2);
			break;
		case -1:
			ret=i2c_write_array_16(client, OV5645_reg_exposure_n1);
			break;
		case 0:
			ret=i2c_write_array_16(client, OV5645_reg_exposure_0);
			break;
		case 1:
			ret=i2c_write_array_16(client, OV5645_reg_exposure_1);
			break;
		case 2:
			ret=i2c_write_array_16(client, OV5645_reg_exposure_2);
			break;
		case 3:
			ret=i2c_write_array_16(client, OV5645_reg_exposure_3);
			break;
		default:
			//printk("%s:default \n",__func__);
			ret = -EINVAL	;		
	}
	if(ret==0)
		userset->exposure_bias=value;
	return ret;
}
/*
static void ov5645_set_awb_mode(int AWB_enable)
{
    u8 AwbTemp;
	  AwbTemp = OV5645MIPIYUV_read_cmos_sensor(0x3406);   
    if (AWB_enable )
    {
             
		OV5645MIPI_write_cmos_sensor(0x3406,AwbTemp&0xFE); 
		
    }
    else
    {             
		OV5645MIPI_write_cmos_sensor(0x3406,AwbTemp|0x01); 
		
    }
	OV5645MIPISENSORDB("[OV5645MIPI]exit OV5645MIPI_set_AWB_mode function:\n ");
}*/

static int ov5645_set_wb(struct v4l2_subdev *sd, struct v4l2_control * ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov5645_state *state = to_state(sd);
	struct ov5645_userset * userset = &state->userset;
	int value=ctrl->value;
	u8 awb;	
	int ret = -EINVAL;

	reg_read_16(client, 0x3406, &awb);
	switch(value){
		case WHITE_BALANCE_AUTO:
			reg_write_16(client, 0x3406, awb&0xfe);
			//ret=i2c_write_array_16(client, OV5645_reg_wb_auto);
			break;
		case WHITE_BALANCE_SUNNY:
			reg_write_16(client, 0x3406, awb|0x01);
			ret=i2c_write_array_16(client, OV5645_reg_wb_sunny);
			break;
		case WHITE_BALANCE_CLOUDY:
			reg_write_16(client, 0x3406, awb|0x01);
			ret=i2c_write_array_16(client, OV5645_reg_wb_cloudy);
			break;
		case WHITE_BALANCE_TUNGSTEN:
			reg_write_16(client, 0x3406, awb|0x01);
			ret=i2c_write_array_16(client, OV5645_reg_wb_tungsten);
			break;
		case WHITE_BALANCE_FLUORESCENT:
			reg_write_16(client, 0x3406, awb|0x01);
			ret=i2c_write_array_16(client, OV5645_reg_wb_fluorscent);
			break;
		default:
			//printk("%s:default \n",__func__);
			ret = -EINVAL	;		
	}
	if(ret==0)
		userset->auto_wb=value;
	return ret;
}

static void ov5645_get_focus_result(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	uint8_t state_3028=0;
	uint8_t state_3029=0;
	uint8_t value;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	reg_read_16(client, 0x3028, &state_3028);
	reg_read_16(client, 0x3029, &state_3029);
	if (state_3028==0)	{
		ctrl->value = SENSOR_AF_ERROR;    
	}
	else if (state_3028==1){
		switch (state_3029){
			case 0x70:
				ctrl->value = SENSOR_AF_IDLE;
				break;
			case 0x00:
				ctrl->value = SENSOR_AF_FOCUSING;
				break;
			case 0x10:
				ctrl->value = SENSOR_AF_FOCUSED;
				break;
			case 0x20:
				ctrl->value = SENSOR_AF_FOCUSED;
				break;
			default:
				ctrl->value = SENSOR_AF_SCENE_DETECTING; 
				break;
		}                                  
	}	
}


static int ov5645_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov5645_state *state = to_state(sd);
	struct ov5645_userset userset = state->userset;
	int err = 0;
	
//	dbg_int(ctrl->id);

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
		ctrl->value = 0;
		break;
	case V4L2_CID_CAMERA_AUTO_FOCUS_RESULT:
		ov5645_get_focus_result(sd,ctrl);
		break;
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

static int ov5645_set_focus_mode(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	int err=0;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	switch(ctrl->value){
		case FOCUS_MODE_AUTO:
		case FOCUS_MODE_TOUCH:
		case FOCUS_MODE_TOUCH_FLASH_AUTO:
		case FOCUS_MODE_TOUCH_FLASH_ON:
		case FOCUS_MODE_TOUCH_FLASH_OFF:
		//	printk("start to single focus\n");
		//	err=i2c_write_array_16(client, OV5645_focus_single);
			break;

		case FOCUS_MODE_CONTINOUS:
		case FOCUS_MODE_CONTINOUS_XY:
			printk("start to constant focus\n");
			err=i2c_write_array_16(client, OV5645_focus_constant);
			break;
		default:
			//printk("%s:value=%d error\n",__func__,ctrl->value);
			err=-EINVAL;
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
		//printk("%s:set flash mode\n",__func__);
		ov5645_set_flash_mode(sd,ctrl);
		break;
	case V4L2_CID_CAMERA_BRIGHTNESS:
		//printk("%s:set brightness value=%d\n",__func__,ctrl->value);
		break;
	case V4L2_CID_CAMERA_WHITE_BALANCE:
		//printk("%s:set white balance value=%d\n",__func__,ctrl->value);
		ov5645_set_wb(sd,ctrl);
	
		break;
	case V4L2_CID_CAMERA_EFFECT:
	case V4L2_CID_CAMERA_ISO:
	case V4L2_CID_CAMERA_METERING:
	case V4L2_CID_CAMERA_CONTRAST: //do not support now
	case V4L2_CID_CAMERA_SATURATION: //do not support now  
	case V4L2_CID_CAMERA_SHARPNESS: //do not support now
	case V4L2_CID_CAMERA_WDR:
	case V4L2_CID_CAMERA_FACE_DETECTION:
		break;
	case V4L2_CID_CAMERA_FOCUS_MODE:
		//printk("%s:set focus mode:value=%d\n",__func__,ctrl->value);
		state->focus_mode=ctrl->value;
		ov5645_set_focus_mode(sd,ctrl);
		break;
	case V4L2_CID_CAM_JPEG_QUALITY:
	case V4L2_CID_CAMERA_SCENE_MODE:
	case V4L2_CID_CAMERA_GPS_LATITUDE:
	case V4L2_CID_CAMERA_GPS_LONGITUDE:
	case V4L2_CID_CAMERA_GPS_TIMESTAMP:
	case V4L2_CID_CAMERA_GPS_ALTITUDE:
		break;
	case V4L2_CID_CAMERA_OBJECT_POSITION_X:
		if(value<16)
			value=16;
		else if(value>state->pix.width-16)
			value=value>state->pix.width-16;
		value=value/16;
		//printk("%s:set posx=%d\n",__func__,value);
		reg_write_16(client,0x3024,value);
		break;
	case V4L2_CID_CAMERA_OBJECT_POSITION_Y:
		if(value<16)
			value=16;
		else if(value>state->pix.height-16)
			value=value>state->pix.height-16;
		value=value/16;
		//printk("%s:set posy=%d\n",__func__,value);
		reg_write_16(client,0x3025,value);
		break;
	case V4L2_CID_CAMERA_SET_AUTO_FOCUS:
		//printk("%s:set auto focus,value=%d\n",__func__,ctrl->value);
		if(ctrl->value==AUTO_FOCUS_OFF){
			state->focus_mode=-1;
			err=i2c_write_array_16(client, OV5645_focus_cancel);
		}else{
			printk("start to single focus22\n");
			state->focus_mode=FOCUS_MODE_AUTO;
			err=i2c_write_array_16(client, OV5645_focus_single);
		}
		break;
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
	case V4L2_CID_CAMERA_EXPOSURE:
		//printk("%s:set exposure value=%d\n",__func__,ctrl->value);
		ov5645_set_exposure(sd,ctrl);
		break;
	default:
		err=0;
	//printk("default");
		/* err = -EINVAL; */
		break;
	}

	if (err < 0)
		dev_dbg(&client->dev, "%s: vidioc_s_ctrl failed\n", __func__);

	return err;
}
#define ARRY_SIZE(A) (sizeof(A)/sizeof(A[0]))
static int ov5645_init(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov5645_state *state = to_state(sd);
	u8 r;
	int err = -EINVAL, i;	
	uint8_t value = -1,v1,v2;
	printk("enter %s\n",__func__);
	 state->framesize_index=0;
	 state->focus_mode=-1;

	
	ov5645_power( 1);

	reg_read_16(client, 0x300a, &v1);
	//printk("enter %s,reg0x300a=0x%02x\n",__func__,v1);
	reg_read_16(client, 0x300b, &v2);
	//printk("enter %s,reg0x300b=0x%02x\n",__func__,v2);	
	state->check_previewdata = 0;
	//printk("%s: camera initialization end\n", __func__);
	if(v1==0x56&&v2==0x45)
		printk("ov5645 camera\n");
	else{
		printk("%s:error,not find 0v5645\n",__func__);
		return -ENODEV;
	}

	i2c_write_array_16(v4l2_get_subdevdata(sd), ov5645_reset_regs);
	usleep_range(5000, 5500);  // must

	i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_auto_focus_init_s1);

	msleep(10);
	err=i2c_master_send(v4l2_get_subdevdata(sd), OV5645_auto_focus_init_s2, ARRY_SIZE(OV5645_auto_focus_init_s2));
	printk("%s:init err=%d\n",__func__,err);
	reg_read_16(client, 0x8000, &value);
	if(value!=0x02){
		printk("###########%s:init error ############\n",__func__);
		printk("###########%s:init error ############\n",__func__);
		printk("###########%s:init error ############\n",__func__);
	}
	
	i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_reg_init_2lane);

	i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_auto_focus_init_s3);

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
	unsigned int r;

	if(enable){		
		i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_reg_start_stream);
		usleep_range(5000, 5500); 
		if((state->focus_mode==FOCUS_MODE_CONTINOUS) ||
		   			(state->focus_mode==FOCUS_MODE_CONTINOUS_XY))
			i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_focus_constant); 
		
	}else{
		i2c_write_array_16(v4l2_get_subdevdata(sd), OV5645_reg_stop_stream);
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

#define PROC_NAME       "ov5645"
#define I2C_PROC_NAME   "i2c"

static int i2c_proc_write(struct file *file, const char *buffer,
                           unsigned long count, void *data)
{
	unsigned int value, reg; 
	value = 0;
	reg = 0;

	if(!s_client) {
		printk("[OV5645] Please open camera first\n");
		return count;
	}

	if(sscanf(buffer, "%x=%x", &reg, &value) == 2)
	{
		reg_write_16(s_client, reg, value);
	}
	else if(sscanf(buffer, "%x", &reg) == 1)
	{
		reg_read_16(s_client, reg, &value);
	}
	printk("reg=0x%08x, value=0x%08x\n", reg, value);

	return count;
}

static int i2c_proc_read(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
	int len;
	len = sprintf( page, "usage(hex):\n  write: echo \"3008=0001\" > /proc/%s\n  read:  echo \"3008\" > /proc/%s\n", 
				   I2C_PROC_NAME, I2C_PROC_NAME);
    return len;
}

static int s_init = 0;
static void ov5645_init_proc_node(void)
{
	struct proc_dir_entry *root_entry;
	struct proc_dir_entry *entry;
	if(s_init) {
		return;
	}
	s_init = 1;

	printk("%s\n", __FUNCTION__);
	root_entry = proc_mkdir(PROC_NAME, NULL);
	entry = create_proc_entry(I2C_PROC_NAME, 0666, root_entry);

	if(entry) {
		entry->write_proc = i2c_proc_write;
		entry->read_proc = i2c_proc_read;
	}
}

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

	s_client = client;
	ov5645_init_proc_node();

	return 0;

}

static int ov5645_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	printk("ov5645_remove\n");
	
	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	ov5645_power(0);
	s_client = NULL;
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



