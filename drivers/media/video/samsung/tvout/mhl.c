/* linux/drivers/media/video/samsung/tv20/mhl.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Core file for MHL driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/irq.h>
#include <linux/kobject.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>

#include <linux/bug.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <plat/gpio-cfg.h>
#include <plat/s5pv210.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/input.h>


#include "mhl.h"
#include	"sii_9244_driver.h"
#ifdef SUPPORT_MHL_RCP
#include "../../../../input/keyboard/s3c_keypad_input.h"
#endif

#define	MHL_RST_PIN	EXYNOS4_GPL0(6) 
#define   MHL_IRQ_PIN      EXYNOS4_GPX2(6) 

static int irq_mhl;
bool_t	vbusPowerState = true;		// false: 0 = vbus output on; true: 1 = vbus output off;
struct i2c_client *mhl_page0;
struct i2c_client *mhl_page1;
struct i2c_client *mhl_page2;
struct i2c_client *mhl_cbus;

static DECLARE_WORK(mhl_work, (void *)work_queue);

/*
	sii9244 timer count
*/
uint16_t g_timerCounters[TIMER_COUNT];

uint16_t g_timerElapsed;
uint16_t g_elapsedTick;
uint16_t g_timerElapsedGranularity;

uint16_t g_timerElapsed1;
uint16_t g_elapsedTick1;
uint16_t g_timerElapsedGranularity1;

static struct timer_list  g_mhl_timer;
static struct regulator *mhl_regulator_1v2;
static struct regulator *mhl_regulator_1v8;
static struct regulator *mhl_regulator_2v8;
//static struct regulator *mhl_regulator_3v3;
/*
 * MHL read ftn.
 */
static u8 mhl_i2c_read(struct i2c_client *i2c, char reg)
{

	u8 ret;
	if (i2c_smbus_write_byte(i2c, reg) < 0)
		return 0;
	
	ret = i2c_smbus_read_byte(i2c);
	if (ret < 0) {
		MHLPRINTK("i2c read fail reg %x data %x\n", reg, ret);
		return 0;
	}
	return ret;
}


/*
 * MHL_write ftn.
 */
s32 mhl_i2c_write(struct i2c_client *i2c, char reg, const u8 val)
{
	return i2c_smbus_write_byte_data(i2c, reg, val);
}

uint8_t ReadBytePage0(uint8_t Offset)
{
	if (mhl_page0 == NULL) {
		MHLPRINTK("invalid mhl page0 client.\n");
		return 0;
	}

	return mhl_i2c_read(mhl_page0, Offset);
}
EXPORT_SYMBOL(ReadBytePage0);

uint8_t ReadBytePage1(uint8_t Offset)
{
	if (mhl_page1 == NULL) {
		MHLPRINTK("invalid mhl page1 client.\n");
		return 0;
	}

	return mhl_i2c_read(mhl_page1, Offset);
}
EXPORT_SYMBOL(ReadBytePage1);

uint8_t ReadBytePage2(uint8_t Offset)
{
	if (mhl_page2 == NULL) {
		MHLPRINTK("invalid mhl page2 client.\n");
		return 0;
	}

	return mhl_i2c_read(mhl_page2, Offset);
}
EXPORT_SYMBOL(ReadBytePage2);

uint8_t ReadByteCBUS(uint8_t Offset)
{
	if (mhl_cbus == NULL) {
		MHLPRINTK("invalid mhl cbus client.\n");
		return 0;
	}

	return mhl_i2c_read(mhl_cbus, Offset);
}
EXPORT_SYMBOL(ReadByteCBUS);

void WriteBytePage0(uint8_t Offset, uint8_t Data)
{
	if (mhl_page0 == NULL) {
		MHLPRINTK("invalid mhl page0 client.\n");
		return;
	}

	if (mhl_i2c_write(mhl_page0, Offset, Data) < 0)
		MHLPRINTK("Failed at Reg = 0x%x, Data = 0x%x\n", Offset, Data);
}
EXPORT_SYMBOL(WriteBytePage0);

void WriteBytePage1(uint8_t Offset, uint8_t Data)
{
	if (mhl_page1 == NULL) {
		MHLPRINTK("invalid mhlpage1 client.\n");
		return;
	}

	if (mhl_i2c_write(mhl_page1, Offset, Data) < 0)
		MHLPRINTK("Failed at Reg = 0x%x, Data = 0x%x\n", Offset, Data);	
}
EXPORT_SYMBOL(WriteBytePage1);

void WriteBytePage2(uint8_t Offset, uint8_t Data)
{
	if (mhl_page2 == NULL) {
		MHLPRINTK("invalid mhl page2 client.\n");
		return;
	}

	if (mhl_i2c_write(mhl_page2, Offset, Data) < 0)
		MHLPRINTK("Failed at Reg = 0x%x, Data = 0x%x\n", Offset, Data);
}
EXPORT_SYMBOL(WriteBytePage2);

void WriteByteCBUS(uint8_t Offset, uint8_t Data)
{
	if (mhl_cbus == NULL) {
		MHLPRINTK("invalid mhl cbus client.\n");
		return;
	}

	if (mhl_i2c_write(mhl_cbus, Offset, Data) < 0)
		MHLPRINTK("Failed at Reg = 0x%x, Data = 0x%x\n", Offset, Data);
}
EXPORT_SYMBOL(WriteByteCBUS);

void ReadModifyWritePage0(uint8_t Offset, uint8_t Mask, uint8_t Data)
{
	uint8_t Temp;

	Temp = ReadBytePage0(Offset);
	Temp &= ~Mask;
	Temp |= (Data & Mask);
	WriteBytePage0(Offset, Temp);
}
EXPORT_SYMBOL(ReadModifyWritePage0);

void SET_BIT(struct i2c_client *i2c, uint8_t Offset, uint8_t bitnumber)
{
	uint8_t Temp;
	Temp = mhl_i2c_read(i2c, Offset);
	Temp |= (1 << bitnumber);
	mhl_i2c_write(i2c, Offset, Temp);
}

void CLR_BIT(struct i2c_client *i2c, uint8_t Offset, uint8_t bitnumber)
{
	uint8_t Temp;
	Temp = mhl_i2c_read(i2c, Offset);
	Temp &= ~(1<<bitnumber);
	mhl_i2c_write(i2c, Offset, Temp);
}

static const bool match_id(const struct i2c_device_id *id,
				const struct i2c_client *client)
{
	if (strcmp(client->name, id->name) == 0)
		return true;

	return false;
}

s32 mhl_read_id(void)
{
	u8 dev_IDL = 0, dev_IDH = 0;
	s32 ret = 0;

	dev_IDL = ReadBytePage0(0x02);
	dev_IDH = ReadBytePage0(0x03);
	
	dev_info(&mhl_page0->adapter->dev, "dev_ID is: [0x%x : 0x%x]", dev_IDH, dev_IDL);	
	
	if (dev_IDL == 0x34 && dev_IDH == 0x92) {
		return 0;
	}

	dev_info(&mhl_page0->adapter->dev, "Error device"
		": dev_ID is: [0x%x : 0x%x]", dev_IDH, dev_IDL);
	return -EIO;
}

int mhl_regulator_onoff(int flag,struct i2c_client *client)
{
	int ret = 0;
/* yqf, for pc4, should keep them alive always after mhl initialization
	MHL33,  		~VDD13_UOTG~LDO12~"vdd33_uotg"
	VDDIOPERI_18 			~LDO13~"vddioperi_18"
	VDDIOPERI_28 			~LDO18~"vddioperi_18"
	VDD12_MHL				~ LDO28~"dvdd12"
*/
#if 0
		mhl_regulator_3v3= regulator_get(NULL, "vdd33_uotg");
		if (IS_ERR(mhl_regulator_3v3)) {
			dev_err(&client->adapter->dev,
				"failed to find mhl power domain\n");
			ret=-1;
		}
#endif		
		mhl_regulator_1v2 = regulator_get(NULL, "dvdd12");
		if (IS_ERR(mhl_regulator_1v2)) {
			dev_err(&client->adapter->dev,
				"failed to find mhl power domain\n");
			ret=-1;
		}
		
		mhl_regulator_1v8 = regulator_get(NULL, "vddioperi_18");
		if (IS_ERR(mhl_regulator_1v8)) {
			dev_err(&client->adapter->dev,
				"failed to find mhl power domain\n");
			ret=-1;
		}
		
		mhl_regulator_2v8 = regulator_get(NULL, "vddioperi_28");
		if (IS_ERR(mhl_regulator_2v8)) {
			dev_err(&client->adapter->dev,
				"failed to find mhl power domain\n");
			ret=-1;
		}
		
	if(flag){	
	//	ret = regulator_enable(mhl_regulator_3v3);
		//if (ret < 0)
		//	dev_err(&client->adapter->dev,
		//		"failed to enable mhl power domain\n");
		
		ret = regulator_enable(mhl_regulator_1v2);
		if (ret < 0)
			dev_err(&client->adapter->dev,
				"failed to enable mhl power domain\n");
		
		ret = regulator_enable(mhl_regulator_1v8);
		if (ret < 0)
			dev_err(&client->adapter->dev,
				"failed to enable mhl power domain\n");

		ret = regulator_enable(mhl_regulator_2v8);
		if (ret < 0)
			dev_err(&client->adapter->dev,
				"failed to enable mhl power domain\n");
	}else{
		
	//	ret = regulator_disable(mhl_regulator_3v3);
	//	if (ret < 0)
	//		dev_err(&client->adapter->dev,
	//			"failed to enable mhl power domain\n");
		
		ret = regulator_disable(mhl_regulator_1v2);
		if (ret < 0)
			dev_err(&client->adapter->dev,
				"failed to enable mhl power domain\n");
		
		ret = regulator_disable(mhl_regulator_1v8);
		if (ret < 0)
			dev_err(&client->adapter->dev,
				"failed to enable mhl power domain\n");

		ret = regulator_disable(mhl_regulator_2v8);
		if (ret < 0)
			dev_err(&client->adapter->dev,
				"failed to enable mhl power domain\n");
		
	}
   return ret;				
}

static bool Sii9244_Reset(void)
{
//pc4: MHL_RST~GPL0_6
	int err;

	err = gpio_request(MHL_RST_PIN, "MHL_RST");

	if (err) {
		printk(KERN_ERR "#### failed to request GPL0_4 for mhl\n");
		return false;
	}

	s3c_gpio_cfgpin(MHL_RST_PIN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(MHL_RST_PIN, S3C_GPIO_PULL_NONE);
	gpio_direction_output(MHL_RST_PIN, 0);
	mdelay(5);
	gpio_direction_output(MHL_RST_PIN, 1);
	gpio_free(MHL_RST_PIN);

	return true;
}

bool  work_initial;
static void TimerTickHandler(void)
{
	uint8_t i;
	static u32 count;
	
	for (i = 0; i < TIMER_COUNT; i++) {
		if (g_timerCounters[i] > 0)
			g_timerCounters[i]--;
	}

	g_elapsedTick++;
	if (g_elapsedTick == g_timerElapsedGranularity) {
		g_timerElapsed++;
		g_elapsedTick = 0;
	}

	g_elapsedTick1++;
	if (g_elapsedTick1 == g_timerElapsedGranularity1) {
		g_timerElapsed1++;
		g_elapsedTick1 = 0;
	}

	//count++;  //yqf, marked

	//if (!(count%3))
		schedule_work(&mhl_work);

	//g_mhl_timer.expires = jiffies + 4;  //yqf, original for lenovo
	g_mhl_timer.expires = jiffies +msecs_to_jiffies(200);
	add_timer(&g_mhl_timer);
	
}

/*------------------------------------------------------------------------------
*  Function: HalTimerInit
* Description:
------------------------------------------------------------------------------*/

void HalTimerInit(void)
{
	uint8_t i;

	/*initializer timer counters in array*/
	for (i = 0; i < TIMER_COUNT; i++)
		g_timerCounters[i] = 0;

	g_timerElapsed = 0;
	g_timerElapsed1 = 0;
	g_elapsedTick = 0;
	g_elapsedTick1 = 0;
	g_timerElapsedGranularity = 0;
	g_timerElapsedGranularity1 = 0;
}

/*------------------------------------------------------------------------------
* Function:    HalTimerSet
* Description:
------------------------------------------------------------------------------*/

void HalTimerSet(uint8_t index, uint16_t m_sec)
{

	switch (index) {
	case TIMER_0:
	case TIMER_POLLING:
	case TIMER_2:
	case TIMER_3:
	case TIMER_4:
	case TIMER_5:
#ifdef IS_EVENT
	case TIMER_LINK_CHECK:
	case TIMER_LINK_RESPONSE:
#endif
		g_timerCounters[index] = m_sec;
		break;

	case ELAPSED_TIMER:
		g_timerElapsedGranularity = m_sec;
		g_timerElapsed = 0;
		g_elapsedTick = 0;
		break;

	case ELAPSED_TIMER1:
		g_timerElapsedGranularity1 = m_sec;
		g_timerElapsed1 = 0;
		g_elapsedTick1 = 0;
		break;
	}
}

/*------------------------------------------------------------------------------
* Function:    HalTimerWait
* Description: Waits for the specified number of milliseconds, using timer 0.
------------------------------------------------------------------------------*/

void HalTimerWait(uint16_t ms)
{
	mdelay(ms);
}

/*------------------------------------------------------------------------------
* Function:    HalTimerExpired
* Description: Returns > 0 if specified timer has expired.
------------------------------------------------------------------------------*/

uint8_t HalTimerExpired(uint8_t timer)
{
	if (timer < TIMER_COUNT)
		return (g_timerCounters[timer] == 0);

	return 0;
}

/*------------------------------------------------------------------------------
* Function:    HalTimerElapsed
* Description: Returns current timer tick.  Rollover depends on the
*              granularity specified in the SetTimer() call.
------------------------------------------------------------------------------*/

uint16_t HalTimerElapsed(uint8_t index)
{
	uint16_t elapsedTime;

	if (index == ELAPSED_TIMER)
		elapsedTime = g_timerElapsed;
	else
		elapsedTime = g_timerElapsed1;

	return elapsedTime;
}

#ifdef SUPPORT_MHL_RCP
static struct input_dev *kpdevice;
#endif

void Rcp_KeySend(unsigned int keycode)
{
#ifdef SUPPORT_MHL_RCP
	if(kpdevice == NULL)
		kpdevice = g_keypad_input;

	input_report_key(kpdevice, keycode, 1);
	input_sync(kpdevice);
	input_report_key(kpdevice, keycode, 0);
	input_sync(kpdevice);
#endif
}
/*
     AppRcpDemo
     This function is supposed to provide a demo code to elicit how to call RCP
     API function.
*/
void	AppRcpDemo(uint8_t event, uint8_t eventParameter)
{
	uint8_t		rcpKeyCode;

	/*MHLPRINTK("App: Got event = %02X, eventParameter = %02X\n",
		(int)event, (int)eventParameter);*/

	switch (event) {
	case MHL_TX_EVENT_DISCONNECTION:
		MHLPRINTK("App: Got event = MHL_TX_EVENT_DISCONNECTION\n");
		break;

	case MHL_TX_EVENT_CONNECTION:
		MHLPRINTK("App: Got event = MHL_TX_EVENT_CONNECTION\n");
		break;

	case MHL_TX_EVENT_RCP_READY:
#if 0 /* removed demo RCP key send in here. by oscar 20101215 */
	/* Demo RCP key code PLAY*/
	rcpKeyCode = APP_DEMO_RCP_SEND_KEY_CODE;

	MHLPRINTK("App: Got event = MHL_TX_EVENT_RCP_READY... \
		Sending RCP (%02X)\n", (int) rcpKeyCode);

	if ((0 == (MHL_FEATURE_RCP_SUPPORT & eventParameter)))
		MHLPRINTK("App: Peer does NOT support RCP\n");

	if ((0 == (MHL_FEATURE_RAP_SUPPORT & eventParameter)))
		MHLPRINTK("App: Peer does NOT support RAP\n");

	if ((0 == (MHL_FEATURE_SP_SUPPORT & eventParameter)))
		MHLPRINTK("App: Peer does NOT support WRITE_BURST\n");

	/*
	 * If RCP engine is ready, send one code
	*/
	if (SiiMhlTxRcpSend(rcpKeyCode))
		MHLPRINTK("App: SiiMhlTxRcpSend (%02X)\n", (int) rcpKeyCode);
	else
		MHLPRINTK("App: SiiMhlTxRcpSend (%02X) Returned Failure.\n",
			(int) rcpKeyCode);
#endif
		break;
	case MHL_TX_EVENT_RCP_RECEIVED:
		/*
		* Check if we got an RCP. Application can perform the operation
		* and send RCPK or RCPE. For now, we send the RCPK
		*/
		rcpKeyCode = eventParameter;
		MHLPRINTK("App1: Received an RCP key code = %02X\n",
			(int)rcpKeyCode);

		/* Added RCP key printf and interface with UI.
			by oscar 20101217*/
		switch (rcpKeyCode) {
		case MHL_RCP_CMD_SELECT:
			Rcp_KeySend(33);
			TX_DEBUG_PRINT(("\nSelect received\n\n"));
			break;
		case MHL_RCP_CMD_UP:
			Rcp_KeySend(25);
			TX_DEBUG_PRINT(("\nUp received\n\n"));
			break;
		case MHL_RCP_CMD_DOWN:
			Rcp_KeySend(41);
			TX_DEBUG_PRINT(("\nDown received\n\n"));
			break;
		case MHL_RCP_CMD_LEFT:
			Rcp_KeySend(57);
			TX_DEBUG_PRINT(("\nLeft received\n\n"));
			break;
		case MHL_RCP_CMD_RIGHT:
			Rcp_KeySend(49);
			TX_DEBUG_PRINT(("\nRight received\n\n"));
			break;
		case MHL_RCP_CMD_ROOT_MENU:
			TX_DEBUG_PRINT(("\nRoot Menu received\n\n"));
			break;
		case MHL_RCP_CMD_EXIT:
			Rcp_KeySend(34);
			TX_DEBUG_PRINT(("\nExit received\n\n"));
			break;
		case MHL_RCP_CMD_NUM_0:
			TX_DEBUG_PRINT(("\nNumber 0 received\n\n"));
			break;
		case MHL_RCP_CMD_NUM_1:
			TX_DEBUG_PRINT(("\nNumber 1 received\n\n"));
			break;
		case MHL_RCP_CMD_NUM_2:
			TX_DEBUG_PRINT(("\nNumber 2 received\n\n"));
			break;
		case MHL_RCP_CMD_NUM_3:
			TX_DEBUG_PRINT(("\nNumber 3 received\n\n"));
			break;
		case MHL_RCP_CMD_NUM_4:
			TX_DEBUG_PRINT(("\nNumber 4 received\n\n"));
			break;
		case MHL_RCP_CMD_NUM_5:
			TX_DEBUG_PRINT(("\nNumber 5 received\n\n"));
			break;
		case MHL_RCP_CMD_NUM_6:
			TX_DEBUG_PRINT(("\nNumber 6 received\n\n"));
			break;
		case MHL_RCP_CMD_NUM_7:
			TX_DEBUG_PRINT(("\nNumber 7 received\n\n"));
			break;
		case MHL_RCP_CMD_NUM_8:
			TX_DEBUG_PRINT(("\nNumber 8 received\n\n"));
			break;
		case MHL_RCP_CMD_NUM_9:
			TX_DEBUG_PRINT(("\nNumber 9 received\n\n"));
			break;
		case MHL_RCP_CMD_DOT:
			TX_DEBUG_PRINT(("\nDot received\n\n"));
			break;
		case MHL_RCP_CMD_ENTER:
			Rcp_KeySend(33);
			TX_DEBUG_PRINT(("\nEnter received\n\n"));
			break;
		case MHL_RCP_CMD_CLEAR:
			TX_DEBUG_PRINT(("\nClear received\n\n"));
			break;
		case MHL_RCP_CMD_SOUND_SELECT:
			TX_DEBUG_PRINT(("\nSound Select received\n\n"));
			break;
		case MHL_RCP_CMD_PLAY:
			TX_DEBUG_PRINT(("\nPlay received\n\n"));
			break;
		case MHL_RCP_CMD_PAUSE:
			TX_DEBUG_PRINT(("\nPause received\n\n"));
			break;
		case MHL_RCP_CMD_STOP:
			TX_DEBUG_PRINT(("\nStop received\n\n"));
			break;
		case MHL_RCP_CMD_FAST_FWD:
			TX_DEBUG_PRINT(("\nFastfwd received\n\n"));
			break;
		case MHL_RCP_CMD_REWIND:
			TX_DEBUG_PRINT(("\nRewind received\n\n"));
			break;
		case MHL_RCP_CMD_EJECT:
			TX_DEBUG_PRINT(("\nEject received\n\n"));
			break;
		case MHL_RCP_CMD_FWD:
			TX_DEBUG_PRINT(("\nForward received\n\n"));
			break;
		case MHL_RCP_CMD_BKWD:
			TX_DEBUG_PRINT(("\nBackward received\n\n"));
			break;
		case MHL_RCP_CMD_PLAY_FUNC:
			TX_DEBUG_PRINT(("\nPlay Function received\n\n"));
			break;
		case MHL_RCP_CMD_PAUSE_PLAY_FUNC:
			TX_DEBUG_PRINT(("\nPause_Play Function received\n\n"));
			break;
		case MHL_RCP_CMD_STOP_FUNC:
			TX_DEBUG_PRINT(("\nStop Function received\n\n"));
			break;
		case MHL_RCP_CMD_F1:
			TX_DEBUG_PRINT(("\nF1 received\n\n"));
			break;
		case MHL_RCP_CMD_F2:
			TX_DEBUG_PRINT(("\nF2 received\n\n"));
			break;
		case MHL_RCP_CMD_F3:
			TX_DEBUG_PRINT(("\nF3 received\n\n"));
			break;
		case MHL_RCP_CMD_F4:
			TX_DEBUG_PRINT(("\nF4 received\n\n"));
			break;
		case MHL_RCP_CMD_F5:
			TX_DEBUG_PRINT(("\nF5 received\n\n"));
			break;
		default:
			break;
		}

		SiiMhlTxRcpkSend(rcpKeyCode);
		break;

	case MHL_TX_EVENT_RCPK_RECEIVED:
		MHLPRINTK("App: Received an RCPK = %02X\n", (int)eventParameter);
		break;

	case MHL_TX_EVENT_RCPE_RECEIVED:
		MHLPRINTK("App: Received an RCPE = %02X\n", (int)eventParameter);
		break;

	default:
		break;
	}
}

/*--------------------------------------------------------------------------
*
* AppVbusControl
*
* This function or macro is invoked from MhlTx driver to ask application to
* control the VBUS power. If powerOn is sent as non-zero, one should assume
* peer does not need power so quickly remove VBUS power.
*
* if value of "powerOn" is 0, then application must turn the VBUS power on
* within 50ms of this call to meet MHL specs timing.
*
* Application module must provide this function.
*-------------------------------------------------------------------------*/
void	AppVbusControl(bool_t powerOn)
{
	s32 err;
	err = gpio_request(EXYNOS4_GPF3(5), "GPF3");
	if (err)
		printk(KERN_ERR "gpio request error : %d\n", err);

	s3c_gpio_cfgpin(EXYNOS4_GPF3(5), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPF3(5), S3C_GPIO_PULL_DOWN);

	if (powerOn) {
		MHLPRINTK("App: Peer's POW bit is set."
			"Turn the VBUS power OFF here.\n");

		gpio_set_value(EXYNOS4_GPF3(5), 0);
	} else {
		MHLPRINTK("App: Peer's POW bit is cleared."
			"Turn the VBUS power ON here.\n");
		gpio_set_value(EXYNOS4_GPF3(5), 1);
	}
	gpio_free(EXYNOS4_GPF3(5));
}


void work_queue(void)
{
	bool_t interruptDriven;
	uint8_t pollIntervalMs;
	uint8_t event;
	uint8_t eventParameter;

	/*Initialize host microcontroller and the timer*/
	/*HalInitCpu();*/
#if 0
	if (!work_initial) {
		HalTimerInit();
		HalTimerSet(TIMER_POLLING, MONITORING_PERIOD);

		printk(KERN_INFO"\n============================================\n");
		printk(KERN_INFO"Copyright 2010 Silicon Image\n");
		printk(KERN_INFO"SiI-9244 Starter Kit Firmware Version 1.00.87\n");
		printk(KERN_INFO"============================================\n");

		/*
		* Initialize the registers as required. Setup firmware vars.
		*/
		SiiMhlTxInitialize(interruptDriven = false,
				pollIntervalMs = MONITORING_PERIOD);

		work_initial = true;

		g_mhl_timer.expires = jiffies + 1;
		add_timer(&g_mhl_timer);
	}
#endif
 	//MHLPRINTK(" %s, entering \n",__func__);
 	
	/*
		Event loop
	*/
	SiiMhlTxGetEvents(&event, &eventParameter);

	if (MHL_TX_EVENT_NONE != event)
		AppRcpDemo(event, eventParameter);
}

u32 Int_count=0;

static irqreturn_t
s3c_mhl_interrupt(int irq, void *dev_id)
{
	if (irq == IRQ_EINT(22)) {
		MHLPRINTK("XEINT22  Interrupt occure\n");
		if (work_initial) {
			Int_count += 10;

			if (Int_count > 20)
				Int_count = 20;
		}
	} else {
		MHLPRINTK("%d  Interrupt occure\n", irq);
		return IRQ_NONE;
	}

	return IRQ_HANDLED;
}

static struct irqaction s3c_mhl_irq = {
	.name		= "s3c MHL",
	.flags		= IRQF_SHARED ,
	.handler	= s3c_mhl_interrupt,
};

static struct i2c_device_id mhl_idtable[] = {
	{"mhl_page0", 0},
	{"mhl_page1", 0},
	{"mhl_page2", 0},
	{"mhl_cbus", 0},
};

/*
 * i2c client ftn.
 */
static int __devinit mhl_probe(struct i2c_client *client,
			const struct i2c_device_id *dev_id)
{
	int ret=0;
	struct device dev_t=client->dev;

	if (match_id(&mhl_idtable[0], client))
		mhl_page0 = client;
	else if (match_id(&mhl_idtable[1], client))
		mhl_page1 = client;
	else if (match_id(&mhl_idtable[2], client))
		mhl_page2 = client;
	else if (match_id(&mhl_idtable[3], client))
		mhl_cbus = client;
	else {
		dev_err(&client->adapter->dev,
			"invalid i2c adapter: can not found dev_id matched\n");
		return -EIO;
	}

	dev_info(&client->adapter->dev, "attached %s "
		"into i2c adapter successfully\n", dev_id->name);

       if (mhl_page0 != NULL && mhl_page1 != NULL
		&& mhl_page2 != NULL && mhl_cbus != NULL) {
		s32 err;
		work_initial = false;
		
#ifdef CONFIG_REGULATOR
	if(mhl_regulator_onoff(1,client)<0)
		return -EIO;
#endif

#if 1 
		if (false == Sii9244_Reset())
			return -EIO;

		if (mhl_read_id() != 0){
			printk("mhl id read error\n");
			return -ENXIO;
			}

		{
			bool_t interruptDriven;
			uint8_t pollIntervalMs;

			HalTimerInit();
			HalTimerSet(TIMER_POLLING, MONITORING_PERIOD);

			printk(KERN_INFO"============================================\n");
			printk(KERN_INFO"Copyright 2010 Silicon Image\n");
			printk(KERN_INFO"SiI-9244 Starter Kit Firmware Version 1.00.87\n");
			printk(KERN_INFO"============================================\n");

			/*
			 * Initialize the registers as required.
			 * Setup firmware vars.
			*/
			SiiMhlTxInitialize(interruptDriven = false,
					pollIntervalMs = MONITORING_PERIOD);

			work_initial = true;
		}
#endif

		/*
			sii9244 interrupt initial&setup
		*/
		err = gpio_request(MHL_IRQ_PIN, "MHL_IRQ");
		if (err)
			printk(KERN_ERR "gpio request error : %d\n", err);

		s3c_gpio_cfgpin(MHL_IRQ_PIN, S3C_GPIO_SFN(0xf));
		s3c_gpio_setpull(MHL_IRQ_PIN, S3C_GPIO_PULL_UP);
#if 0
		irq_set_irq_type(IRQ_EINT(22), IRQ_TYPE_EDGE_FALLING);
		setup_irq(IRQ_EINT(22), &s3c_mhl_irq);
#else
		irq_mhl= gpio_to_irq(MHL_IRQ_PIN);

	//set_irq_wake(irq, 1);
	ret = request_irq(irq_mhl, s3c_mhl_interrupt,
		IRQF_TRIGGER_FALLING, NULL, NULL);
	if (ret != 0) {		
		printk("sii9244 int request failed.\n");
	}
#endif	
          		
		/*
			sii9244 timer&work queue initial
		*/
		/*
		init_timer(&g_mhl_worktimer);
		g_mhl_worktimer.function = work_queue;
		g_mhl_worktimer.expires = jiffies + 10*HZ;
		add_timer(&g_mhl_worktimer);
		*/
		init_timer(&g_mhl_timer);
		g_mhl_timer.function = TimerTickHandler;
		g_mhl_timer.expires = jiffies + 5*HZ;  
		add_timer(&g_mhl_timer);
	}
	return ret;
}

static int mhl_remove(struct i2c_client *client)
{
	int ret = 0;
	dev_info(&client->adapter->dev, "detached s5p_mhl "
		"from i2c adapter successfully\n");

	if (mhl_page0 == client)
		mhl_page0 = NULL;
	else if (mhl_page1 == client)
		mhl_page1 = NULL;
	else if (mhl_page2 == client)
		mhl_page2 = NULL;
	else if (mhl_cbus == client)
		mhl_cbus = NULL;

	if (mhl_page0 == NULL && mhl_page1 == NULL
		&& mhl_page2 == NULL && mhl_cbus == NULL) {
		Sii9244_Reset();
		gpio_free(MHL_IRQ_PIN);

		remove_irq(IRQ_EINT(22), &s3c_mhl_irq);
		del_timer(&g_mhl_timer);

#ifdef CONFIG_REGULATOR
		mhl_regulator_onoff(0, client);
#endif

	}
	return 0;
}

static int mhl_suspend(struct i2c_client *cl, pm_message_t mesg)
{
	int err;
	if (cl == mhl_page0) {
		work_initial = false;
		Int_count=0;  //yqf added
		disable_irq(irq_mhl);  //yqf, reopen it after resume init
		
		err = gpio_request(MHL_RST_PIN, "MHL_RST");
		if (err) {
			printk(KERN_ERR "#### failed to request GPL0_6 for mhl\n");
			return false;
		}
		s3c_gpio_cfgpin(MHL_RST_PIN, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(MHL_RST_PIN, S3C_GPIO_PULL_NONE);
		gpio_direction_output(MHL_RST_PIN, 0);
		gpio_free(MHL_RST_PIN);

#ifdef CONFIG_REGULATOR
		if(mhl_regulator_onoff(0,cl)<0)
			return -EIO;
#endif
	}
	return 0;
}

static int mhl_resume(struct i2c_client *cl)
{
	if (cl == mhl_page0) {
#ifdef CONFIG_REGULATOR
		if(mhl_regulator_onoff(1,cl)<0)
			return -EIO;
#endif
		if (false == Sii9244_Reset())
			return -EIO;

		if (mhl_read_id() != 0)
			return -ENXIO;

		HalTimerInit();
		HalTimerSet(TIMER_POLLING, MONITORING_PERIOD);
		
		/*
		 * Initialize the registers as required.
		 * Setup firmware vars.
		*/
		SiiMhlTxInitialize(false,MONITORING_PERIOD);
		work_initial = true;
		enable_irq(irq_mhl); //added yqf, open it after init
	}
	return 0;
}


MODULE_DEVICE_TABLE(i2c, mhl_idtable);

static struct i2c_driver mhl_driver = {
	.driver = {
		.name = "s5p_mhl",
	},
	.id_table	= mhl_idtable,
	.probe		= mhl_probe,
	.remove		= __devexit_p(mhl_remove),

	.suspend	= mhl_suspend,
	.resume		= mhl_resume,
};

static int __init mhl_init(void)
{
	return i2c_add_driver(&mhl_driver);
}

static void __exit mhl_exit(void)
{
	i2c_del_driver(&mhl_driver);
}


MODULE_AUTHOR("Jiang Shanbin <sb.jiang@samsung.com>");
MODULE_DESCRIPTION("Driver for SiI-9244 devices");
MODULE_LICENSE("GPL");

module_init(mhl_init);
module_exit(mhl_exit);
