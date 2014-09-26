/*
 *  misc power and gpio proc file for Urbest
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

#include <plat/gpio-cfg.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <mach/regs-pmu.h>


#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/mod_devicetable.h>
#include <linux/log2.h>
#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <linux/of.h>
#include <linux/i2c.h>
#include "qn8027.h"


#ifdef BDG
#define P_GPIO_DEBUG(x...) printk(KERN_DEBUG "[gpio drv]" x)
#define BDG(x...)  	printk(KERN_DEBUG "[gpio drv]  %s(); %d\n", __func__, __LINE__)
#else
#define P_GPIO_DEBUG(x...) do { } while(0)
#define BDG(x...) do { } while(0)
#endif


#define PROC_NAME     "QN8027"
#define R_TXRX_MASK    0x20
UINT8   qnd_Crystal  = QND_CRYSTAL_DEFAULT;
UINT8   qnd_PrevMode;
UINT8   qnd_Country  = COUNTRY_CHINA ;
UINT16  qnd_CH_START = 7600;
UINT16  qnd_CH_STOP  = 10800;
UINT8   qnd_CH_STEP  = 1;

extern char g_selected_fm[];

static struct i2c_client * qn8027_i2c_client = NULL;

UINT8 QND_WriteReg(UINT8 Regis_Addr,UINT8 Data)
{


    return i2c_smbus_write_byte_data(qn8027_i2c_client,Regis_Addr,Data);

    return 1;
}


UINT8 QND_ReadReg(UINT8 Regis_Addr)
{
    UINT8 Data;

    Data = i2c_smbus_read_byte_data(qn8027_i2c_client,Regis_Addr);

    return Data;
}






void QNF_SetRegBit(UINT8 reg, UINT8 bitMask, UINT8 data_val) 
{
        UINT8 temp;
    temp = QND_ReadReg(reg);
    temp &= (UINT8)(~bitMask);
    temp |= data_val & bitMask;
//    temp |= data_val;
    QND_WriteReg(reg, temp);
}


void QN_ChipInitialization()
{
    // reset
    QND_WriteReg(0x00,0x80);
    mdelay(20);
    //to be customized: change the crystal setting, according to HW design
    //.........
    //recalibration
    QNF_SetRegBit(0x00,0x40,0x40);
    QNF_SetRegBit(0x00,0x40,0x00);
	
	QNF_SetRegBit(0x04,0x03,0x03);//б┴ии?1?гд??

	
    mdelay(20);                //delay 20 ms
    QND_WriteReg(0x18,0xe4);        //reset
    QND_WriteReg(0x1b,0xf0);
	
    //Done chip recalibration
    QNF_SetRegBit(0x04,0x80,0x00);//12M
    QNF_SetRegBit(0x02,0x30,0x30);//FM work all
}

void QND_SetSysMode(UINT16 mode) 
{    
    UINT8 val;
    switch(mode)        
    {        
    case QND_MODE_SLEEP:                       //set sleep mode        
        QNF_SetRegBit(SYSTEM1, R_TXRX_MASK, 0); 
        break;        
    default:    
            val = (UINT8)(mode >> 8);        
            if (val)
            {
                val = val >> 1;
                if (val & 0x20)
                {
                    // set to new mode if it's not same as old
                    if((QND_ReadReg(SYSTEM1) & R_TXRX_MASK) != val)
                    {
                        QNF_SetRegBit(SYSTEM1, R_TXRX_MASK, val); 
                    }
                    // make sure it's working on analog output
                    QNF_SetRegBit(SYSTEM1, 0x08, 0x00);    
                }
            }    
        break;        
    }    
}


void QND_TXSetPower( UINT8 gain)
{
    UINT8 value = 0;
    value |= 0x40;  
    value |= gain;
    QND_WriteReg(PAG_CAL, value);
}

UINT16 QNF_GetCh() 
{
    UINT8 tCh;
    UINT8  tStep; 
    UINT16 ch = 0;
    // set to reg: CH_STEP
    tStep = QND_ReadReg(CH_STEP);
    tStep &= CH_CH;
    ch  =  tStep ;
    tCh= QND_ReadReg(CH);    
    ch = (ch<<8)+tCh;
    return CHREG2FREQ(ch);
}


UINT8 QNF_SetCh(UINT16 freq) 
{
    // calculate ch parameter used for register setting
    UINT8 tStep;
    UINT8 tCh;
    UINT16 f; 
        f = FREQ2CHREG(freq); 
        // set to reg: CH
        tCh = (UINT8) f;
        QND_WriteReg(CH, tCh);
        // set to reg: CH_STEP
        tStep = QND_ReadReg(CH_STEP);
        tStep &= ~CH_CH;
        tStep |= ((UINT8) (f >> 8) & CH_CH);
        QND_WriteReg(CH_STEP, tStep);

    return 1;
}

void QND_TuneToCH(UINT16 ch) 
{
    QNF_SetCh(ch);
}


UINT8 QND_Init(void) 
{
    QN_ChipInitialization();
//    QND_WriteReg(0x00,  0x01); //resume original status of chip /* 2008 06 13 */
    return 1;
}

void qn8027_init(void)
{	
	 
	  QND_Init();
	  QND_SetSysMode(QND_MODE_FM | QND_MODE_TX); 
	  QND_TXSetPower(0x05);
	  QND_TuneToCH(8800);	// 88MHz
	
}



static int qn8027_write(struct file *file, const char *buffer,
                           unsigned long count, void *data)
{
	int value;
	value = 0;
	sscanf(buffer, "%d", &value);
	QND_TuneToCH(value);	// 88MHz
	return count;
}

static int qn8027_read(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
	int len;
	int value;
	value = (int)QNF_GetCh();
	len   = sprintf( page, "%d\n", value);
	return len;
}


static int qn8027_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	printk("qn8027_probe\n");
	qn8027_i2c_client = client;

	printk("qn8027_i2c_probe\n");
	printk("qn8027: qn8027_i2c_client->addr = 0x%x\n",qn8027_i2c_client->addr);

	qn8027_init();
	
	return 0;
}

static int qn8027_remove(struct i2c_client *client)
{
	return 0;
}


static const struct i2c_device_id qn8027_id[] = {
	{ "qn8027", 0 },
	{},
	
};
MODULE_DEVICE_TABLE(i2c, qn8027_id);

static struct i2c_driver qn8027_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "qn8027",
	},
	.probe		= qn8027_probe,
	.remove		= qn8027_remove,
	.id_table	= qn8027_id,
};

static int __init FM_proc_init(void)
{
	int i, ret = 0;
	struct proc_dir_entry *entry;
			entry = create_proc_entry(PROC_NAME, 0666, &proc_root);
			if(entry) {
				entry->write_proc = qn8027_write;
				entry->read_proc = qn8027_read;
			
	} else {
		printk(KERN_ERR "power_gpio_init init err .\n");
		ret = -1;
		goto err_out;
	}

	printk(KERN_INFO "power_gpio_init enter .\n");
err_out:
	return ret;
}

static __init int init_qn8027(void)
{
	if(strcmp(g_selected_fm,"qn8027"))
		return 0;
	printk("init_qn8027\n");
	i2c_add_driver(&qn8027_driver);
	FM_proc_init();
	return 0;
}

static __exit void exit_qn8027(void)
{
	if(strcmp(g_selected_fm,"qn8027"))
		return ;
	printk("exit_qn8027\n");
	i2c_del_driver(&qn8027_driver);
	remove_proc_entry(PROC_NAME, &proc_root);
}


//late_initcall(init_qn8027);

module_init(init_qn8027);
module_exit(exit_qn8027);
MODULE_DESCRIPTION("UT QN8027 FM driver");

MODULE_LICENSE("GPL");



