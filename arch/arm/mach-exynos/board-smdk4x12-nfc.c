/******************************************************************************
 * board-smdk5250-nfc.c - 
 * 
 * Copyright 2010-2018 Urbetter Co.,Ltd.
 * 
 * DESCRIPTION: - 
 * 
 * modification history
 * --------------------
 * v1.0   2013/05/14, Albert_lee create this file
 * 
 ******************************************************************************/

#include <linux/gpio.h>
#include <linux/i2c.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>
#include <plat/iic.h>
#include <plat/devs.h>
//#include <asm/system_info.h>
#include <linux/i2c-gpio.h>

#if defined(CONFIG_BCM2079X_I2C)
#include <linux/nfc/bcm2079x.h>
#endif

/* GPIO_LEVEL_NONE = 2, GPIO_LEVEL_LOW = 0 */
#if defined(CONFIG_BCM2079X_I2C)
static unsigned int nfc_gpio_table[][4] = {
	{GPIO_NFC_INT, S3C_GPIO_INPUT, 2, S3C_GPIO_PULL_DOWN},
	{GPIO_NFC_PW_EN, S3C_GPIO_OUTPUT, 0, S3C_GPIO_PULL_DOWN},
	{GPIO_AP_WAKE_NFC, S3C_GPIO_OUTPUT, 1, S3C_GPIO_PULL_NONE},
};
#endif

static inline void nfc_setup_gpio(void)
{
#if defined(CONFIG_BCM2079X_I2C)

	int err = 0;
	int array_size = ARRAY_SIZE(nfc_gpio_table);
	u32 i, gpio;
	for (i = 0; i < array_size; i++) {
		gpio = nfc_gpio_table[i][0];

		err = s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(nfc_gpio_table[i][1]));
		if (err < 0)
			pr_err("%s, s3c_gpio_cfgpin gpio(%d) fail(err = %d)\n",
				__func__, i, err);

		err = s3c_gpio_setpull(gpio, nfc_gpio_table[i][3]);
		if (err < 0)
			pr_err("%s, s3c_gpio_setpull gpio(%d) fail(err = %d)\n",
				__func__, i, err);

		if (nfc_gpio_table[i][2] != 2)
			gpio_set_value(gpio, nfc_gpio_table[i][2]);
	}
#endif	
}
    
#if defined(CONFIG_BCM2079X_I2C)
static struct bcm2079x_platform_data bcm2079x_i2c_pdata = {
    .irq_gpio = GPIO_NFC_INT,
    .en_gpio = GPIO_NFC_PW_EN,
    .wake_gpio = GPIO_AP_WAKE_NFC ,
};
#endif

static struct i2c_board_info i2c_dev_nfc[] __initdata = {
        {
                I2C_BOARD_INFO("s5p_ddc", (0x74 >> 1)),
        },

#if defined(CONFIG_BCM2079X_I2C)
	{
		I2C_BOARD_INFO("bcm2079x-i2c", 0x77),
		.irq = IRQ_NFC_INT,
		.platform_data = &bcm2079x_i2c_pdata,
	},
#endif
};

static struct i2c_board_info i2c_dev_nfc6441[] __initdata = {
        {
                I2C_BOARD_INFO("s5p_ddc", (0x74 >> 1)),
        },

#if defined(CONFIG_BCM2079X_I2C)
	{
		I2C_BOARD_INFO("bcm2079x-i2c", 0x76),
		.irq = IRQ_NFC_INT,
		.platform_data = &bcm2079x_i2c_pdata,
	},
#endif
};

/* I2C - GPIO */
//#define GPIO_NFC_SCL EXYNOS4_GPA0(7)
//#define GPIO_NFC_SDA EXYNOS4_GPA0(6)
//static struct i2c_gpio_platform_data i2c_nfc_platdata = {
//	.sda_pin = GPIO_NFC_SDA,	/* GPIO_NFC_SDA */
//	.scl_pin = GPIO_NFC_SCL,	/* GPIO_NFC_SCL */
//	.udelay = 2,
//	.sda_is_open_drain = 0,
//	.scl_is_open_drain = 0,
//	.scl_is_output_only = 0,
//};
//
//static struct platform_device s3c_device_i2c_nfc = {
//	.name = "i2c-gpio",
//	.id = 11,
//	.dev.platform_data = &i2c_nfc_platdata,
//};


extern char g_selected_nfc[];
void __init exynos_ut4412_nfc_init(void)
{
	printk("%s(); + \n", __func__);
	if ( !(strncmp (g_selected_nfc, "bcm2079", strlen ("bcm2079"))==0
			|| strncmp (g_selected_nfc, "gb8649", strlen ("gb8649"))==0
			|| strncmp (g_selected_nfc, "gb86441", strlen ("gb86441"))==0
		) )  {
		printk("%s(); not nfc - \n", __func__);
		return ;
	}
	nfc_setup_gpio();

	s3c_i2c2_set_platdata(NULL);
	if ( strncmp (g_selected_nfc, "gb86441", strlen ("gb86441"))==0) {
		i2c_register_board_info(2, i2c_dev_nfc6441, ARRAY_SIZE(i2c_dev_nfc6441));
	} else {
		i2c_register_board_info(2, i2c_dev_nfc, ARRAY_SIZE(i2c_dev_nfc));
	}
//	platform_device_register(&s3c_device_i2c_nfc);
//	platform_device_register(&s3c_device_i2c2);
	
	printk("%s(); - \n", __func__);

	
}

