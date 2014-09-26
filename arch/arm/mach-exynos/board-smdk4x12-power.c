/* linux/arch/arm/mach-exynos/board-smdk4x12-power.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/max8649.h>
#include <linux/mfd/max8997.h>
#include <linux/mfd/max77686.h>
//#include <linux/mfd/samsung/core.h>
//#include <linux/mfd/samsung/s5m8767.h>
#include <linux/mfd/wm8994/pdata.h>


#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/gpio.h>
#include <linux/io.h>

#include <plat/devs.h>
#include <plat/iic.h>
#include <plat/gpio-cfg.h>

#include <mach/regs-pmu.h>
#include <mach/irqs.h>


#include "board-smdk4x12.h"

#define REG_INFORM4            (S5P_INFORM4)


#ifdef CONFIG_MFD_WM8994

static struct regulator_consumer_supply wm8994_fixed_voltage0_supplies[] = {
	REGULATOR_SUPPLY("AVDD2", "1-001a"),
	REGULATOR_SUPPLY("CPVDD", "1-001a"),
};

static struct regulator_consumer_supply wm8994_fixed_voltage1_supplies[] = {
	REGULATOR_SUPPLY("SPKVDD1", "1-001a"),
	REGULATOR_SUPPLY("SPKVDD2", "1-001a"),
};

static struct regulator_consumer_supply wm8994_fixed_voltage2_supplies =
	REGULATOR_SUPPLY("DBVDD", "1-001a");

static struct regulator_init_data wm8994_fixed_voltage0_init_data = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(wm8994_fixed_voltage0_supplies),
	.consumer_supplies	= wm8994_fixed_voltage0_supplies,
};

static struct regulator_init_data wm8994_fixed_voltage1_init_data = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(wm8994_fixed_voltage1_supplies),
	.consumer_supplies	= wm8994_fixed_voltage1_supplies,
};

static struct regulator_init_data wm8994_fixed_voltage2_init_data = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &wm8994_fixed_voltage2_supplies,
};

static struct fixed_voltage_config wm8994_fixed_voltage0_config = {
	.supply_name	= "VDD_1.8V",
	.microvolts	= 1800000,
	.gpio		= -EINVAL,
	.init_data	= &wm8994_fixed_voltage0_init_data,
};

static struct fixed_voltage_config wm8994_fixed_voltage1_config = {
	.supply_name	= "DC_5V",
	.microvolts	= 5000000,
	.gpio		= -EINVAL,
	.init_data	= &wm8994_fixed_voltage1_init_data,
};

static struct fixed_voltage_config wm8994_fixed_voltage2_config = {
	.supply_name	= "VDD_3.3V",
	.microvolts	= 3300000,
	.gpio		= -EINVAL,
	.init_data	= &wm8994_fixed_voltage2_init_data,
};

static struct platform_device wm8994_fixed_voltage0 = {
	.name		= "reg-fixed-voltage",
	.id		= 0,
	.dev		= {
		.platform_data	= &wm8994_fixed_voltage0_config,
	},
};

static struct platform_device wm8994_fixed_voltage1 = {
	.name		= "reg-fixed-voltage",
	.id		= 1,
	.dev		= {
		.platform_data	= &wm8994_fixed_voltage1_config,
	},
};

static struct platform_device wm8994_fixed_voltage2 = {
	.name		= "reg-fixed-voltage",
	.id		= 2,
	.dev		= {
		.platform_data	= &wm8994_fixed_voltage2_config,
	},
};

static struct regulator_consumer_supply wm8994_avdd1_supply =
	REGULATOR_SUPPLY("AVDD1", "1-001a");

static struct regulator_consumer_supply wm8994_dcvdd_supply =
	REGULATOR_SUPPLY("DCVDD", "1-001a");

static struct regulator_init_data wm8994_ldo1_data = {
	.constraints	= {
		.name		= "AVDD1",
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &wm8994_avdd1_supply,
};

static struct regulator_init_data wm8994_ldo2_data = {
	.constraints	= {
		.name		= "DCVDD",
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &wm8994_dcvdd_supply,
};

static struct wm8994_pdata wm8994_platform_data = {
	/* configure gpio1 function: 0x0001(Logic level input/output) */
	.gpio_defaults[0] = 0x0001,
	/* If the i2s0 and i2s2 is enabled simultaneously */
	.gpio_defaults[7] = 0x8100, /* GPIO8  DACDAT3 in */
	.gpio_defaults[8] = 0x0100, /* GPIO9  ADCDAT3 out */
	.gpio_defaults[9] = 0x0100, /* GPIO10 LRCLK3  out */
	.gpio_defaults[10] = 0x0100,/* GPIO11 BCLK3   out */
	.ldo[0] = { 0, NULL, &wm8994_ldo1_data },
	.ldo[1] = { 0, NULL, &wm8994_ldo2_data },
};
#endif


static struct i2c_board_info i2c_devs1[] __initdata = {
#ifdef CONFIG_MFD_WM8994
	{
		I2C_BOARD_INFO("wm8994", 0x1a),
		.platform_data	= &wm8994_platform_data,
	},
#endif
//#ifdef CONFIG_REGULATOR_ACT8847
//	{
//		I2C_BOARD_INFO("act8847", 0x5A),
//		.platform_data = &act8847_data,
//	},
//#endif
};

#ifdef CONFIG_EXYNOS_BUSFREQ_OPP
//static struct platform_device exynos4_busfreq = {
//	.name = "exynos-busfreq",
//	.id = -1,
//};
#endif

static struct platform_device *smdk4x12_power_devices[] __initdata = {
	&s3c_device_i2c1,
#ifdef CONFIG_EXYNOS_BUSFREQ_OPP
//	&exynos4_busfreq,
#endif
};

#define FACOTRY_RESET_MODE "recovery"
#define AUTO_UPGRADE_MODE "autoupgrade"

static int smdk4x12_notifier_call(struct notifier_block *this,
					unsigned long code, void *_cmd)
{
	int mode = 0;
	char *cmd=(char *)_cmd;

	if (code != SYS_RESTART)
	{
		printk("[ERR]Invalid caller for restart:%lx,%s\n",code, (char *)_cmd);
		return NOTIFY_DONE;
	}
	printk("the reboot command is %s \n", (char *)_cmd);
	if(!cmd)
	{
		mode = 0x0;
		printk("Normal Reboot \n");

	}
	else if (cmd[0]=='r') // "recovery"
	{
		mode = 0xc1;
		printk("Reboot for Factory reset \n");
	}

	else if (cmd[0]=='a') // "autoupgrade"
	{
		mode = 0xc4;
		printk("Reboot for Factory reset \n");
	}	
	else
	{
		mode = 0x0;
		printk("unknow reboot command \n");
	}

	__raw_writel(mode, S5P_INFORM5);
	
	printk("INFORM5 :%x \n",__raw_readl(S5P_INFORM5));
	return NOTIFY_DONE;
}

static struct notifier_block smdk4x12_reboot_notifier = {
	.notifier_call = smdk4x12_notifier_call,
};

void __init exynos4_smdk4x12_power_init(void)
{

	s3c_i2c1_set_platdata(NULL);
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));

	platform_add_devices(smdk4x12_power_devices,
			ARRAY_SIZE(smdk4x12_power_devices));

	register_reboot_notifier(&smdk4x12_reboot_notifier);
}
