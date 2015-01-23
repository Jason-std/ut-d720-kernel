#ifndef _POWER_GPIO_COMMON_H_
#define  _POWER_GPIO_COMMON_H_

#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/sizes.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>

#include "../../../fs/proc/internal.h"

#include <plat/gpio-cfg.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <mach/regs-pmu.h>

#include "urbetter/power_gpio.h"
#include <linux/regulator/consumer.h>

#include <urbetter/check.h>

int power_spk( struct power_gpio_node node , int on);
int power_cam_dldo2_2v8(struct power_gpio_node node , int on);
int power_cam_eldo3_af(struct power_gpio_node node , int on);
int gsm_power_write(struct power_gpio_node node,int on);
int gsm_power_read(struct power_gpio_node node);
void power_gpio_on_boot_common(const struct power_gpio_oem * p);

#endif
