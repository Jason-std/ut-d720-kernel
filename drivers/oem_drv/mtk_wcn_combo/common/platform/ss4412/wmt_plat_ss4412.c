/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */


/*! \file
    \brief  Declaration of library functions

    Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
*/

/*******************************************************************************
* Copyright (c) 2009 MediaTek Inc.
*
* All rights reserved. Copying, compilation, modification, distribution
* or any other use whatsoever of this material is strictly prohibited
* except in accordance with a Software License Agreement with
* MediaTek Inc.
********************************************************************************
*/

/*******************************************************************************
* LEGAL DISCLAIMER
*
* BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND
* AGREES THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK
* SOFTWARE") RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE
* PROVIDED TO BUYER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY
* DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT
* LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
* PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE
* ANY WARRANTY WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY
* WHICH MAY BE USED BY, INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK
* SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY
* WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE
* FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S SPECIFICATION OR TO
* CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
* BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
* LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL
* BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT
* ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY
* BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
* THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
* WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT
* OF LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING
* THEREOF AND RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN
* FRANCISCO, CA, UNDER THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE
* (ICC).
********************************************************************************
*/


/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#include <asm/mach-types.h>
#include <linux/delay.h>
#include <mach/gpio.h>
//#include <mach/io.h> 
#include <linux/platform_device.h>

#if CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#define CFG_WMT_WAKELOCK_SUPPORT 1
#endif

/* added by Albert_lee 2013-03-12 begin */

#define CFG_USE_GPIOLIB (1)
#define CFG_SET_IRQ_WAKE_ONLY_ONCE (1)
#define FM_ANALOG_INPUT (1)

#include <plat/gpio-cfg.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
 #include <linux/interrupt.h>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <mach/irqs.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
 /*Albert_lee 2013-03-12 end*/


#ifdef DFT_TAG
#undef DFT_TAG
#endif
#define DFT_TAG         "[WMT-PLAT]"


/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*header files*/
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/spinlock.h>

/* MTK_WCN_COMBO header files */
#include "wmt_plat.h"
#include "wmt_exp.h"
#include "mtk_wcn_cmb_hw.h"

extern int s3c_wlan_set_carddetect(int onoff);
/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/


#define GPIO_MT6620_PMUEN		GPIO_COMBO_PMUEN
#define GPIO_MT6620_SYSRST		GPIO_COMBO_RST
#define GPIO_MT6620_LDO_EN     GPIO_COMBO_LDO

#define GPIO_MT6620_BGF_INT_B     GPIO_BT_INT
#define GPIO_MT6620_WIFI_INT_B     GPIO_WIFI_WUP
#define INT_6620              IRQ_BT_INT

#define GPIO_COMBO_URXD_PIN ( EXYNOS4_GPA0(0) )
#define GPIO_COMBO_URXD_PIN_GRPIDX (0) /* exynos4_gpio_common_4bit[17] */
#define GPIO_COMBO_UTXD_PIN ( EXYNOS4_GPA0(1) )
#define GPIO_COMBO_UTXD_PIN_GRPIDX (0) /* exynos4_gpio_common_4bit[17] */



/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

static VOID wmt_plat_func_ctrl (UINT32 type, UINT32 on);
static VOID wmt_plat_bgf_eirq_cb (VOID);

static INT32 wmt_plat_ldo_ctrl (ENUM_PIN_STATE state);
static INT32 wmt_plat_pmu_ctrl (ENUM_PIN_STATE state);
static INT32 wmt_plat_rtc_ctrl (ENUM_PIN_STATE state);
static INT32 wmt_plat_rst_ctrl (ENUM_PIN_STATE state);
static INT32 wmt_plat_bgf_eint_ctrl (ENUM_PIN_STATE state);
static INT32 wmt_plat_wifi_eint_ctrl (ENUM_PIN_STATE state);
static INT32 wmt_plat_all_eint_ctrl (ENUM_PIN_STATE state);
static INT32 wmt_plat_uart_ctrl (ENUM_PIN_STATE state);
static INT32 wmt_plat_pcm_ctrl (ENUM_PIN_STATE state);
static INT32 wmt_plat_i2s_ctrl (ENUM_PIN_STATE state);
static INT32 wmt_plat_sdio_pin_ctrl (ENUM_PIN_STATE state);
static INT32 wmt_plat_gps_sync_ctrl (ENUM_PIN_STATE state);
static INT32 wmt_plat_gps_lna_ctrl (ENUM_PIN_STATE state);

static INT32 wmt_plat_dump_pin_conf (VOID);



/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
UINT32 gWmtDbgLvl = WMT_LOG_DBG;//WMT_LOG_INFO;

unsigned int g_bgf_irq;
/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/


static DEFINE_MUTEX(wlan_bt_mutex_lock);

void wlan_bt_lock(void)
{
//	mutex_lock(&wlan_bt_mutex_lock);
}

void wlan_bt_unlock(void)
{
//	mutex_unlock(&wlan_bt_mutex_lock);
}

#if CFG_WMT_WAKELOCK_SUPPORT
static OSAL_SLEEPABLE_LOCK gOsSLock;
static struct wake_lock wmtWakeLock;
static int wmtWakeLock_init_ok = 0;
#endif

irq_cb wmt_plat_bgf_irq_cb = NULL;
device_audio_if_cb wmt_plat_audio_if_cb = NULL;
const static fp_set_pin gfp_set_pin_table[] =
{
    [PIN_LDO] = wmt_plat_ldo_ctrl,
    [PIN_PMU] = wmt_plat_pmu_ctrl,
    [PIN_RTC] = wmt_plat_rtc_ctrl,
    [PIN_RST] = wmt_plat_rst_ctrl,
    [PIN_BGF_EINT] = wmt_plat_bgf_eint_ctrl,
    [PIN_WIFI_EINT] = wmt_plat_wifi_eint_ctrl,
    [PIN_ALL_EINT] = wmt_plat_all_eint_ctrl,
    [PIN_UART_GRP] = wmt_plat_uart_ctrl,
    [PIN_PCM_GRP] = wmt_plat_pcm_ctrl,
    [PIN_I2S_GRP] = wmt_plat_i2s_ctrl,
    [PIN_SDIO_GRP] = wmt_plat_sdio_pin_ctrl,
    [PIN_GPS_SYNC] = wmt_plat_gps_sync_ctrl,
    [PIN_GPS_LNA] = wmt_plat_gps_lna_ctrl,

};


/* added by Albert_lee 2013-03-12 begin */



static unsigned int wlan_sdio_on_table[][4] = {
        {GPIO_WLAN_SDIO_CLK, GPIO_WLAN_SDIO_CLK_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_UP},
        {GPIO_WLAN_SDIO_CMD, GPIO_WLAN_SDIO_CMD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_UP},
        {GPIO_WLAN_SDIO_D0, GPIO_WLAN_SDIO_D0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_UP},
        {GPIO_WLAN_SDIO_D1, GPIO_WLAN_SDIO_D1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_UP},
        {GPIO_WLAN_SDIO_D2, GPIO_WLAN_SDIO_D2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_UP},
        {GPIO_WLAN_SDIO_D3, GPIO_WLAN_SDIO_D3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_UP},
};

static unsigned int wlan_sdio_off_table[][4] = {
        {GPIO_WLAN_SDIO_CLK, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_D0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_D1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_D2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_D3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
};

static void s3c_config_gpio_alive_table(int array_size, unsigned int (*gpio_table)[4])
{
	u32 i, gpio;
	//printk("gpio_table = [%d] \r\n" , array_size);
	for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
		s3c_gpio_setpull(gpio, gpio_table[i][3]);
		if (gpio_table[i][2] != GPIO_LEVEL_NONE)
		    gpio_set_value(gpio, gpio_table[i][2]);
	}
}

 /*Albert_lee 2013-03-12 end*/




/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/*!
 * \brief audio control callback function for CMB_STUB on ALPS
 *
 * A platform function required for dynamic binding with CMB_STUB on ALPS.
 *
 * \param state desired audio interface state to use
 * \param flag audio interface control options
 *
 * \retval 0 operation success
 * \retval -1 invalid parameters
 * \retval < 0 error for operation fail
 */
INT32 wmt_plat_audio_ctrl (CMB_STUB_AIF_X state, CMB_STUB_AIF_CTRL ctrl)
{
    INT32 iRet = 0;
    UINT32 pinShare;

    /* input sanity check */
    if ( (CMB_STUB_AIF_MAX <= state)
        || (CMB_STUB_AIF_CTRL_MAX <= ctrl) ) {
        iRet = -1;
        WMT_ERR_FUNC("WMT-PLAT: invalid para, state(%d), ctrl(%d),iRet(%d) \n", state, ctrl, iRet);
        return iRet;
    }
    if (0/*I2S/PCM share pin*/) {
        // TODO: [FixMe][GeorgeKuo] how about MT6575? The following is applied to MT6573E1 only!!
        pinShare = 1;
        WMT_INFO_FUNC( "PCM/I2S pin share\n");
    }
    else{ //E1 later
        pinShare = 0;
        WMT_INFO_FUNC( "PCM/I2S pin seperate\n");
    }

    iRet = 0;

    /* set host side first */
    switch (state) {
    case CMB_STUB_AIF_0:
        /* BT_PCM_OFF & FM line in/out */
        iRet += wmt_plat_gpio_ctrl(PIN_PCM_GRP, PIN_STA_DEINIT);
        iRet += wmt_plat_gpio_ctrl(PIN_I2S_GRP, PIN_STA_DEINIT);
        break;

    case CMB_STUB_AIF_1:
        iRet += wmt_plat_gpio_ctrl(PIN_PCM_GRP, PIN_STA_INIT);
        iRet += wmt_plat_gpio_ctrl(PIN_I2S_GRP, PIN_STA_DEINIT);
        break;

    case CMB_STUB_AIF_2:
        iRet += wmt_plat_gpio_ctrl(PIN_PCM_GRP, PIN_STA_DEINIT);
        iRet += wmt_plat_gpio_ctrl(PIN_I2S_GRP, PIN_STA_INIT);
        break;

    case CMB_STUB_AIF_3:
        iRet += wmt_plat_gpio_ctrl(PIN_PCM_GRP, PIN_STA_INIT);
        iRet += wmt_plat_gpio_ctrl(PIN_I2S_GRP, PIN_STA_INIT);
        break;

    default:
        /* FIXME: move to cust folder? */
        WMT_ERR_FUNC("invalid state [%d]\n", state);
        return -1;
        break;
    }

    if (CMB_STUB_AIF_CTRL_EN == ctrl) {
        WMT_INFO_FUNC("call chip aif setting \n");
        /* need to control chip side GPIO */
        //iRet += wmt_lib_set_aif(state, (pinShare) ? MTK_WCN_BOOL_TRUE : MTK_WCN_BOOL_FALSE);
       if (NULL != wmt_plat_audio_if_cb)
       {
           iRet += (*wmt_plat_audio_if_cb)(state, (pinShare) ? MTK_WCN_BOOL_TRUE : MTK_WCN_BOOL_FALSE);
       }
       else
       {
           WMT_WARN_FUNC("wmt_plat_audio_if_cb is not registered \n");
           iRet -= 1;
       }
    }
    else {
        WMT_INFO_FUNC("skip chip aif setting \n");
    }
    return iRet;

}

#if CFG_WMT_PS_SUPPORT
irqreturn_t irq_handler(int i, void *arg)
{
    int pin_state;

    /* set to input mode to check real pin state, */
    s3c_gpio_cfgpin(GPIO_MT6620_BGF_INT_B, S3C_GPIO_SFN(0));

    pin_state = gpio_get_value(GPIO_MT6620_BGF_INT_B);
    /* restore to interrupt mode */
    s3c_gpio_cfgpin(GPIO_MT6620_BGF_INT_B, S3C_GPIO_SFN(0xF));

    if (likely(pin_state == 0)) {
        wmt_plat_bgf_eirq_cb();
    }
    else {
        printk(KERN_INFO "%s bgf_irq false alarm:pin_state(%d)!\n", __FUNCTION__, pin_state);
    }
    return IRQ_HANDLED;
}
#else
/* added by Albert_lee 2013-03-16 begin */
irqreturn_t irq_handler(int i, void *arg)
{
//    wmt_plat_bgf_eirq_cb();
    return IRQ_HANDLED;
}
 /*Albert_lee 2013-03-16 end*/
#endif

static VOID
wmt_plat_bgf_eirq_cb (VOID)
{
	
#if CFG_WMT_PS_SUPPORT
	//#error "need to disable EINT here"
		//wmt_lib_ps_irq_cb();
		if (NULL != wmt_plat_bgf_irq_cb)
		{
			(*(wmt_plat_bgf_irq_cb))();
		}
		else
		{
			WMT_WARN_FUNC("WMT-PLAT: wmt_plat_bgf_irq_cb not registered\n");
		}
#else
		return;
#endif
}



VOID wmt_lib_plat_irq_cb_reg (irq_cb bgf_irq_cb)
{
    wmt_plat_bgf_irq_cb = bgf_irq_cb;
}

VOID wmt_lib_plat_aif_cb_reg (device_audio_if_cb aif_ctrl_cb)
{
    wmt_plat_audio_if_cb = aif_ctrl_cb;
}



INT32
wmt_plat_init (P_PWR_SEQ_TIME pPwrSeqTime)
{
    //CMB_STUB_CB stub_cb;
    /*PWR_SEQ_TIME pwr_seq_time;*/
    INT32 iret;

    //stub_cb.aif_ctrl_cb = wmt_plat_audio_ctrl;
    //stub_cb.func_ctrl_cb = wmt_plat_func_ctrl;
    //stub_cb.size = sizeof(stub_cb);

    /* register to cmb_stub */
    //iret = mtk_wcn_cmb_stub_reg(&stub_cb);

    /* init cmb_hw */
    iret = mtk_wcn_cmb_hw_init(pPwrSeqTime);

    /*init wmt function ctrl wakelock if wake lock is supported by host platform*/
    #ifdef CFG_WMT_WAKELOCK_SUPPORT
    wake_lock_init(&wmtWakeLock, WAKE_LOCK_SUSPEND, "wmtFuncCtrl");
    wmtWakeLock_init_ok = 1;
    osal_sleepable_lock_init(&gOsSLock);
    #endif


/* added by Albert_lee 2013-03-27 begin */
    g_bgf_irq = INT_6620;
 /*Albert_lee 2013-03-27 end*/


    WMT_DBG_FUNC("WMT-PLAT: ALPS platform init (%d)\n", iret);
	
    return 0;
}


INT32
wmt_plat_deinit (VOID)
{
    INT32 iret;

    /* 1. de-init cmb_hw */
    iret = mtk_wcn_cmb_hw_deinit();
    /* 2. unreg to cmb_stub */
    iret += mtk_wcn_cmb_stub_unreg();
    /*3. wmt wakelock deinit*/
    #ifdef CFG_WMT_WAKELOCK_SUPPORT
    if (wmtWakeLock_init_ok) {
        wake_lock_destroy(&wmtWakeLock);
        osal_sleepable_lock_deinit(&gOsSLock);
        WMT_DBG_FUNC("destroy wmtWakeLock\n");
        wmtWakeLock_init_ok = 0;		
    }
    #endif
    WMT_DBG_FUNC("WMT-PLAT: ALPS platform init (%d)\n", iret);

    return 0;
}

INT32 wmt_plat_sdio_ctrl (WMT_SDIO_SLOT_NUM sdioPortType, ENUM_FUNC_STATE on)
{

/* added by Albert_lee 2013-05-14 begin */
    wlan_bt_lock();
 /*Albert_lee 2013-05-14 end*/
    if (FUNC_OFF == on)  {
        /* add control logic here to generate SDIO CARD REMOVAL event to mmc/sd
         * controller. SDIO card removal operation and remove success messages
         * are expected.
         */
/* added by Albert_lee 2013-03-12 begin */
        s3c_wlan_set_carddetect(0);
 /*Albert_lee 2013-03-12 end*/

    }
    else {
        /* add control logic here to generate SDIO CARD INSERTION event to mmc/sd
         * controller. SDIO card detection operation and detect success messages
         * are expected.
         */
/* added by Albert_lee 2013-03-12 begin */
        s3c_wlan_set_carddetect(1);
 /*Albert_lee 2013-03-12 end*/
         
    }

/* added by Albert_lee 2013-05-14 begin */
    wlan_bt_unlock();
 /*Albert_lee 2013-05-14 end*/
 
    return 0;
}

#if 0
INT32
wmt_plat_irq_ctrl (
    ENUM_FUNC_STATE state
    )
{
    return -1;
}
#endif

static INT32
wmt_plat_dump_pin_conf (VOID)
{
    WMT_INFO_FUNC( "[WMT-PLAT]=>dump wmt pin configuration start<=\n");
    WMT_INFO_FUNC( "[WMT-PLAT]=>dump wmt pin configuration emds<=\n");
    return 0;
}


INT32 wmt_plat_pwr_ctrl (
    ENUM_FUNC_STATE state
    )
{
    INT32 ret = -1;

/* added by Albert_lee 2013-05-14 begin */
    wlan_bt_lock();
 /*Albert_lee 2013-05-14 end*/
	
    switch (state) {
    case FUNC_ON:
        // TODO:[ChangeFeature][George] always output this or by request throuth /proc or sysfs?
        wmt_plat_dump_pin_conf();
        ret = mtk_wcn_cmb_hw_pwr_on();
        break;

    case FUNC_OFF:
        ret = mtk_wcn_cmb_hw_pwr_off();
        break;

    case FUNC_RST:
        ret = mtk_wcn_cmb_hw_rst();
        break;

    default:
        WMT_WARN_FUNC("invalid state(%d)\n", state);
        break;
    }


/* added by Albert_lee 2013-05-14 begin */
    wlan_bt_unlock();
 /*Albert_lee 2013-05-14 end*/

    return ret;
}

INT32 wmt_plat_ps_ctrl (ENUM_FUNC_STATE state)
{
    return -1;
}

INT32
wmt_plat_eirq_ctrl (
    ENUM_PIN_ID id,
    ENUM_PIN_STATE state
    )
{
    INT32 iRet;

    // TODO: [ChangeFeature][GeorgeKuo]: use another function to handle this, as done in gpio_ctrls

    if ( (PIN_STA_INIT != state )
        && (PIN_STA_DEINIT != state )
        && (PIN_STA_EINT_EN != state )
        && (PIN_STA_EINT_DIS != state ) ) {
        iRet = -1;
        WMT_WARN_FUNC("WMT-PLAT:invalid PIN_STATE(%d) in eirq_ctrl for PIN(%d), ret(%d) \n", state, id, iRet);
        return iRet;
    }

    iRet = -2;
    switch (id) {
    case PIN_BGF_EINT:
        if (PIN_STA_INIT == state) {
            /* set level trigger (low), debounce count (if any), register irq handler wmt_plat_bgf_eirq_cb() */
            iRet = request_irq(g_bgf_irq , irq_handler,
                IRQF_TRIGGER_LOW, "BGF_INT_B", NULL);
            if (iRet) {
                WMT_ERR_FUNC("request_irq(%d, 0x%p, IRQF_TRIGGER_LOW, NULL) fail(%d)\n",
                    g_bgf_irq, irq_handler, iRet);
                return iRet;
            }
#if !CFG_SET_IRQ_WAKE_ONLY_ONCE
            iRet = enable_irq_wake(g_bgf_irq);
            if (iRet) {
                WMT_WARN_FUNC("enable_irq_wake(%d) fail(%d)\n", g_bgf_irq, iRet);
                iRet = 0;
            }
            else {
                WMT_INFO_FUNC("enable_irq_wake (g_bgf_irq) ++, ret(%d)\n", iRet);
            }
#endif
            WMT_DBG_FUNC("BGFInt (init) \n");
        }	
        else if (PIN_STA_EINT_EN == state) {
            /* UNMASK interrupt! */
#if !CFG_SET_IRQ_WAKE_ONLY_ONCE
            iRet = enable_irq_wake(g_bgf_irq);
            WMT_INFO_FUNC("enable_irq_wake (g_bgf_irq) ++, ret(%d)\n", iRet);
#endif
            enable_irq(g_bgf_irq);
            WMT_DBG_FUNC("BGFInt (en) \n");
        }
        else if (PIN_STA_EINT_DIS == state) {
            /* MASK interrupt! */
            disable_irq_nosync(g_bgf_irq);
#if !CFG_SET_IRQ_WAKE_ONLY_ONCE
            iRet = disable_irq_wake(g_bgf_irq);
            WMT_INFO_FUNC("disable_irq_wake (g_bgf_irq) --, ret(%d)\n", iRet);
#endif
            WMT_DBG_FUNC("BGFInt (dis) \n");
        }
        else {
            /* MASK interrupt! */
            /* de-init: nothing to do? */
            free_irq(g_bgf_irq, NULL);
        }
        iRet = 0;
        break;

    case PIN_ALL_EINT:
        WMT_DBG_FUNC("ALLInt (not used yet) \n");
        iRet = 0;
        break;

    default:
        WMT_WARN_FUNC("WMT-PLAT:unsupported EIRQ(PIN_ID:%d) in eirq_ctrl, ret (%d)\n", id, iRet);
        iRet = -1;
        break;
    }

    return iRet;

}

INT32 wmt_plat_gpio_ctrl (
    ENUM_PIN_ID id,
    ENUM_PIN_STATE state
    )
{
    if ( (PIN_ID_MAX > id)
        && (PIN_STA_MAX > state) ) {

        // TODO: [FixMe][GeorgeKuo] do sanity check to const function table when init and skip checking here
        if (gfp_set_pin_table[id]) {
            return (*(gfp_set_pin_table[id]))(state); /* .handler */
        }
        else {
            WMT_WARN_FUNC("WMT-PLAT: null fp for gpio_ctrl(%d)\n", id);
            return -2;
        }
    }
    WMT_ERR_FUNC("WMT-PLAT:[out of range] id(%d), state (%d)\n", id, state);
    return -1;
}

INT32
wmt_plat_ldo_ctrl (
    ENUM_PIN_STATE state
    )
{
#ifdef GPIO_MT6620_LDO_EN
    switch(state)
    {
    case PIN_STA_INIT:
        /*set to gpio output low, disable pull*/
        WMT_DBG_FUNC("WMT-PLAT:LDO init (out 0) \n");
        gpio_direction_output(GPIO_MT6620_LDO_EN, 0);
        s3c_gpio_setpull(GPIO_MT6620_LDO_EN, S3C_GPIO_PULL_NONE);
#if CFG_USE_GPIOLIB
        gpio_direction_output(GPIO_MT6620_LDO_EN, 0);
#endif
        WMT_DBG_FUNC("LDO init (out 0) \n");
        break;

    case PIN_STA_OUT_H:
        s3c_gpio_cfgpin(GPIO_MT6620_LDO_EN, S3C_GPIO_SFN(1));
        s3c_gpio_setpull(GPIO_MT6620_LDO_EN, S3C_GPIO_PULL_NONE);
#if CFG_USE_GPIOLIB
        gpio_direction_output(GPIO_MT6620_LDO_EN, 1);
#else
#endif
        WMT_DBG_FUNC("LDO (out 1) \n");
        break;

    case PIN_STA_OUT_L:
        s3c_gpio_cfgpin(GPIO_MT6620_LDO_EN, S3C_GPIO_SFN(1));
        s3c_gpio_setpull(GPIO_MT6620_LDO_EN, S3C_GPIO_PULL_NONE);
#if CFG_USE_GPIOLIB
        gpio_direction_output(GPIO_MT6620_LDO_EN, 0);
#else
#endif
        WMT_DBG_FUNC("LDO (out 0) \n");
        break;

    case PIN_STA_IN_L:
    case PIN_STA_DEINIT:
        /*set to gpio input low, pull down enable*/
        s3c_gpio_cfgpin(GPIO_MT6620_LDO_EN, S3C_GPIO_INPUT);
        s3c_gpio_setpull(GPIO_MT6620_LDO_EN, S3C_GPIO_PULL_DOWN);
        WMT_DBG_FUNC("LDO deinit (in pd) \n");
        break;

    default:
        WMT_WARN_FUNC("Warnning, invalid state(%d) on LDO\n", state);
        break;
    }
#else
    WMT_INFO_FUNC("LDO is not used\n");
#endif
	
    return 0;
}

INT32
wmt_plat_pmu_ctrl (
    ENUM_PIN_STATE state
    )
{
    switch(state)
    {
    case PIN_STA_INIT:
        /*set to gpio output low, disable pull*/
        s3c_gpio_cfgpin(GPIO_MT6620_PMUEN, S3C_GPIO_SFN(1));
        s3c_gpio_setpull(GPIO_MT6620_PMUEN, S3C_GPIO_PULL_NONE);
	

#if CFG_USE_GPIOLIB
//        gpio_direction_output(GPIO_COMBO_WIFI_WIFI_PWR_EN_PIN, 0);
        gpio_direction_output(GPIO_MT6620_PMUEN, 0);
#else
#endif

        WMT_DBG_FUNC("PMU init (out 0) \n");
        break;

    case PIN_STA_OUT_H:
        s3c_gpio_cfgpin(GPIO_MT6620_PMUEN, S3C_GPIO_SFN(1));
        s3c_gpio_setpull(GPIO_MT6620_PMUEN, S3C_GPIO_PULL_NONE);

#if CFG_USE_GPIOLIB
        gpio_direction_output(GPIO_MT6620_PMUEN, 1);
#else
		
#endif
        WMT_DBG_FUNC("PMU (out 1) \n");
        break;

    case PIN_STA_OUT_L:
        s3c_gpio_cfgpin(GPIO_MT6620_PMUEN, S3C_GPIO_SFN(1));
        s3c_gpio_setpull(GPIO_MT6620_PMUEN, S3C_GPIO_PULL_NONE);

#if CFG_USE_GPIOLIB
        gpio_direction_output(GPIO_MT6620_PMUEN, 0);
#else
#endif
		
        WMT_DBG_FUNC("PMU (out 0) \n");
        break;

    case PIN_STA_IN_L:
    case PIN_STA_DEINIT:
        /*set to gpio input low, pull down enable*/
        s3c_gpio_cfgpin(GPIO_MT6620_PMUEN, S3C_GPIO_INPUT);
        s3c_gpio_setpull(GPIO_MT6620_PMUEN, S3C_GPIO_PULL_DOWN);

        WMT_DBG_FUNC("PMU deinit (in pd) \n");

        break;

    default:
        printk("WMT-PLAT:Warnning, invalid state(%d) on PMU\n", state);
        break;
    }

    return 0;
}

INT32
wmt_plat_rtc_ctrl (
    ENUM_PIN_STATE state
    )
{
    switch(state)
    {
    case PIN_STA_INIT:
        WMT_DBG_FUNC("WMT-PLAT:RTC init \n");
        break;

    default:
        WMT_WARN_FUNC("WMT-PLAT:Warnning, invalid state(%d) on RTC\n", state);
        break;
    }
    return 0;
}


INT32
wmt_plat_rst_ctrl (
    ENUM_PIN_STATE state
    )
{
    switch(state)
    {
        case PIN_STA_INIT:
            /*set to gpio output low, disable pull*/
            s3c_gpio_cfgpin(GPIO_MT6620_SYSRST, S3C_GPIO_SFN(1));
            s3c_gpio_setpull(GPIO_MT6620_SYSRST, S3C_GPIO_PULL_NONE);
#if CFG_USE_GPIOLIB
            gpio_direction_output(GPIO_MT6620_SYSRST, 0);
#else

#endif
            WMT_DBG_FUNC("RST init (out 0) \n");
			
            break;

        case PIN_STA_OUT_H:
            s3c_gpio_cfgpin(GPIO_MT6620_SYSRST, S3C_GPIO_SFN(1));
            s3c_gpio_setpull(GPIO_MT6620_SYSRST, S3C_GPIO_PULL_NONE);
#if CFG_USE_GPIOLIB
            gpio_direction_output(GPIO_MT6620_SYSRST, 1);
#else
#endif
            printk("WMT-PLAT:RST (out 1) \n");
            break;

        case PIN_STA_OUT_L:
            s3c_gpio_cfgpin(GPIO_MT6620_SYSRST, S3C_GPIO_SFN(1));
            s3c_gpio_setpull(GPIO_MT6620_SYSRST, S3C_GPIO_PULL_NONE);
#if CFG_USE_GPIOLIB
            gpio_direction_output(GPIO_MT6620_SYSRST, 0);
#else

#endif
            printk("WMT-PLAT:RST (out 0) \n");

			
            break;

        case PIN_STA_IN_L:
        case PIN_STA_DEINIT:
            /*set to gpio input low, pull down enable*/
            gpio_direction_output(GPIO_MT6620_SYSRST, 0);
            s3c_gpio_setpull(GPIO_MT6620_SYSRST, S3C_GPIO_PULL_DOWN);
            printk("WMT-PLAT:RST deinit (in pd) \n");
			
            break;

        default:
            printk("WMT-PLAT:Warnning, invalid state(%d) on RST\n", state);
            break;
    }

    return 0;
}

INT32
wmt_plat_bgf_eint_ctrl (
    ENUM_PIN_STATE state
    )
{
#ifdef GPIO_MT6620_BGF_INT_B
    switch(state)
    {
        case PIN_STA_INIT:
            /*set to gpio input low, pull down enable*/
            s3c_gpio_setpull(GPIO_MT6620_BGF_INT_B, S3C_GPIO_PULL_DOWN);
            s3c_gpio_cfgpin(GPIO_MT6620_BGF_INT_B, S3C_GPIO_SFN(0));
            WMT_DBG_FUNC("BGFInt init(in pd) \n");
            break;

        case PIN_STA_MUX:
            s3c_gpio_setpull(GPIO_MT6620_BGF_INT_B, S3C_GPIO_PULL_UP); /* S3C_GPIO_PULL_UP,S3C_GPIO_PULL_NONE */
            s3c_gpio_cfgpin(GPIO_MT6620_BGF_INT_B, S3C_GPIO_SFN(0xF));
            WMT_DBG_FUNC("BGFInt mux (eint+pn) \n");
#if CFG_SET_IRQ_WAKE_ONLY_ONCE
            do {
                int iret;
                iret = enable_irq_wake(g_bgf_irq);
                WMT_INFO_FUNC("enable_irq_wake(bgf:%d)++, ret(%d)\n", g_bgf_irq, iret);
            } while (0);
#endif
            break;

        case PIN_STA_IN_L:
        case PIN_STA_DEINIT:
#if CFG_SET_IRQ_WAKE_ONLY_ONCE
            do {
                int iret;
                iret = disable_irq_wake(g_bgf_irq);
                if (iret) {
                    WMT_WARN_FUNC("disable_irq_wake(bgf:%d) fail(%d)\n", g_bgf_irq, iret);
                    iret = 0;
                }
                else {
                    WMT_INFO_FUNC("disable_irq_wake(bgf:%d)--, ret(%d)\n", g_bgf_irq, iret);
                }
            } while (0);
#endif

 	    /* second: set to gpio input low, pull down enable*/
            s3c_gpio_setpull(GPIO_MT6620_BGF_INT_B, S3C_GPIO_PULL_DOWN);
            s3c_gpio_cfgpin(GPIO_MT6620_BGF_INT_B, S3C_GPIO_SFN(0));
            WMT_DBG_FUNC("BGFInt deinit(in pd) \n");
            break;

        default:
            WMT_WARN_FUNC("WMT-PLAT:Warnning, invalid state(%d) on BGF EINT\n", state);
            break;
    }
#else
    WMT_INFO_FUNC("BGF EINT not defined\n", state);
#endif
    return 0;
}


INT32 wmt_plat_wifi_eint_ctrl(ENUM_PIN_STATE state)
{
#ifdef GPIO_MT6620_WIFI_INT_B
    switch(state)
    {
        case PIN_STA_INIT:
            /*set to gpio input low, pull down enable*/
            s3c_gpio_setpull(GPIO_MT6620_WIFI_INT_B, S3C_GPIO_PULL_DOWN);
            s3c_gpio_cfgpin(GPIO_MT6620_WIFI_INT_B, S3C_GPIO_SFN(0));
            WMT_DBG_FUNC("wifi_eint init(in pd) \n");
            break;

        case PIN_STA_MUX:
            s3c_gpio_setpull(GPIO_MT6620_WIFI_INT_B, S3C_GPIO_PULL_NONE); /* S3C_GPIO_PULL_UP,S3C_GPIO_PULL_NONE */
            s3c_gpio_cfgpin(GPIO_MT6620_WIFI_INT_B, S3C_GPIO_SFN(0xF));
            WMT_DBG_FUNC("wifi_eint mux (eint+pn) \n");
            break;

        case PIN_STA_IN_L:
        case PIN_STA_DEINIT:
            /*set to gpio input low, pull down enable*/
            s3c_gpio_setpull(GPIO_MT6620_WIFI_INT_B, S3C_GPIO_PULL_DOWN);
            s3c_gpio_cfgpin(GPIO_MT6620_WIFI_INT_B, S3C_GPIO_SFN(0));
            WMT_DBG_FUNC("BGFInt deinit(in pd) \n");
            break;

        default:
            WMT_WARN_FUNC("Warnning, invalid state(%d) on WIFI EINT\n", state);
            break;
    }
#endif
    return 0;
}


INT32
wmt_plat_all_eint_ctrl (
    ENUM_PIN_STATE state
    )
{
#ifdef GPIO_COMBO_ALL_EINT_PIN
    switch(state)
    {
        case PIN_STA_INIT:
            mt_set_gpio_mode(GPIO_COMBO_ALL_EINT_PIN, GPIO_COMBO_ALL_EINT_PIN_M_GPIO);
            mt_set_gpio_dir(GPIO_COMBO_ALL_EINT_PIN, GPIO_DIR_IN);
            mt_set_gpio_pull_select(GPIO_COMBO_ALL_EINT_PIN, GPIO_PULL_DOWN);
            mt_set_gpio_pull_enable(GPIO_COMBO_ALL_EINT_PIN, GPIO_PULL_ENABLE);
            WMT_DBG_FUNC("ALLInt init(in pd) \n");
            break;

        case PIN_STA_MUX:
            mt_set_gpio_mode(GPIO_COMBO_ALL_EINT_PIN, GPIO_COMBO_ALL_EINT_PIN_M_GPIO);
            mt_set_gpio_pull_enable(GPIO_COMBO_ALL_EINT_PIN, GPIO_PULL_ENABLE);
            mt_set_gpio_pull_select(GPIO_COMBO_ALL_EINT_PIN, GPIO_PULL_UP);
            mt_set_gpio_mode(GPIO_COMBO_ALL_EINT_PIN, GPIO_COMBO_ALL_EINT_PIN_M_EINT);
            break;

        case PIN_STA_IN_L:
        case PIN_STA_DEINIT:
            /*set to gpio input low, pull down enable*/
            mt_set_gpio_mode(GPIO_COMBO_ALL_EINT_PIN, GPIO_COMBO_ALL_EINT_PIN_M_GPIO);
            mt_set_gpio_dir(GPIO_COMBO_ALL_EINT_PIN, GPIO_DIR_IN);
            mt_set_gpio_pull_select(GPIO_COMBO_ALL_EINT_PIN, GPIO_PULL_DOWN);
            mt_set_gpio_pull_enable(GPIO_COMBO_ALL_EINT_PIN, GPIO_PULL_ENABLE);
            break;

        default:
            WMT_WARN_FUNC("Warnning, invalid state(%d) on ALL EINT\n", state);
            break;
    }
#else
    WMT_INFO_FUNC("ALL EINT not defined\n");
#endif
    return 0;
}

INT32 wmt_plat_uart_ctrl(ENUM_PIN_STATE state)
{
    switch(state)
    {
    case PIN_STA_MUX:
    case PIN_STA_INIT:
        /* pull up */
        s3c_gpio_setpull(GPIO_COMBO_URXD_PIN, S3C_GPIO_PULL_UP);
        s3c_gpio_setpull(GPIO_COMBO_UTXD_PIN, S3C_GPIO_PULL_UP); /* S3C_GPIO_PULL_NONE */
        /* UART-x mode */
        s3c_gpio_cfgpin(GPIO_COMBO_URXD_PIN, S3C_GPIO_SFN(0x2));
        s3c_gpio_cfgpin(GPIO_COMBO_UTXD_PIN, S3C_GPIO_SFN(0x2));
        /* driving */
        s5p_gpio_set_drvstr(GPIO_COMBO_URXD_PIN, S5P_GPIO_DRVSTR_LV4);
        s5p_gpio_set_drvstr(GPIO_COMBO_UTXD_PIN, S5P_GPIO_DRVSTR_LV4);
        WMT_DBG_FUNC("UART init (rx pull up, tx pull up) \n");
        break;
    case PIN_STA_IN_L:
    case PIN_STA_DEINIT:
        /* input pull down or output low */
        s3c_gpio_setpull(GPIO_COMBO_URXD_PIN, S3C_GPIO_PULL_DOWN);
        s3c_gpio_setpull(GPIO_COMBO_UTXD_PIN, S3C_GPIO_PULL_DOWN);
        s3c_gpio_cfgpin(GPIO_COMBO_URXD_PIN, S3C_GPIO_SFN(0));
        s3c_gpio_cfgpin(GPIO_COMBO_UTXD_PIN, S3C_GPIO_SFN(0));
        WMT_DBG_FUNC("UART deinit (rx in-pd, tx in-pd\n");
        break;

    default:
        WMT_WARN_FUNC("WMT-PLAT:Warnning, invalid state(%d) on UART Group\n", state);
        break;
    }

    return 0;
}


INT32 wmt_plat_pcm_ctrl(ENUM_PIN_STATE state)
{
#ifdef GPIO_COMBO_PCM_CLK
    switch(state)
    {
    case PIN_STA_MUX:
    case PIN_STA_INIT:
        mt_set_gpio_mode(GPIO_COMBO_PCM_CLK, GPIO_PCM_DAICLK_PIN_M_CLK);
        mt_set_gpio_mode(GPIO_COMBO_PCM_OUT, GPIO_PCM_DAIPCMOUT_PIN_M_DAIPCMOUT);
        mt_set_gpio_mode(GPIO_COMBO_PCM_IN, GPIO_PCM_DAIPCMIN_PIN_M_DAIPCMIN);
        mt_set_gpio_mode(GPIO_COMBO_PCM_SYNC, GPIO_PCM_DAISYNC_PIN_M_BTSYNC);
        WMT_DBG_FUNC("PCM init (pcm) \n");
        break;

    case PIN_STA_IN_L:
    case PIN_STA_DEINIT:
        mt_set_gpio_mode(GPIO_PCM_DAICLK_PIN, GPIO_PCM_DAICLK_PIN_M_GPIO);
        mt_set_gpio_dir(GPIO_PCM_DAICLK_PIN, GPIO_DIR_OUT);
        mt_set_gpio_out(GPIO_PCM_DAICLK_PIN, GPIO_OUT_ZERO);

        mt_set_gpio_mode(GPIO_PCM_DAIPCMOUT_PIN, GPIO_PCM_DAIPCMOUT_PIN_M_GPIO);
        mt_set_gpio_dir(GPIO_PCM_DAIPCMOUT_PIN, GPIO_DIR_OUT);
        mt_set_gpio_out(GPIO_PCM_DAIPCMOUT_PIN, GPIO_OUT_ZERO);

        mt_set_gpio_mode(GPIO_PCM_DAIPCMIN_PIN, GPIO_PCM_DAIPCMIN_PIN_M_GPIO);
        mt_set_gpio_dir(GPIO_PCM_DAIPCMIN_PIN, GPIO_DIR_OUT);
        mt_set_gpio_out(GPIO_PCM_DAIPCMIN_PIN, GPIO_OUT_ZERO);

        mt_set_gpio_mode(GPIO_PCM_DAISYNC_PIN, GPIO_PCM_DAISYNC_PIN_M_GPIO);
        mt_set_gpio_dir(GPIO_PCM_DAISYNC_PIN, GPIO_DIR_OUT);
        mt_set_gpio_out(GPIO_PCM_DAISYNC_PIN, GPIO_OUT_ZERO);
        WMT_DBG_FUNC("PCM deinit (out 0) \n");
        break;

    default:
        WMT_WARN_FUNC("Warnning, invalid state(%d) on PCM Group\n", state);
        break;
    }

#else
    WMT_INFO_FUNC("GPIO_COMBO_PCM_CLK not defined\n");
#endif
    return 0;
}


INT32 wmt_plat_i2s_ctrl(ENUM_PIN_STATE state)
{
#ifndef FM_ANALOG_INPUT
    switch(state)
    {
    case PIN_STA_INIT:
    case PIN_STA_MUX:
            mt_set_gpio_mode(GPIO_COMBO_I2S_CK_PIN, GPIO_COMBO_I2S_CK_PIN_M_I2S0_CK);
            mt_set_gpio_mode(GPIO_COMBO_I2S_WS_PIN, GPIO_COMBO_I2S_WS_PIN_M_I2S0_WS);
            mt_set_gpio_mode(GPIO_COMBO_I2S_DAT_PIN, GPIO_COMBO_I2S_DAT_PIN_M_I2S0_DAT);
            WMT_DBG_FUNC("I2S init (mode_03, I2S0) \n");
        break;
    case PIN_STA_IN_L:
    case PIN_STA_DEINIT:
            mt_set_gpio_mode(GPIO_COMBO_I2S_CK_PIN, GPIO_COMBO_I2S_CK_PIN_M_GPIO);
            mt_set_gpio_dir(GPIO_COMBO_I2S_CK_PIN, GPIO_DIR_OUT);
            mt_set_gpio_out(GPIO_COMBO_I2S_CK_PIN, GPIO_OUT_ZERO);

            mt_set_gpio_mode(GPIO_COMBO_I2S_WS_PIN, GPIO_COMBO_I2S_WS_PIN_M_GPIO);
            mt_set_gpio_dir(GPIO_COMBO_I2S_WS_PIN, GPIO_DIR_OUT);
            mt_set_gpio_out(GPIO_COMBO_I2S_WS_PIN, GPIO_OUT_ZERO);

            mt_set_gpio_mode(GPIO_COMBO_I2S_DAT_PIN, GPIO_COMBO_I2S_DAT_PIN_M_GPIO);
            mt_set_gpio_dir(GPIO_COMBO_I2S_DAT_PIN, GPIO_DIR_OUT);
            mt_set_gpio_out(GPIO_COMBO_I2S_DAT_PIN, GPIO_OUT_ZERO);
            WMT_DBG_FUNC("I2S deinit (out 0) \n");
        break;
    default:
        WMT_WARN_FUNC("Warnning, invalid state(%d) on I2S Group\n", state);
        break;
    }
#else
        WMT_INFO_FUNC( "FM analog mode is set, skip I2S GPIO settings\n");
#endif

    return 0;
}

INT32
wmt_plat_sdio_pin_ctrl (
    ENUM_PIN_STATE state
    )
{

#if 1
    switch (state) {
    case PIN_STA_INIT:
    case PIN_STA_MUX:
        /* configure sdio bus pins, pull up enable. */
        WMT_DBG_FUNC("SDIO init (pu) \n");
        s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_sdio_on_table), wlan_sdio_on_table);
        break;
    case PIN_STA_DEINIT:
        /* configure sdio bus pins, pull down enable. */
        WMT_DBG_FUNC("SDIO deinit (pd) \n");
        s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_sdio_off_table), wlan_sdio_off_table);
        break;
    default:
        WMT_WARN_FUNC("WMT-PLAT:Warnning, invalid state(%d) on SDIO Group\n", state);
        break;
    }
#endif
    return 0;
}

static INT32
wmt_plat_gps_sync_ctrl (
    ENUM_PIN_STATE state
    )
{
#ifdef GPIO_GPS_SYNC_PIN
    switch (state) {
    case PIN_STA_INIT:
    case PIN_STA_DEINIT:
        mt_set_gpio_mode(GPIO_GPS_SYNC_PIN, GPIO_GPS_SYNC_PIN_M_GPIO);
        mt_set_gpio_dir(GPIO_GPS_SYNC_PIN, GPIO_DIR_OUT);
        mt_set_gpio_out(GPIO_GPS_SYNC_PIN, GPIO_OUT_ZERO);
        break;

    case PIN_STA_MUX:
        mt_set_gpio_mode(GPIO_GPS_SYNC_PIN, GPIO_GPS_SYNC_PIN_M_GPS_SYNC);
        break;

    default:
        break;
    }
#endif
    return 0;
}


static INT32
wmt_plat_gps_lna_ctrl (
        ENUM_PIN_STATE state
        )
{
#ifdef GPIO_GPS_LNA_PIN
    switch (state) {
    case PIN_STA_INIT:
    case PIN_STA_DEINIT:
        mt_set_gpio_pull_enable(GPIO_GPS_LNA_PIN, GPIO_PULL_DISABLE);
        mt_set_gpio_dir(GPIO_GPS_LNA_PIN, GPIO_DIR_OUT);
        mt_set_gpio_mode(GPIO_GPS_LNA_PIN, GPIO_GPS_LNA_PIN_M_GPIO);
        mt_set_gpio_out(GPIO_GPS_LNA_PIN, GPIO_OUT_ZERO);
        break;
    case PIN_STA_OUT_H:
        mt_set_gpio_out(GPIO_GPS_LNA_PIN, GPIO_OUT_ONE);
        break;
    case PIN_STA_OUT_L:
        mt_set_gpio_out(GPIO_GPS_LNA_PIN, GPIO_OUT_ZERO);
        break;

    default:
        WMT_WARN_FUNC("%d mode not defined for  gps lna pin !!!\n", state);
        break;
    }
    return 0;
#else
    WMT_WARN_FUNC("host gps lna pin not defined!!!\n");
    return 0;
#endif
}



INT32 wmt_plat_wake_lock_ctrl(ENUM_WL_OP opId)
{
#ifdef CFG_WMT_WAKELOCK_SUPPORT
    static INT32 counter = 0;


    osal_lock_sleepable_lock( &gOsSLock);
    if (WL_OP_GET == opId)
    {
        ++counter;
    }else if (WL_OP_PUT == opId)
    {
        --counter;
    }
    osal_unlock_sleepable_lock( &gOsSLock);
    if (WL_OP_GET == opId && counter == 1)
    {
        wake_lock(&wmtWakeLock);
        WMT_DBG_FUNC("WMT-PLAT: after wake_lock(%d), counter(%d)\n", wake_lock_active(&wmtWakeLock), counter);

    }
    else if (WL_OP_PUT == opId && counter == 0)
    {
        wake_unlock(&wmtWakeLock);
        WMT_DBG_FUNC("WMT-PLAT: after wake_unlock(%d), counter(%d)\n", wake_lock_active(&wmtWakeLock), counter);
    }
    else
    {
        WMT_WARN_FUNC("WMT-PLAT: wakelock status(%d), counter(%d)\n", wake_lock_active(&wmtWakeLock), counter);
    }
    return 0;
#else
    WMT_WARN_FUNC("WMT-PLAT: host awake function is not supported.");
    return 0;

#endif
}

