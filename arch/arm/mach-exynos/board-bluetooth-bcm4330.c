/*
 * Bluetooth Broadcom GPIO and Low Power Mode control
 *
 *  Copyright (C) 2011 Samsung Electronics Co., Ltd.
 *  Copyright (C) 2011 Google, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <linux/serial_core.h>
#include <linux/wakelock.h>

#include <asm/mach-types.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <mach/irqs.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>

#define BT_UART_CFG
#ifdef CONFIG_BT_BRCM_LPM
#undef BT_LPM_ENABLE
#else
#define BT_LPM_ENABLE
#endif

extern char g_selected_utmodel[32];
/*
#define GPIO_BT_EN	BT_LDO_PIN
#define GPIO_BT_nRST	BT_LDO_PIN
*/

static unsigned int GPIO_BT_EN=BT_LDO_PIN;
//static unsigned int GPIO_BT_nRST=BT_LDO_PIN;

#define GPIO_BT_WAKE BT_HOST_WAKE

#define GPIO_BT_HOST_WAKE  GPIO_BT_INT
#define IRQ_BT_HOST_WAKE IRQ_BT_INT

static struct rfkill *bt_rfkill;
static DEFINE_MUTEX(ut_bt_wlan_sync);

struct bcm_bt_lpm {
	int wake;
	int host_wake;

	struct hrtimer enter_lpm_timer;
	ktime_t enter_lpm_delay;

	struct uart_port *uport;

	struct wake_lock wake_lock;
	struct wake_lock host_wake_lock;
} bt_lpm;

int bt_is_running;
EXPORT_SYMBOL(bt_is_running);

#ifdef BT_UART_CFG

extern int s3c_gpio_slp_cfgpin(unsigned int pin, unsigned int config);
extern int s3c_gpio_slp_setpull_updown(unsigned int pin, unsigned int config);

static unsigned int bt_uart_on_table[][4] = {
	{EXYNOS4_GPA0(0), S3C_GPIO_SFN(2), S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_UP},
	{EXYNOS4_GPA0(1), S3C_GPIO_SFN(2), S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_UP},
	{EXYNOS4_GPA0(2), S3C_GPIO_SFN(2), S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_NONE},
	{EXYNOS4_GPA0(3), S3C_GPIO_SFN(2), S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_NONE},
};

void bt_config_gpio_table(int array_size, unsigned int (*gpio_table)[4])
{
	u32 i, gpio;

	for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
		s3c_gpio_setpull(gpio, gpio_table[i][3]);
		if (gpio_table[i][2] != S3C_GPIO_SETPIN_NONE)
			gpio_set_value(gpio, gpio_table[i][2]);
	}
}

void bt_uart_rts_ctrl(int flag)
{
	if (!gpio_get_value(GPIO_BT_EN))
		return;
	if (flag) {
		/* BT RTS Set to HIGH */
		s3c_gpio_cfgpin(EXYNOS4_GPA0(3), S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(EXYNOS4_GPA0(3), S3C_GPIO_PULL_NONE);
		gpio_set_value(EXYNOS4_GPA0(3), 1);
		s3c_gpio_slp_cfgpin(EXYNOS4_GPA0(3), S3C_GPIO_SLP_OUT0);
		s3c_gpio_slp_setpull_updown(EXYNOS4_GPA0(3), S3C_GPIO_PULL_NONE);
	} else {
		/* BT RTS Set to LOW */
		s3c_gpio_cfgpin(EXYNOS4_GPA0(3), S3C_GPIO_OUTPUT);
		gpio_set_value(EXYNOS4_GPA0(3), 0);
		s3c_gpio_cfgpin(EXYNOS4_GPA0(3), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(EXYNOS4_GPA0(3), S3C_GPIO_PULL_NONE);
	}
}
#else
void bt_uart_rts_ctrl(int flag)
{

}
#endif
EXPORT_SYMBOL(bt_uart_rts_ctrl);

void bt_wlan_lock(void)
{
	mutex_lock(&ut_bt_wlan_sync);
}

void bt_wlan_unlock(void)
{
	mutex_unlock(&ut_bt_wlan_sync);
}

static int bcm4330_bt_rfkill_set_power(void *data, bool blocked)
{

	printk("%s(); +%d\n", __func__, blocked);
	/* rfkill_ops callback. Turn transmitter on when blocked is false */
	bt_wlan_lock();
	msleep(300);
	if (!blocked) {
		pr_info("[BT] Bluetooth Power On.\n");
#ifdef BT_UART_CFG
		bt_config_gpio_table(ARRAY_SIZE(bt_uart_on_table),
					bt_uart_on_table);
#endif
/*add for ap6210*/
		usleep_range(10, 10);
		s3c_gpio_cfgpin(EXYNOS4_GPA0(3), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(EXYNOS4_GPA0(3), S3C_GPIO_PULL_NONE);
		s5p_gpio_set_drvstr(EXYNOS4_GPA0(3), S5P_GPIO_DRVSTR_LV3);
/*add for ap6210 end*/
		gpio_direction_output(GPIO_BT_EN, 1);
		gpio_set_value(GPIO_BT_EN, 1);

		s3c_gpio_setpull(GPIO_BT_HOST_WAKE, S3C_GPIO_PULL_NONE);
	} else {
		pr_info("[BT] Bluetooth Power Off.\n");

		bt_is_running = 0;

//		gpio_set_value(GPIO_BT_nRST, 0);
		gpio_set_value(GPIO_BT_EN, 0);
		s3c_gpio_setpull(GPIO_BT_HOST_WAKE, S3C_GPIO_PULL_DOWN);

		wake_unlock(&bt_lpm.host_wake_lock);
	}

	msleep(50);
	bt_wlan_unlock();
	printk("%s(); -\n", __func__);
	return 0;
}

static const struct rfkill_ops bcm4330_bt_rfkill_ops = {
	.set_block = bcm4330_bt_rfkill_set_power,
};

#ifdef BT_LPM_ENABLE
static void set_wake_locked(int wake)
{
	if (wake == bt_lpm.wake)
		return;


	bt_lpm.wake = wake;

	if (wake) {
		wake_lock(&bt_lpm.wake_lock);
		gpio_set_value(GPIO_BT_WAKE, wake);
	} else {
		gpio_set_value(GPIO_BT_WAKE, wake);
		wake_unlock(&bt_lpm.wake_lock);
	}
}

static enum hrtimer_restart enter_lpm(struct hrtimer *timer)
{
	unsigned long flags;
	spin_lock_irqsave(&bt_lpm.uport->lock, flags);
	bt_is_running = 0;
	set_wake_locked(0);
	spin_unlock_irqrestore(&bt_lpm.uport->lock, flags);

	return HRTIMER_NORESTART;
}

void bcm_bt_lpm_exit_lpm_locked(struct uart_port *uport)
{
	bt_lpm.uport = uport;

	hrtimer_try_to_cancel(&bt_lpm.enter_lpm_timer);
	bt_is_running = 1;
	set_wake_locked(1);

	hrtimer_start(&bt_lpm.enter_lpm_timer, bt_lpm.enter_lpm_delay,
		HRTIMER_MODE_REL);
}

static void update_host_wake_locked(int host_wake)
{
	if (host_wake == bt_lpm.host_wake)
		return;

	bt_lpm.host_wake = host_wake;

	bt_is_running = 1;
	if (host_wake) {
		wake_lock(&bt_lpm.host_wake_lock);
	} else  {
		/* Take a timed wakelock, so that upper layers can take it.
		* The chipset deasserts the hostwake lock, when there is no
		* more data to send.
		*/
		wake_lock_timeout(&bt_lpm.host_wake_lock, HZ/2);
	}
}

static irqreturn_t host_wake_isr(int irq, void *dev)
{
	int host_wake;
	unsigned long flags;

	host_wake = gpio_get_value(GPIO_BT_HOST_WAKE);
	irq_set_irq_type(irq, host_wake ? IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH);

	if (!bt_lpm.uport) {
		bt_lpm.host_wake = host_wake;
		return IRQ_HANDLED;
	}

	if(gpio_get_value(GPIO_BT_EN))
		return IRQ_HANDLED;

	spin_lock_irqsave(&bt_lpm.uport->lock, flags);
	update_host_wake_locked(host_wake);
	spin_unlock_irqrestore(&bt_lpm.uport->lock, flags);

	return IRQ_HANDLED;
}

static int bcm_bt_lpm_init(struct platform_device *pdev)
{
	int irq;
	int ret;

	printk("%s(); +\n", __func__);
	hrtimer_init(&bt_lpm.enter_lpm_timer, CLOCK_MONOTONIC,
			HRTIMER_MODE_REL);
	bt_lpm.enter_lpm_delay = ktime_set(3, 0);
	bt_lpm.enter_lpm_timer.function = enter_lpm;

	bt_lpm.host_wake = 0;
	bt_is_running = 0;

	wake_lock_init(&bt_lpm.wake_lock, WAKE_LOCK_SUSPEND,
			 "BTWakeLowPower");
	wake_lock_init(&bt_lpm.host_wake_lock, WAKE_LOCK_SUSPEND,
			 "BTHostWakeLowPower");

	irq = IRQ_BT_HOST_WAKE;
	ret = request_threaded_irq(irq, NULL, host_wake_isr,
		IRQF_TRIGGER_HIGH | IRQF_ONESHOT, "bt_host_wake", NULL);
	if (ret) {
		pr_err("[BT] Request host_wake irq failed.\n");
		goto err_lpm_init;
	}

	ret = irq_set_irq_wake(irq, 1);
	if (ret) {
		pr_err("[BT] Set_irq_wake failed.\n");
		free_irq(irq, NULL);
		goto err_lpm_init;
	}

	printk("%s(); -\n", __func__);
	return 0;

err_lpm_init:
	wake_lock_destroy(&bt_lpm.wake_lock);
	wake_lock_destroy(&bt_lpm.host_wake_lock);
	return ret;
}
#else
void bcm_bt_lpm_exit_lpm_locked(struct uart_port *uport)
{
}
#endif



static int bcm4330_bluetooth_probe(struct platform_device *pdev)
{
	int rc = -1;

	rc = gpio_request(GPIO_BT_EN, "bcm4330_bten_gpio");
	if (unlikely(rc)) {
		pr_err("[BT] GPIO_BT_EN request failed.\n");
		return rc;
	}
#ifndef CONFIG_BT_BRCM_LPM
//	rc = gpio_request(GPIO_BT_nRST, "bcm4330_btnrst_gpio");
//	if (unlikely(rc)) {
//		pr_err("[BT] GPIO_BT_nRST request failed.\n");
//		goto err_gpio_nrst;
//	}

	rc = gpio_request(GPIO_BT_WAKE, "bcm4330_btwake_gpio");
	if (unlikely(rc)) {
		pr_err("[BT] GPIO_BT_WAKE request failed.\n");
		goto err_gpio_bt_wake;
	}

	rc = gpio_request(GPIO_BT_HOST_WAKE, "bcm4330_bthostwake_gpio");
	if (unlikely(rc)) {
		pr_err("[BT] GPIO_BT_HOST_WAKE request failed.\n");
		goto err_gpio_bt_host_wake;
	}

	gpio_direction_input(GPIO_BT_HOST_WAKE);
	gpio_direction_output(GPIO_BT_WAKE, 0);
//	gpio_direction_output(GPIO_BT_nRST, 0);
#endif
	gpio_direction_output(GPIO_BT_EN, 0);

	bt_rfkill = rfkill_alloc("bcm4330 Bluetooth", &pdev->dev,
				RFKILL_TYPE_BLUETOOTH, &bcm4330_bt_rfkill_ops,
				NULL);

	if (unlikely(!bt_rfkill)) {
		pr_err("[BT] bt_rfkill alloc failed.\n");
		rc =  -ENOMEM;
		goto err_rfkill_alloc;
	}

	rfkill_init_sw_state(bt_rfkill, 0);

	rc = rfkill_register(bt_rfkill);

	if (unlikely(rc)) {
		pr_err("[BT] bt_rfkill register failed.\n");
		rc = -1;
		goto err_rfkill_register;
	}

	rfkill_set_sw_state(bt_rfkill, true);

#ifdef BT_LPM_ENABLE
	rc = bcm_bt_lpm_init(pdev);
	if (rc)
		goto err_lpm_init;
#endif
	return rc;

#ifdef BT_LPM_ENABLE
err_lpm_init:
	rfkill_unregister(bt_rfkill);
#endif
err_rfkill_register:
	rfkill_destroy(bt_rfkill);
err_rfkill_alloc:
#ifndef CONFIG_BT_BRCM_LPM
	gpio_free(GPIO_BT_HOST_WAKE);
err_gpio_bt_host_wake:
	gpio_free(GPIO_BT_WAKE);
#endif
err_gpio_bt_wake:
//	gpio_free(GPIO_BT_nRST);
//err_gpio_nrst:
	gpio_free(GPIO_BT_EN);

	return rc;
}

static int bcm4330_bluetooth_remove(struct platform_device *pdev)
{
#ifdef BT_LPM_ENABLE
	int irq;

	irq = gpio_to_irq(GPIO_BT_HOST_WAKE);
	irq_set_irq_wake(irq, 0);
	free_irq(irq, NULL);
	set_wake_locked(0);
	hrtimer_try_to_cancel(&bt_lpm.enter_lpm_timer);
	wake_lock_destroy(&bt_lpm.wake_lock);
	wake_lock_destroy(&bt_lpm.host_wake_lock);
#endif

	rfkill_unregister(bt_rfkill);
	rfkill_destroy(bt_rfkill);

#ifndef CONFIG_BT_BRCM_LPM
	gpio_free(GPIO_BT_HOST_WAKE);
	gpio_free(GPIO_BT_WAKE);
#endif
	gpio_free(GPIO_BT_EN);
//	gpio_free(GPIO_BT_nRST);


	return 0;
}

static struct platform_driver bcm4330_bluetooth_platform_driver = {
	.probe = bcm4330_bluetooth_probe,
	.remove = bcm4330_bluetooth_remove,
	.driver = {
		   .name = "bcm4330_bluetooth",
		   .owner = THIS_MODULE,
		   },
};


//extern int Exynos4x12_rtc_clk_sw(int on);

extern bool is_bcm_combo(void);
static int __init bcm4330_bluetooth_init(void)
{
	int ret = -1;

	if((strstr(g_selected_utmodel,"s502"))
	||(strstr(g_selected_utmodel,"d10"))
	||(strstr(g_selected_utmodel,"d521"))
	||(strstr(g_selected_utmodel,"d721"))
	||(strstr(g_selected_utmodel,"d722"))
	||(strstr(g_selected_utmodel,"d720"))
	||(strstr(g_selected_utmodel,"s106"))){
		GPIO_BT_EN=EXYNOS4_GPL1(0);
//		GPIO_BT_nRST=EXYNOS4_GPL1(0);
	}
	if(!is_bcm_combo()) {
		printk(KERN_INFO"%s() , return -1\n", __func__);
		return -1;
	}

	printk(KERN_INFO"%s(); + \n", __func__);
	gpio_direction_output(GPIO_COMBO_LDO, 1);

	ret = platform_driver_register(&bcm4330_bluetooth_platform_driver);
	if(ret<0) {
		return -1;
	}
	return ret;
}

static void __exit bcm4330_bluetooth_exit(void)
{
	platform_driver_unregister(&bcm4330_bluetooth_platform_driver);
}


module_init(bcm4330_bluetooth_init);
module_exit(bcm4330_bluetooth_exit);

MODULE_ALIAS("platform:bcm4330");
MODULE_DESCRIPTION("bcm4330_bluetooth");
MODULE_LICENSE("GPL");
