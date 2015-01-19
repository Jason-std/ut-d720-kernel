#include <linux/pm.h>
#include <asm/io.h>
#include <asm/sizes.h>
#include <asm/mach-types.h>
#include <plat/gpio-cfg.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <mach/regs-pmu.h>

#include "urbetter/power_gpio.h"
#include <linux/regulator/consumer.h>


extern int hdmi_plug_flag;
int power_spk( struct power_gpio_node node , int on)
{
	printk("enter %s,hdmi=%d,on=%d\n",__func__,hdmi_plug_flag ,on);
	if(hdmi_plug_flag)
		gpio_direction_output(node.pin,node.polarity ?0:1);
	else
		gpio_direction_output(node.pin,node.polarity ?on:!on);
	return 1;
}

int power_cam_dldo2_2v8(struct power_gpio_node node , int on)
{
	struct regulator *vdd28_cam_regulator=regulator_get(NULL, "dldo2_cam_avdd_2v8");
	if(on)
		regulator_enable(vdd28_cam_regulator);
	else
		regulator_disable(vdd28_cam_regulator);
	regulator_put(vdd28_cam_regulator);
	return 0;
}

int power_cam_eldo3_af(struct power_gpio_node node , int on)
{
	struct regulator *vdd18_cam_regulator=regulator_get(NULL, "vdd_eldo3_18");
	if(on)
		regulator_enable(vdd18_cam_regulator);
	else
		regulator_disable(vdd18_cam_regulator);
	regulator_put(vdd18_cam_regulator);
	return 0;
}

volatile int gsm_power_switch = 0;
int gsm_power_write(struct power_gpio_node node,int on)
{
	if(on) {
		gsm_power_switch = 1;
	} else {
		gsm_power_switch = 0;
	}

	return 0;
}

int gsm_power_read(struct power_gpio_node node)
{
	return gsm_power_switch;
}
