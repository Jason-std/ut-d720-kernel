/*
 * linux/drivers/video/backlight/oled_bl.c
 *
 * simple PWM based backlight control, board code has to setup
 * 1) pin configuration so PWM waveforms can output
 * 2) platform_data being correctly configured
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/err.h>
//#include <linux/oled.h>
#include <linux/oled_backlight.h>
#include <linux/slab.h>

struct oled_bl_data {
	struct device		*dev;
	//int			(*notify)(struct device *,
	//				  int brightness);
	//int			(*check_fb)(struct device *, struct fb_info *);
};
extern int s6e8ax0_update_brightness(struct backlight_device *bd);

static int oled_backlight_update_status(struct backlight_device *bl)
{
	//struct oled_bl_data *pb = dev_get_drvdata(&bl->dev);
	
	//printk("brightness = %d\n",brightness);
	if (bl->props.brightness > bl->props.max_brightness){
		printk("%s :exceed max brightness! brightness = %d\n",__func__,bl->props.brightness);
		return 0;
	}
	
	s6e8ax0_update_brightness(bl);

	#if 0
	if (bl->props.power != FB_BLANK_UNBLANK)
		brightness = 0;

	if (bl->props.fb_blank != FB_BLANK_UNBLANK)
		brightness = 0;
	#endif
	return 0;
}

static int oled_backlight_get_brightness(struct backlight_device *bl)
{
	return bl->props.brightness;
}

static const struct backlight_ops oled_backlight_ops = {
	.update_status	= oled_backlight_update_status,
	.get_brightness	= oled_backlight_get_brightness,
};

static int oled_backlight_probe(struct platform_device *pdev)
{
	struct backlight_properties props;
	struct platform_oled_backlight_data *data = pdev->dev.platform_data;
	struct backlight_device *bl;
	struct oled_bl_data *pb;
	int ret;

	if (!data) {
		dev_err(&pdev->dev, "failed to find platform data\n");
		return -EINVAL;
	}

	if (data->init) {
		ret = data->init(&pdev->dev);
		if (ret < 0)
			return ret;
	}

	pb = kzalloc(sizeof(*pb), GFP_KERNEL);
	if (!pb) {
		dev_err(&pdev->dev, "no memory for state\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	pb->dev = &pdev->dev;
	
	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;
	props.max_brightness = data->max_brightness;
	bl = backlight_device_register(dev_name(&pdev->dev), &pdev->dev, pb,
				       &oled_backlight_ops, &props);
	if (IS_ERR(bl)) {
		dev_err(&pdev->dev, "failed to register backlight\n");
		ret = PTR_ERR(bl);
		goto err_bl;
	}

	bl->props.brightness = data->dft_brightness;
	backlight_update_status(bl);

	platform_set_drvdata(pdev, bl);
	return 0;

err_bl:
	kfree(pb);
err_alloc:
	if (data->exit)
		data->exit(&pdev->dev);
	return ret;
}

static int oled_backlight_remove(struct platform_device *pdev)
{
	struct platform_oled_backlight_data *data = pdev->dev.platform_data;
	struct backlight_device *bl = platform_get_drvdata(pdev);
	struct oled_bl_data *pb = dev_get_drvdata(&bl->dev);

	backlight_device_unregister(bl);

	kfree(pb);
	if (data->exit)
		data->exit(&pdev->dev);
	return 0;
}

#ifdef CONFIG_PM
static int oled_backlight_suspend(struct platform_device *pdev,
				 pm_message_t state)
{
	//struct backlight_device *bl = platform_get_drvdata(pdev);
	//struct oled_bl_data *pb = dev_get_drvdata(&bl->dev);

	return 0;
}

static int oled_backlight_resume(struct platform_device *pdev)
{
//	struct backlight_device *bl = platform_get_drvdata(pdev);

//	backlight_update_status(bl);

	return 0;
}
#else
#define oled_backlight_suspend	NULL
#define oled_backlight_resume	NULL
#endif

static struct platform_driver oled_backlight_driver = {
	.driver		= {
		.name	= "pwm-backlight",
		.owner	= THIS_MODULE,
	},
	.probe		= oled_backlight_probe,
	.remove		= oled_backlight_remove,
	.suspend	= oled_backlight_suspend,
	.resume		= oled_backlight_resume,
};

static int __init oled_backlight_init(void)
{
	return platform_driver_register(&oled_backlight_driver);
}
module_init(oled_backlight_init);

static void __exit oled_backlight_exit(void)
{
	platform_driver_unregister(&oled_backlight_driver);
}
module_exit(oled_backlight_exit);

MODULE_DESCRIPTION("PWM based Backlight Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:pwm-backlight");
