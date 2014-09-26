#ifndef _OEM_GPIO_CFG_
#define _OEM_GPIO_CFG_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/switch.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <mach/hardware.h>
#include <asm/io.h>
//#include <mach/io.h>
#include <mach/gpio.h>
#include <plat/adc.h>

#include <linux/delay.h>

#include <linux/io.h>
#include <plat/map-base.h>
#include <linux/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-clock.h>
#include <linux/input.h>
#include <linux/irq.h>


#include <plat/gpio-cfg.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>

#include <linux/proc_fs.h>

#include <linux/gpio.h>
#include <linux/serial_core.h>
#include <plat/devs.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-serial.h>
#include <mach/gpio-ut4x12.h>
#include <plat/cpu.h>


struct gpio_node
{
	char * name;
	unsigned int pin;
	//int polarity;				    //0: low for enable, 1: high for enable	
	int out_value;   // if init_gpio!=NULL , ignore 
	int (*init_gpio)(unsigned int p);		//extra init function
	int (*writeproc)(struct file *file,const char *buffer,unsigned long count, void *data);
	int (*readproc)(char *page, char **start, off_t off,	int count, int *eof, void *data);
};

struct oem_gpio_nodes{
	struct gpio_node * nodes;
	unsigned int count;
	char * ut_model;
	char * dir_name;
};

void register_oem_gpio_cfg(struct oem_gpio_nodes * p);
void unregister_oem_gpio_cfg(struct oem_gpio_nodes * p);


#endif

