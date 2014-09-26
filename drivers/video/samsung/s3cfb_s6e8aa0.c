/* linux/drivers/video/samsung/s3cfb_s6e8aa0.c
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
#include <linux/regulator/driver.h>

#include <plat/gpio-cfg.h>
#include <plat/regs-dsim.h>

#include <mach/dsim.h>
#include <mach/mipi_ddi.h>

#include "s5p-dsim.h"
#include "s3cfb.h"
#include "s3cfb_s6e8aa0_para.h"


static struct mipi_ddi_platform_data *ddi_pd;
struct mutex  mipi_lock;
//static int  s6e8aa0_resume_status = 1;
static int late_set_brigthness = 0; /*during resume, late set brigthness*/
static int late_set_luminance = 0;

extern struct regulator *lcd_regulator ; 
extern struct regulator *lcd_vdd18_regulator ;
//EXPORT_SYMBOL(lcd_regulator);
//EXPORT_SYMBOL(lcd_vdd18_regulator);

static int s6e8ax0_set_brightness(int lum);

/**************start here!*********************/
static int lcd_initialized = 0;

static void mipi_cmd_write(const unsigned char *buf,int length)
{
	//printk("mipi_cmd_write length = %d\n",length);
	mutex_lock(&mipi_lock);
	if (length > 2)
		ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, length);
	else if (length == 2){
		if  (buf[1] == 0)
			ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, buf[0], buf[1]);
		else
			ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, buf[0], buf[1]);
	}
	else
		printk("%s size error!\n",__func__);
	mutex_unlock(&mipi_lock);

}

/*
static void s6e8aa0_read(const u8 addr, u16 count, u8 *buf)
{
	ddi_pd->cmd_read(ddi_pd->dsim_base, addr, count, buf);
}
*/

static void s6e8aa0_display_on(struct device *dev)
{
	mipi_cmd_write(display_on_cmd,sizeof(display_on_cmd));
	lcd_initialized = 1;
	if (late_set_brigthness){
			late_set_brigthness = 0;
			s6e8ax0_set_brightness(late_set_luminance);
	}
}

static void s6e8aa0_display_off(struct device *dev)
{
	mipi_cmd_write(display_off_cmd,sizeof(display_off_cmd));
}

static void s6e8aa0_sleep_in(struct device *dev)
{
	mipi_cmd_write(enter_sleep_cmd,sizeof(enter_sleep_cmd));
}

static void s6e8aa0_sleep_out(struct device *dev)
{
	mipi_cmd_write(exit_sleep_cmd,sizeof(exit_sleep_cmd));
}


static void lcd_level2_cmd_set(void)
{
	mipi_cmd_write(level2_cmd_enable,sizeof(level2_cmd_enable));
	mipi_cmd_write(MTP_key_enable,sizeof(MTP_key_enable));
}

static void lcd_panel_cond_set(void)
{
	mipi_cmd_write(panel_cond_set1,sizeof(panel_cond_set1));
	mipi_cmd_write(panel_cond_set2,sizeof(panel_cond_set2));
}

static void lcd_gamma_cond_set(int gama_table_num)
{
	mipi_cmd_write(gamma_cond_set[gama_table_num].gama_cond,gamma_cond_set[gama_table_num].size);
	mipi_cmd_write(gamma_update_cmd,sizeof(gamma_update_cmd));
}

static void lcd_etc_cond_set(void)
{
	mipi_cmd_write(etc_cond_set_source_ctl,sizeof(etc_cond_set_source_ctl));
	mipi_cmd_write(etc_cond_set_pentile_ctl1,sizeof(etc_cond_set_pentile_ctl1));
	mipi_cmd_write(etc_cond_set_pentile_ctl2,sizeof(etc_cond_set_pentile_ctl2));
	mipi_cmd_write(etc_cond_set_mipi_ctl1,sizeof(etc_cond_set_mipi_ctl1));
	mipi_cmd_write(etc_cond_set_mipi_ctl2,sizeof(etc_cond_set_mipi_ctl2));
	mipi_cmd_write(etc_cond_set_mipi_ctl3,sizeof(etc_cond_set_mipi_ctl3));
	#ifdef MANUAL_120504
	//printk(KERN_ERR"\n\n\n ctrl4 have been writed!\n\n\n");
	mipi_cmd_write(etc_cond_set_mipi_ctl4,sizeof(etc_cond_set_mipi_ctl4));
	#endif
	mipi_cmd_write(etc_cond_set_power_ctl,sizeof(etc_cond_set_power_ctl));	
}

static void lcd_ELVSS_cond_set(int gama_table_num)
{
	ELVSS_cond_set[2] = gamma_cond_set[gama_table_num].elvss_level;
	//printk("ELVSS_cond_set[2] = 0x%x,gama_table_num = %d\n",ELVSS_cond_set[2],gama_table_num);
	mipi_cmd_write(ELVSS_cond_set,sizeof(ELVSS_cond_set));
}

static void lcd_acl_set(int enable)
{
	if (enable){
		mipi_cmd_write(acl_parameter_set,sizeof(acl_parameter_set));
		mipi_cmd_write(acl_on_cmd,sizeof(acl_on_cmd));
	}
	else
		mipi_cmd_write(acl_off_cmd,sizeof(acl_off_cmd));	
}

static int s6e8ax0_get_gamma_table(int lum)
{
	int gama_table_num,size_of_gama_table;
	
	size_of_gama_table = GAMA_COND_SET_TABLE_SIZE;//sizeof(gamma_cond_set)/sizeof(gamma_table);
	//printk("size_of_gama_table = %d,um = %d\n",size_of_gama_table,lum);
	//lum = GAMMA_300CD;
	
	for (gama_table_num=0;gama_table_num<size_of_gama_table;gama_table_num++){
		if (lum == gamma_cond_set[gama_table_num].lum)
			break;
	}
	if (gama_table_num == size_of_gama_table){
		printk("Error luminance:%d",lum);
		return -1;
	}
	
	//printk("get lum = %d\n",gamma_cond_set[gama_table_num].lum);

	return gama_table_num;
}


static int lcd_on_seq(void)
{
	int	ret = 0,gama_table_num;
	
	// in result of test, gamma update can update before gammaTableCommand of LCD-ON-sequence.
	// if update gamma after gammaTableCommand cannot be update to lcd
	// if update gamma after displayOnCommand cannot be update lcd.
	// so gammaTableCommand is moved to ahead of displayOnCommand.

	//mutex_lock(&(s6e8aa0_lcd.lock));

	if(lcd_initialized){
		printk("%s : Already ON -> return 1\n", __func__ );
		ret = -1;
	}
	else{
		printk("%s\n", __func__);
		
		lcd_level2_cmd_set();
		s6e8aa0_sleep_out(NULL);
		mdelay(5);	
		gama_table_num = s6e8ax0_get_gamma_table(LCD_POWER_ON_LUM);
		if (gama_table_num<0){
			gama_table_num = 0;
			printk("%s :gama_table_num error!\n", __func__ );
		}
		lcd_panel_cond_set();
		lcd_gamma_cond_set(gama_table_num);
		lcd_etc_cond_set();	/**/
		lcd_ELVSS_cond_set(gama_table_num);
		#ifdef AS_SPEC
		mdelay(120); /*spec: 120ms*/
		#endif
		if (LCD_ACL_ON) /*default alc off*/
			lcd_acl_set(LCD_ACL_ON);	
		//s6e8aa0_read(0x04,3,buf);
		//printk("=============buf[0]=%x=======buf[1]=%x=======buf[2]=%x==============lcd_pannel_on\n",buf[0],buf[1],buf[2]);	
		////s6e8aa0_display_on(NULL);
		//mdelay(20); /*spec no need delay*/
			
		//lcd_initialized = 1;
	}
	//mutex_unlock(&(s6e8aa0_lcd.lock));

	return ret;
}

static void lcd_pannel_on(void)
{
	#if 1
	lcd_on_seq();
	#else
	s6e8aa0_sleep_out(NULL);
	mdelay(20);	
	s6e8aa0_display_on(NULL);
	mdelay(20); /*spec no need delay*/
	#endif

}

static void lcd_panel_init(void)
{
	lcd_pannel_on();
}

static int s6e8aa0_panel_init(void)
{
	//mdelay(600);
	lcd_panel_init();

	return 0;
}

static int s6e8aa0_set_link(void *pd, unsigned int dsim_base,
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

//struct backlight_device		*s6e8ax0_bl_dev;
//static int brightness;
//static int current_brightness;
static void s6e8aa_s3cfb_set_lcd_info(struct s3cfb_global *ctrl);
static int s6e8aa0_probe(struct device *dev)
{
	int ret = 0;
	s3cfb_set_lcd_info=s6e8aa_s3cfb_set_lcd_info;
        lcd_regulator = regulator_get(NULL, "vdd33_lcd");  
        if (IS_ERR(lcd_regulator)) {
                printk("%s: failed to get %s\n", __func__, "vdd33_lcd");
                ret = -ENODEV;
                goto err_regulator;
        }   
        regulator_enable(lcd_regulator);        //yulu
        lcd_vdd18_regulator = regulator_get(NULL, "lcd_iovdd18");  
        if (IS_ERR(lcd_vdd18_regulator)) {
                printk("%s: failed to get %s\n", __func__, "lcd_iovdd18");
                ret = -ENODEV;
                goto err_regulator;
        }
        regulator_enable(lcd_vdd18_regulator);

	mutex_init(&mipi_lock);
	return ret;

err_regulator:
        regulator_put(lcd_regulator);
        regulator_put(lcd_vdd18_regulator);

	return ret;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static int  s6e8x_released=1;
#define LCD_RESUME_RESET
int s6e8aa0_early_suspend(void)
{

#if 0
	regulator_disable(lcd_regulator);       
	regulator_disable(lcd_vdd18_regulator);
	lcd_initialized = 0;
	s6e8aa0_resume_status = 0;
#else
	//lcd_initialized = 0;
	while(!s6e8x_released){printk("waitfor bl release!!!!!!!\n");}
//	int cid = 1;
	#ifdef LCD_RESUME_RESET
	lcd_initialized = 0;
	#endif
	//s6e8aa0_resume_status = 0;
	//s6e8ax0_set_brightness(0);
	s6e8aa0_display_off(NULL);///LY fot test
	#ifdef LCD_RESUME_RESET
	s6e8aa0_sleep_in(NULL);

	#ifdef AS_SPEC
	mdelay(120); /*spec: 120ms*/
	#else 
	mdelay(10); 
	#endif

	#endif
#endif
	/* disabled in fimd driver early_suspend func*/
	/*
	regulator_disable(lcd_regulator); 	
	regulator_disable(lcd_vdd18_regulator);
	*/	
	return 0;
}

int s6e8aa0_late_resume(void)
{
	printk("%s\n",__func__);

	#ifndef LCD_RESUME_RESET
	s6e8aa0_sleep_out(NULL);
	mdelay(10); /*spec: 120ms*/
	mipi_cmd_write(panel_cond_set1,2); /*rotate 180 degree*/
	//lcd_panel_cond_set();
	//mdelay(10);
	#endif
	//s6e8aa0_resume_status = 1;
	s6e8aa0_display_on(NULL);
	
	//s6e8aa0_resume_status = 1;
	return 0;
}
#else
#ifdef CONFIG_PM
static int s6e8aa0_suspend(void)
{
	
	return 0;
}

static int s6e8aa0_resume(struct device *dev)
{
	return 0;
}
#else
#define s6e8aa0_suspend	NULL
#define s6e8aa0_resume	NULL
#endif
#endif

static struct s3cfb_lcd s6e8aa0_lcd = 
#if 1
{
	.width = 720,
	.height = 1280,
	//.p_width = 60,		/* 59.76 mm */
	//.p_height = 106,	 /* 106.24 mm */
	.bpp = 24,

	.freq = 60,
	
	/*
	.cpu_timing = {
		.cs_setup = 0,
		.wr_setup = 0,
		.wr_act = 1,
		.wr_hold = 0,
	},
	*/
	
	.timing = {
		.h_fp = 5,//60,
		.h_bp = 5,//30,
		.h_sw = 5,//2,
		.v_fp = 13,
		.v_fpe = 1,//0,
		.v_bp = 1,//2,
		.v_bpe = 1,//0,
		.v_sw = 2,//1,
		.cmd_allow_len = 11,//0x4, /* v_fp=stable_vfp + cmd_allow_len */
	},

	.polarity = {
		.rise_vclk = 1,// 0,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};
#elif 0
{
	.name = "s6e8aa0",
	.width = 720,
	.height = 1280,
	.p_width = 60,		/* 59.76 mm */
	.p_height = 106,	 /* 106.24 mm */
	.bpp = 24,

	.freq = 60,

	/* minumun value is 0 except for wr_act time. */
	.cpu_timing = {
		.cs_setup = 0,
		.wr_setup = 0,
		.wr_act = 1,
		.wr_hold = 0,
	},

	.timing = {
		.h_fp = 5,
		.h_bp = 5,
		.h_sw = 5,
		.v_fp = 13,
		.v_fpe = 1,
		.v_bp = 1,
		.v_bpe = 1,
		.v_sw = 2,
		.cmd_allow_len = 11,	 /* v_fp=stable_vfp + cmd_allow_len */
		.stable_vfp = 2,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
}
#else
{
	.width  = 1024,
	.height = 600,
	.bpp    = 32,
	.freq   = 60,
	
	.timing = {
		.h_fp = 105,
		.h_bp = 213,
		.h_sw = 2,
		
		.v_fp = 10,
		.v_fpe = 0,
		.v_bp = 23,
		.v_bpe = 0,
		.v_sw = 2,
	},
	
	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 1,
		.inv_vsync = 1,
		.inv_vden = 0,
	},
};
#endif

static struct mipi_lcd_driver s6e8aa0_mipi_driver = {
	.name = "s6e8aa0",
	.init = s6e8aa0_panel_init,
	.display_on = s6e8aa0_display_on,
	.set_link = s6e8aa0_set_link,
	.probe = s6e8aa0_probe,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend = s6e8aa0_suspend,
	.resume = s6e8aa0_resume,
#endif	
	.display_off = s6e8aa0_display_off,
	.lcd_panel_info = &s6e8aa0_lcd,
};

static int s6e8aa0_init(void)
{
	s5p_dsim_register_lcd_driver(&s6e8aa0_mipi_driver);
	return 0;
}

static void s6e8aa0_exit(void)
{
	regulator_put(lcd_regulator);
        regulator_put(lcd_vdd18_regulator);

	return;
}

/* name should be fixed as 's3cfb_set_lcd_info' */
static void s6e8aa_s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
	s6e8aa0_lcd.init_ldi = NULL;
	ctrl->lcd = &s6e8aa0_lcd;
}

#if 1
#define MIN_BRIGHTNESS		0
#define MAX_BRIGHTNESS		255

#define MAX_GAMMA			300
#define DEFAULT_BRIGHTNESS		160
#define DEFAULT_GAMMA_LEVEL		GAMMA_160CD

#define GAMMA_PARAM_SIZE	26
#define ELVSS_PARAM_SIZE	3


#define  CMD_DELAY  2
static int s6e8ax0_set_brightness(int lum)
{
	
	int gama_table_num;
	static int gama_table_num_pre;

	if (lcd_initialized) {
		//ret = update_brightness(lcd, 0);
		gama_table_num = s6e8ax0_get_gamma_table(lum);
		
		//printk(KERN_ERR"\n\n\n+++xufei:%s is called,lcd_initialized is %d\n",__func__,lcd_initialized);
		//printk(KERN_ERR"gama_table_num is %d\n\n\n",gama_table_num);

		if (gama_table_num < 0){
			printk("%s :gama_table_num error!\n", __func__ );
			return -1;
		}

		/*
		if(gama_table_num_pre!=gama_table_num)
			gama_table_num_pre=gama_table_num;
		else
			return 0;
		lcd_gamma_cond_set(gama_table_num);
		lcd_ELVSS_cond_set(gama_table_num);
		*/
    
		//xufei 
		if(gama_table_num_pre==gama_table_num)
			return 0;
		else{
			
			gama_table_num_pre=gama_table_num;	
			//printk("gama_table_num_pre = %d\n",gama_table_num_pre);		
		}
		//write gamma value three times!
		lcd_gamma_cond_set(gama_table_num);
		mdelay(CMD_DELAY);
		lcd_ELVSS_cond_set(gama_table_num);
		mdelay(CMD_DELAY);

		/*lcd_gamma_cond_set(gama_table_num);
		mdelay(CMD_DELAY);
		lcd_ELVSS_cond_set(gama_table_num);
		mdelay(CMD_DELAY);

		lcd_gamma_cond_set(gama_table_num);
		mdelay(CMD_DELAY);
		lcd_ELVSS_cond_set(gama_table_num);
		mdelay(CMD_DELAY);*/

	}

	return 0;
}

int s6e8ax0_update_brightness(struct backlight_device *bd)
{
	int ret = 0;
	unsigned int cur_lum;
	//static int prev_lum=-1;
	int brightness = bd->props.brightness;
	int div = (bd->props.max_brightness*1000)/GAMMA_MAX;

	if (!lcd_initialized){//ly fot store brightness value
		cur_lum = (brightness*1000/div);

		if (cur_lum >= GAMMA_MAX)
			cur_lum = GAMMA_MAX-1;
		late_set_brigthness = 1;
		late_set_luminance=cur_lum;
		return 0;
	}/*else if (!s6e8aa0_resume_status){
		//ly: init but not resume yet. just wait for release
		while(!s6e8aa0_resume_status){
			printk("wait for lcd resume!!!!!! \n");
			msleep(5);
		}
	}*/

	s6e8x_released=0;
	/* dev_info(&lcd->ld->dev, "%s: brightness=%d\n", __func__, brightness); */
	//printk("brightness = %d\n",brightness);
	
	cur_lum = (brightness*1000/div);
	//printk("cur_lum = %d\n",cur_lum);
	if (cur_lum >= GAMMA_MAX)
		cur_lum = GAMMA_MAX-1;
#if 0
	if (!s6e8aa0_resume_status){
		//printk("s6e8aa0_resume_status is 0\n");
		late_set_brigthness = 1;
		late_set_luminance = cur_lum;
		s6e8x_released=1;
		return 0;
	}
#endif

//	if (prev_lum == cur_lum){
//		printk("same luminance : %d,ignore!\n", cur_lum);
//		s6e8x_released=1;
//		return 0;
//		}
//	else
//		prev_lum = cur_lum;
	
	/* ylk: workaround for low brightness problem at boot once in a while (Å¼¶û)*/
	/*if (cur_lum == 0)
	{
		s6e8x_released=1;
		return 0;
	}*/

	s6e8ax0_set_brightness(cur_lum);
	s6e8x_released=1;
	return ret;
}


#endif

module_init(s6e8aa0_init);
module_exit(s6e8aa0_exit);

MODULE_LICENSE("GPL");
