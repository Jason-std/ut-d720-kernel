/*
 *  urbest register watcher, read/write physical register for debug.
 *  Copyright (c) 2009 urbest Technologies, Inc.
 */

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
#include "../../fs/proc/internal.h"

#define PROC_NAME		"regwatch"

//map SFR region only
#define REGWATCH_PHY_BASE	 0x10000000
#define REGWATCH_PHY_END	 0x14000000


static unsigned int reg_write(unsigned int reg, unsigned int value)
{
	unsigned int * base;

	if((reg < REGWATCH_PHY_BASE) || (reg > REGWATCH_PHY_END))
	{
		printk("invalid address:0x%08x, please input 0xE0000000~0xFFFFFFFC\n", reg);	
		return -1;
	}

	base = (unsigned int *) ioremap(reg, 0x100);
	if(!base)
	{
		printk("base ioremap failed\n");	
		return 0;
	}
	*base = value;
	iounmap(base);
	return 0;
}

static unsigned int reg_read(unsigned int reg)
{
	unsigned int * base, value;

	if((reg < REGWATCH_PHY_BASE) || (reg > REGWATCH_PHY_END))
	{
		printk("invalid address:0x%08x, please input 0xE0000000~0xFFFFFFFC\n", reg);	
		return 0;
	}

	base = (unsigned int *) ioremap(reg, 0x100);
	if(!base)
	{
		printk("base ioremap failed\n");	
		return 0;
	}
	value = *base;
	iounmap(base);
	return value;	
}

static int regwatch_proc_write(struct file *file, const char *buffer, 
                           unsigned long count, void *data) 
{ 
	unsigned int value, reg; 
	value = 0;
	reg = 0;
	if(sscanf(buffer, "%x=%x", &reg, &value) == 2)
	{
		reg_write(reg, value);
	}
	else if(sscanf(buffer, "%x", &reg) == 1)
	{
		value = reg_read(reg);
	}
	printk("reg=0x%08x, value=0x%08x\n", reg, value);
    return count; 
} 

static int regwatch_proc_read(char *page, char **start, off_t off, 
			  int count, int *eof, void *data) 
{
	int len;
	len = sprintf( page, "usage(hex):\n  write: echo \"E0000000=1234abcd\" > /proc/%s\n  read:  echo \"E0000000\" > /proc/%s\n", PROC_NAME, PROC_NAME);
    return len;
}

static int __init regwatch_init(void)
{
	struct proc_dir_entry *entry;

	entry = create_proc_entry(PROC_NAME, 0666, &proc_root);
	if(entry)
	{
		entry->write_proc = regwatch_proc_write;
		entry->read_proc = regwatch_proc_read;
	}

	printk("urbest regwatch: add regwatch proc file\n");
	return 0;
}

module_init(regwatch_init);

static void __exit regwatch_exit(void)
{
	remove_proc_entry(PROC_NAME, &proc_root);
}

module_exit(regwatch_exit);

MODULE_DESCRIPTION("regwatch proc file");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("urbest, inc.");

