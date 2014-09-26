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

#include "oem_gpio_cfg.h"
extern char g_selected_utmodel[];

static struct oem_gpio_nodes * pnodes=NULL;
static struct proc_dir_entry *root_entry;
extern struct proc_dir_entry proc_root;	


static int oem_gpio_cfg_writeproc(struct file *file,const char *buffer,
                           unsigned long count, void *data);
static int oem_gpio_cfg_readproc(char *page, char **start, off_t off,
			  int count, int *eof, void *data);

void register_oem_gpio_cfg(struct oem_gpio_nodes * p){
	int i=0;
	struct proc_dir_entry * s_proc = NULL; 

	if(pnodes||(!p)||(!strstr(g_selected_utmodel,p->ut_model))){
		return;
	}
	pnodes=p;
	root_entry = proc_mkdir(p->dir_name, &proc_root);
	for(i=0;i<p->count;i++){
		gpio_request(p->nodes[i].pin,p->nodes[i].name);
		s_proc = create_proc_entry(p->nodes[i].name, 0666, root_entry);
		if (s_proc != NULL){
			if(p->nodes[i].writeproc)
				s_proc->write_proc=p->nodes[i].writeproc;
			else
				s_proc->write_proc = oem_gpio_cfg_writeproc;

			if(p->nodes[i].readproc)
				s_proc->read_proc=p->nodes[i].readproc;
			else
				s_proc->read_proc = oem_gpio_cfg_readproc;

			s_proc->data = (void*)(p->nodes+i);	

			if(p->nodes[i].init_gpio){
				p->nodes[i].init_gpio(p->nodes[i].pin);
			}else{
				gpio_direction_output(p->nodes[i].pin,p->nodes[i].out_value);
				s5p_gpio_set_drvstr(p->nodes[i].pin, S5P_GPIO_DRVSTR_LV4);

			}
		}
	}	
	
}
void unregister_oem_gpio_cfg(struct oem_gpio_nodes * p){
	int i=0;
	if(pnodes==NULL)
		return;
	if(strcmp(g_selected_utmodel,p->ut_model))
		return;
	for(i=0;i<pnodes->count;i++){
		gpio_free(pnodes->nodes[i].pin);
		remove_proc_entry(pnodes->nodes[i].name, root_entry);
	}
	remove_proc_entry(pnodes->dir_name,NULL);
	pnodes=NULL;
}

static int oem_gpio_cfg_writeproc(struct file *file,const char *buffer,
                           unsigned long count, void *data)
{
	int value; 
	struct gpio_node * n=(struct gpio_node *)data;
	unsigned int pin=n->pin;
	value = 0; 
	sscanf(buffer, "%d", &value);
	gpio_direction_output(pin, value);
	gpio_set_value(pin, value);
	s5p_gpio_set_drvstr(pin, S5P_GPIO_DRVSTR_LV4);

	return count;
}

static int oem_gpio_cfg_readproc(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
	int len,value;
	struct gpio_node * n=(struct gpio_node *)data;
	unsigned int pin=n->pin;
	value=gpio_get_value(pin);
	len = sprintf(page, "%d\n", value);

	if (off + count >= len)
	    *eof = 1;	

	if (len < off)
	    return 0;

	*start = page + off;
	return ((count < len - off) ? count : len - off);
}


