#include <linux/types.h>
#include <linux/string.h>

#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#include <plat/devs.h>
#include <plat/ehci.h>
#include <plat/usbgadget.h>

#include <mach/regs-clock.h>

extern char g_selected_utmodel[];

/************ customer ************/
/*开机充电电流，uA*/
unsigned long int STACHGCUR	=		1000*1000;
//#define STACHGCUR			800*1000
/*关屏充电电流，uA*/
unsigned long int EARCHGCUR	=		1500*1000;
//#define EARCHGCUR			800*1000
/*休眠充电电流，uA*/
unsigned long int SUSCHGCUR	=		2000*1000;
//#define SUSCHGCUR			1200*1000
/*关机充电电流，uA*/
unsigned long int CLSCHGCUR	=		2000*1000;
//#define CLSCHGCUR			1200*1000


/*
const uint8_t ocvreg_d720[0x1f]={
0,    // 3.13V    OCVREG0    
0,    // 3.27V    OCVREG1    
0,   // 3.34V    OCVREG2    
0,  // 3.41V    OCVREG3    
0, // 3.48V    OCVREG4    
0,    // 3.52V    OCVREG5    
0,    // 3.55V    OCVREG6    
2,    // 3.57V    OCVREG7    
4,    // 3.59V    OCVREG8    
6,    // 3.61V    OCVREG9    
8,    // 3.63V    OCVREGA    
9,    // 3.64V    OCVREGB    
12,   // 3.66V    OCVREGC    
22,   // 3.7V     OCVREGD    
32,   // 3.73V    OCVREGE    
46,   // 3.77V    OCVREGF    
49,   // 3.78V    OCVREG10      
54,   // 3.8V     OCVREG11      
59,   // 3.82V    OCVREG12      
63,   // 3.84V    OCVREG13      
64,   // 3.85V    OCVREG14      
67,   // 3.87V    OCVREG15      
73,   // 3.91V    OCVREG16      
78,   // 3.94V    OCVREG17      
83,   // 3.98V    OCVREG18      
86,   // 4.01V    OCVREG19      
90,   // 4.05V    OCVREG1A      
94,   // 4.08V    OCVREG1B      
95,   // 4.1V     OCVREG1C      
97,   // 4.12V    OCVREG1D      
98,   // 4.14V    OCVREG1E      
100,  // 4.15V    OCVREG1F  
};
*/
/*
const uint8_t ocvreg_d720[0x20]={
0,    // 3.13V    OCVREG0    
0,    // 3.27V    OCVREG1    
0,   // 3.34V    OCVREG2    
0,  // 3.41V    OCVREG3    
0, // 3.48V    OCVREG4    
0,    // 3.52V    OCVREG5    
0,    // 3.55V    OCVREG6    
0,    // 3.57V    OCVREG7    
1,    // 3.59V    OCVREG8    
1,    // 3.61V    OCVREG9    
2,    // 3.63V    OCVREGA    
2,    // 3.64V    OCVREGB    
4,   // 3.66V    OCVREGC    
10,   // 3.7V     OCVREGD    
15,   // 3.73V    OCVREGE    
20,   // 3.77V    OCVREGF    
27,   // 3.78V    OCVREG10      
39,   // 3.8V     OCVREG11      
46,   // 3.82V    OCVREG12      
49,   // 3.84V    OCVREG13      
52,   // 3.85V    OCVREG14      
56,   // 3.87V    OCVREG15      
61,   // 3.91V    OCVREG16      
67,   // 3.94V    OCVREG17      
71,   // 3.98V    OCVREG18      
76,   // 4.01V    OCVREG19      
80,   // 4.05V    OCVREG1A      
84,   // 4.08V    OCVREG1B      
88,   // 4.1V     OCVREG1C      
92,   // 4.12V    OCVREG1D      
96,   // 4.14V    OCVREG1E      
100  // 4.15V    OCVREG1F  
};
*/

const uint8_t ocvreg_d720[0x20]={
0,    // 3.13V    OCVREG0    
0,    // 3.27V    OCVREG1    
0,   // 3.34V    OCVREG2    
0,  // 3.41V    OCVREG3    
0, // 3.48V    OCVREG4    
0,    // 3.52V    OCVREG5    
0,    // 3.55V    OCVREG6    
1,    // 3.57V    OCVREG7    
3,    // 3.59V    OCVREG8    
5,    // 3.61V    OCVREG9    
9,    // 3.63V    OCVREGA    
14,    // 3.64V    OCVREGB    
22,   // 3.66V    OCVREGC    
32,   // 3.7V     OCVREGD    
34,   // 3.73V    OCVREGE    
35,   // 3.77V    OCVREGF    
38,   // 3.78V    OCVREG10      
42,   // 3.8V     OCVREG11      
46,   // 3.82V    OCVREG12      
49,   // 3.84V    OCVREG13      
52,   // 3.85V    OCVREG14      
56,   // 3.87V    OCVREG15      
61,   // 3.91V    OCVREG16      
67,   // 3.94V    OCVREG17      
71,   // 3.98V    OCVREG18      
76,   // 4.01V    OCVREG19      
80,   // 4.05V    OCVREG1A      
84,   // 4.08V    OCVREG1B      
88,   // 4.1V     OCVREG1C      
92,   // 4.12V    OCVREG1D      
96,   // 4.14V    OCVREG1E      
100  // 4.15V    OCVREG1F  
};

const uint8_t ocvreg_d721[0x20]={
0,    // 3.13V    OCVREG0    
0,    // 3.27V    OCVREG1    
0,    // 3.34V    OCVREG2    
0,    // 3.41V    OCVREG3    
0,    // 3.48V    OCVREG4    
0,    // 3.52V    OCVREG5    
0,    // 3.55V    OCVREG6    
2,    // 3.57V    OCVREG7    
4,    // 3.59V    OCVREG8    
6,    // 3.61V    OCVREG9    
8,    // 3.63V    OCVREGA    
10,    // 3.64V    OCVREGB    
14,   // 3.66V    OCVREGC    
26,   // 3.7V     OCVREGD    
39,   // 3.73V    OCVREGE    
51,   // 3.77V    OCVREGF    
53,   // 3.78V    OCVREG10      
57,   // 3.8V     OCVREG11      
62,   // 3.82V    OCVREG12      
65,   // 3.84V    OCVREG13      
67,   // 3.85V    OCVREG14      
70,   // 3.87V    OCVREG15      
76,   // 3.91V    OCVREG16      
80,   // 3.94V    OCVREG17      
84,   // 3.98V    OCVREG18      
87,   // 4.01V    OCVREG19      
92,   // 4.05V    OCVREG1A      
95,   // 4.08V    OCVREG1B      
96,   // 4.1V     OCVREG1C      
98,   // 4.12V    OCVREG1D      
99,   // 4.14V    OCVREG1E      
100  // 4.15V    OCVREG1F  
};

const uint8_t * get_ocvreg(void)
{
	if(!strcmp(g_selected_utmodel,"d720")){
		printk("%s:return d720 ocv reg\n",__func__);
		return ocvreg_d720;
	}else if(!strcmp(g_selected_utmodel,"d721")){
		printk("%s:return d721 ocv reg\n",__func__);
		return ocvreg_d721;
	}else {
		printk("%s:return default d720 ocv reg\n",__func__);
		return ocvreg_d721;
	}
}
EXPORT_SYMBOL_GPL(get_ocvreg);

int get_r(void)
{
	if(!strcmp(g_selected_utmodel,"d720")){
		printk("%s:return d720 88\n",__func__);
		return 88;
	}else if(!strcmp(g_selected_utmodel,"d721")){
		printk("%s:return d721 35\n",__func__);
		return 35;
	}else {
		printk("%s:return default 100\n",__func__);
		return 100;
	}
}
EXPORT_SYMBOL_GPL(get_r);

int get_cap(void)
{
	if(!strcmp(g_selected_utmodel,"d720")){
		printk("%s:return d720 5150\n",__func__);
		return 5150;
	}else if(!strcmp(g_selected_utmodel,"d721")){
		printk("%s:return d721 3600\n",__func__);
		return 3600;
	}else {
		printk("%s:return default 6000\n",__func__);
		return 6000;
	}
}
EXPORT_SYMBOL_GPL(get_cap);