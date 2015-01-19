#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>
#include <asm/io.h>
#include <asm/sizes.h>
#include <asm/mach-types.h>

#include "../../../fs/proc/internal.h"

#include <plat/gpio-cfg.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include "urbetter/power_gpio.h"

#include "power_gpio_common.h"
#include <urbetter/check.h>


static const struct power_gpio_node s_gpio_node_d720_v4[] =
{

	{POWER_5V_EN,			EXYNOS4_GPK1(2), 1, 1, NULL},
	{POWER_INT_EN,			EXYNOS4212_GPM3(7), 0, 1, NULL}, // low is  default volt, high is another volt (PMU cc)

	{POWER_GPS_EN,			EXYNOS4212_GPM4(6), 1, 1, NULL},

#if 1
	{POWER_WIFI_EN,			EXYNOS4212_GPM3(1), 1, 1, NULL},
	{POWER_WIFI_LDO,		EXYNOS4_GPL1(1), 1, 1, NULL},
#endif
	{POWER_GSM_SW,		GPIO_GSM_POWER_ON_OFF, 0, 1, gsm_power_write,gsm_power_read},
	
//	{POWER_GSM_WUP,		GPIO_GSM_WAKE_IN, 1, 1, NULL},
//	{POWER_GSM_SW,		GPIO_GSM_POWER_EN, 1, 1, power_gsm_sdio},
	{POWER_LCD33_EN,		EXYNOS4212_GPM2(3), 1, 1, NULL},
	{POWER_LCD18_EN,		EXYNOS4212_GPM2(0), 1, 1, NULL},  // notice: Low for enable

	{POWER_LVDS_PD,		EXYNOS4212_GPM2(4), 1, 1, NULL},
	{POWER_BL_EN,			EXYNOS4212_GPM4(0), 1, 1, NULL},

	{POWER_HUB_RST,		EXYNOS4212_GPM3(2), 1, 1, NULL},
	{POWER_HUB_CON,		EXYNOS4212_GPM3(3), 1, 1, NULL},
//	{POWER_HUB2_RST,		GPIO_USB_HUB2_RESET, 0, 1, usb_hub_reset},

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
};

static void power_gpio_on_boot(const struct power_gpio_oem * p)
{
	write_power_item_value(POWER_SPK_EN, 0);
	write_power_item_value(POWER_5V_EN, 1);
	gpio_direction_output(GPIO_USB_HUB2_RESET, GPIO_LEVEL_LOW);	
}

REGISTER_POWER_GPIO(s_gpio_node_d720_v4,d720,4,power_gpio_on_boot);

MODULE_DESCRIPTION("power_gpio config proc file");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("urbest, inc.");

