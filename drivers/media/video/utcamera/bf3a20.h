/* linux/drivers/media/video/bf3a20.h  <---  hi253
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *	         http://www.samsung.com/
 *
 * Driver for bf3a20 (UXGA camera) from Samsung Electronics
 * 1/4" 2.0Mp CMOS Image Sensor SoC with an Embedded Image Processor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __BF3A20_H__
#define __BF3A20_H__

#define HM2056_TABLE_END 0xFFFE
#define HM5056_TABLE_END HM2056_TABLE_END

struct bf3a20_reg {
	unsigned short addr;
	unsigned char val;
};

struct bf3a20_regset_type {
	unsigned char *regset;
	int len;
};

/*
 * Macro
 */
#define REGSET_LENGTH(x)	(sizeof(x)/sizeof(bf3a20_reg))

/*
 * User defined commands
 */
/* S/W defined features for tune */
#define REG_DELAY	0x0000	/* in ms */
#define REG_CMD		0xFFFF	/* Followed by command */
#define REG_ID		0x8899	/* Followed by command */


/* Following order should not be changed */
enum image_size_bf3a20 {
	/* This SoC supports upto UXGA (1600*1200) */
	QQVGA,	/* 160*120 */
	QCIF,	/* 176*144 */
	QVGA,	/* 320*240 */
	CIF,	/* 352*288 */
	VGA,	/* 640*480 */
	SVGA,	/* 800*600 */
	HD720P,	/* 1280*720 */
	SXGA,	/* 1280*1024 */
	UXGA,	/* 1600*1200 */
};

/*
 * Following values describe controls of camera
 * in user aspect and must be match with index of bf3a20_regset[]
 * These values indicates each controls and should be used
 * to control each control
 */
enum bf3a20_control {
	bf3a20_INIT,
	bf3a20_EV,
	bf3a20_AWB,
	bf3a20_MWB,
	bf3a20_EFFECT,
	bf3a20_CONTRAST,
	bf3a20_SATURATION,
	bf3a20_SHARPNESS,
};

#define bf3a20_REGSET(x)	{	\
	.regset = x,			\
	.len = sizeof(x)/sizeof(bf3a20_reg),}

static unsigned short  bf3a20_init_reg_640_480[][2] = {

	{0x1b,0x2c},
	{0x11,0x38},
	{0x8a,0x7f},
	{0x2a,0x0c},
	{0x2b,0x1c},
	{0x92,0x02},
	{0x93,0x00},

	{0x4a,0x80},
	{0xcc,0x00},
	{0xca,0x00},
	{0xcb,0x00},
	{0xcf,0x00},
	{0xcd,0x00},
	{0xce,0x00},
	{0xc0,0xd3},
	{0xc1,0x04},
	{0xc2,0xc4},
	{0xc3,0x11},
	{0xc4,0x3f},
	{0xb5,0x3f},
	{0x03,0x50},
	{0x17,0x00},
	{0x18,0x00},
	{0x10,0x30},
	{0x19,0x00},
	{0x1a,0xc0},
	{0x0B,0x10},

	{HM2056_TABLE_END, 0x00}
};
#define bf3a20_init_reg_640_480S	\
	(sizeof(bf3a20_init_reg_640_480) / sizeof(bf3a20_init_reg_640_480[0]))

//////////////////////////////////////////not suport yet////////////////////////////////////////////////////////////////////
static unsigned short  bf3a20_init_reg_800_600[][2] = {};

#define bf3a20_init_reg_800_600S	\
	(sizeof(bf3a20_init_reg_800_600) / sizeof(bf3a20_init_reg_800_600[0]))

static unsigned short  bf3a20_init_reg_960_720[][2] = {};

#define bf3a20_init_reg_960_720S	\
	(sizeof(bf3a20_init_reg_960_720) / sizeof(bf3a20_init_reg_960_720[0]))

static unsigned short  bf3a20_init_reg_1280_720[][2] = {
	{0x4a,0x80},
	{0xc0,0xd2},
	{0xc1,0x04},
	{0xc2,0xd4},
	{0xc3,0x11},
	{0xc4,0x3f},
	{0xb5,0x3f},
	{0x03,0x50},
	{0x17,0x00},
	{0x18,0x00},
	{0x10,0x20},
	{0x19,0x00},
	{0x1a,0xd0},
	{0x0b,0x00},
	//0x1b2c,
	//0x1138,
	//0x8abf,
	{0x89,0x35},
	{HM2056_TABLE_END, 0x00},
};

#define bf3a20_init_reg_1280_720S	\
	(sizeof(bf3a20_init_reg_1280_720) / sizeof(bf3a20_init_reg_1280_720[0]))

//////////////////////////////////////////not suport yet ,endline////////////////////////////////////////////////////////////////////



static unsigned short  bf3a20_init_reg_1600_1200[][2] = {

	{0x1b,0x2c},
	{0x11,0x38},
	{0x8a,0x7d},
	{0x2a,0x0c},
	{0x2b,0x1c}, 
	{0x92,0x02},
	{0x93,0x00},
						
	{0x4a,0x80}, 
	{0xcc,0x00}, 
	{0xca,0x00}, 
	{0xcb,0x00}, 
	{0xcf,0x00}, 
	{0xcd,0x00}, 
	{0xce,0x00}, 
	{0xc0,0x00}, 
	{0xc1,0x00}, 
	{0xc2,0x00}, 
	{0xc3,0x00}, 
	{0xc4,0x00}, 
	{0xb5,0x00}, 
	{0x03,0x60}, 
	{0x17,0x00}, 
	{0x18,0x40}, 
	{0x10,0x40}, 
	{0x19,0x00}, 
	{0x1a,0xb0}, 
	{0x0B,0x00},

	{HM2056_TABLE_END, 0x00}
};

#define bf3a20_init_reg_1600_1200S	\
	(sizeof(bf3a20_init_reg_1600_1200) / sizeof(bf3a20_init_reg_1600_1200[0]))
/*
 * User tuned register setting values
 */
#if 1
static unsigned short  bf3a20_init_reg[][2] = {
	{0x09,0x42},
	{0x29,0x60},
	{0x12,0x00},
	{0x13,0x07},
	{0xe2,0xc4},                
	{0xe7,0x4e},  
	{0x29,0x60},
	{0x08,0x16}, 
	{0x16,0x28},  
	{0x1c,0x00},                
	{0x1d,0xf1},   
	{0xbb,0x30},
	{0x3a,0x40},
	{0x3e,0x80},   
	{0x27,0x98},
	{0x35,0x30}, //lens shading gain of R               
  {0x65,0x30}, //lens shading gain of G                
	{0x66,0x2a}, //lens shading gain of B
	{0xbc,0xd3},                 
	{0xbd,0x5c},                
	{0xbe,0x24},                 
	{0xbf,0x13},	              
	{0x9b,0x5c},                 
	{0x9c,0x24},      
	{0x36,0x13},				  
	{0x37,0x5c},				   
	{0x38,0x24},	
	
	{0x92,0x02},//Dummy Line Insert LSB
	{0x93,0x00},
	{0x2a,0x0c},
	{0x2b,0x40},//Dummy Pixel Insert LSB
  
  {0x13,0x00},
	{0x8c,0x0b},  
	{0x8d,0xc4},  
	{0x24,0xc8},
	{0x87,0x2d},
	{0x01,0x12},
	{0x02,0x22},
	{0x13,0x07},

	{0x70,0x01},                 
	{0x72,0x62},//0x72[7:6]: Bright edge enhancement; 0x72[5:4]: Dark edge enhancement      
	{0x78,0x37},				  
	{0x7a,0x29},//0x7a[7:4]: The bigger, the smaller noise; 0x7a[3:0]: The smaller, the smaller noise				   
	{0x7d,0x85},
   
	{0x39,0xa8}, //0xa0 zzg 
	{0x3f,0x28}, //0x20 zzg 
 
 
   
	{0x40,0x28},
	{0x41,0x26},
	{0x42,0x24},
	{0x43,0x22},
	{0x44,0x1c},
	{0x45,0x19},
	{0x46,0x11},
	{0x47,0x10},
	{0x48,0x0f},
	{0x49,0x0e},
	{0x4b,0x0c},
	{0x4c,0x0a},
	{0x4e,0x09},
	{0x4f,0x08},
	{0x50,0x04},  
	{0x5c,0x00},
	{0x51,0x32},
	{0x52,0x17},
	{0x53,0x8C},
	{0x54,0x79},
	{0x57,0x6E},
	{0x58,0x01},
	{0x5a,0x36},
	{0x5e,0x38},
	{0x56,0x28},
	{0x55,0x00},//when 0x56[7:6]=2'b0x,0x55[7:0]:Brightness control.bit[7]: 0:positive  1:negative [6:0]:value	 
	{0xb0,0x8d},
	{0xb1,0x15},//when 0x56[7:6]=2'b0x,0xb1[7]:auto contrast switch(0:on 1:off)		      
	{0x56,0xe8},
	{0x55,0xf0},//when 0x56[7:6]=2'b11,cb coefficient of F light
	{0xb0,0xb8},//when 0x56[7:6]=2'b11,cr coefficient of F light
	{0x56,0xa8},
	{0x55,0xc8},//when 0x56[7:6]=2'b10,cb coefficient of !F light
	{0xb0,0x98},//when 0x56[7:6]=2'b10,cr coefficient of !F light 
   	 
	{0x56,0x28},
	{0xb1,0x95},//when 0x56[7:6]=2'b0x,0xb1[7]:auto contrast switch(0:on 1:off)
	{0x56,0xe8},
	{0xb4,0x40},//when 0x56[7:6]=2'b11,manual contrast coefficient 
  
	{0x13,0x07},
	{0x24,0x40},// AE Target 
	{0x25,0x88},// Bit[7:4]: AE_LOC_INT; Bit[3:0]:AE_LOC_GLB  0x88
	{0x97,0x30},// Y target value1.   
	{0x98,0x3b},// AE 窗口和权重  0x0a zzg 
	{0x80,0xd2},//Bit[3:2]: the bigger, Y_AVER_MODIFY is smaller     0x92
	{0x81,0xff},//AE speed  //0xe0
	{0x82,0x30},//minimum global gain   0x30 zzg 
	{0x83,0x60}, //0x60 zzg 
	{0x84,0x45}, ///0x65 zzg 
	{0x85,0x6d}, ///0x90 zzg 
	{0x86,0xb0},//maximum gain zzg 0xc0
	{0x89,0x3d},//INT_MAX_MID    9d zzg
	{0x8a,0x7d},//50 hz banding 
	{0x94,0xc2},//Bit[7:4]: Threshold for over exposure pixels, the smaller, the more over exposure pixels; Bit[3:0]: Control the start of AE. 0x42 zzg
  
	{0x6a,0x81},
	{0x23,0x66},// green gain 
	{0xa1,0x31},
	{0xa2,0x13},//the low limit of blue gain for indoor scene
	{0xa3,0x25},//the upper limit of blue gain for indoor scene
	{0xa4,0x13},//the low limit of red gain for indoor scene
	{0xa5,0x26},//the upper limit of red gain for indoor scene
	{0xa7,0x9a},//Base B gain不建议修改      
	{0xa8,0x15},//Base R gain不建议修改         
	{0xa9,0x13},
	{0xaa,0x12},
	{0xab,0x16},
	{0xac,0x20}, //avoiding red scence when openning
	{0xc8,0x10},
	{0xc9,0x15},
	{0xd2,0x78},
	{0xd4,0x20},
           
	{0xee,0x2a},
	{0xef,0x1b},
  
 	{0x4a,0x80}, 
	{0xc0,0x00}, 
	{0x0b,0x00}, 
	{0x03,0x60}, 
	{0x17,0x00}, 
	{0x18,0x40}, 
	{0x10,0x40}, 
	{0x19,0x00}, 
	{0x1a,0xb0}, 

	{0x4a,0x80},
	{0xc0,0x00},
	{0x0b,0x10},
	{0x03,0x20},
	{0x17,0x00},
	{0x18,0x80},
	{0x10,0x10},
	{0x19,0x00},
	{0x1a,0xe0},
	
	{0x1b,0x2c},
	{0x11,0x38},//PLL setting  0x38:3倍频;  0x34:2.5倍频； 0x30:2倍频; 0x2c：1.5倍频；  0x28:1倍频   
  
	
 	{0xdb,0x80}, 
	{0xf8,0x83}, 
	{0xde,0x1e}, 
	{0xf9,0xc0}, 
	{0x59,0x18}, 
	{0x5f,0x28}, 
	{0x6b,0x70}, 
	{0x6c,0x00}, 
	{0x6d,0x10}, 
	{0x6e,0x08}, 
	{0xf9,0x80}, 
	{0x59,0x00}, 
	{0x5f,0x10}, 
	{0x6b,0x00}, 
	{0x6c,0x28}, 
	{0x6d,0x08}, 
	{0x6e,0x00}, 
	{0x21,0x55}, 
	
	{0x15,0xf2}, 
//	{0x1e,0x46}, 	//raymanfeng: 0 flip
	{0x1e,0x76}, 	//raymanfeng: V flip

	{0xd0,0x11}, 
	{0xd1,0x90}, 



	{0x80,0xd0}, // 50/60Hz
	{0x8a,0x68}, // 50/60Hz


	{HM2056_TABLE_END, 0x00}
};
#endif

#define bf3a20_INIT_REGS	\
	(sizeof(bf3a20_init_reg) / sizeof(bf3a20_init_reg[0]))

/*
 * EV bias --- exposure --- Brightness
 */

static const struct bf3a20_reg bf3a20_ev_m6[] = {

};

static const struct bf3a20_reg bf3a20_ev_m5[] = {

};

static const struct bf3a20_reg bf3a20_ev_m4[] = {
};

static const struct bf3a20_reg bf3a20_ev_m3[] = {

	{0x56,0x28},
	{0x55,0x48},	//exposure -3
//	{HM2056_TABLE_END, 0x00},
};

static const struct bf3a20_reg bf3a20_ev_m2[2] = {  

	{0x56,0x28},
	{0x55,0x30},	//exposure -2
//	{HM2056_TABLE_END, 0x00},

};

static const struct bf3a20_reg bf3a20_ev_m1[2] = {  

	{0x56,0x28},
	{0x55,0x18},	//exposure -1
//	{HM2056_TABLE_END, 0x00},	
};

static const struct bf3a20_reg bf3a20_ev_default[2] = {   
	{0x56,0x28},
	{0x55,0x00,}
//	{HM2056_TABLE_END, 0x00},
};

static const struct bf3a20_reg bf3a20_ev_p1[2] = {  
	{0x56,0x28},
	{0x55,0x98},	//+1
//	{HM2056_TABLE_END, 0x00},

};

static const struct bf3a20_reg bf3a20_ev_p2[2] = {  

	{0x56,0x28},
	{0x55,0xb0},	//exposure +2
//	{HM2056_TABLE_END, 0x0}, 
};


static const struct bf3a20_reg bf3a20_ev_p3[] = {

	{0x56,0x28},
	{0x55,0xc8},	//exposure +3
//	{HM2056_TABLE_END, 0x00},
};

static const struct bf3a20_reg bf3a20_ev_p4[] = {
};

static const struct bf3a20_reg bf3a20_ev_p5[] = {
};

static const struct bf3a20_reg bf3a20_ev_p6[] = {
};


/* Order of this array should be following the querymenu data */
static const unsigned char *bf3a20_regs_ev_bias[] = {
	(unsigned char *)bf3a20_ev_m6, (unsigned char *)bf3a20_ev_m5,
	(unsigned char *)bf3a20_ev_m4, (unsigned char *)bf3a20_ev_m3,
	(unsigned char *)bf3a20_ev_m2, (unsigned char *)bf3a20_ev_m1,
	(unsigned char *)bf3a20_ev_default, (unsigned char *)bf3a20_ev_p1,
	(unsigned char *)bf3a20_ev_p2, (unsigned char *)bf3a20_ev_p3,
	(unsigned char *)bf3a20_ev_p4, (unsigned char *)bf3a20_ev_p5,
	(unsigned char *)bf3a20_ev_p6,
};

/*
 * Auto White Balance configure
 */
static const struct bf3a20_reg bf3a20_awb_off[] = {

};

static const struct bf3a20_reg bf3a20_awb_on[] = {
	//    {0xfe, 0x00},
	//    {0x82, 0xfc},
};

static const unsigned char *bf3a20_regs_awb_enable[] = {
	(unsigned char *)bf3a20_awb_off,
	(unsigned char *)bf3a20_awb_on,
};

/*
 * Manual White Balance (presets)
 */
static const struct bf3a20_reg  bf3a20_wb_auto[] = {//update
	{0x23,0x55},
	{0x01,0x0F},
	{0x02,0x13},
};

static const struct bf3a20_reg bf3a20_wb_tungsten[] = {
	{0x13, 0x05},
  	{0x01, 0x20},
	{0x02, 0x10},
	{0x23, 0x66},
//	{HM2056_TABLE_END, 0x00}
};

static const struct bf3a20_reg bf3a20_wb_fluorescent[] = {
	{0x13, 0x05},
  	{0x01, 0x1a},
	{0x02, 0x1e},
	{0x23, 0x66},
//	{HM2056_TABLE_END, 0x00,}
};

static const struct bf3a20_reg bf3a20_wb_sunny[] = {
	{0x13, 0x05},
  	{0x01, 0x10},
	{0x02, 0x20},
	{0x23, 0x66},
//	{HM2056_TABLE_END, 0x00}
};

static const struct bf3a20_reg bf3a20_wb_cloudy[] = { // 
	{0x13, 0x05},
  	{0x01, 0x0e},
	{0x02, 0x24},
	{0x23, 0x66},
//	{HM2056_TABLE_END, 0x00}
};

/* Order of this array should be following the querymenu data */
static const unsigned char *bf3a20_regs_wb_preset[] = {
	(unsigned char *)bf3a20_wb_sunny,
	(unsigned char *)bf3a20_wb_cloudy,
	(unsigned char *)bf3a20_wb_tungsten,
	(unsigned char *)bf3a20_wb_fluorescent,
};

/*
 * Color Effect (COLORFX)
 */
static const struct bf3a20_reg bf3a20_color_normal[] = {
		{0xfe, 0x00},
		{0x83, 0xe0},
};

static const struct bf3a20_reg bf3a20_color_monochrome[] = {
	//gray
		{0xfe, 0x00},
		{0x83, 0x12},		
};

static const struct bf3a20_reg bf3a20_color_sepia[] = {
		{0xfe, 0x00},
		{0x83, 0x82},

};

static const struct bf3a20_reg bf3a20_color_aqua[] = {
	//bluish
		{0xfe, 0x00},
		{0x83, 0x62},
};

static const struct bf3a20_reg bf3a20_color_negative[] = {
		{0xfe, 0x00},
		{0x83, 0x01},
};

static const struct bf3a20_reg bf3a20_color_sketch[] = {
	//greenish
		{0xfe, 0x00},
		{0x83, 0x52},
};
	


/* Order of this array should be following the querymenu data */
static const unsigned char *bf3a20_regs_color_effect[] = {
	(unsigned char *)bf3a20_color_normal,
	(unsigned char *)bf3a20_color_monochrome,
	(unsigned char *)bf3a20_color_sepia,
	(unsigned char *)bf3a20_color_aqua,
	(unsigned char *)bf3a20_color_sketch,
	(unsigned char *)bf3a20_color_negative,
};

/*
 * Contrast bias
 */
static const struct bf3a20_reg bf3a20_contrast_m2[] = {
};

static const struct bf3a20_reg bf3a20_contrast_m1[] = {
};

static const struct bf3a20_reg bf3a20_contrast_default[] = {
};

static const struct bf3a20_reg bf3a20_contrast_p1[] = {
};

static const struct bf3a20_reg bf3a20_contrast_p2[] = {
};

static const unsigned char *bf3a20_regs_contrast_bias[] = {
	(unsigned char *)bf3a20_contrast_m2,
	(unsigned char *)bf3a20_contrast_m1,
	(unsigned char *)bf3a20_contrast_default,
	(unsigned char *)bf3a20_contrast_p1,
	(unsigned char *)bf3a20_contrast_p2,
};

/*
 * Saturation bias
 */
static const struct bf3a20_reg bf3a20_saturation_m2[] = {
};

static const struct bf3a20_reg bf3a20_saturation_m1[] = {
};

static const struct bf3a20_reg bf3a20_saturation_default[] = {
};

static const struct bf3a20_reg bf3a20_saturation_p1[] = {
};

static const struct bf3a20_reg bf3a20_saturation_p2[] = {
};

static const unsigned char *bf3a20_regs_saturation_bias[] = {
	(unsigned char *)bf3a20_saturation_m2,
	(unsigned char *)bf3a20_saturation_m1,
	(unsigned char *)bf3a20_saturation_default,
	(unsigned char *)bf3a20_saturation_p1,
	(unsigned char *)bf3a20_saturation_p2,
};

/*
 * Sharpness bias
 */
static const struct bf3a20_reg bf3a20_sharpness_m2[] = {
};

static const struct bf3a20_reg bf3a20_sharpness_m1[] = {
};

static const struct bf3a20_reg bf3a20_sharpness_default[] = {
};

static const struct bf3a20_reg bf3a20_sharpness_p1[] = {
};

static const struct bf3a20_reg bf3a20_sharpness_p2[] = {
};

static const unsigned char *bf3a20_regs_sharpness_bias[] = {
	(unsigned char *)bf3a20_sharpness_m2,
	(unsigned char *)bf3a20_sharpness_m1,
	(unsigned char *)bf3a20_sharpness_default,
	(unsigned char *)bf3a20_sharpness_p1,
	(unsigned char *)bf3a20_sharpness_p2,
};

struct bf3a20_platform_data {
	unsigned int default_width;
	unsigned int default_height;
	unsigned int pixelformat;
	int freq;	/* MCLK in KHz */

	/* This SoC supports Parallel & CSI-2 */
	int is_mipi;
};


#endif

