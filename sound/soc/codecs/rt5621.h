/*
 * rt5621.h  --  RT5621 ALSA SoC audio driver
 *
 * Copyright 2011 Realtek Microelectronics
 * Author: Johnny Hsu <johnnyhsu@realtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __RT5621_H__
#define __RT5621_H__

#define RT5621_RESET				0x00
#define RT5621_SPK_OUT_VOL			0x02
#define RT5621_HP_OUT_VOL			0x04
#define RT5621_MONO_AUX_OUT_VOL		0x06
#define RT5621_AUXIN_VOL			0x08
#define RT5621_LINE_IN_VOL			0x0a
#define RT5621_STEREO_DAC_VOL			0x0c
#define RT5621_MIC_VOL				0x0e
#define RT5621_MIC_ROUTING_CTRL		0x10
#define RT5621_ADC_REC_GAIN			0x12
#define RT5621_ADC_REC_MIXER			0x14
#define RT5621_SOFT_VOL_CTRL_TIME		0x16
#define RT5621_OUTPUT_MIXER_CTRL		0x1c
#define RT5621_MIC_CTRL				0x22
#define RT5621_AUDIO_INTERFACE			0x34
#define RT5621_STEREO_AD_DA_CLK_CTRL		0x36
#define RT5621_COMPANDING_CTRL		0x38
#define RT5621_PWR_MANAG_ADD1		0x3a
#define RT5621_PWR_MANAG_ADD2		0x3c
#define RT5621_PWR_MANAG_ADD3		0x3e
#define RT5621_ADD_CTRL_REG			0x40
#define RT5621_GLOBAL_CLK_CTRL_REG		0x42
#define RT5621_PLL_CTRL				0x44
#define RT5621_GPIO_OUTPUT_PIN_CTRL		0x4a
#define RT5621_GPIO_PIN_CONFIG			0x4c
#define RT5621_GPIO_PIN_POLARITY		0x4e
#define RT5621_GPIO_PIN_STICKY			0x50
#define RT5621_GPIO_PIN_WAKEUP		0x52
#define RT5621_GPIO_PIN_STATUS			0x54
#define RT5621_GPIO_PIN_SHARING		0x56
#define RT5621_OVER_TEMP_CURR_STATUS		0x58
#define RT5621_JACK_DET_CTRL			0x5a
#define RT5621_MISC_CTRL			0x5e
#define RT5621_PSEDUEO_SPATIAL_CTRL		0x60
#define RT5621_EQ_CTRL				0x62
#define RT5621_EQ_MODE_ENABLE			0x66
#define RT5621_AVC_CTRL				0x68
#define RT5621_HID_CTRL_INDEX			0x6a
#define RT5621_HID_CTRL_DATA			0x6c
#define RT5621_VENDOR_ID1			0x7c
#define RT5621_VENDOR_ID2			0x7e

/* global definition */
#define RT5621_L_MUTE				(0x1 << 15)
#define RT5621_L_MUTE_SFT			15
#define RT5621_L_ZC				(0x1 << 14)
#define RT5621_L_SM				(0x1 << 13)
#define RT5621_L_VOL_MASK			(0x1f << 8)
#define RT5621_L_VOL_SFT			8
#define RT5621_ADCL_VOL_SFT			7
#define RT5621_R_MUTE				(0x1 << 7)
#define RT5621_R_MUTE_SFT			7
#define RT5621_R_ZC				(0x1 << 6)
#define RT5621_R_VOL_MASK			(0x1f)
#define RT5621_R_VOL_SFT			0
#define RT5621_M_HPMIX				(0x1 << 15)
#define RT5621_M_SPKMIX			(0x1 << 14)
#define RT5621_M_MONOMIX			(0x1 << 13)
#define RT5621_SPK_CLASS_AB			0
#define RT5621_SPK_CLASS_D			1

/* AUXIN Volume (0x08) */
#define RT5621_M_AXI_TO_HPM			(0x1 << 15)
#define RT5621_M_AXI_TO_HPM_SFT		15
#define RT5621_M_AXI_TO_SPKM			(0x1 << 14)
#define RT5621_M_AXI_TO_SPKM_SFT		14
#define RT5621_M_AXI_TO_MOM			(0x1 << 13)
#define RT5621_M_AXI_TO_MOM_SFT		13

/* LINE_IN Volume (0x0a) */
#define RT5621_M_LINEIN_TO_HPM		(0x1 << 15)
#define RT5621_M_LINEIN_TO_HPM_SFT		15
#define RT5621_M_LINEIN_TO_SPKM		(0x1 << 14)
#define RT5621_M_LINEIN_TO_SPKM_SFT		14
#define RT5621_M_LINEIN_TO_MOM		(0x1 << 13)
#define RT5621_M_LINEIN_TO_MOM_SFT		13

/* Stereo DAC Volume (0x0c) */
#define RT5621_M_DAC_TO_HPM			(0x1 << 15)
#define RT5621_M_DAC_TO_HPM_SFT		15
#define RT5621_M_DAC_TO_SPKM			(0x1 << 14)
#define RT5621_M_DAC_TO_SPKM_SFT		14
#define RT5621_M_DAC_TO_MOM			(0x1 << 13)
#define RT5621_M_DAC_TO_MOM_SFT		13

/* Mic Routing Control(0x10) */
#define RT5621_M_MIC1_TO_HP_MIXER		(0x1 << 15)
#define RT5621_M_MIC1_TO_HP_MIXER_SFT	15
#define RT5621_M_MIC1_TO_SPK_MIXER		(0x1 << 14)
#define RT5621_M_MIC1_TO_SPK_MIXER_SFT	14
#define RT5621_M_MIC1_TO_MONO_MIXER		(0x1 << 13)
#define RT5621_M_MIC1_TO_MONO_MIXER_SFT	13
#define RT5621_MIC1_DIFF_INPUT_CTRL		(0x1 << 12)
#define RT5621_MIC1_DIFF_INPUT_CTRL_SFT	12
#define RT5621_M_MIC2_TO_HP_MIXER		(0x1 << 7)
#define RT5621_M_MIC2_TO_HP_MIXER_SFT	7
#define RT5621_M_MIC2_TO_SPK_MIXER		(0x1 << 6)
#define RT5621_M_MIC2_TO_SPK_MIXER_SFT	6
#define RT5621_M_MIC2_TO_MONO_MIXER		(0x1 << 5)
#define RT5621_M_MIC2_TO_MONO_MIXER_SFT	5
#define RT5621_MIC2_DIFF_INPUT_CTRL		(0x1 << 4)
#define RT5621_MIC2_DIFF_INPUT_CTRL_SFT	4

/* ADC Record Gain (0x12) */
#define RT5621_M_ADC_L_TO_HP_MIXER		(0x1 << 15)
#define RT5621_M_ADC_L_TO_HP_MIXER_SFT	15
#define RT5621_M_ADC_R_TO_HP_MIXER		(0x1 << 14)
#define RT5621_M_ADC_R_TO_HP_MIXER_SFT	14
#define RT5621_M_ADC_L_TO_MONO_MIXER	(0x1 << 13)
#define RT5621_M_ADC_L_TO_MONO_MIXER_SFT	13
#define RT5621_M_ADC_R_TO_MONO_MIXER	(0x1 << 12)
#define RT5621_M_ADC_R_TO_MONO_MIXER_SFT	12
#define RT5621_ADC_L_GAIN_MASK		(0x1f << 7)
#define RT5621_ADC_L_ZC_DET			(0x1 << 6)
#define RT5621_ADC_R_ZC_DET			(0x1 << 5)
#define RT5621_ADC_R_GAIN_MASK		(0x1f << 0)

/* ADC Input Mixer Control (0x14) */
#define RT5621_M_MIC1_TO_ADC_L_MIXER			(0x1 << 14)
#define RT5621_M_MIC1_TO_ADC_L_MIXER_SFT		14
#define RT5621_M_MIC2_TO_ADC_L_MIXER			(0x1 << 13)
#define RT5621_M_MIC2_TO_ADC_L_MIXER_SFT		13
#define RT5621_M_LINEIN_L_TO_ADC_L_MIXER		(0x1 << 12)
#define RT5621_M_LINEIN_L_TO_ADC_L_MIXER_SFT		12
#define RT5621_M_AUXIN_L_TO_ADC_L_MIXER		(0x1 << 11)
#define RT5621_M_AUXIN_L_TO_ADC_L_MIXER_SFT		11
#define RT5621_M_HPMIXER_L_TO_ADC_L_MIXER		(0x1 << 10)
#define RT5621_M_HPMIXER_L_TO_ADC_L_MIXER_SFT	10
#define RT5621_M_SPKMIXER_L_TO_ADC_L_MIXER		(0x1 << 9)
#define RT5621_M_SPKMIXER_L_TO_ADC_L_MIXER_SFT	9
#define RT5621_M_MONOMIXER_L_TO_ADC_L_MIXER	(0x1 << 8)
#define RT5621_M_MONOMIXER_L_TO_ADC_L_MIXER_SFT	8
#define RT5621_M_MIC1_TO_ADC_R_MIXER			(0x1 << 6)
#define RT5621_M_MIC1_TO_ADC_R_MIXER_SFT		6
#define RT5621_M_MIC2_TO_ADC_R_MIXER			(0x1 << 5)
#define RT5621_M_MIC2_TO_ADC_R_MIXER_SFT		5
#define RT5621_M_LINEIN_R_TO_ADC_R_MIXER		(0x1 << 4)
#define RT5621_M_LINEIN_R_TO_ADC_R_MIXER_SFT		4
#define RT5621_M_AUXIN_R_TO_ADC_R_MIXER		(0x1 << 3)
#define RT5621_M_AUXIN_R_TO_ADC_R_MIXER_SFT		3
#define RT5621_M_HPMIXER_R_TO_ADC_R_MIXER		(0x1 << 2)
#define RT5621_M_HPMIXER_R_TO_ADC_R_MIXER_SFT	2
#define RT5621_M_SPKMIXER_R_TO_ADC_R_MIXER		(0x1 << 1)
#define RT5621_M_SPKMIXER_R_TO_ADC_R_MIXER_SFT	1
#define RT5621_M_MONOMIXER_R_TO_ADC_R_MIXER	(0x1 << 0)
#define RT5621_M_MONOMIXER_R_TO_ADC_R_MIXER_SFT	0

/* Output Mixer Control(0x1c) */
#define RT5621_SPKOUT_N_SOUR_MASK			(0x3 << 14)
#define RT5621_SPKOUT_N_SOUR_SFT			14	
#define RT5621_SPKOUT_N_SOUR_LN			(0x2 << 14)
#define RT5621_SPKOUT_N_SOUR_RP			(0x1 << 14)
#define RT5621_SPKOUT_N_SOUR_RN			(0x0 << 14)
#define RT5621_SPK_OUTPUT_CLASS_MASK			(0x1 << 13)
#define RT5621_SPK_OUTPUT_CLASS_SFT			13
#define RT5621_SPK_OUTPUT_CLASS_AB			(0x0 << 13)
#define RT5621_SPK_OUTPUT_CLASS_D			(0x1 << 13)
#define RT5621_SPK_CLASS_AB_S_AMP			(0x0 << 12)
#define RT5621_SPK_CALSS_AB_W_AMP			(0x1 << 12)
#define RT5621_SPKOUT_INPUT_SEL_MASK			(0x3 << 10)
#define RT5621_SPKOUT_INPUT_SEL_SFT			10
#define RT5621_SPKOUT_INPUT_SEL_MONOMIXER		(0x3 << 10)
#define RT5621_SPKOUT_INPUT_SEL_SPKMIXER		(0x2 << 10)
#define RT5621_SPKOUT_INPUT_SEL_HPMIXER		(0x1 << 10)
#define RT5621_SPKOUT_INPUT_SEL_VMID			(0x0 << 10)
#define RT5621_HPL_INPUT_SEL_HPLMIXER_MASK		(0x1 << 9)
#define RT5621_HPL_INPUT_SEL_HPLMIXER_SFT		9
#define RT5621_HPL_INPUT_SEL_HPLMIXER			(0x1 << 9)
#define RT5621_HPR_INPUT_SEL_HPRMIXER_MASK		(0x1 << 8)
#define RT5621_HPR_INPUT_SEL_HPRMIXER_SFT		8
#define RT5621_HPR_INPUT_SEL_HPRMIXER			(0x1 << 8)	
#define RT5621_MONO_AUX_INPUT_SEL_MASK		(0x3 << 6)
#define RT5621_MONO_AUX_INPUT_SEL_SFT		6
#define RT5621_MONO_AUX_INPUT_SEL_MONO		(0x3 << 6)
#define RT5621_MONO_AUX_INPUT_SEL_SPK		(0x2 << 6)
#define RT5621_MONO_AUX_INPUT_SEL_HP			(0x1 << 6)
#define RT5621_MONO_AUX_INPUT_SEL_VMID		(0x0 << 6)

/* Micphone Control define(0x22) */
#define RT5621_MIC1					1
#define RT5621_MIC2					2
#define RT5621_MIC_BIAS_90_PRECNET_AVDD		1
#define RT5621_MIC_BIAS_75_PRECNET_AVDD		2
#define RT5621_MIC1_BOOST_CTRL_MASK			(0x3 << 10)
#define RT5621_MIC1_BOOST_CTRL_SFT			10
#define RT5621_MIC1_BOOST_CTRL_BYPASS		0x0 << 10)
#define RT5621_MIC1_BOOST_CTRL_20DB			(0x1 << 10)
#define RT5621_MIC1_BOOST_CTRL_30DB			(0x2 << 10)
#define RT5621_MIC1_BOOST_CTRL_40DB			(0x3 << 10)
#define RT5621_MIC2_BOOST_CTRL_MASK			(0x3 << 8)
#define RT5621_MIC2_BOOST_CTRL_SFT			8
#define RT5621_MIC2_BOOST_CTRL_BYPASS		(0x0 << 8)
#define RT5621_MIC2_BOOST_CTRL_20DB			(0x1 << 8)
#define RT5621_MIC2_BOOST_CTRL_30DB			(0x2 << 8)
#define RT5621_MIC2_BOOST_CTRL_40DB			(0x3 << 8)
#define RT5621_MICBIAS_VOLT_CTRL_MASK			(0x1 << 5)
#define RT5621_MICBIAS_VOLT_CTRL_90P			(0x0 << 5)
#define RT5621_MICBIAS_VOLT_CTRL_75P			(0x1 << 5)
#define RT5621_MICBIAS_SHORT_CURR_DET_MASK		(0x3)
#define RT5621_MICBIAS_SHORT_CURR_DET_600UA		(0x0)
#define RT5621_MICBIAS_SHORT_CURR_DET_1200UA	(0x1)
#define RT5621_MICBIAS_SHORT_CURR_DET_1800UA	(0x2)

/* Audio Interface (0x34) */
#define RT5621_SDP_MASTER_MODE			(0x0 << 15)
#define RT5621_SDP_SLAVE_MODE				(0x1 << 15)
#define RT5621_I2S_PCM_MODE				(0x1 << 14)
#define RT5621_MAIN_I2S_BCLK_POL_CTRL			(0x1 << 7)
 /* 0:ADC data appear at left phase of LRCK
  * 1:ADC data appear at right phase of LRCK
  */
#define RT5621_ADC_DATA_L_R_SWAP			(0x1 << 5)
 /* 0:DAC data appear at left phase of LRCK
  * 1:DAC data appear at right phase of LRCK
  */
#define RT5621_DAC_DATA_L_R_SWAP			(0x1 << 4)
#define RT5621_I2S_DL_MASK				(0x3 << 2)
#define RT5621_I2S_DL_16				(0x0 << 2)
#define RT5621_I2S_DL_20				(0x1 << 2)
#define RT5621_I2S_DL_24				(0x2 << 2)
#define RT5621_I2S_DL_32				(0x3 << 2)
#define RT5621_I2S_DF_MASK				(0x3)
#define RT5621_I2S_DF_I2S				(0x0)
#define RT5621_I2S_DF_RIGHT				(0x1)
#define RT5621_I2S_DF_LEFT				(0x2)
#define RT5621_I2S_DF_PCM				(0x3)

/* Stereo AD/DA Clock Control(0x36h) */
#define RT5621_I2S_PRE_DIV_MASK			(0x7 << 12)			
#define RT5621_I2S_PRE_DIV_1				(0x0 << 12)
#define RT5621_I2S_PRE_DIV_2				(0x1 << 12)
#define RT5621_I2S_PRE_DIV_4				(0x2 << 12)
#define RT5621_I2S_PRE_DIV_8				(0x3 << 12)
#define RT5621_I2S_PRE_DIV_16				(0x4 << 12)
#define RT5621_I2S_PRE_DIV_32				(0x5 << 12)
#define RT5621_I2S_SCLK_DIV_MASK			(0x7 << 9)
#define RT5621_I2S_SCLK_DIV_1				(0x0 << 9)
#define RT5621_I2S_SCLK_DIV_2				(0x1 << 9)
#define RT5621_I2S_SCLK_DIV_3				(0x2 << 9)
#define RT5621_I2S_SCLK_DIV_4				(0x3 << 9)
#define RT5621_I2S_SCLK_DIV_6				(0x4 << 9)
#define RT5621_I2S_SCLK_DIV_8				(0x5 << 9)
#define RT5621_I2S_SCLK_DIV_12				(0x6 << 9)
#define RT5621_I2S_SCLK_DIV_16				(0x7 << 9)
#define RT5621_I2S_WCLK_DIV_PRE_MASK			(0xF << 5)
#define RT5621_I2S_WCLK_PRE_DIV_1			(0x0 << 5)
#define RT5621_I2S_WCLK_PRE_DIV_2			(0x1 << 5)
#define RT5621_I2S_WCLK_PRE_DIV_3			(0x2 << 5)
#define RT5621_I2S_WCLK_PRE_DIV_4			(0x3 << 5)
#define RT5621_I2S_WCLK_PRE_DIV_5			(0x4 << 5)
#define RT5621_I2S_WCLK_PRE_DIV_6			(0x5 << 5)
#define RT5621_I2S_WCLK_PRE_DIV_7			(0x6 << 5)
#define RT5621_I2S_WCLK_PRE_DIV_8			(0x7 << 5)
#define RT5621_I2S_WCLK_DIV_MASK			(0x7 << 2)
#define RT5621_I2S_WCLK_DIV_2				(0x0 << 2)
#define RT5621_I2S_WCLK_DIV_4				(0x1 << 2)
#define RT5621_I2S_WCLK_DIV_8				(0x2 << 2)
#define RT5621_I2S_WCLK_DIV_16				(0x3 << 2)
#define RT5621_I2S_WCLK_DIV_32				(0x4 << 2)
#define RT5621_ADDA_FILTER_CLK_SEL_256FS		(0 << 1)
#define RT5621_ADDA_FILTER_CLK_SEL_384FS		(1 << 1)
#define RT5621_ADDA_OSR_SEL_64FS			(0)
#define RT5621_ADDA_OSR_SEL_128FS			(1)

/* Power managment addition 1 (0x3a) */
#define RT5621_PWR_MAIN_I2S_EN			(0x1 << 15)
#define RT5621_PWR_MAIN_I2S_EN_BIT			15
#define RT5621_PWR_ZC_DET_PD_EN			(0x1 << 14)	
#define RT5621_PWR_ZC_DET_PD_EN_BIT			14	
#define RT5621_PWR_MIC1_BIAS_EN			(0x1 << 11)
#define RT5621_PWR_MIC1_BIAS_EN_BIT			11
#define RT5621_PWR_SHORT_CURR_DET_EN			(0x1 << 10)
#define RT5621_PWR_SHORT_CURR_DET_EN_BIT		10
#define RT5621_PWR_SOFTGEN_EN				(0x1 << 8)
#define RT5621_PWR_SOFTGEN_EN_BIT			8
#define RT5621_PWR_DEPOP_BUF_HP			(0x1 << 6)
#define RT5621_PWR_DEPOP_BUF_HP_BIT			6
#define RT5621_PWR_HP_OUT_AMP			(0x1 << 5)
#define RT5621_PWR_HP_OUT_AMP_BIT			5
#define RT5621_PWR_HP_OUT_ENH_AMP			(0x1 << 4)
#define RT5621_PWR_HP_OUT_ENH_AMP_BIT		4
#define RT5621_PWR_DEPOP_BUF_AUX			(0x1 << 2)
#define RT5621_PWR_DEPOP_BUF_AUX_BIT			2
#define RT5621_PWR_AUX_OUT_AMP			(0x1 << 1)
#define RT5621_PWR_AUX_OUT_AMP_BIT			1
#define RT5621_PWR_AUX_OUT_ENH_AMP			(0x1)
#define RT5621_PWR_AUX_OUT_ENH_AMP_BIT		0

/* Power managment addition 2 (0x3c) */
#define RT5621_PWR_CLASS_AB				(0x1 << 15)
#define RT5621_PWR_CLASS_AB_BIT			15
#define RT5621_PWR_CLASS_D				(0x1 << 14)
#define RT5621_PWR_CLASS_D_BIT			14
#define RT5621_PWR_VREF				(0x1 << 13)
#define RT5621_PWR_VREF_BIT				13
#define RT5621_PWR_PLL					(0x1 << 12)
#define RT5621_PWR_PLL_BIT				12
#define RT5621_PWR_DAC_REF_CIR			(0x1 << 10)
#define RT5621_PWR_DAC_REF_CIR_BIT			10
#define RT5621_PWR_L_DAC_CLK				(0x1 << 9)
#define RT5621_PWR_L_DAC_CLK_BIT			9
#define RT5621_PWR_R_DAC_CLK				(0x1 << 8)
#define RT5621_PWR_R_DAC_CLK_BIT			8
#define RT5621_PWR_L_ADC_CLK_GAIN			(0x1 << 7)
#define RT5621_PWR_L_ADC_CLK_GAIN_BIT			7
#define RT5621_PWR_R_ADC_CLK_GAIN			(0x1 << 6)
#define RT5621_PWR_R_ADC_CLK_GAIN_BIT		6
#define RT5621_PWR_L_HP_MIXER				(0x1 << 5)
#define RT5621_PWR_L_HP_MIXER_BIT			5
#define RT5621_PWR_R_HP_MIXER				(0x1 << 4)
#define RT5621_PWR_R_HP_MIXER_BIT			4
#define RT5621_PWR_SPK_MIXER				(0x1 << 3)
#define RT5621_PWR_SPK_MIXER_BIT			3
#define RT5621_PWR_MONO_MIXER			(0x1 << 2)
#define RT5621_PWR_MONO_MIXER_BIT			2
#define RT5621_PWR_L_ADC_REC_MIXER			(0x1 << 1)
#define RT5621_PWR_L_ADC_REC_MIXER_BIT		1
#define RT5621_PWR_R_ADC_REC_MIXER			(0x1)
#define RT5621_PWR_R_ADC_REC_MIXER_BIT		0

/* Power managment addition 3 (0x3e) */
#define RT5621_PWR_MAIN_BIAS				(0x1 << 15)
#define RT5621_PWR_MAIN_BIAS_BIT			15
#define RT5621_PWR_AUXOUT_L_VOL_AMP			(0x1 << 14)
#define RT5621_PWR_AUXOUT_L_VOL_AMP_BIT		14
#define RT5621_PWR_AUXOUT_R_VOL_AMP			(0x1 << 13)
#define RT5621_PWR_AUXOUT_R_VOL_AMP_BIT		13
#define RT5621_PWR_SPK_OUT				(0x1 << 12)
#define RT5621_PWR_SPK_OUT_BIT			12
#define RT5621_PWR_HP_L_OUT_VOL			(0x1 << 10)
#define RT5621_PWR_HP_L_OUT_VOL_BIT			10
#define RT5621_PWR_HP_R_OUT_VOL			(0x1 << 9)
#define RT5621_PWR_HP_R_OUT_VOL_BIT			9
#define RT5621_PWR_LINEIN_L_VOL			(0x1 << 7)
#define RT5621_PWR_LINEIN_L_VOL_BIT			7
#define RT5621_PWR_LINEIN_R_VOL			(0x1 << 6)
#define RT5621_PWR_LINEIN_R_VOL_BIT			6
#define RT5621_PWR_AUXIN_L_VOL			(0x1 << 5)
#define RT5621_PWR_AUXIN_L_VOL_BIT			5
#define RT5621_PWR_AUXIN_R_VOL			(0x1 << 4)
#define RT5621_PWR_AUXIN_R_VOL_BIT			4
#define RT5621_PWR_MIC1_FUN_CTRL			(0x1 << 3)
#define RT5621_PWR_MIC1_FUN_CTRL_BIT			3
#define RT5621_PWR_MIC2_FUN_CTRL			(0x1 << 2)
#define RT5621_PWR_MIC2_FUN_CTRL_BIT			2
#define RT5621_PWR_MIC1_BOOST_MIXER			(0x1 << 1)
#define RT5621_PWR_MIC1_BOOST_MIXER_BIT		1
#define RT5621_PWR_MIC2_BOOST_MIXER			(0x1)
#define RT5621_PWR_MIC2_BOOST_MIXER_BIT		0

/* Additional Control Register (0x40) */
#define RT5621_AUXOUT_SEL_DIFF				(0x1 << 15)
#define RT5621_AUXOUT_SEL_SE				(0x1 << 15)
#define RT5621_SPK_AB_AMP_CTRL_MASK			(0x7 << 12)
#define RT5621_SPK_AB_AMP_CTRL_RATIO_225		(0x0 << 12)
#define RT5621_SPK_AB_AMP_CTRL_RATIO_200		(0x1 << 12)
#define RT5621_SPK_AB_AMP_CTRL_RATIO_175		(0x2 << 12)
#define RT5621_SPK_AB_AMP_CTRL_RATIO_150		(0x3 << 12)
#define RT5621_SPK_AB_AMP_CTRL_RATIO_125		(0x4 << 12)
#define RT5621_SPK_AB_AMP_CTRL_RATIO_100		(0x5 << 12)
#define RT5621_SPK_D_AMP_CTRL_MASK			(0x3 << 10)
#define RT5621_SPK_D_AMP_CTRL_RATIO_175		(0x0 << 10)
#define RT5621_SPK_D_AMP_CTRL_RATIO_150		(0x1 << 10)
#define RT5621_SPK_D_AMP_CTRL_RATIO_125		(0x2 << 10)
#define RT5621_SPK_D_AMP_CTRL_RATIO_100		(0x3 << 10)
#define RT5621_STEREO_DAC_HI_PASS_FILTER_EN		(0x1 << 9)
#define RT5621_STEREO_ADC_HI_PASS_FILTER_EN		(0x1 << 8)
#define RT5621_DIG_VOL_BOOST_MASK			(0x3 << 4)
#define RT5621_DIG_VOL_BOOST_0DB			(0x0 << 4)
#define RT5621_DIG_VOL_BOOST_6DB			(0x1 << 4)
#define RT5621_DIG_VOL_BOOST_12DB			(0x2 << 4)
#define RT5621_DIG_VOL_BOOST_18DB			(0x3 << 4)

/* Global Clock Control Register (0x42) */
#define RT5621_SYSCLK_SOUR_SEL_MASK			(0x1 << 15)
#define RT5621_SYSCLK_SOUR_SEL_MCLK			(0x0 << 15)
#define RT5621_SYSCLK_SOUR_SEL_PLL			(0x1 << 15)
#define RT5621_PLLCLK_SOUR_SEL_MCLK			(0x0 << 14)
#define RT5621_PLLCLK_SOUR_SEL_BITCLK			(0x1 << 14)
#define RT5621_PLLCLK_DIV_RATIO_MASK			(0x3 << 1)
#define RT5621_PLLCLK_DIV_RATIO_DIV1			(0x0 << 1)
#define RT5621_PLLCLK_DIV_RATIO_DIV2			(0x1 << 1)
#define RT5621_PLLCLK_DIV_RATIO_DIV4			(0x2 << 1)
#define RT5621_PLLCLK_DIV_RATIO_DIV8			(0x3 << 1)
#define PLLCLK_PRE_DIV1					(0x0)
#define PLLCLK_PRE_DIV2					(0x1)

/* GPIO Pin Configuration (0x4c) */
#define RT5621_GPIO_PIN_MASK				(0x1 << 1)
#define RT5621_GPIO_PIN_SET_INPUT			(0x1 << 1)
#define RT5621_GPIO_PIN_SET_OUTPUT			(0x0 << 1)

/* Pin Sharing (0x56) */
#define RT5621_LINEIN_L_PIN_SHARING			(0x1 << 15)
#define RT5621_LINEIN_L_PIN_AS_LINEIN_L		(0x0 << 15)
#define RT5621_LINEIN_L_PIN_AS_JD1			(0x1 << 15)
#define RT5621_LINEIN_R_PIN_SHARING			(0x1 << 14)
#define RT5621_LINEIN_R_PIN_AS_LINEIN_R		(0x0 << 14)
#define RT5621_LINEIN_R_PIN_AS_JD2			(0x1 << 14)
#define RT5621_GPIO_PIN_SHARE				(0x3)
#define RT5621_GPIO_PIN_AS_GPIO			(0x0)
#define RT5621_GPIO_PIN_AS_IRQOUT			(0x1)
#define RT5621_GPIO_PIN_AS_PLLOUT			(0x3)

/* Jack Detect Control Register (0x5a) */
#define RT5621_JACK_DETECT_MASK			(0x3 << 14)
#define RT5621_JACK_DETECT_USE_JD2			(0x3 << 14)
#define RT5621_JACK_DETECT_USE_JD1			(0x2 << 14)
#define RT5621_JACK_DETECT_USE_GPIO			(0x1 << 14)
#define RT5621_JACK_DETECT_OFF				(0x0 << 14)
#define RT5621_SPK_EN_IN_HI				(0x1 << 11)
#define RT5621_AUX_R_EN_IN_HI				(0x1 << 10)
#define RT5621_AUX_L_EN_IN_HI				(0x1 << 9)
#define RT5621_HP_EN_IN_HI				(0x1 << 8)
#define RT5621_SPK_EN_IN_LO				(0x1 << 7)
#define RT5621_AUX_R_EN_IN_LO				(0x1 << 6)
#define RT5621_AUX_L_EN_IN_LO				(0x1 << 5)
#define RT5621_HP_EN_IN_LO				(0x1 << 4)

/* MISC CONTROL (0x5e) */
#define RT5621_DISABLE_FAST_VREG			(0x1 << 15)
#define RT5621_SPK_CLASS_AB_OC_PD			(0x1 << 13)
#define RT5621_SPK_CLASS_AB_OC_DET			(0x1 << 12)
#define RT5621_HP_DEPOP_MODE3_EN			(0x1 << 10)
#define RT5621_HP_DEPOP_MODE2_EN			(0x1 << 9)
#define RT5621_HP_DEPOP_MODE1_EN			(0x1 << 8)
#define RT5621_AUXOUT_DEPOP_MODE3_EN		(0x1 << 6)
#define RT5621_AUXOUT_DEPOP_MODE2_EN		(0x1 << 5)
#define RT5621_AUXOUT_DEPOP_MODE1_EN		(0x1 << 4)
#define RT5621_M_DAC_L_INPUT				(0x1 << 3)
#define RT5621_M_DAC_R_INPUT				(0x1 << 2)
#define RT5621_IRQOUT_INV_CTRL				(0x1 << 0)

/* Psedueo Stereo & Spatial Effect Block Control (0x60) */
#define RT5621_SPATIAL_CTRL_EN				(0x1 << 15)
#define RT5621_ALL_PASS_FILTER_EN			(0x1 << 14)
#define RT5621_PSEUDO_STEREO_EN			(0x1 << 13)
#define RT5621_STEREO_EXPENSION_EN			(0x1 << 12)
#define RT5621_GAIN_3D_PARA_L_MASK			(0x7 << 9)
#define RT5621_GAIN_3D_PARA_L_1_00			(0x0 << 9)
#define RT5621_GAIN_3D_PARA_L_1_25			(0x1 << 9)
#define RT5621_GAIN_3D_PARA_L_1_50			(0x2 << 9)
#define RT5621_GAIN_3D_PARA_L_1_75			(0x3 << 9)
#define RT5621_GAIN_3D_PARA_L_2_00			(0x4 << 9)
#define RT5621_GAIN_3D_PARA_R_MASK			(0x7 << 6)
#define RT5621_GAIN_3D_PARA_R_1_00			(0x0 << 6)
#define RT5621_GAIN_3D_PARA_R_1_25			(0x1 << 6)
#define RT5621_GAIN_3D_PARA_R_1_50			(0x2 << 6)
#define RT5621_GAIN_3D_PARA_R_1_75			(0x3 << 6)
#define RT5621_GAIN_3D_PARA_R_2_00			(0x4 << 6)
#define RT5621_RATIO_3D_L_MASK				(0x3 << 4)
#define RT5621_RATIO_3D_L_0_0				(0x0 << 4)
#define RT5621_RATIO_3D_L_0_66				(0x1 << 4)
#define RT5621_RATIO_3D_L_1_0				(0x2 << 4)
#define RT5621_RATIO_3D_R_MASK			(0x3 << 2)
#define RT5621_RATIO_3D_R_0_0				(0x0 << 2)
#define RT5621_RATIO_3D_R_0_66				(0x1 << 2)
#define RT5621_RATIO_3D_R_1_0				(0x2 << 2)
#define RT5621_APF_MASK				(0x3)
#define RT5621_APF_FOR_48K				(0x3)
#define RT5621_APF_FOR_44_1K				(0x2)
#define RT5621_APF_FOR_32K				(0x1)

/* EQ CONTROL (0x62) */
#define RT5621_EN_HW_EQ_BLK				(0x1 << 15)
#define RT5621_EN_HW_EQ_HPF_MODE			(0x1 << 14)
#define RT5621_EN_HW_EQ_SOUR				(0x1 << 11)
#define RT5621_EN_HW_EQ_HPF				(0x1 << 4)
#define RT5621_EN_HW_EQ_BP3				(0x1 << 3)
#define RT5621_EN_HW_EQ_BP2				(0x1 << 2)
#define RT5621_EN_HW_EQ_BP1				(0x1 << 1)
#define RT5621_EN_HW_EQ_LPF				(0x1 << 0)

/* EQ Mode Change Enable (0x66) */
#define RT5621_EQ_HPF_CHANGE_EN			(0x1 << 4)
#define RT5621_EQ_BP3_CHANGE_EN			(0x1 << 3)
#define RT5621_EQ_BP2_CHANGE_EN			(0x1 << 2)
#define RT5621_EQ_BP1_CHANGE_EN			(0x1 << 1)
#define RT5621_EQ_LPF_CHANGE_EN			(0x1 << 0)

/* AVC Control (0x68) */
#define RT5621_AVC_ENABLE				(0x1 << 15)
#define RT5621_AVC_TARTGET_SEL_MASK			(0x1 << 14)
#define RT5621_AVC_TARTGET_SEL_R 			(0x1 << 14)
#define RT5621_AVC_TARTGET_SEL_L			(0x0 << 14)


#define RT5621_PLL_FR_MCLK				0
#define RT5621_PLL_FR_BCLK				1\

#define RT5621_CLK_MCLK	3
#define OPCLK_BUSCLK          3

typedef void (*select_route)(struct snd_soc_codec *);
typedef void (*select_mic_route)(struct snd_soc_codec *);

#define DEBUG 1
#if DEBUG
#define rt5621_dbg(fmt,arg...) printk("[RT5621:]"fmt"\n",##arg)
#else
#define rt5621_dbg(fmt,arg...)
#endif


struct wm8978_reg_map{
	u16 reg;
	u16 value;
};
static struct wm8978_reg_map rt5621_main_mic_reg[]={
  {0x12,0xfb97},{0x22,0x0400},{0x10,0xf0f0},{0x14,0x3f3f},
};
static struct wm8978_reg_map rt5621_hp_mic_reg[]={
  {0x12,0xfb97},{0x22,0x0900},{0x10,0xf0f0},{0x14,0x3f3f},
};

static struct wm8978_reg_map rt5621_spk_rcv[]={
	{0x02,0x0000},{0x08,0x6000},{0x3e,0xfe3c},{0x0e,0x0000},{0x22,0x0a00},{0x10,0xd0d0},{0x1c,0x17c0},{0x3c,0xa734},{0x06,0x0000}
};

#endif /* __RT5621_H__ */
