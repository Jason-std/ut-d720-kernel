
/*
 *
 * battery detect driver for the samsung 
 *
 */
#if 1
#include <linux/syscalls.h>
#include <linux/fs.h>


#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/jiffies.h>
#include <linux/sched.h>

#include <linux/pm.h>

#include <plat/adc.h>

#include<plat/regs-adc.h>     
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/clk.h>
#include <mach/irqs.h>
#include<mach/map.h>

#include "power_gpio.h"
#include <linux/proc_fs.h>

#include <linux/delay.h>
#include <linux/time.h>
#include <linux/timer.h>

#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#endif
#include <plat/gpio-cfg.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <plat/adc.h>
#endif
#define CONFIG_BATTERY_VOL3V8

#define   dbug_on  ;

static int samsung_battery_dbg_level = 0;

bool       ut_battery_init = false;
EXPORT_SYMBOL(ut_battery_init);

int        axp_capacity;
EXPORT_SYMBOL(axp_capacity);





//extern  void smdk5250_power_off(void) ;   

module_param_named(dbg_level, samsung_battery_dbg_level, int, 0644);
#define pr_bat( args...) \
	do { \
		if (samsung_battery_dbg_level) { \
			pr_info(args); \
		} \
	} while (0)

//#define    BATTERY_TEST		
#define    TIME_COVEN                                            60*1000
#define	TIMER_TEST_COUNTS		               TIME_COVEN//1000	//¶¨Ê±Æ÷µÄ³¤¶È·Ö



#define	TIMER_MS_COUNTS		               500//1000	//¶¨Ê±Æ÷µÄ³¤¶Èms
#define	SLOPE_SECOND_COUNTS	               15	//Í³¼ÆµçÑ¹Ð±ÂÊµÄÊ±¼ä¼ä¸ôs
#define	DISCHARGE_MIN_SECOND	               45	//×î¿ì·Åµçµç1%Ê±¼ä
#define	CHARGE_MIN_SECOND	                       45	//×î¿ì³äµçµç1%Ê±¼ä
#define	CHARGE_MID_SECOND	                       90	//ÆÕÍ¨³äµçµç1%Ê±¼ä
#define	CHARGE_MAX_SECOND	                       250	//×î³¤³äµçµç1%Ê±¼ä
#define   CHARGE_FULL_DELAY_TIMES             10          //³äµçÂú¼ì²â·À¶¶Ê±¼ä

#define	NUM_VOLTAGE_SAMPLE	                       ((SLOPE_SECOND_COUNTS * 1000) / TIMER_MS_COUNTS)	 
#define	NUM_DISCHARGE_MIN_SAMPLE	         ((DISCHARGE_MIN_SECOND * 1000) / TIMER_MS_COUNTS)	 
#define	NUM_CHARGE_MIN_SAMPLE	         ((CHARGE_MIN_SECOND * 1000) / TIMER_MS_COUNTS)	    
#define	NUM_CHARGE_MID_SAMPLE	         ((CHARGE_MID_SECOND * 1000) / TIMER_MS_COUNTS)	     
#define	NUM_CHARGE_MAX_SAMPLE	         ((CHARGE_MAX_SECOND * 1000) / TIMER_MS_COUNTS)	  
#define    NUM_CHARGE_FULL_DELAY_TIMES         ((CHARGE_FULL_DELAY_TIMES * 1000) / TIMER_MS_COUNTS)	

//#define BAT_2V5_VALUE	                                     2500

#define BATT_FILENAME "/system/etc/bat_last_capacity.dat"
static struct wake_lock batt_wake_lock;

struct batt_vol_cal{
	u32 disp_cal;
	u32 dis_charge_vol;
	u32 charge_vol;
};

struct samsung_adc_battery_platform_data {
        int (*io_init)(void);
        int (*io_deinit)(void);

        int dc_det_pin;
        int batt_low_pin;
        int charge_ok_pin;
        int charge_set_pin;

        int dc_det_level;
        int batt_low_level;
        int charge_ok_level;
        int charge_set_level;
};

static struct s3c_adc_client	*client = NULL;
static int ac_property = 0;
unsigned long AdcTestCnt = 0;

extern char ut_model[];    

unsigned int  capacity_updata;

#define BAT_DEFAULT            1
#define BAT_W122P              2
#define BAT_DDM                3
#define BAT_EUP8092            4
#define BAT_AUG646             5

//#define model_710              


#ifdef model_710
//static int s_model = 1101;
static int s_model = 710;
//static int bat_model = BAT_DEFAULT;
static int bat_model = BAT_EUP8092;
 
static unsigned int BATT_MAX_VOL_VALUE      =   4020;    //
static unsigned int  BATT_ZERO_VOL_VALUE    =   3530;   //
//#define     BATT_NOMAL_VOL_VALUE                        3800               
//extern int g_tmu_powersave;
#else

static int s_model = 706;
static int bat_model = BAT_EUP8092;
static unsigned int BATT_MAX_VOL_VALUE      =   4020;    //
static unsigned int  BATT_ZERO_VOL_VALUE    =   3530;   //
//extern int g_tmu_powersave;
#endif


int g_tmu_powersave = 0;


#define TEM_DBG

#if 1
static struct batt_vol_cal  batt_table[] = {     //wky 3777228p  7200ma

    {0,3552, 3932},
    {9,3568,3954},
    {15,3585, 3986}, 
    {20,3618, 4010},
    {30,3663, 4030},
    {45,3720, 4050},
    {60,3780, 4100},
    {77,3820, 4150},
    {90,3900, 4200},
    {100,4020, 4212},
     
};

static struct batt_vol_cal  batt_table_s710[] = {     //BAT_EUP8092
    {0,3473, 3620},
    {10,3528, 3700},
    {20,3570,3886},
    {30,3604, 3907}, 
    {40,3626, 3945},
    {50,3659, 3971},
    {60,3690, 4002},
    {70,3737, 4031},
    {80,3801, 4088},
    {90,3871, 4146},
    {100,3967, 4174},
};

static struct batt_vol_cal  batt_table_s706[] = {     //BAT_EUP8092
    {0,3415, 3500},
    {10,3506, 3627},
    {20,3615,3754},
    {30,3654, 3801}, 
    {40,3700, 3836},
    {50,3730, 3855},
    {60,3760, 3885},
    {70,3803, 3925},
    {80,3847, 3982},
    {90,3914, 4070},
    {100,4038, 4194},
};

static struct batt_vol_cal  batt_table_d720[] = {     //BAT_AUG646
	{0,3378,3460},	
	{5,3432,3662},	
	{14,3498,3720}, 
	{24,3542,3762}, 
	{34,3574,3777}, 
	{43,3620,3820}, 
	{53,3642,3844}, 
	{62,3709,3900}, 
	{72,3775,3984}, 
	{82,3850,4062}, 
	{91,3935,4142}, 
 	{100,3960,4160},	
}; 


//add for s902  eking 20130723

static struct batt_vol_cal  batt_table902[] = {  

    {0,3550,3670},//0.09  
	{10,3656,3819},
	//{20,3630,3831},
	{20,3703,3849},
	{30,3752,3876},
	{40,3787,3918},
	{50,3820,3960},
	{60,3842,4008},
	{70,3889,4050},
	{80,3900,4086},
	{90,3913,4128},
	{100,4055,4205},
};
static struct batt_vol_cal  batt_table_ddm[] = {  

        {0,  3550,3670},
	{10,3645,3819},
	//{20,3630,3831},
	{20,3717,3849},
	{30,3738,3876},
	{40,3774,3918},
	{50,3816,3960},
	{60,3849,4008},
	{70,3865,4050},
	{80,3921,4086},
	{90,4047,4128},
	{100,4100,4224},
};

#endif

#if  0    

static struct batt_vol_cal  batt_table2[] = {

};

#endif

//static unsigned int BATT_NUM=ARRAY_SIZE(batt_table);  

#define  ADC_CHANL						     0x00
#define  BATTERY_ADC_VREF	    	                     1800
#define BATTERY_ADC_12BIT_MAX    	             0xFFF
#define adc_to_voltage(adc_val)           (adc_val * BATTERY_ADC_VREF  / BATTERY_ADC_12BIT_MAX )* 3


/********************************************************************************/
struct samsung_adc_battery_data {
	int irq;
	
	struct workqueue_struct *wq;
	struct delayed_work 	    delay_work;
	struct work_struct 	    dcwakeup_work;
	struct work_struct                   lowerpower_work;
	bool                    resume;
	int                     full_times;
	
	struct     s3c_adc_client       *client; 
	int                     adc_val;
	int                     adc_samples[NUM_VOLTAGE_SAMPLE+2];
	
	int                     bat_status;
	int                     bat_status_cnt;
	int                     bat_health;
	int                     bat_present;
	int                     bat_voltage;
	int                     bat_capacity;
	int                     bat_change;
	
	int                     old_charge_level;
	int                    *pSamples;
	int                     gBatCapacityDisChargeCnt;
	int                     gBatCapacityChargeCnt;
	int 	          capacitytmp;
	int                     poweron_check;
	int                     suspend_capacity;
	int                     status_lock;

};
static struct samsung_adc_battery_data *gBatteryData;

enum {
	BATTERY_STATUS          = 0,
	BATTERY_HEALTH          = 1,
	BATTERY_PRESENT         = 2,
	BATTERY_CAPACITY        = 3,
	BATTERY_AC_ONLINE       = 4,
	BATTERY_STATUS_CHANGED	= 5,
	AC_STATUS_CHANGED   	= 6,
	BATTERY_INT_STATUS	    = 7,
	BATTERY_INT_ENABLE	    = 8,
};

typedef enum {
	CHARGER_BATTERY = 0,
	CHARGER_USB,
	CHARGER_AC
} charger_type_t;

static int samsung_adc_battery_load_capacity(void)
{
     return 0;

}

static void samsung_adc_battery_put_capacity(int loadcapacity)
{
      return ;

}


static int samsung_adc_battery_get_charge_level(struct samsung_adc_battery_data *bat)
{
	int charge_on = 0;
	//printk("**11**POWER_STATE_AC = %d\n",read_power_item_value(POWER_STATE_AC));
		if ( read_power_item_value(POWER_STATE_AC)){ 
			//printk("**22**POWER_STATE_AC = %d\n",read_power_item_value(POWER_STATE_AC));
			charge_on = 1;
		}

	return charge_on;
}

static int samsung_adc_battery_status_samples(struct samsung_adc_battery_data *bat)
{
	int charge_level;
	charge_level = samsung_adc_battery_get_charge_level(bat);

	if (charge_level != bat->old_charge_level){
		bat->old_charge_level = charge_level;
		bat->bat_change  = 1;

		bat->bat_status_cnt = 0;   
	}

	if(charge_level == 0){   
		bat->full_times = 0;
		bat->bat_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
	}
	else{
		if (read_power_item_value(POWER_STATE_CHARGE)){  

			if (bat->bat_capacity == 100){
				if (bat->bat_status != POWER_SUPPLY_STATUS_FULL){
					bat->bat_status = POWER_SUPPLY_STATUS_FULL;
					bat->bat_change  = 1;
				}
			}
			else{
//add by eking  20130701				
				   if(bat->bat_capacity >=99)
				   	{
	                          //	 bat->bat_capacity = 100;
				 	 bat->bat_status = POWER_SUPPLY_STATUS_FULL;	
					 bat->bat_change  = 1;
				   	}
			         else
//add end				   
				     bat->bat_status = POWER_SUPPLY_STATUS_CHARGING;
				
			            }
		}
		else{                                                                                                 
			if (!read_power_item_value(POWER_STATE_CHARGE)){
                                
				bat->full_times = 0;
				bat->bat_status = POWER_SUPPLY_STATUS_CHARGING;
			}
			else{
				bat->full_times++;

				if (bat->full_times >= NUM_CHARGE_FULL_DELAY_TIMES) {
					bat->full_times = NUM_CHARGE_FULL_DELAY_TIMES + 1;
				}

				if ((bat->full_times >= NUM_CHARGE_FULL_DELAY_TIMES) && (bat->bat_capacity >= 99)){
					if (bat->bat_status != POWER_SUPPLY_STATUS_FULL){
						bat->bat_status = POWER_SUPPLY_STATUS_FULL;
						bat->bat_capacity = 100;
						bat->bat_change  = 1;
					}
				}
				else{
					bat->bat_status = POWER_SUPPLY_STATUS_CHARGING;
				}
			}
		}
	}

	return charge_level;
}

static int *pSamples;

static void samsung_adc_battery_voltage_samples(struct samsung_adc_battery_data *bat)
{
	int value;
	int i,*pStart = bat->adc_samples, num = 0;
	//int level = !(samsung_adc_battery_get_charge_level(bat));//*
    int level = samsung_adc_battery_get_charge_level(bat);//*

	//printk("****charge_status == %d\n",level);//charger in/out
        int BATT_NUM;
	bat->adc_val=s3c_adc_read(client, ADC_CHANL); 
	value = bat->adc_val;
	*pSamples++ = adc_to_voltage(value);

	bat->bat_status_cnt++;
	if (bat->bat_status_cnt > NUM_VOLTAGE_SAMPLE)  bat->bat_status_cnt = NUM_VOLTAGE_SAMPLE + 1;

	num = pSamples - pStart;
	
	if (num >= NUM_VOLTAGE_SAMPLE){
		pSamples = pStart;
		num = NUM_VOLTAGE_SAMPLE;
		
	}

	value = 0;
	for (i = 0; i < num; i++){
		value += bat->adc_samples[i];//
	}
	bat->bat_voltage = value / num;
	//printk("\n---%s--bat->bat_voltage= %d---\n",__func__,bat->bat_voltage);///adc val
	if(s_model == 1101 && bat_model == BAT_DEFAULT)  
	{
	       //printk("***smod----1101  BAT_DEFAULT\n");

        	BATT_NUM=ARRAY_SIZE(batt_table);
	
	        if(1 == level){//**charge
			if(bat->bat_voltage >= batt_table[BATT_NUM-1].charge_vol+ 10)
		       		bat->bat_voltage = batt_table[BATT_NUM-1].charge_vol  + 10;
			else if(bat->bat_voltage <= batt_table[0].charge_vol  - 10)
				bat->bat_voltage =  batt_table[0].charge_vol - 10;
			}
			else{// discharge*
				if(bat->bat_voltage >= batt_table[BATT_NUM-1].dis_charge_vol+ 5)
					bat->bat_voltage = batt_table[BATT_NUM-1].dis_charge_vol  + 5;
				else if(bat->bat_voltage <= batt_table[0].dis_charge_vol  - 5)
					bat->bat_voltage =  batt_table[0].dis_charge_vol - 5;
				//printk("---%s---%d\n",__func__,__LINE__);
			}
	}
   else if(s_model == 710 && bat_model == BAT_EUP8092) 
   {
	       //printk("***smod----s710  BAT_EUP8092\n");

        	BATT_NUM=ARRAY_SIZE(batt_table_s710);
	
	        if(1 == level){//**charge
			if(bat->bat_voltage >= batt_table_s710[BATT_NUM-1].charge_vol+ 10)
		       		bat->bat_voltage = batt_table_s710[BATT_NUM-1].charge_vol  + 10;
			else if(bat->bat_voltage <= batt_table_s710[0].charge_vol  - 10)
				bat->bat_voltage =  batt_table_s710[0].charge_vol - 10;
			}
			else{// discharge*
				if(bat->bat_voltage >= batt_table_s710[BATT_NUM-1].dis_charge_vol+ 10)
					bat->bat_voltage = batt_table_s710[BATT_NUM-1].dis_charge_vol  + 10;
				else if(bat->bat_voltage <= batt_table_s710[0].dis_charge_vol  - 10)
					bat->bat_voltage =  batt_table_s710[0].dis_charge_vol - 10;
				//printk("---%s---%d\n",__func__,__LINE__);
			}


   }
      else if(s_model == 706 && bat_model == BAT_EUP8092) 
   {
	      //printk("***smod----s706  BAT_EUP8092\n");

        	BATT_NUM=ARRAY_SIZE(batt_table_s706);
	
	        if(1 == level){//**charge
			if(bat->bat_voltage >= batt_table_s706[BATT_NUM-1].charge_vol+ 10)
		       		bat->bat_voltage = batt_table_s706[BATT_NUM-1].charge_vol  + 10;
			else if(bat->bat_voltage <= batt_table_s706[0].charge_vol  - 10)
				bat->bat_voltage =  batt_table_s706[0].charge_vol - 10;
			}
			else{// discharge*
				if(bat->bat_voltage >= batt_table_s706[BATT_NUM-1].dis_charge_vol+ 10)
					bat->bat_voltage = batt_table_s706[BATT_NUM-1].dis_charge_vol  + 10;
				else if(bat->bat_voltage <= batt_table_s706[0].dis_charge_vol  - 10)
					bat->bat_voltage =  batt_table_s706[0].dis_charge_vol - 10;
				//printk("---%s---%d\n",__func__,__LINE__);
			}


   }


	   else if(s_model == 720 && bat_model == BAT_AUG646) 
	{
		  // printk("***smod----720  BAT_AUG646\n");
	
			 BATT_NUM=ARRAY_SIZE(batt_table_d720);
	 
			 if(1 == level){//**charge
			 if(bat->bat_voltage >= batt_table_d720[BATT_NUM-1].charge_vol+ 10)
					 bat->bat_voltage = batt_table_d720[BATT_NUM-1].charge_vol	+ 10;
			 else if(bat->bat_voltage <= batt_table_d720[0].charge_vol	- 10)
				 bat->bat_voltage =  batt_table_d720[0].charge_vol - 10;
			 }
			 else{// discharge*
				 if(bat->bat_voltage >= batt_table_d720[BATT_NUM-1].dis_charge_vol+ 10)
					 bat->bat_voltage = batt_table_d720[BATT_NUM-1].dis_charge_vol	+ 10;
				 else if(bat->bat_voltage <= batt_table_d720[0].dis_charge_vol	- 10)
					 bat->bat_voltage =  batt_table_d720[0].dis_charge_vol - 10;
				 //printk("---%s---%d\n",__func__,__LINE__);
			 }
	
	
	}

	else if(s_model == 902 && bat_model == BAT_W122P)  
	{

        	BATT_NUM=ARRAY_SIZE(batt_table902);
		if(1 == level){	
			if(bat->bat_voltage >= batt_table902[BATT_NUM-1].charge_vol+ 10)
		      		 bat->bat_voltage = batt_table902[BATT_NUM-1].charge_vol  + 10;
			else if(bat->bat_voltage <= batt_table902[0].charge_vol  - 10)
				bat->bat_voltage =  batt_table902[0].charge_vol - 10;
			
		}
		else{
			
			if(bat->bat_voltage >= batt_table902[BATT_NUM-1].dis_charge_vol+ 10)
				bat->bat_voltage = batt_table902[BATT_NUM-1].dis_charge_vol  + 10;
			else if(bat->bat_voltage <= batt_table902[0].dis_charge_vol  - 10)
				bat->bat_voltage =  batt_table902[0].dis_charge_vol - 10;
			//printk("---%s---%d\n",__func__,__LINE__);
		}
	}
       else if(s_model == 1101 && bat_model == BAT_DDM)
	   {

        	BATT_NUM=ARRAY_SIZE(batt_table);
	
	        if(1 == level){
			if(bat->bat_voltage >= batt_table_ddm[BATT_NUM-1].charge_vol+ 10)
		       		bat->bat_voltage = batt_table_ddm[BATT_NUM-1].charge_vol  + 10;
			else if(bat->bat_voltage <= batt_table_ddm[0].charge_vol  - 10)
				bat->bat_voltage =  batt_table_ddm[0].charge_vol - 10;
		}
		else{

			if(bat->bat_voltage >= batt_table_ddm[BATT_NUM-1].dis_charge_vol+ 5)
				bat->bat_voltage = batt_table_ddm[BATT_NUM-1].dis_charge_vol  + 5;
			else if(bat->bat_voltage <= batt_table_ddm[0].dis_charge_vol  - 5)
				bat->bat_voltage =  batt_table_ddm[0].dis_charge_vol - 5;
			//printk("---%s---%d\n",__func__,__LINE__);
		}
	}
	
	else{

		BATT_NUM=ARRAY_SIZE(batt_table);
	//printk("---%s--bat->bat_voltage= %d---\n",__func__,bat->bat_voltage);
		if(1 == level){
			if(bat->bat_voltage >= batt_table[BATT_NUM-1].charge_vol+ 10)
		      		 bat->bat_voltage = batt_table[BATT_NUM-1].charge_vol  + 10;
			else if(bat->bat_voltage <= batt_table[0].charge_vol  - 10)
				bat->bat_voltage =  batt_table[0].charge_vol - 10;
			//printk("---%s---%d\n",__func__,__LINE__);
		}
		else{
			if(bat->bat_voltage >= batt_table[BATT_NUM-1].dis_charge_vol+ 10)
				bat->bat_voltage = batt_table[BATT_NUM-1].dis_charge_vol  + 10;
			else if(bat->bat_voltage <= batt_table[0].dis_charge_vol  - 10)
				bat->bat_voltage =  batt_table[0].dis_charge_vol - 10;
			//printk("---%s---%d\n",__func__,__LINE__);
		}
	}
  
}

/**
* get capacity  
* update struct batt_vol_cal{}
*/
static int samsung_adc_battery_voltage_to_capacity(struct samsung_adc_battery_data *bat, int BatVoltage)
{
	int i = 0;
	int capacity = 0;
	struct batt_vol_cal *p;
        int BATT_NUM;
		
	if(s_model == 1101 && bat_model == BAT_DEFAULT)  	
		{
		p = batt_table;
		BATT_NUM=ARRAY_SIZE(batt_table);
		//printk("---%s---%d\n",__func__,__LINE__);
		}
	else if(s_model == 710 && bat_model == BAT_EUP8092)  
		{
			p = batt_table_s710;
			BATT_NUM=ARRAY_SIZE(batt_table_s710);
		    //printk("---%s---%d\n",__func__,__LINE__);
		}
	else if(s_model == 706 && bat_model == BAT_EUP8092)  
		{
			p = batt_table_s706;
			BATT_NUM=ARRAY_SIZE(batt_table_s706);
			//printk("***batt_table_s706--%s---%d\n",__func__,__LINE__);
		}
	else if(s_model == 720 && bat_model == BAT_AUG646)  
		{
			p = batt_table_d720;
			BATT_NUM=ARRAY_SIZE(batt_table_d720);
			//printk("***batt_table_s706--%s---%d\n",__func__,__LINE__);
		}


	else if(s_model == 902 && bat_model == BAT_W122P)  
		{
		p = batt_table902;
		BATT_NUM=ARRAY_SIZE(batt_table902);
	//	printk("---%s---%d\n",__func__,__LINE__);
		}
      	else if(s_model == 1101&& bat_model == BAT_DDM)  
		{
		p = batt_table_ddm;
		BATT_NUM=ARRAY_SIZE(batt_table_ddm);
	//	printk("---%s---%d\n",__func__,__LINE__);
		}
	
        else
        	{
        	p = batt_table;
		BATT_NUM=ARRAY_SIZE(batt_table);
		//printk("---%s---%d\n",__func__,__LINE__);
        	}
	if (samsung_adc_battery_get_charge_level(bat)){  //** charge    
		if(BatVoltage >= (p[BATT_NUM - 1].charge_vol)){//full charging
			capacity = 100;
		}	
		else{
			if(BatVoltage <= (p[0].charge_vol)){//empty battery 
				capacity = 0;
			}
			else{
				for(i = 0; i < BATT_NUM - 1; i++){

					if(((p[i].charge_vol) <= BatVoltage) && (BatVoltage < (p[i+1].charge_vol))){
						capacity = p[i].disp_cal + ((BatVoltage - p[i].charge_vol) *  (p[i+1].disp_cal -p[i].disp_cal ))/ (p[i+1].charge_vol- p[i].charge_vol);
					//printk("********charge____capacity= %d******************\n",capacity);
						break;
					}
				}
			}  
		}

	}
	else{  //discharge
		if(BatVoltage >= (p[BATT_NUM - 1].dis_charge_vol)){//full charging
			capacity = 100;
		}	
		else{//empty battery 
			if(BatVoltage <= (p[0].dis_charge_vol)){
				capacity = 0;
			}
			else{
				for(i = 0; i < BATT_NUM - 1; i++){
					if(((p[i].dis_charge_vol) <= BatVoltage) && (BatVoltage < (p[i+1].dis_charge_vol))){
						capacity =  p[i].disp_cal+ ((BatVoltage - p[i].dis_charge_vol) * (p[i+1].disp_cal -p[i].disp_cal ) )/ (p[i+1].dis_charge_vol- p[i].dis_charge_vol) ;
						//printk("********dis_charge____capacity= %d******************\n",capacity);
						break;
					}
				}
			}  

		}


	}
     //printk("*********samsung_adc_battery_voltage_to_capacity  capacity=%d*************\n",capacity);
	 axp_capacity = capacity; //send a val to axp 
    return capacity;
}

static void samsung_adc_battery_capacity_samples(struct samsung_adc_battery_data *bat)
{
	int capacity = 0;
	
	if (bat->bat_status_cnt < NUM_VOLTAGE_SAMPLE)  {
		bat->gBatCapacityDisChargeCnt = 0;
		bat->gBatCapacityChargeCnt    = 0;
		return;
	}
	//printk("bat->bat_voltage =%d\n",bat->bat_voltage);
	capacity = samsung_adc_battery_voltage_to_capacity(bat, bat->bat_voltage);
	if (samsung_adc_battery_get_charge_level(bat)){//** = =1 chargeå
		if (capacity > bat->bat_capacity){
			//
			if (++(bat->gBatCapacityDisChargeCnt) >= NUM_CHARGE_MIN_SAMPLE){
				bat->gBatCapacityDisChargeCnt  = 0;
				if (bat->bat_capacity < 99){
					bat->bat_capacity++;
					bat->bat_change  = 1;
				}
			}
			bat->gBatCapacityChargeCnt = 0;
		}
		else{  //   
		            bat->gBatCapacityDisChargeCnt = 0;
		            (bat->gBatCapacityChargeCnt)++;
            
				if (read_power_item_value(POWER_STATE_CHARGE)){
				
					if (bat->gBatCapacityChargeCnt >= NUM_CHARGE_MIN_SAMPLE){
						bat->gBatCapacityChargeCnt = 0;
						if (bat->bat_capacity < 99){
							bat->bat_capacity++;
							bat->bat_change  = 1;
						}
					}
				else{

					if (capacity > bat->capacitytmp){
					
						bat->gBatCapacityChargeCnt = 0;
					}
					else{

						if ((bat->bat_capacity >= 85) &&((bat->gBatCapacityChargeCnt) > NUM_CHARGE_MAX_SAMPLE)){
							bat->gBatCapacityChargeCnt = (NUM_CHARGE_MAX_SAMPLE - NUM_CHARGE_MID_SAMPLE);

							if (bat->bat_capacity < 99){
								bat->bat_capacity++;
								bat->bat_change  = 1;
							}
						}
					}
				}
			}
			else{
			
				if (capacity > bat->capacitytmp){
					bat->gBatCapacityChargeCnt = 0;
				}
				else{

					if ((bat->bat_capacity >= 85) &&(bat->gBatCapacityChargeCnt > NUM_CHARGE_MAX_SAMPLE)){
						bat->gBatCapacityChargeCnt = (NUM_CHARGE_MAX_SAMPLE - NUM_CHARGE_MID_SAMPLE);

						if (bat->bat_capacity < 99){
							bat->bat_capacity++;
							bat->bat_change  = 1;
						}
					}
				}
				

			}            
		}
	}    
	else{   
		if (capacity < bat->bat_capacity){
			if (++(bat->gBatCapacityDisChargeCnt) >= NUM_DISCHARGE_MIN_SAMPLE){
				bat->gBatCapacityDisChargeCnt = 0;
				if (bat->bat_capacity > 0){
					bat->bat_capacity-- ;
					bat->bat_change  = 1;
				}
			}
		}
		else{
			bat->gBatCapacityDisChargeCnt = 0;
		}
		bat->gBatCapacityChargeCnt = 0;
	}
		bat->capacitytmp = capacity;
		//printk("*********samsung_adc_battery_capacity_samples  capacity=%d*************\n",capacity);

}

//static int poweron_check = 0;

static void samsung_adc_battery_poweron_capacity_check(void)
{

	int new_capacity, old_capacity;
	new_capacity = gBatteryData->bat_capacity;
	old_capacity = samsung_adc_battery_load_capacity();
	
	//printk("*********old_capacity=%d************************\n",old_capacity);
	//printk("*********new_capacity=%d************************\n",new_capacity);
	if ((old_capacity <= 0) || (old_capacity >= 100)){
		old_capacity = new_capacity;
	}    

	if (gBatteryData->bat_status == POWER_SUPPLY_STATUS_FULL){
		if (new_capacity > 80){
			gBatteryData->bat_capacity = 100;
		}
	}
	else if (gBatteryData->bat_status != POWER_SUPPLY_STATUS_NOT_CHARGING){
	
	        gBatteryData->bat_capacity = new_capacity;
		gBatteryData->bat_capacity = (new_capacity > old_capacity) ? new_capacity : old_capacity;
	}else{

		if(new_capacity > old_capacity + 50 )
			{
			gBatteryData->bat_capacity = new_capacity;
			printk("****++++++****gBatteryData->bat_capacity=%d***************\n",new_capacity);
			}
		else
			gBatteryData->bat_capacity = (new_capacity < old_capacity) ? new_capacity : old_capacity;  //avoid the value of capacity increase 
	}

	gBatteryData->bat_change = 1;
	//printk("*********samsung_adc_battery_poweron_capacity_check  capacity=%d*************\n",gBatteryData->bat_capacity);

}

/*

static int samsung_adc_battery_get_ac_property(struct power_supply *psy,
			enum power_supply_property psp,
			union power_supply_propval *val)
{
	int ret = 0;
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (psy->type == POWER_SUPPLY_TYPE_MAINS)
		{
			if (samsung_adc_battery_get_charge_level(gBatteryData))
			{
				val->intval = 1;
				ac_property= val->intval ;
				}
			else
				{
				val->intval = 0;	
				ac_property= val->intval ;
				}
		}
		//printk("%s:%d\n",__FUNCTION__,val->intval);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}


static enum power_supply_property samsung_adc_battery_ac_props[] = 
{
	POWER_SUPPLY_PROP_ONLINE,
};

static struct power_supply samsung_ac_supply = 
{
	.name = "ac",//"ut_ac",//"ac",
	.type = POWER_SUPPLY_TYPE_MAINS,

	.get_property   = samsung_adc_battery_get_ac_property,

	.properties     = samsung_adc_battery_ac_props,
	.num_properties = ARRAY_SIZE(samsung_adc_battery_ac_props),
};
*/
static int samsung_adc_battery_get_status(struct samsung_adc_battery_data *bat)
{

//add by eking 20130701
	if(samsung_adc_battery_get_charge_level(bat) &&( bat->bat_capacity >=99))
		if( read_power_item_value(POWER_STATE_CHARGE))
		           bat->bat_status = POWER_SUPPLY_STATUS_FULL;
//add end		
	return (bat->bat_status);
}

static int samsung_adc_battery_get_health(struct samsung_adc_battery_data *bat)
{
	return POWER_SUPPLY_HEALTH_GOOD;
}

static int samsung_adc_battery_get_present(struct samsung_adc_battery_data *bat)
{
 
	return (bat->bat_voltage < BATT_MAX_VOL_VALUE) ? 0 : 1;
}

static int samsung_adc_battery_get_voltage(struct samsung_adc_battery_data *bat)
{
	return (bat->bat_voltage );
}

static int samsung_adc_battery_get_capacity(struct samsung_adc_battery_data *bat)
{
//add by eking 20130701
        if( bat->bat_status == POWER_SUPPLY_STATUS_FULL)
               {
               if(bat->bat_capacity >=99)
		       bat->bat_capacity =100;
        	}
		
	else if(bat->bat_status == POWER_SUPPLY_STATUS_CHARGING)
		{
		if(bat->bat_capacity >=99) 
		       bat->bat_capacity =99;
		}
	//printk("*********samsung_adc_battery_get_capacity  capacity=%d*************\n", bat->bat_capacity);
//add end		
	return (bat->bat_capacity);
}



/*
static int samsung_adc_battery_get_property(struct power_supply *psy,
				 enum power_supply_property psp,
				 union power_supply_propval *val)
{		
	int ret = 0;   
	switch (psp) {
		case POWER_SUPPLY_PROP_STATUS:
			val->intval = samsung_adc_battery_get_status(gBatteryData);
			
			break;
		case POWER_SUPPLY_PROP_HEALTH:
			val->intval = samsung_adc_battery_get_health(gBatteryData);
			
			break;
		case POWER_SUPPLY_PROP_PRESENT:
			val->intval = samsung_adc_battery_get_present(gBatteryData);
			
			break;
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			val ->intval = samsung_adc_battery_get_voltage(gBatteryData)*1000;
			
			break;

		case POWER_SUPPLY_PROP_CAPACITY:
			val->intval = samsung_adc_battery_get_capacity(gBatteryData);

			break;
		case POWER_SUPPLY_PROP_TECHNOLOGY:
			val->intval = POWER_SUPPLY_TECHNOLOGY_LION;	
			break;
		case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
			val->intval = BATT_MAX_VOL_VALUE;
			break;
		case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
			val->intval = BATT_ZERO_VOL_VALUE;
			break;
		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}
*/
/*

static enum power_supply_property samsung_adc_battery_props[] = {

	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,

	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
};
static struct power_supply samsung_battery_supply = 
{
	.name = "battery",//"ut_battery"
	.type = POWER_SUPPLY_TYPE_BATTERY,

	.get_property   = samsung_adc_battery_get_property,

	.properties     = samsung_adc_battery_props,
	.num_properties = ARRAY_SIZE(samsung_adc_battery_props),
};

*/
static void samsung_adc_battery_resume_check(void)
{
	int i;
	int level,oldlevel;
	int new_capacity, old_capacity;
	struct samsung_adc_battery_data *bat = gBatteryData;
		 	
	bat->old_charge_level = -1;
	pSamples = bat->adc_samples;

	s3c_adc_read(client, ADC_CHANL);                                         //start adc sample
	level = oldlevel = samsung_adc_battery_status_samples(bat);//init charge status

	for (i = 0; i < NUM_VOLTAGE_SAMPLE; i++) {                     
	
		mdelay(1);
		samsung_adc_battery_voltage_samples(bat);                  //get voltage
		level = samsung_adc_battery_status_samples(bat);        //check charge status
		if (oldlevel != level){		
		    oldlevel = level;                                                               //if charge status changed, reset sample
		    i = 0;
		}        
	}
	new_capacity = samsung_adc_battery_voltage_to_capacity(bat, bat->bat_voltage);
	old_capacity =gBatteryData-> suspend_capacity;

	if (bat->bat_status != POWER_SUPPLY_STATUS_NOT_CHARGING){
	//chargeing state
		bat->bat_capacity = (new_capacity > old_capacity) ? new_capacity : old_capacity;
	}
	else{
		bat->bat_capacity = (new_capacity < old_capacity) ? new_capacity : old_capacity;  // aviod the value of capacity increase    dicharge
	}

}

static int samsung_adc_battery_suspend(struct platform_device *dev, pm_message_t state)
{
	
	gBatteryData->suspend_capacity = gBatteryData->bat_capacity;
	cancel_delayed_work(&gBatteryData->delay_work);
	return 0;
}

static int samsung_adc_battery_resume(struct platform_device *dev)
{
	gBatteryData->resume = true;
	queue_delayed_work(gBatteryData->wq, &gBatteryData->delay_work, msecs_to_jiffies(100));
	return 0;
}

static void samsung_adc_battery_timer_work(struct work_struct *work)
{      
	if (gBatteryData->resume) {
		samsung_adc_battery_resume_check();
		gBatteryData->resume = false;
	}
	samsung_adc_battery_status_samples(gBatteryData);

	if (gBatteryData->poweron_check){   
		gBatteryData->poweron_check = 0;
		samsung_adc_battery_poweron_capacity_check();
	}

	samsung_adc_battery_voltage_samples(gBatteryData);
	if(gBatteryData->bat_voltage  <= 3500 )
		{
		printk("lowerpower oFF\n");
// 		smdk5250_power_off(); // wake up the system	
		}
	samsung_adc_battery_capacity_samples(gBatteryData);
	
        
	if( 1 == samsung_adc_battery_get_charge_level(gBatteryData)){  // charge
		if(0 == gBatteryData->status_lock ){			
			wake_lock(&batt_wake_lock);  //lock
			gBatteryData->status_lock = 1; 
		}
	}
	else{
		if(1 == gBatteryData->status_lock ){			
			wake_unlock(&batt_wake_lock);  //unlock
			gBatteryData->status_lock = 0; 
		}

	}
	
	if(gBatteryData->bat_change){
		gBatteryData->bat_change = 0;
		samsung_adc_battery_put_capacity(gBatteryData->bat_capacity);
		//power_supply_changed(&samsung_battery_supply);
	}
       // if(ac_property !=samsung_adc_battery_get_charge_level(gBatteryData))
        //	{
    
       // 	power_supply_changed(&samsung_ac_supply);
		//	
       // 	}
	if (samsung_battery_dbg_level){
		if (++AdcTestCnt >= 2)
			{
			AdcTestCnt = 0;
			printk("Status = %d, RealAdcVal = %d, RealVol = %d,gBatVol = %d, gBatCap = %d, RealCapacity = %d, dischargecnt = %d, chargecnt = %d\n", 
			gBatteryData->bat_status, gBatteryData->adc_val, adc_to_voltage(gBatteryData->adc_val), 
			gBatteryData->bat_voltage, gBatteryData->bat_capacity, gBatteryData->capacitytmp, gBatteryData->gBatCapacityDisChargeCnt,gBatteryData-> gBatCapacityChargeCnt);

		}
	}
//add by eking for powersave  20130609
/* modified by Albert_lee 2013-06-09 begin */
#if 0
     if( gBatteryData->bat_capacity  < 35)
	 	g_tmu_powersave = 1;
     else 
	 	g_tmu_powersave = 0;
#endif	 
//      g_tmu_powersave =g_tmu_powersave |(battery_save << 0) ;
 /*Albert_lee 2013-06-09 end*/
//add end 
	queue_delayed_work(gBatteryData->wq, &gBatteryData->delay_work, msecs_to_jiffies(TIMER_MS_COUNTS));

}

static void samsung_adc_battery_check(struct samsung_adc_battery_data *bat)
{
	int i;
	int level,oldlevel;

//	printk("%s--%d:\n",__FUNCTION__,__LINE__);

	bat->old_charge_level = -1;
	bat->capacitytmp = 0;
	bat->suspend_capacity = 0;
	
	pSamples = bat->adc_samples;

	s3c_adc_read(client, ADC_CHANL);	                                       //start adc sample
	level = oldlevel = samsung_adc_battery_status_samples(bat);//init charge status

	bat->full_times = 0;
	for (i = 0; i < NUM_VOLTAGE_SAMPLE; i++){          
		mdelay(1);
		samsung_adc_battery_voltage_samples(bat);                  //get voltage
		level = samsung_adc_battery_get_charge_level(bat);

		if (oldlevel != level){
			oldlevel = level;                                                         //if charge status changed, reset sample
			i = 0;
		}        
	}

	bat->bat_capacity = samsung_adc_battery_voltage_to_capacity(bat, bat->bat_voltage);  //init bat_capacity

	
	bat->bat_status = POWER_SUPPLY_STATUS_NOT_CHARGING;//**
	if (samsung_adc_battery_get_charge_level(bat)){                   //
		bat->bat_status = POWER_SUPPLY_STATUS_CHARGING;
		if ( read_power_item_value(POWER_STATE_CHARGE)){
				bat->bat_status = POWER_SUPPLY_STATUS_FULL;
				bat->bat_capacity = 100;
		}
	}
#if 1
	samsung_adc_battery_poweron_capacity_check();
#endif
	gBatteryData->poweron_check = 0;
	if (bat->bat_capacity == 0) bat->bat_capacity = 1;

}

static void check_model(void)/////
{
	extern char g_selected_utmodel[];
    extern char g_selected_battery[];

	printk("g_selected_utmodel = %s  ,g_selected_battery = %s\n",g_selected_utmodel,g_selected_battery);
	if(strncmp(g_selected_utmodel, "s101", strlen("s101")) == 0)
	{
               s_model       =  1101;
	     	//printk("---%s---%d\n",__func__,__LINE__);	   
	} 
	else if(strncmp(g_selected_utmodel, "s710", strlen("s710")) == 0)	

	{
		//printk("g_selected_utmodel :s710\n");
		s_model       = 710 ;
	}
	else if(strncmp(g_selected_utmodel, "s706", strlen("s706")) == 0)	

	{
		printk("g_selected_utmodel :s706\n");
		s_model       = 706 ;
	}
	
	else if(strncmp(g_selected_utmodel, "d720", strlen("d720")) == 0)	

	{
		printk("g_selected_utmodel :d720\n");
		s_model       = 720 ;
	}


	else if(strncmp(g_selected_utmodel, "s902", strlen("s902")) == 0)	

	{
		s_model       =  902;
	}
	
       if(strncmp(g_selected_battery,"W122",strlen("W122")) == 0)
		{
			printk("BAT MODE :W122\n");
			bat_model = BAT_W122P;
			//printk("---%s---%d\n",__func__,__LINE__);
		}
	    else  if(strncmp(g_selected_battery,"EUP8092",strlen("EUP8092")) == 0)//***??
		{
			printk("BAT MODE :EUP8092\n");
			bat_model = BAT_EUP8092;
			//printk("---%s---%d\n",__func__,__LINE__);
		}
		
	    else  if(strncmp(g_selected_battery,"W678",strlen("W678")) == 0)//***??
		{
			printk("BAT MODE :EUP8092\n");
			bat_model = BAT_AUG646;
			//printk("---%s---%d\n",__func__,__LINE__);
		}

       else  if(strncmp(g_selected_battery,"119K",strlen("119K")) == 0)
		{
			printk("BAT MODE :default 101\n");
			bat_model = BAT_DEFAULT;
			//printk("---%s---%d\n",__func__,__LINE__);
		}
       else  if(strncmp(g_selected_battery,"BD2M",strlen("BD2M")) == 0)
		{
			printk("BAT MODE :default 101\n");
			bat_model = BAT_DDM;
			//printk("---%s---%d\n",__func__,__LINE__);
		}	   

       if( s_model   ==  902 &&bat_model ==BAT_W122P )
       	{
       		BATT_MAX_VOL_VALUE   = 4055;
		    BATT_ZERO_VOL_VALUE = 3550;	
       	}
	   	else if(s_model   ==  710 &&bat_model == BAT_EUP8092)
		{
 			BATT_MAX_VOL_VALUE   = 3967;
			BATT_ZERO_VOL_VALUE =  3473;

		}
		else if(s_model   ==  706 &&bat_model == BAT_EUP8092)
		{
 			BATT_MAX_VOL_VALUE   = 4200;
			BATT_ZERO_VOL_VALUE =  3415;
		}
		
		else if(s_model   ==  720 &&bat_model == BAT_AUG646)//***´ý²â
		{
			BATT_MAX_VOL_VALUE	 = 3960;
			BATT_ZERO_VOL_VALUE = 3378; 
		}

		else if(s_model   ==  1101 &&bat_model ==BAT_DDM)
		{
		 	BATT_MAX_VOL_VALUE   = 4100;
			BATT_ZERO_VOL_VALUE = 3550;
		}
#if 0
s_model 	  =  1101;
bat_model = BAT_DDM;
BATT_MAX_VOL_VALUE	 = 4100;
BATT_ZERO_VOL_VALUE = 3550;

#endif
}



static int samsung_adc_battery_probe(struct platform_device *pdev)
{
	int tmp = 0;
	struct samsung_adc_battery_data          *data;
    printk("***samsung_adc_battery probe~~\n");


	check_model();
	
	//return 0;
	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (data == NULL) {
		tmp = -ENOMEM;
		goto err_data_alloc_failed;
	}
	
	gBatteryData = data;
	platform_set_drvdata(pdev, data);
	memset(data->adc_samples, 0, sizeof(int)*(NUM_VOLTAGE_SAMPLE + 2));
	
	client = s3c_adc_register(pdev, NULL, NULL,0);  //pdata->adc_channel = ani0
	if(!client)
		goto err_adc_register_failed;   
	
	 platform_set_drvdata(pdev, client);  
	data->client  = client;
        data->adc_val = s3c_adc_read(client, ADC_CHANL);	

 #if 0// del for D720
 	if(0 != power_supply_register(&pdev->dev, &samsung_ac_supply))
	 {
		 power_supply_unregister(&samsung_ac_supply);
	 	tmp = -1;
		goto err_ac_failed;
 	}

	 samsung_battery_supply.name = pdev->name;
	 
         if(0 != power_supply_register(&pdev->dev, &samsung_battery_supply))
	{
		 power_supply_unregister(&samsung_battery_supply);
		tmp = -1;
		goto err_battery_failed;
	 }
#endif 
 	wake_lock_init(&batt_wake_lock, WAKE_LOCK_SUSPEND, "batt_lock");	

	data->wq = create_singlethread_workqueue("adc_battd");
	INIT_DELAYED_WORK(&data->delay_work, samsung_adc_battery_timer_work);
	samsung_adc_battery_check(data);	
	queue_delayed_work(data->wq, &data->delay_work, msecs_to_jiffies(TIMER_MS_COUNTS));
	printk(KERN_INFO "samsung_adc_battery: driver initialized\n");
	msleep(500);
	ut_battery_init = true;

	return 0;
	
err_ac_failed:
	//power_supply_unregister(&samsung_ac_supply);
err_battery_failed:
	//power_supply_unregister(&samsung_battery_supply);
err_adc_register_failed: 
err_data_alloc_failed:
	kfree(data);
	printk("samsung_adc_battery: error!\n");
	return tmp;
}

static int samsung_adc_battery_remove(struct platform_device *pdev)
{
	struct samsung_adc_battery_data *data = platform_get_drvdata(pdev);
	cancel_delayed_work(&gBatteryData->delay_work);	
	
	//power_supply_unregister(&samsung_ac_supply);
	//power_supply_unregister(&samsung_battery_supply);
	kfree(data);
	
	return 0;
}

static struct platform_device  samsung_adc_battery_dev={

	.name = "samsung-battery",
	.id = -1,
};

static struct platform_driver samsung_adc_battery_driver = {
	.probe		= samsung_adc_battery_probe,
	.remove		= samsung_adc_battery_remove,
	.suspend		= samsung_adc_battery_suspend,
	.resume		= samsung_adc_battery_resume,
	.driver = {
		.name = "samsung-battery",
		.owner	= THIS_MODULE,
	}
};

static int __init battery_init(void)
{
	 int ret = 0; 
    printk("*****urbest Battery device init.\n");
         ret = platform_device_register(&samsung_adc_battery_dev);
   	 if(ret)
    	{
    		printk("--%s--platform_device_register failed!\n",__func__);
      	        return ret;
    	}
    	printk("++ urbest Battery driver init----.\n");//
    	ret = platform_driver_register(&samsung_adc_battery_driver);
     	if(ret)
     	{
     		printk("--%s--platform_driver_register failed!\n",__func__);
        	return ret;
     	}
		
	return 0;
}
static void __exit battery_exit(void)
{

    	//power_supply_unregister(&samsung_battery_supply);
    	//power_supply_unregister(&samsung_ac_supply);
    	platform_device_unregister(&samsung_adc_battery_dev);
    	platform_driver_unregister(&samsung_adc_battery_driver);
		
//	return 0;
}

module_init(battery_init);
module_exit(battery_exit);
MODULE_DESCRIPTION("Battery detect driver for the samsung");
MODULE_AUTHOR("eking@...");
MODULE_LICENSE("GPL");
