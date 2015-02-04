#ifndef POWER_GPIO_INCLUDED
#define POWER_GPIO_INCLUDED
/*
 *  misc power and gpio proc file for Urbest
 *  Copyright (c) 2009 Urbest Technologies, Inc.
 */

enum power_item
{
	POWER_5V_EN=0,
#if 0
	POWER_ARM_EN,
	POWER_3D_EN,
#endif
	POWER_INT_EN,

//	POWER_CAM_EN,
//	POWER_CAMA_PD, //camera A power down
//	POWER_CAMB_PD, //camera B power down
//	POWER_CAM_RST, //camera reset
/*
	POWER_BT_EN,
	POWER_BT_RST,
	POWER_BT_LDO,
	POWER_BT_WUP,	//cpu wake up bt
*/
	POWER_GPS_RST,
	POWER_GPS_EN,
#if 1
	POWER_WIFI_EN,	//wifi power enable
	POWER_WIFI_LDO,   //wifi inter ldo enable
#endif
	POWER_GSM_SW,
	POWER_GSM_WUP,  //cpu wake up 3g/gsm

	POWER_LCD33_EN,
	POWER_LCD18_EN,
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
	POWER_RFID_EN,

	POWER_FCAM_28V,   // front camera
	POWER_FCAM_18V,
	POWER_FCAM_PD,
	POWER_FCAM_RST,

	POWER_BCAM_28V,  // back camera
	POWER_BCAM_18V,
	POWER_BCAM_PD,
	POWER_BCAM_RST,

	POWER_CAM_AF,

	POWER_ITEM_MAX,
	POWER_ITEM_INVALID,
};

//read ON/OFF status
int read_power_item_value(int index);

//set ON/OFF status immediately
int write_power_item_value(int index, int value);


struct power_gpio_node
{
	enum power_item index;
	unsigned int pin;
	int polarity;				    //0: low for enable, 1: high for enable
	int direction;				    //0: input only,     1: input/output
	int (*write_func)(struct power_gpio_node , int);		//extra power function
	int (*read_func)(struct power_gpio_node);		//extra read function
};

struct power_gpio_oem
{
	const struct power_gpio_node * pnode;
	int count;
	char * name;
	char * pcb;
	void (*power_gpio_boot_init)(const struct power_gpio_oem *);
};

int power_gpio_register(const struct power_gpio_oem * p);
int power_gpio_unregister(const struct power_gpio_oem * p);


#define SIMPLE_POWER_GPIO_OEM(node,_name,_pcb,func)  \
static const struct power_gpio_oem __gpio_oem_##_name##_v##_pcb = {  \
	.pnode = node,     \
	.name  = __stringify(_name),      \
	.pcb = __stringify(_pcb),    \
	.power_gpio_boot_init = func,   \
	.count = ARRAY_SIZE(node),    \
}


#define MODULE_POWER_GPIO(NODE)  \
static int __init __power_gpio_reg(void)  \
{                                                    \
	return power_gpio_register(&(NODE));  \
}       \
static void __exit __power_gpio_unreg(void)        \
{                                                             \
	power_gpio_unregister(&(NODE)); \
}        \
fs_initcall(__power_gpio_reg);   \
module_exit(__power_gpio_unreg)


#define REGISTER_POWER_GPIO(node,_name,_pcb,func)  \
SIMPLE_POWER_GPIO_OEM(node,_name,_pcb,func); \
MODULE_POWER_GPIO(__gpio_oem_##_name##_v##_pcb)

#endif //POWER_GPIO_INCLUDED

