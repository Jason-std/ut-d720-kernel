/* linux/drivers/video/samsung/s3cfb-main.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Core file for Samsung Display Controller (FIMD) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/memory.h>
#include <linux/pm_runtime.h>
#include <plat/clock.h>
#include <plat/media.h>
#include <mach/media.h>
#include "s3cfb.h"	
#include <linux/regulator/driver.h> 

#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#include <linux/suspend.h>
#endif

#include <asm/mach-types.h>

#include "../../oem_drv/power_gpio.h"
#include <linux/gpio.h>
#include <linux/delay.h>
#include <plat/gpio-cfg.h>

#include "urbetter/check.h"

#define SLOW_DOWN_FREQ

#define  LCD_POWER_CTRL


extern char s_selected_lcd_name[32];

extern unsigned int lcd_reg_write(unsigned int reg, unsigned int value);

#ifdef SLOW_DOWN_FREQ
#define VSTATUS_IS_ACTIVE(reg)	(reg == VIDCON1_VSTATUS_ACTIVE)
#define VSTATUS_IS_FRONT(reg)		(reg == VIDCON1_VSTATUS_FRONTPORCH)
#define reg_mask(shift, size)		((0xffffffff >> (32 - size)) << shift)
#endif
struct s3cfb_fimd_desc		*fbfimd;
//#if ESD_FOR_LCD
int fbdev_register_finished = 0;
//#endif
//extern int lcd_id;
//extern int ESD_FOR_LCD;
struct s3cfb_global *g_fbdev;

#if defined(CONFIG_TOUCHSCREEN_GOODIX)
extern struct delayed_work tp_led_delayed_work;
extern void led_put_out(void);
extern void led_light_up(void);
#endif

//extern void s5p_dsim_frame_done_interrupt_enable(u8 enable);

//extern struct regulator *lcd_regulator; 
//extern struct regulator *lcd_vdd18_regulator;
//#ifdef ESD_FOR_LCD
struct s3cfb_global *tmp_fbdev[2];
//#endif
inline struct s3cfb_global *get_fimd_global(int id)
{
	struct s3cfb_global *fbdev;

	if (id < 5)
		fbdev = fbfimd->fbdev[0];
	else
		fbdev = fbfimd->fbdev[1];

	return fbdev;
}

int s3cfb_vsync_status_check(void)
{
	struct s3cfb_global *fbdev[2];
	fbdev[0] = fbfimd->fbdev[0];

	if (fbdev[0]->regs != 0 && fbdev[0]->system_state == POWER_ON)
		return s3cfb_check_vsync_status(fbdev[0]);
	else
		return 0;
}

static irqreturn_t s3cfb_irq_frame(int irq, void *dev_id)
{
	struct s3cfb_global *fbdev[2];
	fbdev[0] = fbfimd->fbdev[0];

	if (fbdev[0]->regs != 0)
		s3cfb_clear_interrupt(fbdev[0]);

	fbdev[0]->wq_count++;
	wake_up(&fbdev[0]->wq);

	return IRQ_HANDLED;
}

#ifdef CONFIG_FB_S5P_TRACE_UNDERRUN
static irqreturn_t s3cfb_irq_fifo(int irq, void *dev_id)
{
	struct s3cfb_global *fbdev[2];
	fbdev[0] = fbfimd->fbdev[0];

	if (fbdev[0]->regs != 0)
		s3cfb_clear_interrupt(fbdev[0]);

	return IRQ_HANDLED;
}
#endif

int s3cfb_register_framebuffer(struct s3cfb_global *fbdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	int ret, i, j;

	/* on registering framebuffer, framebuffer of default window is registered at first. */
	for (i = pdata->default_win; i < pdata->nr_wins + pdata->default_win; i++) {
		j = i % pdata->nr_wins;
		ret = register_framebuffer(fbdev->fb[j]);
		if (ret) {
			dev_err(fbdev->dev, "failed to register	\
				framebuffer device\n");
			return -EINVAL;
		}
#ifdef CONFIG_FB_S5P_SOFTBUTTON_UI /* Add Menu UI Window 4 */
		if(j==4){
			dev_info(fbdev->dev, " set parameters for win4");
			s3cfb_check_var_window(fbdev, &fbdev->fb[j]->var, fbdev->fb[j]);
			s3cfb_set_par_window(fbdev, fbdev->fb[j]);
		}
#endif
#ifndef CONFIG_FRAMEBUFFER_CONSOLE
		if (j == pdata->default_win) {
			s3cfb_check_var_window(fbdev, &fbdev->fb[j]->var,
					fbdev->fb[j]);
			s3cfb_set_par_window(fbdev, fbdev->fb[j]);
			s3cfb_draw_logo(fbdev->fb[j]);
		}
#endif
	}
	return 0;
}

static int s3cfb_sysfs_show_win_power(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct s3c_platform_fb *pdata = to_fb_plat(dev);
	struct s3cfb_window *win;
	char temp[16];
	int i;
	struct s3cfb_global *fbdev[1];
	fbdev[0] = fbfimd->fbdev[0];

	for (i = 0; i < pdata->nr_wins; i++) {
		win = fbdev[0]->fb[i]->par;
		sprintf(temp, "[fb%d] %s\n", i, win->enabled ? "on" : "off");
		strcat(buf, temp);
	}

	return strlen(buf);
}

static int s3cfb_sysfs_store_win_power(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t len)
{
	struct s3c_platform_fb *pdata = to_fb_plat(dev);
	char temp[4] = { 0, };
	const char *p = buf;
	int id, to;
	struct s3cfb_global *fbdev[1];
	fbdev[0] = fbfimd->fbdev[0];

	while (*p != '\0') {
		if (!isspace(*p))
			strncat(temp, p, 1);
		p++;
	}

	if (strlen(temp) != 2)
		return -EINVAL;

	id = simple_strtoul(temp, NULL, 10) / 10;
	to = simple_strtoul(temp, NULL, 10) % 10;

	if (id < 0 || id > pdata->nr_wins)
		return -EINVAL;

	if (to != 0 && to != 1)
		return -EINVAL;

	if (to == 0)
		s3cfb_disable_window(fbdev[0], id);
	else
		s3cfb_enable_window(fbdev[0], id);

	return len;
}

//==============================================================
#ifdef SLOW_DOWN_FREQ
static unsigned long level;
static ssize_t level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    if(!level)
       return sprintf(buf, "the FIMD frame is 60HZ\n");
    else
       return sprintf(buf, "the FIMD frame is 30HZ\n");
	//return sprintf(buf, "%dhz, div=%d\n", g_info->table[g_info->level].hz, get_div(g_info->dev));
}
static ssize_t level_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long value;
	int ret;
	//ret = kstrtoul(buf, 0, (unsigned long *)&value);
    ret = strict_strtoul(buf, 0, &value);
	printk("%s :: value=%d\n", __func__, value);
    level = value;
    ret=s3cfb_set_clock_down(g_fbdev,value);      
    return count;
	/*if (value)
		ret = lcdfreq_lock(g_info->dev, value);//if value>0 change to limit mode
	else
		ret = lcdfreq_lock_free(g_info->dev);*/
}
static DEVICE_ATTR(level, S_IRUSR|S_IRGRP|S_IWUSR|S_IWGRP, level_show, level_store);
//static DEVICE_ATTR(usage, S_IRUGO, usage_show, NULL);
//static DEVICE_ATTR(freqswitch, S_IRUGO|S_IWUSR, freqswitch_show, freqswitch_store);

static struct attribute *lcdfreq_attributes[] = {
	&dev_attr_level.attr,
	//&dev_attr_usage.attr,
	//&dev_attr_freqswitch.attr,
	NULL,
};
static struct attribute_group lcdfreq_attr_group = {
	.name = "lcdfreq",
	.attrs = lcdfreq_attributes,
};
#endif 

static DEVICE_ATTR(win_power, 0644,
	s3cfb_sysfs_show_win_power, s3cfb_sysfs_store_win_power);
//ly to fix lcd suspend resume splash issue
static struct early_suspend	blank_early_suspend;
static void s3cfb_blank_screen_early_suspend(struct early_suspend *h);
static void s3cfb_blank_screen_late_resume(struct early_suspend *h);
//void (*s3cfb_set_lcd_info)(struct s3cfb_global *ctrl);
static int s3cfb_probe(struct platform_device *pdev)
{
    
    printk("%s is called \n",__func__);
    struct s3c_platform_fb *pdata = NULL;
	struct resource *res = NULL;
	struct s3cfb_global *fbdev[2];
	int ret = 0;
	int i = 0;

#ifdef CONFIG_EXYNOS_DEV_PD
	/* to use the runtime PM helper functions */
	pm_runtime_enable(&pdev->dev);
	/* enable the power domain */
	pm_runtime_get_sync(&pdev->dev);
#endif

	fbfimd = kzalloc(sizeof(struct s3cfb_fimd_desc), GFP_KERNEL);

	if (FIMD_MAX == 2)
		fbfimd->dual = 1;
	else
		fbfimd->dual = 0;

	for (i = 0; i < FIMD_MAX; i++) {
		/* global structure */
		fbfimd->fbdev[i] = kzalloc(sizeof(struct s3cfb_global), GFP_KERNEL);
		fbdev[i] = fbfimd->fbdev[i];
                g_fbdev = fbfimd->fbdev[i];
		if (!fbdev[i]) {
			dev_err(fbdev[i]->dev, "failed to allocate for	\
				global fb structure fimd[%d]!\n", i);
			goto err0;
		}

		fbdev[i]->dev = &pdev->dev;
		s3cfb_set_lcd_info(fbdev[i]);

		/* platform_data*/
		pdata = to_fb_plat(&pdev->dev);
		if (pdata->cfg_gpio)
			pdata->cfg_gpio(pdev);

		if (pdata->clk_on)
			pdata->clk_on(pdev, &fbdev[i]->clock);

		/* io memory */
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res) {
			dev_err(fbdev[i]->dev,
				"failed to get io memory region\n");
			ret = -EINVAL;
			goto err1;
		}
		res = request_mem_region(res->start,
					res->end - res->start + 1, pdev->name);
		if (!res) {
			dev_err(fbdev[i]->dev,
				"failed to request io memory region\n");
			ret = -EINVAL;
			goto err1;
		}
		fbdev[i]->regs = ioremap(res->start, res->end - res->start + 1);
		if (!fbdev[i]->regs) {
			dev_err(fbdev[i]->dev, "failed to remap io region\n");
			ret = -EINVAL;
			goto err1;
		}

		/* irq */
		fbdev[i]->irq = platform_get_irq(pdev, 0);
		if (request_irq(fbdev[i]->irq, s3cfb_irq_frame, IRQF_SHARED,
				pdev->name, fbdev[i])) {
			dev_err(fbdev[i]->dev, "request_irq failed\n");
			ret = -EINVAL;
			goto err2;
		}

#ifdef CONFIG_FB_S5P_TRACE_UNDERRUN
		if (request_irq(platform_get_irq(pdev, 1), s3cfb_irq_fifo,
				IRQF_DISABLED, pdev->name, fbdev[i])) {
			dev_err(fbdev[i]->dev, "request_irq failed\n");
			ret = -EINVAL;
			goto err2;
		}

		s3cfb_set_fifo_interrupt(fbdev[i], 1);
		dev_info(fbdev[i]->dev, "fifo underrun trace\n");
#endif
		/* hw setting */
		s3cfb_init_global(fbdev[i]);

		fbdev[i]->system_state = POWER_ON;

		/* alloc fb_info */
		if (s3cfb_alloc_framebuffer(fbdev[i], i)) {
			dev_err(fbdev[i]->dev, "alloc error fimd[%d]\n", i);
			goto err3;
		}
       
/*        led_light_up();
        INIT_DELAYED_WORK(&tp_led_delayed_work, led_put_out);
        schedule_delayed_work(&tp_led_delayed_work, 3000);
*/

		/* register fb_info */
		if (s3cfb_register_framebuffer(fbdev[i])) {//display pc4 logo
			dev_err(fbdev[i]->dev, "register error fimd[%d]\n", i);
			goto err3;
		}

		/* enable display */
		s3cfb_set_clock(fbdev[i]);
		s3cfb_enable_window(fbdev[0], pdata->default_win);
#ifdef CONFIG_FB_S5P_SOFTBUTTON_UI /* Add Menu UI */
		s3cfb_enable_window(fbdev[0], 4);
#endif

		s3cfb_update_power_state(fbdev[i], pdata->default_win,FB_BLANK_UNBLANK);
		s3cfb_display_on(fbdev[i]);
#ifdef SLOW_DOWN_FREQ
        ret = sysfs_create_group(&pdev->dev.kobj, &lcdfreq_attr_group);
        if (ret < 0) {
            printk("%s error!: sysfs_create_group\n", __func__);    
        }
#endif
///EARLY_SUSPEND_LEVEL_BLANK_SCREEN
#ifdef CONFIG_HAS_WAKELOCK
#ifdef CONFIG_HAS_EARLYSUSPEND
		fbdev[i]->early_suspend.suspend = s3cfb_early_suspend;
		fbdev[i]->early_suspend.resume = s3cfb_late_resume;//resume\D7\EE\CFȵ\F7\D3\C3
		fbdev[i]->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;//max

		register_early_suspend(&fbdev[i]->early_suspend);



//		if(machine_is_cyit_evt()){
//				blank_early_suspend.level=EARLY_SUSPEND_LEVEL_BLANK_SCREEN;//min
//				blank_early_suspend.suspend=s3cfb_blank_screen_early_suspend;
//				blank_early_suspend.resume=s3cfb_blank_screen_late_resume;//resume\D7\EE\BA\F3\B5\F7\D3\C3
//				register_early_suspend(&blank_early_suspend);//û\D3б\BB\B5\F7\D3\C3
//		}
	
#if 0
#if defined(CONFIG_FB_S5P_BTL507212_MIPI_LCD)||defined(CONFIG_FB_S5P_SSD2075_MIPI_LCD)
#else
		blank_early_suspend.level=EARLY_SUSPEND_LEVEL_BLANK_SCREEN;//min
		blank_early_suspend.suspend=s3cfb_blank_screen_early_suspend;
		blank_early_suspend.resume=s3cfb_blank_screen_late_resume;//resume\D7\EE\BA\F3\B5\F7\D3\C3
		register_early_suspend(&blank_early_suspend);//û\D3б\BB\B5\F7\D3\C3
#endif		
#endif

#endif
#endif
//#if ESD_FOR_LCD
//if (ESD_FOR_LCD)
		tmp_fbdev[i] = fbdev[i];
//#endif
	}
#ifdef CONFIG_FB_S5P_LCD_INIT
	/* panel control */
	if (pdata->backlight_on)
		pdata->backlight_on(pdev);

	if (pdata->lcd_on)
		pdata->lcd_on(pdev);
#endif

//add by ericli 2012-11-06
	write_colorgain_reg( );
#ifdef LCD_POWER_CTRL
	//raymanfeng
	//FIXME: not here.
	
	if(!strncmp(s_selected_lcd_name,"ut9gm",strlen("ut9gm")))
	{
		write_power_item_value(POWER_LCD33_EN, 1);
		write_power_item_value(POWER_LVDS_PD, 1);
	
		mdelay(250);
		write_power_item_value(POWER_LCD33_EN, 0); 
		write_power_item_value(POWER_LCD33_EN, 1); 

		udelay(200);
		write_power_item_value(POWER_LCD33_EN, 0); 
		write_power_item_value(POWER_LCD33_EN, 1); 
		
		mdelay(100);
		write_power_item_value(POWER_BL_EN, 1);
	}
	else{
		write_power_item_value(POWER_LCD33_EN, 1);
		write_power_item_value(POWER_LCD18_EN, 1); // enable LCD 1.8v
		msleep(25);
		write_power_item_value(POWER_LVDS_PD, 1);
		if(strncmp(s_selected_lcd_name,"jcm101",strlen("jcm101")))	// ericli add 2013-07-03
		{
			write_power_item_value(POWER_BL_EN, 1);
		}
		}
#endif

//	mdelay(20);
	mdelay(8);
	mdelay(100);//add by jerry for startup 

//add by albert
	s3c_gpio_cfgpin(EXYNOS4_GPD0(1), S3C_GPIO_SFN(2));

	if(!strncmp(s_selected_lcd_name,"jcm101",strlen("jcm101")))	// ericli add 2013-07-03
	{
		mdelay(100);
		write_power_item_value(POWER_BL_EN, 1);
	}
	
	ret = device_create_file(&(pdev->dev), &dev_attr_win_power);
	if (ret < 0)
		dev_err(fbdev[0]->dev, "failed to add sysfs entries\n");

	dev_info(fbdev[0]->dev, "registered successfully\n");
//#if ESD_FOR_LCD
//if (ESD_FOR_LCD)
//	fbdev_register_finished = 1;
//#endif


    //s5p_dsim_frame_done_interrupt_enable(1);


	return 0;
err3:
	for (i = 0; i < FIMD_MAX; i++)
		free_irq(fbdev[i]->irq, fbdev[i]);
err2:
	for (i = 0; i < FIMD_MAX; i++)
		iounmap(fbdev[i]->regs);
err1:
	for (i = 0; i < FIMD_MAX; i++)
		pdata->clk_off(pdev, &fbdev[i]->clock);
err0:
	return ret;
}

static int s3cfb_remove(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);
	struct s3cfb_window *win;
	struct fb_info *fb;
	struct s3cfb_global *fbdev[2];
	int i;
	int j;

	for (i = 0; i < FIMD_MAX; i++) {
		fbdev[i] = fbfimd->fbdev[i];

#ifdef CONFIG_HAS_WAKELOCK
#ifdef CONFIG_HAS_EARLYSUSPEND
		unregister_early_suspend(&fbdev[i]->early_suspend);
#endif
#endif
		free_irq(fbdev[i]->irq, fbdev[i]);
		iounmap(fbdev[i]->regs);
		pdata->clk_off(pdev, &fbdev[i]->clock);

		for (j = 0; j < pdata->nr_wins; j++) {
			fb = fbdev[i]->fb[j];

			/* free if exists */
			if (fb) {
				win = fb->par;
				if (win->id == pdata->default_win)
					s3cfb_unmap_default_video_memory(fbdev[i], fb);
				else
					s3cfb_unmap_video_memory(fbdev[i], fb);

				s3cfb_set_buffer_address(fbdev[i], j);
				framebuffer_release(fb);
			}
		}

		kfree(fbdev[i]->fb);
		kfree(fbdev[i]);
	}
#ifdef CONFIG_EXYNOS_DEV_PD
	/* disable the power domain */
	pm_runtime_put(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
#endif
	return 0;
}

#ifdef CONFIG_PM
#ifdef CONFIG_FB_S5P_AMS369FG06
extern void ams369fg06_ldi_init(void);
extern void ams369fg06_ldi_enable(void);
extern void ams369fg06_ldi_disable(void);
extern void ams369fg06_gpio_cfg(void);
#elif defined(CONFIG_FB_S5P_LMS501KF03)
extern void lms501kf03_ldi_disable(void);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void s3cfb_blank_screen_early_suspend(struct early_suspend *h)
{
printk("\n\n\n %s is called\n\n\n",__func__);
#if defined(CONFIG_FB_S5P_S6E8AA0)
	s6e8aa0_early_suspend();
#else
	otm1280a_early_suspend();

#endif
	//regulator_disable(lcd_regulator);       
	//regulator_disable(lcd_vdd18_regulator);
}
static void s3cfb_blank_screen_late_resume(struct early_suspend *h)
{
    printk("\n\n\n %s is called\n\n\n",__func__);

#if defined(CONFIG_FB_S5P_S6E8AA0)
	s6e8aa0_late_resume();
#else
	otm1280a_late_resume();

#endif
}

extern char g_selected_utmodel[32] ;

extern char g_selected_bltype[32];

void s3cfb_early_suspend(struct early_suspend *h)
{
	struct s3cfb_global *info = container_of(h, struct s3cfb_global, early_suspend);
	struct s3c_platform_fb *pdata = to_fb_plat(info->dev);
	struct platform_device *pdev = to_platform_device(info->dev);
	struct s3cfb_global *fbdev[2];
	int i;

	printk("s3cfb_early_suspend is called\n");

    #ifdef CONFIG_TOUCHSCREEN_GOODIX
	//if((lcd_id ==LCD_BOE) || (lcd_id ==LCD_LG))
	    led_put_out();
    #endif

    #if 0    
    #ifdef CONFIG_CYIT_DVT
    #if defined(CONFIG_TOUCHSCREEN_GOODIX)
    led_put_out();
    #endif
    #endif
    #endif

	info->system_state = POWER_OFF;
	//s6e8aa0_early_suspend();
	for (i = 0; i < FIMD_MAX; i++) {
		fbdev[i] = fbfimd->fbdev[i];

		if (pdata->backlight_off)
			pdata->backlight_off(pdev);

#ifdef CONFIG_FB_S5P_AMS369FG06
		ams369fg06_ldi_disable();
#elif defined(CONFIG_FB_S5P_LMS501KF03)
		lms501kf03_ldi_disable();
#endif

		s3cfb_display_off(fbdev[i]);
		if (pdata->clk_off)
			pdata->clk_off(pdev, &fbdev[i]->clock);
	}

	//s5p_dsim_early_suspend();
//    regulator_disable(lcd_regulator);       
	//regulator_disable(lcd_vdd18_regulator);

	//info->system_state = POWER_OFF;
#ifdef CONFIG_EXYNOS_DEV_PD
	/* disable the power domain */
	printk(KERN_DEBUG "s3cfb - disable power domain\n");
	pm_runtime_put_sync(&pdev->dev);
#endif

#ifdef LCD_POWER_CTRL
	if(/*0==strncmp( g_selected_utmodel, "s101" , strlen ("s101"))
		||0==strncmp( g_selected_utmodel, "s702" , strlen ("s702"))
		||*/ g_selected_bltype[0] == 'p'
		) {
		gpio_set_value(EXYNOS4_GPD0(1), S3C_GPIO_SETPIN_ONE);
	} else {
		gpio_set_value(EXYNOS4_GPD0(1), S3C_GPIO_SETPIN_ZERO);
	}
	s3c_gpio_cfgpin(EXYNOS4_GPD0(1), S3C_GPIO_OUTPUT);	// albert modify
	
//add by jerry
	if(!strncmp( g_selected_utmodel, "s702" , strlen ("s702")) 
		|| !strncmp( g_selected_utmodel, "s703" , strlen ("s703"))) 
		{
//			gpio_set_value(EXYNOS4_GPD0(1), S3C_GPIO_SETPIN_ONE);
//			lcd_reg_write(0x114000B0, 0x5);  
		}
//end	

	write_power_item_value(POWER_BL_EN, 0);
	udelay(120);

	write_power_item_value(POWER_LCD33_EN, 0);
	if(!CHECK_UTMODEL("d720"))
		write_power_item_value(POWER_LCD18_EN, 0);
	write_power_item_value(POWER_LVDS_PD, 0);
//	udelay(120);
#endif

#if 0 //raymanfeng
  gpio_request(EXYNOS4_GPK1(1), "GPIO_HUB_5V");
  gpio_direction_output(EXYNOS4_GPK1(1), 0);
	s3c_gpio_setpull(EXYNOS4_GPK1(1), S3C_GPIO_PULL_NONE);
	gpio_free(EXYNOS4_GPK1(1));
#endif
	
	return ;
}

void s3cfb_late_resume(struct early_suspend *h)
{
	struct s3cfb_global *info = container_of(h, struct s3cfb_global, early_suspend);
	struct s3c_platform_fb *pdata = to_fb_plat(info->dev);
	struct fb_info *fb;
	struct s3cfb_window *win;
	struct s3cfb_global *fbdev[2];
	int i, j;
	struct platform_device *pdev = to_platform_device(info->dev);

	printk("s3cfb_late_resume is called\n");
	//if((lcd_id ==LCD_BOE) || (lcd_id ==LCD_LG))
	    mdelay(80);//xufei,prevent display abnormally when resume
    //led_light_up();
	//regulator_enable(lcd_regulator);        
	//regulator_enable(lcd_vdd18_regulator);
    //if ((lcd_id ==LCD_BOE)||(lcd_id == LCD_LG))
    mdelay(5);	//xufei,prevent display abnormally when resume
	dev_dbg(info->dev, "wake up from suspend\n");

#ifdef CONFIG_EXYNOS_DEV_PD
	/* enable the power domain */
	printk(KERN_DEBUG "s3cfb - enable power domain\n");
	pm_runtime_get_sync(&pdev->dev);
#endif
	//s5p_dsim_late_resume();
	info->system_state = POWER_ON;

	for (i = 0; i < FIMD_MAX; i++) {
		fbdev[i] = fbfimd->fbdev[i];
		if (pdata->cfg_gpio)
			pdata->cfg_gpio(pdev);

		//if (pdata->backlight_on)
		//	pdata->backlight_on(pdev);

		if (pdata->lcd_on)
			pdata->lcd_on(pdev);

#if defined(CONFIG_FB_S5P_DUMMYLCD)
		max8698_ldo_enable_direct(MAX8698_LDO4);
#endif

		if (info->lcd->init_ldi)
			fbdev[i]->lcd->init_ldi();
		else
			dev_dbg(info->dev, "no init_ldi\n");

		if (pdata->clk_on)
			pdata->clk_on(pdev, &fbdev[i]->clock);

		s3cfb_init_global(fbdev[i]);
		s3cfb_set_clock(fbdev[i]);
		s3cfb_display_on(fbdev[i]);

		for (j = 0; j < pdata->nr_wins; j++) {
			fb = fbdev[i]->fb[j];
			win = fb->par;
			if ((win->path == DATA_PATH_DMA) && (win->enabled)) {
				s3cfb_set_par(fb);
				s3cfb_set_buffer_address(fbdev[i], win->id);
				s3cfb_enable_window(fbdev[i], win->id);
			}
		}

#ifdef CONFIG_FB_S5P_AMS369FG06
		ams369fg06_gpio_cfg();
		ams369fg06_ldi_init();
		ams369fg06_ldi_enable();
#endif

		if (pdata->backlight_on)
			pdata->backlight_on(pdev);
	}
	//s6e8aa0_late_resume();
 
//#ifdef CONFIG_FB_S5P_BTL507212_MIPI_LCD
//	if((lcd_id ==LCD_BOE)||(lcd_id == LCD_LG))
//		info->system_state = POWER_ON;
#if 0
#if defined(CONFIG_FB_S5P_BTL507212_MIPI_LCD)||defined(CONFIG_FB_S5P_SSD2075_MIPI_LCD)
	info->system_state = POWER_ON;
#endif
#endif


#ifdef LCD_POWER_CTRL
	//raymanfeng
	//FIXME: not here.


	if(!strncmp(s_selected_lcd_name,"ut9gm",strlen("ut9gm")))	
	{
		write_power_item_value(POWER_LCD33_EN, 1);
		write_power_item_value(POWER_LVDS_PD, 1);
	
		msleep(190);
		write_power_item_value(POWER_LCD33_EN, 0); 
		write_power_item_value(POWER_LCD33_EN, 1); 

		udelay(200);
		write_power_item_value(POWER_LCD33_EN, 0); 
		write_power_item_value(POWER_LCD33_EN, 1);
		
		msleep(100);

		write_power_item_value(POWER_BL_EN, 1);
	}
	else
	{
		write_power_item_value(POWER_LCD33_EN, 1);
		msleep(25);
		write_power_item_value(POWER_LCD18_EN, 1);
		write_power_item_value(POWER_BL_EN, 1);	
		write_power_item_value(POWER_LVDS_PD, 1);		
	}
	msleep(200);//modify by jerry
	s3c_gpio_cfgpin(EXYNOS4_GPD0(1), S3C_GPIO_SFN(2));
#endif

	if(!strncmp(s_selected_lcd_name,"dpip3",strlen("dpip3")))	
		dp501_train_init();
	

	return;
}
//#if ESD_FOR_LCD
void lcd_te_handle(void)
{
    printk("\n\n\n%s is called\n\n\n",__func__);

    int i;

	for (i = 0; i < FIMD_MAX; i++) {
		s3cfb_early_suspend(&tmp_fbdev[i]->early_suspend);
	}
	for (i = 0; i < FIMD_MAX; i++) {
		s3cfb_late_resume(&tmp_fbdev[i]->early_suspend);
	}
}
//#endif
#else /* else !CONFIG_HAS_EARLYSUSPEND */

int s3cfb_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);
	struct s3cfb_global *fbdev[2];
	int i;

	for (i = 0; i < FIMD_MAX; i++) {
		fbdev[i] = fbfimd->fbdev[i];

		if (atomic_read(&fbdev[i]->enabled_win) > 0) {
			/* lcd_off and backlight_off isn't needed. */
			if (fbdev[i]->lcd->deinit_ldi)
				fbdev[i]->lcd->deinit_ldi();

			s3cfb_display_off(fbdev[i]);
			pdata->clk_off(pdev, &fbdev[i]->clock);
		}

	}

	return 0;
}

int s3cfb_resume(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);
	struct fb_info *fb;
	struct s3cfb_window *win;
	struct s3cfb_global *fbdev[2];
	int i;
	int j;

	for (i = 0; i < FIMD_MAX; i++) {
		fbdev[i] = fbfimd->fbdev[i];
		dev_dbg(fbdev[i]->dev, "wake up from suspend fimd[%d]\n", i);

		if (pdata->cfg_gpio)
			pdata->cfg_gpio(pdev);

		if (pdata->backlight_on)
			pdata->backlight_on(pdev);
		if (pdata->lcd_on)
			pdata->lcd_on(pdev);
		if (fbdev[i]->lcd->init_ldi)
			fbdev[i]->lcd->init_ldi();

		if (pdata->backlight_off)
			pdata->backlight_off(pdev);
		if (pdata->lcd_off)
			pdata->lcd_off(pdev);
		if (fbdev[i]->lcd->deinit_ldi)
			fbdev[i]->lcd->deinit_ldi();

		if (atomic_read(&fbdev[i]->enabled_win) > 0) {
			pdata->clk_on(pdev, &fbdev[i]->clock);
			s3cfb_init_global(fbdev[i]);
			s3cfb_set_clock(fbdev[i]);

			for (j = 0; j < pdata->nr_wins; j++) {
				fb = fbdev[i]->fb[j];
				win = fb->par;
				if (win->owner == DMA_MEM_FIMD) {
					s3cfb_set_win_params(fbdev[i], win->id);
					if (win->enabled) {
						if (win->power_state == FB_BLANK_NORMAL)
							s3cfb_win_map_on(fbdev[i], win->id, 0x0);

						s3cfb_enable_window(fbdev[i], win->id);
					}
				}
			}

			s3cfb_display_on(fbdev[i]);

			if (pdata->backlight_on)
				pdata->backlight_on(pdev);

			if (pdata->lcd_on)
				pdata->lcd_on(pdev);

			if (fbdev[i]->lcd->init_ldi)
				fbdev[i]->lcd->init_ldi();
		}
	}

	return 0;
}
#endif
#else
#define s3cfb_suspend NULL
#define s3cfb_resume NULL
#endif

#ifdef CONFIG_EXYNOS_DEV_PD
static int s3cfb_runtime_suspend(struct device *dev)
{
	return 0;
}

static int s3cfb_runtime_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops s3cfb_pm_ops = {
	.runtime_suspend = s3cfb_runtime_suspend,
	.runtime_resume = s3cfb_runtime_resume,
};
#endif

static struct platform_driver s3cfb_driver = {
	.probe		= s3cfb_probe,
	.remove		= s3cfb_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= s3cfb_suspend,
	.resume		= s3cfb_resume,
#endif
	.driver		= {
		.name	= S3CFB_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_EXYNOS_DEV_PD
		.pm	= &s3cfb_pm_ops,
#endif
	},
};

struct fb_ops s3cfb_ops = {
	.owner		= THIS_MODULE,
	.fb_open	= s3cfb_open,
	.fb_release	= s3cfb_release,
	.fb_check_var	= s3cfb_check_var,
	.fb_set_par	= s3cfb_set_par,
	.fb_setcolreg	= s3cfb_setcolreg,
	.fb_blank	= s3cfb_blank,
	.fb_pan_display	= s3cfb_pan_display,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
	.fb_cursor	= s3cfb_cursor,
	.fb_ioctl	= s3cfb_ioctl,
};

static int s3cfb_register(void)
{
	platform_driver_register(&s3cfb_driver);
	return 0;
}

static void s3cfb_unregister(void)
{
	platform_driver_unregister(&s3cfb_driver);
}

module_init(s3cfb_register);
module_exit(s3cfb_unregister);

MODULE_AUTHOR("Jingoo Han <jg1.han@samsung.com>");
MODULE_AUTHOR("Jonghun, Han <jonghun.han@samsung.com>");
MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_DESCRIPTION("Samsung Display Controller (FIMD) driver");
MODULE_LICENSE("GPL");
