/* linux/drivers/video/samsung/otm1280a_dummymipilcd.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Modified by Samsung Electronics (UK) on May 2010
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/backlight.h>
#include <linux/lcd.h>

#include <plat/gpio-cfg.h>
#include <plat/regs-dsim.h>

#include <mach/dsim.h>
#include <mach/mipi_ddi.h>

#include "s5p-dsim.h"
#include "s3cfb.h"
#include <linux/regulator/driver.h>

#include <asm/mach-types.h>

extern int lcd_id;
extern int ESD_FOR_LCD;

int LG_write_init_cmd (void);
void ZDX_LG_write_init_cmd(void);
int otm1280a_cabc_control(int mode,int brightness );
unsigned char Read_LCDBoe_ID(void);

//#define MIPI_TX_CHECK 1
#define MIPI_DBG 1 

#define CONFIG_BACKLIGHT_STUTTGART
#define MAX_BRIGHTNESS		(255)
#define MIN_BRIGHTNESS		(20)

struct regulator *lcd_regulator = NULL; 
struct regulator *lcd_vdd18_regulator = NULL;

//#define LCD_CE_INTERFACE
#ifdef LCD_CE_INTERFACE
static int flag = 0;
static int ce_mode = 1;
#endif


#define LCD_CABC_MODE // don't remove it ，for LG LCD backlight control
#ifdef LCD_CABC_MODE
#define CABC_ON			1
#define CABC_OFF		0
static int cabc_mode = 1;
#endif

#define TP_LED_DELAY_TIME 3000

#if defined(CONFIG_TOUCHSCREEN_GOODIX)
extern struct delayed_work tp_led_delayed_work;
extern void led_put_out(void);
extern void led_light_up(void);
#endif

//#define ESD_FOR_LCD
//#if ESD_FOR_LCD
#define DELAY_TIME     200 
//600
#define TE_IRQ IRQ_EINT(28)
#define TE_TIME        500

struct delayed_work lcd_delayed_work;
struct delayed_work lcd_init_work;
struct timer_list te_timer;
static int esd_test = 0;
//#endif

static struct mipi_ddi_platform_data *ddi_pd;
static int current_brightness = 100;


//struct pwm_device       *pwm;
#ifdef CONFIG_BACKLIGHT_STUTTGART
static struct backlight_properties props;
#endif

//---------------------------------------------------------------------------
//porting from ZDX
static void btl507212_write_0(unsigned char dcs_cmd)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, /*GEN_SHORT_WR_1_PARA*/DCS_WR_NO_PARA, dcs_cmd, 0);
}

static void btl507212_write_1(unsigned char dcs_cmd, unsigned char param1)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, /*GEN_SHORT_WR_2_PARA*/DCS_WR_1_PARA, dcs_cmd, param1);
}


static void btl507212_write(unsigned char *buf,unsigned int data_size)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base,/*GEN_LONG_WR*/DCS_LONG_WR,(unsigned int)buf,data_size);
}

static void btl507212_display_off(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x28, 0x00);
}

static void s90379_sleep_in(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x10, 0);
}

static void s90379_sleep_out(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x11, 0);
}

static void btl507212_display_on(struct device *dev)
{
//	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x29, 0);
}

//---------------------------------------------------------------------------

void otm1280a_write_0(unsigned char dcs_cmd)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, dcs_cmd, 0);
}

static void otm1280a_write_1(unsigned char dcs_cmd, unsigned char param1)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, dcs_cmd, param1);
}
#if 0
static void otm1280a_write_3(unsigned char dcs_cmd, unsigned char param1, unsigned char param2, unsigned char param3)
{
	unsigned char buf[4];
	buf[0] = dcs_cmd;
	buf[1] = param1;
	buf[2] = param2;
	buf[3] = param3;

	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, 4);
}

static void otm1280a_write(void)
{
	unsigned char buf[15] = {0xf8, 0x01, 0x27, 0x27, 0x07, 0x07, 0x54,
		0x9f, 0x63, 0x86, 0x1a,
		0x33, 0x0d, 0x00, 0x00};
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
}
#endif

static void otm1280a_display_off(struct device *dev)
{
	int ret;
	printk("##%s#####\n", __func__);

	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x51, 0);
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x53, 0x00);

	if (lcd_id == LCD_BOE) 
	{
		ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x28, 0x00);
		printk("%s BOE\n", __func__);
		return;
	} 
	else if (lcd_id == LCD_LG) 
	{
		
	      unsigned char par1[] = {0xde,0x15,0x18,0x11,0x10,0x0f,0x18};
     		unsigned char par2[] = {0xc3,0x00,0x06,0x34,0x05,0x01,0x44,0x54};
		unsigned char par3[] = 	{0xce,0x0c,0x10};
		printk("%s lG\n", __func__);
		btl507212_write_1(0xff,0x01);
		btl507212_write(par1,7);
		btl507212_write(par2,8);
		btl507212_write(par3,3);
		btl507212_write_0(0x28);
		mdelay(20);
		otm1280a_write_0(0x10);
		/*
		otm1280a_write_1(0xc2, 0x00);
		do{
			unsigned char buf[] = {0xc4, 0x00,0x00, 0x00, 0x00,0x00};
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return;
			}
		}while(0);
		otm1280a_write_1(0xc1, 0x02);
		mdelay(20);
		otm1280a_write_1(0xc1, 0x03);
		//mdelay(20);
		mdelay(50);
		*/
	}
	else		//yassy or cmi who use nova chip
	{
		otm1280a_write_0(0x28);
		otm1280a_write_0(0x10);
		mdelay(150);
		printk("%s NOVA\n", __func__);
	}
}

void otm1280a_sleep_in(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x10, 0);
}

void otm1280a_sleep_out(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x11, 0);
}

static void otm1280a_display_on(struct device *dev)
{
	//printk("##%s#####\n", __func__);
	if (lcd_id == LCD_LG) 
	{
	/*
        printk("###%s###:LG_LCD display on!\n",__func__);
        int ret;
        #if 0
		otm1280a_write_1(0xc2, 0x02);
		//mdelay(15);
		otm1280a_write_1(0xc2, 0x06);
		//mdelay(15);
		otm1280a_write_1(0xc2, 0x4E);
		//mdelay(90);
		otm1280a_write_0(0x11);
		//mdelay(20);
		do{
			unsigned char buf[] = {0xF9,0x80,0x00};
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return;
			}
		}while(0);
        #endif
        	otm1280a_write_0(0x11);
		mdelay(120);
		otm1280a_write_0(0x29);
		mdelay(10);
		*/
        
	}//else		//yassy or cmi who use nova chip
	//{
		//otm1280a_write_0(0x11);
		//mdelay(200);
		//otm1280a_write_0(0x29);
		//mdelay(10);
		//mdelay(1);
	//}
}
#if 0
#define LCD_RESET EXYNOS4_GPC1(2)
#define LCD_FRM EXYNOS4_GPD0(0)//MOTOR_PWM
#define LED_CONTROL EXYNOS4_GPX0(6)//HOME_KEYLED
void lcd_reset(void)
{
	int err;

	err=gpio_request(LCD_RESET,"GPC1-2");
	if (err)
	{
		printk("failed to request reset lcd pin GPC1_2 !\n"); 
		return err;
	}
	if(lcd_id == LCD_BOE)
    {
        printk("lcd reset for BOE\n");
        s3c_gpio_setpull(LCD_RESET, S3C_GPIO_PULL_NONE);
	    gpio_direction_output(LCD_RESET, 1);
	    mdelay(1); //50  
	    gpio_direction_output(LCD_RESET, 0);           
	    mdelay(10);//100              
	    gpio_direction_output(LCD_RESET, 1);
	    mdelay(20);//200
	    gpio_free(LCD_RESET);
    }
    else if(lcd_id == LCD_LG)
    {
    	printk("###%s###:lcd reset for LG\n",__func__);
		s3c_gpio_setpull(LCD_RESET, S3C_GPIO_PULL_NONE);
		gpio_direction_output(LCD_RESET, 1);
		mdelay(1/*5*/);   
		gpio_direction_output(LCD_RESET, 0);           
		mdelay(10/*120*/);               
		gpio_direction_output(LCD_RESET, 1);
		mdelay(20/*5*/);
		gpio_free(LCD_RESET);		
    }
}
#endif
#ifdef CONFIG_BACKLIGHT_STUTTGART

void set_maxbrightness(void)
{
    //backlight commad ,set to max brightness
     ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x51, 255);
     ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x53, 0x24);
}
static int s5p_bl_update_status(struct backlight_device *bd)
{

 int bl = bd->props.brightness;
 int ret;
 if (lcd_id == LCD_BOE) {
#ifdef CONFIG_CYIT_CMCC_TEST
    #define MIN_DISPLAY 20
#else
    #define MIN_DISPLAY 40
#endif	
    #define SLOPE  (MAX_BRIGHTNESS - MIN_DISPLAY)/(MAX_BRIGHTNESS - MIN_BRIGHTNESS)
       
    /*printk("\n\n\n%s current_brightness = %d\n\n\n", __func__, bl);*/

	current_brightness = bl * MAX_BRIGHTNESS / 255;
	if(current_brightness == 0)  
    current_brightness=255;
	else if (current_brightness < 10 && current_brightness > 0) 
    current_brightness = 10;     
    else   
	current_brightness = ( bl*SLOPE + MAX_BRIGHTNESS - MAX_BRIGHTNESS*SLOPE);

	/*if (ddi_pd->resume_complete != 1) {
		return 0;
	}*/

	//mdelay(10);
	do{
		unsigned char buf[2] = {0x00,0x00};
		buf[0] = 0x51;
		buf[1] = current_brightness;

		ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
		if (ret == 0){
			printk("cmmd_write error\n");
			return ret;
		}
	}while(0);
    
   // printk("\n\n\n%s bl = %d\n", __func__, bl);
   // printk("\n\n\n%s current_brightness = %d\n\n\n", __func__, current_brightness);
 
 }else if (lcd_id == LCD_LG){
	//return 0;
    #if 1
  //  #define LG_MIN_DISPLAY 0//20
//    #define LG_SLOPE  (MAX_BRIGHTNESS - LG_MIN_DISPLAY)/(MAX_BRIGHTNESS - MIN_BRIGHTNESS)
    	   
    /*printk("\n\n\n%s current_brightness = %d\n\n\n", __func__, bl);*/

	current_brightness = bl; //* MAX_BRIGHTNESS / 255;
	//if(current_brightness == 0)  
   // current_brightness=255;
	//else if (current_brightness < 10 && current_brightness >0) 
  //  current_brightness = 10;     
 //   else   
	//current_brightness = ( bl*LG_SLOPE + MAX_BRIGHTNESS - MAX_BRIGHTNESS*LG_SLOPE); 
    otm1280a_write_1(0x51, current_brightness);//wirte brightness
    //otm1280a_cabc_control(cabc_mode,current_brightness );
    //printk("\n\n\nLG_LCD:%s bl = %d\n", __func__, bl);
    //printk("\n\n\nLG_LCD:%s current_brightness = %d\n\n\n", __func__, current_brightness);
    #else
    current_brightness =255;
    #endif

  }   

    
	/*
	   mdelay(1);
	   do{
	   unsigned char buf[2] = {0x53,0x24};

	   ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
	   if (ret == 0){
	   printk("cmmd_write error\n");
	   return ret;
	   }
	   }while(0);
	   */

	return 0;
}
#if 0
static int s5p_bl_get_brightness(struct backlilght_device *bd)
{
	return 0;
}
#endif

static const struct backlight_ops s5p_bl_ops = {
	.update_status = s5p_bl_update_status,
#if 0
	.get_brightness = s5p_bl_get_brightness,
#endif
};
#endif
static int lcd_init_cmd_write(unsigned int dsim_base, unsigned int data_id, unsigned char * buf, unsigned int size)
{
	int ret = DSIM_FALSE;
	u32 read_buf[100];

	memset(read_buf, 0, sizeof(read_buf));
	ret = ddi_pd->cmd_write(ddi_pd->dsim_base, data_id,  (unsigned int )buf, size);
	if (ret == 0){
		printk("cmmd_write error\n");
		return DSIM_FALSE;
	}
#ifdef  MIPI_TX_CHECK
	int read_size;
	ddi_pd->cmd_write(ddi_pd->dsim_base, SET_MAX_RTN_PKT_SIZE, 100, 0);
	s5p_dsim_clr_state(ddi_pd->dsim_base,RxDatDone);
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_RD_NO_PARA, (u8)buf[0], 0);
	s5p_dsim_wait_state(ddi_pd->dsim_base, RxDatDone);                  
	s5p_dsim_read_rx_fifo(ddi_pd->dsim_base, read_buf, &read_size);
	int i = 0;
#ifdef MIPI_DBG
	u8 *p = (u8 *)read_buf;
	for (i = 0; i < read_size; i++)
		printk("%#x ", p[i]);
	printk("\n");
#endif
	if (memcmp((u8 *)&buf[1], (u8*)read_buf, size -1) == 0)
		return DSIM_TRUE;
	else
		return DSIM_FALSE;
#endif	
	return DSIM_TRUE;

}
#ifdef LCD_CABC_MODE
#if 0
int otm1280a_cabc_control(int mode,int brightness )
{
	int ret;

	if (lcd_id == LCD_BOE) {
		printk(KERN_ERR "%s Invalid argument, the lcd's vender is BOE, not support cabc function\n", __func__);
		return -EINVAL;
	}
	//cabc control
	if (mode < 0 || mode > 3) {
		printk(KERN_ERR "%s Invalid argument, 0: off, 1: ui mode, 2:still mode 3: move mode\n", __func__);
		return -EINVAL;
	}
	if (mode) {
		printk("%s %s mode = %d\n", __func__, mode ? "on" : "off", mode);
		otm1280a_write_1(0x51, brightness);//write brightness value       
        otm1280a_write_1(0x53, 0x24);// BCTRL--Display Dimming--BL
        //otm1280a_write_1(0x53, 0x2c);
		otm1280a_write_1(0x55, mode);//0:manual 1:Conservative   2:Normal  3: Aggressive
		otm1280a_write_1(0x5e, 0x00);//set CABC min brightness
/*        
		do{
			unsigned char buf[] = {0xc8,0x82,0x86,0x01,0x11};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);*/
	}
	else {
		printk("%s cabc off = manul control \n", __func__);
        otm1280a_write_1(0x51, brightness);  
        otm1280a_write_1(0x53, 0x2c);		      
		otm1280a_write_1(0x55, 0x00);
        otm1280a_write_1(0x5e, 0x00);
	}
	return 0;
}

static int otm1280a_cabc_mode(const char *val, struct kernel_param *kp)
{
    printk("\n\n\n###%s is called!###\n\n\n",__func__);
    int value;
	int ret = param_set_int(val, kp);

	if(ret < 0)
	{   
		printk(KERN_ERR"%s Invalid argument\n", __func__);
		return -EINVAL;
	}   
	value = *((int*)kp->arg);
	return otm1280a_cabc_control(value,current_brightness);
}
#endif
#endif

typedef struct 
{
	unsigned char add;
	unsigned char data;
}para_1_data;
para_1_data yt_code[]={
	//{0xFF,0x00},
	//{0xBA,0x03},//0x02//4 lane
	//{0xC2,0x03},//video mode
	{0xFF,0x01},
	{0x00,0x3A},
	{0x01,0x33},
	{0x02,0x53},
	{0x09,0x85},
	{0x0E,0x25},
	{0x0F,0x0A},
	{0x0B,0x97},
	{0x0C,0x97},
	{0x11,0x8A},
	{0x36,0x7B},
	{0x71,0x2C},
	{0xFF,0x05},
	//ltps timing
	{0x01,0x00},
	{0x02,0x8D},
	{0x03,0x8D},
	{0x04,0x8D},
	{0x05,0x30},
	{0x06,0x33},
	{0x07,0x77},
	{0x08,0x00},
	{0x09,0x00},
	{0x0A,0x00},
	{0x0B,0x80},
	{0x0C,0xC8},
	{0x0D,0x00},
	{0x0E,0x1B},
	{0x0F,0x07},
	{0x10,0x57},
	{0x11,0x00},
	{0x12,0x00},
	{0x13,0x1E},
	{0x14,0x00},
	{0x15,0x1A},
	{0x16,0x05},
	{0x17,0x00},
	{0x18,0x1E},
	{0x19,0xFF},
	{0x1A,0x00},
	{0x1B,0xFC},
	{0x1C,0x80},
	{0x1D,0x00},
	{0x1E,0x10},
	{0x1F,0x77},
	{0x20,0x00},
	{0x21,0x06},	//invers display, default is 0x00
	{0x22,0x55},
	{0x23,0x0D},
	{0x31,0xA0},
	{0x32,0x00},
	{0x33,0xB8},
	{0x34,0xBB},
	{0x35,0x11},
	{0x36,0x01},
	{0x37,0x0B},
	{0x38,0x01},
	{0x39,0x0B},
	{0x44,0x08},
	{0x45,0x80},
	{0x46,0xCC},
	{0x47,0x04},
	{0x48,0x00},
	{0x49,0x00},
	{0x4A,0x01},
	{0x6C,0x03},
	{0x6D,0x03},
	{0x6E,0x2F},
	{0x43,0x00},
	{0x4B,0x23},
	{0x4C,0x01},
	{0x50,0x23},
	{0x51,0x01},
	{0x58,0x23},
	{0x59,0x01},
	{0x5D,0x23},
	{0x5E,0x01},
	{0x62,0x23},
	{0x63,0x01},
	{0x67,0x23},
	{0x68,0x01},
	{0x89,0x00},
	{0x8D,0x01},
	{0x8E,0x64},
	{0x8F,0x20},
	{0x97,0x8E},
	{0x82,0x8C},
	{0x83,0x02},
	{0xBB,0x0A},
	{0xBC,0x0A},
	{0x24,0x25},
	{0x25,0x55},
	{0x26,0x05},
	{0x27,0x23},
	{0x28,0x01},
	{0x29,0x31},
	{0x2A,0x5D},
	{0x2B,0x01},
	{0x2F,0x00},
	{0x30,0x10},
	{0xA7,0x12},
	{0x2D,0x03},
	{0xFF,0x01},
	{0x75,0x00},
	{0x76,0x42},
	{0x77,0x00},
	{0x78,0x56},
	{0x79,0x00},
	{0x7A,0x79},
	{0x7B,0x00},
	{0x7C,0x97},
	{0x7D,0x00},
	{0x7E,0xB1},
	{0x7F,0x00},
	{0x80,0xC8},
	{0x81,0x00},
	{0x82,0xDB},
	{0x83,0x00},
	{0x84,0xEC},
	{0x85,0x00},
	{0x86,0xFB},
	{0x87,0x01},
	{0x88,0x26},
	{0x89,0x01},
	{0x8A,0x49},
	{0x8B,0x01},
	{0x8C,0x86},
	{0x8D,0x01},
	{0x8E,0xB3},
	{0x8F,0x01},
	{0x90,0xFC},
	{0x91,0x02},
	{0x92,0x37},
	{0x93,0x02},
	{0x94,0x39},
	{0x95,0x02},
	{0x96,0x6F},
	{0x97,0x02},
	{0x98,0xAA},
	{0x99,0x02},
	{0x9A,0xC9},
	{0x9B,0x02},
	{0x9C,0xFC},
	{0x9D,0x03},
	{0x9E,0x20},
	{0x9F,0x03},
	{0xA0,0x52},
	{0xA2,0x03},
	{0xA3,0x62},
	{0xA4,0x03},
	{0xA5,0x75},
	{0xA6,0x03},
	{0xA7,0x8A},
	{0xA9,0x03},
	{0xAA,0xA1},
	{0xAB,0x03},
	{0xAC,0xB5},
	{0xAD,0x03},
	{0xAE,0xC6},
	{0xAF,0x03},
	{0xB0,0xCF},
	{0xB1,0x03},
	{0xB2,0xD1},
	{0xB3,0x00},
	{0xB4,0x42},
	{0xB5,0x00},
	{0xB6,0x56},
	{0xB7,0x00},
	{0xB8,0x79},
	{0xB9,0x00},
	{0xBA,0x97},
	{0xBB,0x00},
	{0xBC,0xB1},
	{0xBD,0x00},
	{0xBE,0xC8},
	{0xBF,0x00},
	{0xC0,0xDB},
	{0xC1,0x00},
	{0xC2,0xEC},
	{0xC3,0x00},
	{0xC4,0xFB},
	{0xC5,0x01},
	{0xC6,0x26},
	{0xC7,0x01},
	{0xC8,0x49},
	{0xC9,0x01},
	{0xCA,0x86},
	{0xCB,0x01},
	{0xCC,0xB3},
	{0xCD,0x01},
	{0xCE,0xFC},
	{0xCF,0x02},
	{0xD0,0x37},
	{0xD1,0x02},
	{0xD2,0x39},
	{0xD3,0x02},
	{0xD4,0x6F},
	{0xD5,0x02},
	{0xD6,0xAA},
	{0xD7,0x02},
	{0xD8,0xC9},
	{0xD9,0x02},
	{0xDA,0xFC},
	{0xDB,0x03},
	{0xDC,0x20},
	{0xDD,0x03},
	{0xDE,0x52},
	{0xDF,0x03},
	{0xE0,0x62},
	{0xE1,0x03},
	{0xE2,0x75},
	{0xE3,0x03},
	{0xE4,0x8A},
	{0xE5,0x03},
	{0xE6,0xA1},
	{0xE7,0x03},
	{0xE8,0xB5},
	{0xE9,0x03},
	{0xEA,0xC6},
	{0xEB,0x03},
	{0xEC,0xCF},
	{0xED,0x03},
	{0xEE,0xD1},
	{0xEF,0x00},
	{0xF0,0x42},
	{0xF1,0x00},
	{0xF2,0x56},
	{0xF3,0x00},
	{0xF4,0x79},
	{0xF5,0x00},
	{0xF6,0x97},
	{0xF7,0x00},
	{0xF8,0xB1},
	{0xF9,0x00},
	{0xFA,0xC8},
	{0xFF,0x02},
	{0x00,0x00},
	{0x01,0xDB},
	{0x02,0x00},
	{0x03,0xEC},
	{0x04,0x00},
	{0x05,0xFB},
	{0x06,0x01},
	{0x07,0x26},
	{0x08,0x01},
	{0x09,0x49},
	{0x0A,0x01},
	{0x0B,0x86},
	{0x0C,0x01},
	{0x0D,0xB3},
	{0x0E,0x01},
	{0x0F,0xFC},
	{0x10,0x02},
	{0x11,0x37},
	{0x12,0x02},
	{0x13,0x39},
	{0x14,0x02},
	{0x15,0x6F},
	{0x16,0x02},
	{0x17,0xAA},
	{0x18,0x02},
	{0x19,0xC9},
	{0x1A,0x02},
	{0x1B,0xFC},
	{0x1C,0x03},
	{0x1D,0x20},
	{0x1E,0x03},
	{0x1F,0x52},
	{0x20,0x03},
	{0x21,0x62},
	{0x22,0x03},
	{0x23,0x75},
	{0x24,0x03},
	{0x25,0x8A},
	{0x26,0x03},
	{0x27,0xA1},
	{0x28,0x03},
	{0x29,0xB5},
	{0x2A,0x03},
	{0x2B,0xC6},
	{0x2d,0x03},
	{0x2F,0xCF},
	{0x30,0x03},
	{0x31,0xD1},
	{0x32,0x00},
	{0x33,0x42},
	{0x34,0x00},
	{0x35,0x56},
	{0x36,0x00},
	{0x37,0x79},
	{0x38,0x00},
	{0x39,0x97},
	{0x3A,0x00},
	{0x3B,0xB1},
	{0x3D,0x00},
	{0x3F,0xC8},
	{0x40,0x00},
	{0x41,0xdb},
	{0x42,0x00},
	{0x43,0xEC},
	{0x44,0x00},
	{0x45,0xFB},
	{0x46,0x01},
	{0x47,0x26},
	{0x48,0x01},
	{0x49,0x49},
	{0x4A,0x01},
	{0x4B,0x86},
	{0x4C,0x01},
	{0x4D,0xB3},
	{0x4E,0x01},
	{0x4F,0xFC},
	{0x50,0x02},
	{0x51,0x37},
	{0x52,0x02},
	{0x53,0x39},
	{0x54,0x02},
	{0x55,0x6F},
	{0x56,0x02},
	{0x58,0xAA},
	{0x59,0x02},
	{0x5A,0xC9},
	{0x5B,0x02},
	{0x5C,0xFC},
	{0x5D,0x03},
	{0x5E,0x20},
	{0x5F,0x03},
	{0x60,0x52},
	{0x61,0x03},
	{0x62,0x62},
	{0x63,0x03},
	{0x64,0x75},
	{0x65,0x03},
	{0x66,0x8A},
	{0x67,0x03},
	{0x68,0xA1},
	{0x69,0x03},
	{0x6A,0xB5},
	{0x6B,0x03},
	{0x6C,0xC6},
	{0x6D,0x03},
	{0x6E,0xCF},
	{0x6F,0x03},
	{0x70,0xD1},
	{0x71,0x00},
	{0x72,0x42},
	{0x73,0x00},
	{0x74,0x56},
	{0x75,0x00},
	{0x76,0x79},
	{0x77,0x00},
	{0x78,0x97},
	{0x79,0x00},
	{0x7A,0xB1},
	{0x7B,0x00},
	{0x7C,0xC8},
	{0x7D,0x00},
	{0x7E,0xDB},
	{0x7F,0x00},
	{0x80,0xEC},
	{0x81,0x00},
	{0x82,0xFB},
	{0x83,0x01},
	{0x84,0x26},
	{0x85,0x01},
	{0x86,0x49},
	{0x87,0x01},
	{0x88,0x86},
	{0x89,0x01},
	{0x8A,0xB3},
	{0x8B,0x01},
	{0x8C,0xFC},
	{0x8D,0x02},
	{0x8E,0x37},
	{0x8F,0x02},
	{0x90,0x39},
	{0x91,0x02},
	{0x92,0x6F},
	{0x93,0x02},
	{0x94,0xAA},
	{0x95,0x02},
	{0x96,0xC9},
	{0x97,0x02},
	{0x98,0xFC},
	{0x99,0x03},
	{0x9A,0x20},
	{0x9B,0x03},
	{0x9C,0x52},
	{0x9D,0x03},
	{0x9E,0x62},
	{0x9f,0x03},
	{0xA0,0x75},
	{0xA2,0x03},
	{0xA3,0x8A},
	{0xA4,0x03},
	{0xA5,0xA1},
	{0xA6,0x03},
	{0xA7,0xB5},
	{0xA9,0x03},
	{0xAA,0xC6},
	{0xAB,0x03},
	{0xAC,0xCF},
	{0xAD,0x03},
	{0xAE,0xD1},
	{0xAF,0x00},
	{0xB0,0x42},
	{0xB1,0x00},
	{0xB2,0x56},
	{0xB3,0x00},
	{0xB4,0x79},
	{0xB5,0x00},
	{0xB6,0x97},
	{0xB7,0x00},
	{0xB8,0xB1},
	{0xB9,0x00},
	{0xBA,0xC8},
	{0xBB,0x00},
	{0xBC,0xDB},
	{0xBD,0x00},
	{0xBE,0xEC},
	{0xBF,0x00},
	{0xC0,0xFB},
	{0xC1,0x01},
	{0xC2,0x26},
	{0xC3,0x01},
	{0xC4,0x49},
	{0xC5,0x01},
	{0xC6,0x86},
	{0xC7,0x01},
	{0xC8,0xB3},
	{0xC9,0x01},
	{0xCA,0xFC},
	{0xCB,0x02},
	{0xCC,0x37},
	{0xCD,0x02},
	{0xCE,0x39},
	{0xCF,0x02},
	{0xD0,0x6F},
	{0xD1,0x02},
	{0xD2,0xAA},
	{0xD3,0x02},
	{0xD4,0xC9},
	{0xD5,0x02},
	{0xD6,0xFC},
	{0xD7,0x03},
	{0xD8,0x20},
	{0xD9,0x03},
	{0xDA,0x52},
	{0xDB,0x03},
	{0xDC,0x62},
	{0xDD,0x03},
	{0xDE,0x75},
	{0xDF,0x03},
	{0xE0,0x8A},
	{0xE1,0x03},
	{0xE2,0xA1},
	{0xE3,0x03},
	{0xE4,0xB5},
	{0xE5,0x03},
	{0xE6,0xC6},
	{0xE7,0x03},
	{0xE8,0xCF},
	{0xE9,0x03},
	{0xEA,0xD1},
	{0xFF,0x00},
	{0xFB,0x01},
	{0xFF,0x01},
	{0xFB,0x01},
	{0xFF,0x02},
	{0xFB,0x01},
	{0xFF,0x03},
	{0xFB,0x01},
	{0xFF,0x04},
	{0xFB,0x01},
	{0xFF,0x05},
	{0xFB,0x01},
	{0xFF,0x00},
	{0xFF,0xff},	//end
};

#define ID_PIN EXYNOS4_GPX3(3)
#if 0
#ifdef ESD_FOR_LCD
 extern void lcd_te_handle(void);
 extern int fbdev_register_finished;
 static int test_te_function(const char *val, struct kernel_param *kp)
 {
     printk("%s\n", __func__);
     schedule_delayed_work(&lcd_init_work, 0);
     return 0;
 }
 static void lcd_work_handle(struct work_struct *work)
 {
     //otm1280a_write_0(0x38);
     //schedule_delayed_work(&lcd_delayed_work, DELAY_TIME);
     led_put_out();
 }
 static void lcd_te_abnormal_handle(struct work_struct *work)
 {
     //printk("\n\n\n+++xufei: enter the timer interrupt 1ms \n\n\n");
     //printk("HZ is %d\n",HZ);//HZ is 200
     otm1280a_write_0(0x38);
     mod_timer(&te_timer, jiffies +  msecs_to_jiffies(1));
 
   printk("!!!!!!!!!!!!!!!! %s !!!!!!!!!!!!!!!!!\n", __func__);
     //otm1280a_early_suspend();
     lcd_te_handle();
     //otm1280a_late_resume();
     printk("!!!!!!!!!!!!!!!! %s finished!!!!!!!!!!!!!!!!!\n", __func__);
 
     
 }
 
 void te_timer_handle(unsigned long data)
 {
     schedule_delayed_work(&lcd_init_work, 0);
 }
 irqreturn_t lcd_te_irq_handle(int irq, void *dev_id)
 {
     //if (fbdev_register_finished)
         mod_timer(&te_timer, TE_TIME * HZ / 1000 + jiffies);
     return IRQ_HANDLED;
 }
#endif
 
#else
 
 //#if ESD_FOR_LCD
 extern void lcd_te_handle(void);
 extern int fbdev_register_finished;
 static int test_te_function(const char *val, struct kernel_param *kp)
 {
     printk("%s\n", __func__);
     schedule_delayed_work(&lcd_init_work, 0);
     return 0;
 }
 static void lcd_work_handle(struct work_struct *work)
 {
     //printk("\n\n\n ===========================%s is called\n\n\n",__func__);  
     otm1280a_write_0(0x38);
     schedule_delayed_work(&lcd_delayed_work, DELAY_TIME);
 }
 
 static void lcd_te_abnormal_handle(struct work_struct *work)
 {
     printk("!!!!!!!!!!!!!!!! %s !!!!!!!!!!!!!!!!!\n", __func__);
     //otm1280a_early_suspend();
     lcd_te_handle();
     //otm1280a_late_resume();
     printk("!!!!!!!!!!!!!!!! %s finished!!!!!!!!!!!!!!!!!\n", __func__);
 }
 
 void te_timer_handle(unsigned long data)
 {
     schedule_delayed_work(&lcd_init_work, 0);
 }
 irqreturn_t lcd_te_irq_handle(int irq, void *dev_id)
 {
     if (fbdev_register_finished)
         mod_timer(&te_timer, TE_TIME * HZ / 1000 + jiffies);
     return IRQ_HANDLED;
 }
 //#endif
#endif

 int lcd_pre_init(void)
{
    int ret;
    static int pre_initialized=1;
    static int ESD_initialized=1;// static variable ,initialize only once
    printk("ESD_initialized is %#d\n", ESD_initialized);
	if (lcd_id == LCD_BOE) {
		printk("####%s  LCD_BOE####\n", __func__);
		//lcd_reset();
		otm1280a_write_0(0x11); 
		mdelay(200);
		do{
			unsigned char buf[] = {0xB9,0xFF,0x83,0x94};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);   
//test
        #if  0 //xufei check read function     
        //ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x51, 0xaa);
        //unsigned char buf[] = {0x51,0xaa};
        //ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
        #define BUF_SIZE 10
        u32 read_buf[BUF_SIZE];
	    int read_size;
        //设置最多可以返回多少个byte数据
	    ddi_pd->cmd_write(ddi_pd->dsim_base, SET_MAX_RTN_PKT_SIZE, BUF_SIZE, 0);
	    s5p_dsim_clr_state(ddi_pd->dsim_base,RxDatDone);
        //设置读地址(命令),不一定是buf[0]的值，要根据不同LCD spec
	    ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_RD_NO_PARA, (u8)buf[0], 0);
	    s5p_dsim_wait_state(ddi_pd->dsim_base, RxDatDone); 
        //read_size应该和BUF_SIZE是相等的，BUF_SIZE > 写入数,多余的为0
	    s5p_dsim_read_rx_fifo(ddi_pd->dsim_base, read_buf, &read_size);	                                                                   
        #ifdef MIPI_DBG
        int i = 0;
	    u8 *p = (u8 *)read_buf;
	    for (i = 0; i < read_size; i++)
		printk("%#x ", p[i]);
	    printk("\n");
        #endif     
        #endif  
//end
      
		do{
			//unsigned char buf[] = {0xBA,0x13};
			unsigned char buf[] = {0xBA,0x13,0x83,0x00,0x16,0xa6,0x50,0x08,0xff,0x0f,0x24,0x03,0x21,0x24,0x25,0x20,0x02,0x10};
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);
		//besure 0xcc is wrote correctly
		//otm1280a_write_1(0xcc, 0x05);
		//mdelay(10);

#if 0 
		do{
			unsigned char buf[] = {
				0xC1,0x01,0x00,0x07,0x10,0x16,0x1B,0x25,0x2C,0x34,0x38,
				0x44,0x4B,0x52,0x5B,0x61,0x6A,0x71,0x79,0x82,0x89,0x91,
				0x9B,0xA4,0xAC,0xB5,0xBD,0xC6,0xCD,0xD6,0xDD,0xE4,0xEC,0xF4,
				0xFB,0x05,0xEA,0xC9,0xB0,0x02,0x5F,0xFD,0x73,0xC0,0x00,0x03,
				0x0E,0x14,0x19,0x20,0x28,0x2F,0x33,0x3D,0x43,0x4A,0x52,0x57,
				0x60,0x66,0x6E,0x75,0x7C,0x83,0x8B,0x93,0x9B,0xA3,0xAA,0xB3,
				0xBA,0xC2,0xC9,0xCF,0xD7,0xDE,0xE5,0x05,0xEA,0xC9,0xB1,0x02,
				0x5F,0xFD,0x73,0xC0,0x00,0x01,0x0F,0x16,0x1C,0x26,0x2E,0x36,
				0x3B,0x47,0x4E,0x56,0x5F,0x65,0x6F,0x76,0x7F,0x88,0x90,0x99,
				0xA2,0xAC,0xB4,0xBD,0xC5,0xCD,0xD5,0xDE,0xE5,0xEB,0xF4,0xFA,
				0xFF,0x05,0xEA,0xC9,0xB1,0x02,0x5F,0xFD,0x73,0xC0
			};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);
#endif
	if(pre_initialized)
	{
        printk("set the backlight to max!!\n");
        set_maxbrightness();
		pre_initialized = 0;
	}
    if (ESD_FOR_LCD&&ESD_initialized) {//#if ESD_FOR_LCD       
        printk("ESD_initialized is %#d\n", ESD_initialized);
	    if (lcd_id == LCD_BOE){      
            printk("%s, init ESD function\n", __func__);
		    INIT_DELAYED_WORK(&lcd_delayed_work, lcd_work_handle);
		    schedule_delayed_work(&lcd_delayed_work, DELAY_TIME);
	    } else {
		    INIT_DELAYED_WORK(&lcd_init_work, lcd_te_abnormal_handle);
		    init_timer(&te_timer);
		    te_timer.function = te_timer_handle;
		    ret = request_irq(TE_IRQ, lcd_te_irq_handle, IRQF_TRIGGER_RISING, "LCD_TE", NULL);
		    if (ret) {
			    printk(KERN_ERR "lcd te request irq(%d) failure, ret = %d\n", TE_IRQ, ret);
			    return ret;
	        }
        }
        ESD_initialized = 0;
   }//#endif   
 }
   return 0;
}

#ifdef LCD_CE_INTERFACE
static int ce_on_off(int value, int flag)
{
	int ret;
	if (flag)
		ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x51, 0);
	if (lcd_id == LCD_LG) {
		if (value) {
			printk("%s: CE on for LG.\n", __func__);
#if 1
			//otm1280a_write_1(0x70, 0x00);
#if 1
			do{
				unsigned char buf[] = {0x74,0x05,0x03, 0x85};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);
#endif

			do{
				unsigned char buf[] = {0x75,0x03,0x00,0x03};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);

			do{
				unsigned char buf[] = {0x76,0x07,0x00,0x03};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);

#endif
		} else {
			printk("%s: CE off.\n", __func__);
#if 1
			//otm1280a_write_1(0x70, 0x00);
#if 1
			do{
				unsigned char buf[] = {0x74,0x00,0x00,0x06};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);
#endif

			do{
				unsigned char buf[] = {0x75,0x00,0x00,0x07};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);

			do{
				unsigned char buf[] = {0x76,0x00,0x00,0x06};
#if 0
				ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
				if (ret == 0){
					printk("cmmd_write error\n");
					return ret;
				}
#else
				ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
				if (ret == DSIM_FALSE){
					printk("cmd_write error\n");
					return ret;
				}
#endif
			}while(0);

#endif

		}
	} 
#if 0
	else {
		if (value) {
			/*
			printk("%s boe ce on, value = %d\n", __func__, value);
			otm1280a_write_1(0xe6, 0x01);
			otm1280a_write_1(0xe4, value);
			*/
		} else {
			/*
			printk("%s boe ce off\n", __func__);
			otm1280a_write_1(0xe6, 0x00);
			otm1280a_write_1(0xe4, 0x00);
			*/
		}
	}
#endif
	if (flag) {
		//mdelay(100);//xufei  from k860
		ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x51, current_brightness);
	}

	return 0;
}
#endif

static int lcd_pannel_on(void)
{
	int ret = DSIM_TRUE;
	int i = 0;

	if (lcd_id == LCD_BOE) {
		printk("####%s LCD_BOE####\n", __func__);
		do{
			unsigned char buf[] = {0xB1,0x7C,0x00,0x24,0x09,0x01,0x11,0x11,0x36,0x3e,0x26,0x26,0x57,0x0a,0x01,0xe6};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);
        
		do{
			unsigned char buf[] = {0xB2,0x0f,0xc8,0x04,0x04,0x00,0x81};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);
		do{
			unsigned char buf[] = {0xBf,0x06,0x10};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);
		do{
			unsigned char buf[] = {0xB4,0x00,0x00,0x00,0x05,0x06,0x41,0x42,0x02,0x41,0x42,0x43,0x47,0x19,0x58,0x60,0x08,0x85,0x10};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);
		do{
			unsigned char buf[] = {0xD5,0x4C,0x01,0x07,0x01,0xCD,0x23,0xEF,0x45,0x67,0x89,0xAB,0x11,0x00,0xDC,0x10,0xFE,0x32,0xBA,0x98,0x76,0x54,0x00,0x11,0x40};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);
		do{
			unsigned char buf[] = {0xE0,0x24,0x33,0x36,
				0x3F,0x3f,0x3f,0x3c,
				0x56,0x05,0x0c,0x0e,
				0x11,0x13,0x12,0x14,
				0x12,0x1e,0x24,0x33,
				0x36,0x3f,0x3f,0x3f,
				0x3c,0x56,0x05,0x0c,
				0x0e,0x11,0x13,0x12,
				0x14,0x12,0x1e};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);
#if 1
		do{
			unsigned char buf[] = {
				0xC1,
				0x01,
				0x00,
				0x07,
				0x10,
				0x16,
				0x1B,
				0x25,
				0x2C,
				0x34,
				0x38,
				0x44,
				0x4B,
				0x52,
				0x5B,
				0x61,
				0x6A,
				0x71,
				0x79,
				0x82,
				0x89,
				0x91,
				0x9B,
				0xA4,
				0xAC,
				0xB5,
				0xBD,
				0xC6,
				0xCD,
				0xD6,
				0xDD,
				0xE4,
				0xEC,
				0xF4,
				0xFB,
				0x05,
				0xEA,
				0xC9,
				0xB0,
				0x02,
				0x5F,
				0xFD,
				0x73,
				0xC0,
				0x00,
				0x03,
				0x0E,
				0x14,
				0x19,
				0x20,
				0x28,
				0x2F,
				0x33,
				0x3D,
				0x43,
				0x4A,
				0x52,
				0x57,
				0x60,
				0x66,
				0x6E,
				0x75,
				0x7C,
				0x83,
				0x8B,
				0x93,
				0x9B,
				0xA3,
				0xAA,
				0xB3,
				0xBA,
				0xC2,
				0xC9,
				0xCF,
				0xD7,
				0xDE,
				0xE5,
				0x05,
				0xEA,
				0xC9,
				0xB1,
				0x02,
				0x5F,
				0xFD,
				0x73,
				0xC0,
				0x00,
				0x01,
				0x0F,
				0x16,
				0x1C,
				0x26,
				0x2E,
				0x36,
				0x3B,
				0x47,
				0x4E,
				0x56,
				0x5F,
				0x65,
				0x6F,
				0x76,
				0x7F,
				0x88,
				0x90,
				0x99,
				0xA2,
				0xAC,
				0xB4,
				0xBD,
				0xC5,
				0xCD,
				0xD5,
				0xDE,
				0xE5,
				0xEB,
				0xF4,
				0xFA,
				0xFF,
				0x05,
				0xEA,
				0xC9,
				0xB1,
				0x02,
				0x5F,
				0xFD,
				0x73,
				0xC0
			};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);
#endif  
		otm1280a_write_1(0xe3, 0x01);
		do{
			unsigned char buf[] = {0xE5,0x00,0x00,0x04,
				0x04,0x02,0x00,0x80,
				0x20,0x00,0x20,0x00,
				0x00,0x08,0x06,0x04,
				0x00,0x80,0x0E};
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);		
		do{
			unsigned char buf[] = {0xC7,0x00,0x20};
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);

		do{
			unsigned char buf[] = {0xb6,0x2a};
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);
#ifdef LCD_CE_INTERFACE
		//ce_on_off(ce_mode, 0);//xufei from k860 20130523
#endif
		otm1280a_write_1(0x36, 0x01);	//only for command mode
		otm1280a_write_1(0xcc, 0x01);
		/*set sleep_out*/
		/* Set Display ON */
		otm1280a_write_0(0x29);
		//otm1280a_write_1(0xcc, 0x01);
		//mdelay(20);

	} 


//======================================================================

    else if (lcd_id == LCD_LG){
	    //lcd_reset();//xufei

       
    //    mdelay(10);
        /*otm1280a_write_0(0x11);
		mdelay(10);*/      
        ZDX_LG_write_init_cmd();

	 //otm1280a_cabc_control(cabc_mode,MAX_BRIGHTNESS);
        
#if 0
#if 1
		do{
			unsigned char buf[] = {0xE0,0x43,0x00,0x80,0x00,0x00};
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
		}while(0);	
		//mdelay(1);
		//Display mode
		do{
			//unsigned char buf[] = {0xB5,0x2A,0x20,0x40,0x00,0x20};
			unsigned char buf[] = {0xB5,0x34,0x20,0x40,0x00,0x20};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		//GIP SDAC
		do{
			//unsigned char buf[] = {0xB1,0x7C,0x00,0x34,0x09,0x01,0x11,0x11,0x36,0x3E,0x26,0x26,0x57,0x12,0x01,0xE6};
			//unsigned char buf[] = {0xB6,0x04,0x34,0x0F,0x16,0x13};
			unsigned char buf[] = {0xB6,0x04,0x74,0x0F,0x16,0x13};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		//internal ocsillator setting
		do{
			unsigned char buf[] = {0xc0,0x01,0x08};
#if 1
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		do{
			unsigned char buf[] = {0xc1,0x00};
#if 1
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		//		mdelay(1);

		//Power setting 
		do{
			//unsigned char buf[] = {0xC3,0x01,0x09,0x10,0x02,0x00,0x66,0x20,0x03,0x02};
			unsigned char buf[] = {0xC3,0x00,0x09,0x10,0x02,0x00,0x66,0x20,0x13,0x00};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		//		mdelay(1);

		do{
			//unsigned char buf[] = {0xC4,0x22,0x24,0x12,0x18,0x59};
			unsigned char buf[] = {0xC4,0x23,0x24,0x17,0x17,0x59};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);

		//Gamma setting
		do{
			//		unsigned char buf[] = {0xD0,0x10,0x60,0x67,0x35,0x00,0x06,0x60,0x21,0x02};
			unsigned char buf[] = {0xD0,0x21,0x13,0x67,0x37,0x0c,0x06,0x62,0x23,0x03};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif

		}while(0);
		do{
			//unsigned char buf[] = {0xD1,0x21,0x61,0x66,0x35,0x00,0x05,0x60,0x13,0x01};
			unsigned char buf[] = {0xD1,0x32,0x13,0x66,0x37,0x02,0x06,0x62,0x23,0x03};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);

		do{
			//unsigned char buf[] = {0xD2,0x10,0x60,0x67,0x40,0x1F,0x06,0x60,0x21,0x02};
			unsigned char buf[] = {0xD2,0x41,0x14,0x56,0x37,0x0c,0x06,0x62,0x23,0x03};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		do{
			//	unsigned char buf[] = {0xD3,0x21,0x61,0x66,0x40,0x1F,0x05,0x60,0x13,0x01};
			unsigned char buf[] = {0xD3,0x52,0x14,0x55,0x37,0x02,0x06,0x62,0x23,0x03};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		do{
			//	unsigned char buf[] = {0xD4,0x10,0x60,0x67,0x35,0x00,0x06,0x60,0x21,0x02};
			unsigned char buf[] = {0xD4,0x41,0x14,0x56,0x37,0x0c,0x06,0x62,0x23,0x03};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		do{
			//unsigned char buf[] = {0xD5,0x21,0x61,0x66,0x35,0x00,0x05,0x60,0x13,0x01};
			unsigned char buf[] = {0xD5,0x52,0x14,0x55,0x37,0x02,0x06,0x62,0x23,0x03};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		//Gamma setting end
		do{
			unsigned char buf[] = {0x36,0x0B };
#if 1
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		//		otm1280a_write_0(0x11);
		//		mdelay(5);
		do{
			//unsigned char buf[] = {0xF9,0x80,0x00};
			unsigned char buf[] = {0xF9,0x00};
#if 1
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
#endif

#ifdef LCD_CE_INTERFACE
		otm1280a_write_1(0x70, 0x00);

		do{
			unsigned char buf[] = {0x71,0x00,0x00, 0x01, 0x01};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);

		do{
			unsigned char buf[] = {0x72,0x01,0x0e};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);

		do{
			unsigned char buf[] = {0x73,0x34,0x52, 0x00};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
#if 0
		do{
			unsigned char buf[] = {0x74,0x05,0x00, 0x06};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);

		do{
			unsigned char buf[] = {0x75,0x03,0x00,0x07};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);

		do{
			unsigned char buf[] = {0x76,0x07, 0x00 ,0x06};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
#endif

		do{
			unsigned char buf[] = {0x77,0x3f,0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);

		do{
			unsigned char buf[] = {0x78,0x40,0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);

		do{
			unsigned char buf[] = {0x79,0x40,0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);

		do{
			unsigned char buf[] = {0x7A,0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);

		do{
			unsigned char buf[] = {0x7B,0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);

		do{
			unsigned char buf[] = {0x7C,0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
#else
			ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf, sizeof(buf));
			if (ret == DSIM_FALSE){
				printk("cmd_write error\n");
				return ret;
			}
#endif
		}while(0);
		ce_on_off(ce_mode, 0);
#endif
#endif
}

//=============================================================================

    else if (lcd_id == LCD_NOVA_CMI) {
		//####################################################
		printk("####%s  LCD_NOVA_CMI####\n", __func__);
		//i = 2;
		while(1)
		{
			if((yt_code[i].add == 0xff &&  yt_code[i].data == 0xff))
				break;
			//lcd_init_cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA,  &(yt_code[i]), 2);
			otm1280a_write_1(yt_code[i].add, yt_code[i].data);
			udelay(10);
			//printk("write %x  %x\n", yt_code[i].add, yt_code[i].data);
			i++;
		}
		otm1280a_write_0(0x11);
		//  mdelay(200);
		mdelay(120);
		otm1280a_write_0(0x29);
		// mdelay(50);
		mdelay(10);
		//otm1280a_write_1(0x51, 0xff);
		//mdelay(4);
		//otm1280a_write_1(0x53, 0x24);
		//mdelay(4);
		otm1280a_write_1(0x55, 0x00);
		//mdelay(4);
		otm1280a_write_1(0xFF, 0x00);
		//mdelay(4);
		otm1280a_write_1(0x35, 0x00);
		//mdelay(4);	
	}else if (lcd_id == LCD_NOVA_YASSY) {
		//####################################################
		//i = 2;
		printk("####%s  LCD_NOVA_YASSY####\n", __func__);
		while(1)
		{
			if((yt_code[i].add == 0xff &&  yt_code[i].data == 0xff))
				break;
			otm1280a_write_1(yt_code[i].add, yt_code[i].data);
			udelay(10);
			//printk("write %x  %x\n", yt_code[i].add, yt_code[i].data);
			i++;
		}
		otm1280a_write_0(0x11);
		//  mdelay(200);
		mdelay(120);
		otm1280a_write_0(0x29);
		// mdelay(50);
		mdelay(10);
		//  otm1280a_write_1(0x51, 0xff);
		// mdelay(4);
		//  otm1280a_write_1(0x53, 0x24);
		//  mdelay(4);
		otm1280a_write_1(0x55, 0x00);
		//mdelay(4);
		otm1280a_write_1(0xFF, 0x00);
		// mdelay(4);
		otm1280a_write_1(0x35, 0x00);
		// mdelay(4);
	}

	return DSIM_TRUE;
}
static int  lcd_panel_init(void)
{
	return  lcd_pannel_on();

}

static int stuttgart_panel_init(void)
{
	return lcd_panel_init();
}

static int otm1280a_set_link(void *pd, unsigned int dsim_base,
		unsigned char (*cmd_write) (unsigned int dsim_base, unsigned int data0,
			unsigned int data1, unsigned int data2),
		unsigned char (*cmd_read) (unsigned int dsim_base, unsigned int data0,
			unsigned int data1, unsigned int data2))
{
	struct mipi_ddi_platform_data *temp_pd = NULL;

	temp_pd = (struct mipi_ddi_platform_data *) pd;
	if (temp_pd == NULL) {
		printk(KERN_ERR "mipi_ddi_platform_data is null.\n");
		return -1;
	}

	ddi_pd = temp_pd;

	ddi_pd->dsim_base = dsim_base;

	if (cmd_write)
		ddi_pd->cmd_write = cmd_write;
	else
		printk(KERN_WARNING "cmd_write function is null.\n");

	if (cmd_read)
		ddi_pd->cmd_read = cmd_read;
	else
		printk(KERN_WARNING "cmd_read function is null.\n");

	return 0;
}
#ifdef CONFIG_HAS_EARLYSUSPEND
void otm1280a_early_suspend(void)
{
   #if defined(CONFIG_TOUCHSCREEN_GOODIX)
   led_put_out();
   #endif
   if(ESD_FOR_LCD){//#if ESD_FOR_LCD
	if (lcd_id == LCD_BOE)
		;
		//cancel_delayed_work_sync(&lcd_delayed_work);
	else {
		disable_irq(TE_IRQ);
		del_timer(&te_timer);
		//cancel_delayed_work_sync(&lcd_init_work);
	 }
   }  //#endif
    printk("%s\n", __func__);
	otm1280a_display_off(NULL);
#ifdef LCD_CABC_MODE
	//if (lcd_id == LCD_LG) 
	//	otm1280a_cabc_control(CABC_OFF,current_brightness);
#endif
	if (lcd_id == LCD_BOE){
		otm1280a_write_0(0x10);
		//mdelay(200);
		//mdelay(20);
		otm1280a_write_0(0x28);
		//mdelay(20);
	}

	s5p_dsim_frame_done_interrupt_enable(0);
}

void otm1280a_late_resume(void)
{
    #if defined(CONFIG_TOUCHSCREEN_GOODIX)
    led_light_up();
    /*schedule_delayed_work(&tp_led_delayed_work, TP_LED_DELAY_TIME);*/
    #endif
    if(lcd_id == LCD_BOE){
    printk("%s\n", __func__);
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x51, current_brightness);
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x53, 0x24);
	printk("%s current_bl = %d\n", __func__, current_brightness);
    }//else if ( lcd_id == LCD_LG){
	//ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x51, current_brightness);
	//}
	
#ifdef LCD_CABC_MODE
	//if (lcd_id == LCD_LG && cabc_mode) 
	//	otm1280a_cabc_control(cabc_mode,current_brightness);
#endif
	s5p_dsim_frame_done_interrupt_enable(1);
    if (ESD_FOR_LCD) {//#if ESD_FOR_LCD
        printk("%s, enter late_resume ESD function\n", __func__);
	    if (lcd_id == LCD_BOE) {        
            schedule_delayed_work(&lcd_delayed_work, DELAY_TIME);
		    //printk("%s, resume ESD function\n", __func__);
	    } else 
		    enable_irq(TE_IRQ);
   }//#endif 	 
}
#else
#ifdef CONFIG_PM
static int otm1280a_suspend(void)
{
	//printk("%s\n", __func__);
	/*
	   otm1280a_write_0(0x28);
	   mdelay(20);
	   otm1280a_write_0(0x10);
	   mdelay(200);
	   */
	printk("%s\n", __func__);
	return 0;
}

static int otm1280a_resume(struct device *dev)
{
	printk("%s\n", __func__);
	return 0;
}
#else
#define otm1280a_suspend	NULL
#define otm1280a_resume	NULL
#endif
#endif

//---------------------------------------------------------------
static void stuttgart_s3cfb_set_lcd_info(struct s3cfb_global *ctrl);
static int otm1280a_probe(struct device *dev)
{ 
    struct s3cfb_global *fbdev;
	int ret, err;
	//printk("%s:lcd_id is %s\n", __func__, LCD_BOE ? "LCD_BOE":"LCD_LG");
	s3cfb_set_lcd_info=stuttgart_s3cfb_set_lcd_info;
	lcd_regulator = regulator_get(NULL, "vdd33_lcd");
	if (IS_ERR(lcd_regulator)) {
		printk("%s: failed to get %s\n", __func__, "vdd33_lcd");
		err = -ENODEV;
		goto err_regulator;
	}
	regulator_enable(lcd_regulator);        //yulu

	lcd_vdd18_regulator = regulator_get(NULL, "lcd_iovdd18");
	if (IS_ERR(lcd_vdd18_regulator)) {
		printk("%s: failed to get %s\n", __func__, "lcd_iovdd18");
		err = -ENODEV;
		goto err_regulator;
	}
	regulator_enable(lcd_vdd18_regulator);


	/*err = gpio_request(LED_CONTROL, "GPX06");
	  if (err)
	  printk(KERN_ERR "#### failed to request GPX0 6 ####\n");*/

	memset(&props, 0, sizeof(struct backlight_properties));
	props.max_brightness = 255;
	props.type = BACKLIGHT_RAW;
	backlight_device_register("pwm-backlight.0", dev, &props, &s5p_bl_ops, &props);
	fbdev = kzalloc(sizeof(struct s3cfb_global), GFP_KERNEL);
	if (!fbdev) {
		printk(KERN_ERR "failed to allocate for "
				"global fb structure\n");
		ret = -ENOMEM;
		return ret;
	}

#if 0   
#if 1
	if (lcd_id == LCD_BOE) 
	{
		INIT_DELAYED_WORK(&lcd_delayed_work, lcd_work_handle);
	}
#endif
#ifdef ESD_FOR_LCD
	{
		INIT_DELAYED_WORK(&lcd_init_work, lcd_te_abnormal_handle);
		
		init_timer(&te_timer);        
        te_timer.expires = jiffies +  msecs_to_jiffies(1);       
		te_timer.function = te_timer_handle;
        add_timer(&te_timer);
        
		/*ret = request_irq(TE_IRQ, lcd_te_irq_handle, IRQF_TRIGGER_RISING, "LCD_TE", NULL);
		if (ret) 
		{
			printk(KERN_ERR "lcd te request irq(%d) failure, ret = %d\n", TE_IRQ, ret);
			return ret;
		}*/
	}
#endif

#else
/*
 if(ESD_FOR_LCD){//#if ESD_FOR_LCD
	if (lcd_id == LCD_BOE) {
		INIT_DELAYED_WORK(&lcd_delayed_work, lcd_work_handle);
		schedule_delayed_work(&lcd_delayed_work, DELAY_TIME);
	} else {
		INIT_DELAYED_WORK(&lcd_init_work, lcd_te_abnormal_handle);
		init_timer(&te_timer);
		te_timer.function = te_timer_handle;
		ret = request_irq(TE_IRQ, lcd_te_irq_handle, IRQF_TRIGGER_RISING, "LCD_TE", NULL);
		if (ret) {
			printk(KERN_ERR "lcd te request irq(%d) failure, ret = %d\n", TE_IRQ, ret);
			return ret;
		}
	}
}//#endif
*/
#endif


#if 1
#ifdef CONFIG_HAS_EARLYSUSPEND
	fbdev->early_suspend.suspend = otm1280a_early_suspend;
	fbdev->early_suspend.resume = otm1280a_late_resume;
	fbdev->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
	register_early_suspend(&fbdev->early_suspend);
#endif
#endif


	return 0;
err_regulator:
	regulator_put(lcd_regulator);
	regulator_put(lcd_vdd18_regulator);
	return err;
}

static s32 otm1280a_remove(struct device *dev)
{
	//gpio_free(LCD_RESET);
	return 0;
}

static void  otm1280a_shutdown(struct device *dev)
{
	otm1280a_display_off(dev);
}
static struct mipi_lcd_driver otm1280a_mipi_driver = {
	.name = "btl507212_mipi_lcd",//"stuttgart_mipi_lcd",
	.pre_init = lcd_pre_init,
	.read_id=Read_LCDBoe_ID,
	.init = stuttgart_panel_init,
	.display_on = otm1280a_display_on,
	.set_link = otm1280a_set_link,
	.probe = otm1280a_probe,
	.remove = otm1280a_remove,
	.shutdown = otm1280a_shutdown,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend = otm1280a_suspend,
	.resume = otm1280a_resume,
#endif
	.display_off = otm1280a_display_off,
	.lcd_panel_info=NULL,
};

static int otm1280a_init(void)
{
	
	s5p_dsim_register_lcd_driver(&otm1280a_mipi_driver);
	return 0;
}

static void otm1280a_exit(void)
{
	return;
}


struct s3cfb_lcd btl507212_mipi_lcd = {
	.width	= 720,
	.height	= 1280,
	.bpp	= 24,
#ifdef CONFIG_CYIT_CMCC_TEST
	.freq	= 40,
#else
	.freq	= 60,
#endif

	.timing = {
		.h_fp = 0x20,
		.h_bp = 0x3b,
		.h_sw = 0x09,
		.v_fp = 0x0b,
		.v_fpe = 1,
		.v_bp = 0x0b,
		.v_bpe = 1,
		.v_sw = 0x02,
		.cmd_allow_len = 0xf,
	},

	.polarity = {
		.rise_vclk = 0,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};
static struct s3cfb_lcd stuttgart_mipi_lcd_boe = {
	.width	= 720,
	.height	= 1280,
	.bpp	= 24,
#ifdef CONFIG_CYIT_CMCC_TEST
	.freq	= 40,
#else
	.freq	= 60,
#endif

	.timing = {
		.h_fp = 0x20,
		.h_bp = 0x3b,
		.h_sw = 0x09,
		.v_fp = 0x0b,
		.v_fpe = 1,
		.v_bp = 0x0b,
		.v_bpe = 1,
		.v_sw = 0x02,
		.cmd_allow_len = 0xf,
	},

	.polarity = {
		.rise_vclk = 0,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};

 struct s3cfb_lcd stuttgart_mipi_lcd_lg = { 
	.width  = 720,
	.height = 1280,
	.bpp    = 24, 
#ifdef CONFIG_CYIT_CMCC_TEST
	.freq	= 40,
#else
	.freq	= 60,
#endif

	.timing = { 
		.h_fp =10, // 8,//10, 
		.h_bp =10, // 12,//10,
		.h_sw =8,// 8,   //  3
		.v_fp =10,// 50,//10, 
		.v_fpe =1,// 1,
		.v_bp =10,// 28,//10, 
		.v_bpe =1,// 1,
		.v_sw = 2,
		.cmd_allow_len = 0xf,
	},  

	.polarity = { 
		.rise_vclk = 0,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},  
};

static struct s3cfb_lcd stuttgart_mipi_lcd_yassy = { 
	.width  = 720,
	.height = 1280,
	.bpp    = 24, 
#ifdef CONFIG_CYIT_CMCC_TEST
	.freq	= 40,
#else
	.freq	= 60,
#endif

	.timing = { 
		.h_fp = 14, 
		.h_bp = 112,
		.h_sw = 4,
		.v_fp = 6, 
		.v_fpe = 2,
		.v_bp = 1, 
		.v_bpe = 1,
		.v_sw = 2,
		.cmd_allow_len = 0xf,
	},  

	.polarity = { 
		.rise_vclk = 0,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},  
};

/* name should be fixed as 's3cfb_set_lcd_info' */
static void stuttgart_s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
	btl507212_mipi_lcd.init_ldi=NULL;
	stuttgart_mipi_lcd_boe.init_ldi = NULL;
	stuttgart_mipi_lcd_lg.init_ldi = NULL;
	stuttgart_mipi_lcd_yassy.init_ldi = NULL;
	if (lcd_id == LCD_BOE)
		ctrl->lcd = &btl507212_mipi_lcd;//&stuttgart_mipi_lcd_boe;
	else if(lcd_id == LCD_LG){
        printk("###%s###:LCD is LG --->for FIMD\n",__func__);
		ctrl->lcd = &stuttgart_mipi_lcd_lg;
    }
	else if(lcd_id == LCD_NOVA_CMI)
		ctrl->lcd = &stuttgart_mipi_lcd_yassy;
	else//yassy
		ctrl->lcd = &stuttgart_mipi_lcd_yassy;
}

module_init(otm1280a_init);
module_exit(otm1280a_exit);
#ifdef LCD_CE_INTERFACE
static int ce_control_func(const char *val, struct kernel_param *kp)
{
    printk("###%s### is called!\n",__func__);
    int value;
	int ret = param_set_int(val, kp);

	if(ret < 0)
	{   
		printk(KERN_ERR"%s Invalid argument\n", __func__);
		return -EINVAL;
	}   
	value = *((int*)kp->arg);

	if (flag == 0) {
		flag = 1;
		ce_on_off(value, 0);
	} else {
		ce_on_off(value, 1);
	}
	return 0;
}
#endif

/*#ifdef LCD_CABC_MODE
module_param_call(set_cabc_mode, otm1280a_cabc_mode, param_get_int, &cabc_mode, S_IRUSR | S_IWUSR);
#endif*/
#ifdef LCD_CE_INTERFACE
module_param_call(ce_control, ce_control_func, param_get_int, &ce_mode, S_IRUSR | S_IWUSR);
#endif
//#if ESD_FOR_LCD
module_param_call(test_te, test_te_function, param_get_int, &esd_test, S_IRUSR | S_IWUSR);
//#endif
MODULE_LICENSE("GPL");

//=======================================================================
#if 0
#define MORE 1
unsigned char buf0[] = {
    0x70,0x00,0xb1,
    0x72,0x02,0x02,
    
    0x70,0x00,0xb2,
    0x72,0x10,0x1e,
    
    0x70,0x00,0xb3,
    0x72,0x08,0x32,
    
    0x70,0x00,0xb4,
    0x72,0x02,0xd0,
    
    0x70,0x00,0xb5,
    0x72,0x05,0x00,
    
    0x70,0x00,0xb6,
    0x72,0x00,0x2b,
    
    0x70,0x00,0xde,
    0x72,0x00,0x02,
    
    0x70,0x00,0xd6,
    0x72,0x00,0x05,
    
    0x70,0x00,0xb9,
    0x72,0x00,0x00,
    
    0x70,0x00,0xba,
    0x72,0x80,0x28,
    
    0x70,0x00,0xbb,
    0x72,0x00,0x0b,
    
    0x70,0x00,0xb9,
    0x72,0x00,0x01,
    
    0x70,0x00,0xb8,
    0x72,0x00,0x00,
    
    0x70,0x00,0xb7,
    0x72,0x03,0x00,
    
    0x70,0x00,0xcb,
    0x72,0x04,0x7c
};
unsigned char buf1_START[] = {
    //2075 start
    0x70,0x00,0xbc,
    0x72,0x00,0x02,
    
    0x70,0x00,0xbf,
    0x72,0xa3,0xe1,
    
    0x70,0x00,0xbf,
    0x72,0x00,0xb3,
    
    0x70,0x00,0xbc,
    0x72,0x00,0x05,
    
    0x70,0x00,0xbf,
    0x72,0x16,0xb6,
    0x72,0x00,0x0f,
    0x72,0x00,0x00,
    
    0x70,0x00,0xbc,
    0x72,0x00,0x07,
    
    0x70,0x00,0xbf,
    0x72,0x04,0xb9,
    0x72,0x22,0x08,
    0x72,0xff,0xff,
    0x72,0x00,0x0f,
    
    0x70,0x00,0xbc,
    0x72,0x00,0x09,
    
    0x70,0x00,0xbf,
    0x72,0x0e,0xba,
    0x72,0x10,0x0e,
    0x72,0x0a,0x10,
    0x72,0x0c,0x0a,
    0x72,0x00,0x0c,
    
    0x70,0x00,0xbf,
    0x72,0x00,0xb8,
    0x72,0x08,0x06,
    0x72,0x07,0x00,
    0x72,0x23,0x09,
    0x72,0x00,0x04,
    
    0x70,0x00,0xbf,
    0x72,0xa1,0xbb,
    0x72,0xa1,0xa1,
    0x72,0xa1,0xa1,
    0x72,0xa1,0xa1,
    0x72,0x00,0xa1,
    
    0x70,0x00,0xbf,
    0x72,0x00,0xbc,
    0x72,0x00,0x00,
    0x72,0x00,0x00,
    0x72,0x00,0x00,
    0x72,0x00,0x00,
    
    0x70,0x00,0xbf,
    0x72,0x0f,0xbd,
    0x72,0x11,0x0f,
    0x72,0x0b,0x11,
    0x72,0x0d,0x0b,
    0x72,0x00,0x0d,
    
    0x70,0x00,0xbf,
    0x72,0xa1,0xbe,
    0x72,0xa1,0xa1,
    0x72,0xa1,0xa1,
    0x72,0xa1,0xa1,
    0x72,0x00,0xa1,
    
    0x70,0x00,0xbf,
    0x72,0x00,0xbf,
    0x72,0x00,0x00,
    0x72,0x00,0x00,
    0x72,0x00,0x00,
    0x72,0x00,0x00
};

unsigned char buf2[] = {
    0x70,0x00,0xbc,
    0x72,0x00,0x04,
    
    0x70,0x00,0xbf,
    0x72,0x16,0xb1,
    0x72,0x12,0x1e,
    
    0x70,0x00,0xbc,
    0x72,0x00,0x06,
    
    0x70,0x00,0xbf,
    0x72,0x01,0xe0,
    0x72,0x02,0x03,
    0x72,0x01,0x00
};

unsigned char buf3_GAMMA[] = {
        //gamma
0x70,0x00,0xbc,
0x72,0x00,0x07,

0x70,0x00,0xbf,
0x72,0x00,0xD0,
0x72,0x10,0x00,
0x72,0x22,0x1E,
0x72,0x00,0x2E,

0x70,0x00,0xbf,
0x72,0x00,0xD2,
0x72,0x10,0x00,
0x72,0x22,0x1E,
0x72,0x00,0x2E,

0x70,0x00,0xbf,
0x72,0x00,0xD4,
0x72,0x10,0x00,
0x72,0x22,0x1E,
0x72,0x00,0x2E,
	
0x70,0x00,0xbf,
0x72,0x00,0xD6,
0x72,0x10,0x00,
0x72,0x22,0x1E,
0x72,0x00,0x2E,
	
0x70,0x00,0xbf,
0x72,0x00,0xD8,
0x72,0x10,0x00,
0x72,0x22,0x1E,
0x72,0x00,0x2E,

0x70,0x00,0xbf,
0x72,0x00,0xDa,
0x72,0x10,0x00,
0x72,0x22,0x1E,
0x72,0x00,0x2E,

0x70,0x00,0xbc,
0x72,0x00,0x06,

0x70,0x00,0xbf,
0x72,0x26,0xD1,
0x72,0x23,0x2B,
0x72,0x0A,0x1B,
	
0x70,0x00,0xbf,
0x72,0x26,0xD3,
0x72,0x23,0x2B,
0x72,0x0A,0x1B,

0x70,0x00,0xbf,
0x72,0x26,0xD5,
0x72,0x23,0x2B,
0x72,0x0A,0x1B,

0x70,0x00,0xbf,
0x72,0x26,0xD7,
0x72,0x23,0x2B,
0x72,0x0A,0x1B,

0x70,0x00,0xbf,
0x72,0x26,0xD9,
0x72,0x23,0x2B,
0x72,0x0A,0x1B,

0x70,0x00,0xbf,
0x72,0x26,0xDb,
0x72,0x23,0x2B,
0x72,0x0A,0x1B
};
unsigned char buf4_END_GAMMA[] = {
 //end gamma

0x70,0x00,0xbc,
0x72,0x00,0x05,

0x70,0x00,0xbf,
0x72,0xd8,0x70,
0x72,0xff,0x00,
0x72,0x00,0x80,

0x70,0x00,0xbc,
0x72,0x00,0x02,

0x70,0x00,0xbf,
0x72,0x01,0xff,

0x70,0x00,0xbc,
0x72,0x00,0x03,

0x70,0x00,0xbf,
0x72,0x99,0xc6,
0x72,0x00,0x33,

0x70,0x00,0xbc,
0x72,0x00,0x03,

0x70,0x00,0xbf,
0x72,0x9d,0xde,
0x72,0x00,0x30,

0x70,0x00,0xbc,
0x72,0x00,0x02,

0x70,0x00,0xbf,
0x72,0x00,0x14,

0x70,0x00,0xbf,
0x72,0x07,0xe9,

0x70,0x00,0xbc,
0x72,0x00,0x03,

0x70,0x00,0xbf,
0x72,0x60,0xed,
0x72,0x00,0x10,

0x70,0x00,0xbc,
0x72,0x00,0x02,

0x70,0x00,0xbf,
0x72,0x12,0xec,

0x70,0x00,0xbc,
0x72,0x00,0x05,

0x70,0x00,0xbf,
0x72,0x77,0xcd,
0x72,0x34,0x7b,
0x72,0x00,0x08,

0x70,0x00,0xbc,
0x72,0x00,0x08,

0x70,0x00,0xbf,
0x72,0x03,0xc3,
0x72,0x34,0x05,
0x72,0x01,0x05,
0x72,0x54,0x44,

0x70,0x00,0xbc,
0x72,0x00,0x06,

0x70,0x00,0xbf,
0x72,0x02,0xc4,
0x72,0x58,0x03,
0x72,0x5a,0x58,

0x70,0x00,0xbc,
0x72,0x00,0x04,

0x70,0x00,0xbf,
0x72,0xdf,0xcb,
0x72,0x00,0x80,

0x70,0x00,0xbc,
0x72,0x00,0x03,

0x70,0x00,0xbf,
0x72,0x15,0xea,
0x72,0x00,0x28,

0x70,0x00,0xbc,
0x72,0x00,0x05,

0x70,0x00,0xbf,
0x72,0x38,0xf0,
0x72,0x00,0x00,
0x72,0x00,0x00,

0x70,0x00,0xbc,
0x72,0x00,0x04,

0x70,0x00,0xbf,
0x72,0x60,0xc9,
0x72,0x82,0x00,

0x70,0x00,0xbc,
0x72,0x00,0x09,

0x70,0x00,0xbf,
0x72,0x00,0xb5,
0x72,0x05,0x05,
0x72,0x04,0x1e,
0x72,0x20,0x40,
0x72,0x00,0xfc,

0x70,0x00,0xbc,
0x72,0x00,0x02,

0x70,0x00,0xbf,
0x72,0x08,0x36
};
unsigned char buf5[] = {
    0x70,0x00,0xbc,
    0x72,0x00,0x01,
    
    0x70,0x00,0xbf,
    0x72,0x00,0x11
};    
unsigned char buf6[] = {
    0x70,0x00,0xbf,
    0x72,0x00,0x29
} ;   
unsigned char buf7[] = {
    0x70,0x00,0xbc,
    0x72,0x00,0x05,

    0x70,0x00,0xbf,
    0x72,0x18,0xF0,
    0x72,0xFF,0xFF,
    0x72,0x00,0x00    
};
unsigned char buf8[] = {
    //Enable MIPI Video
    0x70,0x00,0xb7,
    0x72,0x03,0x09 
};


int LG_write_init_cmd (void)
{
        int ret;
        printk("###%s###:send LG_LCD init CMD\n",__func__);
		do{
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf0, sizeof(buf0));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);
        
	    mdelay(100+MORE);
        
 		do{
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf1_START, sizeof(buf1_START));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);
        
        mdelay(100+MORE);
        
 		do{
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf2, sizeof(buf2));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);

        mdelay(MORE);
        
 		do{
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf3_GAMMA, sizeof(buf3_GAMMA));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);

        mdelay(MORE);

        do{
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf4_END_GAMMA, sizeof(buf4_END_GAMMA));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);

        mdelay(10+MORE);

        do{
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf5, sizeof(buf5));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);

        mdelay(50+MORE); 
        
        do{
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf6, sizeof(buf6));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);

        mdelay(200+MORE); 

        do{
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf7, sizeof(buf7));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
		}while(0);

        mdelay(MORE); 
        
        do{
            #if 0
			ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf8, sizeof(buf8));
			if (ret == 0){
				printk("cmmd_write error\n");
				return ret;
			}
            #else
            ret = lcd_init_cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  buf8, sizeof(buf8));
		    if (ret == DSIM_FALSE){
	            printk("cmd_write error\n");
	            return ret;
            }
            #endif
		}while(0);
        
        mdelay(MORE);        
}
#endif
//==========================================================

#define TEST_NORMAL




//  unsigned char param3[] = {0xb6,0x16,0x0f,0x00,0x77};
unsigned char param3[] = {0xb6,0x16,0x0f,0x00,0x00};

  unsigned char param3_a[] = {0xb8,0x00,0x06,0x08,0x00,0x07,0x09,0x23,0x04};
  	
//unsigned char param4[] = {0x04,0xb9,0x22,0x08,0xff,0xff,0x0f};
  unsigned char param4[] = {0xb9,0x04,0x08,0x22,0xff,0xff,0x0f};
//unsigned char param5[] = {0x0e,0xba,0x10,0x0e,0x0a,0x10,0x0c,0x0a,0x0c};
  unsigned char param5[] = {0xba,0x0e,0x0e,0x10,0x10,0x0a,0x0a,0x0c,0x0c};
//unsigned char param6[] = {0x00,0xb8,0x08,0x06,0x07,0x00,0x23,0x09,0x04};
 unsigned char param6[] = {0xb8,0x00,0x06,0x08,0x00,0x07,0x09,0x23,0x04};////**** 
//unsigned char param7[] = {0xa1,0xbb,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1};
  unsigned char param7[] = {0xbb,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1};
//unsigned char param8[] = {0x00,0xbc,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  unsigned char param8[] = {0xbc,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
//unsigned char param9[] = {0x0f,0xbd,0x11,0x0f,0x0b,0x11,0x0d,0x0b,0x0d};
  unsigned char param9[] = {0xbd,0x0f,0x0f,0x11,0x11,0x0b,0x0b,0x0d,0x0d};
//unsigned char param10[] = {0xa1,0xbe,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1};
  unsigned char param10[] = {0xbe,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1};
//unsigned char param11[] = {0x00,0xbf,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  unsigned char param11[] = {0xbf,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
//wait 100ms
//unsigned char param12[] = {0x16,0xb1,0x12,0x1e};
  unsigned char param12[] = {0xb1,0x16,0x1e,0x12};
//unsigned char param13[] = {0x01,0xe0,0x02,0x03,0x01,0x00};
  unsigned char param13[] = {0xe0,0x01,0x03,0x02,0x00,0x01};
//gamma
//unsigned char param14[] = {0x00,0xD0,0x10,0x00,0x22,0x1E,0x2E};
  unsigned char param14[] = {0xD0,0x00,0x00,0x1E,0x27,0x2d,0x35};/////ly: gama
 // unsigned char param14[] = {0xD0,0x00,0x00,0x10,0x1E,0x22,0x2E};//gama from zdx

  unsigned char param15[] = {0xD1,0x2d,0x27,0x1a,0x1b,0x0a};
  
//unsigned char param15[] = {0x00,0xD2,0x10,0x00,0x22,0x1E,0x2E};
  unsigned char param16[] = {0xD2,0x00,0x00,0x1e,0x27,0x2d,0x35};

  unsigned char param17[] = {0xD3,0x2d,0x27,0x1a,0x1b,0x0a};

//unsigned char param16[] = {0x00,0xD4,0x10,0x00,0x22,0x1E,0x2E};
  unsigned char param18[] = {0xD4,0x00,0x00,0x1e,0x27,0x2d,0x35};

  unsigned char param19[] = {0xD5,0x2d,0x27,0x1a,0x1b,0x0a};

//unsigned char param17[] = {0x00,0xD6,0x10,0x00,0x22,0x1E,0x2E};
  unsigned char param20[] = {0xD6,0x00,0x00,0x1e,0x1E,0x27,0x2d,0x35};

  unsigned char param21[] = {0xD7,0x2d,0x27,0x1a,0x1b,0x0a};

//unsigned char param18[] = {0x00,0xD8,0x10,0x00,0x22,0x1E,0x2E};
  unsigned char param22[] = {0xD8,0x00,0x00,0x1e,0x27,0x2d,0x35};

  unsigned char param23[] = {0xD9,0x2d,0x27,0x1a,0x1b,0x0a};


//unsigned char param19[] = {0x00,0xDa,0x10,0x00,0x22,0x1E,0x2E};
  unsigned char param24[] = {0xDa,0x00,0x00,0x1e,0x27,0x2d,0x35};

  unsigned char param25[] = {0xDb,0x2d,0x27,0x1a,0x1b,0x0a};

//unsigned char param26[] = {0xd8,0x70,0xff,0x00,0x80};
  unsigned char param26[] = {0x70,0xd8,0x00,0xff,0x80};
//unsigned char param27[] = {0x01,0xff};
  unsigned char param27[] = {0xff,0x01};
//unsigned char param28[] = {0x99,0xc6,0x33};
  //unsigned char param28[] = {0xc6,0x88,0x33};
  unsigned char param28[] = {0xc6,0x99,0x33};
//unsigned char param29[] = {0x9d,0xde,0x30};
  unsigned char param29[] = {0xde,0x9d,0x30};
//unsigned char param30[] = {0x00,0x14};
  unsigned char param30[] = {0x14,0x00};
//unsigned char param31[] = {0x07,0xe9};
  unsigned char param31[] = {0xe9,0x07};
//unsigned char param32[] = {0x60,0xed,0x10};
  unsigned char param32[] = {0xed,0x60,0x10};
//unsigned char param33[] = {0x12,0xec};
  unsigned char param33[] = {0xec,0x12};
//unsigned char param34[] = {0x77,0xcd,0x34,0x7b,0x08};
  unsigned char param34[] = {0xcd,0x77,0x7b,0x34,0x08};
//unsigned char param35[] = {0x03,0xc3,0x34,0x05,0x01,0x05,0x54,0x44};
  unsigned char param35[] = {0xc3,0x03,0x05,0x34,0x05,0x01,0x44,0x54};
//unsigned char param36[] = {0x02,0xc4,0x58,0x03,0x5a,0x58};
  unsigned char param36[] = {0xc4,0x02,0x03,0x58,0x58,0x5a};
//unsigned char param37[] = {0xdf,0xcb,0x00,0x80};
//  unsigned char param37[] = {0xcb,0xd8,0x80,0x00};
  unsigned char param37[] = {0xcb,0xdf,0x80,0x00};
//unsigned char param38[] = {0x15,0xea,0x28};
  unsigned char param38[] = {0xea,0x15,0x28};
//unsigned char param39[] = {0x38,0xf0,0x00,0x00,0x00};
  unsigned char param39[] = {0xf0,0x38,0x00,0x00,0x00};

//unsigned char param40[] = {0x60,0xc9,0x82,0x00};
  unsigned char param40[] = {0xc9,0x60,0x00,0x82};
//unsigned char param41[] = {0x00,0xb5,0x05,0x05,0x04,0x1e,0x20,0x40,0xfc};
  unsigned char param41[] = {0xb5,0x00,0x05,0x05,0x1e,0x04,0x40,0x20,0xfc};
//unsigned char param42[] = {0x08,0x36};
  unsigned char param42[] = {0x36,0x08};
//wait 10ms
  unsigned char param43[] = {0x11};
//wait 50ms
  unsigned char param44[] = {0x29};
//wait 200ms
//unsigned char param45[] = { 0x18,0xF0,0xFF,0xFF,0x00};
  unsigned char param45[] = {0xF0,0x18,0xFF,0xFF,0x00};




void ZDX_LG_write_init_cmd(void)
{
    printk("###%s is called!###\n",__func__);
    btl507212_write_1(0xe1,0xa3);   //param1
        mdelay(1);
	btl507212_write_1(0xb3,0x00);   //param2
        mdelay(1);
	btl507212_write(param3,sizeof(param3));
	mdelay(1);
	btl507212_write(param3_a,sizeof(param3_a));
	mdelay(1);
	btl507212_write(param4,sizeof(param4));
        mdelay(1);
	btl507212_write(param5,sizeof(param5));
        mdelay(1);
	//btl507212_write(param6,sizeof(param6));
     //   mdelay(1);
	btl507212_write(param7,sizeof(param7));
        mdelay(1);
	btl507212_write(param8,sizeof(param8));
        mdelay(1);
	btl507212_write(param9,sizeof(param9));
        mdelay(1);
	btl507212_write(param10,sizeof(param10));
        mdelay(1);
	btl507212_write(param11,sizeof(param11));
        mdelay(1);
	btl507212_write(param12,sizeof(param12));
        mdelay(1);
	btl507212_write(param13,sizeof(param13));
	mdelay(1);
	btl507212_write(param14,sizeof(param14));
        mdelay(1);
	btl507212_write(param15,sizeof(param15));
        mdelay(1);
	btl507212_write(param16,sizeof(param16));
        mdelay(1);
	btl507212_write(param17,sizeof(param17));
        mdelay(1);
	btl507212_write(param18,sizeof(param18));
        mdelay(1);
	btl507212_write(param19,sizeof(param19));
        mdelay(1);
	btl507212_write(param20,sizeof(param20));
        mdelay(1);
	btl507212_write(param21,sizeof(param21));
        mdelay(1);
	btl507212_write(param22,sizeof(param22));
        mdelay(1);
	btl507212_write(param23,sizeof(param23));
	mdelay(1);
	btl507212_write(param24,sizeof(param24));
        mdelay(1);
	btl507212_write(param25,sizeof(param25));
        mdelay(1);
	btl507212_write(param26,sizeof(param26));
        mdelay(1);
#ifdef TEST_NORMAL
//	btl507212_write(param27,sizeof(param27));
	btl507212_write_1(0xff,0x01);    //param27
        mdelay(1);
#endif
	btl507212_write(param28,sizeof(param28));
        mdelay(1);
	btl507212_write(param29,sizeof(param29));
        mdelay(1);
#ifdef TEST_NORMAL
	btl507212_write_1(0x14,0x00);    //param30
        mdelay(1);
	btl507212_write_1(0xe9,0x07);    //param30
        mdelay(1);
#endif
	btl507212_write(param32,sizeof(param32));
        mdelay(1);
#ifdef TEST_NORMAL
	btl507212_write_1(0xec,0x12); //param33
#endif
	mdelay(1);
	btl507212_write(param34,sizeof(param34));
        mdelay(1);
	btl507212_write(param35,sizeof(param35));
        mdelay(1);
	btl507212_write(param36,sizeof(param36));
        mdelay(1);
	btl507212_write(param37,sizeof(param37));
        mdelay(1);
	btl507212_write(param38,sizeof(param38));
        mdelay(1);
	btl507212_write(param39,sizeof(param39));
        mdelay(1);
	btl507212_write(param40,sizeof(param40));
        mdelay(1);
	btl507212_write(param41,sizeof(param41));
        mdelay(1);
        
#ifdef TEST_NORMAL
	#if 0
	btl507212_write_1(0x36,0x08); //param42
	#else
	btl507212_write_1(0x36,0x0B); //rotation 180 degree
	#endif
#else
	
#endif
        mdelay(10);

	btl507212_write_1(0x53,0x2c); //open bl 0x24 

	btl507212_write_1(0x51,current_brightness);		

	btl507212_write_1(0x55,0x01);//0x03
	
	btl507212_write_1(0x5e,0x00);
	 mdelay(10);
	btl507212_write_0(0x11);   //param43
	mdelay(120);
	btl507212_write_0(0x29);   //param44
	mdelay(10);
	
	//btl507212_write(param45,sizeof(param45));
}

#if 1
unsigned char Read_LCDBoe_ID(void)
{
    //read chip ID
    int i;
	u32 read_buf[100];
	memset(read_buf, 0, sizeof(read_buf));
    //write long packet ,then boe read it by itself to distinguish different lcd
	u8 buf[] = {0xB9,0xFF,0x83,0x94};
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  (unsigned int )buf, sizeof buf);
    
    /*u8 buf[] = {0xf4};
	//ret = ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,  (unsigned int )buf, sizeof buf);
	otm1280a_write_1(0xf4, 0x00);*/
	int read_size;
	ddi_pd->cmd_write(ddi_pd->dsim_base, SET_MAX_RTN_PKT_SIZE, 100, 0);
	s5p_dsim_clr_state(ddi_pd->dsim_base,RxDatDone);
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_RD_NO_PARA, (u8)buf[0], 0);
	s5p_dsim_wait_state(ddi_pd->dsim_base, RxDatDone);
	s5p_dsim_read_rx_fifo(ddi_pd->dsim_base, read_buf, &read_size);
	u8 *p = &read_buf;
	for (i = 0; i < 3; i++)
	        printk("id = %#x \n", p[i]);
	printk("\n");
    return p[2];
}
#endif

