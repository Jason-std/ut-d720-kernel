/*
 * Regulator driver for National Semiconductors ACT8847 PMIC chip
 *
 *  Copyright (C) 2009 Samsung Electronics
 *  Author: Marek Szyprowski <m.szyprowski@samsung.com>
 *
 * Based on wm8350.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/bug.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/act8847.h>
#include <plat/gpio-cfg.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/delay.h>

//#define ACT8847_DEBUG
#ifdef ACT8847_DEBUG
#define act_dbg(x...)		\
	printk(x)
#else
#define act_dbg(x...)		\
	({ if (0) 	printk(x); 0; })
#endif


struct act8847 {	
	struct device *dev;	
	struct mutex io_lock;	
	struct i2c_client *i2c;	
	int num_regulators;	
	struct regulator_dev **rdev;
};

//grobal
static struct act8847 *g_act8847;

static u8 act8847_reg_read(struct act8847 *act8847, u8 reg);
static int act8847_set_bits(struct act8847 *act8847, u8 reg, u16 mask, u16 val);


/*	
		  VSET[2:0]	
VSET[5:3]
*/

const static int regulator_voltage_map[] = {	
 	600,  625,  650,  675,  700,  725,  750,  775, 	
 	800,  825,  850,  875,  900,  925,  950,  975,	
	1000, 1025, 1050, 1075, 1100, 1125, 1150, 1175,	
	1200, 1250, 1300, 1350, 1400, 1450, 1500, 1550,	
	1600, 1650, 1700, 1750, 1800, 1850, 1900, 1950,	
	2000, 2050, 2100, 2150, 2200, 2250, 2300, 2350,	
	2400, 2500, 2600, 2700, 2800, 2900, 3000, 3100,
	3200, 3300, 3400, 3500, 3600, 3700, 3800, 3900,
};


#define ACT8847_BUCK1_BASE 0x10
#define ACT8847_BUCK2_BASE 0x20
#define ACT8847_BUCK3_BASE 0x30
#define ACT8847_BUCK4_BASE 0x40

#define ACT8847_LDO5_BASE 0x50
#define ACT8847_LDO6_BASE 0x58
#define ACT8847_LDO7_BASE 0x60
#define ACT8847_LDO8_BASE 0x68
#define ACT8847_LDO9_BASE 0x70
#define ACT8847_LDO10_BASE 0x80
#define ACT8847_LDO11_BASE 0x90
#define ACT8847_LDO12_BASE 0xA0
#define ACT8847_LDO13_BASE 0xB0

const static int regulator_base_addr[] = {	
	ACT8847_BUCK1_BASE,	
	ACT8847_BUCK2_BASE,	
	ACT8847_BUCK3_BASE,	
	ACT8847_BUCK4_BASE,	
	ACT8847_LDO5_BASE,	
	ACT8847_LDO6_BASE,	
	ACT8847_LDO7_BASE,
	ACT8847_LDO8_BASE,
	ACT8847_LDO9_BASE,
	ACT8847_LDO10_BASE,
	ACT8847_LDO11_BASE,
	ACT8847_LDO12_BASE,
	ACT8847_LDO13_BASE,
};

#define ACT8847_BUCK_TARGET_VSET1_REG(x) (regulator_base_addr[x])
#define ACT8847_BUCK_TARGET_VSET2_REG(x) (regulator_base_addr[x]+1)
#define ACT8847_BUCK_TARGET_CON_REG(x)    (regulator_base_addr[x]+2)


#define BUCK_TARGET_VOL_MASK 0x3f
#define BUCK_TARGET_VOL_MIN_IDX 0x00
#define BUCK_TARGET_VOL_MAX_IDX 0x3f
#define BUCK_TARGET_DELAY_MASK  (0x7<<2)    /*for BUCK2 VDD_INT, BUCK3 VDD_ARM, DELAY*/

#define BUCK_ENABLE_BIT_MASK      (0x1<<7)
#define BUCK_PHASE_BIT_MASK       (0x1<<6)
#define BUCK_MODE_BIT_MASK        (0x1<<5)

#define ACT8847_LDO_TARGET_VSET_REG(x)   (regulator_base_addr[x])
#define ACT8847_LDO_TARGET_CON_REG(x)    (regulator_base_addr[x]+1)

#define LDO_TARGET_VOL_MASK  0x3f
#define LDO_TARGET_VOL_MIN_IDX 0x00
#define LDO_TARGET_VOL_MAX_IDX 0x3f
#define LDO_ENABLE_BIT_MASK     (0x1<<7)
#define LDO_DIS_BIT_MASK           (0x1<<6)
#define LDO_LOWIQ_BIT_MASK      (0x1<<5)

static int act8847_ldo_list_voltage(struct regulator_dev *dev, unsigned index)
{	
	//act_dbg("act8847_ldo_list_voltage index: %u\n", index);
	if(index < 0 || index >= dev->desc->n_voltages) 
		return -EINVAL;

	//act_dbg("act8847_ldo_list_voltage [%d] : %u mv\n", index, regulator_voltage_map[index] );
	return 1000 * regulator_voltage_map[index];
}

static int act8847_ldo_is_enabled(struct regulator_dev *dev)
{
	struct act8847 *act8847 = rdev_get_drvdata(dev);
	int ldo = rdev_get_id(dev) - ACT8847_DCDC1;
	u16 val;
	val = act8847_reg_read(act8847, ACT8847_LDO_TARGET_CON_REG(ldo));
	
	act_dbg("act8847_ldo_is_enabled [LDO%d] : %s\n",(ldo+1) ,(val & LDO_ENABLE_BIT_MASK)?"enabled":"disabled");
	
	return (val & LDO_ENABLE_BIT_MASK) != 0;
}

static int act8847_ldo_enable(struct regulator_dev *dev)
{
	struct act8847 *act8847 = rdev_get_drvdata(dev);
	int ldo = rdev_get_id(dev) - ACT8847_DCDC1;

	act_dbg("act8847_ldo_enable [LDO%d]\n",(ldo+1));
	
	return act8847_set_bits(act8847, ACT8847_LDO_TARGET_CON_REG(ldo), LDO_ENABLE_BIT_MASK, LDO_ENABLE_BIT_MASK);
}

static int act8847_ldo_disable(struct regulator_dev *dev)
{
	struct act8847 *act8847 = rdev_get_drvdata(dev);
	int ldo = rdev_get_id(dev) - ACT8847_DCDC1;

	act_dbg("act8847_ldo_disable [LDO%d] \n",(ldo+1));
	
	return act8847_set_bits(act8847, ACT8847_LDO_TARGET_CON_REG(ldo), LDO_ENABLE_BIT_MASK, 0);
}

static int act8847_ldo_get_voltage(struct regulator_dev *dev)
{
	struct act8847 *act8847 = rdev_get_drvdata(dev);
	int ldo = rdev_get_id(dev) - ACT8847_DCDC1;
	u16 val, reg;

	reg = act8847_reg_read(act8847, ACT8847_LDO_TARGET_VSET_REG(ldo));
	val = reg & LDO_TARGET_VOL_MASK;
	
	act_dbg("act8847_ldo_get_voltage [LDO%d] : %dmA\n",(ldo+1),regulator_voltage_map[val]);

	return 1000 * regulator_voltage_map[val];
}

static int act8847_ldo_set_voltage(struct regulator_dev *dev,
				  int min_uV, int max_uV, unsigned *selector)
{
	struct act8847 *act8847 = rdev_get_drvdata(dev);
	int ldo = rdev_get_id(dev) - ACT8847_DCDC1;
	int min_vol = min_uV / 1000, max_vol = max_uV / 1000;
	const int *vol_map = regulator_voltage_map;
	u16 val;

	if (min_vol < vol_map[LDO_TARGET_VOL_MIN_IDX] ||
	    min_vol > vol_map[LDO_TARGET_VOL_MAX_IDX])
		return -EINVAL;

	for (val = LDO_TARGET_VOL_MIN_IDX; val <= LDO_TARGET_VOL_MAX_IDX; val++)
		if (vol_map[val] >= min_vol)
			break;

	if (vol_map[val] > max_vol)
		return -EINVAL;
	
	act_dbg("act8847_ldo_set_voltage [LDO%d] : %dmA\n",(ldo+1),vol_map[val]);

  	return act8847_set_bits(act8847, ACT8847_LDO_TARGET_VSET_REG(ldo),
		LDO_TARGET_VOL_MASK, val);
}

static struct regulator_ops act8847_ldo_ops = {
	.list_voltage = act8847_ldo_list_voltage,
	.is_enabled = act8847_ldo_is_enabled,
	.enable = act8847_ldo_enable,
	.disable = act8847_ldo_disable,
	.get_voltage = act8847_ldo_get_voltage,
	.set_voltage = act8847_ldo_set_voltage,	
};

static int act8847_dcdc_list_voltage(struct regulator_dev *dev, unsigned index)
{
	//act_dbg("act8847_dcdc_list_voltage index: %u\n", index);
	if(index < 0 || index >= dev->desc->n_voltages) 
		return -EINVAL;

	//act_dbg("act8847_dcdc_list_voltage [%d] : %u mv\n", index, regulator_voltage_map[index] );
	return 1000 * regulator_voltage_map[index];
}

static int act8847_dcdc_is_enabled(struct regulator_dev *dev)
{
	struct act8847 *act8847 = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - ACT8847_DCDC1;
	u16 val;

	val = act8847_reg_read(act8847, ACT8847_BUCK_TARGET_CON_REG(buck));
	return (val & BUCK_ENABLE_BIT_MASK) != 0;
}

static int act8847_dcdc_enable(struct regulator_dev *dev)
{
	struct act8847 *act8847 = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - ACT8847_DCDC1;
	
	return act8847_set_bits(act8847, ACT8847_BUCK_TARGET_CON_REG(buck), BUCK_ENABLE_BIT_MASK, BUCK_ENABLE_BIT_MASK);
}

static int act8847_dcdc_disable(struct regulator_dev *dev)
{
	struct act8847 *act8847 = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - ACT8847_DCDC1;
	
	return act8847_set_bits(act8847, ACT8847_BUCK_TARGET_CON_REG(buck), BUCK_ENABLE_BIT_MASK, 0);
}

static int act8847_dcdc_get_voltage(struct regulator_dev *dev)
{
	struct act8847 *act8847 = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - ACT8847_DCDC1;
	u16 reg;
	int val;

	if(buck==0)
		reg = act8847_reg_read(act8847, 0x10);
	else
		reg = act8847_reg_read(act8847, ACT8847_BUCK_TARGET_VSET2_REG(buck));
	reg &= BUCK_TARGET_VOL_MASK;

	if (reg <= BUCK_TARGET_VOL_MAX_IDX)
		val = 1000 * regulator_voltage_map[reg];
	else {
		val = 0;
		dev_warn(&dev->dev, "chip reported incorrect voltage value.\n");
	}

	return val;
}

static int act8847_dcdc_set_voltage(struct regulator_dev *dev,
				  int min_uV, int max_uV, unsigned *selector)
{
	struct act8847 *act8847 = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - ACT8847_DCDC1  ;
	int min_vol = min_uV / 1000, max_vol = max_uV / 1000;
	const int *vol_map = regulator_voltage_map;
	u16 val;
	int ret;
	act_dbg("act8847_dcdc_set_voltage:buck=%d,min_vol=%dmv,max_vol=%dmv, \n", buck, min_vol, max_vol);
	if (min_vol < vol_map[BUCK_TARGET_VOL_MIN_IDX] ||
	    min_vol > vol_map[BUCK_TARGET_VOL_MAX_IDX])
		return -EINVAL;

	for (val = BUCK_TARGET_VOL_MIN_IDX; val <= BUCK_TARGET_VOL_MAX_IDX;
	     val++)
		if (vol_map[val] >= min_vol)
			break;

	if (vol_map[val] > max_vol){
		printk("%s:cannot find a suitable vol(%d-%d) for buck=%d,"
		      "use %d instead\n",__func__,min_vol,max_vol,buck,vol_map[val]);
		//return -EINVAL;
	}

	act_dbg("act8847_dcdc_set_voltage: index=%u, \n", val);


	if(buck==0){
		ret = act8847_set_bits(act8847, 0x10,BUCK_TARGET_VOL_MASK, val);
	}else{
		ret = act8847_set_bits(act8847, ACT8847_BUCK_TARGET_VSET2_REG(buck),
		       BUCK_TARGET_VOL_MASK, val);
	}
	#if 0
	if (ret)
		return ret;
	#endif

	// TODO
//	mdelay(10);
	udelay(10); 

	return ret;
}

static struct regulator_ops act8847_dcdc_ops = {
	.list_voltage = act8847_dcdc_list_voltage,
	.is_enabled = act8847_dcdc_is_enabled,
	.enable = act8847_dcdc_enable,
	.disable = act8847_dcdc_disable,
	.get_voltage = act8847_dcdc_get_voltage,
	.set_voltage = act8847_dcdc_set_voltage,
};


//***************** for dummy ******************
static int s_dummy_vol_enabled = 0;
static int s_dummy_vol_index = 0;

static int act8847_dummy_list_voltage(struct regulator_dev *dev, unsigned index)
{
	//act_dbg("act8847_dcdc_list_voltage index: %u\n", index);
	if(index < 0 || index >= dev->desc->n_voltages) 
		return -EINVAL;

	//act_dbg("act8847_dcdc_list_voltage [%d] : %u mv\n", index, regulator_voltage_map[index] );
	return 1000 * regulator_voltage_map[index];
}

static int act8847_dummy_is_enabled(struct regulator_dev *dev)
{
	return s_dummy_vol_enabled;
}

static int act8847_dummy_enable(struct regulator_dev *dev)
{
	s_dummy_vol_enabled = 1;
	return 0;
}

static int act8847_dummy_disable(struct regulator_dev *dev)
{
	s_dummy_vol_enabled = 0;
	return 0;
}

static int act8847_dummy_get_voltage(struct regulator_dev *dev)
{
	int val;
	if (s_dummy_vol_index <= BUCK_TARGET_VOL_MAX_IDX)
		val = 1000 * regulator_voltage_map[s_dummy_vol_index];
	else {
		val = 0;
	}
	return val;
}

static int act8847_dummy_set_voltage(struct regulator_dev *dev,
				  int min_uV, int max_uV, unsigned *selector)
{
	int min_vol = min_uV / 1000, max_vol = max_uV / 1000;
	const int *vol_map = regulator_voltage_map;
	u16 val;
	act_dbg("act8847_dummy_set_voltage:min_vol=%dmv,max_vol=%dmv, \n", min_vol, max_vol);

	if (min_vol < regulator_voltage_map[BUCK_TARGET_VOL_MIN_IDX] ||
	    min_vol > regulator_voltage_map[BUCK_TARGET_VOL_MAX_IDX])
		return -EINVAL;

	for (val = BUCK_TARGET_VOL_MIN_IDX; val <= BUCK_TARGET_VOL_MAX_IDX;
	     val++)
		if (vol_map[val] >= min_vol)
			break;

	if (vol_map[val] > max_vol)
		return -EINVAL;

	s_dummy_vol_index = val;

	return 0;
}

static struct regulator_ops act8847_dummy_ops = {
	.list_voltage = act8847_dummy_list_voltage,
	.is_enabled = act8847_dummy_is_enabled,
	.enable = act8847_dummy_enable,
	.disable = act8847_dummy_disable,
	.get_voltage = act8847_dummy_get_voltage,
	.set_voltage = act8847_dummy_set_voltage,	
};

//*********************************************

static struct regulator_desc regulators[] = {
	{
		.name = "DCDC1",
		.id = ACT8847_DCDC1,
		.ops = &act8847_dcdc_ops,
		.n_voltages = ARRAY_SIZE(regulator_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "DCDC2",
		.id = ACT8847_DCDC2,
		.ops = &act8847_dcdc_ops,
		.n_voltages = ARRAY_SIZE(regulator_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "DCDC3",
		.id = ACT8847_DCDC3,
		.ops = &act8847_dcdc_ops,
		.n_voltages = ARRAY_SIZE(regulator_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "DCDC4",
		.id = ACT8847_DCDC4,
		.ops = &act8847_dcdc_ops,
		.n_voltages = ARRAY_SIZE(regulator_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO5",
		.id = ACT8847_LDO5,
		.ops = &act8847_ldo_ops,
		.n_voltages = ARRAY_SIZE(regulator_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO6",
		.id = ACT8847_LDO6,
		.ops = &act8847_ldo_ops,
		.n_voltages = ARRAY_SIZE(regulator_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO7",
		.id = ACT8847_LDO7,
		.ops = &act8847_ldo_ops,
		.n_voltages = ARRAY_SIZE(regulator_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO8",
		.id = ACT8847_LDO8,
		.ops = &act8847_ldo_ops,
		.n_voltages = ARRAY_SIZE(regulator_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO9",
		.id = ACT8847_LDO9,
		.ops = &act8847_ldo_ops,
		.n_voltages = ARRAY_SIZE(regulator_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO10",
		.id = ACT8847_LDO10,
		.ops = &act8847_ldo_ops,
		.n_voltages = ARRAY_SIZE(regulator_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO11",
		.id = ACT8847_LDO11,
		.ops = &act8847_ldo_ops,
		.n_voltages = ARRAY_SIZE(regulator_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO12",
		.id = ACT8847_LDO12,
		.ops = &act8847_ldo_ops,
		.n_voltages = ARRAY_SIZE(regulator_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO13",
		.id = ACT8847_LDO13,
		.ops = &act8847_ldo_ops,
		.n_voltages = ARRAY_SIZE(regulator_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO_DUMMY",
		.id = ACT8847_DCDC_DUMMY,
		.ops = &act8847_dummy_ops,
		.n_voltages = ARRAY_SIZE(regulator_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
};


static int act8847_i2c_read(struct i2c_client *i2c, char reg, int count,
	u16 *dest)
{
	int ret;

	if (count != 1)
		return -EIO;
	ret = i2c_smbus_read_byte_data(i2c, reg);
	if (ret < 0 || count != 1)
		return -EIO;

	*dest = ret;
	return 0;
}

static int act8847_i2c_write(struct i2c_client *i2c, char reg, int count,
	const u16 *src)
{
	int ret;

	if (count != 1)
		return -EIO;
	ret = i2c_smbus_write_byte_data(i2c, reg, *src);
	if (ret >= 0)
		return 0;

	return ret;
}

static u8 act8847_reg_read(struct act8847 *act8847, u8 reg)
{
	u16 val = 0;

	mutex_lock(&act8847->io_lock);

	act8847_i2c_read(act8847->i2c, reg, 1, &val);

	dev_dbg(act8847->dev, "reg read 0x%02x -> 0x%02x\n", (int)reg,
		(unsigned)val&0xff);

	mutex_unlock(&act8847->io_lock);

	return val & 0xff;
}

static u8 act8847_reg_write(struct act8847 *act8847, u8 reg, u16 val)
{
	int ret;

	mutex_lock(&act8847->io_lock);
	
	ret = act8847_i2c_write(act8847->i2c, reg, 1, &val);
	
	dev_dbg(act8847->dev, "reg write 0x%02x -> 0x%02x\n", (int)reg,
		(unsigned)val&0xff);
	
	mutex_unlock(&act8847->io_lock);

	return ret;
}

static int act8847_set_bits(struct act8847 *act8847, u8 reg, u16 mask, u16 val)
{
	u16 tmp;
	int ret;

	mutex_lock(&act8847->io_lock);

	ret = act8847_i2c_read(act8847->i2c, reg, 1, &tmp);
	act_dbg("read reg[0x%02x]=0x%02x\n,ret=%d\n",(int)reg,(unsigned)tmp&0xff,ret);
	tmp = (tmp & ~mask) | val;
	if (ret == 0) {
		ret = act8847_i2c_write(act8847->i2c, reg, 1, &tmp);
		act_dbg( "reg write 0x%02x -> 0x%02x\n", (int)reg,
			(unsigned)val&0xff);

	}
	mutex_unlock(&act8847->io_lock);

	return ret;
}

/*
   act8847_i2c ops for charger
*/
int act8847_write(unsigned int adr,  unsigned int val)
{
	if (!g_act8847 && !g_act8847->i2c)
	{
		printk("act8847 Write Error: i2c is not ready.\n");
		return -EAGAIN;
	}

	return act8847_reg_write(g_act8847, adr, val);
}
EXPORT_SYMBOL_GPL(act8847_write);

unsigned int act8847_read(unsigned int adr)
{
	if (!g_act8847 && !g_act8847->i2c)
	{
		printk("act8847 Write Error: i2c is not ready.\n");
		return -EAGAIN;
	}
	
	return act8847_reg_read(g_act8847, adr);
}
EXPORT_SYMBOL_GPL(act8847_read);


static char g_selected_ddr[32] = "50";
static int __init select_ddr(char *str)
{
	printk(KERN_DEBUG"%s: str=%s\n", __func__, str);
	strcpy(g_selected_ddr, str);
	return 0;
}
__setup("ddr=", select_ddr); /* 1=1.2v, 2=1.25v, 3=1.3v, 4=1.35v, 5=1.4v, 6=1.45v, 7=1.5v */

static void set_ddr_volt(void)
{
	unsigned int reg_val = 0x1C;  //1.4v
	switch(g_selected_ddr[0]) {
		case '1':   //1.2v
			reg_val = 0x18;
			break;
			
		case '2':   //1.25v
			reg_val = 0x19;
			break;
			
		case '3':   //1.3v
			reg_val = 0x1a;
			break;

		case '4':   //1.35v
			reg_val = 0x1b;
			break;
			
		default:	
		case '5'://1.4v
			reg_val = 0x1c;  
			break;

		case '6':   //1.45v
			reg_val = 0x1d;
			break;
			
		case '7':   //1.5v
			reg_val = 0x1e;
			break;		
	}
	
	act8847_write(0x10, reg_val); 
//	act8847_write(0x10, 0x14); //1.1V  is not run
	
	reg_val = act8847_read(0x12); // enable volt	
	reg_val |= (1<<7);
	act8847_write(0x12, reg_val);
}

static void set_ddrQ_volt(struct act8847_platform_data *pdata, int no)
{
	unsigned int uV_temp = 1300000;  //1.3v
	switch(g_selected_ddr[1]) {
		case '1':   //1.2v
			uV_temp = 1200000;
			break;
			
		case '2':   //1.25v
			uV_temp = 1250000;
			break;
			
		case '3':   //1.3v
			uV_temp = 1300000;
			break;

		case '4':   //1.35v
			uV_temp = 1350000;
			break;
			
//		default:	
		case '5'://1.4v
			uV_temp = 1400000;  
			break;

		case '6':   //1.45v
			uV_temp = 1450000;
			break;
			
		case '7':   //1.5v
			uV_temp = 1500000;
			break;		
	}
	
	printk(KERN_DEBUG"%s().  ldo10 min=%d, max=%d, uV=%d, uV_temp=%d, [0x%x]\n "
		, __func__
		, pdata->regulators[no].initdata->constraints.min_uV
		, pdata->regulators[no].initdata->constraints.max_uV
		, pdata->regulators[no].initdata->constraints.state_mem.uV
		, uV_temp
		, g_selected_ddr[1]
	);
	pdata->regulators[no].initdata->constraints.min_uV = uV_temp;
	pdata->regulators[no].initdata->constraints.max_uV = uV_temp;
	pdata->regulators[no].initdata->constraints.state_mem.uV= uV_temp;
	
}

static int setup_regulators(struct act8847 *act8847,
	struct act8847_platform_data *pdata)
{
	int i, err;
	int num_regulators = pdata->num_regulators;
	act8847->num_regulators = num_regulators;
	act8847->rdev = kzalloc(sizeof(struct regulator_dev *) * num_regulators,
		GFP_KERNEL);

	/* Instantiate the regulators */
	for (i = 0; i < num_regulators; i++) {
		int id = pdata->regulators[i].id;

		if(id==ACT8847_LDO10) {
			set_ddrQ_volt(pdata, i);
		}


		act8847->rdev[i] = regulator_register(&regulators[id],
			act8847->dev, pdata->regulators[i].initdata, act8847);

		err = IS_ERR(act8847->rdev[i]);
		if (err) {
			dev_err(act8847->dev, "regulator init failed: %d\n",
				err);
			goto error;
		}
	}

	return 0;
error:
	for (i = 0; i < num_regulators; i++)
		if (act8847->rdev[i])
			regulator_unregister(act8847->rdev[i]);
	kfree(act8847->rdev);
	act8847->rdev = NULL;
	return err;
}



static int __devinit act8847_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	//struct act8847 *act8847;
	struct act8847_platform_data *pdata = i2c->dev.platform_data;
	int ret;

	act_dbg("act8847_i2c_probe\n");

	g_act8847 = kzalloc(sizeof(struct act8847), GFP_KERNEL);
	if (g_act8847 == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	g_act8847->i2c = i2c;
	g_act8847->dev = &i2c->dev;
	i2c_set_clientdata(i2c, g_act8847);

	mutex_init(&g_act8847->io_lock);

#if 0 //raymanfeng
	/* Detect ACT8847 */
	u16 val;
	ret = act8847_i2c_read(i2c, ACT8847_SYS_CONTROL1_REG, 1, &val);
	if (ret == 0 && (val & SYS_CONTROL1_INIT_MASK) != SYS_CONTROL1_INIT_VAL)
		ret = -ENODEV;
	if (ret < 0) {
		dev_err(&i2c->dev, "failed to detect device\n");
		goto err_detect;
	}

	/*here to restore vset2 related voltages to default value, otherwise hardware reset may fail*/
	act8847_write(0x41, 0x19); //vdd_int 1.25V
	act8847_write(0x31, 0x14); //vdd_arm 1.1V
	act8847_write(0x21, 0x39); //vdd_mem 3.3V
#endif

	//prepare for vset2
	act8847_write(0x21, 0x1a); //vdd_arm 1.3V
	act8847_write(0x31, 0x10); //vdd_3D 1.0V

	if (!is_act_pmu_class2) {
		set_ddr_volt();
	}
	mdelay(2);
	
	act_dbg("Raymanfeng C1=0x%02x\n", act8847_read(0xC1));
	act_dbg("Raymanfeng C2=0x%02x\n", act8847_read(0xC2));
	act_dbg("Raymanfeng C3=0x%02x\n", act8847_read(0xC3));

	//pull GPM3_5 up for using vset2, vset1 is used for reset case
	{
		int err;
		err = gpio_request(EXYNOS4212_GPM3(5), "GPM3_5 PMU ARM Set");
		if (err)
			printk(KERN_ERR "#### failed to request GPH3_5\n");

		gpio_direction_output(EXYNOS4212_GPM3(5), 1);
		gpio_free(EXYNOS4212_GPM3(5));
	}

	//pull GPM3_6 up for using vset2, vset1 is used for reset case
	{
		int err;
		err = gpio_request(EXYNOS4212_GPM3(6), "GPM3_6 PMU 3D Set");
		if (err)
			printk(KERN_ERR "#### failed to request GPH3_6\n");

		gpio_direction_output(EXYNOS4212_GPM3(6), 1);
		gpio_free(EXYNOS4212_GPM3(6));
	}


	if (pdata) {
		ret = setup_regulators(g_act8847, pdata);
		if (ret < 0)
			goto err_detect;
	} else
		dev_warn(g_act8847->dev, "No platform init data supplied\n");

	act_dbg("act8847_i2c_probe ok\n");
	return 0;

err_detect:
	i2c_set_clientdata(i2c, NULL);
	kfree(g_act8847);
err:
	return ret;
}

static int __devexit act8847_i2c_remove(struct i2c_client *i2c)
{
	struct act8847 *act8847 = i2c_get_clientdata(i2c);
	int i;
	for (i = 0; i < act8847->num_regulators; i++)
		if (act8847->rdev[i])
			regulator_unregister(act8847->rdev[i]);
	kfree(act8847->rdev);
	i2c_set_clientdata(i2c, NULL);
	kfree(act8847);

	return 0;
}

static const struct i2c_device_id act8847_i2c_id[] = {
       { "act8847", 0 },
       { }
};
MODULE_DEVICE_TABLE(i2c, act8847_i2c_id);

static struct i2c_driver act8847_i2c_driver = {
	.driver = {
		.name = "act8847",
		.owner = THIS_MODULE,
	},
	.probe    = act8847_i2c_probe,
	.remove   = __devexit_p(act8847_i2c_remove),
	.id_table = act8847_i2c_id,
};


static int __init act8847_module_init(void)
{
	int ret = -1;

	printk("%s(); +\n", __func__);
	
	if ( is_act_pmu )  {
		ret = i2c_add_driver(&act8847_i2c_driver);
		if (ret != 0)
			pr_err("Failed to register I2C driver: %d\n", ret);
	}
	
	printk("%s(); ret=%d -\n", __func__, ret);

	return ret;
}

static void __exit act8847_module_exit(void)
{
	i2c_del_driver(&act8847_i2c_driver);
}

device_initcall(act8847_module_init);

//device_initcall(act8847_module_init);
//subsys_initcall(act8847_module_init);
//late_initcall(act8847_module_init);
module_exit(act8847_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marek Szyprowski <m.szyprowski@samsung.com>");
MODULE_DESCRIPTION("ACT8847 PMIC driver");
