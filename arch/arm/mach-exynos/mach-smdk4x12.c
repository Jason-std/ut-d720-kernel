/* linux/arch/arm/mach-exynos/mach-smdk4x12.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/clk.h>
#include <linux/lcd.h>
#include <linux/gpio.h>
#include <linux/gpio_event.h>
#include <linux/gpio_keys.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#ifdef CONFIG_BACKLIGHT_PWM
#include <linux/pwm_backlight.h>
#endif
#include <linux/input.h>
#include <linux/mmc/host.h>
#include <linux/regulator/machine.h>
#ifdef CONFIG_REGULATOR_MAX8649
#include <linux/regulator/max8649.h>
#endif
#include <linux/regulator/fixed.h>
#include <linux/mfd/wm8994/pdata.h>
#if defined(CONFIG_MFD_MAX8997)
#include <linux/mfd/max8997.h>
#endif
#ifdef CONFIG_MFD_MAX77686
#include <linux/mfd/max77686.h>
#endif
#include <linux/v4l2-mediabus.h>
#include <linux/memblock.h>
#include <linux/delay.h>
#if defined(CONFIG_S5P_MEM_CMA)
#include <linux/cma.h>
#endif
#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif
#include <linux/phy.h>
#include <linux/notifier.h>
#include <linux/reboot.h>

#include <asm/mach/arch.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/exynos4.h>
#include <plat/cpu.h>
#include <plat/clock.h>
#include <plat/keypad.h>
#include <plat/devs.h>
#include <plat/fb.h>
#include <plat/fb-s5p.h>
#include <plat/fb-core.h>
#include <plat/regs-fb-v4.h>
#include <plat/backlight.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-adc.h>
#include <plat/adc.h>
#include <plat/iic.h>
#include <plat/pd.h>
#include <plat/sdhci.h>
#include <plat/mshci.h>
#include <plat/ehci.h>
#include <plat/usbgadget.h>
#include <plat/s3c64xx-spi.h>
#if defined(CONFIG_VIDEO_FIMC)
#include <plat/fimc.h>
#elif defined(CONFIG_VIDEO_SAMSUNG_S5P_FIMC)
#include <plat/fimc-core.h>
#include <media/s5p_fimc.h>
#endif
#if defined(CONFIG_VIDEO_FIMC_MIPI)
#include <plat/csis.h>
#elif defined(CONFIG_VIDEO_S5P_MIPI_CSIS)
#include <plat/mipi_csis.h>
#endif
#include <plat/tvout.h>
#include <plat/media.h>
#include <plat/regs-srom.h>
#include <plat/tv-core.h>
#if defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC) || defined(CONFIG_VIDEO_MFC5X)
#include <plat/s5p-mfc.h>
#endif

#include <media/s5k4ba_platform.h>
#include <media/ut2055_platform.h>
#include <media/s5k4ea_platform.h>
#include <media/exynos_flite.h>
#include <media/exynos_fimc_is.h>
#include <video/platform_lcd.h>
#include <media/m5mo_platform.h>
#include <media/m5mols.h>
#include <mach/board_rev.h>
#include <mach/map.h>
#include <mach/spi-clocks.h>
#include <mach/exynos-ion.h>
#include <mach/regs-pmu.h>
#ifdef CONFIG_EXYNOS4_DEV_DWMCI
#include <mach/dwmci.h>
#endif
#include <mach/map.h>
#include <mach/regs-pmu.h>
#ifdef CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION
#include <mach/secmem.h>
#endif
#include <mach/dev.h>
#include <mach/ppmu.h>
#ifdef CONFIG_EXYNOS_C2C
#include <mach/c2c.h>
#endif
#if defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC) || defined(CONFIG_VIDEO_MFC5X)
#include <plat/s5p-mfc.h>
#endif

#ifdef CONFIG_FB_S5P_MIPI_DSIM
#include <mach/mipi_ddi.h>
#include <mach/dsim.h>
#include <../../../drivers/video/samsung/s3cfb.h>
#endif
#include <plat/fimg2d.h>
#include <mach/dev-sysmmu.h>
#include <plat/sysmmu.h>

#ifdef CONFIG_VIDEO_SAMSUNG_S5P_FIMC
#include <plat/fimc-core.h>
#include <media/s5p_fimc.h>
#endif

#ifdef CONFIG_VIDEO_JPEG_V2X
#include <plat/jpeg.h>
#endif

#ifdef CONFIG_MFD_S5M_CORE
#include <linux/mfd/s5m87xx/s5m-core.h>
#include <linux/mfd/s5m87xx/s5m-pmic.h>
#endif

#include <mach/gpio-ut4x12.h>
#if defined(CONFIG_EXYNOS_THERMAL)
#include <mach/tmu.h>
#endif

#if defined(CONFIG_EXYNOS4_SETUP_THERMAL)
#include <plat/s5p-tmu.h>
#include <mach/regs-tmu.h>
#endif

#ifdef CONFIG_PN544

#include <linux/pn544.h>
#endif

#include "board-smdk4x12.h"

#if (defined CONFIG_BCM2079X_I2C) || (defined CONFIG_BCM2079X_I2C_MODULE)
#include <linux/nfc/bcm2079x.h>
#endif
#include <linux/power/bq24190_charger.h>
#include <urbetter/check.h>
//int samsung_board_rev;

/******************************  add by urbettr for OEM start *****************************************/
int g_touchscreen_init = 0;
EXPORT_SYMBOL(g_touchscreen_init);

char g_selected_battery[32] = {'\0'};
EXPORT_SYMBOL(g_selected_battery);
static  int __init select_battery(char *str)
{
	printk(KERN_DEBUG"%s:str:%s\n",__func__,str);
	strcpy(g_selected_battery,str);
	return 0;
}
__setup("battery=",select_battery);

char g_selected_utmodel[32] = {'\0'};
EXPORT_SYMBOL(g_selected_utmodel);
static int __init select_utmodel(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	strcpy(g_selected_utmodel, str);
	return 0;
}
__setup("utmodel=", select_utmodel);


char g_selected_pcb[32] = {'\0'};
EXPORT_SYMBOL(g_selected_pcb);
static int __init select_pcb(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	strcpy(g_selected_pcb, str);
	return 0;
}
__setup("pcb=", select_pcb);

char g_selected_bltype[32] = {'\0'};
EXPORT_SYMBOL(g_selected_bltype);
static int __init select_bltype(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	strcpy(g_selected_bltype, str);
	return 0;
}
__setup("bltype=", select_bltype);


char g_selected_codec[32] = "wm8978";
EXPORT_SYMBOL(g_selected_codec);
static int __init select_codec(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	strcpy(g_selected_codec, str);
	return 0;
}
__setup("codec=", select_codec);


char g_selected_bt[32] = {'\0'};
EXPORT_SYMBOL(g_selected_bt);
static int __init select_bt(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	strcpy(g_selected_bt, str);
	return 0;
}
__setup("bt=", select_bt);


char g_selected_wifi[32] = "mt6x";
EXPORT_SYMBOL(g_selected_wifi);
static int __init select_wifi(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	strcpy(g_selected_wifi, str);
	return 0;
}
__setup("wifi=", select_wifi);


char g_selected_tp[32] = {'\0'};
EXPORT_SYMBOL(g_selected_tp);
static int __init select_tp(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	strcpy(g_selected_tp, str);
	return 0;
}
__setup("tp=", select_tp);


char g_selected_fm[32] = {'\0'};
EXPORT_SYMBOL(g_selected_fm);
static int __init select_fm(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	strcpy(g_selected_fm, str);
	return 0;
}
__setup("fm=", select_fm);

char g_selected_gsmd[32] = {'\0'};
EXPORT_SYMBOL(g_selected_gsmd);
static int __init select_gsmd(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	strcpy(g_selected_gsmd, str);
	return 0;
}
__setup("gsmd=", select_gsmd);


char g_selected_came[32] = {'\0'};
EXPORT_SYMBOL(g_selected_came);
static int __init select_came(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	strcpy(g_selected_came, str);
	return 0;
}
__setup("came=", select_came);



char g_selected_nfc[32] = {'\0'};
EXPORT_SYMBOL(g_selected_nfc);
static int __init select_nfc(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	strcpy(g_selected_nfc, str);
	return 0;
}
__setup("nfc=", select_nfc);


char g_selected_umsvor[32] = {'\0'};
EXPORT_SYMBOL(g_selected_umsvor);
static int __init select_umsvor(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	strcpy(g_selected_umsvor, str);
	return 0;
}
__setup("umsvor=", select_umsvor);

char g_selected_umspct[32] = {'\0'};
EXPORT_SYMBOL(g_selected_umspct);
static int __init select_umspct(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	strcpy(g_selected_umspct, str);
	return 0;
}
__setup("umspct=", select_umspct);

// only xx:xx:xx:11:22:33
unsigned int g_selected_mac_id=0;
EXPORT_SYMBOL(g_selected_mac_id);
static int __init select_mac_id(char *str)
{
	if(strlen(str)>0) {
		sscanf(str, "%x", &g_selected_mac_id);
	}
	printk(KERN_DEBUG"%s: str=%s, g_selected_mac_id=%x\n", __func__, str, g_selected_mac_id);

	return 0;
}
__setup("macID=", select_mac_id);


int g_dock_keypad_type = 0;
EXPORT_SYMBOL(g_dock_keypad_type);
static int __init select_dock(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	if(strlen(str)>0) {
		if(str[0]=='y' ||str[0]=='Y')
			g_dock_keypad_type = 1;
	}

	return 0;
}
__setup("dock=", select_dock);


int g_amplifier_type = 0;
EXPORT_SYMBOL(g_amplifier_type);
static int __init select_amplifier(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	if(strlen(str)>0) {
		if(str[2]=='4' && str[3]=='5')
			g_amplifier_type = 1;
	}

	return 0;
}
__setup("amp=", select_amplifier);


char g_selected_motor[32] = {'\0'};
EXPORT_SYMBOL(g_selected_motor);
static int __init select_motor(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	strcpy(g_selected_motor, str);
	return 0;
}
__setup("motor=", select_motor);


int g_motor_value = 100;
EXPORT_SYMBOL(g_motor_value);
static int __init select_motor_value(char *str)
{
	if(strlen(str)>0) {
		g_motor_value = 0;
		sscanf(str, "%d", &g_motor_value);
	}
	printk("%s: g_motor_value=%d\n", __func__, g_motor_value);
	return 0;
}
__setup("motor_value=", select_motor_value);


char g_device_serial[32] = {'\0'};
static int __init select_serial(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	strcpy(g_device_serial, str);

	return 0;
}
__setup("serialno=", select_serial);

char g_device_macW[32] = {'\0'};
static int __init select_mac_wifi(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	strcpy(g_device_macW, str);

	return 0;
}
__setup("macw=", select_mac_wifi);

char g_device_macB[32] = {'\0'};
static int __init select_mac_bt(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	strcpy(g_device_macB, str);

	return 0;
}
__setup("macb=", select_mac_bt);

char g_device_gauge[32] = {'\0'};
static int __init select_gauge(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	strcpy(g_device_gauge, str);

	return 0;
}
__setup("gae=", select_gauge);



/*======================================*/
//add by jerry
int g_wakeup_flag = 1;
EXPORT_SYMBOL(g_wakeup_flag);
//add end

bool is_mtk_combo(void)
{
	if(strncmp(g_selected_wifi, "mt6x", strlen("mt6x"))==0) {
		return true;
	}
	return false;
}
EXPORT_SYMBOL(is_mtk_combo);


bool is_bcm_combo(void)
{
	if(strncmp(g_selected_wifi, "gb86", strlen("gb86"))==0
		|| strncmp(g_selected_wifi, "ap6", strlen("ap6"))==0
	) {
		return true;
	}
	return false;
}
EXPORT_SYMBOL(is_bcm_combo);


int g_system_flag = 0;
EXPORT_SYMBOL(g_system_flag);


int update_status_flag = 0;
EXPORT_SYMBOL_GPL(update_status_flag);

int TP_I2C_ADDR = 0;
EXPORT_SYMBOL_GPL(TP_I2C_ADDR);
/******************************  add by urbettr for OEM end *****************************************/
extern char s_selected_lcd_name[32];



//#define REG_INFORM4            (S5P_INFORM4)

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define SMDK4X12_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define SMDK4X12_ULCON_DEFAULT	S3C2410_LCON_CS8

#define SMDK4X12_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)
extern void bcm_bt_lpm_exit_lpm_locked(struct uart_port *uport);

static struct s3c2410_uartcfg smdk4x12_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon		= SMDK4X12_UCON_DEFAULT,
		.ulcon		= SMDK4X12_ULCON_DEFAULT,
		.ufcon		= SMDK4X12_UFCON_DEFAULT,
		.wake_peer	= bcm_bt_lpm_exit_lpm_locked,
	},
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= SMDK4X12_UCON_DEFAULT,
		.ulcon		= SMDK4X12_ULCON_DEFAULT,
		.ufcon		= SMDK4X12_UFCON_DEFAULT,
	},
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon		= SMDK4X12_UCON_DEFAULT,
		.ulcon		= SMDK4X12_ULCON_DEFAULT,
		.ufcon		= SMDK4X12_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= SMDK4X12_UCON_DEFAULT,
		.ulcon		= SMDK4X12_ULCON_DEFAULT,
		.ufcon		= SMDK4X12_UFCON_DEFAULT,
	},
};

#ifdef CONFIG_EXYNOS_MEDIA_DEVICE
struct platform_device exynos_device_md0 = {
	.name = "exynos-mdev",
	.id = -1,
};
#endif
static struct i2c_gpio_platform_data gpio_i2c3_data = {	
	.sda_pin            = EXYNOS4_GPA1(2), // f0_0	
	.scl_pin            = EXYNOS4_GPA1(3),  // f0_1		
	.udelay             = 2,	
	.sda_is_open_drain  = 0,  // 0  // must mod	
	.scl_is_open_drain  = 0,  //  0  	
	.scl_is_output_only = 0,
};

struct platform_device gpio_i2c3_dev = {	
	.name               = "i2c-gpio",	
		.id                 = 3,	
		.dev.platform_data  = &gpio_i2c3_data,
};

//add by jerry
#ifdef CONFIG_VIDEO_UTCAMERA
static struct ut2055_platform_data ut2055_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_YUYV,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info ut2055_i2c_info = {
	I2C_BOARD_INFO("UT2055", 0x24),
	.platform_data = &ut2055_plat,
};


static struct s3c_platform_camera ut2055 = {
#if 0
#ifdef CONFIG_ITU_A
	.id		= CAMERA_PAR_A,
	.clk_name	= "sclk_cam0",
	.i2c_busnum	= 7,
	.cam_power	= smdk4x12_cam0_reset,
#endif
#ifdef CONFIG_ITU_B
	.id		= CAMERA_PAR_B,
	.clk_name	= "sclk_cam1",
	.i2c_busnum	= 7,
	.cam_power	= smdk4x12_cam1_reset,
#endif
#endif
#if 1

	.id		= CAMERA_PAR_A,
	.clk_name	= "sclk_cam0",
	.i2c_busnum	= 7,
//	.cam_power	= smdk4x12_cam0_reset,
#endif
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.info		= &ut2055_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_YUYV,
	.srclk_name	= "xusbxti",
	.clk_rate	= 24000000,
	.line_length	= 1920,
	.width		= 1600,
	.height		= 1200,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 1600,
		.height	= 1200,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,
	.reset_camera	= 1,
	.initialized	= 0,
};
#endif
//add end

#define WRITEBACK_ENABLED

#if defined(CONFIG_VIDEO_FIMC) || defined(CONFIG_VIDEO_SAMSUNG_S5P_FIMC)
/*
 * External camera reset
 * Because the most of cameras take i2c bus signal, so that
 * you have to reset at the boot time for other i2c slave devices.
 * This function also called at fimc_init_camera()
 * Do optimization for cameras on your platform.
*/
#if defined(CONFIG_ITU_A) || defined(CONFIG_CSI_C) \
	|| defined(CONFIG_S5K3H2_CSI_C) || defined(CONFIG_S5K3H7_CSI_C) \
	|| defined(CONFIG_S5K4E5_CSI_C) || defined(CONFIG_S5K6A3_CSI_C)
static int smdk4x12_cam0_reset(int dummy)
{
	int err;
	/* Camera A */
	err = gpio_request(EXYNOS4_GPX1(2), "GPX1");
	if (err)
		printk(KERN_ERR "#### failed to request GPX1_2 ####\n");

	s3c_gpio_setpull(EXYNOS4_GPX1(2), S3C_GPIO_PULL_NONE);
	gpio_direction_output(EXYNOS4_GPX1(2), 0);
	gpio_direction_output(EXYNOS4_GPX1(2), 1);
	gpio_free(EXYNOS4_GPX1(2));

	return 0;
}
#endif
#if defined(CONFIG_ITU_B) || defined(CONFIG_CSI_D) \
	|| defined(CONFIG_S5K3H2_CSI_D) || defined(CONFIG_S5K3H7_CSI_D) \
	|| defined(CONFIG_S5K4E5_CSI_D) || defined(CONFIG_S5K6A3_CSI_D)
static int smdk4x12_cam1_reset(int dummy)
{
	int err;

	/* Camera B */
	err = gpio_request(EXYNOS4_GPX1(0), "GPX1");
	if (err)
		printk(KERN_ERR "#### failed to request GPX1_0 ####\n");

	s3c_gpio_setpull(EXYNOS4_GPX1(0), S3C_GPIO_PULL_NONE);
	gpio_direction_output(EXYNOS4_GPX1(0), 0);
	gpio_direction_output(EXYNOS4_GPX1(0), 1);
	gpio_free(EXYNOS4_GPX1(0));

	return 0;
}
#endif
#endif

#ifdef CONFIG_VIDEO_FIMC


#ifdef CONFIG_VIDEO_OV5645  

static struct ut2055_platform_data ov5645_plat = {
	.default_width = 2592,
	.default_height = 1944,
	.pixelformat = V4L2_PIX_FMT_YUYV,
	.freq = 24000000,
	.is_mipi = 1,
};

static struct i2c_board_info ov5645_i2c_info = {
	I2C_BOARD_INFO("OV5645", 0x3c),
	.platform_data = &ov5645_plat,
};

static struct s3c_platform_camera ov5645 = {

	.id		= CAMERA_CSI_C,
	.clk_name	= "sclk_cam0",
	.i2c_busnum	= 7,
	.type		= CAM_TYPE_MIPI,
	.fmt		= MIPI_CSI_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_YCBYCR,
	.info		= &ov5645_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_YUYV,
	.srclk_name	= "xusbxti",
	.clk_rate	= 24000000,
	.line_length	= 2800,
	.width		= 2592,
	.height		= 1944,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 2592,
		.height	= 1944,
	},

	.mipi_lanes	= 2,
	.mipi_settle = 12,
	.mipi_align	= 32,

	/* Polarity */
	.inv_pclk	= 1,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,
	.reset_camera	= 1,
	.initialized	= 0,
};

#endif

#ifdef CONFIG_VIDEO_HM5065
static struct ut2055_platform_data hm5065_plat = {
	.default_width = 2592,
	.default_height = 1944,
	.pixelformat = V4L2_PIX_FMT_YUYV,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info hm5065_i2c_info = {
	I2C_BOARD_INFO("HM5065",  (0x1f)),  // hm5065
	.platform_data = &hm5065_plat,
};

static struct s3c_platform_camera hm5065 = {
	.id		= CAMERA_PAR_A,
	.clk_name	= "sclk_cam0",
	.i2c_busnum	= 7,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_656_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_YCBYCR,
	.info		= &hm5065_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_YUYV,
	.srclk_name	= "xusbxti",
	.clk_rate	= 24000000,
	.line_length	= 2800,
	.width		= 2592,
	.height		= 1944,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 2592,
		.height	= 1944,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,
	.reset_camera	= 1,
	.initialized	= 0,
};
#endif

#ifdef CONFIG_VIDEO_BF3A20  

static struct ut2055_platform_data bf3a20_plat = {
	.default_width = 1600,
	.default_height = 1200,
	.pixelformat = V4L2_PIX_FMT_YUYV,
	.freq = 24000000,
	.is_mipi = 1,
};

static struct i2c_board_info bf3a20_i2c_info = {
	I2C_BOARD_INFO("BF3A20", 0x6F),
	.platform_data = &bf3a20_plat,
};

static struct s3c_platform_camera bf3a20 = {

	.id		= CAMERA_CSI_D,
	.clk_name	= "sclk_cam1",
//	.id		= CAMERA_CSI_C,
//	.clk_name	= "sclk_cam0",
	.i2c_busnum	= 7,
	.type		= CAM_TYPE_MIPI,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.info		= &bf3a20_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_YUYV,
	.srclk_name	= "xusbxti",
	.clk_rate	= 24000000,
	.line_length	= 1920,
	.width		= 1600,
	.height		= 1200,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 1600,
		.height	= 1200,
	},

	.mipi_lanes	= 2,
	.mipi_settle = 12,
	.mipi_align	= 32,

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,
	.reset_camera	= 1,
	.initialized	= 0,
};

#endif

#ifdef CONFIG_VIDEO_GC2035  

static struct ut2055_platform_data gc2035_plat = {
	.default_width = 1600,
	.default_height = 1200,
	.pixelformat = V4L2_PIX_FMT_YUYV,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info gc2035_i2c_info = {
	I2C_BOARD_INFO("GC2035", 0x3c),
	.platform_data = &gc2035_plat,
};

static struct s3c_platform_camera gc2035 = {
	.id		= CAMERA_PAR_A,
	.clk_name	= "sclk_cam0",
	.i2c_busnum	= 7,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_656_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_YCBYCR,
	.info		= &gc2035_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_YUYV,
	.srclk_name	= "xusbxti",
	.clk_rate	= 24000000,
	.line_length	= 1920,
	.width		= 1600,
	.height		= 1200,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 1600,
		.height	= 1200,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,
	.reset_camera	= 1,
	.initialized	= 0,
};

#endif

#ifdef CONFIG_VIDEO_S5K4ECGX
static struct s5k4ea_platform_data s5k4ec_plat = {
	.default_width = 2592,
	.default_height = 1944,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 1,
};

static struct i2c_board_info s5k4ec_i2c_info = {
	I2C_BOARD_INFO("S5K4EC", 0x2d),
	.platform_data = &s5k4ec_plat,
};

static struct s3c_platform_camera s5k4ec = {

	.id		= CAMERA_CSI_C,
	.clk_name	= "sclk_cam0",
	.i2c_busnum	= 7,
	//.cam_power	= smdk4x12_cam0_reset,

	.type		= CAM_TYPE_MIPI,
	.fmt		= MIPI_CSI_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_YCBYCR,
	.info		= &s5k4ec_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti",
	.clk_rate	= 24000000,
	.line_length	= 2560,
	.width		= 2560,
	.height		= 1920,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 2560,
		.height	= 1920,
	},

	.mipi_lanes	= 2,
	.mipi_settle	= 12,
	.mipi_align	= 34,

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized	= 0,
};
#endif



#ifdef CONFIG_VIDEO_S5K4BA
static struct s5k4ba_platform_data s5k4ba_plat = {
	.default_width = 800,
	.default_height = 600,
	.pixelformat = V4L2_PIX_FMT_YUYV,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info s5k4ba_i2c_info = {
	I2C_BOARD_INFO("S5K4BA", 0x2d),
	.platform_data = &s5k4ba_plat,
};

static struct s3c_platform_camera s5k4ba = {
#ifdef CONFIG_ITU_A
	.id		= CAMERA_PAR_A,
	.clk_name	= "sclk_cam0",
	.i2c_busnum	= 4,
	.cam_power	= smdk4x12_cam0_reset,
#endif
#ifdef CONFIG_ITU_B
	.id		= CAMERA_PAR_B,
	.clk_name	= "sclk_cam1",
	.i2c_busnum	= 5,
	.cam_power	= smdk4x12_cam1_reset,
#endif
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.info		= &s5k4ba_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_YUYV,
	.srclk_name	= "xusbxti",
	.clk_rate	= 24000000,
	.line_length	= 1920,
	.width		= 1600,
	.height		= 1200,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 1600,
		.height	= 1200,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,
	.reset_camera	= 1,
	.initialized	= 0,
};
#endif

/* 2 MIPI Cameras */
#ifdef CONFIG_VIDEO_S5K4EA
static struct s5k4ea_platform_data s5k4ea_plat = {
	.default_width = 1920,
	.default_height = 1080,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 1,
};

static struct i2c_board_info s5k4ea_i2c_info = {
	I2C_BOARD_INFO("S5K4EA", 0x2d),
	.platform_data = &s5k4ea_plat,
};

static struct s3c_platform_camera s5k4ea = {
#ifdef CONFIG_CSI_C
	.id		= CAMERA_CSI_C,
	.clk_name	= "sclk_cam0",
	.i2c_busnum	= 4,
	.cam_power	= smdk4x12_cam0_reset,
#endif
#ifdef CONFIG_CSI_D
	.id		= CAMERA_CSI_D,
	.clk_name	= "sclk_cam1",
	.i2c_busnum	= 5,
	.cam_power	= smdk4x12_cam1_reset,
#endif
	.type		= CAM_TYPE_MIPI,
	.fmt		= MIPI_CSI_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_YCBYCR,
	.info		= &s5k4ea_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti",
	.clk_rate	= 24000000,
	.line_length	= 1920,
	.width		= 1920,
	.height		= 1080,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 1920,
		.height	= 1080,
	},

	.mipi_lanes	= 2,
	.mipi_settle	= 12,
	.mipi_align	= 32,

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized	= 0,
};
#endif

#ifdef WRITEBACK_ENABLED
static struct i2c_board_info writeback_i2c_info = {
	I2C_BOARD_INFO("WriteBack", 0x0),
};

static struct s3c_platform_camera writeback = {
	.id		= CAMERA_WB,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum	= 0,
	.info		= &writeback_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_YUV444,
	.line_length	= 800,
	.width		= 480,
	.height		= 800,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 480,
		.height	= 800,
	},

	.initialized	= 0,
};
#endif

#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
#ifdef CONFIG_VIDEO_S5K3H2
static struct i2c_board_info s5k3h2_sensor_info = {
	.type = "S5K3H2",
};

static struct s3c_platform_camera s5k3h2 = {
#ifdef CONFIG_S5K3H2_CSI_C
	.id		= CAMERA_CSI_C,
	.clk_name	= "sclk_cam0",
	.cam_power	= smdk4x12_cam0_reset,
#endif
#ifdef CONFIG_S5K3H2_CSI_D
	.id		= CAMERA_CSI_D,
	.clk_name	= "sclk_cam1",
	.cam_power	= smdk4x12_cam1_reset,
#endif
	.type		= CAM_TYPE_MIPI,
	.fmt		= MIPI_CSI_RAW10,
	.info		= &s5k3h2_sensor_info,
	.order422	= CAM_ORDER422_8BIT_YCBYCR,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.line_length	= 1920,
	.width		= 1920,
	.height		= 1080,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 1920,
		.height	= 1080,
	},
	.srclk_name	= "xusbxti",
	.clk_rate	= 24000000,
	.mipi_lanes	= 2,
	.mipi_settle	= 12,
	.mipi_align	= 24,

	.initialized	= 0,
#ifdef CONFIG_S5K3H2_CSI_C
	.flite_id	= FLITE_IDX_A,
#endif
#ifdef CONFIG_S5K3H2_CSI_D
	.flite_id	= FLITE_IDX_B,
#endif
	.use_isp	= true,
#ifdef CONFIG_S5K3H2_CSI_C
	.sensor_index	= 1,
#endif
#ifdef CONFIG_S5K3H2_CSI_D
	.sensor_index	= 101,
#endif
};
#endif

#ifdef CONFIG_VIDEO_S5K3H7
static struct i2c_board_info s5k3h7_sensor_info = {
	.type = "S5K3H7",
};

static struct s3c_platform_camera s5k3h7 = {
#ifdef CONFIG_S5K3H7_CSI_C
	.id		= CAMERA_CSI_C,
	.clk_name	= "sclk_cam0",
	.cam_power	= smdk4x12_cam0_reset,
#endif
#ifdef CONFIG_S5K3H7_CSI_D
	.id		= CAMERA_CSI_D,
	.clk_name	= "sclk_cam1",
	.cam_power	= smdk4x12_cam1_reset,
#endif
	.type		= CAM_TYPE_MIPI,
	.fmt		= MIPI_CSI_RAW10,
	.info		= &s5k3h7_sensor_info,
	.order422	= CAM_ORDER422_8BIT_YCBYCR,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.line_length	= 1920,
	.width		= 1920,
	.height		= 1080,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 1920,
		.height	= 1080,
	},
	.srclk_name	= "xusbxti",
	.clk_rate	= 24000000,
	.mipi_lanes	= 2,
	.mipi_settle	= 12,
	.mipi_align	= 24,

	.initialized	= 0,
#ifdef CONFIG_S5K3H7_CSI_C
	.flite_id	= FLITE_IDX_A,
#endif
#ifdef CONFIG_S5K3H7_CSI_D
	.flite_id	= FLITE_IDX_B,
#endif
	.use_isp	= true,
#ifdef CONFIG_S5K3H7_CSI_C
	.sensor_index	= 4,
#endif
#ifdef CONFIG_S5K3H7_CSI_D
	.sensor_index	= 104,
#endif
};
#endif

#ifdef CONFIG_VIDEO_S5K4E5
static struct i2c_board_info s5k4e5_sensor_info = {
	.type = "S5K4E5",
};

static struct s3c_platform_camera s5k4e5 = {
#ifdef CONFIG_S5K4E5_CSI_C
	.id		= CAMERA_CSI_C,
	.clk_name	= "sclk_cam0",
	.cam_power	= smdk4x12_cam0_reset,
#endif
#ifdef CONFIG_S5K4E5_CSI_D
	.id		= CAMERA_CSI_D,
	.clk_name	= "sclk_cam1",
	.cam_power	= smdk4x12_cam1_reset,
#endif
	.type		= CAM_TYPE_MIPI,
	.fmt		= MIPI_CSI_RAW10,
	.info		= &s5k4e5_sensor_info,
	.order422	= CAM_ORDER422_8BIT_YCBYCR,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.line_length	= 1920,
	.width		= 1920,
	.height		= 1080,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 1920,
		.height	= 1080,
	},
	.srclk_name	= "xusbxti",
	.clk_rate	= 24000000,
	.mipi_lanes	= 2,
	.mipi_settle	= 12,
	.mipi_align	= 24,

	.initialized	= 0,
#ifdef CONFIG_S5K4E5_CSI_C
	.flite_id	= FLITE_IDX_A,
#endif
#ifdef CONFIG_S5K4E5_CSI_D
	.flite_id	= FLITE_IDX_B,
#endif
	.use_isp	= true,
#ifdef CONFIG_S5K4E5_CSI_C
	.sensor_index	= 3,
#endif
#ifdef CONFIG_S5K4E5_CSI_D
	.sensor_index	= 103,
#endif
};
#endif


#ifdef CONFIG_VIDEO_S5K6A3
static struct i2c_board_info s5k6a3_sensor_info = {
	.type = "S5K6A3",
};

static struct s3c_platform_camera s5k6a3 = {
#ifdef CONFIG_S5K6A3_CSI_C
	.id		= CAMERA_CSI_C,
	.clk_name	= "sclk_cam0",
	.cam_power	= smdk4x12_cam0_reset,
#endif
#ifdef CONFIG_S5K6A3_CSI_D
	.id		= CAMERA_CSI_D,
	.clk_name	= "sclk_cam1",
	.cam_power	= smdk4x12_cam1_reset,
#endif
	.type		= CAM_TYPE_MIPI,
	.fmt		= MIPI_CSI_RAW10,
	.info		= &s5k6a3_sensor_info,
	.order422	= CAM_ORDER422_8BIT_YCBYCR,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.line_length	= 1920,
	.width		= 1920,
	.height		= 1080,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 1920,
		.height	= 1080,
	},
	.srclk_name	= "xusbxti",
	.clk_rate	= 24000000,
	.mipi_lanes	= 1,
	.mipi_settle	= 18,
	.mipi_align	= 24,

	.initialized	= 0,
#ifdef CONFIG_S5K6A3_CSI_C
	.flite_id	= FLITE_IDX_A,
#endif
#ifdef CONFIG_S5K6A3_CSI_D
	.flite_id	= FLITE_IDX_B,
#endif
	.use_isp	= true,
#ifdef CONFIG_S5K6A3_CSI_C
	.sensor_index	= 2,
#endif
#ifdef CONFIG_S5K6A3_CSI_D
	.sensor_index	= 102,
#endif
};
#endif

#if defined(CONFIG_VIDEO_S5K6A3) && defined(CONFIG_S5K6A3_CSI_D)
static struct i2c_board_info s5k6a3_fd_sensor_info = {
	.type = "S5K6A3_FD",
};

static struct s3c_platform_camera s5k6a3_fd = {
	.id		= CAMERA_CSI_D,
	.clk_name	= "sclk_cam1",
	.cam_power	= smdk4x12_cam1_reset,

	.type		= CAM_TYPE_MIPI,
	.fmt		= MIPI_CSI_RAW10,
	.info		= &s5k6a3_fd_sensor_info,
	.order422	= CAM_ORDER422_8BIT_YCBYCR,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.line_length	= 1920,
	.width		= 1920,
	.height		= 1080,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 1920,
		.height	= 1080,
	},
	.srclk_name	= "xusbxti",
	.clk_rate	= 24000000,
	.mipi_lanes	= 1,
	.mipi_settle	= 18,
	.mipi_align	= 24,

	.initialized	= 0,
	.flite_id	= FLITE_IDX_B,
	.use_isp	= true,
	.sensor_index	= 200
};
#endif

#endif

/* legacy M5MOLS Camera driver configuration */
#ifdef CONFIG_VIDEO_M5MO
#define CAM_CHECK_ERR_RET(x, msg)	\
	if (unlikely((x) < 0)) { \
		printk(KERN_ERR "\nfail to %s: err = %d\n", msg, x); \
		return x; \
	}
#define CAM_CHECK_ERR(x, msg)	\
		if (unlikely((x) < 0)) { \
			printk(KERN_ERR "\nfail to %s: err = %d\n", msg, x); \
		}

static int m5mo_config_isp_irq(void)
{
	s3c_gpio_cfgpin(EXYNOS4_GPX3(3), S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(EXYNOS4_GPX3(3), S3C_GPIO_PULL_NONE);
	return 0;
}

static struct m5mo_platform_data m5mo_plat = {
	.default_width = 640, /* 1920 */
	.default_height = 480, /* 1080 */
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 1,
	.config_isp_irq = m5mo_config_isp_irq,
	.irq = IRQ_EINT(27),
};

static struct i2c_board_info m5mo_i2c_info = {
	I2C_BOARD_INFO("M5MO", 0x1F),
	.platform_data = &m5mo_plat,
	.irq = IRQ_EINT(27),
};

static struct s3c_platform_camera m5mo = {
#ifdef CONFIG_CSI_C
	.id		= CAMERA_CSI_C,
	.clk_name	= "sclk_cam0",
	.i2c_busnum	= 4,
	.cam_power	= smdk4x12_cam0_reset,
#endif
#ifdef CONFIG_CSI_D
	.id		= CAMERA_CSI_D,
	.clk_name	= "sclk_cam1",
	.i2c_busnum	= 5,
	.cam_power	= smdk4x12_cam1_reset,
#endif
	.type		= CAM_TYPE_MIPI,
	.fmt		= MIPI_CSI_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_YCBYCR,
	.info		= &m5mo_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti", /* "mout_mpll" */
	.clk_rate	= 24000000, /* 48000000 */
	.line_length	= 1920,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	.mipi_lanes	= 2,
	.mipi_settle	= 12,
	.mipi_align	= 32,

	/* Polarity */
	.inv_pclk	= 1,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,
	.reset_camera	= 0,
	.initialized	= 0,
};
#endif

/* Interface setting */
static struct s3c_platform_fimc fimc_plat = {
#ifdef CONFIG_ITU_A
	.default_cam	= CAMERA_PAR_A,
#endif
#ifdef CONFIG_ITU_B
	.default_cam	= CAMERA_PAR_B,
#endif
#ifdef CONFIG_CSI_C
	.default_cam	= CAMERA_CSI_C,
#endif
#ifdef CONFIG_CSI_D
	.default_cam	= CAMERA_CSI_D,
#endif
#ifdef WRITEBACK_ENABLED
	.default_cam	= CAMERA_WB,
#endif
	.camera		= {
#ifdef CONFIG_VIDEO_OV5645
		&ov5645,
#endif

#ifdef CONFIG_VIDEO_HM5065
		&hm5065,
#endif

#ifdef CONFIG_VIDEO_S5K4ECGX
		&s5k4ec,
#endif

#ifdef CONFIG_VIDEO_BF3A20
		&bf3a20,
#endif

#ifdef CONFIG_VIDEO_GC2035
		&gc2035,
#endif

#ifdef CONFIG_VIDEO_S5K4BA
		&s5k4ba,
#endif
#ifdef CONFIG_VIDEO_S5K4EA
		&s5k4ea,
#endif
#ifdef CONFIG_VIDEO_M5MO
		&m5mo,
#endif
//add by jerry
#ifdef CONFIG_VIDEO_UTCAMERA
		&ut2055,
#endif
#ifdef CONFIG_VIDEO_UTCAMERA
		&ut2055,
#endif
//add end
#ifdef CONFIG_VIDEO_S5K3H2
		&s5k3h2,
#endif
#ifdef CONFIG_VIDEO_S5K3H7
		&s5k3h7,
#endif
#ifdef CONFIG_VIDEO_S5K4E5
		&s5k4e5,
#endif
#ifdef CONFIG_VIDEO_S5K6A3
		&s5k6a3,
#endif
#ifdef WRITEBACK_ENABLED
		&writeback,
#endif
#if defined(CONFIG_VIDEO_S5K6A3) && defined(CONFIG_S5K6A3_CSI_D)
		&s5k6a3_fd,
#endif
	},
	.hw_ver		= 0x51,
};
#endif /* CONFIG_VIDEO_FIMC */

/* for mainline fimc interface */
#ifdef CONFIG_VIDEO_SAMSUNG_S5P_FIMC
#ifdef WRITEBACK_ENABLED
struct writeback_mbus_platform_data {
	int id;
	struct v4l2_mbus_framefmt fmt;
};

static struct i2c_board_info __initdata writeback_info = {
	I2C_BOARD_INFO("writeback", 0x0),
};
#endif

#ifdef CONFIG_VIDEO_S5K4BA
static struct s5k4ba_mbus_platform_data s5k4ba_mbus_plat = {
	.id		= 0,
	.fmt = {
		.width	= 1600,
		.height	= 1200,
		/*.code	= V4L2_MBUS_FMT_UYVY8_2X8, */
		.code	= V4L2_MBUS_FMT_VYUY8_2X8,
	},
	.clk_rate	= 24000000UL,
#ifdef CONFIG_ITU_A
	.set_power	= smdk4x12_cam0_reset,
#endif
#ifdef CONFIG_ITU_B
	.set_power	= smdk4x12_cam1_reset,
#endif
};

static struct i2c_board_info s5k4ba_info = {
	I2C_BOARD_INFO("S5K4BA", 0x2d),
	.platform_data = &s5k4ba_mbus_plat,
};
#endif

/* 2 MIPI Cameras */
#ifdef CONFIG_VIDEO_S5K4EA
static struct s5k4ea_mbus_platform_data s5k4ea_mbus_plat = {
#ifdef CONFIG_CSI_C
	.id		= 0,
	.set_power = smdk4x12_cam0_reset,
#endif
#ifdef CONFIG_CSI_D
	.id		= 1,
	.set_power = smdk4x12_cam1_reset,
#endif
	.fmt = {
		.width	= 1920,
		.height	= 1080,
		.code	= V4L2_MBUS_FMT_VYUY8_2X8,
	},
	.clk_rate	= 24000000UL,
};

static struct i2c_board_info s5k4ea_info = {
	I2C_BOARD_INFO("S5K4EA", 0x2d),
	.platform_data = &s5k4ea_mbus_plat,
};
#endif

#ifdef CONFIG_VIDEO_M5MOLS
static struct m5mols_platform_data m5mols_platdata = {
#ifdef CONFIG_CSI_C
	.gpio_rst = EXYNOS4_GPX1(2), /* ISP_RESET */
#endif
#ifdef CONFIG_CSI_D
	.gpio_rst = EXYNOS4_GPX1(0), /* ISP_RESET */
#endif
	.enable_rst = true, /* positive reset */
	.irq = IRQ_EINT(27),
};

static struct i2c_board_info m5mols_board_info = {
	I2C_BOARD_INFO("M5MOLS", 0x1F),
	.platform_data = &m5mols_platdata,
};

#endif

#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
#ifdef CONFIG_VIDEO_S5K3H2
static struct i2c_board_info s5k3h2_sensor_info = {
	.type = "S5K3H2",
};
#endif
#ifdef CONFIG_VIDEO_S5K3H7
static struct i2c_board_info s5k3h7_sensor_info = {
	.type = "S5K3H7",
};
#endif
#ifdef CONFIG_VIDEO_S5K4E5
static struct i2c_board_info s5k4e5_sensor_info = {
	.type = "S5K4E5",
};
#endif
#ifdef CONFIG_VIDEO_S5K6A3
static struct i2c_board_info s5k6a3_sensor_info = {
	.type = "S5K6A3",
};
#endif
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_LITE
/* This is for platdata of fimc-lite */
#ifdef CONFIG_VIDEO_S5K3H2
static struct s3c_platform_camera s5k3h2 = {
	.type  = CAM_TYPE_MIPI,
	.use_isp = true,
	.inv_pclk = 0,
	.inv_vsync = 0,
	.inv_href = 0,
	.inv_hsync = 0,
};
#endif

#ifdef CONFIG_VIDEO_S5K3H7
static struct s3c_platform_camera s5k3h7 = {
	.type  = CAM_TYPE_MIPI,
	.use_isp = true,
	.inv_pclk = 0,
	.inv_vsync = 0,
	.inv_href = 0,
	.inv_hsync = 0,
};
#endif

#ifdef CONFIG_VIDEO_S5K4E5
static struct s3c_platform_camera s5k4e5 = {
	.type  = CAM_TYPE_MIPI,
	.use_isp = true,
	.inv_pclk = 0,
	.inv_vsync = 0,
	.inv_href = 0,
	.inv_hsync = 0,
};
#endif


#ifdef CONFIG_VIDEO_S5K6A3
static struct s3c_platform_camera s5k6a3 = {
	.type  = CAM_TYPE_MIPI,
	.use_isp = true,
	.inv_pclk = 0,
	.inv_vsync = 0,
	.inv_href = 0,
	.inv_hsync = 0,
};
#endif
#endif
#endif /* CONFIG_VIDEO_SAMSUNG_S5P_FIMC */

#ifdef CONFIG_S3C64XX_DEV_SPI
static struct s3c64xx_spi_csinfo spi0_csi[] = {
	[0] = {
		.line = EXYNOS4_GPB(1),
		.set_level = gpio_set_value,
		.fb_delay = 0x2,
	},
};

static struct spi_board_info spi0_board_info[] __initdata = {
	{
		.modalias = "spidev",
		.platform_data = NULL,
		.max_speed_hz = 10*1000*1000,
		.bus_num = 0,
		.chip_select = 0,
		.mode = SPI_MODE_0,
		.controller_data = &spi0_csi[0],
	}
};

static struct s3c64xx_spi_csinfo spi1_csi[] = {
	[0] = {
		.line = EXYNOS4_GPB(5),
		.set_level = gpio_set_value,
		.fb_delay = 0x2,
	},
};

static struct spi_board_info spi1_board_info[] __initdata = {
	{
		.modalias = "spidev",
		.platform_data = NULL,
		.max_speed_hz = 10*1000*1000,
		.bus_num = 1,
		.chip_select = 0,
		.mode = SPI_MODE_3,
		.controller_data = &spi1_csi[0],
	}
};

static struct s3c64xx_spi_csinfo spi2_csi[] = {
	[0] = {
		.line = EXYNOS4_GPC1(2),
		.set_level = gpio_set_value,
		.fb_delay = 0x2,
	},
};

static struct spi_board_info spi2_board_info[] __initdata = {
	{
		.modalias = "spidev",
		.platform_data = NULL,
		.max_speed_hz = 10*1000*1000,
		.bus_num = 2,
		.chip_select = 0,
		.mode = SPI_MODE_0,
		.controller_data = &spi2_csi[0],
	}
};
#endif


#ifdef CONFIG_INV_SENSORS
// shengliang: @2012-07-23
#include <linux/mpu.h>
#endif


#ifdef CONFIG_INV_SENSORS
// shengliang: @2012-07-23

static struct mpu_platform_data mpu3050_data = {
	.int_config  = 0x10,
#if 0// shengliang, it's for TC4 screen in HZSSCR
#ifdef CONFIG_TF4_PORTRAIT_MODE
	// shengliang, need to be calibrated, @2012-08-02
	.orientation = {  0,  1,  0,
			  1,  0,  0,
			  0,  0, -1 },
#else
	// shengliang, has been calibrated, @2012-08-02
	.orientation = {  0,  1,  0,
			 -1,  0,  0,
			  0,  0,  1 },
#endif
#endif
	// yangmiansi, has been calibrated, @2013-10-12
	.orientation = {  -1,  0,  0,
			            0,  1,  0,
			            0,  0, -1 },
};

/* accel */
static struct ext_slave_platform_data inv_mpu_bma250_data = {
	.bus         = EXT_SLAVE_BUS_SECONDARY,
	// yangmiansi, has been calibrated, @2013-10-12
	.orientation = { -1, 0,   0,
			          0,  1,   0,
			          0,   0,  -1 },
};

/* compass */
static struct ext_slave_platform_data inv_mpu_hmc5883_data = {
	.bus         = EXT_SLAVE_BUS_PRIMARY,
	// yangmiansi, has been calibrated, @2013-10-12
	.orientation = {  1,  0,  0,
			          0,  1,  0,
			          0,  0,  1 },
};
#endif



#ifdef CONFIG_FB_S3C
//#if defined(CONFIG_LCD_AMS369FG06)
//static int lcd_power_on(struct lcd_device *ld, int enable)
//{
//	return 1;
//}
//
//static int reset_lcd(struct lcd_device *ld)
//{
//	int err = 0;
//
//	err = gpio_request_one(EXYNOS4_GPX0(6), GPIOF_OUT_INIT_HIGH, "GPX0");
//	if (err) {
//		printk(KERN_ERR "failed to request GPX0 for "
//				"lcd reset control\n");
//		return err;
//	}
//	gpio_set_value(EXYNOS4_GPX0(6), 0);
//	mdelay(1);
//
//	gpio_set_value(EXYNOS4_GPX0(6), 1);
//
//	gpio_free(EXYNOS4_GPX0(6));
//
//	return 1;
//}
//
//
//
//
//static struct s3c_fb_pd_win smdk4x12_fb_win0 = {
//	.win_mode = {
//		.left_margin	= 9,
//		.right_margin	= 9,
//		.upper_margin	= 5,
//		.lower_margin	= 5,
//		.hsync_len	= 2,
//		.vsync_len	= 2,
//		.xres		= 480,
//		.yres		= 800,
//	},
//	.virtual_x		= 480,
//	.virtual_y		= 1600,
//	.width			= 48,
//	.height			= 80,
//	.max_bpp		= 32,
//	.default_bpp		= 24,
//};
//
//static struct s3c_fb_pd_win smdk4x12_fb_win1 = {
//	.win_mode = {
//		.left_margin	= 9,
//		.right_margin	= 9,
//		.upper_margin	= 5,
//		.lower_margin	= 5,
//		.hsync_len	= 2,
//		.vsync_len	= 2,
//		.xres		= 480,
//		.yres		= 800,
//	},
//	.virtual_x		= 480,
//	.virtual_y		= 1600,
//	.width			= 48,
//	.height			= 80,
//	.max_bpp		= 32,
//	.default_bpp		= 24,
//};
//
//static struct s3c_fb_pd_win smdk4x12_fb_win2 = {
//	.win_mode = {
//		.left_margin	= 9,
//		.right_margin	= 9,
//		.upper_margin	= 5,
//		.lower_margin	= 5,
//		.hsync_len	= 2,
//		.vsync_len	= 2,
//		.xres		= 480,
//		.yres		= 800,
//	},
//	.virtual_x		= 480,
//	.virtual_y		= 1600,
//	.width			= 48,
//	.height			= 80,
//	.max_bpp		= 32,
//	.default_bpp		= 24,
//};
//#elif defined(CONFIG_LCD_LMS501KF03)
//static int lcd_power_on(struct lcd_device *ld, int enable)
//{
//	return 1;
//}
//
//static int reset_lcd(struct lcd_device *ld)
//{
//	int err = 0;
//
//	if (samsung_board_rev_is_0_1()) {
//		err = gpio_request_one(EXYNOS4212_GPM3(6),
//				GPIOF_OUT_INIT_HIGH, "GPM3");
//		if (err) {
//			printk(KERN_ERR "failed to request GPM3 for "
//					"lcd reset control\n");
//			return err;
//		}
//		gpio_set_value(EXYNOS4212_GPM3(6), 0);
//		mdelay(1);
//
//		gpio_set_value(EXYNOS4212_GPM3(6), 1);
//
//		gpio_free(EXYNOS4212_GPM3(6));
//	} else {
//		err = gpio_request_one(EXYNOS4_GPX1(5),
//				GPIOF_OUT_INIT_HIGH, "GPX1");
//		if (err) {
//			printk(KERN_ERR "failed to request GPX1 for "
//					"lcd reset control\n");
//			return err;
//		}
//		gpio_set_value(EXYNOS4_GPX1(5), 0);
//		mdelay(1);
//
//		gpio_set_value(EXYNOS4_GPX1(5), 1);
//
//		gpio_free(EXYNOS4_GPX1(5));
//	}
//
//	return 1;
//}
//
//static struct lcd_platform_data lms501kf03_platform_data = {
//	.reset			= reset_lcd,
//	.power_on		= lcd_power_on,
//	.lcd_enabled		= 0,
//	.reset_delay		= 100,	/* 100ms */
//};
//
//#define		LCD_BUS_NUM	3
//#define		DISPLAY_CS	EXYNOS4_GPB(5)
//#define		DISPLAY_CLK	EXYNOS4_GPB(4)
//#define		DISPLAY_SI	EXYNOS4_GPB(7)
//
//static struct spi_board_info spi_board_info[] __initdata = {
//	{
//		.modalias		= "lms501kf03",
//		.platform_data		= (void *)&lms501kf03_platform_data,
//		.max_speed_hz		= 1200000,
//		.bus_num		= LCD_BUS_NUM,
//		.chip_select		= 0,
//		.mode			= SPI_MODE_3,
//		.controller_data	= (void *)DISPLAY_CS,
//	}
//};
//
//static struct spi_gpio_platform_data lms501kf03_spi_gpio_data = {
//	.sck	= DISPLAY_CLK,
//	.mosi	= DISPLAY_SI,
//	.miso	= -1,
//	.num_chipselect = 1,
//};
//
//static struct platform_device s3c_device_spi_gpio = {
//	.name	= "spi_gpio",
//	.id	= LCD_BUS_NUM,
//	.dev	= {
//		.parent		= &s5p_device_fimd0.dev,
//		.platform_data	= &lms501kf03_spi_gpio_data,
//	},
//};
//
//static struct s3c_fb_pd_win smdk4x12_fb_win0 = {
//	.win_mode = {
//		.left_margin	= 8,		/* HBPD */
//		.right_margin	= 8,		/* HFPD */
//		.upper_margin	= 6,	/* VBPD */
//		.lower_margin	= 6,		/* VFPD */
//		.hsync_len	= 6,		/* HSPW */
//		.vsync_len	= 4,		/* VSPW */
//		.xres		= 480,
//		.yres		= 800,
//	},
//	.virtual_x		= 480,
//	.virtual_y		= 1600,
//	.width			= 48,
//	.height			= 80,
//	.max_bpp		= 32,
//	.default_bpp		= 24,
//};
//
//static struct s3c_fb_pd_win smdk4x12_fb_win1 = {
//	.win_mode = {
//		.left_margin	= 8,		/* HBPD */
//		.right_margin	= 8,		/* HFPD */
//		.upper_margin	= 6,	/* VBPD */
//		.lower_margin	= 6,		/* VFPD */
//		.hsync_len	= 6,		/* HSPW */
//		.vsync_len	= 4,		/* VSPW */
//		.xres		= 480,
//		.yres		= 800,
//	},
//	.virtual_x		= 480,
//	.virtual_y		= 1600,
//	.width			= 48,
//	.height			= 80,
//	.max_bpp		= 32,
//	.default_bpp		= 24,
//};
//
//static struct s3c_fb_pd_win smdk4x12_fb_win2 = {
//	.win_mode = {
//		.left_margin	= 8,		/* HBPD */
//		.right_margin	= 8,		/* HFPD */
//		.upper_margin	= 6,	/* VBPD */
//		.lower_margin	= 6,		/* VFPD */
//		.hsync_len	= 6,		/* HSPW */
//		.vsync_len	= 4,		/* VSPW */
//		.xres		= 480,
//		.yres		= 800,
//	},
//	.virtual_x		= 480,
//	.virtual_y		= 1600,
//	.width			= 48,
//	.height			= 80,
//	.max_bpp		= 32,
//	.default_bpp		= 24,
//};
//#elif defined(CONFIG_LCD_WA101S)
//static void lcd_wa101s_set_power(struct plat_lcd_data *pd,
//				   unsigned int power)
//{
//	if (power) {
//#if !defined(CONFIG_BACKLIGHT_PWM)
//		gpio_request_one(EXYNOS4_GPD0(1), GPIOF_OUT_INIT_HIGH, "GPD0");
//		gpio_free(EXYNOS4_GPD0(1));
//#endif
//	} else {
//#if !defined(CONFIG_BACKLIGHT_PWM)
//		gpio_request_one(EXYNOS4_GPD0(1), GPIOF_OUT_INIT_LOW, "GPD0");
//		gpio_free(EXYNOS4_GPD0(1));
//#endif
//	}
//}
//
//static struct plat_lcd_data smdk4x12_lcd_wa101s_data = {
//	.set_power		= lcd_wa101s_set_power,
//};
//
//static struct platform_device smdk4x12_lcd_wa101s = {
//	.name			= "platform-lcd",
//	.dev.parent		= &s5p_device_fimd0.dev,
//	.dev.platform_data      = &smdk4x12_lcd_wa101s_data,
//};
//
//static struct s3c_fb_pd_win smdk4x12_fb_win0 = {
//	.win_mode = {
//		.left_margin	= 80,
//		.right_margin	= 48,
//		.upper_margin	= 14,
//		.lower_margin	= 3,
//		.hsync_len	= 32,
//		.vsync_len	= 5,
//		.xres		= 1360, /* real size : 1366 */
//		.yres		= 768,
//	},
//	.virtual_x		= 1360, /* real size : 1366 */
//	.virtual_y		= 768 * 2,
//	.width			= 223,
//	.height			= 125,
//	.max_bpp		= 32,
//	.default_bpp		= 24,
//};
//
//static struct s3c_fb_pd_win smdk4x12_fb_win1 = {
//	.win_mode = {
//		.left_margin	= 80,
//		.right_margin	= 48,
//		.upper_margin	= 14,
//		.lower_margin	= 3,
//		.hsync_len	= 32,
//		.vsync_len	= 5,
//		.xres		= 1360, /* real size : 1366 */
//		.yres		= 768,
//	},
//	.virtual_x		= 1360, /* real size : 1366 */
//	.virtual_y		= 768 * 2,
//	.width			= 223,
//	.height			= 125,
//	.max_bpp		= 32,
//	.default_bpp		= 24,
//};
//
//static struct s3c_fb_pd_win smdk4x12_fb_win2 = {
//	.win_mode = {
//		.left_margin	= 80,
//		.right_margin	= 48,
//		.upper_margin	= 14,
//		.lower_margin	= 3,
//		.hsync_len	= 32,
//		.vsync_len	= 5,
//		.xres		= 1360, /* real size : 1366 */
//		.yres		= 768,
//	},
//	.virtual_x		= 1360, /* real size : 1366 */
//	.virtual_y		= 768 * 2,
//	.width			= 223,
//	.height			= 125,
//	.max_bpp		= 32,
//	.default_bpp		= 24,
//};
//
//#elif defined(CONFIG_LCD_LTE480WV)
//static void lcd_lte480wv_set_power(struct plat_lcd_data *pd,
//				   unsigned int power)
//{
//	if (power) {
//#if !defined(CONFIG_BACKLIGHT_PWM)
//		gpio_request_one(EXYNOS4_GPD0(1), GPIOF_OUT_INIT_HIGH, "GPD0");
//		gpio_free(EXYNOS4_GPD0(1));
//#endif
//		/* fire nRESET on power up */
//		gpio_request_one(EXYNOS4_GPX0(6), GPIOF_OUT_INIT_HIGH, "GPX0");
//		mdelay(100);
//
//		gpio_set_value(EXYNOS4_GPX0(6), 0);
//		mdelay(10);
//
//		gpio_set_value(EXYNOS4_GPX0(6), 1);
//		mdelay(10);
//
//		gpio_free(EXYNOS4_GPX0(6));
//	} else {
//#if !defined(CONFIG_BACKLIGHT_PWM)
//		gpio_request_one(EXYNOS4_GPD0(1), GPIOF_OUT_INIT_LOW, "GPD0");
//		gpio_free(EXYNOS4_GPD0(1));
//#endif
//	}
//}
//
//static struct plat_lcd_data smdk4x12_lcd_lte480wv_data = {
//	.set_power		= lcd_lte480wv_set_power,
//};
//
//static struct platform_device smdk4x12_lcd_lte480wv = {
//	.name			= "platform-lcd",
//	.dev.parent		= &s5p_device_fimd0.dev,
//	.dev.platform_data      = &smdk4x12_lcd_lte480wv_data,
//};
//
//static struct s3c_fb_pd_win smdk4x12_fb_win0 = {
//	.win_mode = {
//		.left_margin	= 13,
//		.right_margin	= 8,
//		.upper_margin	= 7,
//		.lower_margin	= 5,
//		.hsync_len	= 3,
//		.vsync_len	= 1,
//		.xres		= 800,
//		.yres		= 480,
//	},
//	.virtual_x		= 800,
//	.virtual_y		= 960,
//	.width			= 104,
//	.height			= 62,
//	.max_bpp		= 32,
//	.default_bpp		= 24,
//};
//
//static struct s3c_fb_pd_win smdk4x12_fb_win1 = {
//	.win_mode = {
//		.left_margin	= 13,
//		.right_margin	= 8,
//		.upper_margin	= 7,
//		.lower_margin	= 5,
//		.hsync_len	= 3,
//		.vsync_len	= 1,
//		.xres		= 800,
//		.yres		= 480,
//	},
//	.virtual_x		= 800,
//	.virtual_y		= 960,
//	.width			= 104,
//	.height			= 62,
//	.max_bpp		= 32,
//	.default_bpp		= 24,
//};
//
//static struct s3c_fb_pd_win smdk4x12_fb_win2 = {
//	.win_mode = {
//		.left_margin	= 13,
//		.right_margin	= 8,
//		.upper_margin	= 7,
//		.lower_margin	= 5,
//		.hsync_len	= 3,
//		.vsync_len	= 1,
//		.xres		= 800,
//		.yres		= 480,
//	},
//	.virtual_x		= 800,
//	.virtual_y		= 960,
//	.width			= 104,
//	.height			= 62,
//	.max_bpp		= 32,
//	.default_bpp		= 24,
//};
//#elif defined(CONFIG_LCD_MIPI_S6E63M0)
//static void mipi_lcd_set_power(struct plat_lcd_data *pd,
//				unsigned int power)
//{
//	gpio_request_one(EXYNOS4_GPX2(7), GPIOF_OUT_INIT_HIGH, "GPX2");
//
//	mdelay(100);
//	if (power) {
//		/* fire nRESET on power up */
//		gpio_set_value(EXYNOS4_GPX2(7), 0);
//		mdelay(100);
//		gpio_set_value(EXYNOS4_GPX2(7), 1);
//		mdelay(100);
//		gpio_free(EXYNOS4_GPX2(7));
//	} else {
//		/* fire nRESET on power off */
//		gpio_set_value(EXYNOS4_GPX2(7), 0);
//		mdelay(100);
//		gpio_set_value(EXYNOS4_GPX2(7), 1);
//		mdelay(100);
//		gpio_free(EXYNOS4_GPX2(7));
//	}
//}
//
//static struct plat_lcd_data smdk4x12_mipi_lcd_data = {
//	.set_power	= mipi_lcd_set_power,
//};
//
//static struct platform_device smdk4x12_mipi_lcd = {
//	.name			= "platform-lcd",
//	.dev.parent		= &s5p_device_fimd0.dev,
//	.dev.platform_data	= &smdk4x12_mipi_lcd_data,
//};
//
//static struct s3c_fb_pd_win smdk4x12_fb_win0 = {
//	.win_mode = {
//		.left_margin	= 0x16,
//		.right_margin	= 0x16,
//		.upper_margin	= 0x1,
//		.lower_margin	= 0x28,
//		.hsync_len	= 0x2,
//		.vsync_len	= 0x3,
//		.xres		= 480,
//		.yres		= 800,
//	},
//	.virtual_x		= 480,
//	.virtual_y		= 1600,
//	.width			= 48,
//	.height			= 80,
//	.max_bpp		= 32,
//	.default_bpp		= 24,
//};
//
//static struct s3c_fb_pd_win smdk4x12_fb_win1 = {
//	.win_mode = {
//		.left_margin	= 0x16,
//		.right_margin	= 0x16,
//		.upper_margin	= 0x1,
//		.lower_margin	= 0x28,
//		.hsync_len	= 0x2,
//		.vsync_len	= 0x3,
//		.xres		= 480,
//		.yres		= 800,
//	},
//	.virtual_x		= 480,
//	.virtual_y		= 1600,
//	.width			= 48,
//	.height			= 80,
//	.max_bpp		= 32,
//	.default_bpp		= 24,
//};
//
//static struct s3c_fb_pd_win smdk4x12_fb_win2 = {
//	.win_mode = {
//		.left_margin	= 0x16,
//		.right_margin	= 0x16,
//		.upper_margin	= 0x1,
//		.lower_margin	= 0x28,
//		.hsync_len	= 0x2,
//		.vsync_len	= 0x3,
//		.xres		= 480,
//		.yres		= 800,
//	},
//	.virtual_x		= 480,
//	.virtual_y		= 1600,
//	.width			= 48,
//	.height			= 80,
//	.max_bpp		= 32,
//	.default_bpp		= 24,
//};
//#endif

static struct s3c_fb_platdata smdk4x12_lcd0_pdata __initdata = {
#if defined(CONFIG_LCD_AMS369FG06) || defined(CONFIG_LCD_WA101S) || \
	defined(CONFIG_LCD_LTE480WV) || defined(CONFIG_LCD_LMS501KF03) || \
	defined(CONFIG_LCD_MIPI_S6E63M0)
	.win[0]		= &smdk4x12_fb_win0,
	.win[1]		= &smdk4x12_fb_win1,
	.win[2]		= &smdk4x12_fb_win2,
#endif
	.default_win	= 2,
	.vidcon0	= VIDCON0_VIDOUT_RGB | VIDCON0_PNRMODE_RGB,
#if defined(CONFIG_LCD_AMS369FG06)
	.vidcon1	= VIDCON1_INV_VCLK | VIDCON1_INV_VDEN |
			  VIDCON1_INV_HSYNC | VIDCON1_INV_VSYNC,
#elif defined(CONFIG_LCD_LMS501KF03)
	.vidcon1	= VIDCON1_INV_HSYNC | VIDCON1_INV_VSYNC,
#elif defined(CONFIG_LCD_WA101S)
	.vidcon1	= VIDCON1_INV_VCLK | VIDCON1_INV_HSYNC |
			  VIDCON1_INV_VSYNC,
#elif defined(CONFIG_LCD_LTE480WV)
	.vidcon1	= VIDCON1_INV_HSYNC | VIDCON1_INV_VSYNC,
#endif
	.setup_gpio	= exynos4_fimd0_gpio_setup_24bpp,
};
#endif

#ifdef CONFIG_FB_S5P
#if defined(CONFIG_FB_S5P_DUMMY_MIPI_LCD)
#define		LCD_BUS_NUM	3
#define		DISPLAY_CS	EXYNOS4_GPB(5)
#define		DISPLAY_CLK	EXYNOS4_GPB(4)
#define		DISPLAY_SI	EXYNOS4_GPB(7)

static struct s3cfb_lcd dummy_mipi_lcd = {
	.width = 480,
	.height = 800,
	.bpp = 24,

	.freq = 60,

	.timing = {
		.h_fp = 0x16,
		.h_bp = 0x16,
		.h_sw = 0x2,
		.v_fp = 0x28,
		.v_fpe = 2,
		.v_bp = 0x1,
		.v_bpe = 1,
		.v_sw = 3,
		.cmd_allow_len = 0x4,
	},

	.polarity = {
		.rise_vclk = 0,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};

static struct s3c_platform_fb fb_platform_data __initdata = {
	.hw_ver		= 0x70,
	.clk_name	= "sclk_lcd",
	.nr_wins	= 5,
	.default_win	= CONFIG_FB_S5P_DEFAULT_WINDOW,
	.swap		= FB_SWAP_HWORD | FB_SWAP_WORD,
};

static void lcd_cfg_gpio(void)
{
	return;
}

static int reset_lcd(void)
{
	int err = 0;

	/* fire nRESET on power off */
	err = gpio_request(EXYNOS4_GPX3(1), "GPX3");
	if (err) {
		printk(KERN_ERR "failed to request GPX0 for lcd reset control\n");
		return err;
	}

#ifdef CONFIG_CPU_EXYNOS4212
	gpio_direction_output(EXYNOS4_GPX2(7), 1);
	mdelay(100);

	gpio_set_value(EXYNOS4_GPX2(7), 0);
	mdelay(100);
	gpio_set_value(EXYNOS4_GPX2(7), 1);
	mdelay(100);
	gpio_free(EXYNOS4_GPX2(7));
#else
	gpio_direction_output(EXYNOS4_GPX3(1), 1);
	mdelay(100);

	gpio_set_value(EXYNOS4_GPX3(1), 0);
	mdelay(100);
	gpio_set_value(EXYNOS4_GPX3(1), 1);
	mdelay(100);
	gpio_free(EXYNOS4_GPX3(1));
#endif
	return 0;
}

static int lcd_power_on(void *pdev, int enable)
{
	return 1;
}

static void __init mipi_fb_init(void)
{
	struct s5p_platform_dsim *dsim_pd = NULL;
	struct mipi_ddi_platform_data *mipi_ddi_pd = NULL;
	struct dsim_lcd_config *dsim_lcd_info = NULL;

	/* gpio pad configuration for rgb and spi interface. */
	lcd_cfg_gpio();

	/*
	 * register lcd panel data.
	 */
	dsim_pd = (struct s5p_platform_dsim *)
		s5p_device_dsim.dev.platform_data;

	strcpy(dsim_pd->lcd_panel_name, "dummy_mipi_lcd");

	dsim_lcd_info = dsim_pd->dsim_lcd_info;
	dsim_lcd_info->lcd_panel_info = (void *)&dummy_mipi_lcd;

	mipi_ddi_pd = (struct mipi_ddi_platform_data *)
		dsim_lcd_info->mipi_ddi_pd;
	mipi_ddi_pd->lcd_reset = reset_lcd;
	mipi_ddi_pd->lcd_power_on = lcd_power_on;

	platform_device_register(&s5p_device_dsim);

	s3cfb_set_platdata(&fb_platform_data);

	printk(KERN_INFO "platform data of %s lcd panel has been registered.\n",
			dsim_pd->lcd_panel_name);
}
#else //defined(CONFIG_FB_S5P_MIPI_S500HD)#if defined(CONFIG_FB_S5P_MIPI_DSIM)
#define		LCD_BUS_NUM	3
#define		DISPLAY_CS	EXYNOS4_GPB(5)
#define		DISPLAY_CLK	EXYNOS4_GPB(4)
#define		DISPLAY_SI	EXYNOS4_GPB(7)

static struct s3cfb_lcd s500hd_mipi_lcd = {
	.width	= 720,
	.height	= 1280,
	.bpp	= 24,
	.freq	= 65,

	.timing = {
		.h_fp = 10,
		.h_bp = 10,
		.h_sw = 3,
		.v_fp = 10,
		.v_fpe = 1,
		.v_bp = 10,
		.v_bpe = 1,
		.v_sw = 3,
		//.cmd_allow_len = 4,
		.cmd_allow_len = 11,	/*v_fp=stable_vfp + cmd_allow_len */
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};

static struct s3c_platform_fb fb_platform_data __initdata = {
	.hw_ver		= 0x70,
	.clk_name	= "sclk_lcd",
	.nr_wins	= 5,
	.default_win	= CONFIG_FB_S5P_DEFAULT_WINDOW,
	.swap		= FB_SWAP_HWORD | FB_SWAP_WORD,
};

static void lcd_cfg_gpio(void)
{
	return;
}

static int reset_lcd(void)
{
	int err = 0;
	printk("_+_+mipi reset lcd_+_+_\n");
	/* fire nRESET on power off */

	s3c_gpio_cfgpin(EXYNOS4212_GPM2(0), S3C_GPIO_SFN(1));
	gpio_direction_output(EXYNOS4212_GPM2(0),1);
	
	s3c_gpio_cfgpin(EXYNOS4212_GPM2(3), S3C_GPIO_SFN(1));
	gpio_direction_output(EXYNOS4212_GPM2(3),1);	
	//mdelay(100);

	s3c_gpio_cfgpin(EXYNOS4212_GPM2(4), S3C_GPIO_SFN(1));
	gpio_direction_output(EXYNOS4212_GPM2(4),1);

	//mdelay(10);
	gpio_direction_output(EXYNOS4212_GPM2(4),0);

	mdelay(10);
	gpio_direction_output(EXYNOS4212_GPM2(4),1);
	
	return 0;
}

static int lcd_power_on(void *pdev, int enable)
{
	printk(KERN_INFO"_+_+mipi lcd_power_on+_+_\n");
	return 1;
}

static void __init mipi_fb_init(void)
{
	struct s5p_platform_dsim *dsim_pd = NULL;
	struct mipi_ddi_platform_data *mipi_ddi_pd = NULL;
	struct dsim_lcd_config *dsim_lcd_info = NULL;

	printk(KERN_INFO"_+_+%s_+_+_\n",__func__);

	/* gpio pad configuration for rgb and spi interface. */
	lcd_cfg_gpio();

	/*
	 * register lcd panel data.
	 */
	dsim_pd = (struct s5p_platform_dsim *)
		s5p_device_dsim.dev.platform_data;

	strcpy(dsim_pd->lcd_panel_name, "mipi_s500hd");

	dsim_lcd_info = dsim_pd->dsim_lcd_info;
	dsim_lcd_info->lcd_panel_info = (void *)&s500hd_mipi_lcd;

	/* 483Mbps for Q1 */
	//dsim_pd->dsim_info->p = 3;
	//dsim_pd->dsim_info->m = 100;
	//dsim_pd->dsim_info->s = 1;


	mipi_ddi_pd = (struct mipi_ddi_platform_data *)
		dsim_lcd_info->mipi_ddi_pd;
	mipi_ddi_pd->lcd_reset = reset_lcd;
	mipi_ddi_pd->lcd_power_on = lcd_power_on;

	platform_device_register(&s5p_device_dsim);

	s3cfb_set_platdata(&fb_platform_data);

	printk(KERN_INFO "platform data of %s lcd panel has been registered.\n",
			dsim_pd->lcd_panel_name);
}
#endif
#endif

//static int exynos4_notifier_call(struct notifier_block *this,
//					unsigned long code, void *_cmd)
//{
//	int mode = 0;
//
//	if ((code == SYS_RESTART) && _cmd)
//		if (!strcmp((char *)_cmd, "recovery"))
//			mode = 0xf;
//
//	__raw_writel(mode, REG_INFORM4);
//
//	return NOTIFY_DONE;
//}
//
//static struct notifier_block exynos4_reboot_notifier = {
//	.notifier_call = exynos4_notifier_call,
//};

#ifdef CONFIG_EXYNOS4_DEV_DWMCI
static void exynos_dwmci_cfg_gpio(int width)
{
	unsigned int gpio;
//	printk("%s(); width=%d\n", __func__, width);

	gpio = EXYNOS4210_GPJ1(3);

	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV4);


	for (gpio = EXYNOS4_GPK0(0); gpio < EXYNOS4_GPK0(2); gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
//		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV2);
		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV3);
	}

	switch (width) {
	case 8:
		for (gpio = EXYNOS4_GPK1(3); gpio <= EXYNOS4_GPK1(6); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(4));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
			s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV2);
//			s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV3);
		}
	case 4:
		for (gpio = EXYNOS4_GPK0(3); gpio <= EXYNOS4_GPK0(6); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
			s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV2);
//			s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV3);
		}
		break;
	case 1:
		gpio = EXYNOS4_GPK0(3);
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV2);
//		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV3);
	default:
		break;
	}
}

static struct dw_mci_board exynos_dwmci_pdata __initdata = {
	.num_slots		= 1,
	.quirks			= DW_MCI_QUIRK_BROKEN_CARD_DETECTION | DW_MCI_QUIRK_HIGHSPEED,
//	.bus_hz			= 100 * 1000 * 1000,
	.bus_hz			= 80 * 1000 * 1000,
//	.bus_hz			= 40 * 1000 * 1000,
	.caps			= /*MMC_CAP_UHS_DDR50 |*/ MMC_CAP_1_8V_DDR |
				MMC_CAP_8_BIT_DATA |
				MMC_CAP_CMD23,
	.fifo_depth		= 0x80,
	.detect_delay_ms	= 200,
	.hclk_name		= "dwmci",
	.cclk_name		= "sclk_dwmci",
	.cfg_gpio		= exynos_dwmci_cfg_gpio,
};
#endif

#ifdef CONFIG_S3C_DEV_HSMMC
static struct s3c_sdhci_platdata smdk4x12_hsmmc0_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_INTERNAL,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
#ifdef CONFIG_EXYNOS4_SDHCI_CH0_8BIT
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#endif
};
#endif

#ifdef CONFIG_S3C_DEV_HSMMC1
static struct s3c_sdhci_platdata smdk4x12_hsmmc1_pdata_d816 __initdata = {
	.cd_type		= S3C_SDHCI_CD_PERMANENT/*S3C_SDHCI_CD_INTERNAL*/,
	.clk_type		= S3C_SDHCI_CLK_DIV_INTERNAL /*S3C_SDHCI_CLK_DIV_EXTERNAL*/,
	.max_width		= 4,
};

static struct s3c_sdhci_platdata smdk4x12_hsmmc1_pdata_d720 __initdata = {
//	.cd_type		= S3C_SDHCI_CD_PERMANENT/*S3C_SDHCI_CD_INTERNAL*/,
	.cd_type		= S3C_SDHCI_CD_GPIO,   // Modified by Leslie 
//	.clk_type		= S3C_SDHCI_CLK_DIV_INTERNAL /*S3C_SDHCI_CLK_DIV_EXTERNAL*/,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
	.max_width		= 4,

	
	.ext_cd_gpio=EXYNOS4_GPX0(3),
	.ext_cd_gpio_invert=1,

};

#endif

#ifdef CONFIG_S3C_DEV_HSMMC2
static struct s3c_sdhci_platdata smdk4x12_hsmmc2_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_INTERNAL,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
#ifdef CONFIG_EXYNOS4_SDHCI_CH2_8BIT
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#endif
};
#endif

#ifdef CONFIG_S3C_DEV_HSMMC3
static struct s3c_sdhci_platdata smdk4x12_bcm4330_hsmmc3_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_PERMANENT,
	.clk_type		= S3C_SDHCI_CLK_DIV_INTERNAL,
//	.cd_type		= S3C_SDHCI_CD_EXTERNAL,
//	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
	.pm_flags = MMC_PM_IGNORE_SUSPEND_RESUME|MMC_PM_FOR_WIFI_CHANNEL,
};

static struct s3c_sdhci_platdata smdk4x12_hsmmc3_pdata __initdata = {

#if defined( CONFIG_WIFI_TEST)
	.cd_type		= S3C_SDHCI_CD_INTERNAL
//	.cd_type		= S3C_SDHCI_CD_EXTERNAL,
//	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
#else
//	.cd_type		= S3C_SDHCI_CD_PERMANENT,
//	.clk_type		= S3C_SDHCI_CLK_DIV_INTERNAL,
	.cd_type		= S3C_SDHCI_CD_NONE,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
#endif


	.pm_flags = MMC_PM_IGNORE_SUSPEND_RESUME|MMC_PM_FOR_WIFI_CHANNEL,
};
#endif

#ifdef CONFIG_S5P_DEV_MSHC
static struct s3c_mshci_platdata exynos4_mshc_pdata __initdata = {
	.cd_type		= S3C_MSHCI_CD_PERMANENT,
	.has_wp_gpio		= true,
	.wp_gpio		= 0xffffffff,
#if defined(CONFIG_EXYNOS4_MSHC_8BIT) && \
	defined(CONFIG_EXYNOS4_MSHC_DDR)
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA | MMC_CAP_1_8V_DDR |
				  MMC_CAP_UHS_DDR50,
#elif defined(CONFIG_EXYNOS4_MSHC_8BIT)
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#elif defined(CONFIG_EXYNOS4_MSHC_DDR)
	.host_caps		= MMC_CAP_1_8V_DDR | MMC_CAP_UHS_DDR50,
#endif
};
#endif

#ifdef CONFIG_USB_EHCI_S5P
static struct s5p_ehci_platdata smdk4x12_ehci_pdata;

static void __init smdk4x12_ehci_init(void)
{
	struct s5p_ehci_platdata *pdata = &smdk4x12_ehci_pdata;

	s5p_ehci_set_platdata(pdata);
}
#endif

#ifdef CONFIG_USB_OHCI_S5P
static struct s5p_ohci_platdata smdk4x12_ohci_pdata;

static void __init smdk4x12_ohci_init(void)
{
	struct s5p_ohci_platdata *pdata = &smdk4x12_ohci_pdata;

	s5p_ohci_set_platdata(pdata);
}
#endif

/* USB GADGET */
#ifdef CONFIG_USB_GADGET
static struct s5p_usbgadget_platdata smdk4x12_usbgadget_pdata;

#include <linux/usb/android_composite.h>

static void __init smdk4x12_usbgadget_init(void)
{
	struct s5p_usbgadget_platdata *pdata = &smdk4x12_usbgadget_pdata;

	struct android_usb_platform_data *android_pdata =
		s3c_device_android_usb.dev.platform_data;
	if (android_pdata) {
		unsigned int newluns = 2;
		printk(KERN_DEBUG "usb: %s: default luns=%d, new luns=%d\n",
				__func__, android_pdata->nluns, newluns);
		android_pdata->nluns = newluns;
	} else {
		printk(KERN_DEBUG "usb: %s android_pdata is not available\n",
				__func__);
	}

	s5p_usbgadget_set_platdata(pdata);
}
#endif


#ifdef CONFIG_VIDEO_S5P_MIPI_CSIS
static struct regulator_consumer_supply mipi_csi_fixed_voltage_supplies[] = {
	REGULATOR_SUPPLY("mipi_csi", "s5p-mipi-csis.0"),
	REGULATOR_SUPPLY("mipi_csi", "s5p-mipi-csis.1"),
};

static struct regulator_init_data mipi_csi_fixed_voltage_init_data = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(mipi_csi_fixed_voltage_supplies),
	.consumer_supplies	= mipi_csi_fixed_voltage_supplies,
};

static struct fixed_voltage_config mipi_csi_fixed_voltage_config = {
	.supply_name	= "DC_5V",
	.microvolts	= 5000000,
	.gpio		= -EINVAL,
	.init_data	= &mipi_csi_fixed_voltage_init_data,
};

static struct platform_device mipi_csi_fixed_voltage = {
	.name		= "reg-fixed-voltage",
	.id		= 3,
	.dev		= {
		.platform_data	= &mipi_csi_fixed_voltage_config,
	},
};
#endif

//#ifdef CONFIG_VIDEO_M5MOLS
//static struct regulator_consumer_supply m5mols_fixed_voltage_supplies[] = {
//	REGULATOR_SUPPLY("core", NULL),
//	REGULATOR_SUPPLY("dig_18", NULL),
//	REGULATOR_SUPPLY("d_sensor", NULL),
//	REGULATOR_SUPPLY("dig_28", NULL),
//	REGULATOR_SUPPLY("a_sensor", NULL),
//	REGULATOR_SUPPLY("dig_12", NULL),
//};
//
//static struct regulator_init_data m5mols_fixed_voltage_init_data = {
//	.constraints = {
//		.always_on = 1,
//	},
//	.num_consumer_supplies	= ARRAY_SIZE(m5mols_fixed_voltage_supplies),
//	.consumer_supplies	= m5mols_fixed_voltage_supplies,
//};
//
//static struct fixed_voltage_config m5mols_fixed_voltage_config = {
//	.supply_name	= "CAM_SENSOR",
//	.microvolts	= 1800000,
//	.gpio		= -EINVAL,
//	.init_data	= &m5mols_fixed_voltage_init_data,
//};
//
//static struct platform_device m5mols_fixed_voltage = {
//	.name		= "reg-fixed-voltage",
//	.id		= 4,
//	.dev		= {
//		.platform_data	= &m5mols_fixed_voltage_config,
//	},
//};
//#endif


//static struct i2c_board_info i2c_devs0[] __initdata = {
//#if defined(CONFIG_MFD_MAX77686)
//	{
//		I2C_BOARD_INFO("max77686", (0x12 >> 1)),
//		.platform_data	= &exynos4_max77686_info,
//	},
//#endif
//#ifdef CONFIG_REGULATOR_S5M8767
//	{
//		I2C_BOARD_INFO("s5m87xx", 0xCC >> 1),
//		.platform_data = &exynos4_s5m8767_pdata,
//		.irq		= IRQ_EINT(26),
//	},
//#endif
//};

//static struct platform_device host_detected_device = {
//	.name             = "detected-device",
//	.dev = {
//		.platform_data    = NULL,
//	},
//	.num_resources	= 0,
//	.resource = NULL,
//};

static struct i2c_board_info i2c_devs0[] __initdata = {
#ifdef CONFIG_VIDEO_TVOUT
	{
		I2C_BOARD_INFO("s5p_ddc", (0x74 >> 1)),
	},
#endif
};

//static struct i2c_board_info i2c_devs1[] __initdata = {
//#ifdef CONFIG_MFD_WM8994
//	{
//		I2C_BOARD_INFO("wm8994", 0x1a),
//		.platform_data	= &wm8994_platform_data,
//	},
//#endif
//#ifdef CONFIG_REGULATOR_ACT8847
//	{
//		I2C_BOARD_INFO("act8847", 0x5A),
//		.platform_data = &act8847_data,
//	},
//#endif
//};

static struct bq24190_platform_data bq24190_data = {
	.gpio_int=EXYNOS4_GPX2(7),
};

static struct i2c_board_info i2c_devs1[] __initdata = {
	{
		I2C_BOARD_INFO("bq24190-charger", 0x6B),
		.platform_data = &bq24190_data,
	},
};

struct cw_bat_platform_data {

        int is_dc_charge;
        int dc_det_pin;
        int dc_det_level;

        int is_usb_charge;
        int chg_mode_sel_pin;
        int chg_mode_sel_level;

        int bat_low_pin;
        int bat_low_level;
        int chg_ok_pin;
        int chg_ok_level;
        u8* cw_bat_config_info;
};

#define SIZE_BATINFO    64 
/*
static u8 cw201x_config_info[SIZE_BATINFO] = {
		0x15, 0x42, 0x60, 0x59, 0x52,
		0x58, 0x4D, 0x48, 0x48, 0x44,
		0x44, 0x46, 0x49, 0x48, 0x32,
		0x24, 0x20, 0x17, 0x13, 0x0F,
		0x19, 0x3E, 0x51, 0x45, 0x08,
		0x76, 0x0B, 0x85, 0x0E, 0x1C,
		0x2E, 0x3E, 0x4D, 0x52, 0x52,
		0x57, 0x3D, 0x1B, 0x6A, 0x2D,
		0x25, 0x43, 0x52, 0x87, 0x8F,
		0x91, 0x94, 0x52, 0x82, 0x8C,
		0x92, 0x96, 0xFF, 0x7B, 0xBB,
		0xCB, 0x2F, 0x7D, 0x72, 0xA5,
		0xB5, 0xC1, 0x46, 0xAE};
*/
static u8 cw201x_config_info[SIZE_BATINFO] = {
		0x14,0x83,0x59,0x59,0x55,0x50,0x4D,0x4C,0x47,0x46,
		0x3E,0x38,0x2A,0x29,0x2E,0x20,0x13,0x0C,0x0A,0x10,
		0x16,0x35,0x4B,0x6B,0x85,0x82,0x0C,0xCD,0x22,0x43,
		0x48,0x48,0x45,0x47,0x4B,0x4C,0x3D,0x13,0x34,0x00,
		0x00,0x1B,0x52,0x87,0x8F,0x91,0x94,0x52,0x82,0x8C,
		0x92,0x96,0x4C,0x54,0x9C,0xCB,0x2F,0x7D,0x72,0xA5,
		0xB5,0xC1,0x46,0xAE};
struct cw_bat_platform_data cw201x_platform_data = {
/*	.dc_det_pin = NULL,
	.chg_mode_sel_pin = NULL,
	.chg_ok_pin = NULL,
	.bat_low_pin = EXYNOS4_GPX0(2),
	.cw_bat_config_info = &cw201x_config_info, */
	.dc_det_pin = EXYNOS4_GPX0(2),
	.dc_det_level = 1,
	.chg_mode_sel_pin = NULL,
	.chg_ok_pin = NULL,
	.bat_low_pin = NULL,
	.cw_bat_config_info = cw201x_config_info,
};


#if (defined CONFIG_BCM2079X_I2C) || (defined CONFIG_BCM2079X_I2C_MODULE)
static struct bcm2079x_platform_data bcm2079x_pdata = {
    .irq_gpio = EXYNOS4_GPX1(1),
    .en_gpio = EXYNOS4_GPL2(4),
    .wake_gpio = EXYNOS4_GPL0(3),
};
#endif


static struct i2c_board_info i2c_devs2[] __initdata = {

#ifdef CONFIG_QN8027_FM
	{
		I2C_BOARD_INFO("qn8027", 0x2c),
	},
#endif

	{
		I2C_BOARD_INFO("lm75a", (0x48)),
	},

#ifdef CONFIG_CW2015_BATTERY
	{
		I2C_BOARD_INFO("cw201x", (0xC4 >> 1)),
		.platform_data	= &cw201x_platform_data,
	},
#endif
};

#if (defined CONFIG_BCM2079X_I2C) || (defined CONFIG_BCM2079X_I2C_MODULE)
static struct i2c_board_info i2c_devs2_ap6441[] __initdata = {
	{
		I2C_BOARD_INFO("bcm2079x-i2c", 0x76),
		.irq = IRQ_EINT(9),
		.platform_data = &bcm2079x_pdata,
	},

};

static struct i2c_board_info i2c_devs2_ap6493[] __initdata = {
	{
		I2C_BOARD_INFO("bcm2079x-i2c", 0x77),
		.irq = IRQ_EINT(9),
		.platform_data = &bcm2079x_pdata,
	},

};
#endif 

#ifdef CONFIG_TOUCHSCREEN_VTL_CT36X
#include "../../../drivers/oem_drv/touchscreen/vtl_ts/vtl_ts.h"

struct ts_config_info	vtl_ts_config_info = {

	.screen_max_x	 = 1920,
	.screen_max_y	 = 1200,
	.irq_gpio_number = GPIO_CTP_INT,
	.rst_gpio_number = GPIO_CTP_RST,
	.b_CtpRestPinOpposite = 0,
};
#endif
static struct i2c_board_info i2c_devs3[] __initdata = {
#ifdef CONFIG_TOUCHSCREEN_VTL_CT36X
	{
		.type		="vtl_ts",
		.addr		=0x01,
		.flags		=0,
		.irq		=  GPIO_CTP_INT,
		.platform_data = &vtl_ts_config_info,
	},
#endif
	{ I2C_BOARD_INFO("Goodix-TS", 0x5D), },
	{ I2C_BOARD_INFO("novatek_ts", 0x5C), },
	{ I2C_BOARD_INFO("uc_ts", 0x44),   },
	{ I2C_BOARD_INFO("ft5x0x_ts",0x38),   },
	{ I2C_BOARD_INFO("dp501_p0", (0x10 >> 1)), },
	{ I2C_BOARD_INFO("dp501_p2", (0x14 >> 1)), },
	{ I2C_BOARD_INFO("gslx680", 0x40),	}
};

static struct i2c_board_info i2c_devs4_bh1721[] __initdata = {
	 {
		I2C_BOARD_INFO("bh1721",0x23),
	},
};

static struct i2c_board_info i2c_devs4_rt5621[] __initdata = {
	 {
	 		I2C_BOARD_INFO("rt5621", 0x1a),
	 },
};
static struct i2c_board_info i2c_devs4_wm8978[] __initdata = {
	 {
	 		I2C_BOARD_INFO("wm8978", 0x1a),
	 },
};

static struct i2c_board_info i2c_devs4_ac100[] __initdata = {
	 {
	 		I2C_BOARD_INFO("ac100", 0x1a),
	 },
};

#ifdef CONFIG_PN544
static struct pn544_i2c_platform_data pn544_device = {
//	.irq_gpio = GPIO_GYRO_INT,
	.irq_gpio = GPIO_SENSOR_INT,
//	ven_gpio,
//	firm_gpio,
};

#endif

static struct i2c_board_info i2c_devs5[] __initdata = {

	// {I2C_BOARD_INFO("mma7660",0x4C),}, // alter by yangmiansi @2013-09-28
	// MPU3050 (Gyro)
	{
		I2C_BOARD_INFO(MPU_NAME, 0x68),
		.platform_data 	= &mpu3050_data,
		.irq		= IRQ_GYRO_INT/*IRQ_EINT(9)*/,
	},
	// BMA250 (Accel)
	{
		I2C_BOARD_INFO("bma250", (0x30>>1)),
		.platform_data = &inv_mpu_bma250_data,
	},	

#ifdef CONFIG_MPU_SENSORS_MMC314X_ICS
	// MMC314x (Mag)
	{
		I2C_BOARD_INFO("mmc341x", (0x60>>1)), // alter by yangmiansi @2013-09-28
		.platform_data = &inv_mpu_hmc5883_data,
	},
#endif
	
	{
		I2C_BOARD_INFO("mma7660", 0x4C),
	},
#ifdef CONFIG_PN544
	{
		I2C_BOARD_INFO("pn544", 0x28),
		.platform_data = &pn544_device,
//		.irq = IRQ_GYRO_INT,
		.irq = IRQ_SENSOR_INT,
	},
#endif

	{
		I2C_BOARD_INFO("bh1721",0x23),
	},

};

static struct i2c_board_info i2c_devs6[] __initdata = {
	{
		I2C_BOARD_INFO("gsensor", (0x30 >> 1)),
		I2C_BOARD_INFO("gsensor2", (0x7a >> 1)),
	},
};

static struct i2c_board_info i2c_devs7[] __initdata = {
	#ifdef CONFIG_TOUCHSCREEN_PIXCIR
	{
		I2C_BOARD_INFO("pixcir-ts", 0x5C),
	},
	#endif
};
#ifdef CONFIG_ANDROID_PMEM
static struct android_pmem_platform_data pmem_pdata = {
	.name		= "pmem",
	.no_allocator	= 1,
	.cached		= 0,
	.start		= 0,
	.size		= 0
};

static struct android_pmem_platform_data pmem_gpu1_pdata = {
	.name		= "pmem_gpu1",
	.no_allocator	= 1,
	.cached		= 0,
	.start		= 0,
	.size		= 0,
};

static struct platform_device pmem_device = {
	.name	= "android_pmem",
	.id	= 0,
	.dev	= {
		.platform_data = &pmem_pdata
	},
};

static struct platform_device pmem_gpu1_device = {
	.name	= "android_pmem",
	.id	= 1,
	.dev	= {
		.platform_data = &pmem_gpu1_pdata
	},
};

static void __init android_pmem_set_platdata(void)
{
#if defined(CONFIG_S5P_MEM_CMA)
	pmem_pdata.size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM * SZ_1K;
	pmem_gpu1_pdata.size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_GPU1 * SZ_1K;
#endif
}
#endif

#ifdef CONFIG_BATTERY_SAMSUNG
static struct platform_device samsung_device_battery = {
	.name	= "samsung-fake-battery",
	.id	= -1,
};
#endif
/* s5p-pmic interface */
static struct resource s5p_pmic_resource[] = {

};


struct platform_device s5p_device_pmic = {
  .name             = "s5p-pmic",
  .id               = -1,
  .num_resources    = ARRAY_SIZE(s5p_pmic_resource),
  .resource         = s5p_pmic_resource,



};

EXPORT_SYMBOL(s5p_device_pmic);
/*
struct gpio_keys_button smdk4x12_button[] = {
	{
		.code = KEY_POWER,
		.gpio = EXYNOS4_GPX0(0),
		.active_low = 1,
		.wakeup = 1,
	}
};

struct gpio_keys_platform_data smdk4x12_gpiokeys_platform_data = {
	smdk4x12_button,
	ARRAY_SIZE(smdk4x12_button),
};

#if defined(CONFIG_KEYBOARD_GPIO)
static struct platform_device smdk4x12_gpio_keys = {
	.name	= "gpio-keys",
	.dev	= {
		.platform_data = &smdk4x12_gpiokeys_platform_data,
	},
};
#endif
*/
#ifdef CONFIG_KEYBOARD_GPIO_STUTTGART
struct gpio_keys_button stuttgart_button_table[] = {
	{
		.desc = "power",
		.code = KEY_POWER,
		.gpio = EXYNOS4_GPX0(2),/*EXYNOS4_GPX3(1)*/
		.active_low = 1,
		.type = EV_KEY,
		.can_disable = 0,
		.wakeup = 1,
	},{
		.desc = "volumedown",
		.code = KEY_VOLUMEDOWN,
		.gpio = EXYNOS4_GPX2(0),
		.active_low = 1,
		.type = EV_KEY,
		.can_disable = 0,
		.wakeup = 0,
		.debounce_interval = 12,
	}, {
		.desc = "volumeup",
		.code = KEY_VOLUMEUP,
		.gpio = EXYNOS4_GPX2(1),
		.active_low = 1,
		.type = EV_KEY,
		.can_disable = 0,
		.wakeup = 0,
		.debounce_interval = 12,
	},

};

static struct gpio_keys_platform_data stuttgart_button_data = {
	.buttons  = stuttgart_button_table,
	.nbuttons = ARRAY_SIZE(stuttgart_button_table),
};

struct platform_device stuttgart_buttons = {
	.name   = "stuttgart-gpio-keypad",
	.dev  = {
		.platform_data = &stuttgart_button_data,
	},
	.id   = -1,
};
#endif



#ifdef CONFIG_WAKEUP_ASSIST
static struct platform_device wakeup_assist_device = {
	.name   = "wakeup_assist",
};
#endif

#ifdef CONFIG_VIDEO_FIMG2D
static struct fimg2d_platdata fimg2d_data __initdata = {
	.hw_ver = 0x41,
	.parent_clkname = "mout_g2d0",
	.clkname = "sclk_fimg2d",
	.gate_clkname = "fimg2d",
	.clkrate = 267 * 1000000,	/* 266 Mhz */
};
#endif



#ifdef CONFIG_USB_EXYNOS_SWITCH
static struct s5p_usbswitch_platdata smdk4x12_usbswitch_pdata;

static void __init smdk4x12_usbswitch_init(void)
{
	struct s5p_usbswitch_platdata *pdata = &smdk4x12_usbswitch_pdata;
	int err;

	pdata->gpio_host_detect = EXYNOS4_GPX3(5); /* low active */
	err = gpio_request_one(pdata->gpio_host_detect, GPIOF_IN, "HOST_DETECT");
	if (err) {
		printk(KERN_ERR "failed to request gpio_host_detect\n");
		return;
	}

	s3c_gpio_cfgpin(pdata->gpio_host_detect, S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(pdata->gpio_host_detect, S3C_GPIO_PULL_NONE);
	gpio_free(pdata->gpio_host_detect);

	pdata->gpio_device_detect = EXYNOS4_GPX3(4); /* high active */
	err = gpio_request_one(pdata->gpio_device_detect, GPIOF_IN, "DEVICE_DETECT");
	if (err) {
		printk(KERN_ERR "failed to request gpio_host_detect for\n");
		return;
	}

	s3c_gpio_cfgpin(pdata->gpio_device_detect, S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(pdata->gpio_device_detect, S3C_GPIO_PULL_NONE);
	gpio_free(pdata->gpio_device_detect);

	if (is_axp_pmu) {
		pdata->gpio_host_vbus = GPIO_EXAXP22(4);
	} else {
		pdata->gpio_host_vbus = EXYNOS4_GPL2(0);

		err = gpio_request_one(pdata->gpio_host_vbus, GPIOF_OUT_INIT_LOW, "HOST_VBUS_CONTROL");
		if (err) {
			printk(KERN_ERR "\n\n\n!!! failed to request gpio_host_vbus\n");
			return;
		}

		s3c_gpio_setpull(pdata->gpio_host_vbus, S3C_GPIO_PULL_NONE);
		gpio_free(pdata->gpio_host_vbus);
	}

	s5p_usbswitch_set_platdata(pdata);
}
#endif

#ifdef CONFIG_MTK_COMBO
static DEFINE_MUTEX(notify_lock);

extern void sdhci_s3c_notify_change(struct platform_device *dev, int state);
//extern void mmc_miscwifi_sdio_remove(void);
int s3c_wlan_set_carddetect(int onoff)
{
	printk("\n#%s(%d) . \n", __func__, onoff);

	if(!is_mtk_combo())
		return -1;

	mutex_lock(&notify_lock);
	if (onoff){
		sdhci_s3c_notify_change(&s3c_device_hsmmc3, 1);
	} else {
		sdhci_s3c_notify_change(&s3c_device_hsmmc3, 0);
	}
	mutex_unlock(&notify_lock);

	return 0;
}
EXPORT_SYMBOL(s3c_wlan_set_carddetect);
#endif


#ifdef CONFIG_BUSFREQ_OPP
/* BUSFREQ to control memory/bus*/
static struct device_domain busfreq;
#endif

static struct platform_device exynos4_busfreq = {
	.id = -1,
	.name = "exynos-busfreq",
};

static struct platform_device *smdk4412_devices[] __initdata = {
	&s3c_device_adc,
};

#ifdef CONFIG_SEC_WATCHDOG_RESET  //have bug
static struct platform_device watchdog_reset_device = {
        .name = "watchdog-reset",
        .id = -1,
};
#endif

#ifdef CONFIG_URBETTER_SIM_DETECT  //have bug
static struct platform_device gsm_device = {
        .name = "gsm-module",
        .id = -1,
};
#endif

#if defined(CONFIG_DHD_1_88_10)||defined(CONFIG_BCMDHD)
static struct platform_device bcm4330_bluetooth_device = {
	.name = "bcm4330_bluetooth",
	.id = -1,
};
#endif				/* CONFIG_BT_BCM4330 */


#ifdef CONFIG_KEYBOARD_SAMSUNG
static uint32_t smdk4x12_keymap[] __initdata = {
	/* KEY(row, col, keycode) */
	KEY(0, 0, KEY_1), KEY(0, 1, KEY_2), KEY(0, 2, KEY_3),KEY(0, 3, KEY_4), 
	KEY(1, 1, KEY_5),KEY(1, 2, KEY_6), KEY(1, 3, KEY_7), KEY(1, 4, KEY_8),
	KEY(2, 1, KEY_9), KEY(2, 2, KEY_A),KEY(2, 3, KEY_B), KEY(2, 4, KEY_C),
	KEY(3, 1, KEY_E), KEY(3, 2, KEY_C),KEY(3, 3, KEY_DOWN), KEY(3, 4, KEY_MENU)
};

static struct matrix_keymap_data smdk4x12_keymap_data __initdata = {
        .keymap         = smdk4x12_keymap,
        .keymap_size    = ARRAY_SIZE(smdk4x12_keymap),
};

static struct samsung_keypad_platdata smdk4x12_keypad_data __initdata = {
        .keymap_data    = &smdk4x12_keymap_data,
        .rows           = 4,
        .cols           = 4,
};
#endif

static struct platform_device *smdk4x12_devices[] __initdata = {
#ifdef CONFIG_ANDROID_PMEM
	&pmem_device,
	&pmem_gpu1_device,
#endif
#ifdef CONFIG_URBETTER_SIM_DETECT
        &gsm_device,
#endif
#ifdef CONFIG_SEC_WATCHDOG_RESET
        &watchdog_reset_device,
#endif
	/* Samsung Power Domain */
	&exynos4_device_pd[PD_MFC],
	&exynos4_device_pd[PD_G3D],
	&exynos4_device_pd[PD_LCD0],
	&exynos4_device_pd[PD_CAM],
	&exynos4_device_pd[PD_TV],
	&exynos4_device_pd[PD_GPS],
	&exynos4_device_pd[PD_GPS_ALIVE],
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
	&exynos4_device_pd[PD_ISP],
#endif
#ifdef CONFIG_FB_MIPI_DSIM
	&s5p_device_mipi_dsim,
#endif
/* mainline fimd */

	/* legacy fimd */
#ifdef CONFIG_FB_S5P
	&s3c_device_fb,
#endif
	&s3c_device_wdt,
	&s3c_device_rtc,
	&s3c_device_i2c0,
//	&s3c_device_i2c1,
	&s3c_device_i2c2,
//	&s3c_device_i2c3,
	&gpio_i2c3_dev,
	&s3c_device_i2c4,
	&s3c_device_i2c5,
#if 1// i2c6
	&s3c_device_i2c6,
#endif
	&s3c_device_i2c7,
	&s5p_device_pmic,
#ifdef CONFIG_USB_EHCI_S5P
	&s5p_device_ehci,
#endif
#ifdef CONFIG_USB_OHCI_S5P
	&s5p_device_ohci,
#endif
#ifdef CONFIG_USB_GADGET
	&s3c_device_usbgadget,
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
	&s3c_device_rndis,
#endif
#if defined( CONFIG_USB_ANDROID) || defined(CONFIG_USB_G_ANDROID)
	&s3c_device_android_usb,
	&s3c_device_usb_mass_storage,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC
//	&s3c_device_hsmmc0,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
//	&s3c_device_hsmmc1,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	&s3c_device_hsmmc2,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	&s3c_device_hsmmc3,
#endif
#ifdef CONFIG_S5P_DEV_MSHC
	&s3c_device_mshci,
#endif
#ifdef CONFIG_EXYNOS4_DEV_DWMCI
	&exynos_device_dwmci,
#endif
#ifdef CONFIG_SND_SAMSUNG_AC97
	&exynos_device_ac97,
#endif
#ifdef CONFIG_SND_SAMSUNG_I2S
	&exynos_device_i2s0,
#endif
#ifdef CONFIG_SND_SAMSUNG_PCM
	&exynos_device_pcm0,
#endif
#ifdef CONFIG_SND_SAMSUNG_SPDIF
	&exynos_device_spdif,
#endif
#if defined(CONFIG_SND_SAMSUNG_RP) || defined(CONFIG_SND_SAMSUNG_ALP)
	&exynos_device_srp,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
	&exynos4_device_fimc_is,
#endif
#ifdef CONFIG_VIDEO_TVOUT
	&s5p_device_tvout,
	&s5p_device_cec,
	&s5p_device_hpd,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_TV
	&s5p_device_i2c_hdmiphy,
	&s5p_device_hdmi,
	&s5p_device_sdo,
	&s5p_device_mixer,
	&s5p_device_cec,
#endif
#if defined(CONFIG_VIDEO_FIMC)
	&s3c_device_fimc0,
	&s3c_device_fimc1,
	&s3c_device_fimc2,
	&s3c_device_fimc3,
/* CONFIG_VIDEO_SAMSUNG_S5P_FIMC is the feature for mainline */
#elif defined(CONFIG_VIDEO_SAMSUNG_S5P_FIMC)
	&s5p_device_fimc0,
	&s5p_device_fimc1,
	&s5p_device_fimc2,
	&s5p_device_fimc3,
#endif
#if defined(CONFIG_VIDEO_FIMC_MIPI)
	&s3c_device_csis0,
	&s3c_device_csis1,
#elif defined(CONFIG_VIDEO_S5P_MIPI_CSIS)
	&s5p_device_mipi_csis0,
	&s5p_device_mipi_csis1,
#endif
#ifdef CONFIG_VIDEO_S5P_MIPI_CSIS
	&mipi_csi_fixed_voltage,
#endif
#ifdef CONFIG_VIDEO_M5MOLS
	&m5mols_fixed_voltage,
#endif

#if defined(CONFIG_VIDEO_MFC5X) || defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
	&s5p_device_mfc,
#endif
#ifdef CONFIG_S5P_SYSTEM_MMU
	&SYSMMU_PLATDEV(g2d_acp),
	&SYSMMU_PLATDEV(fimc0),
	&SYSMMU_PLATDEV(fimc1),
	&SYSMMU_PLATDEV(fimc2),
	&SYSMMU_PLATDEV(fimc3),
	&SYSMMU_PLATDEV(jpeg),
	&SYSMMU_PLATDEV(mfc_l),
	&SYSMMU_PLATDEV(mfc_r),
	&SYSMMU_PLATDEV(tv),
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
	&SYSMMU_PLATDEV(is_isp),
	&SYSMMU_PLATDEV(is_drc),
	&SYSMMU_PLATDEV(is_fd),
	&SYSMMU_PLATDEV(is_cpu),
#endif
#endif /* CONFIG_S5P_SYSTEM_MMU */
#ifdef CONFIG_ION_EXYNOS
	&exynos_device_ion,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_LITE
	&exynos_device_flite0,
	&exynos_device_flite1,
#endif
#ifdef CONFIG_VIDEO_FIMG2D
	&s5p_device_fimg2d,
#endif
#ifdef CONFIG_EXYNOS_MEDIA_DEVICE
	&exynos_device_md0,
#endif
#if	defined(CONFIG_VIDEO_JPEG_V2X) || defined(CONFIG_VIDEO_JPEG)

	&s5p_device_jpeg,
#endif
//	&wm8994_fixed_voltage0,
//	&wm8994_fixed_voltage1,
//	&wm8994_fixed_voltage2,
	&samsung_asoc_dma,
	&samsung_asoc_idma,
#ifdef CONFIG_BATTERY_SAMSUNG
	&samsung_device_battery,
#endif
//	&samsung_device_keypad,
#ifdef CONFIG_EXYNOS_C2C
	&exynos_device_c2c,
#endif
	//&smdk4x12_input_device, yulu
	//&smdk4x12_smsc911x,
/*	
#ifdef CONFIG_S3C64XX_DEV_SPI
	&exynos_device_spi0,
	&exynos_device_spi1,
	&exynos_device_spi2,
#endif
*/
#ifdef CONFIG_EXYNOS_THERMAL
//	&exynos_device_tmu,
#endif
#if defined(CONFIG_DHD_1_88_10)||defined(CONFIG_BCMDHD)
	&bcm4330_bluetooth_device,
#endif
#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
	&s5p_device_tmu,
#endif
#ifdef CONFIG_S5P_DEV_ACE
	&s5p_device_ace,
#endif
	&exynos4_busfreq,
};

static struct platform_device *hsmm1_d816_devices[] __initdata = {
#ifdef CONFIG_S3C_DEV_HSMMC1
	&s3c_device_hsmmc1,
#endif
};

#ifdef CONFIG_EXYNOS_THERMAL
/* below temperature base on the celcius degree */
struct tmu_data exynos_tmu_data __initdata = {
	.ts = {
		.stop_throttle  = 82,
		.start_throttle = 85,
		.stop_warning  = 102,
		.start_warning = 105,
		.start_tripping = 110, /* temp to do tripping */
		.start_hw_tripping = 113,	/* temp to do hw_trpping*/
		.stop_mem_throttle = 80,
		.start_mem_throttle = 85,

		.stop_tc = 13,
		.start_tc = 10,
	},
	.cpulimit = {
		.throttle_freq = 800000,
		.warning_freq = 200000,
	},
	.temp_compensate = {
		.arm_volt = 925000, /* vdd_arm in uV for temperature compensation */
		.bus_volt = 900000, /* vdd_bus in uV for temperature compensation */
		.g3d_volt = 900000, /* vdd_g3d in uV for temperature compensation */
	},
	.efuse_value = 55,
	.slope = 0x10008802,
	.mode = 0,
};
#endif

#ifdef CONFIG_EXYNOS4_SETUP_THERMAL

/* below temperature base on the celcius degree */
struct s5p_platform_tmu ut_tmu_data __initdata = {
	.ts = {
#if 0
		.stop_1st_throttle  = 80,		// 78
		.start_1st_throttle = 83,		// 80
		.stop_2nd_throttle  = 90,	// 87
		.start_2nd_throttle = 103,
		.start_tripping	    = 110, /* temp to do tripping */
		.start_emergency    = 120, /* To protect chip,forcely kernel panic */
		.stop_mem_throttle  = 80,
		.start_mem_throttle = 85,
		.stop_tc  = 13,
		.start_tc = 10,
#endif
//modify by jerry
#if 1
		.stop_1st_throttle  = 61,		// 78
		.start_1st_throttle = 64,		// 80
		.stop_2nd_throttle  = 87,	// 87
		.start_2nd_throttle = 100,
		.start_tripping	    = 105, /* temp to do tripping */
		.start_emergency    = 110, /* To protect chip,forcely kernel panic */

		.stop_mem_throttle  = 75,
		.start_mem_throttle = 90,

		.stop_tc  = 13,
		.start_tc = 10,
#endif
	},
	.cpufreq = {
		.limit_1st_throttle  = 800000, /* 800MHz in KHz order */
		.limit_2nd_throttle  = 200000, /* 200MHz in KHz order */
//modify by jerry
//		.limit_1st_throttle  = 600000, /* 800MHz in KHz order */
//		.limit_2nd_throttle  = 200000, /* 200MHz in KHz order */
	},
/* TODO by Albert_lee 2014-04-25 begin */
//	.temp_compensate = {
//		.arm_volt = 925000, /* vdd_arm in uV for temperature compensation */
//		.bus_volt = 900000, /* vdd_bus in uV for temperature compensation */
//		.g3d_volt = 900000, /* vdd_g3d in uV for temperature compensation */
//	},

        .temp_compensate = {
                .arm_volt = 925000, /* vdd_arm in uV for temperature compensation */
                .bus_volt = 900000, /* vdd_bus in uV for temperature compensation */
                .g3d_volt = 900000, /* vdd_g3d in uV for temperature compensation */
        },
 /*Albert_lee 2014-04-25 end*/
};


struct s5p_platform_tmu ut_tmu_data_d721 __initdata = {
	.ts = {
		.stop_1st_throttle  = 50,		// 78
		.start_1st_throttle = 56,		// 80
		.stop_2nd_throttle  = 90,	// 87
		.start_2nd_throttle = 80,
		.start_tripping	    = 105, /* temp to do tripping */
		.start_emergency    = 110, /* To protect chip,forcely kernel panic */

		.stop_mem_throttle  = 75,
		.start_mem_throttle = 90,

		.stop_tc  = 13,
		.start_tc = 10,
	},
	.cpufreq = {
		.limit_1st_throttle  = 600000, /* 800MHz in KHz order */
		.limit_2nd_throttle  = 200000, /* 200MHz in KHz order */

	},

        .temp_compensate = {
                .arm_volt = 925000, /* vdd_arm in uV for temperature compensation */
                .bus_volt = 900000, /* vdd_bus in uV for temperature compensation */
                .g3d_volt = 900000, /* vdd_g3d in uV for temperature compensation */
        },
};

#endif

#if defined(CONFIG_VIDEO_TVOUT)
static struct s5p_platform_hpd hdmi_hpd_data __initdata = {

};
static struct s5p_platform_cec hdmi_cec_data __initdata = {

};
#endif

#ifdef CONFIG_VIDEO_EXYNOS_HDMI_CEC
static struct s5p_platform_cec hdmi_cec_data __initdata = {

};
#endif

#ifdef CONFIG_VIDEO_SAMSUNG_S5P_FIMC
static struct s5p_fimc_isp_info isp_info[] = {
#if defined(CONFIG_VIDEO_S5K4BA)
	{
		.board_info	= &s5k4ba_info,
		.clk_frequency  = 24000000UL,
		.bus_type	= FIMC_ITU_601,
#ifdef CONFIG_ITU_A
		.i2c_bus_num	= 0,
		.mux_id		= 0, /* A-Port : 0, B-Port : 1 */
#endif
#ifdef CONFIG_ITU_B
		.i2c_bus_num	= 1,
		.mux_id		= 1, /* A-Port : 0, B-Port : 1 */
#endif
		.flags		= FIMC_CLK_INV_VSYNC,
	},
#endif
#if defined(CONFIG_VIDEO_S5K4EA)
	{
		.board_info	= &s5k4ea_info,
		.clk_frequency  = 24000000UL,
		.bus_type	= FIMC_MIPI_CSI2,
#ifdef CONFIG_CSI_C
		.i2c_bus_num	= 0,
		.mux_id		= 0, /* A-Port : 0, B-Port : 1 */
#endif
#ifdef CONFIG_CSI_D
		.i2c_bus_num	= 1,
		.mux_id		= 1, /* A-Port : 0, B-Port : 1 */
#endif
		.flags		= FIMC_CLK_INV_VSYNC,
		.csi_data_align = 32,
	},
#endif
#if defined(CONFIG_VIDEO_M5MOLS)
	{
		.board_info	= &m5mols_board_info,
		.clk_frequency  = 24000000UL,
		.bus_type	= FIMC_MIPI_CSI2,
#ifdef CONFIG_CSI_C
		.i2c_bus_num	= 4,
		.mux_id		= 0, /* A-Port : 0, B-Port : 1 */
#endif
#ifdef CONFIG_CSI_D
		.i2c_bus_num	= 5,
		.mux_id		= 1, /* A-Port : 0, B-Port : 1 */
#endif
		.flags		= FIMC_CLK_INV_PCLK | FIMC_CLK_INV_VSYNC,
		.csi_data_align = 32,
	},
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
#if defined(CONFIG_VIDEO_S5K3H2)
	{
		.board_info	= &s5k3h2_sensor_info,
		.clk_frequency  = 24000000UL,
		.bus_type	= FIMC_MIPI_CSI2,
#ifdef CONFIG_S5K3H2_CSI_C
		.i2c_bus_num	= 0,
		.mux_id		= 0, /* A-Port : 0, B-Port : 1 */
		.flite_id	= FLITE_IDX_A,
		.cam_power	= smdk4x12_cam0_reset,
#endif
#ifdef CONFIG_S5K3H2_CSI_D
		.i2c_bus_num	= 1,
		.mux_id		= 1, /* A-Port : 0, B-Port : 1 */
		.flite_id	= FLITE_IDX_B,
		.cam_power	= smdk4x12_cam1_reset,
#endif
		.flags		= 0,
		.csi_data_align = 24,
		.use_isp	= true,
	},
#endif
#if defined(CONFIG_VIDEO_S5K3H7)
	{
		.board_info	= &s5k3h7_sensor_info,
		.clk_frequency  = 24000000UL,
		.bus_type	= FIMC_MIPI_CSI2,
#ifdef CONFIG_S5K3H7_CSI_C
		.i2c_bus_num	= 0,
		.mux_id		= 0, /* A-Port : 0, B-Port : 1 */
		.flite_id	= FLITE_IDX_A,
		.cam_power	= smdk4x12_cam0_reset,
#endif
#ifdef CONFIG_S5K3H7_CSI_D
		.i2c_bus_num	= 1,
		.mux_id		= 1, /* A-Port : 0, B-Port : 1 */
		.flite_id	= FLITE_IDX_B,
		.cam_power	= smdk4x12_cam1_reset,
#endif
		.csi_data_align = 24,
		.use_isp	= true,
	},
#endif
#if defined(CONFIG_VIDEO_S5K4E5)
	{
		.board_info	= &s5k4e5_sensor_info,
		.clk_frequency  = 24000000UL,
		.bus_type	= FIMC_MIPI_CSI2,
#ifdef CONFIG_S5K4E5_CSI_C
		.i2c_bus_num	= 0,
		.mux_id		= 0, /* A-Port : 0, B-Port : 1 */
		.flite_id	= FLITE_IDX_A,
		.cam_power	= smdk4x12_cam0_reset,
#endif
#ifdef CONFIG_S5K4E5_CSI_D
		.i2c_bus_num	= 1,
		.mux_id		= 1, /* A-Port : 0, B-Port : 1 */
		.flite_id	= FLITE_IDX_B,
		.cam_power	= smdk4x12_cam1_reset,
#endif
		.csi_data_align = 24,
		.use_isp	= true,
	},
#endif
#if defined(CONFIG_VIDEO_S5K6A3)
	{
		.board_info	= &s5k6a3_sensor_info,
		.clk_frequency  = 12000000UL,
		.bus_type	= FIMC_MIPI_CSI2,
#ifdef CONFIG_S5K6A3_CSI_C
		.i2c_bus_num	= 0,
		.mux_id		= 0, /* A-Port : 0, B-Port : 1 */
		.flite_id	= FLITE_IDX_A,
		.cam_power	= smdk4x12_cam0_reset,
#endif
#ifdef CONFIG_S5K6A3_CSI_D
		.i2c_bus_num	= 1,
		.mux_id		= 1, /* A-Port : 0, B-Port : 1 */
		.flite_id	= FLITE_IDX_B,
		.cam_power	= smdk4x12_cam1_reset,
#endif
		.flags		= 0,
		.csi_data_align = 12,
		.use_isp	= true,
	},
#endif
#endif
#if defined(WRITEBACK_ENABLED)
	{
		.board_info	= &writeback_info,
		.bus_type	= FIMC_LCD_WB,
		.i2c_bus_num	= 0,
		.mux_id		= 0, /* A-Port : 0, B-Port : 1 */
		.flags		= FIMC_CLK_INV_VSYNC,
	},
#endif
};

static void __init smdk4x12_subdev_config(void)
{
	s3c_fimc0_default_data.isp_info[0] = &isp_info[0];
	s3c_fimc0_default_data.isp_info[0]->use_cam = true;
	s3c_fimc0_default_data.isp_info[1] = &isp_info[1];
	s3c_fimc0_default_data.isp_info[1]->use_cam = true;
	/* support using two fimc as one sensore */
	{
		static struct s5p_fimc_isp_info camcording1;
		static struct s5p_fimc_isp_info camcording2;
		memcpy(&camcording1, &isp_info[0], sizeof(struct s5p_fimc_isp_info));
		memcpy(&camcording2, &isp_info[1], sizeof(struct s5p_fimc_isp_info));
		s3c_fimc1_default_data.isp_info[0] = &camcording1;
		s3c_fimc1_default_data.isp_info[0]->use_cam = false;
		s3c_fimc1_default_data.isp_info[1] = &camcording2;
		s3c_fimc1_default_data.isp_info[1]->use_cam = false;
	}
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
#ifdef CONFIG_VIDEO_S5K3H2
#ifdef CONFIG_S5K3H2_CSI_C
	s5p_mipi_csis0_default_data.clk_rate	= 160000000;
	s5p_mipi_csis0_default_data.lanes	= 2;
	s5p_mipi_csis0_default_data.alignment	= 24;
	s5p_mipi_csis0_default_data.hs_settle	= 12;
#endif
#ifdef CONFIG_S5K3H2_CSI_D
	s5p_mipi_csis1_default_data.clk_rate	= 160000000;
	s5p_mipi_csis1_default_data.lanes	= 2;
	s5p_mipi_csis1_default_data.alignment	= 24;
	s5p_mipi_csis1_default_data.hs_settle	= 12;
#endif
#endif
#ifdef CONFIG_VIDEO_S5K3H7
#ifdef CONFIG_S5K3H7_CSI_C
	s5p_mipi_csis0_default_data.clk_rate	= 160000000;
	s5p_mipi_csis0_default_data.lanes	= 2;
	s5p_mipi_csis0_default_data.alignment	= 24;
	s5p_mipi_csis0_default_data.hs_settle	= 12;
#endif
#ifdef CONFIG_S5K3H7_CSI_D
	s5p_mipi_csis1_default_data.clk_rate	= 160000000;
	s5p_mipi_csis1_default_data.lanes	= 2;
	s5p_mipi_csis1_default_data.alignment	= 24;
	s5p_mipi_csis1_default_data.hs_settle	= 12;
#endif
#endif
#ifdef CONFIG_VIDEO_S5K4E5
#ifdef CONFIG_S5K4E5_CSI_C
	s5p_mipi_csis0_default_data.clk_rate	= 160000000;
	s5p_mipi_csis0_default_data.lanes	= 2;
	s5p_mipi_csis0_default_data.alignment	= 24;
	s5p_mipi_csis0_default_data.hs_settle	= 12;
#endif
#ifdef CONFIG_S5K4E5_CSI_D
	s5p_mipi_csis1_default_data.clk_rate	= 160000000;
	s5p_mipi_csis1_default_data.lanes	= 2;
	s5p_mipi_csis1_default_data.alignment	= 24;
	s5p_mipi_csis1_default_data.hs_settle	= 12;
#endif
#endif
#ifdef CONFIG_VIDEO_S5K6A3
#ifdef CONFIG_S5K6A3_CSI_C
	s5p_mipi_csis0_default_data.clk_rate	= 160000000;
	s5p_mipi_csis0_default_data.lanes 	= 1;
	s5p_mipi_csis0_default_data.alignment	= 24;
	s5p_mipi_csis0_default_data.hs_settle	= 12;
#endif
#ifdef CONFIG_S5K6A3_CSI_D
	s5p_mipi_csis1_default_data.clk_rate	= 160000000;
	s5p_mipi_csis1_default_data.lanes 	= 1;
	s5p_mipi_csis1_default_data.alignment	= 24;
	s5p_mipi_csis1_default_data.hs_settle	= 12;
#endif
#endif
#endif
}
static void __init smdk4x12_camera_config(void)
{
	/* CAM A port(b0010) : PCLK, VSYNC, HREF, DATA[0-4] */
	s3c_gpio_cfgrange_nopull(EXYNOS4212_GPJ0(0), 8, S3C_GPIO_SFN(2));
	/* CAM A port(b0010) : DATA[5-7], CLKOUT(MIPI CAM also), FIELD */
	s3c_gpio_cfgrange_nopull(EXYNOS4212_GPJ1(0), 5, S3C_GPIO_SFN(2));
	/* CAM B port(b0011) : PCLK, DATA[0-6] */
	s3c_gpio_cfgrange_nopull(EXYNOS4212_GPM0(0), 8, S3C_GPIO_SFN(3));
	/* CAM B port(b0011) : FIELD, DATA[7]*/
	s3c_gpio_cfgrange_nopull(EXYNOS4212_GPM1(0), 2, S3C_GPIO_SFN(3));
	/* CAM B port(b0011) : VSYNC, HREF, CLKOUT*/
	s3c_gpio_cfgrange_nopull(EXYNOS4212_GPM2(0), 3, S3C_GPIO_SFN(3));

	/* note : driver strength to max is unnecessary */
#ifdef CONFIG_VIDEO_M5MOLS
	s3c_gpio_cfgpin(EXYNOS4_GPX2(6), S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(EXYNOS4_GPX2(6), S3C_GPIO_PULL_NONE);
#endif
}
#endif /* CONFIG_VIDEO_SAMSUNG_S5P_FIMC */

#ifdef CONFIG_VIDEO_EXYNOS_FIMC_LITE
static void __set_flite_camera_config(struct exynos_platform_flite *data,
					u32 active_index, u32 max_cam)
{
	data->active_cam_index = active_index;
	data->num_clients = max_cam;
}

static void __init smdk4x12_set_camera_flite_platdata(void)
{
	int flite0_cam_index = 0;
	int flite1_cam_index = 0;
#ifdef CONFIG_VIDEO_S5K3H2
#ifdef CONFIG_S5K3H2_CSI_C
	exynos_flite0_default_data.cam[flite0_cam_index++] = &s5k3h2;
#endif
#ifdef CONFIG_S5K3H2_CSI_D
	exynos_flite1_default_data.cam[flite1_cam_index++] = &s5k3h2;
#endif
#endif
#ifdef CONFIG_VIDEO_S5K3H7
#ifdef CONFIG_S5K3H7_CSI_C
	exynos_flite0_default_data.cam[flite0_cam_index++] = &s5k3h7;
#endif
#ifdef CONFIG_S5K3H7_CSI_D
	exynos_flite1_default_data.cam[flite1_cam_index++] = &s5k3h7;
#endif
#endif
#ifdef CONFIG_VIDEO_S5K4E5
#ifdef CONFIG_S5K4E5_CSI_C
	exynos_flite0_default_data.cam[flite0_cam_index++] = &s5k4e5;
#endif
#ifdef CONFIG_S5K4E5_CSI_D
	exynos_flite1_default_data.cam[flite1_cam_index++] = &s5k4e5;
#endif
#endif

#ifdef CONFIG_VIDEO_S5K6A3
#ifdef CONFIG_S5K6A3_CSI_C
	exynos_flite0_default_data.cam[flite0_cam_index++] = &s5k6a3;
#endif
#ifdef CONFIG_S5K6A3_CSI_D
	exynos_flite1_default_data.cam[flite1_cam_index++] = &s5k6a3;
#endif
#endif
	__set_flite_camera_config(&exynos_flite0_default_data, 0, flite0_cam_index);
	__set_flite_camera_config(&exynos_flite1_default_data, 0, flite1_cam_index);
}
#endif

#if defined(CONFIG_CMA)
static void __init exynos4_reserve_mem(void)
{
	static struct cma_region regions[] = {
#ifndef CONFIG_VIDEOBUF2_ION
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_TV
		{
			.name = "tv",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_TV * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_JPEG
		{
			.name = "jpeg",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_JPEG * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_AUDIO_SAMSUNG_MEMSIZE_SRP
		{
			.name = "srp",
			.size = CONFIG_AUDIO_SAMSUNG_MEMSIZE_SRP * SZ_1K,
			.start = 0,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMG2D
		{
			.name = "fimg2d",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMG2D * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD
		{
			.name = "fimd",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0
		{
			.name = "fimc0",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0 * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2
		{
			.name = "fimc2",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2 * SZ_1K,
			.start = 0
		},
#endif
#if !defined(CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION) && \
	defined(CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC3)
		{
			.name = "fimc3",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC3 * SZ_1K,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1
		{
			.name = "fimc1",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1 * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC1
		{
#ifdef CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION
			.name = "mfc-normal",
#else
			.name = "mfc1",
#endif
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC1 * SZ_1K,
			{ .alignment = 1 << 17 },
		},
#endif
#if !defined(CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION) && \
	defined(CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0)
		{
			.name = "mfc0",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0 * SZ_1K,
			{ .alignment = 1 << 17 },
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC
		{
			.name = "mfc",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC * SZ_1K,
			{ .alignment = 1 << 17 },
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0
		{
			.name = "fimc0",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0 * SZ_1K,
			.start = 0
		},
#endif

#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
		{
			.name = "fimc_is",
			.size = CONFIG_VIDEO_EXYNOS_MEMSIZE_FIMC_IS * SZ_1K,
			{
				.alignment = 1 << 26,
			},
			.start = 0
		},
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS_BAYER
		{
			.name = "fimc_is_isp",
			.size = CONFIG_VIDEO_EXYNOS_MEMSIZE_FIMC_IS_ISP * SZ_1K,
			.start = 0
		},
#endif
#endif
#if !defined(CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION) && \
	defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
		{
			.name		= "b2",
			.size		= 32 << 20,
			{ .alignment	= 128 << 10 },
		},
		{
			.name		= "b1",
			.size		= 32 << 20,
			{ .alignment	= 128 << 10 },
		},
		{
			.name		= "fw",
			.size		= 1 << 20,
			{ .alignment	= 128 << 10 },
		},
#endif
#else /* !CONFIG_VIDEOBUF2_ION */
#ifdef CONFIG_FB_S5P
#error CONFIG_FB_S5P is defined. Select CONFIG_FB_S3C, instead
#endif
		{
			.name	= "ion",
			.size	= CONFIG_ION_EXYNOS_CONTIGHEAP_SIZE * SZ_1K,
		},
#endif /* !CONFIG_VIDEOBUF2_ION */
		{
			.size = 0
		},
	};
#ifdef CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION
	static struct cma_region regions_secure[] = {
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD_VIDEO
		{
			.name = "video",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD_VIDEO * SZ_1K,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC3
		{
			.name = "fimc3",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC3 * SZ_1K,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0
		{
			.name = "mfc-secure",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0 * SZ_1K,
		},
#endif
		{
			.name = "sectbl",
			.size = SZ_1M,
			{
				.alignment = SZ_64M,
			},
		},
		{
			.size = 0
		},
	};
#else /* !CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION */
	struct cma_region *regions_secure = NULL;
#endif
	static const char map[] __initconst =

		"android_pmem.0=pmem;android_pmem.1=pmem_gpu1;"
		"s3cfb.0/fimd=fimd;exynos4-fb.0/fimd=fimd;"
#ifdef CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION
		"s3cfb.0/video=video;exynos4-fb.0/video=video;"
#endif
		"s3c-fimc.0=fimc0;s3c-fimc.1=fimc1;s3c-fimc.2=fimc2;s3c-fimc.3=fimc3;"
		"exynos4210-fimc.0=fimc0;exynos4210-fimc.1=fimc1;exynos4210-fimc.2=fimc2;exynos4210-fimc.3=fimc3;"
#ifdef CONFIG_VIDEO_MFC5X
		"s3c-mfc/A=mfc0,mfc-secure;"
		"s3c-mfc/B=mfc1,mfc-normal;"
		"s3c-mfc/AB=mfc;"
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_S5P_MFC
		"s5p-mfc/f=fw;"
		"s5p-mfc/a=b1;"
		"s5p-mfc/b=b2;"
#endif
		"samsung-rp=srp;"
		"s5p-jpeg=jpeg;"
		"exynos4-fimc-is/f=fimc_is;"
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS_BAYER
		"exynos4-fimc-is/i=fimc_is_isp;"
#endif
		"s5p-mixer=tv;"
		"s5p-fimg2d=fimg2d;"
		"ion-exynos=ion,fimd,fimc0,fimc1,fimc2,fimc3,fw,b1,b2;"
#ifdef CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION
		"s5p-smem/video=video;"
		"s5p-smem/sectbl=sectbl;"
#endif
		"s5p-smem/mfc=mfc0,mfc-secure;"
		"s5p-smem/fimc=fimc3;"
		"s5p-smem/mfc-shm=mfc1,mfc-normal;"
		"s5p-smem/fimd=fimd;";

	s5p_cma_region_reserve(regions, regions_secure, 0, map);
}
#else
static inline void exynos4_reserve_mem(void)
{
}
#endif /* CONFIG_CMA */
#ifdef CONFIG_BACKLIGHT_PWM /*macroliu*/
/* LCD Backlight data */
static struct samsung_bl_gpio_info smdk4x12_bl_gpio_info = {
	.no = EXYNOS4_GPD0(1),
	.func = S3C_GPIO_SFN(2),
};

static struct platform_pwm_backlight_data smdk4x12_bl_data = {
	.pwm_id = 1,
};
#endif
static void __init smdk4x12_map_io(void)
{
	clk_xusbxti.rate = 24000000;
	s5p_init_io(NULL, 0, S5P_VA_CHIPID);
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(smdk4x12_uartcfgs, ARRAY_SIZE(smdk4x12_uartcfgs));

	exynos4_reserve_mem();
}

static void __init exynos_sysmmu_init(void)
{
	ASSIGN_SYSMMU_POWERDOMAIN(fimc0, &exynos4_device_pd[PD_CAM].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(fimc1, &exynos4_device_pd[PD_CAM].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(fimc2, &exynos4_device_pd[PD_CAM].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(fimc3, &exynos4_device_pd[PD_CAM].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(jpeg, &exynos4_device_pd[PD_CAM].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(mfc_l, &exynos4_device_pd[PD_MFC].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(mfc_r, &exynos4_device_pd[PD_MFC].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(tv, &exynos4_device_pd[PD_TV].dev);
#ifdef CONFIG_VIDEO_FIMG2D
	sysmmu_set_owner(&SYSMMU_PLATDEV(g2d_acp).dev, &s5p_device_fimg2d.dev);
#endif
#if defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC) || defined(CONFIG_VIDEO_MFC5X)
	sysmmu_set_owner(&SYSMMU_PLATDEV(mfc_l).dev, &s5p_device_mfc.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(mfc_r).dev, &s5p_device_mfc.dev);
#endif
#if defined(CONFIG_VIDEO_FIMC)
	sysmmu_set_owner(&SYSMMU_PLATDEV(fimc0).dev, &s3c_device_fimc0.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(fimc1).dev, &s3c_device_fimc1.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(fimc2).dev, &s3c_device_fimc2.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(fimc3).dev, &s3c_device_fimc3.dev);
#elif defined(CONFIG_VIDEO_SAMSUNG_S5P_FIMC)
	sysmmu_set_owner(&SYSMMU_PLATDEV(fimc0).dev, &s5p_device_fimc0.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(fimc1).dev, &s5p_device_fimc1.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(fimc2).dev, &s5p_device_fimc2.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(fimc3).dev, &s5p_device_fimc3.dev);
#endif
#ifdef CONFIG_VIDEO_EXYNOS_TV
	sysmmu_set_owner(&SYSMMU_PLATDEV(tv).dev, &s5p_device_mixer.dev);
#endif
#ifdef CONFIG_VIDEO_TVOUT
	sysmmu_set_owner(&SYSMMU_PLATDEV(tv).dev, &s5p_device_tvout.dev);
#endif
#ifdef CONFIG_VIDEO_JPEG_V2X
	sysmmu_set_owner(&SYSMMU_PLATDEV(jpeg).dev, &s5p_device_jpeg.dev);
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
	ASSIGN_SYSMMU_POWERDOMAIN(is_isp, &exynos4_device_pd[PD_ISP].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(is_drc, &exynos4_device_pd[PD_ISP].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(is_fd, &exynos4_device_pd[PD_ISP].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(is_cpu, &exynos4_device_pd[PD_ISP].dev);

	sysmmu_set_owner(&SYSMMU_PLATDEV(is_isp).dev,
						&exynos4_device_fimc_is.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(is_drc).dev,
						&exynos4_device_fimc_is.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(is_fd).dev,
						&exynos4_device_fimc_is.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(is_cpu).dev,
						&exynos4_device_fimc_is.dev);
#endif
}


static void __init smdk4x12_machine_init(void)
{

#if defined(CONFIG_EXYNOS_DEV_PD) && defined(CONFIG_PM_RUNTIME)
	exynos_pd_disable(&exynos4_device_pd[PD_MFC].dev);
	exynos_pd_disable(&exynos4_device_pd[PD_G3D].dev);
	exynos_pd_disable(&exynos4_device_pd[PD_LCD0].dev);
	exynos_pd_disable(&exynos4_device_pd[PD_CAM].dev);
	exynos_pd_disable(&exynos4_device_pd[PD_TV].dev);
	exynos_pd_disable(&exynos4_device_pd[PD_GPS].dev);
	exynos_pd_disable(&exynos4_device_pd[PD_GPS_ALIVE].dev);
	exynos_pd_disable(&exynos4_device_pd[PD_ISP].dev);
#elif defined(CONFIG_EXYNOS_DEV_PD)
	/*
	 * These power domains should be always on
	 * without runtime pm support.
	 */
	exynos_pd_enable(&exynos4_device_pd[PD_MFC].dev);
	exynos_pd_enable(&exynos4_device_pd[PD_G3D].dev);
	exynos_pd_enable(&exynos4_device_pd[PD_LCD0].dev);
	exynos_pd_enable(&exynos4_device_pd[PD_CAM].dev);
	exynos_pd_enable(&exynos4_device_pd[PD_TV].dev);
	exynos_pd_enable(&exynos4_device_pd[PD_GPS].dev);
	exynos_pd_enable(&exynos4_device_pd[PD_GPS_ALIVE].dev);
	exynos_pd_enable(&exynos4_device_pd[PD_ISP].dev);
#endif

	s3c_gpio_cfgpin(EXYNOS4212_GPM2(2), S3C_GPIO_SFN(3));// bf3a20 CLK
	s3c_gpio_cfgpin(EXYNOS4212_GPJ1(3), S3C_GPIO_SFN(2));// OV5645 CLK



	/* for d721 boot LOGO */
	if(CHECK_UTMODEL("d721")){
	//	gpio_direction_output(EXYNOS4_GPD0(1),0);
	//	s3c_gpio_cfgpin(EXYNOS4_GPD0(1), S3C_GPIO_SFN(3));
		gpio_direction_output(EXYNOS4212_GPM4(0),0);
		s3c_gpio_cfgpin(EXYNOS4212_GPM4(0), S3C_GPIO_SFN(1));

		gpio_direction_output(EXYNOS4212_GPM2(3),0);
		s3c_gpio_cfgpin(EXYNOS4212_GPM2(3), S3C_GPIO_SFN(1));

		gpio_direction_output(EXYNOS4212_GPM2(0),1);
		s3c_gpio_cfgpin(EXYNOS4212_GPM2(0), S3C_GPIO_SFN(1));

		gpio_direction_output(EXYNOS4212_GPM2(4),0);
		s3c_gpio_cfgpin(EXYNOS4212_GPM2(4), S3C_GPIO_SFN(1));
	}

	

// add by albert TODO start
	/*
	  * prevent 4x12 ISP power off problem
	  * ISP_SYS Register has to be 0 before ISP block power off.
	  */
	__raw_writel(0x0, S5P_CMU_RESET_ISP_SYS);

	/* initialise the gpios */
	ut4x12_config_gpio_table();
	exynos4_sleep_gpio_table_set = ut4x12_config_sleep_gpio_table;
// add by albert TODO end

	exynos4_smdk4x12_power_init();

	s3c_i2c0_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));

	s3c_i2c1_set_platdata(NULL);
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));

	s3c_i2c2_set_platdata(NULL);
	i2c_register_board_info(2, i2c_devs2, ARRAY_SIZE(i2c_devs2));
#if (defined CONFIG_BCM2079X_I2C) || (defined CONFIG_BCM2079X_I2C_MODULE)
	if(!strcmp(g_selected_wifi,"gb86441"))
		i2c_register_board_info(2, i2c_devs2_ap6441, ARRAY_SIZE(i2c_devs2_ap6441));
	else
		i2c_register_board_info(2, i2c_devs2_ap6493, ARRAY_SIZE(i2c_devs2_ap6493));
#endif

	s3c_i2c3_set_platdata(NULL);
	i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3));

	s3c_i2c4_set_platdata(NULL);

	if(!strcmp(g_selected_codec,"rt5621")){
		i2c_register_board_info(4, i2c_devs4_rt5621, ARRAY_SIZE(i2c_devs4_rt5621));
	} else if(!strcmp(g_selected_codec,"wm8978")){
		i2c_register_board_info(4, i2c_devs4_wm8978, ARRAY_SIZE(i2c_devs4_wm8978));
	}else if(!strcmp(g_selected_codec,"ac100")){
		i2c_register_board_info(4, i2c_devs4_ac100, ARRAY_SIZE(i2c_devs4_ac100));
	}
	if(CHECK_UTMODEL("d1011"))
		i2c_register_board_info(4, i2c_devs4_bh1721, ARRAY_SIZE(i2c_devs4_bh1721));

	s3c_i2c5_set_platdata(NULL);
	i2c_register_board_info(5, i2c_devs5, ARRAY_SIZE(i2c_devs5));

#if 1 // i2c6
	s3c_i2c6_set_platdata(NULL);
	i2c_register_board_info(6, i2c_devs6, ARRAY_SIZE(i2c_devs6));
#endif

	s3c_i2c7_set_platdata(NULL);

	i2c_register_board_info(7, i2c_devs7, ARRAY_SIZE(i2c_devs7));

#ifdef CONFIG_ANDROID_PMEM
	android_pmem_set_platdata();
#endif
#if defined(CONFIG_FB_S5P_MIPI_DSIM)
	if(!strcmp("s5hd", s_selected_lcd_name))
		mipi_fb_init();
#endif
//#ifdef CONFIG_FB_S3C
//	dev_set_name(&s5p_device_fimd0.dev, "s3cfb.0");
//	clk_add_alias("lcd", "exynos4-fb.0", "lcd", &s5p_device_fimd0.dev);
//	clk_add_alias("sclk_fimd", "exynos4-fb.0", "sclk_fimd", &s5p_device_fimd0.dev);
//	s5p_fb_setname(0, "exynos4-fb");
//
//	s5p_fimd0_set_platdata(&smdk4x12_lcd0_pdata);
//#ifdef CONFIG_FB_MIPI_DSIM
//	s5p_device_mipi_dsim.dev.parent = &exynos4_device_pd[PD_LCD0].dev;
//#endif
//#ifdef CONFIG_EXYNOS_DEV_PD
//	s5p_device_fimd0.dev.parent = &exynos4_device_pd[PD_LCD0].dev;
//#endif
//#endif
#ifdef CONFIG_FB_S5P
	s3cfb_set_platdata(NULL);
#ifdef CONFIG_FB_S5P_MIPI_DSIM
	s5p_device_dsim.dev.parent = &exynos4_device_pd[PD_LCD0].dev;
#endif
#ifdef CONFIG_EXYNOS_DEV_PD
	s3c_device_fb.dev.parent = &exynos4_device_pd[PD_LCD0].dev;
#endif
#endif
#ifdef CONFIG_USB_EHCI_S5P
	smdk4x12_ehci_init();
#endif
#ifdef CONFIG_USB_OHCI_S5P
	smdk4x12_ohci_init();
#endif
#ifdef CONFIG_USB_GADGET
	smdk4x12_usbgadget_init();
#endif
#ifdef CONFIG_USB_EXYNOS_SWITCH
	smdk4x12_usbswitch_init();
#endif

#ifdef CONFIG_BACKLIGHT_PWM
	samsung_bl_set(&smdk4x12_bl_gpio_info, &smdk4x12_bl_data);
#endif

#ifdef CONFIG_EXYNOS4_DEV_DWMCI
	if (strstr(g_selected_utmodel, "d816")||CHECK_TABLET("d720","1")) {
		exynos_dwmci_pdata.caps &= ~(MMC_CAP_8_BIT_DATA);
		exynos_dwmci_pdata.caps |= (MMC_CAP_4_BIT_DATA);
	}

	exynos_dwmci_set_platdata(&exynos_dwmci_pdata);
#endif

#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
	exynos4_fimc_is_set_platdata(NULL);
#ifdef CONFIG_EXYNOS_DEV_PD
	exynos4_device_fimc_is.dev.parent = &exynos4_device_pd[PD_ISP].dev;
#endif
#endif
#ifdef CONFIG_S3C_DEV_HSMMC
	s3c_sdhci0_set_platdata(&smdk4x12_hsmmc0_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	if (strstr(g_selected_utmodel, "d816")) {
		s3c_sdhci1_set_platdata(&smdk4x12_hsmmc1_pdata_d816);
	}else if(CHECK_TABLET("d720","1")){
		s3c_sdhci1_set_platdata(&smdk4x12_hsmmc1_pdata_d720);
	}

#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	s3c_sdhci2_set_platdata(&smdk4x12_hsmmc2_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	if(is_bcm_combo()) {
		gpio_direction_output(GPIO_COMBO_LDO, GPIO_LEVEL_HIGH);
		s3c_sdhci3_set_platdata(&smdk4x12_bcm4330_hsmmc3_pdata);
	} else {
		s3c_sdhci3_set_platdata(&smdk4x12_hsmmc3_pdata);
	}
#endif
#ifdef CONFIG_S5P_DEV_MSHC
	s3c_mshci_set_platdata(&exynos4_mshc_pdata);
#endif
#if defined(CONFIG_VIDEO_EXYNOS_TV) && defined(CONFIG_VIDEO_EXYNOS_HDMI)
	dev_set_name(&s5p_device_hdmi.dev, "exynos4-hdmi");
	clk_add_alias("hdmi", "s5p-hdmi", "hdmi", &s5p_device_hdmi.dev);
	clk_add_alias("hdmiphy", "s5p-hdmi", "hdmiphy", &s5p_device_hdmi.dev);

	s5p_tv_setup();

	/* setup dependencies between TV devices */
	s5p_device_hdmi.dev.parent = &exynos4_device_pd[PD_TV].dev;
	s5p_device_mixer.dev.parent = &exynos4_device_pd[PD_TV].dev;

	s5p_i2c_hdmiphy_set_platdata(NULL);
#ifdef CONFIG_VIDEO_EXYNOS_HDMI_CEC
	s5p_hdmi_cec_set_platdata(&hdmi_cec_data);
#endif
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_LITE
	smdk4x12_set_camera_flite_platdata();
	s3c_set_platdata(&exynos_flite0_default_data,
			sizeof(exynos_flite0_default_data), &exynos_device_flite0);
	s3c_set_platdata(&exynos_flite1_default_data,
			sizeof(exynos_flite1_default_data), &exynos_device_flite1);
#ifdef CONFIG_EXYNOS_DEV_PD
	exynos_device_flite0.dev.parent = &exynos4_device_pd[PD_ISP].dev;
	exynos_device_flite1.dev.parent = &exynos4_device_pd[PD_ISP].dev;
#endif
#endif
#ifdef CONFIG_EXYNOS_THERMAL
	exynos_tmu_set_platdata(&exynos_tmu_data);
#endif
#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
	s5p_tmu_set_platdata(&ut_tmu_data);
#endif
#ifdef CONFIG_VIDEO_FIMC
	s3c_fimc0_set_platdata(&fimc_plat);
	s3c_fimc1_set_platdata(&fimc_plat);
        s3c_fimc2_set_platdata(NULL);
	s3c_fimc3_set_platdata(NULL);
#ifdef CONFIG_EXYNOS_DEV_PD
	s3c_device_fimc0.dev.parent = &exynos4_device_pd[PD_CAM].dev;
	s3c_device_fimc1.dev.parent = &exynos4_device_pd[PD_CAM].dev;
	s3c_device_fimc2.dev.parent = &exynos4_device_pd[PD_CAM].dev;
	s3c_device_fimc3.dev.parent = &exynos4_device_pd[PD_CAM].dev;
#ifdef CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION
	secmem.parent = &exynos4_device_pd[PD_CAM].dev;
#endif
#endif
#ifdef CONFIG_VIDEO_FIMC_MIPI
	s3c_csis0_set_platdata(NULL);
	s3c_csis1_set_platdata(NULL);
#ifdef CONFIG_EXYNOS_DEV_PD
	s3c_device_csis0.dev.parent = &exynos4_device_pd[PD_CAM].dev;
	s3c_device_csis1.dev.parent = &exynos4_device_pd[PD_CAM].dev;
#endif
#endif

#if defined(CONFIG_ITU_A) || defined(CONFIG_CSI_C) \
	|| defined(CONFIG_S5K3H1_CSI_C) || defined(CONFIG_S5K3H2_CSI_C) \
	|| defined(CONFIG_S5K6A3_CSI_C)
	smdk4x12_cam0_reset(1);
#endif
#if defined(CONFIG_ITU_B) || defined(CONFIG_CSI_D) \
	|| defined(CONFIG_S5K3H1_CSI_D) || defined(CONFIG_S5K3H2_CSI_D) \
	|| defined(CONFIG_S5K6A3_CSI_D)
	smdk4x12_cam1_reset(1);
#endif
#endif /* CONFIG_VIDEO_FIMC */

#ifdef CONFIG_VIDEO_SAMSUNG_S5P_FIMC
	smdk4x12_camera_config();
	smdk4x12_subdev_config();

	dev_set_name(&s5p_device_fimc0.dev, "s3c-fimc.0");
	dev_set_name(&s5p_device_fimc1.dev, "s3c-fimc.1");
	dev_set_name(&s5p_device_fimc2.dev, "s3c-fimc.2");
	dev_set_name(&s5p_device_fimc3.dev, "s3c-fimc.3");

	clk_add_alias("fimc", "exynos4210-fimc.0", "fimc", &s5p_device_fimc0.dev);
	clk_add_alias("sclk_fimc", "exynos4210-fimc.0", "sclk_fimc",
			&s5p_device_fimc0.dev);
	clk_add_alias("fimc", "exynos4210-fimc.1", "fimc", &s5p_device_fimc1.dev);
	clk_add_alias("sclk_fimc", "exynos4210-fimc.1", "sclk_fimc",
			&s5p_device_fimc1.dev);
	clk_add_alias("fimc", "exynos4210-fimc.2", "fimc", &s5p_device_fimc2.dev);
	clk_add_alias("sclk_fimc", "exynos4210-fimc.2", "sclk_fimc",
			&s5p_device_fimc2.dev);
	clk_add_alias("fimc", "exynos4210-fimc.3", "fimc", &s5p_device_fimc3.dev);
	clk_add_alias("sclk_fimc", "exynos4210-fimc.3", "sclk_fimc",
			&s5p_device_fimc3.dev);

	s3c_fimc_setname(0, "exynos4210-fimc");
	s3c_fimc_setname(1, "exynos4210-fimc");
	s3c_fimc_setname(2, "exynos4210-fimc");
	s3c_fimc_setname(3, "exynos4210-fimc");
	/* FIMC */
	s3c_set_platdata(&s3c_fimc0_default_data,
			 sizeof(s3c_fimc0_default_data), &s5p_device_fimc0);
	s3c_set_platdata(&s3c_fimc1_default_data,
			 sizeof(s3c_fimc1_default_data), &s5p_device_fimc1);
	s3c_set_platdata(&s3c_fimc2_default_data,
			 sizeof(s3c_fimc2_default_data), &s5p_device_fimc2);
	s3c_set_platdata(&s3c_fimc3_default_data,
			 sizeof(s3c_fimc3_default_data), &s5p_device_fimc3);
#ifdef CONFIG_EXYNOS_DEV_PD
	s5p_device_fimc0.dev.parent = &exynos4_device_pd[PD_CAM].dev;
	s5p_device_fimc1.dev.parent = &exynos4_device_pd[PD_CAM].dev;
	s5p_device_fimc2.dev.parent = &exynos4_device_pd[PD_CAM].dev;
	s5p_device_fimc3.dev.parent = &exynos4_device_pd[PD_CAM].dev;
#endif
#ifdef CONFIG_VIDEO_S5P_MIPI_CSIS
	dev_set_name(&s5p_device_mipi_csis0.dev, "s3c-csis.0");
	dev_set_name(&s5p_device_mipi_csis1.dev, "s3c-csis.1");
	clk_add_alias("csis", "s5p-mipi-csis.0", "csis",
			&s5p_device_mipi_csis0.dev);
	clk_add_alias("sclk_csis", "s5p-mipi-csis.0", "sclk_csis",
			&s5p_device_mipi_csis0.dev);
	clk_add_alias("csis", "s5p-mipi-csis.1", "csis",
			&s5p_device_mipi_csis1.dev);
	clk_add_alias("sclk_csis", "s5p-mipi-csis.1", "sclk_csis",
			&s5p_device_mipi_csis1.dev);
	dev_set_name(&s5p_device_mipi_csis0.dev, "s5p-mipi-csis.0");
	dev_set_name(&s5p_device_mipi_csis1.dev, "s5p-mipi-csis.1");

	s3c_set_platdata(&s5p_mipi_csis0_default_data,
			sizeof(s5p_mipi_csis0_default_data), &s5p_device_mipi_csis0);
	s3c_set_platdata(&s5p_mipi_csis1_default_data,
			sizeof(s5p_mipi_csis1_default_data), &s5p_device_mipi_csis1);
#ifdef CONFIG_EXYNOS_DEV_PD
	s5p_device_mipi_csis0.dev.parent = &exynos4_device_pd[PD_CAM].dev;
	s5p_device_mipi_csis1.dev.parent = &exynos4_device_pd[PD_CAM].dev;
#endif
#endif
#if defined(CONFIG_ITU_A) || defined(CONFIG_CSI_C) \
	|| defined(CONFIG_S5K3H1_CSI_C) || defined(CONFIG_S5K3H2_CSI_C) \
	|| defined(CONFIG_S5K6A3_CSI_C)
	smdk4x12_cam0_reset(1);
#endif
#if defined(CONFIG_ITU_B) || defined(CONFIG_CSI_D) \
	|| defined(CONFIG_S5K3H1_CSI_D) || defined(CONFIG_S5K3H2_CSI_D) \
	|| defined(CONFIG_S5K6A3_CSI_D)
	smdk4x12_cam1_reset(1);
#endif
#endif

#if defined(CONFIG_VIDEO_TVOUT)
	s5p_hdmi_hpd_set_platdata(&hdmi_hpd_data);
	s5p_hdmi_cec_set_platdata(&hdmi_cec_data);
#ifdef CONFIG_EXYNOS_DEV_PD
	s5p_device_tvout.dev.parent = &exynos4_device_pd[PD_TV].dev;
	exynos4_device_pd[PD_TV].dev.parent = &exynos4_device_pd[PD_LCD0].dev;
#endif
#endif

#if	defined(CONFIG_VIDEO_JPEG_V2X) || defined(CONFIG_VIDEO_JPEG)
#ifdef CONFIG_EXYNOS_DEV_PD
	s5p_device_jpeg.dev.parent = &exynos4_device_pd[PD_CAM].dev;
	exynos4_jpeg_setup_clock(&s5p_device_jpeg.dev, 160000000);
#endif
#endif

#ifdef CONFIG_ION_EXYNOS
	exynos_ion_set_platdata();
#endif

#if defined(CONFIG_VIDEO_MFC5X) || defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
#ifdef CONFIG_EXYNOS_DEV_PD
	s5p_device_mfc.dev.parent = &exynos4_device_pd[PD_MFC].dev;
#endif
	if (soc_is_exynos4412() && samsung_rev() >= EXYNOS4412_REV_2_0)
		exynos4_mfc_setup_clock(&s5p_device_mfc.dev, 220 * MHZ);
	else if ((soc_is_exynos4412() && samsung_rev() >= EXYNOS4412_REV_1_0) ||
		(soc_is_exynos4212() && samsung_rev() >= EXYNOS4212_REV_1_0))
		exynos4_mfc_setup_clock(&s5p_device_mfc.dev, 200 * MHZ);
	else
		exynos4_mfc_setup_clock(&s5p_device_mfc.dev, 267 * MHZ);
#endif

#if defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
	dev_set_name(&s5p_device_mfc.dev, "s3c-mfc");
	clk_add_alias("mfc", "s5p-mfc", "mfc", &s5p_device_mfc.dev);
	s5p_mfc_setname(&s5p_device_mfc, "s5p-mfc");
#endif

#ifdef CONFIG_VIDEO_FIMG2D
	s5p_fimg2d_set_platdata(&fimg2d_data);
#endif

#if defined(CONFIG_KEYBOARD_SAMSUNG)
	samsung_keypad_set_platdata(&smdk4x12_keypad_data);
#endif

//	smdk4x12_smsc911x_init();
#ifdef CONFIG_EXYNOS_C2C
	exynos_c2c_set_platdata(&smdk4x12_c2c_pdata);
#endif

	exynos_sysmmu_init();

	platform_add_devices(smdk4x12_devices, ARRAY_SIZE(smdk4x12_devices));
#ifdef CONFIG_S3C_DEV_HSMMC1
	if (strstr(g_selected_utmodel, "d816")||CHECK_TABLET("d720","1")) {
		platform_add_devices(hsmm1_d816_devices, ARRAY_SIZE(hsmm1_d816_devices));
	}
#endif	
	if (soc_is_exynos4412())
		platform_add_devices(smdk4412_devices, ARRAY_SIZE(smdk4412_devices));

#ifdef CONFIG_S3C64XX_DEV_SPI
	if(!strcmp(g_selected_utmodel,"d722")){
		platform_device_register(&exynos_device_spi1);
	}else{
	/*	platform_device_register(&exynos_device_spi0);
		platform_device_register(&exynos_device_spi1);
		platform_device_register(&exynos_device_spi2); */
	}
#endif

	if(!strcmp(g_selected_utmodel,"d722"))
		platform_device_register(&samsung_device_keypad);
//#ifdef CONFIG_FB_S3C
//	exynos4_fimd0_setup_clock(&s5p_device_fimd0.dev, "mout_mpll_user",
//				800 * MHZ);
//#endif
#ifdef CONFIG_S3C64XX_DEV_SPI
int gpio;
//struct device *spi1_dev = &exynos_device_spi1.dev;

//	struct clk *sclk=NULL;
//	struct clk *prnt=NULL;
/*
	sclk = clk_get(spi0_dev, "dout_spi0");
	if (IS_ERR(sclk))
		dev_err(spi0_dev, "failed to get sclk for SPI-0\n");
	prnt = clk_get(spi0_dev, "mout_mpll_user");
	if (IS_ERR(prnt))
		dev_err(spi0_dev, "failed to get prnt\n");
	if (clk_set_parent(sclk, prnt))
		printk(KERN_ERR "Unable to set parent %s of clock %s.\n",
				prnt->name, sclk->name);

	clk_set_rate(sclk, 800 * 1000 * 1000);
	clk_put(sclk);
	clk_put(prnt);

	if (!gpio_request(EXYNOS4_GPB(1), "SPI_CS0")) {
		gpio_direction_output(EXYNOS4_GPB(1), 1);
		s3c_gpio_cfgpin(EXYNOS4_GPB(1), S3C_GPIO_SFN(1));
		s3c_gpio_setpull(EXYNOS4_GPB(1), S3C_GPIO_PULL_UP);
		exynos_spi_set_info(0, EXYNOS_SPI_SRCCLK_SCLK,
			ARRAY_SIZE(spi0_csi));
	}

	spi_register_board_info(spi0_board_info, ARRAY_SIZE(spi0_board_info));
*/
	if(!strcmp(g_selected_utmodel,"d722")){
	/*	sclk = clk_get(spi1_dev, "dout_spi1");
		if (IS_ERR(sclk))
			dev_err(spi1_dev, "failed to get sclk for SPI-1\n");
		prnt = clk_get(spi1_dev, "mout_mpll_user");
		if (IS_ERR(prnt))
			dev_err(spi1_dev, "failed to get prnt\n");
		if (clk_set_parent(sclk, prnt))
			printk(KERN_ERR "Unable to set parent %s of clock %s.\n",
					prnt->name, sclk->name);

		clk_set_rate(sclk, 800 * 1000 * 1000);
		clk_put(sclk);
		clk_put(prnt);
*/

		if (!gpio_request(EXYNOS4_GPB(5), "SPI_CS1")) {
			gpio_direction_output(EXYNOS4_GPB(5), 1);
			s3c_gpio_cfgpin(EXYNOS4_GPB(5), S3C_GPIO_SFN(1));
			s3c_gpio_setpull(EXYNOS4_GPB(5), S3C_GPIO_PULL_UP);
			exynos_spi_set_info(1, EXYNOS_SPI_SRCCLK_SCLK,
				ARRAY_SIZE(spi1_csi));
		}

		for (gpio = EXYNOS4_GPB(4); gpio < EXYNOS4_GPB(8); gpio++)
			s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV3);

		spi_register_board_info(spi1_board_info, ARRAY_SIZE(spi1_board_info));
	}
/*	sclk = clk_get(spi2_dev, "dout_spi2");
	if (IS_ERR(sclk))
		dev_err(spi2_dev, "failed to get sclk for SPI-2\n");
	prnt = clk_get(spi2_dev, "mout_mpll_user");
	if (IS_ERR(prnt))
		dev_err(spi2_dev, "failed to get prnt\n");
	if (clk_set_parent(sclk, prnt))
		printk(KERN_ERR "Unable to set parent %s of clock %s.\n",
				prnt->name, sclk->name);

	clk_set_rate(sclk, 800 * 1000 * 1000);
	clk_put(sclk);
	clk_put(prnt);

	if (!gpio_request(EXYNOS4_GPC1(2), "SPI_CS2")) {
		gpio_direction_output(EXYNOS4_GPC1(2), 1);
		s3c_gpio_cfgpin(EXYNOS4_GPC1(2), S3C_GPIO_SFN(1));
		s3c_gpio_setpull(EXYNOS4_GPC1(2), S3C_GPIO_PULL_UP);
		exynos_spi_set_info(2, EXYNOS_SPI_SRCCLK_SCLK,
			ARRAY_SIZE(spi2_csi));
	}

	spi_register_board_info(spi2_board_info, ARRAY_SIZE(spi2_board_info));
*/
#endif
#ifdef CONFIG_BUSFREQ_OPP
	dev_add(&busfreq, &exynos4_busfreq.dev);
	ppmu_init(&exynos_ppmu[PPMU_DMC0], &exynos4_busfreq.dev);
	ppmu_init(&exynos_ppmu[PPMU_DMC1], &exynos4_busfreq.dev);
	ppmu_init(&exynos_ppmu[PPMU_CPU], &exynos4_busfreq.dev);
#endif
//	register_reboot_notifier(&exynos4_reboot_notifier);
}

MACHINE_START(SMDK4212, "UT4412")
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.init_irq	= exynos4_init_irq,
	.map_io		= smdk4x12_map_io,
	.init_machine	= smdk4x12_machine_init,
	.timer		= &exynos4_timer,
	//.fixup		= exynos4x12_fixup,	//urbetter

MACHINE_END

MACHINE_START(SMDK4412, "UT4412")
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.init_irq	= exynos4_init_irq,
	.map_io		= smdk4x12_map_io,
	.init_machine	= smdk4x12_machine_init,
	.timer		= &exynos4_timer,

MACHINE_END
