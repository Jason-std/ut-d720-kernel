/*
 *  misc power and gpio proc file for Urbest
 *  Copyright (c) 2009 urbest Technologies, Inc.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/sizes.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>

#include "../../fs/proc/internal.h"

#include <plat/gpio-cfg.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <mach/regs-pmu.h>

#include "power_gpio.h"

#include <plat/sdhci.h>
#include <plat/devs.h>

#include <linux/sched.h>

#include <linux/regulator/consumer.h>

#define CONFIG_GPIO_DEBUG

#undef P_GPIO_DEBUG
#undef BDG
#ifdef CONFIG_GPIO_DEBUG
#define P_GPIO_DEBUG(x...) printk(KERN_DEBUG "[gpio drv]" x)
#define BDG(x...)  	printk(KERN_DEBUG "[gpio drv]  %s(); %d\n", __func__, __LINE__)
#else
#define P_GPIO_DEBUG(x...) do { } while(0)
#define BDG(x...) do { } while(0)
#endif



#define MAX_NODE_COUNT		(64)

#define PROC_NAME                    "power_gpio"
#define SUSPEND_PROC_NAME            "suspend_4x12"
static int s_suspend_ref = 0;

static int s_post_value[POWER_ITEM_MAX] = {-1};
static int s_post_value_init = 0;
static struct timer_list s_write_timer[POWER_ITEM_MAX];

static int s_pm_save_state[POWER_ITEM_MAX];


extern char g_selected_utmodel[];


volatile int gsm_power_switch = 0;

static inline unsigned int ms_to_jiffies(unsigned int ms)
{
        unsigned int j;
        j = (ms * HZ + 500) / 1000;
        return (j > 0) ? j : 1;
}


/*
	POWER_5V_EN=0,
	POWER_INT_EN,

	POWER_CAM_EN,
	POWER_CAMA_PD, //camera A power down
	POWER_CAMB_PD, //camera B power down
	POWER_CAM_RST, //camera reset

	POWER_GPS_RST,
	POWER_GPS_EN,

	POWER_GSM_SW,
	POWER_GSM_WUP,  //cpu wake up 3g/gsm

	POWER_LCD33_EN,
	POWER_LVDS_PD,
	POWER_BL_EN,

	POWER_HUB_RST,
	POWER_HUB_CON, //high en
	POWER_HUB2_EN,
	POWER_HUB2_RST,

	POWER_SPK_EN,

	POWER_USB_SW,
	POWER_MOTOR_EN,

	POWER_TS_RST,

	POWER_STATE_AC,
	POWER_STATE_CHARGE,

	POWER_SCAN_EN,
	POWER_SCAN_BUZ,
*/

static const char * s_node_names[POWER_ITEM_MAX] =
{
	/* 0*/
	"power_5v_en",
#if 0
	"power_arm_en",
	"power_3d_en",
#endif
	"power_int_en",
//	"power_cam_en",
	/* 5*/
//	"power_cama_pd", //camera A power down
//	"power_camb_pd", //camera B power down
//	"power_camb_rst",
#if 0
	"power_bt_en",
	"power_bt_rst",
	"power_bt_ldo",
	/*10*/
	"power_bt_wup",	//cpu wake up bt
#endif
	"power_gps_rst",
	"power_gps_en",
#if 1
	"power_wifi_en",	//wifi power enable
	"power_wifi_ldo",   //wifi inter ldo enable
#endif
	/*15*/
	"power_gsm_sw",
	"power_gsm_wup",  //cpu wake up 3g/gsm

	"power_lcd_en",
	"power_lcd_18v_en",
	"power_lvds_pd",
	"power_bl_en",

	"power_hub_rst",
	/*20*/
	"power_hub_con",
	"power_hub2_en",
	"power_hub2_rst",
	"power_spk_en",
	"power_usb_sw",
	"power_motor_en",
	"power_ts_rst",

	"power_state_ac",
	"power_state_charge",

	"power_scan_en",
	"power_scan_buz",
	"power_rfid_en",

	"power_fcam_28v",   // front camera
	"power_fcam_18v",
	"power_fcam_pd",
	"power_fcam_rst",

	"power_bcam_28v",  // back camera
	"power_bcam_18v",
	"power_bcam_pd",
	"power_bcam_rst",

	"power_cam_af",

};

struct power_gpio_node
{
	enum power_item index;
	unsigned int pin;
	int polarity;				    //0: low for enable, 1: high for enable
	int direction;				    //0: input only,     1: input/output
//	int (*power_func)(int on);		//extra power function
//	int (*read_func)(void);		//extra read function
	int (*write_func)(struct power_gpio_node , int);		//extra power function
	int (*read_func)(struct power_gpio_node);		//extra read function
};

static int usb_hub_reset(struct power_gpio_node node, int on);
static int gsm_power_ctl(struct power_gpio_node node,int on);
static int gsm_power_read(struct power_gpio_node node);
static int gps_power_on(struct power_gpio_node node,int on);

static int delay_1_ms(struct power_gpio_node node,int on)
{
	msleep(1);
	return 0;
}


static struct power_gpio_node s_gpio_node[POWER_ITEM_MAX+1] =
{

	{POWER_5V_EN,			EXYNOS4_GPK1(1), 1, 1, NULL},
/* added by Albert_lee 2013-09-20 begin */

/* OEM FOR D816 by Albert_lee 2014-04-23 begin */
	{POWER_5V_EN,			EXYNOS4_GPK1(2), 1, 1, NULL},
 /*Albert_lee 2014-04-23 end*/
//	{POWER_5V_EN,			EXYNOS4_GPL2(4), 1, 1, NULL},

 /*Albert_lee 2013-09-20 end*/
/*
	{POWER_ARM_EN,			EXYNOS4212_GPM3(5), 0, 1, NULL}, // low is  default volt, high is another volt (PMU cc)
	{POWER_3D_EN,			EXYNOS4212_GPM3(6), 0, 1, NULL}, // low is  default volt, high is another volt (PNU cc)
*/
	{POWER_INT_EN,			EXYNOS4212_GPM3(7), 0, 1, NULL}, // low is  default volt, high is another volt (PMU cc)
//	{POWER_CAM_EN,			EXYNOS4212_GPM3(0),  1, 1, NULL},
//	{POWER_CAMA_PD,		EXYNOS4_GPL0(0), 0, 1, NULL},
//	{POWER_CAMB_PD,		EXYNOS4_GPL0(1), 0, 1, NULL},
//	{POWER_CAM_RST,		EXYNOS4212_GPJ1(4), 0, 1, NULL},
/*
	{POWER_BT_EN,			EXYNOS4212_GPM3(1), 1, 1, NULL},
	{POWER_BT_RST,			EXYNOS4_GPL0(2), 0, 1, NULL},
	{POWER_BT_LDO,			EXYNOS4_GPL0(3), 1, 1, NULL},
 	{POWER_BT_WUP,			EXYNOS4_GPL0(4), 1, 1, NULL},
*/
 	{POWER_GPS_RST,		EXYNOS4_GPL0(6), 0, 1, NULL},
	{POWER_GPS_EN,			EXYNOS4_GPL1(6), 1, 1, gps_power_on},

#if 1
	{POWER_WIFI_EN,			EXYNOS4212_GPM3(1), 1, 1, NULL},
	{POWER_WIFI_LDO,		EXYNOS4_GPL1(1), 1, 1, NULL},
#endif
	{POWER_GSM_SW,		GPIO_GSM_POWER_ON_OFF, 0, 1, gsm_power_ctl},
	{POWER_GSM_WUP,		GPIO_GSM_WAKE_IN, 1, 1, NULL},
	{POWER_LCD33_EN,		EXYNOS4212_GPM2(3), 1, 1, NULL},
	{POWER_LVDS_PD,		EXYNOS4212_GPM2(4), 1, 1, NULL},
	{POWER_BL_EN,			EXYNOS4212_GPM4(0), 1, 1, NULL},

	{POWER_HUB_RST,		EXYNOS4212_GPM3(2), 1, 1, NULL},
	{POWER_HUB_CON,		EXYNOS4212_GPM3(3), 1, 1, NULL},
	{POWER_HUB2_RST,		GPIO_USB_HUB2_RESET, 0, 1, usb_hub_reset},

	{POWER_SPK_EN,			EXYNOS4212_GPM1(4), 1, 1, NULL},
	{POWER_USB_SW,			EXYNOS4212_GPM1(5), 1, 1, NULL},
	{POWER_MOTOR_EN,		EXYNOS4212_GPM1(6), 1, 1, NULL},
//	{POWER_TS_RST,			EXYNOS4212_GPM3(4), 1, 1, NULL},

	{POWER_STATE_AC,		EXYNOS4_GPX0(2), 1, 0, NULL},  //EINT2
	{POWER_STATE_CHARGE,	EXYNOS4_GPX2(7), 1, 0, NULL}, //EINT23

	{POWER_SCAN_EN,		GPIO_SCAN_EN, 1, 1, NULL},
	{POWER_SCAN_BUZ,		GPIO_SCAN_BUZ, 1, 0, NULL},

	{POWER_RFID_EN,			GPIO_RFID_EN, 1, 1, NULL},

	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, delay_1_ms},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
};


const static struct power_gpio_node s_gpio_node_vserion1[] =
{
	{POWER_5V_EN,			EXYNOS4_GPK1(1), 1, 1, NULL},

	{POWER_5V_EN,			EXYNOS4_GPK1(2), 1, 1, NULL},

#if 0
	{POWER_ARM_EN,			EXYNOS4212_GPM3(5), 0, 1, NULL}, // low is  default volt, high is another volt (PMU cc)
	{POWER_3D_EN,			EXYNOS4212_GPM3(6), 0, 1, NULL}, // low is  default volt, high is another volt (PNU cc)
#endif
	{POWER_INT_EN,			EXYNOS4212_GPM3(7), 0, 1, NULL}, // low is  default volt, high is another volt (PMU cc)
//	{POWER_CAM_EN,			EXYNOS4212_GPM3(0),  1, 1, NULL},
//	{POWER_CAMA_PD,		EXYNOS4_GPL0(0), 0, 1, NULL},
//	{POWER_CAMB_PD,		EXYNOS4_GPL0(1), 0, 1, NULL},
/*
	{POWER_BT_EN,			EXYNOS4212_GPM3(1), 1, 1, NULL},
	{POWER_BT_RST,			EXYNOS4_GPL0(2), 0, 1, NULL},
	{POWER_BT_LDO,			EXYNOS4_GPL0(3), 1, 1, NULL},
 	{POWER_BT_WUP,			EXYNOS4_GPL0(4), 1, 1, NULL},
*/
 	{POWER_GPS_RST,		EXYNOS4_GPL0(6), 0, 1, NULL},
	{POWER_GPS_EN,			EXYNOS4_GPL1(0), 1, 1, NULL},
/*
	{POWER_WIFI_EN,			EXYNOS4212_GPM3(1), 1, 1, NULL},
	{POWER_WIFI_LDO,		EXYNOS4_GPL1(1), 1, 1, NULL},
*/
	{POWER_GSM_SW,		GPIO_GSM_POWER_ON_OFF, 1, 1, NULL},
	{POWER_GSM_WUP,		GPIO_GSM_WAKE_IN, 1, 1, NULL},
	{POWER_LCD33_EN,		EXYNOS4212_GPM2(3), 1, 1, NULL},
	{POWER_LVDS_PD,		EXYNOS4212_GPM2(4), 1, 1, NULL},
	{POWER_HUB_RST,		EXYNOS4212_GPM3(2), 1, 1, NULL},
	{POWER_HUB_CON,		EXYNOS4212_GPM3(3), 1, 1, NULL},
	{POWER_SPK_EN,			EXYNOS4212_GPM1(4), 1, 1, NULL},
	{POWER_USB_SW,			EXYNOS4212_GPM1(5), 1, 1, NULL},
	{POWER_MOTOR_EN,		EXYNOS4212_GPM1(6), 1, 1, NULL},
//	{POWER_TS_RST,			EXYNOS4212_GPM3(4), 1, 1, NULL},


	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
};



static struct power_gpio_node s_gpio_node_d721[] =
{

	{POWER_5V_EN,			EXYNOS4_GPK1(1), 1, 1, NULL},
/* added by Albert_lee 2013-09-20 begin */

/* OEM FOR D816 by Albert_lee 2014-04-23 begin */
//	{POWER_5V_EN,			EXYNOS4_GPK1(2), 1, 1, NULL},
 /*Albert_lee 2014-04-23 end*/
//	{POWER_5V_EN,			EXYNOS4_GPL2(4), 1, 1, NULL},

 /*Albert_lee 2013-09-20 end*/
/*
	{POWER_ARM_EN,			EXYNOS4212_GPM3(5), 0, 1, NULL}, // low is  default volt, high is another volt (PMU cc)
	{POWER_3D_EN,			EXYNOS4212_GPM3(6), 0, 1, NULL}, // low is  default volt, high is another volt (PNU cc)
*/
	{POWER_INT_EN,			EXYNOS4212_GPM3(7), 0, 1, NULL}, // low is  default volt, high is another volt (PMU cc)
/*	{POWER_CAM_EN,			EXYNOS4212_GPM3(0),  1, 1, NULL},
	{POWER_CAMA_PD,		EXYNOS4_GPL0(0), 0, 1, NULL},
	{POWER_CAMB_PD,		EXYNOS4_GPL0(1), 0, 1, NULL},
	{POWER_CAM_RST,		EXYNOS4212_GPJ1(4), 0, 1, NULL}, */
/*
	{POWER_BT_EN,			EXYNOS4212_GPM3(1), 1, 1, NULL},
	{POWER_BT_RST,			EXYNOS4_GPL0(2), 0, 1, NULL},
	{POWER_BT_LDO,			EXYNOS4_GPL0(3), 1, 1, NULL},
 	{POWER_BT_WUP,			EXYNOS4_GPL0(4), 1, 1, NULL},
*/
 	{POWER_GPS_RST,		EXYNOS4_GPL0(6), 0, 1, NULL},
//	{POWER_GPS_EN,			EXYNOS4_GPL1(6), 1, 1, gps_power_on},

#if 1
	{POWER_WIFI_EN,			EXYNOS4212_GPM3(1), 1, 1, NULL},
	{POWER_WIFI_LDO,		EXYNOS4_GPL1(1), 1, 1, NULL},
#endif
//	{POWER_GSM_SW,		GPIO_GSM_POWER_ON_OFF, 0, 1, gsm_power_ctl},
	{POWER_GSM_WUP,		GPIO_GSM_WAKE_IN, 1, 1, NULL},
	{POWER_LCD33_EN,		EXYNOS4212_GPM2(3), 1, 1, NULL},
	{POWER_LCD18_EN,		EXYNOS4212_GPM2(0), 0, 1, NULL},  // Low enable
	{POWER_LVDS_PD,		EXYNOS4212_GPM2(4), 1, 1, NULL},
	{POWER_BL_EN,			EXYNOS4212_GPM4(0), 1, 1, NULL},

	{POWER_HUB_RST,		EXYNOS4212_GPM3(2), 1, 1, NULL},
	{POWER_HUB_CON,		EXYNOS4212_GPM3(3), 1, 1, NULL},
	{POWER_HUB2_RST,		GPIO_USB_HUB2_RESET, 0, 1, usb_hub_reset},

	{POWER_SPK_EN,			EXYNOS4212_GPM1(4), 1, 1, NULL},
	{POWER_USB_SW,			EXYNOS4212_GPM1(5), 1, 1, NULL},
	{POWER_MOTOR_EN,		EXYNOS4212_GPM1(6), 1, 1, NULL},
//	{POWER_TS_RST,			EXYNOS4212_GPM3(4), 1, 1, NULL},

	{POWER_STATE_AC,		EXYNOS4_GPX0(2), 1, 0, NULL},  //EINT2
	{POWER_STATE_CHARGE,	EXYNOS4_GPX2(7), 1, 0, NULL}, //EINT23

//	{POWER_SCAN_EN,		GPIO_SCAN_EN, 1, 1, NULL},
//	{POWER_SCAN_BUZ,		GPIO_SCAN_BUZ, 1, 0, NULL},

//	{POWER_RFID_EN,			GPIO_RFID_EN, 1, 1, NULL},

	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, delay_1_ms},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
};

static struct power_gpio_node s_gpio_node_s106[] =
{

	{POWER_5V_EN,			EXYNOS4_GPK1(1), 1, 1, NULL},
/* added by Albert_lee 2013-09-20 begin */

/* OEM FOR D816 by Albert_lee 2014-04-23 begin */
	{POWER_5V_EN,			EXYNOS4_GPK1(2), 1, 1, NULL},
 /*Albert_lee 2014-04-23 end*/
//	{POWER_5V_EN,			EXYNOS4_GPL2(4), 1, 1, NULL},

 /*Albert_lee 2013-09-20 end*/
/*
	{POWER_ARM_EN,			EXYNOS4212_GPM3(5), 0, 1, NULL}, // low is  default volt, high is another volt (PMU cc)
	{POWER_3D_EN,			EXYNOS4212_GPM3(6), 0, 1, NULL}, // low is  default volt, high is another volt (PNU cc)
*/
	{POWER_INT_EN,			EXYNOS4212_GPM3(7), 0, 1, NULL}, // low is  default volt, high is another volt (PMU cc)

 	{POWER_GPS_RST,		EXYNOS4_GPL0(6), 0, 1, NULL},
	{POWER_GPS_EN,			EXYNOS4_GPL1(6), 1, 1, gps_power_on},

#if 1
	{POWER_WIFI_EN,			EXYNOS4212_GPM3(1), 1, 1, NULL},
	{POWER_WIFI_LDO,		EXYNOS4_GPL1(1), 1, 1, NULL},
#endif
	{POWER_GSM_SW,		GPIO_GSM_POWER_ON_OFF, 0, 1, gsm_power_ctl,gsm_power_read},
	{POWER_GSM_WUP,		GPIO_GSM_WAKE_IN, 1, 1, NULL},
	{POWER_LCD33_EN,		EXYNOS4212_GPM2(3), 1, 1, NULL},
	{POWER_LVDS_PD,		EXYNOS4212_GPM2(4), 1, 1, NULL},
	{POWER_BL_EN,			EXYNOS4212_GPM4(0), 1, 1, NULL},

	{POWER_HUB_RST,		EXYNOS4212_GPM3(2), 1, 1, NULL},
	{POWER_HUB_CON,		EXYNOS4212_GPM3(3), 1, 1, NULL},
	{POWER_HUB2_RST,		GPIO_USB_HUB2_RESET, 0, 1, usb_hub_reset},

	{POWER_SPK_EN,			EXYNOS4212_GPM1(4), 1, 1, NULL},
	{POWER_USB_SW,			EXYNOS4212_GPM1(5), 1, 1, NULL},
	{POWER_MOTOR_EN,		EXYNOS4212_GPM1(6), 1, 1, NULL},
//	{POWER_TS_RST,			EXYNOS4212_GPM3(4), 1, 1, NULL},

	{POWER_STATE_AC,		EXYNOS4_GPX0(2), 1, 0, NULL},  //EINT2
	{POWER_STATE_CHARGE,	EXYNOS4_GPX2(7), 1, 0, NULL}, //EINT23

	{POWER_FCAM_28V,	EXYNOS4212_GPM3(0), 1, 1, NULL}, 
	{POWER_FCAM_18V,	EXYNOS4212_GPM3(0), 1, 1, NULL}, 
	{POWER_FCAM_PD,	EXYNOS4_GPL0(1), 1, 1, NULL}, 
	{POWER_FCAM_RST,	EXYNOS4212_GPJ1(4), 1, 1, NULL}, 

	{POWER_BCAM_28V,	EXYNOS4212_GPM3(0), 1, 1, NULL}, 
	{POWER_BCAM_18V,	EXYNOS4212_GPM3(0), 1, 1, NULL}, 
	{POWER_BCAM_PD,	EXYNOS4_GPL0(0), 1, 1, NULL}, 
	{POWER_BCAM_RST,	EXYNOS4212_GPJ1(4), 1, 1, NULL}, 



	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, delay_1_ms},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
};

extern void sdhci_s3c_notify_change(struct platform_device *dev, int state);

static int gsm_on=0;

static int power_gsm_sdio_thread(int * on)
{
	printk("enter %s:on=%d\n",__func__,*on);
	if(*on){
		printk("111%s:on=%d 111111111\n",__func__,*on);
		sdhci_s3c_notify_change(&s3c_device_hsmmc1, *on);
		msleep(6000);
	}
	printk("222222%s:on=%d 222222\n",__func__,*on);
	sdhci_s3c_notify_change(&s3c_device_hsmmc1, *on);
	return 1;
}

static int power_gsm_sdio(struct power_gpio_node node ,int on)
{
	gpio_direction_output(node.pin,node.polarity ?on:!on);
	gsm_on=on;
	kernel_thread(power_gsm_sdio_thread, &gsm_on, 0);
	return 1;
}

extern int hdmi_plug_flag;
static int power_spk( struct power_gpio_node node , int on)
{
	printk("enter %s,hdmi=%d,on=%d\n",__func__,hdmi_plug_flag ,on);
	if(hdmi_plug_flag)
		gpio_direction_output(node.pin,node.polarity ?0:1);
	else
		gpio_direction_output(node.pin,node.polarity ?on:!on);
	return 1;
}

static int power_cam_dldo2_2v8(struct power_gpio_node node , int on)
{
	struct regulator *vdd28_cam_regulator=regulator_get(NULL, "dldo2_cam_avdd_2v8");
	if(on)
		regulator_enable(vdd28_cam_regulator);
	else
		regulator_disable(vdd28_cam_regulator);
	regulator_put(vdd28_cam_regulator);
}

static int power_cam_eldo3_af(struct power_gpio_node node , int on)
{
	struct regulator *vdd18_cam_regulator=regulator_get(NULL, "vdd_eldo3_18");
	if(on)
		regulator_enable(vdd18_cam_regulator);
	else
		regulator_disable(vdd18_cam_regulator);
	regulator_put(vdd18_cam_regulator);
}

static struct power_gpio_node s_gpio_node_d720[] =
{

	{POWER_5V_EN,			EXYNOS4_GPK1(2), 1, 1, NULL},
	{POWER_INT_EN,			EXYNOS4212_GPM3(7), 0, 1, NULL}, // low is  default volt, high is another volt (PMU cc)

	{POWER_GPS_EN,			EXYNOS4212_GPM4(6), 1, 1, NULL},

#if 1
	{POWER_WIFI_EN,			EXYNOS4212_GPM3(1), 1, 1, NULL},
	{POWER_WIFI_LDO,		EXYNOS4_GPL1(1), 1, 1, NULL},
#endif
	{POWER_GSM_SW,		GPIO_GSM_POWER_ON_OFF, 0, 1, gsm_power_ctl,gsm_power_read},
	
//	{POWER_GSM_WUP,		GPIO_GSM_WAKE_IN, 1, 1, NULL},
//	{POWER_GSM_SW,		GPIO_GSM_POWER_EN, 1, 1, power_gsm_sdio},
	{POWER_LCD33_EN,		EXYNOS4212_GPM2(3), 1, 1, NULL},
	{POWER_LCD18_EN,		EXYNOS4212_GPM2(0), 0, 1, NULL},  // notice: Low for enable

	{POWER_LVDS_PD,		EXYNOS4212_GPM2(4), 1, 1, NULL},
	{POWER_BL_EN,			EXYNOS4212_GPM4(0), 1, 1, NULL},

	{POWER_HUB_RST,		EXYNOS4212_GPM3(2), 1, 1, NULL},
	{POWER_HUB_CON,		EXYNOS4212_GPM3(3), 1, 1, NULL},
	{POWER_HUB2_RST,		GPIO_USB_HUB2_RESET, 0, 1, usb_hub_reset},

	{POWER_SPK_EN,			EXYNOS4212_GPM1(4), 1, 1, power_spk},
	{POWER_USB_SW,			EXYNOS4212_GPM1(5), 1, 1, NULL},
	{POWER_MOTOR_EN,		EXYNOS4212_GPM1(6), 1, 1, NULL},
	{POWER_TS_RST,			EXYNOS4212_GPM3(4), 1, 1, NULL},

	{POWER_STATE_AC,		EXYNOS4_GPX0(2), 1, 0, NULL},  //EINT2

	{POWER_FCAM_28V,	                             0, 1, 1, power_cam_dldo2_2v8}, 
	{POWER_FCAM_18V,	     GPIO_EXAXP22(1), 1, 1, NULL}, 
	{POWER_FCAM_PD,	EXYNOS4_GPL0(1), 1, 1, NULL}, 
	{POWER_FCAM_RST,	EXYNOS4212_GPJ1(4), 1, 1, NULL}, 

	{POWER_BCAM_28V,	                             0, 1, 1, power_cam_dldo2_2v8}, 
	{POWER_BCAM_18V,	     GPIO_EXAXP22(1), 1, 1, NULL}, 
	{POWER_BCAM_PD,	EXYNOS4_GPL0(0), 1, 1, NULL}, 
	{POWER_BCAM_RST,	EXYNOS4212_GPJ1(4), 1, 1, NULL}, 

	{POWER_CAM_AF,	                                    0,  1, 1, power_cam_eldo3_af}, 

	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, delay_1_ms},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
};

const static struct power_gpio_node s_gpio_node_d816[] =
{
	{POWER_5V_EN,			EXYNOS4_GPL2(0), 1, 1, NULL},

#if 0
	{POWER_ARM_EN,			EXYNOS4212_GPM3(5), 0, 1, NULL}, // low is  default volt, high is another volt (PMU cc)
	{POWER_3D_EN,			EXYNOS4212_GPM3(6), 0, 1, NULL}, // low is  default volt, high is another volt (PNU cc)
#endif
	{POWER_INT_EN,			EXYNOS4212_GPM3(7), 0, 1, NULL}, // low is  default volt, high is another volt (PMU cc)
/*	{POWER_CAM_EN,			EXYNOS4212_GPM3(0),  1, 1, NULL},
	{POWER_CAMA_PD,		EXYNOS4_GPL0(0), 0, 1, NULL},
	{POWER_CAMB_PD,		EXYNOS4_GPL0(1), 0, 1, NULL},  */
/*
	{POWER_BT_EN,			EXYNOS4212_GPM3(1), 1, 1, NULL},
	{POWER_BT_RST,			EXYNOS4_GPL0(2), 0, 1, NULL},
	{POWER_BT_LDO,			EXYNOS4_GPL0(3), 1, 1, NULL},
 	{POWER_BT_WUP,			EXYNOS4_GPL0(4), 1, 1, NULL},
*/
 	{POWER_GPS_RST,		EXYNOS4_GPL0(6), 0, 1, NULL},
	{POWER_GPS_EN,			EXYNOS4_GPL1(0), 1, 1, NULL},
/*
	{POWER_WIFI_EN,			EXYNOS4212_GPM3(1), 1, 1, NULL},
	{POWER_WIFI_LDO,		EXYNOS4_GPL1(1), 1, 1, NULL},
*/
	{POWER_GSM_SW,		GPIO_GSM_POWER_ON_OFF, 1, 1, NULL},
	{POWER_GSM_WUP,		GPIO_GSM_WAKE_IN, 1, 1, NULL},
	{POWER_LCD33_EN,		EXYNOS4212_GPM2(3), 1, 1, NULL},
	{POWER_LVDS_PD,		EXYNOS4212_GPM2(4), 1, 1, NULL},
	{POWER_HUB_RST,		EXYNOS4212_GPM3(2), 1, 1, NULL},
	{POWER_HUB_CON,		EXYNOS4212_GPM3(3), 1, 1, NULL},
	{POWER_SPK_EN,			EXYNOS4212_GPM1(4), 1, 1, NULL},
	{POWER_USB_SW,			EXYNOS4212_GPM1(5), 1, 1, NULL},
	{POWER_MOTOR_EN,		EXYNOS4212_GPM1(6), 1, 1, NULL},
	{POWER_TS_RST,			EXYNOS4212_GPM3(4), 1, 1, NULL},

	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
	{POWER_ITEM_INVALID,	EXYNOS4_GPL1(6), 1, 1, NULL},      // unused, only for expand array size
};


static void power_gpio_input_init(void)
{
	int i;
	for(i = 0; i < ARRAY_SIZE(s_gpio_node); i++)
	{
		if(s_gpio_node[i].direction == 0)	//0 for input only
		{
			s3c_gpio_setpull(s_gpio_node[i].pin, S3C_GPIO_PULL_NONE);
			gpio_direction_input(s_gpio_node[i].pin);
		}
	}
}

int read_power_item_value(int index)
{
	int i;
	power_gpio_input_init();

	for(i = 0; i < ARRAY_SIZE(s_gpio_node); i++)
	{
		if(s_gpio_node[i].index == index)
		{
			if(s_gpio_node[i].read_func){
				return s_gpio_node[i].read_func(s_gpio_node[i]);
			}
		
			if(s_gpio_node[i].polarity == 0)
				return !gpio_get_value(s_gpio_node[i].pin);
			else
				return gpio_get_value(s_gpio_node[i].pin);
		}
	}
	return 0;
}
EXPORT_SYMBOL(read_power_item_value);

static int is_ignored_item(int index, int value)
{
	return 0;
}


int write_power_item_value(int index, int value)
{
	int i;
	//printk("write_power_item_value %s = %d\n", s_node_names[index], value);

	if(index >= 0 && index < POWER_ITEM_MAX)
		s_post_value[index] = -1;	//clear the timer action

	if(is_ignored_item(index, value))
		return 0;

	for(i = 0; i < ARRAY_SIZE(s_gpio_node); i++)
	{
		if(s_gpio_node[i].index == index)
		{

			if(s_gpio_node[i].write_func){
				s_gpio_node[i].write_func(s_gpio_node[i],value);
				break;
			}
		
			if(s_gpio_node[value].direction == 0)	//input only
				continue;

			s3c_gpio_setpull(s_gpio_node[i].pin, S3C_GPIO_PULL_UP);
			if(s_gpio_node[i].polarity == 0)
				gpio_direction_output(s_gpio_node[i].pin, (value > 0) ? 0 : 1);
			else
				gpio_direction_output(s_gpio_node[i].pin, value);
		}
	}

	if(value == 123)
		panic("Watch dog test\n");

	return 0;
}
EXPORT_SYMBOL(write_power_item_value);


static void write_timer_handler(unsigned long data)
{
	int index = data;
	if(index >= 0 && index < POWER_ITEM_MAX && s_post_value[index] >= 0)
	{
	/*
		if(POWER_SPK_EN==index && s_post_value[index]==0 && ) {

		} else
*/
		{
			write_power_item_value(index, s_post_value[index]);
		}
		s_post_value[index] = -1;
	}
}

static int touch_write_timer(int index, int value, int ms_debounce)
{
	if(index >= 0 && index < POWER_ITEM_MAX)
	{
		s_post_value[index] = value;
		s_write_timer[index].data = index;
		s_write_timer[index].function = write_timer_handler;
		mod_timer(&s_write_timer[index], jiffies + ms_to_jiffies(ms_debounce));
		return 0;
	}
	else
		return -1;
}


int post_power_item_value(int index, int value, int ms_debounce)
{
	int i;
	//printk("post_power_item_value %s = %d, delay=%dms\n", s_node_names[index], value, ms_debounce);
	if(!s_post_value_init)
	{
		for(i = 0; i < POWER_ITEM_MAX; i++)
		{
			s_post_value[i] = -1;
			init_timer(&s_write_timer[i]);
		}
		s_post_value_init = 1;
	}

	return touch_write_timer(index, value, ms_debounce);
}
EXPORT_SYMBOL(post_power_item_value);


static int usb_hub_reset(struct power_gpio_node node,int on)
{
	P_GPIO_DEBUG("%s(%d)\n",__func__, on);
#if 0
	s3c_gpio_setpull(EXYNOS4_GPL2(2), S3C_GPIO_PULL_DOWN);
	gpio_direction_output(EXYNOS4_GPL2(2), GPIO_LEVEL_LOW);
//	msleep(10);
	gpio_direction_output(EXYNOS4_GPL2(2), GPIO_LEVEL_HIGH);
	udelay(10);
	gpio_direction_output(EXYNOS4_GPL2(2), GPIO_LEVEL_LOW);
	s3c_gpio_setpull(EXYNOS4_GPL2(2), S3C_GPIO_PULL_DOWN);
//	msleep(5);
#endif
	return 0;
}



static int gps_power_on(struct power_gpio_node node,int on)
{
	P_GPIO_DEBUG("%s(%d)\n",__func__, on);

	return 0;

}

static int gsm_power_ctl(struct power_gpio_node node,int on)
{
	if(on) {
		gsm_power_switch = 1;
	} else {
		gsm_power_switch = 0;
	}

	return 0;
}

static int gsm_power_read(struct power_gpio_node node)
{
	return gsm_power_switch;
}

static void power_gpio_sleep_mode_init(void)
{
	P_GPIO_DEBUG("%s();\n",__func__);
}

static int scan_en_tmp = 0;
static int rfid_en_tmp = 0;
void ut7gm_pm_suspend(void)
{
//	int i;

	P_GPIO_DEBUG("%s();\n",__func__);

#if 1

#if 0
//	write_power_item_value(POWER_SPK_EN, 0);
	for(i = 0; i < POWER_ITEM_MAX; i++)
		s_pm_save_state[i] = read_power_item_value(i);
#endif
	power_gpio_sleep_mode_init();

	scan_en_tmp = read_power_item_value(POWER_SCAN_EN);
	rfid_en_tmp = read_power_item_value(POWER_RFID_EN);
	write_power_item_value(POWER_SCAN_EN, 0);
	write_power_item_value(POWER_RFID_EN, 0);

//	extern void usb_hub_cleanup(void);
//	usb_hub_cleanup();
#endif

	s_suspend_ref = 1;

}
EXPORT_SYMBOL(ut7gm_pm_suspend);

void ut7gm_pm_wakeup(void)  // NCNCNCNCNCN
{
	int i;
	P_GPIO_DEBUG("%s();\n",__func__);

	if(scan_en_tmp)
		write_power_item_value(POWER_SCAN_EN, 1);

	if(rfid_en_tmp)
		write_power_item_value(POWER_RFID_EN, 1);
#if 0
	for(i = 0; i < POWER_ITEM_MAX; i++) {
		if(i == POWER_LCD33_EN || i==POWER_LVDS_PD)
			continue;

		write_power_item_value(i, s_pm_save_state[i]);
	}
#endif


	printk(KERN_DEBUG "ut7gm_pm_wakeup--\n");
}
EXPORT_SYMBOL(ut7gm_pm_wakeup);

void ut7gm_pm_power_off(void)
{
	P_GPIO_DEBUG("%s();\n",__func__);

	write_power_item_value(POWER_SPK_EN, 0);
//	write_power_item_value(POWER_GSM_SW, 0);

	writel(0x5200, S5P_PS_HOLD_CONTROL);

	while(1);
}

void ut7gm_pm_power_off_prepare(void)
{
	P_GPIO_DEBUG("%s();\n",__func__);
}

static void power_gpio_on_boot(void)
{
	P_GPIO_DEBUG("%s();\n",__func__);

	power_gpio_input_init(); // first

	write_power_item_value(POWER_SPK_EN, 0);
	write_power_item_value(POWER_5V_EN, 1);

	write_power_item_value(POWER_SCAN_EN, 1);

//	write_power_item_value(POWER_BT_EN, 1);

// 3G module init start
	gpio_direction_output(GPIO_GSM_POWER_ON_OFF,  GPIO_LEVEL_LOW);
	s3c_gpio_setpull(GPIO_GSM_WAKE_IN, S3C_GPIO_PULL_NONE);
	gpio_direction_output(GPIO_GSM_WAKE_IN, GPIO_LEVEL_LOW);

// 3G module init end
	power_gpio_sleep_mode_init();

//	gpio_direction_output(GPIO_USB_HUB2_RESET, GPIO_LEVEL_HIGH);		// POWER_HUB2_RST  test
/* TODO by Albert_lee 2013-09-20 begin */
	gpio_direction_output(GPIO_USB_HUB2_RESET, GPIO_LEVEL_LOW);		// POWER_HUB2_RST  test
 /*Albert_lee 2013-09-20 end*/


	write_power_item_value(POWER_GSM_SW, 0);


	if (is_axp_pmu) {

	} else {
		if(!pm_power_off)
			pm_power_off = ut7gm_pm_power_off;
	}

	if(!pm_power_off_prepare)
		pm_power_off_prepare = ut7gm_pm_power_off_prepare;

//add by jerry for s501 usb keyboard
	if(!strncmp(g_selected_utmodel, "s501", strlen("s501"))) {
		gpio_direction_output(EXYNOS4212_GPM4(1),1);
	}else if(!strncmp(g_selected_utmodel, "d722", strlen("d722"))) { // for d722 eth
		gpio_direction_output(EXYNOS4212_GPM4(1),1);
	}
//add end

}

static void init_request_gpio(void)
{
	int i;
//	unsigned int gpio;
	for(i = 0; i < ARRAY_SIZE(s_gpio_node); i++)
	{
		gpio_request(s_gpio_node[i].pin, PROC_NAME);
		//printk("power_gpio: %d, %s\n", s_gpio_node[i].pin, s_node_names[i]);
	}

}

static void init_gpio_nodes(void)
{

	int i;
	extern char g_selected_utmodel[];

	if(!strcmp(g_selected_utmodel, "version1"))
	{
		printk(KERN_DEBUG "power_gpio: detected version1 board type PCB.\n");
		for(i = 0; i < ARRAY_SIZE(s_gpio_node); i++) {
			s_gpio_node[i].index = POWER_ITEM_INVALID;
		}
		memcpy(s_gpio_node, s_gpio_node_vserion1, sizeof(s_gpio_node_vserion1));
	}

	if(strstr(g_selected_utmodel, "d816"))
	{
		printk(KERN_DEBUG "power_gpio: detected version1 board type PCB.\n");
		for(i = 0; i < ARRAY_SIZE(s_gpio_node); i++) {
			s_gpio_node[i].index = POWER_ITEM_INVALID;
		}
		memcpy(s_gpio_node, s_gpio_node_d816, sizeof(s_gpio_node_d816));
	}

	if(strstr(g_selected_utmodel, "d720"))
	{
		printk(KERN_DEBUG "power_gpio: detected version1 board type PCB.\n");
		for(i = 0; i < ARRAY_SIZE(s_gpio_node); i++) {
			s_gpio_node[i].index = POWER_ITEM_INVALID;
		}
		memcpy(s_gpio_node, s_gpio_node_d720, sizeof(s_gpio_node_d720));
	}

	if(strstr(g_selected_utmodel, "d721"))
	{
		printk(KERN_DEBUG "power_gpio: detected version1 board type PCB.\n");
		for(i = 0; i < ARRAY_SIZE(s_gpio_node); i++) {
			s_gpio_node[i].index = POWER_ITEM_INVALID;
		}
		memcpy(s_gpio_node, s_gpio_node_d721, sizeof(s_gpio_node_d721));
	}
	if(strstr(g_selected_utmodel, "s106"))
	{
		printk(KERN_DEBUG "power_gpio: detected s106 board type PCB.\n");
		for(i = 0; i < ARRAY_SIZE(s_gpio_node); i++) {
			s_gpio_node[i].index = POWER_ITEM_INVALID;
		}
		memcpy(s_gpio_node, s_gpio_node_s106, sizeof(s_gpio_node_s106));
	}

}

static int stringToIndex(const char * str)
{
	int i;
	if(!str) {
		return POWER_ITEM_INVALID;
	}

	for(i = 0; i < POWER_ITEM_MAX; i++) {
		if(!strncmp(s_node_names[i], str, strlen(s_node_names[i]))) {
			return i;
		}
	}
	return POWER_ITEM_INVALID;
}

static int power_gpio_proc_write(struct file *file, const char *buffer,
                           unsigned long count, void *data)
{
	int value;
	value = 0;
	sscanf(buffer, "%d", &value);
	write_power_item_value(stringToIndex((const char*)data), value);
	return count;
}

static int power_gpio_proc_read(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
	int len;
	int value;
	int index=stringToIndex((const char*)data);
/*	if(POWER_GSM_SW==index) {
		value = gsm_power_switch;
	} else { */
		value = read_power_item_value(index);
//	}
	len = sprintf( page, "%d\n", value);
	return len;
}

static int suspend_writeproc(struct file *file,const char *buffer,
                           unsigned long count, void *data)
{
	if(buffer[0] == '0') s_suspend_ref = 0;
	return count;
}


static int suspend_readproc(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
	int len = 0;
	len = sprintf(page, "suspend=%d\n", s_suspend_ref);
	return len;
}

static int __init power_gpio_init(void)
{
	int i, ret = 0;
	struct proc_dir_entry *root_entry;
	struct proc_dir_entry *entry;

	root_entry = proc_mkdir(PROC_NAME, &proc_root);

	if(root_entry) {
		for(i = 0; i < POWER_ITEM_MAX; i++) {
			entry = create_proc_entry(s_node_names[i], 0666, root_entry);
			if(entry) {
				entry->write_proc = power_gpio_proc_write;
				entry->read_proc = power_gpio_proc_read;
				entry->data = (void*)s_node_names[i];
			}
		}

		entry = create_proc_entry(SUSPEND_PROC_NAME, 0666, root_entry);
		if(entry) {
			entry->write_proc = suspend_writeproc;
			entry->read_proc = suspend_readproc;
		}
	} else {
		printk(KERN_ERR "power_gpio_init init err .\n");
		ret = -1;
		goto err_out;
	}

	printk(KERN_INFO "power_gpio_init enter .\n");

	init_gpio_nodes();
	init_request_gpio();
	power_gpio_on_boot();

err_out:
	return ret;
}

static void __exit power_gpio_exit(void)
{
	remove_proc_entry(PROC_NAME, &proc_root);
}


module_init(power_gpio_init);
module_exit(power_gpio_exit);

MODULE_DESCRIPTION("power_gpio config proc file");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("urbest, inc.");

