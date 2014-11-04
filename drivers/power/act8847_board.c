/*
 * Base driver for X-Powers AXP
 *
 * Copyright (C) 2013 X-Powers, Ltd.
 *  Zhang Donglu <zhangdonglu@x-powers.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/i2c.h>
#include <mach/irqs.h>
#include <linux/power_supply.h>
#include <linux/apm_bios.h>
#include <linux/apm-emulation.h>
#include <linux/module.h>


#ifdef CONFIG_REGULATOR_ACT8847

#include <linux/regulator/act8847.h>

static struct regulator_consumer_supply act8847_buck1_consumer =
	REGULATOR_SUPPLY("vdd_mem", NULL);

static struct regulator_consumer_supply act8847_buck1_mif_consumer =
	REGULATOR_SUPPLY("vdd_mif", NULL);

static struct regulator_consumer_supply act8847_buck2_consumer =
	REGULATOR_SUPPLY("vdd_arm", NULL);

static struct regulator_consumer_supply act8847_buck3_consumer =
	REGULATOR_SUPPLY("vdd_g3d", NULL);

static struct regulator_consumer_supply act8847_buck4_consumer =
	REGULATOR_SUPPLY("vdd_int", NULL);

static struct regulator_consumer_supply act8847_ldo5_consumer =
	REGULATOR_SUPPLY("vdd_pll", NULL);

static struct regulator_consumer_supply act8847_ldo6_consumer =
	REGULATOR_SUPPLY("vdd_alive", NULL);

#if 1
static struct regulator_consumer_supply act8847_ldo8_consumer =
	REGULATOR_SUPPLY("vdd_18", NULL);

static struct regulator_consumer_supply act8847_ldo9_consumer =
	REGULATOR_SUPPLY("vdd_33", NULL);
#endif

static struct regulator_consumer_supply act8847_ldo10_consumer =
	REGULATOR_SUPPLY("vddq_m12", NULL);
#if 1
static struct regulator_consumer_supply act8847_ldo11_consumer =
	REGULATOR_SUPPLY("vdd_10", NULL);
#endif

//static struct regulator_consumer_supply act8847_dummy_consumer =
//	REGULATOR_SUPPLY("vdd_mif", NULL);

static struct regulator_consumer_supply act8847_dummy_consumers[] = {
	REGULATOR_SUPPLY("vdd_mif", NULL),		//4412 bus mif
	REGULATOR_SUPPLY("vdd", NULL),			//hdmi
	REGULATOR_SUPPLY("vdd_osc", NULL),		//hdmi
	REGULATOR_SUPPLY("vusb_d", NULL),		//digital USB supply, 1.2V
	REGULATOR_SUPPLY("vusb_a", NULL),		//analog USB supply, 1.1V
	REGULATOR_SUPPLY("vdd8_mipi", NULL),	//1.0V (smdk4x12) MIPI CSI suppply
	REGULATOR_SUPPLY("vdd10_mipi", NULL),	//VDD 1.8V and MIPI CSI PLL supply
	REGULATOR_SUPPLY("vdd_tmu", NULL),
	REGULATOR_SUPPLY("vdd18_mipi", NULL),
};

static struct regulator_consumer_supply act8847_dummy_nomif_consumers[] = {
	REGULATOR_SUPPLY("vdd", NULL),			//hdmi
	REGULATOR_SUPPLY("vdd_osc", NULL),		//hdmi
	REGULATOR_SUPPLY("vusb_d", NULL),		//digital USB supply, 1.2V
	REGULATOR_SUPPLY("vusb_a", NULL),		//analog USB supply, 1.1V
	REGULATOR_SUPPLY("vdd8_mipi", NULL),	//1.0V (smdk4x12) MIPI CSI suppply
	REGULATOR_SUPPLY("vdd10_mipi", NULL),	//VDD 1.8V and MIPI CSI PLL supply
	REGULATOR_SUPPLY("vdd_tmu", NULL),
	REGULATOR_SUPPLY("vdd18_mipi", NULL),
};


static struct regulator_init_data act8847_buck1_data = {
	.constraints	= {
		.name		= "vdd_mem range",
		.min_uV		= 1200000,
		.max_uV		= 1500000,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
		.always_on = 1,
		.boot_on = 1,
		.state_mem	= {
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &act8847_buck1_consumer,
};

static struct regulator_init_data act8847_buck1_mif_data = {
	.constraints	= {
		.name		= "vdd_mif range",
		.min_uV = 800000,
		.max_uV		= 1500000,
		.always_on = 1,
		.boot_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
		
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &act8847_buck1_mif_consumer,
};

static struct regulator_init_data act8847_buck2_data = {
	.constraints = {
		.name = "vdd_arm range",
		.min_uV = 800000,
		.max_uV = 1350000,
		.always_on = 1,
		.boot_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = 1,
	.consumer_supplies = &act8847_buck2_consumer,
};

static struct regulator_init_data act8847_buck3_data = {
	.constraints = {
		.name = "vdd_g3d range",
		.min_uV = 850000,
		.max_uV = 1200000,
		.boot_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
		.state_mem = {
			.disabled = 1,
		},
	},
	.num_consumer_supplies = 1,
	.consumer_supplies = &act8847_buck3_consumer,
};

static struct regulator_init_data act8847_buck4_data = {
	.constraints = {
		.name = "vdd_int range",
		.min_uV = 800000,
		.max_uV = 1150000,
		.always_on = 1,
		.boot_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = 1,
	.consumer_supplies = &act8847_buck4_consumer,

};

// we use fix dc-dc for vdd-mif, etc.
static struct regulator_init_data act8847_dummy_data = {
	.constraints	= {
		.name		= "vdd dummy",
		.min_uV = 850000,
		.max_uV = 1200000,
		.boot_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
		.state_mem = {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(act8847_dummy_consumers),
	.consumer_supplies	= act8847_dummy_consumers,
};

static struct regulator_init_data act8847_dummy_nomif_data = {
	.constraints	= {
		.name		= "vdd dummy",
		.min_uV = 850000,
		.max_uV = 1200000,
		.boot_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
		.state_mem = {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(act8847_dummy_nomif_consumers),
	.consumer_supplies	= act8847_dummy_nomif_consumers,
};


static struct regulator_init_data act8847_ldo5_data = {
	.constraints	= {
		.name		= "VDD_PLL",
		.min_uV		= 1100000,
		.max_uV		= 1100000,
		.apply_uV	= 1,
		.state_mem = {
			.uV		= 1100000,
			.mode		= REGULATOR_MODE_NORMAL,
			.enabled	= 0,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &act8847_ldo5_consumer,
};

static struct regulator_init_data act8847_ldo6_data = {
	.constraints	= {
		.name		= "VDD_ALIVE",
		.min_uV		= 1000000,
		.max_uV		= 1000000,
		.apply_uV	= 1,
		.boot_on	= 1,
		.state_mem = {
			.uV		= 1000000,
			.mode		= REGULATOR_MODE_NORMAL,
			.enabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &act8847_ldo6_consumer,
};

#if 0
static struct regulator_init_data act8847_ldo8_data = {
	.constraints	= {
		.name		= "VDD_18",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.boot_on	= 1,
		.state_mem = {
			.uV		= 1800000,
			.mode		= REGULATOR_MODE_NORMAL,
			.enabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &act8847_ldo8_consumer,
};

static struct regulator_init_data act8847_ldo9_data = {
	.constraints	= {
		.name		= "VDD_33",
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.apply_uV	= 1,
		.boot_on	= 1,
		.state_mem = {
			.uV		= 3300000,
			.mode		= REGULATOR_MODE_NORMAL,
			.enabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &act8847_ldo9_consumer,
};
#endif

#define VDDQ_VOLT	1300000

static struct regulator_init_data act8847_ldo10_data = {
	.constraints	= {
		.name		= "VDDQ_M12",
		.min_uV		= VDDQ_VOLT,
		.max_uV		= VDDQ_VOLT,
		.apply_uV	= 1,
		.boot_on	= 1,
		.state_mem = {
			.uV		= VDDQ_VOLT,
			.mode		= REGULATOR_MODE_NORMAL,
			.enabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &act8847_ldo10_consumer,
};

#if 0
static struct regulator_init_data act8847_ldo11_data = {
	.constraints	= {
		.name		= "VDD_10",
		.min_uV		= 1000000,
		.max_uV		= 1000000,
		.apply_uV	= 1,
		.boot_on	= 1,
		.state_mem = {
			.uV		= 1000000,
			.mode		= REGULATOR_MODE_NORMAL,
			.enabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &act8847_ldo11_consumer,
};
#endif

static struct act8847_regulator_subdev ut4x12_act8847_regulators[] = {
	{ ACT8847_DCDC1, &act8847_buck1_data },
	{ ACT8847_DCDC2, &act8847_buck2_data },
	{ ACT8847_DCDC3, &act8847_buck3_data },
	{ ACT8847_DCDC4, &act8847_buck4_data },
	{ ACT8847_LDO5,   &act8847_ldo5_data },
	{ ACT8847_LDO6,   &act8847_ldo6_data },
#if 1
//	{ ACT8847_LDO7,   &act8847_ldo7_data },
//	{ ACT8847_LDO8,   &act8847_ldo8_data },
//	{ ACT8847_LDO9,   &act8847_ldo9_data },
	{ ACT8847_LDO10,   &act8847_ldo10_data },
//	{ ACT8847_LDO11,   &act8847_ldo11_data },
//	{ ACT8847_LDO12,   &act8847_ldo12_data },
//	{ ACT8847_LDO13,   &act8847_ldo13_data },
#endif
	{ ACT8847_DCDC_DUMMY, &act8847_dummy_nomif_data  },
};


static struct act8847_regulator_subdev ut4x12_act8847_regulators_class2[] = {
	{ ACT8847_DCDC1, &act8847_buck1_mif_data },
	{ ACT8847_DCDC2, &act8847_buck2_data },
	{ ACT8847_DCDC3, &act8847_buck3_data },
	{ ACT8847_DCDC4, &act8847_buck4_data },
	{ ACT8847_LDO5,   &act8847_ldo5_data },
	{ ACT8847_LDO6,   &act8847_ldo6_data },
#if 1
//	{ ACT8847_LDO7,   &act8847_ldo7_data },
//	{ ACT8847_LDO8,   &act8847_ldo8_data },
//	{ ACT8847_LDO9,   &act8847_ldo9_data },
	{ ACT8847_LDO10,   &act8847_ldo10_data },
//	{ ACT8847_LDO11,   &act8847_ldo11_data },
//	{ ACT8847_LDO12,   &act8847_ldo12_data },
//	{ ACT8847_LDO13,   &act8847_ldo13_data },
#endif
	{ ACT8847_DCDC_DUMMY, &act8847_dummy_data },
};


struct act8847_platform_data act8847_data = {
	.num_regulators = ARRAY_SIZE(ut4x12_act8847_regulators),
	.regulators     = ut4x12_act8847_regulators,
};


static struct i2c_board_info act_mfd_i2c_board_info[] __initdata = {
#ifdef CONFIG_REGULATOR_ACT8847
	{
		I2C_BOARD_INFO("act8847", 0x5A),
		.platform_data = &act8847_data,
	},
#endif
};


struct act8847_platform_data act8847_data_class2 = {
	.num_regulators = ARRAY_SIZE(ut4x12_act8847_regulators_class2),
	.regulators     = ut4x12_act8847_regulators_class2,
};


static struct i2c_board_info act_mfd_i2c_board_info_class2[] __initdata = {
#ifdef CONFIG_REGULATOR_ACT8847
	{
		I2C_BOARD_INFO("act8847", 0x5A),
		.platform_data = &act8847_data_class2,
	},
#endif
};

#endif


#define	ACT_I2CBUS			1

static int __init act8847_board_init(void)
{
	int ret = -1;

	printk("%s();  start !\n", __func__);

	if (is_act_pmu) {
		if (is_act_pmu_class2) {
			ret =  i2c_register_board_info(ACT_I2CBUS, act_mfd_i2c_board_info_class2, ARRAY_SIZE(act_mfd_i2c_board_info_class2));
		} else {
			ret =  i2c_register_board_info(ACT_I2CBUS, act_mfd_i2c_board_info, ARRAY_SIZE(act_mfd_i2c_board_info));
		}
	}
	printk("%s(); ret=%d -\n", __func__, ret);

	return ret;
}

arch_initcall(act8847_board_init);

MODULE_DESCRIPTION("X-powers act board");
MODULE_AUTHOR("Albert Lee");
MODULE_LICENSE("GPL");
