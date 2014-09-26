/*
 * File:        drivers/char/ut_bt_dev.c
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
#include <linux/device.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <linux/platform_device.h>

/*
#include <asm/mach-types.h>
#include <plat/gpio-cfg.h>
#include <mach/gpio-bank.h>
#include <mach/regs-gpio.h>
#include <mach/gpio-smdkv210.h>
#include <mach/ut210_gpio_reg.h>
*/
#include <plat/gpio-cfg.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>

#include "power_gpio.h"

#include <linux/proc_fs.h>
#define BT_PROC_RESUME_NAME "bt_resume"
#define BT_PROC_NAME "bluetooth_ut"


#define BT_DEV_ON 1
#define BT_DEV_OFF 0

#define BT_DEV_MAJOR_NUM 234
#define BT_DEV_MINOR_NUM 1

#define IOCTL_BT_DEV_POWER _IO(BT_DEV_MAJOR_NUM, 100)
#define IOCTL_BT_DEV_SPECIFIC _IO(BT_DEV_MAJOR_NUM, 101)
#define IOCTL_BT_DEV_CTRL	_IO(BT_DEV_MAJOR_NUM, 102)

#define CSR_MODULE                  0x12
#define BRCM_MODULE                 0x34
#define RDA_MODULE                  0x56

#define BT_LDO_OFF	0x2
#define BT_LDO_ON	0x3

#define BT_DEBUG_F
//#undef BT_DEBUG_F

#ifdef BT_DEBUG_F
#define BT_DEBUG(fmt,args...)  printk(KERN_DEBUG"[ ut_bt_dev ]: " fmt, ## args) 
#else
#define BT_DEBUG(fmt,args...)		do{ }while(0)
#endif

#define DEV_NAME "ut_bt_dev"

#if 0//
//debug version
#define BT_LDO_PIN S5PV210_GPB(2) //S5PV210_GPC1(2)
//#define BT_LDO_PIN 		S5PV210_GPC1(2)
#define BT_POWER_PIN S5PV210_GPB(2) //S5PV210_GPC1(1)
#else
//release version
//#define BT_LDO_PIN EXYNOS4_GPL0(3)
//#define BT_POWER_PIN EXYNOS4212_GPM3(1)
#endif


static struct class *bt_dev_class;

typedef struct {
	int module;  // 0x12:CSR, 0x34:Broadcom, 0x56 RDA58XX
	int resume_flg;
	int TMP2;
} ut_bt_info_t;


static int s32_resume_flg = 0;
static int s32_ldo_flg = 0;

//GPB2

static void ut_bt_ldo_power_ctr(int onoff)
{
	BT_DEBUG("%s(); onoff=%d\n", __func__, onoff);


	gpio_request(BT_POWER_PIN, "BT_POWER_PIN");
	gpio_request(BT_LDO_PIN, "BT_LDO_PIN");
	
	if(onoff){
		gpio_direction_output(BT_LDO_PIN, 1);
		msleep(5);
		gpio_direction_output(BT_LDO_PIN, 1);
		msleep(50);
		s32_ldo_flg = BT_LDO_ON;
	} else {
		gpio_direction_output(BT_LDO_PIN, 0);
		msleep(5);
		gpio_direction_output(BT_LDO_PIN, 0);
		msleep(5);
		s32_ldo_flg = BT_LDO_OFF;
	}

	gpio_free(BT_POWER_PIN);
	gpio_free(BT_LDO_PIN);
}

static int ut_bt_dev_open(struct inode *inode, struct file *file)
{
	BT_DEBUG("%s();\n", __func__);
	return 0;
}

static int ut_bt_dev_release(struct inode *inode, struct file *file)
{
	BT_DEBUG("%s();\n", __func__);
	return 0;
}

int ut_bt_power33_control(int on_off)
{
	BT_DEBUG("##%s(%d); ==============power_33===\n", __func__, on_off);

	gpio_request(BT_POWER_PIN, "BT_POWER_PIN");
	gpio_request(BT_LDO_PIN, "BT_LDO_PIN");

#if 1	


	if(on_off == BT_DEV_ON) {

	
		//write_power_item_value(POWER_BLUETOOTH, 1);
		gpio_direction_output(BT_POWER_PIN, 1);
		msleep(10);
		/*
		gpio_direction_output(BT_POWER_PIN, 0);
		//write_power_item_value(POWER_BLUETOOTH, 0);
		msleep(200);
		*/
		gpio_direction_output(BT_POWER_PIN, 1);
		//write_power_item_value(POWER_BLUETOOTH, 1);
		msleep(100);
		
	} else if(on_off==BT_DEV_OFF) {
//		gpio_direction_output(BT_POWER_PIN, 1);
		//write_power_item_value(POWER_BLUETOOTH, 0);
//		msleep(200);
	}
#endif

	gpio_free(BT_POWER_PIN);
	gpio_free(BT_LDO_PIN);

	return 0;
}


static int ut_bt_get_info(ut_bt_info_t* arg)
{
	ut_bt_info_t *info_t;
	int module_t;

	ut_bt_info_t a;
	int ret=0;
      info_t=&a;
	
	//info_t = (ut_bt_info_t *)arg;
	ret=copy_from_user(info_t, (ut_bt_info_t *)arg, sizeof(ut_bt_info_t));
	module_t = 0;
	
	module_t = RDA_MODULE;

	info_t->module = module_t;
	info_t->resume_flg = s32_resume_flg;
	info_t->TMP2 = 0;
	BT_DEBUG(" module[%d, %d, ]\n", module_t, s32_resume_flg);
	
	ret=copy_to_user((ut_bt_info_t *)arg, info_t, sizeof(ut_bt_info_t));

//	s32_resume_flg = 0;
	return 0;
}


long ut_bt_dev_ioctl(struct file *file,
               unsigned  int cmd,unsigned long arg)
{
	int *parm1;

	//memset(&parm1, 0, sizeof(int));
	BT_DEBUG(" ut_bt_dev_ioctl cmd[%d] arg[%d]\n", cmd, *(int *)( arg));	

	switch(cmd)
	{
		case IOCTL_BT_DEV_POWER:	//power		NU
			parm1 = (int*)arg;
			BT_DEBUG(" IOCTL_BT_DEV_POWER    cmd[%d] parm1[%d]\n", cmd, *parm1);
			ut_bt_power33_control(*parm1);
			break;

		case IOCTL_BT_DEV_SPECIFIC:
			BT_DEBUG(" IOCTL_BT_DEV_SPECIFIC    cmd[%d]\n", cmd);
			ut_bt_get_info((ut_bt_info_t*)arg);
			break;

		case IOCTL_BT_DEV_CTRL://ldo
    			parm1 = (int*)arg;
			BT_DEBUG(" IOCTL_BT_DEV_CTRL    cmd[%d] parm1[%d]\n", cmd, *parm1);
			ut_bt_ldo_power_ctr(*parm1);
			break;

		default :
			BT_DEBUG(KERN_WARNING" default  [## BT ##] ut_bt_dev_ioctl cmd[%d]\n", cmd);
			break;
	}

	return 0;
}

struct file_operations ut_bt_dev_ops = {
    .owner      = THIS_MODULE,
    .unlocked_ioctl      = ut_bt_dev_ioctl,
    .open       = ut_bt_dev_open,
    .release    = ut_bt_dev_release,
};


static struct platform_device ut_bt_device = {
	.name		= "bt_sleep",
};

static struct platform_device *devices[] __initdata = {
	&ut_bt_device,
};


static int bt_sleep_probe(struct platform_device *pdev)
{
	BT_DEBUG("%s(); name =%s\n", __func__, pdev->name);
	
	return 0;
}

static int bt_sleep_remove(struct platform_device *pdev)
{
	BT_DEBUG("%s();\n", __func__);
	return 0;
}

static int bt_suspend(struct platform_device *dev, pm_message_t state)
{
	BT_DEBUG("%s()\n", __func__);
	gpio_direction_output(BT_LDO_PIN, 1);
	return 0;
}

static int bt_resume(struct platform_device *dev)
{
	BT_DEBUG("%s()\n", __func__);
	gpio_direction_output(BT_LDO_PIN, 1);
	s32_resume_flg = s32_ldo_flg;
	return 0;
}

static struct platform_driver bt_sleep_driver = {
	.probe = bt_sleep_probe,
	.remove = bt_sleep_remove,
	.driver = {
		.name = "bt_sleep",
		.owner = THIS_MODULE,
	},
	.suspend = bt_suspend,
	.resume = bt_resume,
};

static int bt_switch_writeproc(struct file *file,const char *buffer,
                           unsigned long count, void *data)
{
    int value; 
    value = 0; 
    sscanf(buffer, "%d", &value);

    s32_resume_flg = value;
    
    return count;
}

static int bt_switch_readproc(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
    int len;
    len = sprintf(page, "%d\n", (s32_resume_flg==BT_LDO_ON) ? 1 : 0);

    if (off + count >= len)
        *eof = 1;	
    
    if (len < off)
        return 0;
    
    *start = page + off;

    s32_resume_flg = 0;
	
    return ((count < len - off) ? count : len - off);
}

static int __init bt_init_module(void)
{
	int ret;
	extern struct proc_dir_entry proc_root;
	extern char g_selected_bt[];
	
	struct proc_dir_entry *root_entry;
	struct proc_dir_entry * s_proc = NULL; 
	
	BT_DEBUG("[## BT ##] init_module\n");

	if(strncmp(g_selected_bt, "rda", strlen("rda"))) {
		printk(KERN_INFO"INFO : is no rda module !");
		return -1;
	}

	root_entry = proc_mkdir(BT_PROC_NAME, &proc_root);
	s_proc = create_proc_entry(BT_PROC_RESUME_NAME, 0666, root_entry);
	if (s_proc != NULL){
	    s_proc->write_proc = bt_switch_writeproc;
	    s_proc->read_proc = bt_switch_readproc;
	}
	
	ret = register_chrdev(BT_DEV_MAJOR_NUM, DEV_NAME, &ut_bt_dev_ops);

	bt_dev_class = class_create(THIS_MODULE, DEV_NAME);
	device_create(bt_dev_class, NULL, MKDEV(BT_DEV_MAJOR_NUM, BT_DEV_MINOR_NUM), NULL, DEV_NAME);

	gpio_request(BT_POWER_PIN, "BT_POWER_PIN");
	gpio_request(BT_LDO_PIN, "BT_LDO_PIN");
	gpio_direction_output(BT_POWER_PIN, 1);
	gpio_direction_output(BT_LDO_PIN, 0);

	gpio_free(BT_POWER_PIN);
	gpio_free(BT_LDO_PIN);
	
	if(ret < 0){
		printk(KERN_ERR"[## BT ##] [%d]fail to register the character device\n", ret);
		return ret;
	}

	platform_add_devices(devices, ARRAY_SIZE(devices));

	ret = platform_driver_register(&bt_sleep_driver);
	if(ret < 0) {
		return ret;
	}

	printk(KERN_DEBUG"[## BT ##] init_module OK \n");
	
	return ret;
}

static void __exit bt_cleanup_module(void)
{
    printk("[## BT ##] cleanup_module\n");
    unregister_chrdev(BT_DEV_MAJOR_NUM, DEV_NAME);
    platform_driver_unregister(&bt_sleep_driver);
}

//module_init(bt_init_module);
late_initcall(bt_init_module);
module_exit(bt_cleanup_module);

MODULE_AUTHOR("samsung Inc. linux@samsung.com");
MODULE_DESCRIPTION("Urbest bluetooth driver");
MODULE_LICENSE("GPL");

