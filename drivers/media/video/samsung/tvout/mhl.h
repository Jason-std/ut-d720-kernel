/***********************************************************************************/
/*  Copyright (c) 2010, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/
#ifndef __SII_9244_API_H__
#define __SII_9244_API_H__

#define MHLDEBUG  //yqf test
#ifdef MHLDEBUG
#define MHLPRINTK(fmt, args...) \
	printk(KERN_ERR/*INFO*/"  [MHL] %s: " fmt, __func__ , ## args) //yqf
#else
#define MHLPRINTK(fmt, args...)
#endif

#define TX_DEBUG_PRINT(x) MHLPRINTK x


/*
* Support RCP function, need keypad driver support. Jiangshanbin
*/
#define  SUPPORT_MHL_RCP

typedef	bool	bool_t;

#define LOW			0
#define HIGH			1

/*=================================
  Generic Masks
=================================*/
#define _ZERO			0x00
#define BIT_0			0x01
#define BIT_1			0x02
#define BIT_2			0x04
#define BIT_3			0x08
#define BIT_4			0x10
#define BIT_5			0x20
#define BIT_6			0x40
#define BIT_7                   0x80

/* Timers - Target system uses these timers*/
#define ELAPSED_TIMER		0xFF
#define ELAPSED_TIMER1		0xFE

enum TimerId {
    TIMER_0 = 0,		/* DO NOT USE - reserved for TimerWait()*/
    TIMER_POLLING,		/* Reserved for main polling loop*/
    TIMER_2,			/* Available*/
    TIMER_3,			/* Available*/
    TIMER_4,			/* Available*/
    TIMER_5,			/* Available*/
    TIMER_COUNT,			/* MUST BE LAST!!!!*/
};

#define T_MONITORING_PERIOD		100
/*
* This is the time in milliseconds we poll what we poll.
*/
#define MONITORING_PERIOD		 3 // 3 /*->13->50*/  //yqf, 3->50:refer to fw

#define SiI_DEVICE_ID			0xB0

#define TX_HW_RESET_PERIOD		10

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* Debug Definitions
 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#define DISABLE 0x00
#define ENABLE  0xFF

extern bool_t	vbusPowerState;
/*------------------------------------------------------------------------------
* Array of timer values
------------------------------------------------------------------------------*/

extern uint16_t g_timerCounters[TIMER_COUNT];

extern uint16_t g_timerElapsed;
extern uint16_t g_elapsedTick;
extern uint16_t g_timerElapsedGranularity;

extern uint16_t g_timerElapsed1;
extern uint16_t g_elapsedTick1;
extern uint16_t g_timerElapsedGranularity1;

void HalTimerSet(uint8_t index, uint16_t m_sec);
uint8_t HalTimerExpired(uint8_t index);
void HalTimerWait(uint16_t m_sec);
uint16_t HalTimerElapsed(uint8_t index);


/*
*
* AppMhlTxDisableInterrupts
*
* This function or macro is invoked from MhlTx driver to secure the processor
* before entering into a critical region.
*
* Application module must provide this function.
*/
extern void AppMhlTxDisableInterrupts(void);

/*
*
* AppMhlTxRestoreInterrupts
*
* This function or macro is invoked from MhlTx driver to secure the processor
* before entering into a critical region.
*
* Application module must provide this function.
*/
extern void AppMhlTxRestoreInterrupts(void);

/*
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
*/
extern void AppVbusControl(bool powerOn);

/*
	PM check the cable state.
*/
extern void s3c_cable_check_status(int flag);

/*
	I2C client for sii9244
*/
extern struct i2c_client *mhl_page0;
extern struct i2c_client *mhl_page1;
extern struct i2c_client *mhl_page2;
extern struct i2c_client *mhl_cbus;

void work_queue(void);

s32 mhl_i2c_write(struct i2c_client *i2c, char reg, const u8 val);

uint8_t ReadBytePage0(uint8_t Offset);
uint8_t ReadBytePage1(uint8_t Offset);
uint8_t ReadBytePage2(uint8_t Offset);
uint8_t ReadByteCBUS(uint8_t Offset);

void WriteBytePage0(uint8_t Offset, uint8_t Data);
void WriteBytePage1(uint8_t Offset, uint8_t Data);
void WriteBytePage2(uint8_t Offset, uint8_t Data);
void WriteByteCBUS(uint8_t Offset, uint8_t Data);

void ReadModifyWritePage0(uint8_t Offset, uint8_t Mask, uint8_t Data);

void SET_BIT(struct i2c_client *i2c, uint8_t Offset, uint8_t bitnumber);
void CLR_BIT(struct i2c_client *i2c, uint8_t Offset, uint8_t bitnumber);

#define MHL_DEVICE_CATEGORY	(MHL_DEV_CAT_SOURCE)
#define MHL_LOGICAL_DEVICE_MAP \
    (MHL_DEV_LD_AUDIO | MHL_DEV_LD_VIDEO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_GUI)

/*
* 90[0] = Enable / Disable MHL Discovery on MHL link
*/
#define	DISABLE_DISCOVERY		CLR_BIT(mhl_page0, 0x90, 0);
#define	ENABLE_DISCOVERY		SET_BIT(mhl_page0, 0x90, 0);

#define	STROBE_POWER_ON          CLR_BIT(mhl_page0, 0x90, 1);

/*
*	Look for interrupts on INTR4 (Register 0x74)
*		7 = RSVD		(reserved)
*		6 = RGND Rdy	(interested)
*		5 = VBUS Low	(ignore)
*		4 = CBUS LKOUT	(interested)
*		3 = USB EST		(interested)
*		2 = MHL EST		(interested)
*		1 = RPWR5V Change	(ignore)
*		0 = SCDT Change	(only if necessary)
*/
#define	INTR_4_DESIRED_MASK \
	(BIT_0 | BIT_2 | BIT_3 | BIT_4 | BIT_5 | BIT_6)
#define	UNMASK_INTR_4_INTERRUPTS \
	mhl_i2c_write(mhl_page0, 0x78, INTR_4_DESIRED_MASK)
#define	MASK_INTR_4_INTERRUPTS		mhl_i2c_write(mhl_page0, 0x78, 0x00)

/*	Look for interrupts on INTR_1 (Register 0x71)
*		7 = RSVD		(reserved)
*		6 = MDI_HPD		(interested)
*		5 = RSEN CHANGED(interested)
*		4 = RSVD		(reserved)
*		3 = RSVD		(reserved)
*		2 = RSVD		(reserved)
*		1 = RSVD		(reserved)
*		0 = RSVD		(reserved)
*/
#define	INTR_1_DESIRED_MASK		(BIT_5)
#define	UNMASK_INTR_1_INTERRUPTS \
	mhl_i2c_write(mhl_page0, 0x75, INTR_1_DESIRED_MASK)
#define	MASK_INTR_1_INTERRUPTS		mhl_i2c_write(mhl_page0, 0x75, 0x00)

/*	Look for interrupts on CBUS:CBUS_INTR_STATUS [0xC8:0x08]
*		7 = RSVD			(reserved)
*		6 = MSC_RESP_ABORT	(interested)
*		5 = MSC_REQ_ABORT	(interested)
*		4 = MSC_REQ_DONE	(interested)
*		3 = MSC_MSG_RCVD	(interested)
*		2 = DDC_ABORT		(interested)
*		1 = RSVD		(reserved)
*		0 = rsvd		(reserved)
*/
#define	INTR_CBUS1_DESIRED_MASK		(BIT_2 | BIT_3 | BIT_4 | BIT_5 | BIT_6)
#define	UNMASK_CBUS1_INTERRUPTS	\
	mhl_i2c_write(mhl_cbus, 0x09, INTR_CBUS1_DESIRED_MASK)
#define	MASK_CBUS1_INTERRUPTS		mhl_i2c_write(mhl_cbus, 0x09, 0x00)

/*	Look for interrupts on CBUS:CBUS_INTR_STATUS2 [0xC8:0x1E]
*		7 = RSVD			(reserved)
*		6 = RSVD			(reserved)
*		5 = RSVD			(reserved)
*		4 = RSVD			(reserved)
*		3 = WRT_STAT_RECD	(interested)
*		2 = SET_INT_RECD	(interested)
*		1 = RSVD			(reserved)
*		0 = WRT_BURST_RECD (interested)
*/
#define	INTR_CBUS2_DESIRED_MASK		(BIT_2 | BIT_3)
#define	UNMASK_CBUS2_INTERRUPTS	\
	mhl_i2c_write(mhl_cbus, 0x1F, INTR_CBUS2_DESIRED_MASK)
#define	MASK_CBUS2_INTERRUPTS		mhl_i2c_write(mhl_cbus, 0x1F, 0x00)

#endif  /* __SII_9244_API_H__*/
