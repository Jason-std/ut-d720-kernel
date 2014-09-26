/********************************** 
  *			add by skyworth linda for wifi 
  **********************************/

#include <asm/mach-types.h>
#include <asm/mach/mmc.h>
#include <linux/regulator/consumer.h>
#include <mach/gpio.h>

#include <asm/io.h>
#include <mach/regs-gpio.h>

#include <asm/delay.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/mmc/core.h>
#include <linux/mmc/host.h>
#include <plat/devs.h>

#include <plat/sdhci.h>
extern void sdhci_s3c_force_presence_change(struct platform_device *pdev);
//extern int flag_wifi1;
int brcm_wifi_power(int on)
{
	int err=0;
	if(on) {
			printk(KERN_NOTICE "power");			
			//power low
			
			gpio_direction_output(GPIO_COMBO_LDO, 1);//output 
			
			err=gpio_request(GPIO_COMBO_PMUEN, "COMBO PMUEN");
			if (err) {
 				printk(KERN_ERR "failed to request GPH1 for WiFi power control\n");
//				return 0;
			}
			gpio_direction_output(GPIO_COMBO_PMUEN, 1);//output 
			gpio_set_value(GPIO_COMBO_PMUEN, 0);//low
			gpio_free(GPIO_COMBO_PMUEN);				
			msleep(500);
			
			//power high
			err = gpio_request(GPIO_COMBO_PMUEN, "COMBO PMUEN");
			if (err) {
 				printk(KERN_ERR "failed to request GPH1 for WiFi power control\n");
//				return 0;
			}
			gpio_direction_output(GPIO_COMBO_PMUEN, 1);//output
			gpio_set_value(GPIO_COMBO_PMUEN, 1);//high
			gpio_free(GPIO_COMBO_PMUEN);
			msleep(500);
//			flag_wifi1=1;
			printk(KERN_NOTICE "on\n");
	}else{
	#if 1 // albert debug
			printk(KERN_NOTICE "power");
			//power
			err=gpio_request(GPIO_COMBO_PMUEN, "COMBO PMUEN");
			if (err) {
 				printk(KERN_ERR "failed to request GPH1 for WiFi power control\n");
//				return ;
			}
			gpio_direction_output(GPIO_COMBO_PMUEN, 1);//output 
			gpio_set_value(GPIO_COMBO_PMUEN, 0);//low
			gpio_free(GPIO_COMBO_PMUEN);
			msleep(500);
//			flag_wifi1=0;
			printk(KERN_NOTICE"off\n");
	#endif		
	}

	return 0;
}
EXPORT_SYMBOL(brcm_wifi_power);

int brcm_wifi_reset(int on){
	printk(KERN_NOTICE "power reset,do nothing...");
	return 0;
}
EXPORT_SYMBOL(brcm_wifi_reset);

extern void sdhci_s3c_notify_change(struct platform_device *dev, int state);

static DEFINE_MUTEX(wlan_mutex_lock);


int brcm_wifi_set_carddetect(int val)
{
	printk(KERN_INFO "%s: %d\n", __func__, val);
	printk(KERN_INFO "%s: wifi_reg_on,%d  vddio,%d\n", __func__
		, gpio_get_value(GPIO_COMBO_PMUEN)
		, gpio_get_value(GPIO_COMBO_LDO)
		);

	mutex_lock(&wlan_mutex_lock);

	sdhci_s3c_notify_change(&s3c_device_hsmmc3, val);
	mutex_unlock(&wlan_mutex_lock);

	return 0;
}
EXPORT_SYMBOL(brcm_wifi_set_carddetect);

