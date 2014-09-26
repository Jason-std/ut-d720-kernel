/********************************** 
  *			add by skyworth linda for wifi 
  **********************************/
  
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/skbuff.h>
#include <linux/wlan_plat.h>
#include <plat/irqs.h>
#include <mach/irqs.h>
//#include <mach/board-samsung.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-rtc.h>

extern int brcm_wifi_power(int on);
extern int brcm_wifi_reset(int on);
extern int brcm_wifi_set_carddetect(int val);

//#define WLAN_OOB_IRQ  2
static struct resource brcm_wifi_resources[] = {
	[0] = {
		.name		= "bcmdhd_wlan_irq",
		.start		= IRQ_WIFI_WUP/*IRQ_EINT(2)*/, 
		.end		= IRQ_WIFI_WUP/*IRQ_EINT(2)*/,
		.flags          = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE,
	},
};

static struct wifi_platform_data brcm_wifi_control = {
	.set_power      = brcm_wifi_power,
	.set_reset      = brcm_wifi_reset,
	.set_carddetect = brcm_wifi_set_carddetect,	
};

static struct platform_device brcm_wifi_device = {
        .name           = "bcmdhd_wlan",
        .id             = 1,
        .num_resources  = ARRAY_SIZE(brcm_wifi_resources),
        .resource       = brcm_wifi_resources,
        .dev            = {
        .platform_data = &brcm_wifi_control,
        },
};



int Exynos4x12_rtc_clk_sw(int on)
{
	u32 val;
	
	void __iomem *rtc_ctrl = ioremap((EXYNOS4_PA_RTC+S3C2410_RTCCON), SZ_64);
	if (!rtc_ctrl) {
		printk(KERN_ERR"failed to allocate memory for EXYNOS4_PA_RTC\n");
		return -1;
	}
	if(on) {
	       val = __raw_readl(rtc_ctrl ) & ~S3C2410_RTCCON_CLKEN;
	       val |= S3C2410_RTCCON_CLKEN;
	       __raw_writel(val, rtc_ctrl );	
	} else {
		val = __raw_readl(rtc_ctrl ) & ~S3C2410_RTCCON_CLKEN;
	       __raw_writel(val, rtc_ctrl );	
	}
	
	iounmap(rtc_ctrl);   

	return 0;
}
EXPORT_SYMBOL(Exynos4x12_rtc_clk_sw);


extern bool is_bcm_combo(void);
int __init broadcom_wifi_init(void)
{
	int ret;

	if(!is_bcm_combo()) {
		printk(KERN_INFO"%s(), return -1\n", __func__);
		return -1;
	}
	printk(KERN_INFO"%s(); bdwg \n", __func__);

// bcm wifi  start 
	gpio_direction_output(GPIO_COMBO_LDO, 1);//output 
	Exynos4x12_rtc_clk_sw(1);
	//gpio_set_value(S5PV210_GPH2(7),1);
//	msleep(3000);

	s3c_gpio_cfgpin(GPIO_WIFI_WUP, S3C_GPIO_INPUT);//output 
	s3c_gpio_setpull(GPIO_WIFI_WUP, S3C_GPIO_PULL_DOWN);
// bcm wifi  end 
	
	ret = platform_device_register(&brcm_wifi_device);
	return ret;
}
late_initcall(broadcom_wifi_init);
