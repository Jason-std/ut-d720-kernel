#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <linux/synclink.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/random.h>
#include <linux/proc_fs.h>
#include "../../fs/proc/internal.h"
#include <mach/regs-clock.h>
#include <asm/io.h>
#include <linux/clk.h>

#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/spinlock.h>

//#define DEBUG 1
#undef DEBUG
#define DEBUG_ALPU05
#ifdef DEBUG_ALPU05
#define dbg(format, arg...) printk(KERN_ALERT format, ## arg)
#else
#define dbg(format, arg...)
#endif


#define GSENSOR_PROC_NAME 		"ls"
static struct proc_dir_entry * s_proc = NULL;
static struct i2c_client * s_i2c_client_alpu05 = NULL;         //alpu05 client
static struct i2c_client * s_i2c_client_alpum = NULL;          //alpu-m client
static int s_alpu05_exist = -1;		// -1: unknown 0:none 1:exist
static int s_alpum_exist = -1;		// -1: unknown 0:none 1:exist

unsigned char _alpu_rand(void)
{
	static unsigned long seed; // 2byte, must be a static variable

	srandom32(jiffies);
	seed = seed + random32();
	seed =  seed * 1103515245 + 12345;

	return (seed/65536) % 32768;
}


#define ERROR_CODE_TRUE				    0
#define ERROR_CODE_FALSE				1
#define ERROR_CODE_ADDSHIFT_WRITE	    21
#define ERROR_CODE_ADDSHIFT_READ		22
#define ERROR_CODE_ADDSHIFT_DATA		23
#define ERROR_CODE_FEEDBACK_WRITE	    31
#define ERROR_CODE_FEEDBACK_READ		32
#define ERROR_CODE_FEEDBACK_DATA		33
#define ERROR_CODE_APROCESS_WRITE	    41	
#define ERROR_CODE_APROCESS_READ		42
#define ERROR_CODE_APROCESS_DATA		43

#define ERROR_CODE_WRITE_ADDR	10
#define ERROR_CODE_WRITE_DATA	20
#define ERROR_CODE_READ_ADDR	30
#define ERROR_CODE_READ_DATA	40
#define ERROR_CODE_START_BIT	50
#define ERROR_CODE_APROCESS		60
#define ERROR_CODE_DENY			70

//************************ codes for alpu05 *************************************

#if 0
extern unsigned char _alpu05_process(void);
extern unsigned char _i2c_Write(unsigned char addr, unsigned char * buf, int count);
extern unsigned char _i2c_Read(unsigned char addr, unsigned char * buf, int count);

unsigned char _alpu_05_algorithm;
unsigned char _alpu_05_data_w[5];
unsigned char _alpu_05_data_r[5];
unsigned char _alpu_05_data_Enc_r[5];
unsigned char _alpu_05_data_e[5]; 

EXPORT_SYMBOL(_alpu_05_algorithm);
EXPORT_SYMBOL(_alpu_05_data_w);
EXPORT_SYMBOL(_alpu_05_data_r);
EXPORT_SYMBOL(_alpu_05_data_Enc_r);
EXPORT_SYMBOL(_alpu_05_data_e);

unsigned char _i2c_Read(unsigned char addr, unsigned char * buf, int count)
{
	int ret;
	if(!s_i2c_client_alpu05)
	{
		printk("s_i2c_client is not ready\n");
		return -1;
	}

	ret = i2c_master_recv(s_i2c_client_alpu05, buf, count);
	if(count == ret)
		return ERROR_CODE_TRUE;
	else
	{
		printk("i2c_master_recv return %d\n", ret);	
		return ERROR_CODE_FALSE;
	}
}

unsigned char _i2c_Write(unsigned char addr, unsigned char * buf, int count)
{
	int ret;
	if(!s_i2c_client_alpu05)
	{
		printk("s_i2c_client is not ready\n");
		return -1;
	}

	ret = i2c_master_send(s_i2c_client_alpu05, buf, count);
	if(count == ret)
		return ERROR_CODE_TRUE;
	else
	{
		printk("i2c_master_send return %d\n", ret);	
		return ERROR_CODE_FALSE;
	}
}

static void alpu05_chip_init(void)
{
#if 0
	unsigned int rate, div, tmp;
	//set it about 8MHz for alpu05.
#if 0	
	struct clk * clock_epll;

	clock_epll = clk_get(NULL, "fout_epll");
	rate = clk_get_rate(clock_epll);
#else
	//FIXME: it reports wrong clock rate. actually about 20MHz.
	rate = 20 * 1000 * 1000;
#endif
	div = rate / (8 * 1000 * 1000);
	if(div > 0x0F)
		div = 0x0F;

	tmp = readl(S5P_CLK_OUT);
	tmp &= ~S5P_CLKOUT_DIV_MASK;
	tmp |= (div << S5P_CLKOUT_DIV_SHIFT) | S5P_CLKOUT_CLKSEL_EPLL;
	writel(tmp, S5P_CLK_OUT);
#endif
}
#endif

#if 1
#define DEVICE_ADDRESS	0x7A
#define ERROR_CODE_DATA_COMPARE 70

unsigned char alpum1_tx_data[8];
unsigned char alpum1_rx_data[10];
unsigned char alpum1_ex_data[8];
void _alpum1_bypass(void);
unsigned char _alpum1_process(void);

extern unsigned char _alpu_rand(void);
extern void _alpu_delay_ms(int);
extern unsigned char _art_write(unsigned char, unsigned char, unsigned char *, int);
extern unsigned char _art_read(unsigned char, unsigned char, unsigned char *, int);

#endif

//************************* codes for alpu05 end ***************************


//************************ codes for alpu-m *************************************
extern unsigned char _alpum_process(void);
extern unsigned char _i2c_write(unsigned char device_addr, unsigned char reg, unsigned char * buf, int count);
extern unsigned char _i2c_read(unsigned char device_addr, unsigned char reg, unsigned char * buf, int count);
extern void _alpu_delay_ms(unsigned int );

unsigned char alpum_tx_data[8];
unsigned char alpum_rx_data[10];
unsigned char alpum_ex_data[8];
unsigned char buffer_data[8];

EXPORT_SYMBOL(alpum_tx_data);
EXPORT_SYMBOL(alpum_rx_data);
EXPORT_SYMBOL(alpum_ex_data);
EXPORT_SYMBOL(buffer_data);

#if 1
extern unsigned char _alpum_process(void);
#else

#define DEVICE_ADDRESS	0x7a

void _alpum_bypass(void)
{
	int i;

	for ( i=0; i<8; i++ ) alpum_ex_data[i] = alpum_tx_data[i] ^ 0x01 ;
}

unsigned char _alpum_process(void)
{
	unsigned char error_code;
	int i;

    //Read Protocol Test(No Stop Condition)
	error_code=_i2c_read(DEVICE_ADDRESS, 0x75, buffer_data, 8);
  if (error_code) return error_code;
  for (i=0; i<8; i++)
	  printk("rx%d=0x%02x\n", i, buffer_data[i]);

    //Seed Generate
	for (i=0; i<8; i++) alpum_tx_data[i] = _alpu_rand();

    //Writh Seed Data to ALPU-M
    //sub_address=0x80(Bypass Mode Set)
	error_code = _i2c_write(DEVICE_ADDRESS, 0x80, alpum_tx_data, 8);
	if (error_code) return error_code;

    //Read Result Data from ALPU-M
	error_code = _i2c_read(DEVICE_ADDRESS, 0x80, alpum_rx_data, 10);
	if (error_code) return error_code;

    //XOR operation
	_alpum_bypass();

	for (i=0; i<8; i++)
		printk("tx%d=0x%02x ex%d=0x%02x rx%d=0x%02x\n", i, alpum_tx_data[i], i, alpum_ex_data[i], i, alpum_rx_data[i]);

	//Compare the encoded data and received data
	for (i=0; i<8; i++) {
		if (alpum_ex_data[i] != alpum_rx_data[i]) return 60;
	}
	return 0; //If Bypass Mode is Success, return value is "0".
}
#endif

void _alpu_delay_ms(unsigned int ms)
{
	mdelay(ms);
}

unsigned char _i2c_read(unsigned char device_addr, unsigned char reg, unsigned char * data, int count)
{
	//FIXME: should check (device_addr == s_i2c_client_alpum->addr);
	int ret;
	unsigned char buf[2];

	//printk("%s, reg=0x%02x, count=%d, addr=%d\n", __FUNCTION__, reg, count, s_i2c_client_alpum->addr);

	if(!s_i2c_client_alpum)
	{
		printk("s_i2c_client is not ready\n");
		return -1;
	}

	buf[0] = reg;
	struct i2c_msg msgs[] = {
		{
			.addr	= s_i2c_client_alpum->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= buf,
		},
		{
			.addr	= s_i2c_client_alpum->addr,
			.flags	= I2C_M_RD,
			.len	= count,
			.buf	= data,
		},
	};

	//msleep(1);
	ret = i2c_transfer(s_i2c_client_alpum->adapter, msgs, 2); 
	if (ret < 0)
	{	
		printk("msg %s i2c read error: %d\n", __func__, ret);
		return ERROR_CODE_FALSE;
	}

	return ERROR_CODE_TRUE;
}

unsigned char _i2c_write(unsigned char device_addr, unsigned char reg, unsigned char * data, int count)
{
	//FIXME: should check (device_addr == s_i2c_client_alpum->addr);
	int ret;
	unsigned char buf[256];	//we only provide 256 bytes;
	//printk("%s, reg=0x%02x, count=%d\n", __FUNCTION__, reg, count);

	if(!s_i2c_client_alpum)
	{
		printk("s_i2c_client is not ready\n");
		return -1;
	}

	if(count > sizeof(buf) - 1)
	{
		printk("%s warning: data length is too big, can not transfer.\n",  __FUNCTION__);
		count = sizeof(buf) - 1;
	}

	buf[0] = reg;
	memcpy(buf + 1, data, count);

	struct i2c_msg msg[] = {
		{
			.addr	= s_i2c_client_alpum->addr,
			.flags	= 0,
			.len	= count + 1,
			.buf	= buf,
		},
	};

	//msleep(1);
 	ret = i2c_transfer(s_i2c_client_alpum->adapter, msg, 1);
	if (ret < 0)
	{	
		printk("msg %s i2c read error: %d\n", __func__, ret);
		return ERROR_CODE_FALSE;
	}

	return ERROR_CODE_TRUE;
}

static void alpum_chip_init(void)
{
	//no init.
}
//************************* codes for alpu-m end ***************************






static int __devinit alpu05_i2c_probe(struct i2c_client * client, const struct i2c_device_id * id)
{
	int code;
	printk("************************************* id->name = %s\n", id->name);
	if(!strncmp(id->name, "gsensor2", 8))
	{
		s_i2c_client_alpum = client;

		alpum_chip_init();

		code = _alpum_process();
		printk("gsensor_i2c_probe %s: ", id->name);
		if(code != ERROR_CODE_TRUE)
		{	
			printk("Error, code = %d\n", code);
			s_alpum_exist = 0;
		}
		else
		{	
			printk("OK\n");
			s_alpum_exist = 1;
		}
	}
	else if(!strncmp(id->name, "gsensor", 7))
	{
 
		s_i2c_client_alpu05 = client;

		alpu05_chip_init();

		code = _alpu05_process();
		printk("gsensor_i2c_probe %s: ", id->name);
		if(code != ERROR_CODE_TRUE)
		{	
			printk("Error\n");
			s_alpu05_exist = 0;
		}
		else
		{	
			printk("OK\n");
			s_alpu05_exist = 1;
		}
	}

	if(s_alpu05_exist == 0 && s_alpum_exist == 0)
	{
		panic("error.");
	}
	return 0;
}

static int alpu05_i2c_suspend(struct i2c_client * client, pm_message_t mesg)
{
	return 0;
}

static int alpu05_i2c_resume(struct i2c_client * client)
{
	alpu05_chip_init();
	return 0;
}

static struct i2c_device_id alpu05_idtable[] = {
	//{"alpu05", 0},
	{"gsensor", 0},
	{"gsensor2", 1},
};

static struct i2c_driver alpu05_i2c_driver = 
{
	.driver = 
	{
		.name = "gsensor_i2c",
		.owner = THIS_MODULE,
	},
	.id_table = alpu05_idtable,
	.probe = alpu05_i2c_probe,
	.suspend = alpu05_i2c_suspend,
	.resume = alpu05_i2c_resume,
};

static struct platform_device alpu05_platform_i2c_device = 
{
    .name           = "gsensor_i2c",
    .id             = -1,
};

static int alpu05_writeproc(struct file *file,const char *buffer,
                           unsigned long count, void *data)
{
	return count;
}

static void walkersunEncMsg(unsigned char* pMsg, int msgLen, unsigned char *result);
static int alpu05_readproc(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
	int value, len = 0;
	if(off <= 256 * 1024 * 1024)
	{
		msleep(20);
//		value = _alpu05_process();
		len = sprintf(page, "gsensor=%d\n", value);
/*		if(value != 0)
		{
			panic("application proc error");
		}*/
	}
	else
	{
		int i;
		char num[32];
		char result[32];
		char outStr[128];
		memset(num, 0, sizeof(num));
		memset(result, 0, sizeof(result));
		memset(outStr, 0, sizeof(outStr));
		sprintf(num, "%ld", off);
		walkersunEncMsg(num, 32, result);
		//printk("alpu05_readproc num=%s\n", num);
		for(i = 0; i < sizeof(result); i++)
        {
            char tmp[4];
            sprintf(tmp, "%02X", result[i]);
			strcat(outStr, tmp);
        }

		//printk("alpu05:%s\n", outStr);
		len = sprintf(page, outStr);
		*start = page;
	}
	*eof = 1;
	//printk("count = %d, len=%d\n", count, len);
	return len;
}

static int __init alpu05_init(void)
{
    int ret;
	printk(KERN_INFO "G-Sensor driver: init\n");

	ret = platform_device_register(&alpu05_platform_i2c_device);
    if(ret) {
		printk(KERN_INFO "G-Sensor driver: can not register platform device\n");
        return ret;
	}

	s_proc = create_proc_entry(GSENSOR_PROC_NAME, 0666, &proc_root);
	if (s_proc != NULL)
	{
		s_proc->write_proc = alpu05_writeproc;
		s_proc->read_proc = alpu05_readproc;
		s_proc->size = ULONG_MAX;
	}

	return i2c_add_driver(&alpu05_i2c_driver); 
}

static void __exit alpu05_exit(void)
{
	if (s_proc != NULL)
		remove_proc_entry(GSENSOR_PROC_NAME, &proc_root);
	i2c_del_driver(&alpu05_i2c_driver);
    platform_device_unregister(&alpu05_platform_i2c_device);
}

//module_init(alpu05_init);
late_initcall(alpu05_init);
module_exit(alpu05_exit);
MODULE_AUTHOR("Urbest inc.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Urbest gsensor");

#if 1/*add by albert lee*/
#define DEVICE_NAME    "encrypt_check"  /* cat /proc/devices */
#define ENC_DEV_MAX 210
#define ENC_DEV_MIN 0

#define BUFF_LEN  32

struct encrypt_dev {   
  unsigned char rer[BUFF_LEN];   
  //unsigned int buffersize;
  //unsigned int len;
  //unsigned int start;
  struct semaphore sem;
  struct cdev cdev;
};   
   
struct encrypt_dev encrypt_device;   

static void walkersunEncMsg(unsigned char* pMsg, int msgLen, unsigned char *result)	// walkersun
{
	unsigned char* pKey;
	int keyLen;
	int i, j;
 
	pKey = "1960b33098979b7e674c3b10b50d9a7c"; //get key
	keyLen = strlen((const char*)pKey);
	
	if(!keyLen) {
		return;
	}
	
	memcpy(result, pMsg, msgLen);
		
	j=keyLen;
	for(i=msgLen; i>0; i--) {
		result[i-1] ^= pKey[j-1];
		j--;
		if(!j) {
			j = keyLen;
		}
	}
}

static int check_encrypt_open(struct inode *inode, struct file *filp)
{   
	struct encrypt_dev *mydev;   

	mydev = container_of(inode->i_cdev,struct encrypt_dev,cdev);   
	filp->private_data = mydev;   

	if (down_interruptible(&mydev->sem))   
		return -ERESTARTSYS;   
	up(&mydev->sem);   

	return 0;   
}

//read
ssize_t check_encrypt_read(struct file *filp, char __user *buf, size_t count/*BUFF_LEN*/, loff_t *ppos)   
{     
	int i;    
	struct encrypt_dev *mydev;    
	mydev = filp->private_data;   

	if (down_interruptible(&mydev->sem))   
		return -ERESTARTSYS;   

	for(i=0; i<BUFF_LEN/2; i++) {
		mydev->rer[i] = _alpu_rand();
	}
	walkersunEncMsg(mydev->rer, BUFF_LEN/2, &mydev->rer[BUFF_LEN/2] );

	if(copy_to_user(buf, mydev->rer, BUFF_LEN)) {   
		printk("copy error...\n");   
	}   

#ifdef DEBUG	
	else {
		printk("copy sucess => ");      
		for(i=0; i<BUFF_LEN; i++) {
			printk(" %02x", mydev->rer[i]);
		}
		printk("\n");
	}  
#endif

	up(&mydev->sem);   
	return 0;   
}  

static int check_encrypt_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations check_encrypt_fops = {
                                 .owner = THIS_MODULE,
				     .read = check_encrypt_read, 
                                 .open = check_encrypt_open,
                                 .release = check_encrypt_release,
                              };
#if 0

static struct miscdevice check_encrypt_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &check_encrypt_fops,
};

static int __init check_encrypt_init(void)
{
    int ret;
	printk(KERN_INFO "check_encrypt_initr init ...\n");

#if 0
	s_proc = create_proc_entry(DEVICE_NAME, 0666, &proc_root);
	if (s_proc != NULL)
	{
		//s_proc->write_proc = mma7660_writeproc;
		//s_proc->read_proc = mma7660_readproc;
	}
#endif

	ret = misc_register(&check_encrypt_device);
	if (ret) {
		printk("check_encrypt_init register failed ...\n");
		return ret;
	}

	return 0;
}

#else
//struct cdev check_encrypt_cdev;
static int __init check_encrypt_init(void)
{
	int err; 

	int devno = MKDEV(ENC_DEV_MAX, ENC_DEV_MIN);
	cdev_init(&encrypt_device.cdev, &check_encrypt_fops);
	encrypt_device.cdev.ops = &check_encrypt_fops;
	encrypt_device.cdev.owner = THIS_MODULE;
	err = cdev_add (&encrypt_device.cdev, devno, 1);

	if(err<0) {
		printk(DEVICE_NAME" can't register check encrypt device !\n");
		return err;
	} else {
		// 初始化互斥信号量   
#define init_MUTEX(sem)		sema_init(sem, 1)
		 
		init_MUTEX(&encrypt_device.sem);   

		printk(DEVICE_NAME" Check encrypt device registed successed!!\n");
		return 0; /* succeed */  
	}
}
#endif

/* when use: rmmod led.ko*/
static void __exit check_encrypt_exit(void)
{
#if 1
	cdev_del(&encrypt_device.cdev);
#else
	misc_deregister(&check_encrypt_device);
#endif
	printk(KERN_ALERT " Check encrypt device cdev is deleted!!!\n"); 
}

module_init(check_encrypt_init);
module_exit(check_encrypt_exit);
//MODULE_AUTHOR("Urbest inc. Albert Lee");
//MODULE_LICENSE("GPL");
//MODULE_DESCRIPTION("check the alpu05");
//mknod /dev/test c major minor
#endif


