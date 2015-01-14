

/* linux/drivers/media/video/hm5065.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *	         http://www.samsung.com/
 *
 * Driver for hm5065 (UXGA camera) from Samsung Electronics
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

#include "ut_cam_common.h"

#include "hm5065.h"

#ifdef CONFIG_VIDEO_SAMSUNG_V4L2
#include <linux/videodev2_samsung.h>
#endif

#include "urbetter/power_gpio.h"
//static int output_format_index = 0;
#define BACK_SENSOR_DRIVER_NAME	"HM5065" 

/* Default resolution & pixelformat. plz ref back_sensor_platform.h */
#define DEFAULT_FMT		V4L2_PIX_FMT_UYVY	/* YUV422 */


#define dbg_fun     printk("*** func:%s\n",__FUNCTION__)
#define dbg_int(i)  printk("*** func:%s,i = %d\n",__FUNCTION__,i)
#define dbg_str(i)  printk("*** func:%s,i = %s\n",__FUNCTION__,i)

static struct i2c_client * s_client = NULL;

typedef enum {
	BACK_HM2056 = 0,
	BACK_GC0329 = 1,
	BACK_HM5065 = 2,
} back_cam_type;

static back_cam_type s_back_sensor_type = BACK_HM5065;

#define GC0329_IIC_ADDR	(0x62>>1)
#define HM2056_IIC_ADDR	(0x48>>1)
#define HM5065_IIC_ADDR	(0x1F)

static int back_sensor_init(struct v4l2_subdev *sd, u32 val);

struct back_sensor_userset
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

struct back_sensor_state
{
	struct back_sensor_platform_data *pdata;
	struct v4l2_subdev sd;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	struct back_sensor_userset userset;
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
	back_sensor_PREVIEW_XGA,
};

enum back_sensor_capture_frame_size {

//	back_sensor_CAPTURE_QVGA=0, /*320x240*/
	back_sensor_CAPTURE_VGA =0,	/* 640x480 */
	back_sensor_CAPTURE_SVGA,    /*800X600*/
	back_sensor_CAPTURE_1MP,	/* 1280x720 */
	back_sensor_CAPTURE_960,    /*1280x960*/
	back_sensor_CAPTURE_2MP,	/* UXGA  - 1600x1200 */
	back_sensor_CAPTURE_1080P,
	back_sensor_CAPTURE_5MP, /*2592x1944*/
};


struct back_sensor_enum_framesize
{
	unsigned int index;
	unsigned int width;
	unsigned int height;
};
struct back_sensor_enum_framesize back_sensor_capture_framesize_list[] = {

	{back_sensor_CAPTURE_VGA,   640,  480 },
	{back_sensor_CAPTURE_1MP,   1280,  720 },
	{back_sensor_CAPTURE_960,    1280,  960},
	{back_sensor_CAPTURE_2MP, 1600, 1200 },
	{back_sensor_CAPTURE_1080P, 1920, 1080},
	{back_sensor_CAPTURE_5MP, 2592, 1944 },
 };


static int back_sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret=0;

	if(on){
		write_power_item_value(POWER_BCAM_18V,1);
		write_power_item_value(POWER_BCAM_28V,1);
		write_power_item_value(POWER_BCAM_PD,0);
		write_power_item_value(POWER_BCAM_RST,1);
		msleep(50);
		write_power_item_value(POWER_BCAM_PD,1);
		msleep(25);
		write_power_item_value(POWER_BCAM_RST,0);
		msleep(50);
		write_power_item_value(POWER_BCAM_RST,1);
		msleep(30);
	}else{
		write_power_item_value(POWER_BCAM_28V,0);
		write_power_item_value(POWER_BCAM_18V,0);
		write_power_item_value(POWER_BCAM_PD,0);
	}

	printk("%s(%d); -\n", __func__, on);

	return ret;
}


static inline struct back_sensor_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct back_sensor_state, sd);
}

static int back_sensor_reset(struct v4l2_subdev *sd)
{
	dbg_fun;

	return back_sensor_init(sd, 0);
}

static const char *back_sensor_querymenu_wb_preset[] =
{ "WB Tungsten", "WB Fluorescent", "WB sunny", "WB cloudy", NULL };

static const char *back_sensor_querymenu_effect_mode[] =
{ "Effect Sepia", "Effect Aqua", "Effect Monochrome", "Effect Negative", "Effect Sketch", NULL };

static const char *back_sensor_querymenu_ev_bias_mode[] =
{ "-3EV", "-2,1/2EV", "-2EV", "-1,1/2EV", "-1EV", "-1/2EV", "0", "1/2EV", "1EV", "1,1/2EV", "2EV", "2,1/2EV", "3EV", NULL };

static struct v4l2_queryctrl back_sensor_controls[] =
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
		.maximum = ARRAY_SIZE(back_sensor_querymenu_wb_preset) - 2,//5个元素，5-2 =3 从0-3 ,4个挡
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
		.maximum = ARRAY_SIZE(back_sensor_querymenu_ev_bias_mode) - 2,
		.step = 1,
		.default_value =
		(ARRAY_SIZE(back_sensor_querymenu_ev_bias_mode) - 2) / 2,//默认取中间值
		/* 0 EV */
	},
	{
		.id = V4L2_CID_CAMERA_EFFECT,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Image Effect",
		.minimum = 0,
		.maximum = ARRAY_SIZE(back_sensor_querymenu_effect_mode) - 2,
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

const char **back_sensor_ctrl_get_menu(u32 id)
{
	dbg_int(id);

	switch (id)
	{
	case V4L2_CID_WHITE_BALANCE_PRESET:
		return back_sensor_querymenu_wb_preset;

	case V4L2_CID_CAMERA_EFFECT:
		return back_sensor_querymenu_effect_mode;

	case V4L2_CID_EXPOSURE:
		return back_sensor_querymenu_ev_bias_mode;

	default:
		return v4l2_ctrl_get_menu(id);
	}
}

static inline struct v4l2_queryctrl const *back_sensor_find_qctrl(int id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(back_sensor_controls); i++)
		if (back_sensor_controls[i].id == id)
			return &back_sensor_controls[i];

	return NULL;
}

static int back_sensor_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	int i;
	dbg_fun;

	for (i = 0; i < ARRAY_SIZE(back_sensor_controls); i++)
	{
		if (back_sensor_controls[i].id == qc->id)
		{
			memcpy(qc, &back_sensor_controls[i], sizeof(struct v4l2_queryctrl));
			return 0;
		}
	}

	return -EINVAL;
}

static int back_sensor_querymenu(struct v4l2_subdev *sd, struct v4l2_querymenu *qm)
{
	struct v4l2_queryctrl qctrl;

	qctrl.id = qm->id;
	back_sensor_queryctrl(sd, &qctrl);

	return v4l2_ctrl_query_menu(qm, &qctrl, back_sensor_ctrl_get_menu(qm->id));
}

static int back_sensor_g_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
	int err = 0;
	dbg_fun;

	return err;
}

static int back_sensor_s_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
//	int ret = -EINVAL;
//	int i, err=0;
//	u8 r;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct back_sensor_state *state =to_state(sd);

	printk("lzy: %s,width = %d,height = %d\n",__func__,fmt->width,fmt->height);
	state->pix.width = fmt->width;
	state->pix.height = fmt->height;


	state->framesize_index=back_sensor_CAPTURE_5MP;
	//sensor_i2c_write_array_16(v4l2_get_subdevdata(sd), back_sensor_reset_regs);
//	sensor_i2c_write_array_16(v4l2_get_subdevdata(sd), HM5065_reg_stop_stream);

	if(2592==fmt->width&&(1944==fmt->height))
	{
		printk("hm5065 5M set\n");
		state->framesize_index=back_sensor_CAPTURE_5MP;
		sensor_i2c_write_array_16(client, HM5065_res_2592x1944);
	}
	else if(1920==fmt->width&&(1080==fmt->height))
	{
		printk("hm5065 1080P set TODO \n");
		state->framesize_index=back_sensor_CAPTURE_1080P;
		sensor_i2c_write_array_16(client, HM5065_res_1920x1080);
	}
	else if(1600==fmt->width&&(1200==fmt->height))
	{
		printk("hm5065 2M set\n");
		state->framesize_index=back_sensor_CAPTURE_2MP;
		sensor_i2c_write_array_16(client, HM5065_res_1600x1200);
	}
	else if(1280==fmt->width&&(960==fmt->height))
	{
		printk("hm5065 1.3M set\n");
		state->framesize_index=back_sensor_CAPTURE_960;
		sensor_i2c_write_array_16(client, HM5065_res_1280x960);
	}
	else if(1280==fmt->width&&(720==fmt->height))
	{
		printk("hm5065 1M set\n");
		state->framesize_index=back_sensor_CAPTURE_1MP;
		sensor_i2c_write_array_16(client, HM5065_res_1280x720);
	}
	else if(640==fmt->width&&(480==fmt->height))
	{
		printk("hm5065 VGA set\n");
		state->framesize_index=back_sensor_CAPTURE_VGA;
		sensor_i2c_write_array_16(client, HM5065_res_640x480);
	}


	sensor_i2c_write_array_16(client, HM5065_reg_stop_stream);

	return 0;

}



static int back_sensor_enum_framesizes(struct v4l2_subdev *sd, struct v4l2_frmsizeenum *fsize)
{
	struct back_sensor_state *state = to_state(sd);
	int num_entries = sizeof(back_sensor_capture_framesize_list) / sizeof(struct back_sensor_enum_framesize);
	struct back_sensor_enum_framesize *elem;
	int index = 0;
	int i = 0;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;

	index = state->framesize_index;

	printk("%s : state->framesize_index=%d\n",__func__,state->framesize_index);

	for (i = 0; i < num_entries; i++)
	{
		elem = &back_sensor_capture_framesize_list[i];
		printk("%s:i=%d,index=%d,w=%d,h=%d\n",__func__,i,elem->index,elem->width,elem->height);
		if (elem->index == index)
		{
			fsize->discrete.width = back_sensor_capture_framesize_list[i].width;
			fsize->discrete.height = back_sensor_capture_framesize_list[i].height;
			printk("%s : fsize->discrete.width= %d, fsize->discrete.height=%d \n ",__func__,fsize->discrete.width,fsize->discrete.height);
			return 0;
		}
	}
	return -EINVAL;
}

static int back_sensor_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_frmivalenum *fival)
{
	int err = 0;
	dbg_fun;
	return err;
}

//static int back_sensor_enum_fmt(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmtdesc)
//{
//	int err = 0;
//
//	dbg_fun;
//
//	return err;
//}
//
//static int back_sensor_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
//{
//	int err = 0;
//	dbg_fun;
//
//	return err;
//}

static int back_sensor_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0;

	dbg_fun;

	return err;
}

static int back_sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0;
	dbg_fun;
	dev_dbg(&client->dev, "%s: numerator %d, denominator: %d\n", __func__,
			param->parm.capture.timeperframe.numerator,
			param->parm.capture.timeperframe.denominator);

	return err;
}

static void back_sensor_enable_torch(struct v4l2_subdev *sd)
{
//	gpio_direction_output(GPIO_CAMERA_FLASH_EN,1);
//	gpio_direction_output(GPIO_CAMERA_FLASH_MODE,0);
}

static void back_sensor_enable_flash(struct v4l2_subdev *sd)
{
//	gpio_direction_output(GPIO_CAMERA_FLASH_EN,1);
//	gpio_direction_output(GPIO_CAMERA_FLASH_MODE,1);
}

static void back_sensor_disable_flash(struct v4l2_subdev *sd)
{
//	gpio_direction_output(GPIO_CAMERA_FLASH_EN,0);
//	gpio_direction_output(GPIO_CAMERA_FLASH_MODE,0);
}
static int back_sensor_set_flash_mode(struct v4l2_subdev *sd, struct v4l2_control * ctrl)
{
	struct back_sensor_state *state = to_state(sd);
	struct back_sensor_userset * userset = &state->userset;
	int value=ctrl->value;
	if ((value >= FLASH_MODE_OFF) && (value <= FLASH_MODE_TORCH)) {
		printk("%s: setting flash mode to %d\n",__func__, value);
		if (value == FLASH_MODE_TORCH)
			back_sensor_enable_torch(sd);
		else if(value == FLASH_MODE_ON)
			back_sensor_enable_flash(sd);
		else if(value == FLASH_MODE_OFF)
			back_sensor_disable_flash(sd);
		else if(value == FLASH_MODE_AUTO){
			printk("set flash auto\n");
		}
		userset->flash_mode=value;
		return 0;
	}
	printk("%s: trying to set invalid flash mode %d\n",__func__, value);
	return -EINVAL;
}

static int back_sensor_set_exposure(struct v4l2_subdev *sd, struct v4l2_control * ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct back_sensor_state *state = to_state(sd);
	struct back_sensor_userset * userset = &state->userset;
	int value=ctrl->value;
	int ret = -EINVAL;
	switch(value){
		case -3:
			ret=sensor_i2c_write_array_16(client, HM5065_reg_exposure_n3);
			break;
		case -2:
			ret=sensor_i2c_write_array_16(client, HM5065_reg_exposure_n2);
			break;
		case -1:
			ret=sensor_i2c_write_array_16(client, HM5065_reg_exposure_n1);
			break;
		case 0:
			ret=sensor_i2c_write_array_16(client, HM5065_reg_exposure_0);
			break;
		case 1:
			ret=sensor_i2c_write_array_16(client, HM5065_reg_exposure_1);
			break;
		case 2:
			ret=sensor_i2c_write_array_16(client, HM5065_reg_exposure_2);
			break;
		case 3:
			ret=sensor_i2c_write_array_16(client, HM5065_reg_exposure_3);
			break;
		default:
			printk("%s:default \n",__func__);
			ret = -EINVAL	;
	}
	if(ret==0)
		userset->exposure_bias=value;
	return ret;
}
/*
static void back_sensor_set_awb_mode(int AWB_enable)
{
    u8 AwbTemp;
	  AwbTemp = HM5065MIPIYUV_read_cmos_sensor(0x3406);
    if (AWB_enable )
    {

		HM5065MIPI_write_cmos_sensor(0x3406,AwbTemp&0xFE);

    }
    else
    {
		HM5065MIPI_write_cmos_sensor(0x3406,AwbTemp|0x01);

    }
	HM5065MIPISENSORDB("[HM5065MIPI]exit HM5065MIPI_set_AWB_mode function:\n ");
}*/

static int back_sensor_set_wb(struct v4l2_subdev *sd, struct v4l2_control * ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct back_sensor_state *state = to_state(sd);
	struct back_sensor_userset * userset = &state->userset;
	int value=ctrl->value;
//	u8 awb;
	int ret = -EINVAL;

//	reg_read_16(client, 0x3406, &awb);
	switch(value){
		case WHITE_BALANCE_AUTO:
//			reg_write_16(client, 0x3406, awb&0xfe);
			ret=sensor_i2c_write_array_16(client, HM5065_reg_wb_auto);
			break;
		case WHITE_BALANCE_SUNNY:
//			reg_write_16(client, 0x3406, awb|0x01);
			ret=sensor_i2c_write_array_16(client, HM5065_reg_wb_sunny);
			break;
		case WHITE_BALANCE_CLOUDY:
//			reg_write_16(client, 0x3406, awb|0x01);
			ret=sensor_i2c_write_array_16(client, HM5065_reg_wb_cloudy);
			break;
		case WHITE_BALANCE_TUNGSTEN:
//			reg_write_16(client, 0x3406, awb|0x01);
			ret=sensor_i2c_write_array_16(client, HM5065_reg_wb_tungsten);
			break;
		case WHITE_BALANCE_FLUORESCENT:
//			reg_write_16(client, 0x3406, awb|0x01);
			ret=sensor_i2c_write_array_16(client, HM5065_reg_wb_fluorscent);
			break;
		default:
			printk("%s:default \n",__func__);
			ret = -EINVAL	;
	}
	if(ret==0)
		userset->auto_wb=value;
	return ret;
}

static void back_sensor_get_focus_result(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	uint8_t state_af = 0;
	uint8_t u8_tmp_h = 0;
	uint8_t u8_tmp_l = 0;
	int i;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	mdelay(1);
	do {
		reg_read_16(client, 0x07ae, &state_af);
		if (state_af==1) {
			ctrl->value = SENSOR_AF_FOCUSED;  

			reg_read_16(client, 0x06F0, &u8_tmp_h);
			reg_read_16(client, 0x06F1, &u8_tmp_l);
			printk("[HM5065]brant read current af pos: %02x%02x.\n", u8_tmp_h, u8_tmp_l);
			break;
		}
		else if(state_af==0)
		{
			ctrl->value = SENSOR_AF_FOCUSING;
/*
			switch (state_af)
			{
				case 0x70:
					*pFeatureReturnPara32 = SENSOR_AF_IDLE;
					break;
				case 0x00:
					*pFeatureReturnPara32 = SENSOR_AF_FOCUSING;
					break;
				case 0x10:
					*pFeatureReturnPara32 = SENSOR_AF_FOCUSED;
					break;
				case 0x20:
					*pFeatureReturnPara32 = SENSOR_AF_FOCUSED;
					break;
				default:
					*pFeatureReturnPara32 = SENSOR_AF_SCENE_DETECTING; 
					break;
			}                                 
*/ 
		} else {
			ctrl->value = SENSOR_AF_ERROR;
		}
		i++;
		if(i>10)
			break;
	}while(1);
	
//	if (state_3028==0)	{
//		ctrl->value = SENSOR_AF_ERROR;
//	}
//	else if (state_3028==1){
//		switch (state_3029){
//			case 0x70:
//				ctrl->value = SENSOR_AF_IDLE;
//				break;
//			case 0x00:
//				ctrl->value = SENSOR_AF_FOCUSING;
//				break;
//			case 0x10:
//				ctrl->value = SENSOR_AF_FOCUSED;
//				break;
//			case 0x20:
//				ctrl->value = SENSOR_AF_FOCUSED;
//				break;
//			default:
//				ctrl->value = SENSOR_AF_SCENE_DETECTING;
//				break;
//		}
//	}
}

static int back_sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct back_sensor_state *state = to_state(sd);
	struct back_sensor_userset userset = state->userset;
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
		back_sensor_get_focus_result(sd,ctrl);
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


static int back_sensor_set_focus_mode(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
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
			err = sensor_i2c_write_array_16(client, HM5065_focus_constant);
			break;
		default:
			//printk("%s:value=%d error\n",__func__,ctrl->value);
			err=-EINVAL;
			break;
	}
	return err;

}

static int back_sensor_set_scene_mode(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	switch(ctrl->value){
		case SCENE_MODE_BASE:
		case SCENE_MODE_NONE:
		case SCENE_MODE_PORTRAIT:
		case SCENE_MODE_NIGHTSHOT:
		case SCENE_MODE_BACK_LIGHT:
		case SCENE_MODE_LANDSCAPE:
		case SCENE_MODE_SPORTS:
		case SCENE_MODE_PARTY_INDOOR:
		case SCENE_MODE_BEACH_SNOW:
		case SCENE_MODE_SUNSET:
		case SCENE_MODE_DUSK_DAWN:
		case SCENE_MODE_FALL_COLOR:
		case SCENE_MODE_FIREWORKS:
		case SCENE_MODE_TEXT:
		case SCENE_MODE_CANDLE_LIGHT:
			err=sensor_i2c_write_array_16(client, HM5065_focus_single);
			break;
			
		default:
			printk("%s:value=%d error\n",__func__,ctrl->value);
			err=-EINVAL;
			break;
	}
	return err;

}


#define FACE_LC 			0x0714
#define FACE_START_XH 	0x0715
#define FACE_START_XL 	0x0716
#define FACE_SIZE_XH  	0x0717
#define FACE_SIZE_XL	 0x0718
#define FACE_START_YH	 0x0719
#define FACE_START_YL	 0x071A
#define FACE_SIZE_YH	 0x071B
#define FACE_SIZE_YL 	0x071C

static int back_sensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct back_sensor_state *state = to_state(sd);
	int err = 0;
	int value = ctrl->value;

	dbg_int(ctrl->id);

	switch (ctrl->id)
	{

	case V4L2_CID_CAMERA_FLASH_MODE:
		printk("%s:set flash mode\n",__func__);
		back_sensor_set_flash_mode(sd,ctrl);
		break;
	case V4L2_CID_CAMERA_BRIGHTNESS:
		printk("%s:set brightness value=%d\n",__func__,ctrl->value);
		break;
	case V4L2_CID_CAMERA_WHITE_BALANCE:
		printk("%s:set white balance value=%d\n",__func__,ctrl->value);
		back_sensor_set_wb(sd,ctrl);

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
		printk("%s:set focus mode:value=%d\n",__func__,ctrl->value);
		state->focus_mode=ctrl->value;
		back_sensor_set_focus_mode(sd,ctrl);
		break;
	case V4L2_CID_CAMERA_SCENE_MODE:
		printk("%s:set scene mode:value=%d\n",__func__,ctrl->value);
		back_sensor_set_scene_mode(sd, ctrl);
		break;
	case V4L2_CID_CAM_JPEG_QUALITY:

	case V4L2_CID_CAMERA_GPS_LATITUDE:
	case V4L2_CID_CAMERA_GPS_LONGITUDE:
	case V4L2_CID_CAMERA_GPS_TIMESTAMP:
	case V4L2_CID_CAMERA_GPS_ALTITUDE:
		break;
	case V4L2_CID_CAMERA_OBJECT_POSITION_X:
		printk("set position x=%d\n",value);

		reg_write_16(client,0x0808,0x01);
		reg_write_16(client,0x0809,0x00); 
		reg_write_16(client,0x080a,0x00);
		reg_write_16(client,0x080b,0x00);  
		reg_write_16(client,0x080c,0x00);
		reg_write_16(client,0x080d,0x00);
		reg_write_16(client,0x080e,0x00); 

		reg_write_16(client,0x0751,0x00);	  
		reg_write_16(client,FACE_LC,0x01);

		
		if(value>state->pix.width-17)
			value = state->pix.width-17;
		else if(value<17)
			value = 17;
		value -=16;
		reg_write_16(client,FACE_START_XH, value>>8);
		reg_write_16(client,FACE_START_XL, value&0xff);		

		reg_write_16(client,FACE_SIZE_XH,(0x32)>>8);
		reg_write_16(client,FACE_SIZE_XL,(0x32)&0xff);
			
		break;
	case V4L2_CID_CAMERA_OBJECT_POSITION_Y:
		printk("set position y=%d\n",value);
		if(value>state->pix.height-17)
			value = state->pix.height-17;
		else if(value < 17)
			value = 17;
		value -=16;
		reg_write_16(client,FACE_START_YH, value>>8);
		reg_write_16(client,FACE_START_YL, value&0xff);
		reg_write_16(client,FACE_SIZE_YH,(0x32)>>8);
		reg_write_16(client,FACE_SIZE_YL,(0x32)&0xff);	
		
		break;
	case V4L2_CID_CAMERA_SET_AUTO_FOCUS:
		printk("%s:set auto focus,value=%d\n",__func__,ctrl->value);
		if(ctrl->value==AUTO_FOCUS_OFF){
			state->focus_mode=-1;
		//	err=sensor_i2c_write_array_16(client, HM5065_focus_cancel);
		}else{
			printk("start to single focus22\n");
			state->focus_mode=FOCUS_MODE_AUTO;
			err=sensor_i2c_write_array_16(client, HM5065_focus_single);
		}
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
		err = back_sensor_reset(sd);
		break;
	case V4L2_CID_CAMERA_EXPOSURE:
		printk("%s:set exposure value=%d\n",__func__,ctrl->value);
		back_sensor_set_exposure(sd,ctrl);
		break;
	default:
		err=0;
	printk("default");
		/* err = -EINVAL; */
		break;
	}

	if (err < 0)
		dev_dbg(&client->dev, "%s: vidioc_s_ctrl failed\n", __func__);

	return err;
}


static int back_sensor_init(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct back_sensor_state *state = to_state(sd);
//	u8 r;
	int err = -EINVAL, i;
	uint8_t v1,v2;
//	int count = 0;

	printk("%s(%d); +\n",__func__, val);

	state->framesize_index = back_sensor_CAPTURE_VGA;
	state->focus_mode = -1;

	back_sensor_power( sd, 1);

#ifdef HM5065_IIC_ADDR
	client->addr = HM5065_IIC_ADDR;

//	gpio_direction_output(GPIO_ITU_CAM_B_PD, 1);
//	gpio_direction_output(GPIO_ITU_CAM_A_PD, 1);
//	mdelay(300);

	for(i=0; i < 4; i++) {
		err = reg_write_16(client, 0x0022, 0x00);
		if (err >= 0)
			break;
	}

	if(err < 0) {
		printk("BACK HM5065 not exist\n");
		return -1;
	} else {
		printk("BACK HM5065 exist \n");
		s_back_sensor_type = BACK_HM5065;
		goto __back_sensors_found;
	}
#endif

__back_sensors_found:
	reg_read_16(client, 0x0000, &v1);
	reg_read_16(client, 0x0001, &v2);
	printk("enter %s, chipID =0x%02x%02x\n",__func__,v1, v2);

	state->check_previewdata = 0;
	printk("%s: camera initialization end\n", __func__);

	sensor_i2c_write_array_16(client, HM5065_reset_regs);
	usleep_range(5000, 5500);  // must
	sensor_i2c_write_array_16(client, HM5065_reg_init_data);
	usleep_range(100000, 150000);  // must

	sensor_i2c_write_array_16(client, HM5065_reg_init_data1);
	usleep_range(100000, 150000);  // must
	sensor_i2c_write_array_16(client, HM5065_reg_init_data2);
	usleep_range(100000, 150000);  // must

//	sensor_i2c_write_array_16(client, HM5065_auto_focus_init_s1);

	msleep(10);

	reg_write_16(client, 0x01a0, 0x01); // auto white blance
	mdelay(50);
	reg_write_16(client, 0x0010, 0x02);
	mdelay(50);
	reg_write_16(client, 0x0010, 0x01);
	mdelay(20);

//	err=i2c_master_send(client, HM5065_auto_focus_init_s2, ARRY_SIZE(HM5065_auto_focus_init_s2));
//	printk("%s:init err=%d\n",__func__,err);
//	reg_read_16(client, 0x8000, &value);
//	if(value!=0x02){
//		printk("###########%s:init error ############\n",__func__);
//	}

//	sensor_i2c_write_array_16(client, HM5065_auto_focus_init_s3);

	printk("%s(%d); -\n",__func__, val);

	return 0;
}
//
//static int back_sensor_s_config(struct v4l2_subdev *sd, int irq, void *platform_data)
//{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	struct back_sensor_state *state = to_state(sd);
//	struct back_sensor_platform_data *pdata;
//
//	dbg_fun;
//
//	pdata = client->dev.platform_data;
//
//	if (!pdata)
//	{
//		dev_err(&client->dev, "%s: no platform data\n", __func__);
//		return -ENODEV;
//	}
//
//	if (!(pdata->default_width && pdata->default_height))
//	{
//		/* TODO: assign driver default resolution */
//	}
//	else
//	{
//		state->pix.width = pdata->default_width;
//		state->pix.height = pdata->default_height;
//	}
//
//	if (!pdata->pixelformat)
//		state->pix.pixelformat = DEFAULT_FMT;
//	else
//		state->pix.pixelformat = pdata->pixelformat;
//
//	if (!pdata->freq)
//		state->freq = 24000000; /* 24MHz default */
//	else
//		state->freq = pdata->freq;
//
//	if (!pdata->is_mipi)
//	{
//		state->is_mipi = 0;
//		dev_info(&client->dev, "parallel mode\n");
//	}
//	else
//		state->is_mipi = pdata->is_mipi;
//
//	return 0;
//}

static int back_sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct back_sensor_state *state =to_state(sd);
//	unsigned int r;

	if(enable){
		sensor_i2c_write_array_16(client, HM5065_reg_start_stream);
		usleep_range(5000, 5500);
		if((state->focus_mode==FOCUS_MODE_CONTINOUS) ||
		   			(state->focus_mode==FOCUS_MODE_CONTINOUS_XY))
			sensor_i2c_write_array_16(client, HM5065_focus_constant);
	} else {
		sensor_i2c_write_array_16(client, HM5065_reg_stop_stream);
	}

	return 0;
}

static const struct v4l2_subdev_core_ops back_sensor_core_ops =
{
	.init = back_sensor_init, /* initializing API */
	//.s_config = back_sensor_s_config, /* Fetch platform data */
	.queryctrl = back_sensor_queryctrl,
	.querymenu = back_sensor_querymenu,
	.s_power = back_sensor_power,
	.g_ctrl = back_sensor_g_ctrl,
	.s_ctrl = back_sensor_s_ctrl,
};

static const struct v4l2_subdev_video_ops back_sensor_video_ops =
{
	.g_mbus_fmt = back_sensor_g_fmt,
	.s_mbus_fmt = back_sensor_s_fmt,
	.enum_framesizes = back_sensor_enum_framesizes,
	.enum_frameintervals = back_sensor_enum_frameintervals,
	//.enum_fmt = back_sensor_enum_fmt,//
	//.try_fmt = back_sensor_try_fmt,
	.g_parm = back_sensor_g_parm,
	.s_parm = back_sensor_s_parm,
	.s_stream = back_sensor_s_stream,
};

static const struct v4l2_subdev_ops back_sensor_ops =
{
	.core = &back_sensor_core_ops,
	.video = &back_sensor_video_ops,
};

#define PROC_NAME       "back_cam"
#define I2C_PROC_NAME   "i2c"

static int i2c_proc_write(struct file *file, const char *buffer,
                           unsigned long count, void *data)
{
	int value = 0;
	int reg = 0;

	if(!s_client) {
		printk("[HM5065] Please open camera first\n");
		return count;
	}

	if(sscanf(buffer, "%x=%x", &reg, &value) == 2)
	{
		reg_write_16(s_client, (u16)reg, (u16)value);
	}
	else if(sscanf(buffer, "%x", &reg) == 1)
	{
		reg_read_16(s_client, (u16)reg, (u8 *)&value);
	}
	printk("reg=0x%04x, value=0x%04x\n", reg, value);

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
static void back_sensor_init_proc_node(void)
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

static int back_sensor_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct back_sensor_state *state;
	struct v4l2_subdev *sd;
//	int ret =0;
	dbg_fun;
	//back_sensor_init();
	state = kzalloc(sizeof(struct back_sensor_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

	sd = &state->sd;
	strcpy(sd->name, BACK_SENSOR_DRIVER_NAME);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &back_sensor_ops);
	dev_info(&client->dev, "*** back sensors has been probed\n");

	s_client = client;
	back_sensor_init_proc_node();

	return 0;

}

static int back_sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	printk("back_sensor_remove\n");

/* test by Albert_lee 2014-09-10 begin */
	back_sensor_power(sd, 0);
 /*Albert_lee 2014-09-10 end*/

	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));

	s_client = NULL;
	return 0;
}

static const struct i2c_device_id back_sensor_id[] = {
	{ BACK_SENSOR_DRIVER_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, back_sensor_id);

static struct i2c_driver back_sensor_i2c_driver =
{
	.driver = {
		.name = BACK_SENSOR_DRIVER_NAME,
	},
	.probe = back_sensor_probe,
	.remove = __devexit_p(back_sensor_remove),
	.id_table = back_sensor_id,
};

extern char g_selected_utmodel[];

static int __init back_sensor_mod_init(void)
{
	printk("%s\n",__func__);
//		return -1;

	if (strstr(g_selected_utmodel, "d108") ) {
		printk("[%s], no video sensor !\n",g_selected_utmodel);
		return -1;
	}
	return i2c_add_driver(&back_sensor_i2c_driver);
}

static void __exit back_sensor_mod_exit(void)
{
	printk("%s\n",__func__);
	i2c_del_driver(&back_sensor_i2c_driver);
}

module_init(back_sensor_mod_init);
module_exit(back_sensor_mod_exit);

MODULE_DESCRIPTION("Ut back sensors UXGA camera driver");
MODULE_AUTHOR("ut system team");
MODULE_LICENSE("GPL");
