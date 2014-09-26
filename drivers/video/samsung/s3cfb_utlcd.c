#include "linux/delay.h"
#include <linux/platform_device.h>
#include <linux/leds.h>
#include "s3cfb.h"

#include "../../oem_drv/power_gpio.h"


//******************* DP501 I2C *******************************
#include <linux/proc_fs.h>
#include "../../../fs/proc/internal.h"
static struct i2c_client * s_i2c_client_p0 = NULL;
static struct i2c_client * s_i2c_client_p2 = NULL;

static struct proc_dir_entry * s_proc = NULL;

static int dp501_prob_init = 0;

int mali_for_high_flag = 0;
EXPORT_SYMBOL(mali_for_high_flag);


#define DP501_PROC_NAME "dp501"

//******************* DP501 I2C *******************************



#define MAX_LCD_COUNT  64
static struct s3cfb_lcd * s_lcd[MAX_LCD_COUNT];
static int s_lcd_count = 0;
char s_selected_lcd_name[32] = {'\0'};

int colorgain_value = 0x10040100;

EXPORT_SYMBOL(s_selected_lcd_name);


//map SFR region only
#define REGWATCH_PHY_BASE	 0x10000000
#define REGWATCH_PHY_END	 0x1FFFFFFC


extern unsigned int lcd_reg_write(unsigned int reg, unsigned int value);
extern unsigned int lcd_reg_read(unsigned int reg);

//extern int write_power_item_value(int index, int value);

int ut_register_lcd(struct s3cfb_lcd *lcd)
{
	if(!lcd)
		return -1;

	if(s_lcd_count >= MAX_LCD_COUNT - 1){
		printk("ut_register_lcd: can not add lcd: %s, reach max count.\n", lcd->name);
		return -1;
	}

	s_lcd[s_lcd_count] = lcd;
	s_lcd_count++;
	printk("ut_register_lcd: add lcd: %s\n", lcd->name);	
	return 0;
}


extern int step0_up;
extern int step1_down;
extern int step1_up;
extern int step2_down;
#ifdef CONFIG_CPU_EXYNOS4210
#define MALI_DVFS_STEPS 2
#else
#define MALI_DVFS_STEPS 5
#endif


typedef struct mali_dvfs_thresholdTag{
	unsigned int downthreshold;
	unsigned int upthreshold;
}mali_dvfs_threshold_table;

extern mali_dvfs_threshold_table mali_dvfs_threshold[MALI_DVFS_STEPS];



static int __init select_lcd_name(char *str)
{
	printk("select_lcd_name: str=%s\n", str);
	strcpy(s_selected_lcd_name, str);
	

	
	if(!strncmp(s_selected_lcd_name,"dpip3",strlen("dpip3")))
	{
		mali_for_high_flag = 1;
		
		step0_up = 10;
		step1_down = 5;
		step1_up = 10;
		step2_down = 5;


		mali_dvfs_threshold[0].downthreshold = 0;
		mali_dvfs_threshold[0].upthreshold = 20;
#if (MALI_DVFS_STEPS > 1)		
		mali_dvfs_threshold[1].downthreshold = 0;
		mali_dvfs_threshold[1].upthreshold = 20;
#if (MALI_DVFS_STEPS > 2)		
		mali_dvfs_threshold[2].downthreshold = 0;
		mali_dvfs_threshold[2].upthreshold = 20;
#if (MALI_DVFS_STEPS > 3)		
		mali_dvfs_threshold[3].downthreshold = 0;
		mali_dvfs_threshold[3].upthreshold = 90;
#if (MALI_DVFS_STEPS > 4)		
		mali_dvfs_threshold[4].downthreshold = 0;
		mali_dvfs_threshold[4].upthreshold = 100;
#endif
#endif
#endif
#endif
		
	}
	return 0;
}

/* name should be fixed as 's3cfb_set_lcd_info' */
void s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
	int i;
	for(i = 0; i < s_lcd_count; i++)
	{
		if(!strcmp(s_lcd[i]->name, s_selected_lcd_name))
		{
			ctrl->lcd = s_lcd[i];
			return;
		}
	}

	//could not find lcd, use default.
	ctrl->lcd = s_lcd[0];
}


__setup("lcd=", select_lcd_name);


static int __init set_colorgain_reg(char *str)
{
	printk("_+_+_color gain: str=%s\n", str);
	if(*str == '\0')
	{
		printk("no colorgain value_+_+_+\n");
	}
	else
	{
		sscanf(str, "%x", &colorgain_value);
	}
	printk("set color gain = %x _+_+_+\n",colorgain_value);
	return 0;
}

void write_colorgain_reg(void)
{
	int value,reg;

	printk("_+_+_+_+write_colorgain_reg_+_+_+_+\n");	

	if(colorgain_value == 0x10040100)
	{
		printk("the value is no need change\n");
	}
	else
	{
		reg = 0x11c0000c; // color gain control enable reg
		value = lcd_reg_read(reg);
		value |=0x40000;
		lcd_reg_write(reg, value);

		reg = 0x11c001c0; // color gain value reg
		lcd_reg_write(reg, colorgain_value);
	}
}

__setup("lcdRGB=", set_colorgain_reg);



//******************** lcd parameter begin ************************
static void ut_lcd_init(void)
{
	static int s_init = 0;
	printk("LCD ut lcd selecter init.");
	if(!s_init)
	{
		write_power_item_value(POWER_LCD33_EN, 1);
		write_power_item_value(POWER_LVDS_PD, 1);
		s_init = 1;
	}
}

static struct s3cfb_lcd s_ut_lcd_param[] = 
{
	{
			.name = "htk070",
			.init_ldi = ut_lcd_init,
			.width	= 800,
			.height = 600,
			.bpp	= 24,
			.freq	= 60,

			.timing = {
				.h_fp = 40,
				.h_bp = 24,
				.h_sw = 36,
				.v_fp = 5,
				.v_fpe = 1,
				.v_bp = 3,
				.v_bpe = 1,
				.v_sw = 3,
			},

			.polarity = {
				.rise_vclk	= 1,
				.inv_hsync	= 1,
				.inv_vsync	= 1,
				.inv_vden	= 0,
			},
	},

	{
		.name = "wa101",
		.init_ldi = ut_lcd_init,
		.width	= 1024,
		.height	= 768,
		.bpp	= 24,
		.freq	= 60,

		.timing = {
			.h_fp = 260,
			.h_bp = 480,
			.h_sw = 36,
			.v_fp = 16,
			.v_fpe = 1,
			.v_bp = 6,
			.v_bpe = 1,
			.v_sw = 3,
		},

		.polarity = {
			.rise_vclk	= 1,
			.inv_hsync	= 1,
			.inv_vsync	= 1,
			.inv_vden	= 0,
		},
	},
	{
		.name = "clap070",
		.init_ldi = ut_lcd_init,
		.width	= 800,
		.height	= 1280,
		.bpp	= 24,
		.freq	= 60,

		.timing = {
			.h_fp = 40,
			.h_bp = 24,
			.h_sw = 36,
			.v_fp = 5,
			.v_fpe = 1,
			.v_bp = 3,
			.v_bpe = 1,
			.v_sw = 3,
		},

		.polarity = {
			.rise_vclk	= 1,
			.inv_hsync	= 1,
			.inv_vsync	= 1,
			.inv_vden	= 0,
		},
	},	
	{
		.name = "hs101h",
		.init_ldi = ut_lcd_init,
		.width	= 800,
		.height	= 1280,
		.bpp	= 24,
//		.freq	= 60,
//		.freq	= 80,//modify by jerry
//
		.freq	= 70,//mod by eric li 2013-01-21
		.timing = {
/*		
			.h_fp = 18,
			.h_bp = 100,
			.h_sw = 36,
			.v_fp = 6,
			.v_fpe = 1,
			.v_bp = 8,
			.v_bpe = 1,
			.v_sw = 3,
			
//modify by jerry
			.h_fp = 64,
			.h_bp = 10,
			.h_sw = 36,
			.v_fp = 6,
			.v_fpe = 1,
			.v_bp = 8,
			.v_bpe = 1,
			.v_sw = 3,
*/			
	//mod by eric li	2012-12-20
	.h_fp = 18,
	.h_bp = 10,
	.h_sw = 36,
	.v_fp = 5,
	.v_fpe = 1,
	.v_bp = 3,
	.v_bpe = 1,
	.v_sw = 3,

		},

		.polarity = {
			.rise_vclk	= 1,
			.inv_hsync	= 0,
			.inv_vsync	= 0,
			.inv_vden	= 0,
		},
	},	
			
	{ /* add by Albert_lee 2013-02-20 begin */
		.name = "gl05",
		.init_ldi = ut_lcd_init,
		.width	= 800,
		.height	= 480,
		.bpp	= 24,

		.freq	= 70,
		.timing = {

			.h_fp = 18,
			.h_bp = 100,
			.h_sw = 36,
			.v_fp = 6,
			.v_fpe = 1,
			.v_bp = 8,
			.v_bpe = 1,
			.v_sw = 3,

		},

		.polarity = {
			.rise_vclk	= 1,
			.inv_hsync	= 0,
			.inv_vsync	= 0,
			.inv_vden	= 0,
		},
	},
/*Albert_lee 2013-02-20 end*/
			
	{
		.name = "hs101",
		.init_ldi = ut_lcd_init,
		.width	= 1280,
		.height	= 800,
		.bpp	= 24,
		.freq	= 50,	// mod by ericli 2012-12-26

		.timing = {
			.h_fp = 18,
			.h_bp = 100,
			.h_sw = 36,
			.v_fp = 6,
			.v_fpe = 1,
			.v_bp = 8,
			.v_bpe = 1,
			.v_sw = 3,
		},

		.polarity = {
			.rise_vclk	= 1,
			.inv_hsync	= 0,
			.inv_vsync	= 0,
			.inv_vden	= 0,
		},
	},

//ericli add for boe lcd 2013-06-20	start
	{
		.name = "boe101",
		.init_ldi = ut_lcd_init,
		.width	= 1280,
		.height	= 800,
		.bpp	= 24,
		.freq	= 50,	// mod by ericli 2012-12-26

		.timing = {
			.h_fp = 18,
			.h_bp = 100,
			.h_sw = 36,
			.v_fp = 6,
			.v_fpe = 1,
			.v_bp = 8,
			.v_bpe = 1,
			.v_sw = 3,
		},

		.polarity = {
			.rise_vclk	= 1,
			.inv_hsync	= 0,
			.inv_vsync	= 0,
			.inv_vden	= 0,
		},
	},
//ericli add for boe lcd 2013-06-20	end
//ericli add for jcm lcd 2013-07-03	start
	{
		.name = "jcm101",
		.init_ldi = ut_lcd_init,
		.width	= 1280,
		.height	= 800,
		.bpp	= 24,
		.freq	= 50,	// mod by ericli 2012-12-26

		.timing = {
			.h_fp = 18,
			.h_bp = 100,
			.h_sw = 36,
			.v_fp = 6,
			.v_fpe = 1,
			.v_bp = 8,
			.v_bpe = 1,
			.v_sw = 3,
		},

		.polarity = {
			.rise_vclk	= 1,
			.inv_hsync	= 0,
			.inv_vsync	= 0,
			.inv_vden	= 0,
		},
	},
//ericli add for jcm lcd 2013-07-03	end
	{
		.name = "ut9gm",
		.init_ldi = ut_lcd_init,
			.width	= 1024,
			.height	= 768,
			.bpp	= 24,
			.freq	= 50,// mod by ericli 2012-12-26

		.timing = {
			.h_fp = 160,
			.h_bp = 160,
			.h_sw = 36,
			.v_fp = 15,
			.v_fpe = 1,
			.v_bp = 23,
			.v_bpe = 1,
			.v_sw = 3,
		},

		.polarity = {
			.rise_vclk	= 1,
			.inv_hsync	= 1,
			.inv_vsync	= 1,
			.inv_vden	= 0,
		},
	},
	{
		.name = "ut7gm",
		.init_ldi = ut_lcd_init,
		.width = 800,
		.height = 480,
		.bpp = 32/*24*/,
		.freq = 60,

		.timing = {
			.h_fp = 10,
			.h_bp = 78,
			.h_sw = 10,
			.v_fp = 30,
			.v_fpe = 1,
			.v_bp = 30,
			.v_bpe = 1,
			.v_sw = 2,
		},

		.polarity = {
			.rise_vclk = 0,
			.inv_hsync = 1,
			.inv_vsync = 1,
			.inv_vden = 0,
		},
	},
	{
		.name = "ut8gm",
		.init_ldi = ut_lcd_init,
		.width = 800,
		.height = 480,
		.bpp = 32/*24*/,
		.freq = 60,

		.timing = {
			.h_fp = 10,
			.h_bp = 78,
			.h_sw = 10,
			.v_fp = 30,
			.v_fpe = 1,
			.v_bp = 30,
			.v_bpe = 1,
			.v_sw = 2,
		},

		.polarity = {
			.rise_vclk = 0,
			.inv_hsync = 1,
			.inv_vsync = 1,
			.inv_vden = 0,
		},
	},
	{
		.name = "wa101hd",
		.init_ldi = ut_lcd_init,
		.width	= 1280,
		.height	= 800,
		.bpp	= 24,
		.freq	= 60,

		.timing = {
			.h_fp = 260,
			.h_bp = 480,
			.h_sw = 36,
			.v_fp = 16,
			.v_fpe = 1,
			.v_bp = 6,
			.v_bpe = 1,
			.v_sw = 3,
		},

		.polarity = {
			.rise_vclk	= 1,
			.inv_hsync	= 1,
			.inv_vsync	= 1,
			.inv_vden	= 0,
		},
	},
	{
		.name = "hj080",
		.init_ldi = ut_lcd_init,
		.width	= 1024,
		.height	= 768,
		.bpp	= 24,
		.freq	= 60,

		.timing = {
			.h_fp = 24,
			.h_bp = 160,
			.h_sw = 136,
			.v_fp = 16,
			.v_fpe = 1,
			.v_bp = 22,
			.v_bpe = 1,
			.v_sw = 3,
		},

		.polarity = {
			.rise_vclk	= 1,
			.inv_hsync	= 1,
			.inv_vsync	= 1,
			.inv_vden	= 0,
		},
	},	
	{
		.name = "dpip3",
		.init_ldi = NULL,
		.width = 2048,
		.height = 1536,
		.bpp = 16,
		.freq = 50,
			
		.timing = {
			.h_fp = 100,
			.h_bp = 3,
			.h_sw = 3,
			.v_fp = 1,
			.v_fpe = 1,
			.v_bp = 7,
			.v_bpe = 1,
			.v_sw = 1,
		},
		.polarity = {
			.rise_vclk = 1,
			.inv_hsync = 1,
			.inv_vsync = 1,
			.inv_vden = 0,
		},

	},			

};

static int __init regiser_lcd(void)
{
	int i;
	
	for(i = 0; i < sizeof(s_ut_lcd_param) / sizeof(s_ut_lcd_param[0]); i++)
		ut_register_lcd(&(s_ut_lcd_param[i]));
	return 0;
}

early_initcall(regiser_lcd);


unsigned char dp501_i2c_p0_read(unsigned char reg)
{
	if(!s_i2c_client_p0)
	{
		printk("s_i2c_client_p0 is not ready\n");
		return -1;
	}
	return i2c_smbus_read_byte_data(s_i2c_client_p0, reg);
}

int dp501_i2c_p0_write(unsigned char reg, unsigned char data)
{
	if(!s_i2c_client_p0)
	{
		printk("s_i2c_client_p0 is not ready\n");
		return -1;
	}
	return i2c_smbus_write_byte_data(s_i2c_client_p0, reg, data);
}


unsigned char dp501_i2c_p2_read(unsigned char reg)
{
	if(!s_i2c_client_p2)
	{
		printk("s_i2c_client_p2 is not ready\n");
		return -1;
	}
	return i2c_smbus_read_byte_data(s_i2c_client_p2, reg);
}

int dp501_i2c_p2_write(unsigned char reg, unsigned char data)
{
	if(!s_i2c_client_p2)
	{
		printk("s_i2c_client_p2 is not ready\n");
		return -1;
	}
	return i2c_smbus_write_byte_data(s_i2c_client_p2, reg, data);
}



static void dp501_dump(void)
{
	int i;
	printk("***************** dp501 dump page 0***************\n");
	for(i = 0; i <= 0xFF; i++)
	{
		printk("p0reg=0x%02X data=0x%02X\n", i, dp501_i2c_p0_read(i));
	}

	printk("***************** page 2 ***************\n");

	for(i = 0; i <= 0xFF; i++)
	{
		printk("p2reg=0x%02X data=0x%02X\n", i, dp501_i2c_p2_read(i));
	}

	printk("***************** end dump *****************\n");
}

static int dp501_ConnectStatus(void)
{
	dp501_i2c_p0_write(0x8f,0x02); // connection status
	printk("result: p0.0x8D=0x%02X\n", dp501_i2c_p0_read(0x8D));
	if(dp501_i2c_p0_read(0x8D) & 0x04)	//connected
	{
		printk("_+_sink connected!_+_+\n");
		return 1;
	}
	else {
		printk("_+_sink not connected!_+_+\n");
		return 0;
	}
	
}


void dp501_train_init(void)
{
	int i = 0;
	
	dp501_ConnectStatus( );
	dp501_i2c_p2_write(0x24,0x02);
	dp501_i2c_p2_write(0x25,0x04);
	dp501_i2c_p2_write(0x26,0x10); //PIO setting

	//dp501_ConnectStatus( );

	dp501_i2c_p0_write(0x77,0xff); //mask aux irq
	dp501_i2c_p0_write(0x78,0x33); //mask hpd irq 
	
	
	dp501_i2c_p0_write(0x77,0xaf); //enable hpd-assert irq
	
	dp501_i2c_p0_write(0x0a,0x0c); //block 74 & 76
	dp501_i2c_p0_write(0x27,0x30); //auto detect CRTC 
	dp501_i2c_p0_write(0x2f,0x82); //reset tpfifo at v blank 
	dp501_i2c_p0_write(0x24,0x04); //DVO mapping ; crtc follow mode; this register setting related to HW setting!!!
	dp501_i2c_p0_write(0x28,0x00); //crtc follow mode
	dp501_i2c_p0_write(0x87,0x7f); //aux retry
	dp501_i2c_p0_write(0x88,0x1e); //aux retry
	dp501_i2c_p0_write(0xbb,0x06); //aux retry
	dp501_i2c_p0_write(0x72,0xa9); //DPCD readable
	dp501_i2c_p0_write(0x60,0x00); //Scramble on
	dp501_i2c_p0_write(0x8f,0x02); //debug select
	//dp501_ConnectStatus( );
	//second, set up training
	
	dp501_i2c_p0_write(0x5d,0x06); //training link rate(1.62Gbps)
	dp501_i2c_p0_write(0x5e,0x84); //training lane count(4Lanes),
	//dp501_i2c_p0_write(0x5e,0x01); //training lane count(4Lanes),
	dp501_i2c_p0_write(0x74,0x00); //idle pattern
	dp501_i2c_p0_write(0x5f,0x0d); //trigger training
	mdelay(100); //delay 100ms
	
	//then, check training result
	//printk("result: p0.0x63=0x%02X\n", dp501_i2c_p0_read(0x63)); 
	//printk("result: p0.0x64=0x%02X\n", dp501_i2c_p0_read(0x64)); 
	while(i++ < 4)
	{
		
		if(dp501_i2c_p0_read(0x63) == 0x77 && dp501_i2c_p0_read(0x64) == 0x77)
		{
			printk("training ok !_+_+");
			break;
		}
		else
		{
			if(i == 4)
			{
				printk("training fail !_+_+");	
			}
			dp501_i2c_p0_write(0x5f,0x0d); //trigger training
			mdelay(100); //delay 100ms
		}
	}

	//printk("result: p2.0x24=0x%02X\n", dp501_i2c_p2_read(0x24)); 
	//printk("result: p2.0x25=0x%02X\n", dp501_i2c_p2_read(0x25)); 
	//printk("result: p2.0x26=0x%02X\n", dp501_i2c_p2_read(0x26)); 
	//dp501_i2c_p0_read(0x64); //Each 4bits stand for one lane, 0x77/0x77 means training succeed with 4Lanes.
}

static int dp501_writeproc(struct file *file,const char *buffer,
                           unsigned long count, void *data)
{
	//dp501_train_init();
	int  cmd1,cmd2 = 0;
	
	sscanf(buffer, "%d%x", &cmd1,&cmd2);

	if(cmd1 == 0)	// 
	{
		dp501_i2c_p0_write(0x8f,0x02); // connection status
		printk("cmd0\n");
		printk("result: p0.0x63=0x%02X\n", dp501_i2c_p0_read(0x63)); 
		printk("result: p0.0x64=0x%02X\n", dp501_i2c_p0_read(0x64)); 
		printk("result: p0.0x5f=0x%02X\n", dp501_i2c_p0_read(0x5f)); 
		printk("result: p0.0x15=0x%02X\n", dp501_i2c_p0_read(0x15)); 
		printk("result: p0.0x14=0x%02X\n", dp501_i2c_p0_read(0x14)); 
		printk("result: p0.0x1B=0x%02X\n", dp501_i2c_p0_read(0x1B)); 
		printk("result: p0.0x1A=0x%02X\n", dp501_i2c_p0_read(0x1A));		
		printk("result: p0.0x8F=0x%02X\n", dp501_i2c_p0_read(0x8F)); 
		printk("result: p0.0x8D=0x%02X\n", dp501_i2c_p0_read(0x8D));			
		
		printk("result: p2.0x24=0x%02X\n", dp501_i2c_p2_read(0x24)); 
		printk("result: p2.0x25=0x%02X\n", dp501_i2c_p2_read(0x25)); 
		printk("result: p2.0x26=0x%02X\n", dp501_i2c_p2_read(0x26)); 

	}	
	
	if(cmd1 == 1)	// 
	{
		dp501_i2c_p0_write(0x5f,0x0d); //trigger training
		printk("cmd1\n");
		printk("result: p0.0x63=0x%02X\n", dp501_i2c_p0_read(0x63)); 
		printk("result: p0.0x64=0x%02X\n", dp501_i2c_p0_read(0x64)); 
		printk("result: p0.0x5f=0x%02X\n", dp501_i2c_p0_read(0x5f)); 
		printk("result: p0.0x15=0x%02X\n", dp501_i2c_p0_read(0x15)); 
		printk("result: p0.0x14=0x%02X\n", dp501_i2c_p0_read(0x14)); 
		printk("result: p0.0x1B=0x%02X\n", dp501_i2c_p0_read(0x1B)); 
		printk("result: p0.0x1A=0x%02X\n", dp501_i2c_p0_read(0x1A));		
		
		printk("result: p2.0x24=0x%02X\n", dp501_i2c_p2_read(0x24)); 
		printk("result: p2.0x25=0x%02X\n", dp501_i2c_p2_read(0x25)); 
		printk("result: p2.0x26=0x%02X\n", dp501_i2c_p2_read(0x26)); 

	}

	if(cmd1 == 2)
	{
		//dp501_i2c_p0_write(0x24,0xC3); //self-test mode
		
		//cmd2 |= 0xc0;
		//dp501_i2c_p0_write(0x27,cmd2);		
		dp501_i2c_p0_write(0x24,cmd2); //self-test mode
		
		printk("result: p0.0x63=0x%02X\n", dp501_i2c_p0_read(0x63)); 
		printk("result: p0.0x64=0x%02X\n", dp501_i2c_p0_read(0x64)); 
		printk("result: p0.0x24=0x%02X\n", dp501_i2c_p0_read(0x24)); 
		printk("result: p0.0x27=0x%02X\n", dp501_i2c_p0_read(0x27));
		
		printk("result: p2.0x24=0x%02X\n", dp501_i2c_p2_read(0x24)); 
		printk("result: p2.0x25=0x%02X\n", dp501_i2c_p2_read(0x25)); 
		printk("result: p2.0x26=0x%02X\n", dp501_i2c_p2_read(0x26)); 

	}

	if(cmd1 == 3)
	{
		dp501_i2c_p0_write(0x24,0x03); //self-test mode

		cmd2 |= 0x30;
		dp501_i2c_p0_write(0x27,cmd2);
		
		printk("result: p0.0x63=0x%02X\n", dp501_i2c_p0_read(0x63)); 
		printk("result: p0.0x64=0x%02X\n", dp501_i2c_p0_read(0x64)); 
		printk("result: p0.0x24=0x%02X\n", dp501_i2c_p0_read(0x24)); 
		printk("result: p0.0x27=0x%02X\n", dp501_i2c_p0_read(0x27)); 
		
		printk("result: p2.0x24=0x%02X\n", dp501_i2c_p2_read(0x24)); 
		printk("result: p2.0x25=0x%02X\n", dp501_i2c_p2_read(0x25)); 
		printk("result: p2.0x26=0x%02X\n", dp501_i2c_p2_read(0x26)); 

	}

	return count;
}

static int dp501_readproc(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
	int len = 0;
/*
	len = sprintf(page, "x=%d\ny=%d\nz=%d\nx_degree=%d\ny_degree=%d\nz_degree=%d\n", 
				  xyz_g_table[x] * dp501_DIR_X, 
				  xyz_g_table[y] * dp501_DIR_Y, 
				  xyz_g_table[z] * dp501_DIR_Z,
				  xyz_degree_table[x][0] * dp501_DIR_X,
				  xyz_degree_table[y][0] * dp501_DIR_Y,
 				  xyz_degree_table[z][0] * dp501_DIR_Z);
*/
	dp501_dump();
	return len;
}

static int __devinit dp501_i2c_probe(struct i2c_client * client, const struct i2c_device_id * id)
{
	int i = -1;
	printk("dp501_i2c_probe\n");
	if(client->addr == (0x10 >> 1))
	{
		s_i2c_client_p0 = client;
		printk("dp501: s_i2c_client_p0->addr = 0x%x\n",s_i2c_client_p0->addr);
	}
	else if(client->addr == (0x14 >> 1))
	{
		s_i2c_client_p2 = client;
		printk("dp501: s_i2c_client_p2->addr = 0x%x\n",s_i2c_client_p2->addr);
	}
	

	

	if(dp501_prob_init)
	{
		printk("dp501: lane map = 0x%02X\n", dp501_i2c_p0_read(0xac));
		
		dp501_train_init();
		//dp501_dump();
		s_proc = create_proc_entry(DP501_PROC_NAME, 0666, &proc_root);
		if (s_proc != NULL)
		{
			s_proc->write_proc = dp501_writeproc;
			s_proc->read_proc = dp501_readproc;
		}
	}
	
	dp501_prob_init ++;
	return 0;
}

static struct i2c_device_id dp501_idtable[] = {
	{"dp501_p0", 0},
	{"dp501_p2", 0},	
};

static struct i2c_driver dp501_i2c_driver = 
{
	.driver = 
	{
		.name = "dp501_i2c",
		.owner = THIS_MODULE,
	},
	.id_table = dp501_idtable,
	.probe = dp501_i2c_probe,
#if 0  //raymanfeng: moved to early suspend
	.suspend = dp501_i2c_suspend,
	.resume = dp501_i2c_resume,
#endif
};

static struct platform_device dp501_platform_i2c_device = 
{
    .name           = "dp501_i2c",
    .id             = -1,
};

static int __init dp501_init(void)
{
    int ret;
	if(strncmp(s_selected_lcd_name,"dpip3",strlen("dpip3")))
		return -1;
	printk(KERN_INFO "dp501 G-Sensor driver: init\n");

	ret = platform_device_register(&dp501_platform_i2c_device);
    if(ret)
        return ret;

	return i2c_add_driver(&dp501_i2c_driver); 
}

static void __exit dp501_exit(void)
{
	if(strncmp(s_selected_lcd_name,"dpip3",strlen("dpip3")))
		return;
	
	if (s_proc != NULL)
		remove_proc_entry(DP501_PROC_NAME, &proc_root);
	i2c_del_driver(&dp501_i2c_driver);
    platform_device_unregister(&dp501_platform_i2c_device);
}


//late_initcall(dp501_init);
module_init(dp501_init);

module_exit(dp501_exit);
MODULE_AUTHOR("Urbest inc.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Urbest dp501 DisplayPort");


