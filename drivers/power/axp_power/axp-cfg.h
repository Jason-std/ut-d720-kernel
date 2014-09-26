#ifndef __LINUX_AXP_CFG_H_
#define __LINUX_AXP_CFG_H_
#include "axp-mfd.h"

/*
val 1 is exynos5410
val 2 is exynos4412
*/
#define  UT_AXP_CFG	2

#if UT_AXP_CFG
#include <mach/gpio.h>
#endif

/*�豸��ַ*/
/*
	һ�㲻�ı䣺
	AXP22:0x34
*/
#define	AXP_DEVICES_ADDR	(0x68 >> 1)
/*i2c���������豸��:���忴��ʹ��ƽ̨Ӳ��������*/
/* modified by Albert_lee 2013-06-29 begin */


#define	AXP_I2CBUS			1

 /*Albert_lee 2013-06-29 end*/
/*��ԴоƬ��Ӧ���жϺţ����忴��ʹ�õ�ƽ̨Ӳ�������ӣ�
�ж���nmi����cpu����·irq����gpio*/
#if UT_AXP_CFG
#define AXP_IRQNO			IRQ_PMU_INT
#else
#define AXP_IRQNO			64
#endif

/*��ʼ����·�������λmV��0��Ϊ�ر�*/
/*
	ldo1��
		��Ӳ�����������ѹ������Ĳ��ˣ�ֻ���������ʾ��ѹ��
*/
//#if UT_AXP_CFG
//#define AXP_LDO1_VALUE			1800
//#else
#define AXP_LDO1_VALUE			3000
//#endif
/*
	ldo2��
		AXP22:700~3300,100/step
*/
#if UT_AXP_CFG==1
/* aldo 1  3.3v */
#define AXP_LDO2_VALUE		2800
#elif UT_AXP_CFG==2
#define AXP_LDO2_VALUE		1800
#else
#define AXP_LDO2_VALUE		700
#endif

#define AXP_ALIVE_VALUE		1000


/*
	ldo3��
		AXP22:700~3300,100/step
*/
#if UT_AXP_CFG
/* aldo 2  1.8v */
#define AXP_LDO3_VALUE		1800
#else
#if defined(CONFIG_AXP_DVT_VER)
	#define AXP_LDO3_VALUE		1000
	#else
	#define AXP_LDO3_VALUE		1800
	#endif
#endif


#define AXP_PLL_VALUE	1100

/*
	ldo4��
		AXP22:700~3300,100/step
*/
#if UT_AXP_CFG==1
/* aldo 3  3.3v */
#define AXP_LDO4_VALUE		3300
#elif UT_AXP_CFG==2 /*VDDQ_M12*/
#define AXP_LDO4_VALUE		1200
#else
#define AXP_LDO4_VALUE		3000
#endif

/*
	ldo5��
		AXP22:700~3300,100/step
*/
#if UT_AXP_CFG==1
/* axp22_dldo1  1.8v */
#define AXP_LDO5_VALUE		1800
#elif UT_AXP_CFG==2 /*VDD_OFF33*/
#define AXP_LDO5_VALUE		3300
#else
#define AXP_LDO5_VALUE		700
#endif
/*
	ldo6��
		AXP22:700~3300,100/step
*/
#if UT_AXP_CFG==1
/* axp22_dldo2  2.8v */
#define AXP_LDO6_VALUE		2800
#elif UT_AXP_CFG==2 /*VDD_CAM28*/
#define AXP_LDO6_VALUE		2800
#else
#define AXP_LDO6_VALUE		700
#endif
/*
	ldo7��
		AXP22:700~3300,100/step
*/
#if UT_AXP_CFG==1
/* axp22_dldo3  1.8v */
#define AXP_LDO7_VALUE		1800
#elif UT_AXP_CFG==2 /*VDD_NC*/
#if defined(CONFIG_AXP_DVT_VER)
#define AXP_LDO7_VALUE		1000
#else
#define AXP_LDO7_VALUE		700
#endif
#else
#define AXP_LDO7_VALUE		700
#endif
/*
	ldo8��
		AXP22:700~3300,100/step
*/
#if UT_AXP_CFG==1
/* axp22_dldo4  3.3v */
#define AXP_LDO8_VALUE		3300
#elif UT_AXP_CFG==2 /*VDD_NC*/
	#if	defined(CONFIG_AXP_DVT_VER)
#define AXP_LDO8_VALUE		1800
	#else
#define AXP_LDO8_VALUE		700
	#endif
#else
#define AXP_LDO8_VALUE		700
#endif
/*
	ldo9��
		AXP22:700~3300,100/step
*/

#if UT_AXP_CFG==1
/* axp22_eldo1  1.0v */
#define AXP_LDO9_VALUE		1000
#elif UT_AXP_CFG==2 /*VDD_18ON*/
#define AXP_LDO9_VALUE		1800
#else
#define AXP_LDO9_VALUE		700
#endif

/*
	ldo10��
		AXP22:700~3300,100/step
*/
#if UT_AXP_CFG==1 ||  defined(CONFIG_AXP_DVT_VER)
/* axp22_eldo2  1.8v */
#define AXP_LDO10_VALUE		1800
#elif UT_AXP_CFG==2 /*VDD_10PLL*/
#define AXP_LDO10_VALUE		1000
#else
#define AXP_LDO10_VALUE		700
#endif
/*
	ldo11��
		AXP22:700~3300,100/step
*/
#if UT_AXP_CFG==1
/* axp22_eldo3  1.8v */
#define AXP_LDO11_VALUE		3000
#elif UT_AXP_CFG==2 /*VDD_NC*/
#define AXP_LDO11_VALUE		3000
#else
#define AXP_LDO11_VALUE		3000
#endif

/*
	ldo12��
		AXP22:700~1400,100/step
*/
#if UT_AXP_CFG==1
/* axp22_dc5ldo  1.0v */
#define AXP_LDO12_VALUE		1000
#elif UT_AXP_CFG==2 /*VDD_ALIVE*/
#define AXP_LDO12_VALUE		1000
#else
#define AXP_LDO12_VALUE		1100
#endif

/*
	DCDC1: axp22_dcdc1
		AXP22:1600~3400,100/setp
*/
#if UT_AXP_CFG
#define AXP_DCDC1_VALUE		3300
#else
#define AXP_DCDC1_VALUE		3000
#endif
/*
	DCDC2��axp22_dcdc2
		AXP22:600~1540��20/step
*/
#if UT_AXP_CFG
#define AXP_DCDC2_VALUE		1000
#else
#define AXP_DCDC2_VALUE		1200
#endif
/*
	DCDC3��axp22_dcdc3
		AXP22:600~1860��20/step
*/
#if UT_AXP_CFG
#define AXP_DCDC3_VALUE		1100
#else
#define AXP_DCDC3_VALUE		1260
#endif
/*
	DCDC4��axp22_dcdc4
		AXP22:600~1540��20/step
*/
#if UT_AXP_CFG
#define AXP_DCDC4_VALUE		1000
#else
#define AXP_DCDC4_VALUE		1200
#endif
/*
	DCDC5��axp22_dcdc5
		AXP22:1000~2550��50/step
*/
#if UT_AXP_CFG==1
#define AXP_DCDC5_VALUE		1200
#elif UT_AXP_CFG==2
#define AXP_DCDC5_VALUE		1100
#else
#define AXP_DCDC5_VALUE		1500
#endif


#define AXP_LDOIO0_VALUE		1800



/*���������mAh������ʵ�ʵ�����������壬�Կ��ؼƷ�����˵
�����������Ҫ����������*/
#define BATCAP				4000

/*��ʼ��������裬m����һ����100~200֮�䣬������ø���ʵ��
���Գ�����ȷ���������Ǵ򿪴�ӡ��Ϣ�����ӵ���պù̼���
�ϵ�أ����ӳ����������������1���Ӻ󣬽��ϳ��������
1~2���ӣ�����ӡ��Ϣ�е�rdcֵ����������*/
#if UT_AXP_CFG
#define BATRDC				60
#else
#define BATRDC				100
#endif
/*��·��ѹ�����еĵ�ص�ѹ�Ļ���*/
#define AXP_VOL_MAX			1
/*
	��繦��ʹ�ܣ�
        AXP22:0-�رգ�1-��
*/
#define CHGEN       1

/*
	���������ã�uA��0Ϊ�رգ�
		AXP22:300~2550,100/step
*/
#if 1

#define INITCHGCUR 1000*1000

/*������������uA*/
extern unsigned long int STACHGCUR	;
//#define STACHGCUR			800*1000
/*������������uA*/
extern unsigned long int EARCHGCUR	;
//#define EARCHGCUR			800*1000
/*���߳�������uA*/
extern unsigned long int SUSCHGCUR;
//#define SUSCHGCUR			1200*1000
/*�ػ���������uA*/
extern unsigned long int CLSCHGCUR	;
//#define CLSCHGCUR			1200*1000
#else
/*������������uA*/
#define STACHGCUR			900*1000
/*������������uA*/
#define EARCHGCUR			900*1000
/*���߳�������uA*/
#define SUSCHGCUR			1500*1000
/*�ػ���������uA*/
#define CLSCHGCUR			1500*1000
#endif

/*Ŀ�����ѹ��mV*/
/*
	AXP22:4100/4220/4200/4240
*/
#define CHGVOL				4200*1000
/*������С�����õ�����ENDCHGRATE%ʱ��ֹͣ��磬%*/
/*
	AXP22:10\15
*/ 
#define ENDCHGRATE			10
/*�ػ���ѹ��mV*/
/*
	ϵͳ��ƵĹػ�����ĵ�ض˵�ѹ����Ҫ��ػ��ٷֱȡ�
	��·��ѹ��Ӧ�ٷֱȱ��͵羯���ѹ�໥��ϲŻ�������
*/
//#define SHUTDOWNVOL			3280
#define SHUTDOWNVOL			2850

/*adc���������ã�Hz*/
/*
	AXP22:100\200\400\800
*/
#define ADCFREQ				200
/*Ԥ��糬ʱʱ�䣬min*/
/*
	AXP22:40\50\60\70
*/
#define CHGPRETIME			50
/*������糬ʱʱ�䣬min*/
/*
	AXP22:360\480\600\720
*/
#define CHGCSTTIME			480


/*pek����ʱ�䣬ms*/
/*
	��power��Ӳ������ʱ�䣺
		AXP22:128/1000/2000/3000
*/
#define PEKOPEN				1000
/*pek����ʱ�䣬ms*/
/*
	��power���������жϵ�ʱ�䣬���ڴ�ʱ���Ƕ̰������̰���irq��
	���ڴ�ʱ���ǳ�������������irq��
		AXP22:1000/1500/2000/2500
*/
#define PEKLONG				1500
/*pek�����ػ�ʹ��*/
/*
	��power�������ػ�ʱ��Ӳ���ػ�����ʹ�ܣ�
		AXP22:0-���أ�1-�ػ�
*/
#define PEKOFFEN			1
/*pek�����ػ�ʹ�ܺ󿪻�ѡ��*/
/*
	��power�������ػ�ʱ��Ӳ���ػ���������ѡ��:
		AXP22:0-ֻ�ػ���������1-�ػ�������
*/
#define PEKOFFRESTART			0
/*pekpwr�ӳ�ʱ�䣬ms*/
/*
	������powerok�źŵ��ӳ�ʱ�䣺
		AXP20:8/16/32/64
*/
#define PEKDELAY			32
/*pek�����ػ�ʱ�䣬ms*/
/*
	��power���Ĺػ�ʱ����
		AXP22:4000/6000/8000/10000
*/
#define PEKOFF				6000
/*���¹ػ�ʹ��*/
/*
	AXP�ڲ��¶ȹ���Ӳ���ػ�����ʹ�ܣ�
		AXP22:0-���أ�1-�ػ�
*/
#define OTPOFFEN			0
/* ����ѹ����ʹ��*/
/*
	AXP22:0-�رգ�1-��
*/
#define USBVOLLIMEN		1
/*  �����ѹ��mV��0Ϊ������*/
/*
	AXP22:4000~4700��100/step
*/
#define USBVOLLIM			4700
/*  USB�����ѹ��mV��0Ϊ������*/
/*
	AXP22:4000~4700��100/step
*/
#define USBVOLLIMPC			4700

/* ����������ʹ��*/
/*
	AXP22:0-�رգ�1-��
*/
#define USBCURLIMEN		1
/* ���������mA��0Ϊ������*/
/*
	AXP22:500/900
*/
#define USBCURLIM			0
/* usb ���������mA��0Ϊ������*/
/*
	AXP22:500/900
*/
#define USBCURLIMPC			500
/* PMU �жϴ�������ʹ��*/
/*
	AXP22:0-�����ѣ�1-����
*/
#define IRQWAKEUP			1
/* N_VBUSEN PIN ���ܿ���*/
/*
	AXP22:0-���������OTG��ѹģ�飬1-���룬����VBUSͨ·
*/
#define VBUSEN			0
/* ACIN/VBUS In-short ��������*/
/*
	AXP22:0-AC VBUS�ֿ���1-ʹ��VBUS��AC,�޵���AC
*/
#define VBUSACINSHORT			0
/* CHGLED �ܽſ�������*/
/*
	AXP22:0-������1-�ɳ�繦�ܿ���
*/
#define CHGLEDFUN			1
/* CHGLED LED ��������*/
/*
	AXP22:0-���ʱled������1-���ʱled��˸
*/
#define CHGLEDTYPE			0
/* ���������У��ʹ��*/
/*
	AXP22:0-�رգ�1-��
*/
#define BATCAPCORRENT			1 /*TODO albert*/


/* �����ɺ󣬳�����ʹ��*/
/*
	AXP22:0-�رգ�1-��
*/
#define BATREGUEN			1


/* ��ؼ�⹦��ʹ��*/
/*
	AXP22:0-�رգ�1-��
*/
#define BATDET		1

/* �ڲ��¶ȹ��߹ػ�����ʹ��*/
/*
	AXP22:0-�رգ�1-��
*/
#define TEMP_PROTECT		1

/* PMU����ʹ��*/
/*
	AXP22:0-�رգ�1-�򿪰���Դ��16������PMU����
*/
#define PMURESET		0
/*�͵羯���ѹ1��%*/
/*
	����ϵͳ���������
	AXP22:5%~20%
*/
#define BATLOWLV1    19
/*�͵羯���ѹ2��%*/
/*
	����ϵͳ���������
	AXP22:0%~15%
*/
#define BATLOWLV2    8

#define ABS(x)				((x) >0 ? (x) : -(x) )

#ifdef	CONFIG_KP_AXP22
/*AXP GPIO start NUM,����ƽ̨ʵ���������*/
//#if UT_AXP_CFG
#define AXP22_NR_BASE (/*CONFIG_SAMSUNG_GPIO_EXTRA + */S3C_GPIO_END)
//#else
//#define AXP22_NR_BASE 100
//#endif

/*AXP GPIO NUM,�������������LCD power�Լ�VBUS driver pin*/
#define AXP22_NR 5

/*��ʼ����·��ѹ��Ӧ�ٷֱȱ�*/
/*
	����ʹ��Ĭ��ֵ��������ø���ʵ�ʲ��Եĵ����ȷ��ÿ��
	��Ӧ��ʣ��ٷֱȣ��ر���Ҫע�⣬�ػ���ѹSHUTDOWNVOL�͵��
	������ʼУ׼ʣ�������ٷֱ�BATCAPCORRATE��������׼ȷ��
	AXP22����
*/
#define OCVREG0				0		 // 3.13V
#define OCVREG1				0		 // 3.27V
#define OCVREG2				0		 // 3.34V
#define OCVREG3				0		 // 3.41V
#define OCVREG4				0		 // 3.48V
#define OCVREG5				0		 // 3.52V
#define OCVREG6				0		 // 3.55V
#define OCVREG7				0		 // 3.57V
#define OCVREG8				5		 // 3.59V
#define OCVREG9				8		 //3.61V
#define OCVREGA				9		 //3.63V
#define OCVREGB				10		 //3.64V
#define OCVREGC				13		 //3.66V
#define OCVREGD				16		 //3.7V
#define OCVREGE				20		 //3.73V 
#define OCVREGF				33		 //3.77V
#define OCVREG10		 	41                //3.78V
#define OCVREG11		 	46                //3.8V
#define OCVREG12		 	50                //3.82V 
#define OCVREG13		 	53                //3.84V
#define OCVREG14		 	57                //3.85V
#define OCVREG15		 	61                //3.87V
#define OCVREG16		 	67                //3.91V
#define OCVREG17		 	73                //3.94V
#define OCVREG18		 	78                //3.98V
#define OCVREG19		 	84                //4.01V
#define OCVREG1A		 	88                //4.05V
#define OCVREG1B		 	92                //4.08V
#define OCVREG1C		 	93                //4.1V 
#define OCVREG1D		 	94                //4.12V
#define OCVREG1E		 	95                //4.14V
#define OCVREG1F		 	100                //4.15V

/*  AXP�ж�*/
#define AXP_IRQ_USBLO		AXP22_IRQ_USBLO	//usb �͵�
#define AXP_IRQ_USBRE		AXP22_IRQ_USBRE	//usb �γ�
#define AXP_IRQ_USBIN		AXP22_IRQ_USBIN	//usb ����
#define AXP_IRQ_USBOV		AXP22_IRQ_USBOV	//usb ��ѹ
#define AXP_IRQ_ACRE			AXP22_IRQ_ACRE	//ac  �γ�
#define AXP_IRQ_ACIN			AXP22_IRQ_ACIN	//ac  ����
#define AXP_IRQ_ACOV		AXP22_IRQ_ACOV //ac  ��ѹ
#define AXP_IRQ_TEMLO		AXP22_IRQ_TEMLO //��ص���
#define AXP_IRQ_TEMOV		AXP22_IRQ_TEMOV //��ع���
#define AXP_IRQ_CHAOV		AXP22_IRQ_CHAOV //��س�����
#define AXP_IRQ_CHAST		AXP22_IRQ_CHAST //��س�翪ʼ
#define AXP_IRQ_BATATOU	AXP22_IRQ_BATATOU //����˳�����ģʽ
#define AXP_IRQ_BATATIN		AXP22_IRQ_BATATIN //��ؽ��뼤��ģʽ
#define AXP_IRQ_BATRE		AXP22_IRQ_BATRE //��ذγ�
#define AXP_IRQ_BATIN		AXP22_IRQ_BATIN //��ز���
#define AXP_IRQ_PEKLO		AXP22_IRQ_POKLO //power������
#define AXP_IRQ_PEKSH		AXP22_IRQ_POKSH //power���̰�

#define AXP_IRQ_CHACURLO	AXP22_IRQ_CHACURLO //������С������ֵ
#define AXP_IRQ_ICTEMOV		AXP22_IRQ_ICTEMOV //AXPоƬ�ڲ�����
#define AXP_IRQ_EXTLOWARN2	AXP22_IRQ_EXTLOWARN2 //APS��ѹ�����ѹ2
#define AXP_IRQ_EXTLOWARN1	AXP22_IRQ_EXTLOWARN1 //APS��ѹ�����ѹ1

#define AXP_IRQ_GPIO0TG		AXP22_IRQ_GPIO0TG //GPIO0������ش���
#define AXP_IRQ_GPIO1TG		AXP22_IRQ_GPIO1TG //GPIO1������ش���

#define AXP_IRQ_PEKFE		AXP22_IRQ_PEKFE //power���½��ش���
#define AXP_IRQ_PEKRE		AXP22_IRQ_PEKRE //power�������ش���
#define AXP_IRQ_TIMER		AXP22_IRQ_TIMER //��ʱ����ʱ

#endif

/*ѡ����Ҫ�򿪵��ж�ʹ��*/
static const uint64_t AXP22_NOTIFIER_ON = (AXP_IRQ_USBIN |AXP_IRQ_USBRE |
				       		                            AXP_IRQ_ACIN |AXP_IRQ_ACRE |
				       		                            AXP_IRQ_BATIN |AXP_IRQ_BATRE |
				       		                            AXP_IRQ_CHAST |AXP_IRQ_CHAOV|
						                            (uint64_t)AXP_IRQ_PEKFE |(uint64_t)AXP_IRQ_PEKRE);

static const uint64_t AXP22_NOTIFIER_ON_NOCHARGER = (AXP_IRQ_USBIN |AXP_IRQ_USBRE |
				       		                            /*AXP_IRQ_ACIN |AXP_IRQ_ACRE |
				       		                            AXP_IRQ_BATIN |AXP_IRQ_BATRE |
				       		                            AXP_IRQ_CHAST |AXP_IRQ_CHAOV|*/
						                            (uint64_t)AXP_IRQ_PEKFE |(uint64_t)AXP_IRQ_PEKRE);

static const uint64_t AXP22_NOTIFIER_SUSPEND = (/*AXP_IRQ_USBIN |AXP_IRQ_USBRE |*/
				       		                            /*AXP_IRQ_ACIN |AXP_IRQ_ACRE |*/
				       		                            /*AXP_IRQ_BATIN |AXP_IRQ_BATRE |*/
				       		                            /*AXP_IRQ_CHAST |AXP_IRQ_CHAOV|*/
						                            (uint64_t)AXP_IRQ_PEKFE |(uint64_t)AXP_IRQ_PEKRE);

/* ��Ҫ�������ţ��usb�ػ�������bootʱpower_start����Ϊ1������Ϊ0*/
#define POWER_START 0


#endif
