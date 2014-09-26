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

#include "axp-cfg.h"
#include "axp-mfd.h"
#include "axp-regu.h"
#if defined(CONFIG_REGULATOR_MP884X)
#include <linux/regulator/mp8845.h>
#endif

/* Reverse engineered partly from Platformx drivers */
enum axp_regls{

	vcc_ldo1,
	vcc_ldo2,
	vcc_ldo3,
	vcc_ldo4,
	vcc_ldo5,
	vcc_ldo6,
	vcc_ldo7,
	vcc_ldo8,
	vcc_ldo9,
	vcc_ldo10,
	vcc_ldo11,
	vcc_ldo12,

	vcc_DCDC1,
	vcc_DCDC2,
	vcc_DCDC3,
	vcc_DCDC4,
	vcc_DCDC5,
	vcc_ldoio0,
	vcc_ldoio1,
};

/* The values of the various regulator constraints are obviously dependent
 * on exactly what is wired to each ldo.  Unfortunately this information is
 * not generally available.  More information has been requested from Xbow
 * but as of yet they haven't been forthcoming.
 *
 * Some of these are clearly Stargate 2 related (no way of plugging
 * in an lcd on the IM2 for example!).
 */
static struct regulator_consumer_supply ldo1_data[] = {
		{
			.supply = "axp22_rtc",
		},
	};

#if 1

#ifdef CONFIG_AXP_DVT_VER
static struct regulator_consumer_supply ldo2_data[] = {
	REGULATOR_SUPPLY("vdd_alive", NULL),
};

static struct regulator_consumer_supply ldo3_data[] = {
	REGULATOR_SUPPLY("vdd_pll", NULL),
};

#else
static struct regulator_consumer_supply ldo2_data[] = {
	//	REGULATOR_SUPPLY("vqmmc", "dw_mmc.2"),
	//	REGULATOR_SUPPLY("vdd19_tflash_2v8", NULL),
	REGULATOR_SUPPLY("vdd10_off", NULL),
};

static struct regulator_consumer_supply ldo3_data[] = {
	REGULATOR_SUPPLY("vdd_18off", NULL),
};
#endif

//axp22_aldo3
static struct regulator_consumer_supply ldo4_data[] = {
	REGULATOR_SUPPLY("vddq_m12", NULL),
};

static struct regulator_consumer_supply ldo5_data[] = {
	REGULATOR_SUPPLY("vdd33_off", NULL),
};

static struct regulator_consumer_supply ldo6_data[] = {
	REGULATOR_SUPPLY("dldo2_cam_avdd_2v8", NULL),
};

#ifdef CONFIG_AXP_DVT_VER
static struct regulator_consumer_supply ldo7_data[] = {
	REGULATOR_SUPPLY("vdd_10off", NULL),
};

static struct regulator_consumer_supply ldo8_data[] = {
	REGULATOR_SUPPLY("vdd_18off", NULL),
};
#else
static struct regulator_consumer_supply ldo7_data[] = {
	REGULATOR_SUPPLY("dldo3_cam_led_1v8", NULL),
};

static struct regulator_consumer_supply ldo8_data[] = {
	REGULATOR_SUPPLY("eldo4_peri_3v3", NULL),
};
#endif

static struct regulator_consumer_supply ldo9_data[] = {
	REGULATOR_SUPPLY("vdd18_on", NULL),
};

#ifdef CONFIG_AXP_DVT_VER
static struct regulator_consumer_supply ldo10_data[] = {
	REGULATOR_SUPPLY("vdd_comb_18", NULL),
};

static struct regulator_consumer_supply ldo11_data[] = {
	REGULATOR_SUPPLY("vdd_eldo3_18", NULL),
};

static struct regulator_consumer_supply ldo12_data[] ={
	REGULATOR_SUPPLY("vdd1_dc5_ldo", NULL),
};
#else
static struct regulator_consumer_supply ldo10_data[] = {
	REGULATOR_SUPPLY("vdd10_pll", NULL),
};

static struct regulator_consumer_supply ldo11_data[] = {
	REGULATOR_SUPPLY("vdd_comb_18", NULL),
};

static struct regulator_consumer_supply ldo12_data[] ={
	REGULATOR_SUPPLY("vdd1_alive_1v0", NULL),
};
#endif

static struct regulator_consumer_supply ldoio0_data[] = {
	REGULATOR_SUPPLY("ldoio0_camaio_1v8", NULL),
};

#else
static struct regulator_consumer_supply ldo2_data[] = {
		{
			.supply = "axp22_aldo1",
		},
	};

static struct regulator_consumer_supply ldo3_data[] = {
		{
			.supply = "axp22_aldo2",
		},
	};

static struct regulator_consumer_supply ldo4_data[] = {
		{
			.supply = "axp22_aldo3",
		},
	};

static struct regulator_consumer_supply ldo5_data[] = {
		{
			.supply = "axp22_dldo1",
		},
	};

static struct regulator_consumer_supply ldo6_data[] = {
		{
			.supply = "axp22_dldo2",
		},
	};

static struct regulator_consumer_supply ldo7_data[] = {
		{
			.supply = "axp22_dldo3",
		},
	};

static struct regulator_consumer_supply ldo8_data[] = {
		{
			.supply = "axp22_dldo4",
		},
	};

static struct regulator_consumer_supply ldo9_data[] = {
		{
			.supply = "axp22_eldo1",
		},
	};

static struct regulator_consumer_supply ldo10_data[] = {
		{
			.supply = "axp22_eldo2",
		},
	};

static struct regulator_consumer_supply ldo11_data[] = {
		{
			.supply = "axp22_eldo3",
		},
	};

static struct regulator_consumer_supply ldo12_data[] = {
		{
			.supply = "axp22_dc5ldo",
		},
	};
static struct regulator_consumer_supply ldoio0_data[] = {
		{
			.supply = "axp22_ldoio0",
		},
	};

#endif

static struct regulator_consumer_supply ldoio1_data[] = {
		{
			.supply = "axp22_ldoio1",
		},
	};


#if 1//defined(CONFIG_UT_OEM)

static struct regulator_consumer_supply __initdata DCDC1_data[] = {
	REGULATOR_SUPPLY("vdd28_lcd_3v3", NULL),
};

static struct regulator_consumer_supply DCDC2_data[] = {
	REGULATOR_SUPPLY("vdd_arm", NULL),
};

static struct regulator_consumer_supply DCDC3_data[] = {
	REGULATOR_SUPPLY("vdd_g3d", NULL),
};

static struct regulator_consumer_supply DCDC4_data[] = {
	REGULATOR_SUPPLY("vdd_mif", NULL),
};

static struct regulator_consumer_supply DCDC5_data[] = {
	REGULATOR_SUPPLY("vdd_int", NULL),
};

#else


static struct regulator_consumer_supply DCDC1_data[] = {
		{
			.supply = "axp22_dcdc1",
		},
	};


static struct regulator_consumer_supply DCDC2_data[] = {
		{
			.supply = "axp22_dcdc2",
		},
	};

static struct regulator_consumer_supply DCDC3_data[] = {
		{
			.supply = "axp22_dcdc3",
		},
	};

static struct regulator_consumer_supply DCDC4_data[] = {
		{
			.supply = "axp22_dcdc4",
		},
	};


static struct regulator_consumer_supply DCDC5_data[] = {
		{
			.supply = "axp22_dcdc5",
		},
	};
#endif


#if 1//defined(CONFIG_UT_OEM)
static struct regulator_init_data axp_regl_init_data[] = {
	[vcc_ldo1] = {
		.constraints = {
			.name = "vdd_rtc",
			.min_uV =  AXP22_LDO1_MIN,
			.max_uV =  AXP22_LDO1_MAX,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo1_data),
		.consumer_supplies = ldo1_data,
	},

#ifdef CONFIG_AXP_DVT_VER
	[vcc_ldo2] = {
		.constraints	= {
			.name		= "VDD_ALIVE",
			.min_uV		= AXP_ALIVE_VALUE* 1000,
			.max_uV		= AXP_ALIVE_VALUE* 1000,
			.apply_uV	= 1,
			.boot_on	= 1,
			.state_mem = {
				.uV		= 1000000,
				.mode		= REGULATOR_MODE_NORMAL,
				.enabled	= 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo2_data),
		.consumer_supplies = ldo2_data,
	},


	[vcc_ldo3] = {
		.constraints = {
			.name = "VDD_PLL",
			.min_uV =  AXP_PLL_VALUE* 1000,
			.max_uV =  AXP_PLL_VALUE* 1000,
			.always_on	= 1,
			.boot_on    = 1,
			.apply_uV	  = 1,
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = 0 * 1000,
				.enabled = 1,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo3_data),
		.consumer_supplies = ldo3_data,
	},	
#else
	[vcc_ldo2] = {
		.constraints = {
			.name = "vdd10_off",
			.min_uV = AXP_LDO2_VALUE * 1000,
			.max_uV = AXP_LDO2_VALUE * 1000,
			.always_on	= 1,
			.boot_on    = 1,
			.apply_uV	  = 1,
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
			.state_standby = {
				.enabled = 0,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo2_data),
		.consumer_supplies = ldo2_data,
	},
	[vcc_ldo3] = {
		.constraints = {
			.name = "vdd18_off range",
			.min_uV =  AXP_LDO3_VALUE* 1000,
			.max_uV =  AXP_LDO3_VALUE* 1000,
			.always_on	= 1,
			.boot_on    = 1,
			.apply_uV	  = 1,
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = 0 * 1000,
				.enabled = 1,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo3_data),
		.consumer_supplies = ldo3_data,
	},
#endif
	[vcc_ldo4] = {
		.constraints = {
			.name = "vdd33_off range",
			.min_uV= AXP_LDO4_VALUE * 1000,
			.max_uV= AXP_LDO4_VALUE * 1000,
			.always_on	= 1,
			.boot_on    = 1,
			.apply_uV	  = 1,
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
			.state_mem	= {
				.uV = 0 * 1000,
				.enabled	= 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo4_data),
		.consumer_supplies = ldo4_data,
	},

	[vcc_ldo5] = {
		.constraints = {
			.name = "vdd18_on range",
			.min_uV = AXP_LDO5_VALUE* 1000,
			.max_uV = AXP_LDO5_VALUE* 1000,
			.always_on	= 1,
			.boot_on    = 1,
			.apply_uV	  = 1,
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = AXP_LDO5_VALUE * 1000,
				.enabled = 1,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo5_data),
		.consumer_supplies = ldo5_data,
	},

	[vcc_ldo6] = {
		.constraints	= {
			.name		= "vdd_cam range",
			.min_uV = AXP_LDO6_VALUE* 1000,
			.max_uV = AXP_LDO6_VALUE* 1000,
			#if defined(CONFIG_CAMERA_POWER_CTL)
			.always_on	= 0,
			.boot_on    = 0,
			.apply_uV	  = 1,
			#else
			.always_on	= 1,
			.boot_on    = 1,
			.apply_uV	  = 1,
			#endif
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
			.state_mem	= {
				.uV = 0 * 1000,
				.disabled	= 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo6_data),
		.consumer_supplies = ldo6_data,
	},



#ifdef CONFIG_AXP_DVT_VER

	[vcc_ldo7] = {
		.constraints = {
			.name = "vdd10_off",
			.min_uV = AXP_LDO7_VALUE * 1000,
			.max_uV = AXP_LDO7_VALUE * 1000,
			.always_on	= 1,
			.boot_on    = 1,
			.apply_uV	  = 1,
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = AXP_LDO7_VALUE * 1000,
				.enabled = 1,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo7_data),
		.consumer_supplies = ldo7_data,
	},

	[vcc_ldo8] = {
		.constraints = {
			.name = "vdd18_off range",
			.min_uV =  AXP_LDO8_VALUE* 1000,
			.max_uV =  AXP_LDO8_VALUE* 1000,
			.always_on	= 1,
			.boot_on    = 1,
			.apply_uV	  = 1,
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = AXP_LDO8_VALUE * 1000,
				.enabled = 1,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo8_data),
		.consumer_supplies = ldo8_data,
	},
#else
	[vcc_ldo7] = {
		.constraints	= {
			.name		= "vdd_cam led range",
			.min_uV =  AXP_LDO7_VALUE* 1000,
			.max_uV =  AXP_LDO7_VALUE* 1000,
			#if defined(CONFIG_CAMERA_POWER_CTL)
			.always_on	= 0,
			.boot_on    = 0,
			.apply_uV	  = 1,
			#else
			.always_on	= 1,
			.boot_on    = 1,
			.apply_uV	  = 1,
			#endif
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
			.state_mem	= {
				.uV = 0 * 1000,
				.enabled	= 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo7_data),
		.consumer_supplies = ldo7_data,
	},
	[vcc_ldo8] = {
		.constraints	= {
			.name		= "vdd_ldo8 range",
			.min_uV =  AXP_LDO8_VALUE* 1000,
			.max_uV =  AXP_LDO8_VALUE* 1000,
			.always_on	= 1,
			.boot_on    = 1,
			.apply_uV	  = 1,
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
			.state_mem	= {
				.uV = AXP_LDO8_VALUE * 1000,
				.enabled	= 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo7_data),
		.consumer_supplies = ldo8_data,
	},
#endif	

	[vcc_ldo9] = {
		.constraints = {
			.name = "vdd18_oon range",
			.min_uV =  AXP_LDO9_VALUE* 1000,
			.max_uV =  AXP_LDO9_VALUE* 1000,
			.always_on	= 1,
			.boot_on    = 1,
			.apply_uV	  = 1,
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
			.state_mem	= {
				.disabled	= 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo9_data),
		.consumer_supplies = ldo9_data,
	},

#if defined(CONFIG_AXP_DVT_VER)
	[vcc_ldo10] = {
		.constraints	= {
			.name		= "vdd_comb_18 range",
			.min_uV		= AXP_LDO10_VALUE * 1000,
			.max_uV		= AXP_LDO10_VALUE * 1000,
			.always_on	= 1,
			.boot_on    = 1,
			.apply_uV	  = 1,
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
			.state_mem	= {
				.uV = AXP_LDO10_VALUE * 1000,
				.disabled	= 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo10_data),
		.consumer_supplies = ldo10_data,
	},


	[vcc_ldo11] = {
		.constraints = {
			.name = "eldo3 range",
			.min_uV =  AXP_LDO11_VALUE* 1000,
			.max_uV =  AXP_LDO11_VALUE* 1000,
			.always_on	= 1,
			.boot_on    = 1,
			.apply_uV	  = 1,
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_mem	= {
				.uV = 0 * 1000,
				.enabled	= 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo11_data),
		.consumer_supplies = ldo11_data,
	},

	[vcc_ldo12] = {
		.constraints = {
			.name = "axp22_ldo12 range",
			.min_uV = AXP_LDO12_VALUE* 1000,
			.max_uV = AXP_LDO12_VALUE* 1000,
			.always_on	= 0,
			.boot_on    = 0,
			.apply_uV	  = 0,
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_mem	= {
				.uV = 0 * 1000,
				.enabled	= 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo12_data),
		.consumer_supplies = ldo12_data,
	},
#else
	[vcc_ldo10] = {
		.constraints	= {
			.name		= "peri_1v8 range",
			.min_uV		= AXP_LDO10_VALUE * 1000,
			.max_uV		= AXP_LDO10_VALUE * 1000,
#if 1// for 4412 816 board !!!!!!!!!!!!!!
			.always_on	= 0,
			.boot_on    = 0,
			.apply_uV	  = 0,
#else
			.always_on	= 1,
			.boot_on    = 1,
			.apply_uV	  = 1,
#endif
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
			.state_mem	= {
#if 1// for 4412 816 board !!!!!!!!!!!!!!
//				.uV = AXP_LDO10_VALUE * 1000,
//				.enabled	= 1,
#else
				.uV = AXP_LDO10_VALUE * 1000,
				.enabled	= 1,
#endif
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo10_data),
		.consumer_supplies = ldo10_data,
	},
	[vcc_ldo11] = {
		.constraints = {
			.name = "vdd_comb_18 range",
			.min_uV =  AXP_LDO11_VALUE* 1000,
			.max_uV =  AXP_LDO11_VALUE* 1000,
			.always_on	= 1,
			.boot_on    = 1,
			.apply_uV	  = 1,
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_mem	= {
				.uV = AXP_LDO11_VALUE * 1000,
				.enabled	= 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo11_data),
		.consumer_supplies = ldo11_data,
	},

	[vcc_ldo12] = {
		.constraints = {
			.name = "axp22_ldo12 range",
			.min_uV = AXP_LDO12_VALUE* 1000,
			.max_uV = AXP_LDO12_VALUE* 1000,
			.always_on	= 1,
			.boot_on    = 1,
			.apply_uV	  = 1,
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
//			.initial_state = PM_SUSPEND_STANDBY,
			.state_mem	= {
				.uV = AXP_LDO12_VALUE * 1000,
				.enabled	= 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo12_data),
		.consumer_supplies = ldo12_data,
	},
#endif
	[vcc_DCDC1] = {
		.constraints	= {
			.name		= "vdd_dcdc1 range",
			.min_uV		= AXP_DCDC1_VALUE* 1000,
			.max_uV		= AXP_DCDC1_VALUE* 1000,
			//.always_on	= 1,  //ericli remove 2013-11-12
			.boot_on    = 1,
			.apply_uV	  = 1,
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
			.state_mem	= {
				.uV = AXP_DCDC1_VALUE * 1000,
				.enabled	= 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(DCDC1_data),
		.consumer_supplies = DCDC1_data,
	},

	[vcc_DCDC2] = {
#if UT_AXP_CFG==2
		.constraints = {
			.name = "vdd_arm range",
			.min_uV = 800000,
			.max_uV = 1350000,
			.always_on = 1,
			.boot_on = 1,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
		},
#else
		.constraints	= {
			.name		= "vdd_g3d range",
			.min_uV = AXP22_DCDC2_MIN,
			.max_uV = AXP22_DCDC2_MAX,
			.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
					  REGULATOR_CHANGE_STATUS,
			.always_on = 1,
			.boot_on = 1,
			.state_mem	= {
				.disabled	= 1,
			},
		},
#endif
		.num_consumer_supplies = ARRAY_SIZE(DCDC2_data),
		.consumer_supplies	= DCDC2_data,
	},

	[vcc_DCDC3] = {
#if UT_AXP_CFG==2
		.constraints = {
			.name = "vdd_g3d range",
			.min_uV = AXP22_DCDC3_MIN,
			.max_uV = AXP22_DCDC3_MAX,
			.boot_on = 1,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE |
					  REGULATOR_CHANGE_STATUS,
			.state_mem = {
				.disabled = 1,
			},
		},
#else
		.constraints	= {
			.name		= "vdd_int range",
			.min_uV		=  AXP22_DCDC3_MIN,
			.max_uV		= AXP22_DCDC3_MAX,
			.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
					REGULATOR_CHANGE_STATUS,
			.always_on = 1,
			.boot_on = 1,
			.state_mem	= {
				.uV = AXP_DCDC3_VALUE * 1000,
				.mode		= REGULATOR_MODE_NORMAL,
				.disabled	= 1,
			},
		},
#endif
		.num_consumer_supplies = ARRAY_SIZE(DCDC3_data),
		.consumer_supplies = DCDC3_data,
	},

	[vcc_DCDC4] = {
#if UT_AXP_CFG==2
		.constraints = {
			.name = "vdd_mif range",
			.min_uV		=  AXP22_DCDC4_MIN,
			.max_uV		= AXP22_DCDC4_MAX,
			.always_on = 1,
			.boot_on = 1,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE |
					REGULATOR_CHANGE_STATUS,
		},
#else
		.constraints	= {
			.name		= "vdd_kfc range",
			.min_uV		=  AXP22_DCDC4_MIN,
			.max_uV		= AXP22_DCDC4_MAX,
			.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
					  REGULATOR_CHANGE_STATUS,
			.always_on = 1,
			.boot_on = 1,
			.state_mem	= {
				.disabled	= 1,
			},
		},
#endif
		.num_consumer_supplies = ARRAY_SIZE(DCDC4_data),
		.consumer_supplies = DCDC4_data,
	},

	[vcc_DCDC5] = { // ddr12
#if UT_AXP_CFG==2
	.constraints = {
		.name = "vdd_int range",
		.min_uV = AXP_DCDC5_VALUE * 1000,
		.max_uV = AXP_DCDC5_VALUE * 1000,
		.always_on = 1,
		.boot_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
#else
		.constraints = {
			.name = "dcdc5 range",
			.min_uV = AXP_DCDC5_VALUE * 1000,
			.max_uV = AXP_DCDC5_VALUE * 1000,
			.always_on	= 1,
			.boot_on    = 1,
			.apply_uV	  = 1,
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
			.state_standby = {
				.uV = AXP_DCDC5_VALUE * 1000,
				.enabled = 1,
			}
		},
#endif
		.num_consumer_supplies = ARRAY_SIZE(DCDC5_data),
		.consumer_supplies = DCDC5_data,
	},
	[vcc_ldoio0] = {
		.constraints = {
			.name = "axp22_ldoio0",
			.min_uV = AXP_LDOIO0_VALUE* 1000,
			.max_uV = AXP_LDOIO0_VALUE* 1000,
			#if defined(CONFIG_CAMERA_POWER_CTL)
			.always_on	= 0,
			.boot_on    = 0,
			.apply_uV	  = 1,
			#else
			.always_on	= 1,
			.boot_on    = 1,
			.apply_uV	  = 1,
			#endif
			.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
			.state_mem	= {
//				.uV = AXP_LDOIO0_VALUE * 1000,
				.disabled	= 1,
			},
		},
		.num_consumer_supplies = ARRAY_SIZE(ldoio0_data),
		.consumer_supplies = ldoio0_data,
	},

/* commented by Albert_lee 2013-07-13 begin */
#if 0
	[vcc_ldoio1] = {
		.constraints = {
			.name = "axp22_ldoio1",
			.min_uV = AXP22_LDOIO1_MIN,
			.max_uV = AXP22_LDOIO1_MAX,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldoio1_data),
		.consumer_supplies = ldoio1_data,
	},
#endif
 /*Albert_lee 2013-07-13 end*/
};
#else
static struct regulator_init_data axp_regl_init_data[] = {
	[vcc_ldo1] = {
		.constraints = {
			.name = "axp22_ldo1",
			.min_uV =  AXP22_LDO1_MIN,
			.max_uV =  AXP22_LDO1_MAX,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo1_data),
		.consumer_supplies = ldo1_data,
	},
	[vcc_ldo2] = {
		.constraints = {
			.name = "axp22_ldo2",
			.min_uV = AXP22_LDO2_MIN,
			.max_uV = AXP22_LDO2_MAX,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
#if defined (CONFIG_KP_OUTPUTINIT)
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = AXP_LDO2_VALUE * 1000,
				.enabled = 1,
			}
#endif
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo2_data),
		.consumer_supplies = ldo2_data,
	},
	[vcc_ldo3] = {
		.constraints = {
			.name = "axp22_ldo3",
			.min_uV =  AXP22_LDO3_MIN,
			.max_uV =  AXP22_LDO3_MAX,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
#if defined (CONFIG_KP_OUTPUTINIT)
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = AXP_LDO3_VALUE * 1000,
				.enabled = 1,
			}
#endif
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo3_data),
		.consumer_supplies = ldo3_data,
	},
	[vcc_ldo4] = {
		.constraints = {
			.name = "axp22_ldo4",
			.min_uV = AXP22_LDO4_MIN,
			.max_uV = AXP22_LDO4_MAX,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
#if defined (CONFIG_KP_OUTPUTINIT)
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = AXP_LDO4_VALUE * 1000,
				.enabled = 1,
			}
#endif
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo4_data),
		.consumer_supplies = ldo4_data,
	},
	[vcc_ldo5] = {
		.constraints = {
			.name = "axp22_ldo5",
			.min_uV = AXP22_LDO5_MIN,
			.max_uV = AXP22_LDO5_MAX,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
#if defined (CONFIG_KP_OUTPUTINIT)
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = AXP_LDO5_VALUE * 1000,
				.enabled = 0,
			}
#endif
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo5_data),
		.consumer_supplies = ldo5_data,
	},
	[vcc_ldo6] = {
		.constraints = {
			.name = "axp22_ldo6",
			.min_uV = AXP22_LDO6_MIN,
			.max_uV = AXP22_LDO6_MAX,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
#if defined (CONFIG_KP_OUTPUTINIT)
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = AXP_LDO6_VALUE * 1000,
				.enabled = 0,
			}
#endif
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo6_data),
		.consumer_supplies = ldo6_data,
	},
	[vcc_ldo7] = {
		.constraints = {
			.name = "axp22_ldo7",
			.min_uV =  AXP22_LDO7_MIN,
			.max_uV =  AXP22_LDO7_MAX,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
#if defined (CONFIG_KP_OUTPUTINIT)
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = AXP_LDO7_VALUE * 1000,
				.enabled = 0,
			}
#endif
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo7_data),
		.consumer_supplies = ldo7_data,
	},
	[vcc_ldo8] = {
		.constraints = {
			.name = "axp22_ldo8",
			.min_uV = AXP22_LDO8_MIN,
			.max_uV = AXP22_LDO8_MAX,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
#if defined (CONFIG_KP_OUTPUTINIT)
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = AXP_LDO8_VALUE * 1000,
				.enabled = 0,
			}
#endif
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo8_data),
		.consumer_supplies = ldo8_data,
	},
	[vcc_ldo9] = {
		.constraints = {
			.name = "axp22_ldo9",
			.min_uV = AXP22_LDO9_MIN,
			.max_uV = AXP22_LDO9_MAX,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
#if defined (CONFIG_KP_OUTPUTINIT)
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = AXP_LDO9_VALUE * 1000,
				.enabled = 0,
			}
#endif
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo9_data),
		.consumer_supplies = ldo9_data,
	},
	[vcc_ldo10] = {
		.constraints = {
			.name = "axp22_ldo10",
			.min_uV = AXP22_LDO10_MIN,
			.max_uV = AXP22_LDO10_MAX,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
#if defined (CONFIG_KP_OUTPUTINIT)
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = AXP_LDO10_VALUE * 1000,
				.enabled = 0,
			}
#endif
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo10_data),
		.consumer_supplies = ldo10_data,
	},
	[vcc_ldo11] = {
		.constraints = {
			.name = "axp22_ldo11",
			.min_uV =  AXP22_LDO11_MIN,
			.max_uV =  AXP22_LDO11_MAX,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
#if defined (CONFIG_KP_OUTPUTINIT)
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = AXP_LDO11_VALUE * 1000,
				.enabled = 0,
			}
#endif
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo11_data),
		.consumer_supplies = ldo11_data,
	},
	[vcc_ldo12] = {
		.constraints = {
			.name = "axp22_ldo12",
			.min_uV = AXP22_LDO12_MIN,
			.max_uV = AXP22_LDO12_MAX,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
#if defined (CONFIG_KP_OUTPUTINIT)
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = AXP_LDO12_VALUE * 1000,
				.enabled = 1,
			}
#endif
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo12_data),
		.consumer_supplies = ldo12_data,
	},


	[vcc_DCDC1] = {
		.constraints = {
			.name = "axp22_DCDC1",
			.min_uV = AXP22_DCDC1_MIN,
			.max_uV = AXP22_DCDC1_MAX,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
#if defined (CONFIG_KP_OUTPUTINIT)
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = AXP_DCDC1_VALUE * 1000,
				.enabled = 1,
			}
#endif
		},
		.num_consumer_supplies = ARRAY_SIZE(DCDC1_data),
		.consumer_supplies = DCDC1_data,
	},

	[vcc_DCDC2] = {
		.constraints = {
			.name = "axp22_DCDC2",
			.min_uV = AXP22_DCDC2_MIN,
			.max_uV = AXP22_DCDC2_MAX,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
#if defined (CONFIG_KP_OUTPUTINIT)
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = AXP_DCDC2_VALUE * 1000,
				.enabled = 1,
			}
#endif
		},
		.num_consumer_supplies = ARRAY_SIZE(DCDC2_data),
		.consumer_supplies = DCDC2_data,
	},
	[vcc_DCDC3] = {
		.constraints = {
			.name = "axp22_DCDC3",
			.min_uV = AXP22_DCDC3_MIN,
			.max_uV = AXP22_DCDC3_MAX,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
#if defined (CONFIG_KP_OUTPUTINIT)
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = AXP_DCDC3_VALUE * 1000,
				.enabled = 1,
			}
#endif
		},
		.num_consumer_supplies = ARRAY_SIZE(DCDC3_data),
		.consumer_supplies = DCDC3_data,
	},

	[vcc_DCDC4] = {
		.constraints = {
			.name = "axp22_DCDC4",
			.min_uV = AXP22_DCDC4_MIN,
			.max_uV = AXP22_DCDC4_MAX,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
#if defined (CONFIG_KP_OUTPUTINIT)
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = AXP_DCDC4_VALUE * 1000,
				.enabled = 1,
			}
#endif
		},
		.num_consumer_supplies = ARRAY_SIZE(DCDC4_data),
		.consumer_supplies = DCDC4_data,
	},

	[vcc_DCDC5] = {
		.constraints = {
			.name = "axp22_DCDC5",
			.min_uV = AXP22_DCDC5_MIN,
			.max_uV = AXP22_DCDC5_MAX,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
#if defined (CONFIG_KP_OUTPUTINIT)
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				.uV = AXP_DCDC5_VALUE * 1000,
				.enabled = 1,
			}
#endif
		},
		.num_consumer_supplies = ARRAY_SIZE(DCDC5_data),
		.consumer_supplies = DCDC5_data,
	},

	[vcc_ldoio0] = {
		.constraints = {
			.name = "axp22_ldoio0",
			.min_uV = AXP22_LDOIO0_MIN,
			.max_uV = AXP22_LDOIO0_MAX,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldoio0_data),
		.consumer_supplies = ldoio0_data,
	},
	[vcc_ldoio1] = {
		.constraints = {
			.name = "axp22_ldoio1",
			.min_uV = AXP22_LDOIO1_MIN,
			.max_uV = AXP22_LDOIO1_MAX,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldoio1_data),
		.consumer_supplies = ldoio1_data,
	},
};
#endif


static struct axp_funcdev_info axp_regldevs[] = {
	{
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO1,
		.platform_data = &axp_regl_init_data[vcc_ldo1],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO2,
		.platform_data = &axp_regl_init_data[vcc_ldo2],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO3,
		.platform_data = &axp_regl_init_data[vcc_ldo3],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO4,
		.platform_data = &axp_regl_init_data[vcc_ldo4],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO5,
		.platform_data = &axp_regl_init_data[vcc_ldo5],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO6,
		.platform_data = &axp_regl_init_data[vcc_ldo6],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO7,
		.platform_data = &axp_regl_init_data[vcc_ldo7],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO8,
		.platform_data = &axp_regl_init_data[vcc_ldo8],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO9,
		.platform_data = &axp_regl_init_data[vcc_ldo9],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO10,
		.platform_data = &axp_regl_init_data[vcc_ldo10],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO11,
		.platform_data = &axp_regl_init_data[vcc_ldo11],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO12,
		.platform_data = &axp_regl_init_data[vcc_ldo12],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_DCDC1,
		.platform_data = &axp_regl_init_data[vcc_DCDC1],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_DCDC2,
		.platform_data = &axp_regl_init_data[vcc_DCDC2],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_DCDC3,
		.platform_data = &axp_regl_init_data[vcc_DCDC3],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_DCDC4,
		.platform_data = &axp_regl_init_data[vcc_DCDC4],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_DCDC5,
		.platform_data = &axp_regl_init_data[vcc_DCDC5],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDOIO0,
		.platform_data = &axp_regl_init_data[vcc_ldoio0],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDOIO1,
		.platform_data = &axp_regl_init_data[vcc_ldoio1],
	},
};

static struct power_supply_info battery_data ={
		.name ="PTI PL336078",
		.technology = POWER_SUPPLY_TECHNOLOGY_LION,
		.voltage_max_design = CHGVOL,
		.voltage_min_design = SHUTDOWNVOL,
		.energy_full_design = BATCAP,
		.use_for_apm = 1,
};


static struct axp_supply_init_data axp_sply_init_data = {
	.battery_info = &battery_data,
	//.chgcur = STACHGCUR,
	.chgcur = INITCHGCUR,
	.chgvol = CHGVOL,
	.chgend = ENDCHGRATE,
	.chgen = CHGEN,
	.sample_time = ADCFREQ,
	.chgpretime = CHGPRETIME,
	.chgcsttime = CHGCSTTIME,
};

static struct axp_funcdev_info axp_splydev[]={
	{
		.name = "axp22-supplyer",
		.id = AXP22_ID_SUPPLY,
		.platform_data = &axp_sply_init_data,
	},
};

static struct axp_funcdev_info axp_gpiodev[]={
	{
		.name = "axp22-gpio",
		.id = AXP22_ID_GPIO,
	},
};

static struct axp_platform_data axp_pdata = {
	.num_regl_devs = ARRAY_SIZE(axp_regldevs),
	.num_sply_devs = ARRAY_SIZE(axp_splydev),
	.num_gpio_devs = ARRAY_SIZE(axp_gpiodev),
	.regl_devs = axp_regldevs,
	.sply_devs = axp_splydev,
	.gpio_devs = axp_gpiodev,
	.gpio_base = 0,
};


//#if defined(CONFIG_REGULATOR_MP884X)
//
//static struct regulator_consumer_supply mp884x_consumer =
//	REGULATOR_SUPPLY("vdd_mif", NULL);
//
//static struct regulator_init_data mp884x_data = {
//	.constraints	= {
//		.name		= "vdd_mif range",
//		.min_uV		=  600000,
//		.max_uV		= 1450000,
//		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
//				  REGULATOR_CHANGE_STATUS,
//		.always_on = 1,
//		.boot_on = 1,
//		.state_mem	= {
//			.disabled	= 1,
//		},
//	},
//	.num_consumer_supplies	= 1,
//	.consumer_supplies	= &mp884x_consumer,
//};
//
//static struct mp8845_platform_data mp884x_platform_data = {
//	.regulator		= &mp884x_data,
//};
//
////static struct i2c_board_info i2c_devs16[] __initdata = {
////	{
////		I2C_BOARD_INFO("mp884x_mif", (0x38 >> 1) ),
////		.platform_data = &mp884x_platform_data,
////	},
////};
//
////static struct i2c_gpio_platform_data gpio_i2c16_data  = {
////	.sda_pin	= EXYNOS5410_GPV2(0),
////	.scl_pin	= EXYNOS5410_GPV2(1),
////	.udelay	= 5,
////};
////
////struct platform_device gpio_i2c16_dev = {
////	.name	= "i2c-gpio",
////	.id		= 16,
////	.dev		= { .platform_data = &gpio_i2c16_data },
////};
//
//#endif


   /*×¢²áÉè±¸ */
static struct i2c_board_info __initdata axp_mfd_i2c_board_info[] = {
	{
		.type = "axp22_mfd",
		.addr = AXP_DEVICES_ADDR,
		.platform_data = &axp_pdata,
		.irq = AXP_IRQNO,
	},
//#if defined(CONFIG_REGULATOR_MP884X)
//	{
//		I2C_BOARD_INFO("mp884x_mif", (0x38 >> 1) ),
//		.platform_data = &mp884x_platform_data,
//	},
//#endif
};


static int __init axp22_board_init(void)
{
	int ret = -1;
	printk("%s();  start !\n", __func__);

	if (is_axp_pmu) {
		ret =  i2c_register_board_info(AXP_I2CBUS, axp_mfd_i2c_board_info, ARRAY_SIZE(axp_mfd_i2c_board_info));
	}
	printk("%s(); ret=%d -\n", __func__, ret);

	return ret;
}
arch_initcall(axp22_board_init);

MODULE_DESCRIPTION("X-powers axp board");
MODULE_AUTHOR("Weijin Zhong");
MODULE_LICENSE("GPL");
