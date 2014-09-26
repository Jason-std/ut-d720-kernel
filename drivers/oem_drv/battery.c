#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/jiffies.h>
#include <linux/sched.h>

#include <plat/adc.h>
#include "power_gpio.h"
#include <linux/proc_fs.h>

#include <linux/delay.h>
#include <linux/time.h>
#include <linux/timer.h>

#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#endif


#include <plat/gpio-cfg.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
//#include <mach/gpio-smdkv210.h>
//#include <mach/ut210_gpio_reg.h>

#include <plat/adc.h>

//#define BAT_DEBUG_F
#undef BAT_DEBUG_F

#ifdef BAT_DEBUG_F
#define BAT_DEBUG(fmt,args...)  printk(KERN_INFO"[exynos bat ]: " fmt, ## args) //printk( KERN_DEBUG "[egalax_i2c]: " fmt, ## args)
#define DBG() printk("[exynos bat]: %s, %d => \n",__FUNCTION__,__LINE__)
#else
#define BAT_DEBUG(fmt,args...)		do{ }while(0)
#define DBG()	do{ }while(0)
#endif

#define BATTERY_ADC_VREF	    	1800 /*3300 */   //mv
#define BATTERY_ADC_12BIT_MAX    	0xFFF
#define BATTERY_ADC_CHANNEL	    	0
#define BATTERY_ADC_SAMPLE_COUNT	16

#define BATTERY_TIMER_INTERVAL		(1000)	//ms
//#define BATTERY_TIMER_INTERVAL		(5000)	//ms

#define BATTERY_AC_INTERVAL	        BATTERY_TIMER_INTERVAL
#define BATTERY_ADC_INTERVAL	    (BATTERY_TIMER_INTERVAL * 2)  // (BATTERY_TIMER_INTERVAL * 20)  modified by denis_wei 2010-09-17

static struct platform_device * battery_dev;

static int gAdc_mv_compare;
static int gIsCompare = 0;

static int resume_count = 0;
static int first_boot = 0; 
static struct s3c_adc_client	*client = NULL;

//struct work_struct 	battery_pen_event_work;
struct delayed_work battery_pen_event_work;
struct workqueue_struct *battery_workqueue;

//extern int s3c_adc_get_adc_data(int channel);

#if 0 // for debug s3c_adc_get_adc_data()
//raymanfeng
static struct work_struct delay_open_micbias;

static void delay_micbias_open(struct work_struct* work)
{
	printk("battery:");
	s3c_adc_get_adc_data(0);
	msleep(1000);
	schedule_work(&delay_open_micbias);
}
#endif

#define BAT_DEFAULT 		2
#define BAT_WKY3557115 	3
#define BAT_WKY4062122		4
#define BAT_YJH28113130	5
#define BAT_BRD377589		6
#define BAT_WKY35106140	7
#define BAT_HYDMOMO11		8
#define BAT_F8DDW506		9
#define BAT_WKY387885P		10
#define BAT_WKY467678P         11
//add by eking
#define BAT_WKY5278105P       12
//add end

//add by eking 20130313  Q101-36107142
#define Q101_36107142               13
//add end 



static int g_no_ac = 0;
static int s_model = 9704;
static int bat_model = BAT_DEFAULT;

static int s_ac_online      = 0;
static int s_ac_charging    = 0;

static int s_adc_raw        = 0;
static int s_adc_mv         = 0;
static int s_adc_percent    = 100;

static int s_is_resume = 0;
static int s_times = 0;

#define PROC_NAME "battery"
struct proc_dir_entry *root_entry;
struct proc_dir_entry *entry;


#define SAMPLE_NUM		16
static int ucTemp[SAMPLE_NUM];
static int  BatteryLifePercent =0;
static int ucIndex=0xff;

static int is_charging = 0;
static int charger_is_online =0;

static int gTimes = 0;

#if 0 //to-do
extern int battery_using;   //to-do
extern int ts_battery;
extern char g_selected_utmodel[];
extern char g_selected_manufacturer[];
extern char g_selected_pcb[32];
#endif

int battery_using = 1;

extern struct proc_dir_entry proc_root;
extern char g_selected_utmodel[32];
extern char g_selected_battery[32];

#define ADC_ERR_DELAY	200
#define ADC_ERR_CNT 5


/*
TODO, all driver adc must  attention :  when.......  the ADC val is ERROR !!!!!!!
*/
int s3c_adc_get_adc_data(int channel){
	int adc_value = 0;
	int retry_cnt = 0;

	if (IS_ERR(client)) {
		return -1;  
	}
	
	do {
		adc_value = s3c_adc_read(client, channel);
		if (adc_value < 0) {
			pr_info("%s: adc read(%d), retry(%d)", __func__, adc_value, retry_cnt++);
			msleep(ADC_ERR_DELAY);
		}
	} while (((adc_value < 0) && (retry_cnt <= ADC_ERR_CNT)));

	if(retry_cnt > ADC_ERR_CNT ) 
		return -1;

	return adc_value;
}
EXPORT_SYMBOL(s3c_adc_get_adc_data);

static void check_model(void)
{
	//extern char g_selected_utmodel[32];
	//extern char g_selected_battery[32];

	if(strncmp(g_selected_utmodel, "s901", strlen("s901")) == 0)	
	{		
		s_model = 901;		// S901
		
	}
	else if(strncmp(g_selected_utmodel, "s702", strlen("s702")) == 0)	
	{		
		s_model = 702;		// S702
		
	}
	else if(strncmp(g_selected_utmodel, "s101", strlen("s101")) == 0)	
	{		
		s_model = 101;		// S101
		
	}
	else if(strncmp(g_selected_utmodel, "s501", strlen("s501")) == 0)	
	{		
		s_model = 501;		// S501
		
	}
	//add by eking
       else if(strncmp(g_selected_utmodel, "s703", strlen("s703")) == 0)	
	{		
		s_model = 703;		// S703
		
	}
	//add end
	// battery mode	
	if(strncmp(g_selected_battery,"W115",strlen("W115")) == 0)
	{
		printk("BAT MODE :W115\n");
		bat_model = BAT_WKY3557115;
	}
	else if(strncmp(g_selected_battery,"W122",strlen("W122")) == 0)
	{
		printk("BAT MODE :W122\n");
		bat_model = BAT_WKY4062122;
	}
	else if(strncmp(g_selected_battery,"Y130",strlen("Y130")) == 0)
	{
		printk("BAT MODE :Y130\n");
		bat_model = BAT_YJH28113130;
	}	
	else if(strncmp(g_selected_battery,"B589",strlen("B589")) == 0)
	{
		printk("BAT MODE :B589\n");
		bat_model = BAT_BRD377589;
	}
	else if(strncmp(g_selected_battery,"W140",strlen("W140")) == 0)
	{
		printk("BAT MODE :W140\n");
		bat_model = BAT_WKY35106140;
	}		
	else if(strncmp(g_selected_battery,"HYD1",strlen("HYD1")) == 0)
	{
		printk("BAT MODE :HYD1\n");
		bat_model = BAT_HYDMOMO11;
	}
	else if(strncmp(g_selected_battery,"F506",strlen("F506")) == 0)
	{
		printk("BAT MODE :F506\n");
		bat_model = BAT_F8DDW506;
	}
	else if(strncmp(g_selected_battery,"W885",strlen("W885")) == 0)
	{
		printk("BAT MODE :W885\n");
		bat_model = BAT_WKY387885P;
	}	
	else if(strncmp(g_selected_battery,"W678",strlen("W678")) == 0)
	{
		printk("BAT MODE :W678\n");
		bat_model = BAT_WKY467678P;
	}
	//add by eking
	else if(strncmp(g_selected_battery,"W05P",strlen("W05P")) == 0)
	{
		printk("BAT MODE :W05P\n");
		bat_model = BAT_WKY5278105P;
	}
	//add end
	
         //add by eking 20130313 forQ101_36107142
   else if(strncmp(g_selected_battery,"Q142",strlen("Q142")) == 0)
	{
		printk("BAT MODE :Q142\n");
		bat_model = Q101_36107142;
	}
	//add end
	printk("select model %d bat_model:%d\n", s_model,bat_model);

}

static void battery_detect(unsigned long data);
static struct timer_list battery_timer =
		TIMER_INITIALIZER(battery_detect, 0, 0);


static enum power_supply_property properties_ac[] =
{
	POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property properties_usb[] =
{
	POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property properties_battery[] = 
{
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_SERIAL_NUMBER,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

static int get_property_ac(struct power_supply *supply,
                           enum power_supply_property property,
                           union power_supply_propval * value)
{
    switch (property)
    {
    case POWER_SUPPLY_PROP_ONLINE:
        value->intval = s_ac_online;
        break;
    default:
        break;
    }
    return 0;
}

/*
static int get_property_usb(struct power_supply *supply,
                           enum power_supply_property property,
                           union power_supply_propval * value)
{
    switch (property)
    {
    case POWER_SUPPLY_PROP_ONLINE:
        value->intval = 1;//s3c_is_udc_cable_connected();
        break;
    default:
        break;
    }
    return 0;
}
*/

static int get_property_battery(struct power_supply * supply,
                                enum power_supply_property property,
                                union power_supply_propval * value)
{

    int ret;
    ret	= 0;

    switch (property)
    {
    case POWER_SUPPLY_PROP_STATUS:
        if(s_ac_online) 
		{
			if(s_ac_charging)
				value->intval = POWER_SUPPLY_STATUS_CHARGING;
			else
				value->intval = POWER_SUPPLY_STATUS_FULL;
		} 
		else
            value->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
        break;
    case POWER_SUPPLY_PROP_PRESENT:
        value->intval = 1;
        break;
    case POWER_SUPPLY_PROP_HEALTH:
        value->intval = POWER_SUPPLY_HEALTH_GOOD;
        break;
    case POWER_SUPPLY_PROP_MANUFACTURER:
        value->strval = "Urbest.inc";
        break;
    case POWER_SUPPLY_PROP_TECHNOLOGY:
        value->intval = POWER_SUPPLY_TECHNOLOGY_LION;
        break;
    case POWER_SUPPLY_PROP_CHARGE_COUNTER:
        value->intval = 1024;
        break;
    case POWER_SUPPLY_PROP_TEMP:
        value->intval = 200;   //internal temperature 20.0 degree
        break;
    case POWER_SUPPLY_PROP_CAPACITY:
        value->intval = s_adc_percent;
        break;
    case POWER_SUPPLY_PROP_SERIAL_NUMBER:
        value->strval = "20100726";
        break;
    case POWER_SUPPLY_PROP_VOLTAGE_NOW:
        value->intval = s_adc_mv * 1000;	//needs uv, not mv.
        break;
    default:
        ret = -EINVAL;
        break;
    }
    return ret;
}

static struct power_supply supply_ac =
{
    .name = "ac",
    .type = POWER_SUPPLY_TYPE_MAINS,
    .properties = properties_ac,
    .num_properties = ARRAY_SIZE(properties_ac),
    .get_property = get_property_ac,
};

static struct power_supply supply_battery =
{
    .name = "battery",
    .properties = properties_battery,
    .num_properties = ARRAY_SIZE(properties_battery),
    .get_property = get_property_battery,
    .use_for_apm = 1,
};
/*
static struct power_supply supply_usb =
{
    .name = "usb",
    .type = POWER_SUPPLY_TYPE_USB,
    .properties = properties_usb,
    .num_properties = ARRAY_SIZE(properties_usb),
    .get_property = get_property_usb,
};
*/

static inline unsigned int ms_to_jiffies(unsigned int ms)
{
        unsigned int j;
        j = (ms * HZ + 500) / 1000;
        return (j > 0) ? j : 1;
}

struct cap_table 
{
	int vol;
	int cap;
};
//add by eking 20130313  for  Q101-36107142
static struct cap_table s_capTable_s101_2[] =
{
    {3500, 0},
    {3550, 2},
    {3600, 5},
    {3620, 8},
    {3640, 13},
    {3650, 20},
    {3670, 30},
    {3680, 40},
    {3700, 50},
    {3710, 60},
    {3720, 75},
    {3740, 80},
    {3780, 90},
    {3845, 95},
    {3960, 100},
};
//add end

//add by eking 
static struct cap_table s_capTable_s703[] =
{

    {3550, 0},       //
    {3630, 7},	
    {3670, 14},
    {3690, 21},
    {3710, 28},
    {3720, 35},
    {3735, 42},
    {3750, 49},
    {3780, 56},
    {3830, 63},
    {3860, 70},
    {3890, 77},
    {3941, 85},
    {3990, 90},
    {4055, 95},
    {4088, 100},
};
//add end
static struct cap_table s_capTable_s501[] =
{

    {3500, 0},
    {3585,5},
    {3600, 12},
		
    {3640, 18},
    {3670, 26},
    {3720, 40},
    {3750, 52},
    {3790, 62},
    {3840, 70},
    {3940, 82},
    {4000, 92},
    {4030, 100},
};


// for 9704 v2 BAT_DEFAULT
static struct cap_table s_capTable_9704_default[] =
{
/*
    {3450, 0},
    {3480, 3},
    {3520, 8},
    {3552, 12},
*/
    {3500, 0},
    {3535, 8},
    {3560, 12},
		
    {3590, 18},
    {3650, 26},
    {3700, 40},
    {3765, 52},
    {3830, 62},
    {3880, 70},
    {3960, 82},
    {4000, 92},
    {4055, 100},
};

//for 9704 BAT_WKY3557115
static struct cap_table s_capTable_9704[] =
{
/*  ORIG
    {3400, 0},
    {3575, 4},
    {3650, 10},
    {3700, 23},
    {3765, 38},
    {3810, 50},
    {3860, 65},
    {3900, 78},
    {3935, 86},
    {3960, 92},
    {4000, 96},
    {4050, 100},
*/
//v2 version battery
/*
    {3450, 0},
    {3480, 3},
    {3520, 8},
    {3552, 12},
    {3590, 18},
    {3650, 26},
    {3700, 40},
    {3765, 52},
    {3830, 62},
    {3880, 70},
    {3960, 82},
    {4000, 92},
    {4055, 100},
*/
//v3 battery  :WKY 3557115 3.7V 2500MAH * 2

/* 
    {3450, 0},
    {3480, 3},
    {3520, 5},
    {3590, 8},
*/
    {3500, 0},
    {3550, 5},
    {3600, 8},

    {3650, 15},
    {3700, 26},
    {3765, 40},
    {3830, 58},
    {3880, 70},
    {3960, 81},
    {4000, 90},
    {4055, 100},

};

// 
static struct cap_table s_capTable_s901[] =
{
#if 0	//WKY 4062122-3600mAh*2  wky4062122
    {3450, 0},
    {3545, 3},
    {3569, 8},
    {3584, 11},
    {3610, 16},
    {3644, 25},
    {3668, 40},
    {3707, 51},
    {3770, 62},
    {3827, 70},
    {3902, 81},
    {3995, 92},
    {4050, 100},


#endif		//WKY 4012122-3600mAh*2  wky4012122
/*
	{3450, 0},
	{3480, 3},
	{3560, 8},
	{3600, 16},
*/

	{3500, 0},
	{3570, 8},
	{3610, 16},
		
	{3640, 25},
	{3690, 40},
	{3740, 54},
	{3787, 62},
	{3844, 70},
	{3931, 83},
	{4000, 92},
	{4055, 100},
};


// yongjiahe28113130-4800mAh BAT_YJH28113130
static struct cap_table s_capTable_9704_1[] =
{
/*	
    {3450, 0},
    {3475, 3},
    {3490, 8},
    {3505, 11},
    {3528, 15},
    {3570, 25},
    {3670, 40},
    {3710, 51},
    {3770, 62},
    {3825, 70},
    {3900, 81},
    {3990, 92},
    {4050, 100},
*/
    {3500, 0},
    {3550, 8},
    {3590, 15},
    {3625, 25},
    {3700, 40},
    {3740, 51},
    {3790, 62},
    {3840, 70},
    {3910, 81},
    {3990, 92},
    {4050, 100},	
};

// BRD377589-2800mAh BAT_BRD377589
static struct cap_table s_capTable_s702[] =
{
/*	
    {3350, 0},
    {3450, 3},
    {3490, 8},
    {3570, 10},
    {3591, 16},
    */

    {3500, 0},
    {3550, 5},
    {3570, 10},
    {3595, 16},
	
    {3615, 22},
    {3642, 34},
    {3684, 46},
    {3741, 58},
    {3810, 70},
    {3900, 82},
    {4000, 94},
    {4075, 100},
};

//WKY 21.46WH 35106140P-5800mAh
static struct cap_table s_capTable_s101[] =
{
	{3500, 0},
	{3570, 6},
	{3600, 10},
	{3630, 16},
	{3660, 25},
	{3690, 35},
	{3735, 50},
	{3785, 60},
	{3850, 70},
	{3920, 80},
	{4005, 90},
	{4070, 100},
};

// for MOMO11 BAT_HYDMOMO11 huanyuda
static struct cap_table s_capTable_MOMO11[] =
{
/*	
    {3450, 0},
    {3480, 2},
    {3520, 4},
    {3552, 6},
    {3590, 12}, 
*/
    {3500, 0},
    {3520, 2},
    {3540, 4},
    {3560, 6},
    {3600, 12}, 

    {3650, 22},
    {3700, 37},
    {3765,49},
    {3830, 60},
    {3880, 70},
    {3960, 82},
    {4000, 94},
    {4050, 100},
};


// for F8DDW506
static struct cap_table s_capTable_F506[] =
{
    {3510, 0},
    {3530, 2},
    {3560, 5},
    {3580, 11},
    {3635, 20}, 
    {3660, 30},
    {3690, 41},
    {3738, 52},
    {3818, 66},
    {3895, 77},
    {3990, 88},
    {4030, 100},
};


// for wky387885P
static struct cap_table s_capTable_W885[] =
{
    {3500, 0},
    {3570, 5},
    {3610, 9},
    {3645, 19}, 
    {3670, 28},
    {3715, 42},
    {3740, 52},
    {3820, 66},
    {3890, 77},
    {4000, 90},
    {4050, 100},
};


#if 0
int mv_to_percent_9704(int voltage)
{
   	 int i, percent, d_vol, d_cap, len;
        len = sizeof(s_capTable_9704) / sizeof(s_capTable_9704[0]);
	
	//BAT_DEBUG("%s\n",__func__);

	if (voltage <= s_capTable_9704[0].vol)
	        percent = 0;
	if (voltage >= s_capTable_9704[len - 1].vol)
	        percent = 100;
	else
    	{
	        for (i = len - 2; i > 0; i--)
	        {
	           	 if (voltage >= s_capTable_9704[i].vol)
	               	 break;
	        }
    
        d_vol = s_capTable_9704[i + 1].vol - s_capTable_9704[i].vol;
        d_cap = s_capTable_9704[i + 1].cap - s_capTable_9704[i].cap;
        percent = s_capTable_9704[i].cap + (voltage - s_capTable_9704[i].vol) * d_cap / d_vol;
    	}
	if(percent < 0)
		percent = 0;
	if(percent > 100)
	        percent = 100;
	
       //BAT_DEBUG("voltage:%d, percent:%d\n", voltage, percent);
	return percent;
}

#else

int mv_to_percent(int voltage,struct cap_table *table,int table_lenth)
{
   	 int i, percent, d_vol, d_cap, len;
        //len = sizeof(table) / sizeof(table[0]);
		len = table_lenth;
	//BAT_DEBUG("table_lenth:%d\n", table_lenth);	
	//BAT_DEBUG("table_lenth:%d, table[2].vol:%d\n", len, table[2].vol);	
	
	//BAT_DEBUG("%s\n",__func__);

	if (voltage <= table[0].vol)
	        percent = 0;
	if (voltage >= table[len - 1].vol)
	        percent = 100;
	else
    	{
	        for (i = len - 2; i > 0; i--)
	        {
	           	 if (voltage >= table[i].vol)
	               	 break;
	        }
    
        d_vol = table[i + 1].vol - table[i].vol;
        d_cap = table[i + 1].cap - table[i].cap;
        percent = table[i].cap + (voltage - table[i].vol) * d_cap / d_vol;
    	}
	if(percent < 0)
		percent = 0;
	if(percent > 100)
	        percent = 100;
	
       BAT_DEBUG("voltage:%d, percent:%d\n", voltage, percent);
	return percent;
}

#endif

static void poll_adc_raw(int channel)
{
   	 int count, sum, i, avg;
   	 int buffer[BATTERY_ADC_SAMPLE_COUNT];
	
	int percent,tab_lenth;

	static int timeout = 0;

	 sum = 0;
   	 count = 0;
   	// BAT_DEBUG("%s(%d).\n", __func__, channel);

	
	battery_using = 1;
	
   	 s3c_adc_get_adc_data(channel);	
// del udelay  ,,,,  it is very dangerous  !!!!!!!! 
//   	 udelay(10); /* the first sample data is error */ 	
   	 for(i = 0; i < BATTERY_ADC_SAMPLE_COUNT; i++)
   	 {
	        buffer[i] = s3c_adc_get_adc_data(channel);
			if(sum += buffer[i] != 0)
			{
		    		sum += buffer[i];
				count++;
			}
 // del udelay  ,,,,  it is very dangerous  !!!!!!!!
 //		udelay(10);
   	 }
	
	battery_using = 0;

	if (count == 0)
	{
		return;  
	}

	avg = sum / count;

    sum = 0;
    count = 0;
    for(i = 0; i < BATTERY_ADC_SAMPLE_COUNT; i++)
    {
	        if(abs(buffer[i] - avg) < /*(avg / 3)*/200)
	        {
	            sum += buffer[i];
	            count++;
	        }
    }

	if (count == 0)  {
		return;
	}

#if 1  // open if the chg_dec pin OK
	charger_is_online = read_power_item_value(POWER_STATE_AC); // to-do
	if(charger_is_online)
   	{
		is_charging = read_power_item_value(POWER_STATE_CHARGE);    //to-do
		
		// for 9704
		if(s_model == 9704 || s_model == 901 || s_model == 702 || s_model == 101 ||s_model == 501)
			is_charging = !is_charging;
   	}
	else
		is_charging=0;
#endif

#if 0 //for debug; must delete future
	is_charging = read_power_item_value(POWER_STATE_CHARGE);    //to-do
	is_charging = !is_charging;
	if(is_charging)
		charger_is_online = 1;
	else
		charger_is_online = 0;
#endif

	//printk("charger_is_online = %d,is_charging = %d\n",charger_is_online,is_charging);
		
	s_adc_raw = sum / count;
	
	s_adc_mv = (s_adc_raw * BATTERY_ADC_VREF / BATTERY_ADC_12BIT_MAX) * 3;
       // printk("---s_adc_mv = %d--\n",s_adc_mv);//曲线调试ADC值
	
	//s_adc_mv  =s_adc_mv+180;  //denis_wei add 2011-01-05  采样值比实际电压值小0.2v左右 
	//s_adc_mv  =s_adc_mv - 60;
	
#if 1  // adjust s_adc_mv  second  in different MID style either in charging or not
	if (s_model == 9704 && bat_model == BAT_WKY3557115 ) 
		//s_adc_mv += 80;  //最好是让这次调整后电压值和万用表测得的一样，即不充电的电压等于实际电压
		s_adc_mv  =s_adc_mv - 60;
	else if(s_model == 9704 &&bat_model == BAT_HYDMOMO11)
		s_adc_mv = s_adc_mv - 55 ;
	else if (s_model == 901)
	{
		if(bat_model == BAT_F8DDW506)
			s_adc_mv  =s_adc_mv - 30;
		else
			s_adc_mv  =s_adc_mv - 35;
	}
	else if (s_model == 702)
	{
		if(bat_model == BAT_WKY387885P)
			s_adc_mv  =s_adc_mv - 20;
		else
			s_adc_mv  =s_adc_mv - 35;
	}
	//add by eking 20130313
	else if (s_model == 101&& bat_model ==Q101_36107142 )
		{
		 if(s_adc_mv > 3700)
			s_adc_mv  =s_adc_mv - 30;	
		 else
		 	s_adc_mv  =s_adc_mv;	
		}
	//add end
	else if (s_model == 101)
		s_adc_mv  =s_adc_mv - 10;	
	
	else if(s_model == 501)
		{
			if(bat_model == BAT_WKY467678P)
			 	s_adc_mv  =s_adc_mv - 15;	
			else 
				s_adc_mv  =s_adc_mv - 10;	
		}
	//add by eking
    else if(s_model == 703 && bat_model == BAT_WKY5278105P)
         	{
         	    if(!is_charging)
         	    	{
			  if(s_adc_mv >3800)
                            {
                          	  s_adc_mv = s_adc_mv+20;
			    }
			   else if(s_adc_mv >3600)
                             {
                            	 s_adc_mv = s_adc_mv+35;
			     }
			   else if(s_adc_mv >3500)
			  	{
			  	s_adc_mv = s_adc_mv+40;
			        } 
         	    	}
		    else{
                            s_adc_mv = s_adc_mv; 
			  }		
         	}	 	
	//add end
	else 
		s_adc_mv  =s_adc_mv - 60;
#endif

	//printk("111==s_adc_raw:%d,1111s_adc_mv[0]:%d  \n",s_adc_raw,s_adc_mv);	
 
//adjust  s_adc_mv  when in charging  in different mid style
	if(is_charging)
	{
	      
		if (s_model == 9704 && bat_model == BAT_WKY3557115 ) 
		{
			//printk("9704 BAT_WKY3557115 mv\n");
			if(s_adc_mv > 4180)   // todo
			{
				s_adc_mv = s_adc_mv - 140;
			}
			else if(s_adc_mv > 4175)
			{
				s_adc_mv = s_adc_mv - 150;
			}
			else if(s_adc_mv > 4168)
			{
				s_adc_mv = s_adc_mv - 165;
			}
			else if(s_adc_mv > 4150)
			{
				s_adc_mv = s_adc_mv - 175;
			}
			else if(s_adc_mv > 4100)
			{
				s_adc_mv = s_adc_mv - 185;  // 110 //160 
			}
			else if(s_adc_mv > 4000)
			{
				s_adc_mv = s_adc_mv - 195;  // 110 // 155
			}
			else if(s_adc_mv > 3900)
			{
				s_adc_mv = s_adc_mv - 205; // 115 // 160
			}
			else if(s_adc_mv > 3800)
			{
				s_adc_mv = s_adc_mv - 215; // 115 // 170
			}
			else if(s_adc_mv > 3700)
			{
				s_adc_mv = s_adc_mv -245; // 125 //175
			}
			//else if(s_adc_mv > 3600) // 3530
			//	{ 
			//		s_adc_mv = s_adc_mv -180; //130 //180
			//	}
			else
			{
				s_adc_mv = 3455; //3400
			}
		}
		else if (s_model == 9704 && bat_model == BAT_YJH28113130 ) 
		{
			//printk("9704 BAT_YJH28113130 mv\n");
			if(s_adc_mv > 4185)
			{
				s_adc_mv = s_adc_mv - 100; 
			}
			else if(s_adc_mv > 4182)
			{
				s_adc_mv = s_adc_mv - 200; 
			}
			else if(s_adc_mv > 4180)
			{
				s_adc_mv = s_adc_mv - 260; 
			}		
			else if(s_adc_mv > 4170)
			{
				s_adc_mv = s_adc_mv - 270; 
			}					
			else if(s_adc_mv > 4150)
			{
				s_adc_mv = s_adc_mv - 300; 
			}				
			else if(s_adc_mv > 4000)
			{
				s_adc_mv = s_adc_mv - 320; 
			}
			else if(s_adc_mv > 3900)
			{
				s_adc_mv = s_adc_mv - 330; 
			}				
			else if(s_adc_mv > 3800)
			{
				s_adc_mv = s_adc_mv - 350;
			}
			else
			{
				s_adc_mv = 3450;
			}

		}
		else if (s_model == 9704 && bat_model == BAT_HYDMOMO11 ) 
		{
			//printk("9704 BAT_HYDMOMO11 mv\n");
			if(s_adc_mv > 4000){
				s_adc_mv = s_adc_mv - 140;  // 110  // 140
			}
			else if(s_adc_mv > 3900){
				s_adc_mv = s_adc_mv - 170; // 115  // 150
			}
			else if(s_adc_mv > 3800){
				s_adc_mv = s_adc_mv - 190; // 115  // 165
			}
			else if(s_adc_mv > 3700){
				s_adc_mv = s_adc_mv -195; // 125 // 175
			}
			else if(s_adc_mv > 3600){ // 3530
				s_adc_mv = s_adc_mv -195; //130 // 180
			}
			else{
				s_adc_mv = 3450; //3400
			}
		}		
		else if (s_model == 901 && bat_model == BAT_WKY4062122)
		{
			//printk("901 BAT_WKY4062122 mv\n");
		#if 0	
			if(s_adc_mv > 4180)
			{
				s_adc_mv = s_adc_mv - 100; 
			}				
			else 
		#endif		
			if(s_adc_mv > 4180)
			{
				s_adc_mv = s_adc_mv - 125; 
			}				
			else if(s_adc_mv > 4100)
			{
				s_adc_mv = s_adc_mv - 130; 
			}				
			else if(s_adc_mv > 3900)
			{
				s_adc_mv = s_adc_mv - 135; 
			}
			else if(s_adc_mv > 3800)
			{
				s_adc_mv = s_adc_mv - 145; 
			}				
			else if(s_adc_mv > 3750)
			{
				s_adc_mv = s_adc_mv - 150;
			}
			else if(s_adc_mv > 3650)
			{
				s_adc_mv = s_adc_mv - 155;
			}
			else
			{
				s_adc_mv = 3450;
			}
		}		
		else if (s_model == 901 && bat_model == BAT_F8DDW506)
		{
			printk("901 BAT_F8DDW506 mv\n");

			if(s_adc_mv > 4206)
			{
				s_adc_mv = s_adc_mv - 180; 
			}			
			else if(s_adc_mv > 4200)
			{
				s_adc_mv = s_adc_mv - 250; 
			}
			else if(s_adc_mv > 4100)
			{
				s_adc_mv = s_adc_mv - 280; 
			}				
			else if(s_adc_mv > 4070)
			{
				s_adc_mv = s_adc_mv - 290; 
			}				
			else if(s_adc_mv > 4050)
			{
				s_adc_mv = s_adc_mv - 310; 
			}				
			else if(s_adc_mv > 4030)
			{
				s_adc_mv = s_adc_mv - 320; 
			}
			else if(s_adc_mv > 4000)
			{
				s_adc_mv = s_adc_mv - 330; 
			}				
			else if(s_adc_mv > 3900)
			{
				s_adc_mv = s_adc_mv - 335;
			}
			else if(s_adc_mv > 3860)
			{
				s_adc_mv = s_adc_mv - 345;
			}
			else
			{
				s_adc_mv = 3510;
			}
		}				
		else if (s_model == 702 && bat_model == BAT_BRD377589)
		{
			//printk("_+_+_+702 BAT_BRD377589 mv\n");
			if(s_adc_mv > 4175)
			{
				s_adc_mv = s_adc_mv - 100; 
			}							
			else if(s_adc_mv > 3900)
			{
				s_adc_mv = s_adc_mv - 105;
			}
			else if(s_adc_mv > 3850)
			{
				s_adc_mv = s_adc_mv - 110;
			}			
			else if(s_adc_mv > 3750)
			{
				s_adc_mv = s_adc_mv - 115;
			}
			else if(s_adc_mv > 3530)
			{
				s_adc_mv = s_adc_mv - 130;
			}
			else
			{
				s_adc_mv = 3400;
			}
		}				
		else if (s_model == 702 && bat_model == BAT_WKY387885P)
		{
			//printk("_+_+_+702 BAT_WKY387885P mv\n");
			if(s_adc_mv > 4160)
			{
				s_adc_mv = s_adc_mv - 120; 
			}							
			else if(s_adc_mv > 4050)
			{
				s_adc_mv = s_adc_mv - 125; 
			}							
			else if(s_adc_mv > 3950)
			{
				s_adc_mv = s_adc_mv - 130;
			}
			else if(s_adc_mv > 3850)
			{
				s_adc_mv = s_adc_mv - 135;
			}			
			else if(s_adc_mv > 3750)
			{
				s_adc_mv = s_adc_mv - 140;
			}
			else if(s_adc_mv > 3650)
			{
				s_adc_mv = s_adc_mv - 150;
			}
			else
			{
				s_adc_mv = 3500;
			}
		}
		// add by eking 20130313
		else if (s_model == 101&&bat_model == Q101_36107142 ) 
		{
			if(s_adc_mv >4000)
				{
				s_adc_mv = s_adc_mv - 200;
				}
			else if(s_adc_mv >3900)
				{
				s_adc_mv = s_adc_mv - 160;
				}
			else if(s_adc_mv >3850)
				{
				s_adc_mv = s_adc_mv - 140;
				}
			else if(s_adc_mv >3800)
				{
				s_adc_mv = s_adc_mv - 180;
				}
		}
		//add end
		else if (s_model == 101)
		{
			//printk("_+_+_+101 35106140 mv\n");
			if(s_adc_mv > 4150)
			{
				s_adc_mv = s_adc_mv - 110;
			}				
			else if(s_adc_mv > 4000)
			{
				s_adc_mv = s_adc_mv - 115;
			}					
			else if(s_adc_mv > 3890)
			{
				s_adc_mv = s_adc_mv - 120;
			}
			else if(s_adc_mv > 3645)
			{
				s_adc_mv = s_adc_mv - 125;
			}
			else
			{
				s_adc_mv = 3500;
			}
		}	
		else if((s_model == 501) && (bat_model ==BAT_WKY467678P) )
		{
                      //   printk("BAT_WKY467678P is de\n");
			 if(s_adc_mv  > 4150)
			 	{
			 		s_adc_mv = s_adc_mv - 100;
			 	}
			else if(s_adc_mv  > 3950)
			 	{
			 		s_adc_mv = s_adc_mv - 125;
				}
			 else if(s_adc_mv  > 3850)
			 	{
			 		s_adc_mv = s_adc_mv - 175;
			 	}
			 else if(s_adc_mv  > 3740)
			 	{
			 		s_adc_mv = s_adc_mv - 165;
			 	}
			 else
			 	{
			 		s_adc_mv = 3500;
			 	}
		}
              //add by eking
              else if(s_model == 703 && bat_model == BAT_WKY5278105P)
              	{
                         if(s_adc_mv > 4180)
                         	{
                         	s_adc_mv = s_adc_mv-100;
                         	}
			 else if(s_adc_mv > 4100)
                         	{
                         	s_adc_mv = s_adc_mv-155;
                         	}  
                         else if(s_adc_mv > 3950)
                         	{
                         	s_adc_mv = s_adc_mv-160;
                         	}  
                          else if(s_adc_mv > 3900)
                         	{
                         	s_adc_mv = s_adc_mv-175;
                         	} 
			  else if(s_adc_mv > 3800)
                         	{
                         	s_adc_mv = s_adc_mv-170;
                         	} 			  
			 else if(s_adc_mv > 3780)
                         	{
                         	s_adc_mv = s_adc_mv-180;
                         	} 	
              	}
			  
	   //add end

	
		else
		{
			if(s_adc_mv > 4000){
				s_adc_mv = s_adc_mv - 155;  // 110
			}
			else if(s_adc_mv > 3900){
				s_adc_mv = s_adc_mv - 160; // 115
			}
			else if(s_adc_mv > 3800){
				s_adc_mv = s_adc_mv - 170; // 115
			}
			else if(s_adc_mv > 3700){
				s_adc_mv = s_adc_mv -175; // 125
			}
			else if(s_adc_mv > 3600){ // 3530
				s_adc_mv = s_adc_mv -180; //130
			}
			else{
				s_adc_mv = 3420; //3400
			}
		}

	}
  //  printk("[1]s_adc_mv==:%d   \n",s_adc_mv );

#if 1		
	if(!gIsCompare)
	{
		gAdc_mv_compare = s_adc_mv;
		gIsCompare = 1;
	}
	else
	{
		if(abs(s_adc_mv - gAdc_mv_compare) >75)
		{
			s_adc_mv = gAdc_mv_compare;
			gIsCompare = 0;
		}
		else
			gAdc_mv_compare = s_adc_mv;
	}
#endif


	if(s_model == 901 && bat_model == BAT_WKY4062122)
	{
		//printk("901 BAT_WKY4062122 percent\n");
		tab_lenth = sizeof(s_capTable_s901) / sizeof(s_capTable_s901[0]);
		percent = mv_to_percent(s_adc_mv,s_capTable_s901,tab_lenth);
	}
	else if (s_model == 9704 && bat_model == BAT_WKY3557115)
	{
		//printk("9704 BAT_WKY3557115 percent\n");
		tab_lenth = sizeof(s_capTable_9704) / sizeof(s_capTable_9704[0]);
		percent = mv_to_percent(s_adc_mv,s_capTable_9704,tab_lenth);
	}
	else if (s_model == 9704 && bat_model == BAT_YJH28113130)
	{
		//printk("9704 BAT_YJH28113130 percent\n");
		tab_lenth = sizeof(s_capTable_9704_1) / sizeof(s_capTable_9704_1[0]);
		percent = mv_to_percent(s_adc_mv,s_capTable_9704_1,tab_lenth);
	}
	else if (s_model == 9704 && bat_model == BAT_HYDMOMO11)
	{
		//printk("9704 BAT_HYDMOMO11 percent\n");
		tab_lenth = sizeof(s_capTable_MOMO11) / sizeof(s_capTable_MOMO11[0]);
		percent = mv_to_percent(s_adc_mv,s_capTable_MOMO11,tab_lenth);
	}	
	else if (s_model == 702 && bat_model == BAT_BRD377589)
	{
		//printk("702 BAT_BRD377589 percent\n");
		tab_lenth = sizeof(s_capTable_s702) / sizeof(s_capTable_s702[0]);
		percent = mv_to_percent(s_adc_mv,s_capTable_s702,tab_lenth);
	}	
	else if (s_model == 101)
	{
		//printk("101 BAT_WKY35106140 percent\n");
		tab_lenth = sizeof(s_capTable_s101) / sizeof(s_capTable_s101[0]);
		percent = mv_to_percent(s_adc_mv,s_capTable_s101,tab_lenth);
	}
	else if(s_model == 901 && bat_model == BAT_F8DDW506)
	{
		//printk("901 BAT_WKY4062122 percent\n");
		tab_lenth = sizeof(s_capTable_F506) / sizeof(s_capTable_F506[0]);
		percent = mv_to_percent(s_adc_mv,s_capTable_F506,tab_lenth);
	}	
	else if(s_model == 702 && bat_model == BAT_WKY387885P)
	{
		//printk("702 BAT_WKY4062122 percent\n");
		tab_lenth = sizeof(s_capTable_W885) / sizeof(s_capTable_W885[0]);
		percent = mv_to_percent(s_adc_mv,s_capTable_W885,tab_lenth);
	}
	else if(s_model ==501&& bat_model == BAT_WKY467678P)
	{
		//printk("501   BAT_WKY467678P percent\n");
		tab_lenth = sizeof(s_capTable_s501) / sizeof(s_capTable_s501[0]);	
		percent = mv_to_percent(s_adc_mv,s_capTable_s501,tab_lenth);
	}
	//add by eking
	else if(s_model ==703&& bat_model == BAT_WKY5278105P)
	{
		//printk("703   BAT_WKY467678P percent\n");
		tab_lenth = sizeof(s_capTable_s703) / sizeof(s_capTable_s703[0]);	
		percent = mv_to_percent(s_adc_mv,s_capTable_s703,tab_lenth);
	}
	//add end

	//add by eking 20130313
	else if (s_model == 101&&bat_model == Q101_36107142 ) 
	{
		//printk("101   Q101_36107142 percent\n");
		tab_lenth = sizeof(s_capTable_s101_2) / sizeof(s_capTable_s101_2[0]);	
		percent = mv_to_percent(s_adc_mv,s_capTable_s101_2,tab_lenth);
	}
	//add end
	
	else
	{
		//printk("default percent\n");
		tab_lenth = sizeof(s_capTable_9704_default)/sizeof(s_capTable_9704_default[0]);
		percent = mv_to_percent(s_adc_mv, s_capTable_9704_default,tab_lenth);
	}
	

	if(!is_charging)
		{
			if ((gTimes < 3) && (percent == 0))
			{
					percent = 1;
					gTimes++;
			}
			
			if ((ucIndex==0xff) || (timeout > 4))
				{
					s_adc_percent = percent;
					timeout = 0;
				}
			else
				{
					if (abs(s_adc_percent - percent) < 7)
						{
							s_adc_percent = percent;
							timeout = 0;
						}
					else
						timeout++;
				}
			
		}
	else
		{
			gTimes = 0;
			s_adc_percent = percent;
			if (s_adc_percent < 1)
				s_adc_percent = 1;
		}

 	if(first_boot < 3)	
			{
				for(i=0;i<SAMPLE_NUM;i++)
				{
					ucTemp[i]=0;
					ucTemp[i]=s_adc_percent;
				}
				first_boot = first_boot + 1;
			}

#if 1
       // 平滑百分比 denis_wei add 2010-09-19
	if(ucIndex==0xff)
	{	
		for(i=0;i<SAMPLE_NUM;i++)
			ucTemp[i]=s_adc_percent;

		ucIndex = 0;

	}
	else
	{
		if(ucIndex>=SAMPLE_NUM)
			ucIndex=0;
		
		 ucTemp[ ucIndex++ ]=s_adc_percent;
      		 BatteryLifePercent=0;
			  
		for(i=0;i<SAMPLE_NUM;i++)
		{
			BatteryLifePercent = BatteryLifePercent+ucTemp[i] ;
			//printk("ucTemp[ %d]=%d\n", i, ucTemp[i]);
		}
		s_adc_percent = (BatteryLifePercent / SAMPLE_NUM);

	}
	
		if(is_charging)
           	{   

			if(s_adc_percent == 0)
			{
				s_adc_percent =1;
			}
			
           		if(s_adc_percent > 98)
			{
				s_adc_percent =99;
			}
           	}
	
#endif       

	//printk("charger_is_online=%d is_charging=%d\n", charger_is_online, is_charging);
	//printk("s_adc_raw=%d s_adc_mv=%d s_adc_percent=%d\n\n\n", s_adc_raw, s_adc_mv, s_adc_percent);
}


/*******
**	1: report state of supply_battery 
**	2: report state of supply_ac
********/
static void battert_work(struct work_struct *work)
{
  	 static int count = BATTERY_ADC_INTERVAL / BATTERY_TIMER_INTERVAL;
   	 int online, charging;

//1: report state of supply_battery 
#if 1			
	   if(count * BATTERY_TIMER_INTERVAL >= BATTERY_ADC_INTERVAL)
	   {
	        poll_adc_raw(BATTERY_ADC_CHANNEL);
	 // 		battery_using = 0;
	        power_supply_changed(&supply_battery);
	        count = 0;
	    }
	    else
	        count++;
#else
		 poll_adc_raw(BATTERY_ADC_CHANNEL);	
     		power_supply_changed(&supply_battery);
#endif


//2: report state of supply_ac 	
	if (g_no_ac)//no ac
	{
		charging = read_power_item_value(POWER_STATE_CHARGE);  // to-do
	}
	else
	{
		online = read_power_item_value(POWER_STATE_AC);  // to-do
		if(online)
			{
				charging = read_power_item_value(POWER_STATE_CHARGE); //to-do
				
				#if 1  // for future in different mid style
					if (s_model == 9704 || s_model == 901 || s_model == 702 || s_model == 101||s_model == 501)
						charging = !charging;
				#endif
			}
		else
			charging=0;
	}

	//printk("online = %d,charging = %d\n",online,charging);
#if 0
	if(charging)
		online = 1;
	else
		online = 0;
#endif

    if((online != s_ac_online) || (charging != s_ac_charging))
    {    
        s_ac_online = online;
        s_ac_charging = charging;
	
        power_supply_changed(&supply_ac);
    }

  // printk("s_ac_online=%d s_ac_charging=%d\n", s_ac_online, s_ac_charging);

}


static void battery_detect(unsigned long data)
{
	#if 1  // todo for special mid style
	if (s_model == 9704 || s_model == 901 || s_model == 702 || s_model == 101)
		{
			if (resume_count < 3)
				{
									
					if (/*!work_pending(&battery_pen_event_work)*/ !delayed_work_pending(&battery_pen_event_work)) 
						{
							//queue_work(battery_workqueue, &battery_pen_event_work);
							queue_delayed_work(battery_workqueue, &battery_pen_event_work, ms_to_jiffies(50));
						}
					mod_timer(&battery_timer, jiffies + ms_to_jiffies(200));
					resume_count++;
					return;
				}
		}
	#endif
	
	if (/*!work_pending(&battery_pen_event_work)*/ !delayed_work_pending(&battery_pen_event_work)) 
	{	
		//queue_work(battery_workqueue, &battery_pen_event_work);
		queue_delayed_work(battery_workqueue, &battery_pen_event_work, ms_to_jiffies(50));
	}
	mod_timer(&battery_timer, jiffies + ms_to_jiffies(BATTERY_TIMER_INTERVAL));

}

#if 1
//denis_wei add for read write battery info 2011-01-05
static int battery_proc_write(struct file *file, const char *buffer, 
                           unsigned long count, void *data) 
{      
	printk("\n battery_proc_write\n");

	return 1; 
} 

static int battery_proc_read(char *page, char **start, off_t off, 
			  int count, int *eof, void *data) 
{
#if 1
	int value;
	int i;
	int len;

	len = sprintf(page, "\n%s = \n", __func__);
	for (i=0; i<5; i++)  {
		value = s3c_adc_get_adc_data(0);   // to-do
		len += sprintf(page+len, " %d,", value);
		udelay(10);
	}
	udelay(100);
	len += sprintf(page+len, "\n");
	for (i=0; i<5; i++)  {
		value = s3c_adc_get_adc_data(1);// to-do
		len += sprintf(page+len, " %d,", value);
		udelay(10);
	}
	len += sprintf(page+len, "}\n");
	
	return len;
#endif
	return 0;
}
//add end 2011-01-05
#endif


#ifdef CONFIG_HAS_EARLYSUSPEND
void battery_early_suspend(struct early_suspend *h)
{
	mod_timer(&battery_timer, jiffies + ms_to_jiffies(5000)); 
	//cancel_work_sync(&battery_pen_event_work);
	cancel_delayed_work_sync(&battery_pen_event_work);

	printk("[BAT]battery_suspend\n");
}

void battery_late_resume(struct early_suspend *h)
{
#if 1   // todo
//	int i = 0;
	printk("[BAT]battery_resume++\n");
	
	s_is_resume = 1;
	s_times = 0;

	gTimes = 0;
	resume_count = 0;
	
	//schedule_work(&battery_pen_event_work);
	//schedule_delayed_work(&battery_pen_event_work, ms_to_jiffies(80));
	queue_delayed_work(battery_workqueue, &battery_pen_event_work, ms_to_jiffies(80));

	#if 0
		ucIndex = 0xff;
		poll_adc_raw(BATTERY_ADC_CHANNEL);
		mdelay(10);

		ucIndex = 0xff;
		mod_timer(&battery_timer, jiffies + ms_to_jiffies(80)); 
		ucIndex = 0xff;

		power_supply_changed(&supply_battery);
		power_supply_changed(&supply_ac);
	#else
	//	if (!strncmp(g_selected_utmodel, "105", strlen("105")))  // orig
		if(s_model == 9704) // for debug 
		{
			ucIndex = 0xff;
			mod_timer(&battery_timer, jiffies + ms_to_jiffies(2000)); // attention:: >= 2000ms , if not will wakeup FAIL
			ucIndex = 0xff;		
		}
		else
		{
			ucIndex = 0xff;
			mod_timer(&battery_timer, jiffies + ms_to_jiffies(80)); 
			ucIndex = 0xff;
		}
	#endif
#endif
	printk("[BAT]battery_resume--\n");

}

static struct early_suspend battery_android_suspend = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1,
	.suspend = battery_early_suspend,
	.resume  = battery_late_resume,
};
#endif /* CONFIG_HAS_EARLYSUSPEND */

static int __init battery_init(void)
{
    int ret = 0; 
    
    
    printk("++ urbest Battery driver init.\n");
    battery_dev = platform_device_register_simple("battery", 0, NULL, 0);
    printk("++ urbest Battery driver init----.\n");



	client = s3c_adc_register(battery_dev, NULL, NULL, 0);
	if (IS_ERR(client)) {
		dev_err(&battery_dev->dev, "cannot register adc\n");
		return PTR_ERR(client);
	}

	platform_set_drvdata(battery_dev, client);

	
    if(0 != power_supply_register(&battery_dev->dev, &supply_ac))
    {
        power_supply_unregister(&supply_ac);
        platform_device_unregister(battery_dev);
        ret = -1;
    }
	printk("++ urbest Battery driver init.---1\n");

	gIsCompare = 0;
    
    supply_battery.name = battery_dev->name;
    if(0 != power_supply_register(&battery_dev->dev, &supply_battery))
    {
        power_supply_unregister(&supply_battery);
        platform_device_unregister(battery_dev);
        ret = -1;
    }

#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&battery_android_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */

    printk("++ urbest Battery driver init.---2\n");
	
       #if 1
    	//denis_wei add for read batter info 2011-01-05

	root_entry = proc_mkdir(PROC_NAME, &proc_root);
	if(root_entry)
	{
		//try = create_proc_entry(s_nods[i], 0666, root_entry);
		entry = create_proc_entry("battery_info" ,0666, root_entry);
		if(entry)
		{
			entry->write_proc = battery_proc_write;
			entry->read_proc =  battery_proc_read;
			//try->data = (void*)s_nods[i];	
			entry->data = (void*)0;	
		}
	}
	//add end  2011-01-05
	#endif

	check_model();
	
#if 0  // todo for future more mid style,set the value of s_model
	if(!strcmp(g_selected_manufacturer, "coby"))
		{
			s_model = 7024;
			g_no_ac = 1;
		} 
#endif


	//INIT_WORK(&battery_pen_event_work, battert_work);
	INIT_DELAYED_WORK(&battery_pen_event_work, battert_work);

#if 0 // for debug s3c_adc_get_adc_data()
	INIT_WORK(&delay_open_micbias, delay_micbias_open);
	schedule_work(&delay_open_micbias);
#endif

	battery_workqueue = create_singlethread_workqueue("battery");

	battery_detect(0);
	
    return 0;
}

static void __exit battery_exit(void)
{
    //cancel_work_sync(&battery_pen_event_work);
    cancel_delayed_work_sync(&battery_pen_event_work);

    power_supply_unregister(&supply_battery);
    power_supply_unregister(&supply_ac);
    platform_device_unregister(battery_dev);
}

module_init(battery_init);
module_exit(battery_exit);

MODULE_AUTHOR("Urbest inc.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Urbest battery");


