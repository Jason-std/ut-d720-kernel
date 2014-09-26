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
		
	POWER_CAM_EN,
	POWER_CAMA_PD, //camera A power down
	POWER_CAMB_PD, //camera B power down
	POWER_CAM_RST, //camera reset
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

	POWER_ITEM_MAX,
	POWER_ITEM_INVALID,
};

//read ON/OFF status
int read_power_item_value(int index);

//set ON/OFF status immediately
int write_power_item_value(int index, int value);

//set ON/OFF status with debounce in milliseconds.
int post_power_item_value(int index, int value, int ms_debounce);
#endif //POWER_GPIO_INCLUDED

