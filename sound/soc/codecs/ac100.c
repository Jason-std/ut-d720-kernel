/*
 * sound\soc\codec\ac100.c
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <linux/io.h>
//#include <mach/sys_config.h>
#include <linux/regulator/consumer.h>
#include <linux/i2c.h>
#include <linux/switch.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <mach/gpio.h>
#include <linux/clk.h>
#include <linux/gpio.h>
//#include <linux/power/scenelock.h>
#include <mach/gpio.h>
#include "ac100.h"
//#include <oem_ut/power_gpio.h>
#include "../../../drivers/oem_drv/power_gpio.h"

/*i2c_bus id, according to your platform i2c bus id to modify it. */
static unsigned int twi_id = 0;
extern char g_selected_codec[];
#define SUNXI_CODEC_NAME	"ac100"

static int req_status = 0;
//static script_item_u item;
static volatile int route_flag = -1;
static volatile int reset_flag = 0;
static int play_running = 0;
static int cap_running = 0;
static int hook_flag1 = 0;
static int hook_flag2 = 0;
static int KEY_VOLUME_FLAG = 0;

#define HEADSET_CHECKCOUNT  (7)




#define sndvir_audio_RATES  (SNDRV_PCM_RATE_8000_192000|SNDRV_PCM_RATE_KNOT)
#define sndvir_audio_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
		                     SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE)

/*for phone call flag*/
static bool codec_analog_phonein_en		   		= false;
static bool codec_analog_mainmic_en    			= false;
static bool codec_analog_phoneout_en          	= false;
static bool codec_lineinin_en          			= false;
static bool codec_lineincap_en         			= false;
static bool codec_analog_headsetmic_en 			= false;
static bool codec_speakerout_en        			= false;

static bool codec_earpieceout_en       = false;

static bool codec_voice_record_en      = false;
static bool codec_headphoneout_en      = false;
static bool codec_phonein_left_en	   = false;
static bool codec_speakerout_lntor_en  = false;
static bool codec_dacphoneout_reduced_en=false;
static bool codec_noise_reduced_adcin_en=false;
/*Digital_bb*/
static bool codec_digital_headsetmic_en = false;
static bool codec_digital_mainmic_en	= false;
static bool codec_digital_phoneout_en	= false;
static bool codec_digital_output_en		= false;
static bool codec_digital_phonein_en 	= false;
static bool codec_digital_bb_clk_format_init = false;

/*bluetooth*/
static bool codec_bt_clk_format 		= false;
static bool codec_bt_out_en 			= false;
static bool bt_bb_button_voice 			= false;

static bool codec_analog_btmic_en 		= false;
static bool codec_analog_btphonein_en 	= false;

static bool codec_digital_btmic_en 		= false;
static bool codec_digital_btphonein_en 	= false;
static bool codec_digital_bb_bt_clk_format = false;

static bool codec_system_bt_capture_en = false;
static bool codec_headsetmic_voicerecord = false;

static bool codec_digitalbb_capture_mic = false;
static bool codec_digitalbb_capture_bt = false;
static bool codec_analogbb_capture_mainmic = false;
static bool codec_analogbb_capture_bt = false;
static bool codec_analogbb_capture_headsetmic = false;

static int codec_speaker_headset_earpiece_en=0;
static struct snd_soc_codec *debug_codec;
static int digital_bb_cap_keytone_used 		= 0;
//static int speaker_val = 0x1d;
static int speaker_val = 0x31; // max=0x1f 31
static int headset_val = 0x3b;

/*
*	codec_lineinin_en == 1, open the linein in(axil/axir in).
*	codec_lineinin_en == 0, close the linein in(axil/axir in).
*/
static int codec_set_lineinin(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int reg_val;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	codec_lineinin_en = ucontrol->value.integer.value[0];

	if (codec_lineinin_en) {
		/*AUXI pre-amplifier gain control*/
		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val |= (0x7<<AUXI_PREG);
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

		/*select the auxi source */
		reg_val =  snd_soc_read(codec, OMIXER_SR);
		reg_val |= (0x1<<RMIXMUTEAUXINR)|(0x1<<LMIXMUTEAUXINL);
		snd_soc_write(codec, OMIXER_SR, reg_val);

		/*enble l/r Analog Output Mixer */
		reg_val =  snd_soc_read(codec, OMIXER_DACA_CTRL);
		reg_val |= (0x1<<LMIXEN)|(0x1<<RMIXEN);
		snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);
	} else {
		/*AUXI pre-amplifier gain control(default value)*/
		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val &= ~(0x7<<AUXI_PREG);
		reg_val |= (0x4<<AUXI_PREG);
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);
		/*select the default source */
		reg_val =  snd_soc_read(codec, OMIXER_SR);
		reg_val &= ~((0x1<<RMIXMUTEAUXINR)|(0x1<<LMIXMUTEAUXINL));
		snd_soc_write(codec, OMIXER_SR, reg_val);

		/*disable l/r Analog Output Mixer */
		reg_val =  snd_soc_read(codec, OMIXER_DACA_CTRL);
		reg_val &= ~((0x1<<LMIXEN)|(0x1<<RMIXEN));
		snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);
	}

	return 0;
}

static int codec_get_lineinin(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_lineinin_en;
	return 0;
}

/*
*	use for linein record
*/
static int codec_set_lineincap(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_lineincap_en = ucontrol->value.integer.value[0];
	return 0;
}

static int codec_get_lineincap(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_lineincap_en;
	return 0;
}

/*
*	codec_speakerout_en == 1, open the speaker.
*	codec_speakerout_en == 0, close the speaker.
*/
static int codec_set_speakerout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int reg_val;
	int i = 0;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	codec_speakerout_en = ucontrol->value.integer.value[0];

	if (codec_speakerout_en) {
		/*select input source*/
		reg_val = snd_soc_read(codec, SPKOUT_CTRL);
		reg_val &= ~((0x1<<RSPKS)|(0x1<<LSPKS));
		snd_soc_write(codec, SPKOUT_CTRL, reg_val);

		/*enble l/r negative output*/
		reg_val = snd_soc_read(codec, SPKOUT_CTRL);
		reg_val |= (0x1<<RSPKINVEN)|(0x1<<LSPKINVEN);
		snd_soc_write(codec, SPKOUT_CTRL, reg_val);

		/*enble l/r spk*/
		reg_val = snd_soc_read(codec, SPKOUT_CTRL);
		reg_val |= (0x1<<RSPK_EN)|(0x1<<LSPK_EN);
		snd_soc_write(codec, SPKOUT_CTRL, reg_val);

		usleep_range(2000, 3000);

		write_power_item_value(POWER_SPK_EN,1);
	//	gpio_set_value(item.gpio.gpio, 1);
		msleep(62);
		for (i = 1; i < 32; i = i + 1) {
			/*set  volume*/
			reg_val = snd_soc_read(codec, SPKOUT_CTRL);
			//reg_val &=(0x1f<<SPK_VOL);
			reg_val |= i<<SPK_VOL;
			snd_soc_write(codec, SPKOUT_CTRL, reg_val);
			if (i%2 == 0)
				usleep_range(2000, 3000);

		}
		codec_headphoneout_en = 0;
		codec_earpieceout_en = 0;
	} else {
		reg_val = snd_soc_read(codec, SPKOUT_CTRL);
		reg_val &= 0x1f;
		for (i = reg_val; i > 0; i = i - 1 ) {
			/*set  volume*/
			reg_val = snd_soc_read(codec, SPKOUT_CTRL);
			reg_val &= ~(0x1f<<SPK_VOL);
			reg_val |= i<<SPK_VOL;
			snd_soc_write(codec, SPKOUT_CTRL, reg_val);
			if (i%2 == 0)
				usleep_range(2000, 3000);
		}
		/*set  volume*/
		reg_val = snd_soc_read(codec, SPKOUT_CTRL);
		reg_val &= ~(0x1f<<SPK_VOL);
		snd_soc_write(codec, SPKOUT_CTRL, reg_val);
		write_power_item_value(POWER_SPK_EN,0);
	//	gpio_set_value(item.gpio.gpio, 0);
		msleep(62);
		/*disable l/r spk*/
		reg_val = snd_soc_read(codec, SPKOUT_CTRL);
		reg_val &= ~((0x1<<RSPK_EN)|(0x1<<LSPK_EN));
		snd_soc_write(codec, SPKOUT_CTRL, reg_val);
	}
	return 0;
}

/*
*	codec_headphoneout_en == 1, open the headphone.
*	codec_headphoneout_en == 0, close the headphone.
*/
static int codec_get_speakerout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_speakerout_en;
	return 0;
}

static int codec_set_headphoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int reg_val;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	codec_headphoneout_en = ucontrol->value.integer.value[0];
	if (codec_headphoneout_en) {
		printk("%s,line:%d\n",__func__,__LINE__);
		/*zero cross*/
		reg_val = snd_soc_read(codec, ADDA_TUNE1);
		reg_val &= ~(0x1<<8);
		snd_soc_write(codec, ADDA_TUNE1, reg_val);

		/*select pa input source*/
		reg_val = snd_soc_read(codec, HPOUT_CTRL);
		reg_val |= (0x1<<RHPS)|(0x1<<LHPS);
		snd_soc_write(codec, HPOUT_CTRL, reg_val);
		/*set headphone volume 0x3b=59*/
//		reg_val = snd_soc_read(codec, HPOUT_CTRL);
//		reg_val &= ~(0x3f<<HP_VOL);
//		reg_val |= (0x39<<HP_VOL);
//		snd_soc_write(codec, HPOUT_CTRL, reg_val);

		/*hardware xzh support*/
		reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
		reg_val |= (0xf<<HPOUTPUTENABLE);
		snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

		/*enable headphone power amplifier*/
		reg_val = snd_soc_read(codec, HPOUT_CTRL);
		reg_val |= (0x1<<HPPA_EN);
		snd_soc_write(codec, HPOUT_CTRL, reg_val);
		msleep(10);
		/*unmute l/r headphone pa*/
		reg_val = snd_soc_read(codec, HPOUT_CTRL);
		reg_val |= (0x1<<RHPPA_MUTE)|(0x1<<LHPPA_MUTE);
		snd_soc_write(codec, HPOUT_CTRL, reg_val);
		codec_speakerout_en = 0;
		codec_earpieceout_en = 0;
	} else {
		/*mute l/r headphone pa*/
		reg_val = snd_soc_read(codec, HPOUT_CTRL);
		reg_val &= ~((0x1<<RHPPA_MUTE)|(0x1<<LHPPA_MUTE));
		snd_soc_write(codec, HPOUT_CTRL, reg_val);

		/*hardware xzh support*/
		reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
		reg_val &= ~(0xf<<HPOUTPUTENABLE);
		snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

		/*disable headphone power amplifier*/
		reg_val = snd_soc_read(codec, HPOUT_CTRL);
		reg_val &= ~(0x1<<HPPA_EN);
		snd_soc_write(codec, HPOUT_CTRL, reg_val);

		/*select default input source*/
		reg_val = snd_soc_read(codec, HPOUT_CTRL);
		reg_val &= ~((0x1<<RHPS)|(0x1<<LHPS));
		snd_soc_write(codec, HPOUT_CTRL, reg_val);

		/*set headphone volume(default value)*/
		reg_val = snd_soc_read(codec, HPOUT_CTRL);
		reg_val &= ~(0x3f<<HP_VOL);
		snd_soc_write(codec, HPOUT_CTRL, reg_val);
	}

	return 0;
}

static int codec_get_headphoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_headphoneout_en;
	return 0;
}

/*
*	codec_earpieceout_en == 1, open the earpiece.
*	codec_earpieceout_en == 0, close the earpiece.
*/
static int codec_set_earpieceout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int reg_val;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	codec_earpieceout_en = ucontrol->value.integer.value[0];
	if (codec_earpieceout_en) {
		/*earpiece input source select Left Analog Mixer*/
		reg_val = snd_soc_read(codec, ESPKOUT_CTRL);
		reg_val |= (0x3<<ESPSR);
		snd_soc_write(codec, ESPKOUT_CTRL, reg_val);

		/*set earpiece volume 0x3b=59*/
		reg_val = snd_soc_read(codec, ESPKOUT_CTRL);
		//reg_val &=~(0x1f<<ESP_VOL);
		reg_val |= (0x1f<<ESP_VOL);
		snd_soc_write(codec, ESPKOUT_CTRL, reg_val);

		/*unmute earpiece line*/
		reg_val = snd_soc_read(codec, ESPKOUT_CTRL);
		reg_val |= (0x1<<ESPPA_MUTE);
		snd_soc_write(codec, ESPKOUT_CTRL, reg_val);

		/*enable earpiece power amplifier*/
		reg_val = snd_soc_read(codec, ESPKOUT_CTRL);
		reg_val |= (0x1<<ESPPA_EN);
		snd_soc_write(codec, ESPKOUT_CTRL, reg_val);

		codec_speakerout_en = 0;
		codec_headphoneout_en = 0;
	} else {
		/*earpiece input source select default value*/
		reg_val = snd_soc_read(codec, ESPKOUT_CTRL);
		reg_val &= ~(0x3<<ESPSR);
		snd_soc_write(codec, ESPKOUT_CTRL, reg_val);

		/*unmute earpiece line*/
		reg_val = snd_soc_read(codec, ESPKOUT_CTRL);
		reg_val &= ~(0x1<<ESPPA_MUTE);
		snd_soc_write(codec, ESPKOUT_CTRL, reg_val);

		/*enable earpiece power amplifier*/
		reg_val = snd_soc_read(codec, ESPKOUT_CTRL);
		reg_val &= ~(0x1<<ESPPA_EN);
		snd_soc_write(codec, ESPKOUT_CTRL, reg_val);

		/*set earpiece volume(default value)*/
		reg_val = snd_soc_read(codec, ESPKOUT_CTRL);
		reg_val &= ~(0x1f<<ESP_VOL);
		snd_soc_write(codec, ESPKOUT_CTRL, reg_val);
	}

	return 0;
}

static int codec_get_earpieceout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_earpieceout_en;
	return 0;
}

/*
*	codec_analog_phonein_en == 1, the phone in is open.
*	while you open one of the device(speaker,earpiece,headphone).
*	you can hear the caller's voice.
*	codec_analog_phonein_en == 0. the phone in is close.
*/
static int codec_analog_set_phonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int reg_val;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	codec_analog_phonein_en = ucontrol->value.integer.value[0];

	if (codec_analog_phonein_en) {
		/*select the line in source */
		reg_val =  snd_soc_read(codec, OMIXER_SR);
		reg_val |= (0x1<<LMIXMUTELINEINLR)|(0x1<<RMIXMUTELINEINLR);
		snd_soc_write(codec, OMIXER_SR, reg_val);

		/*enble l/r Analog Output Mixer */
		reg_val =  snd_soc_read(codec, OMIXER_DACA_CTRL);
		reg_val |= (0x1<<LMIXEN)|(0x1<<RMIXEN);
		snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);
	} else {
		/*select the line in source */
		reg_val =  snd_soc_read(codec, OMIXER_SR);
		reg_val &= ~((0x1<<LMIXMUTELINEINLR)|(0x1<<RMIXMUTELINEINLR));
		snd_soc_write(codec, OMIXER_SR, reg_val);

		/*enble l/r Analog Output Mixer */
		reg_val =  snd_soc_read(codec, OMIXER_DACA_CTRL);
		reg_val &= ~((0x1<<LMIXEN)|(0x1<<RMIXEN));
		snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);
	}

	return 0;
}

static int codec_analog_get_phonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_analog_phonein_en;
	return 0;
}

/*
*	codec_analog_phoneout_en == 1, the phone out is open. receiver can hear the voice what you say.
*	codec_analog_phoneout_en == 0, the phone out is close.
*/
static int codec_analog_set_phoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int reg_val;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	codec_analog_phoneout_en = ucontrol->value.integer.value[0];

	reg_val = snd_soc_read(codec, LOUT_CTRL);
	if (codec_analog_phoneout_en) {
		reg_val |= 0x1<<LINEOUTEN;
	} else {
		reg_val &= ~(0x1<<LINEOUTEN);
	}
	snd_soc_write(codec, LOUT_CTRL, reg_val);

	return 0;
}

static int codec_analog_get_phoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_analog_phoneout_en;
	return 0;
}

/*
*	codec_analog_mainmic_en == 1, open mic1.
*	codec_analog_mainmic_en == 0, close mic1.
*/
static int codec_analog_set_mainmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg_val;
	codec_analog_mainmic_en = ucontrol->value.integer.value[0];

	if (codec_analog_mainmic_en) {
		/*close headset mic(mic2) routeway*/
		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val &= ~(0x1<<MIC2AMPEN);
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

		/*open mic1 routeway*/
		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val |= 0x1<<MIC1AMPEN;
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

		/*MIC1 boost amplifier Gain control*/
	 	 reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
	 	 reg_val |= (0x4<<ADC_MIC1G);
	 	 snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

		/*Master microphone BIAS Enable*/
		reg_val = snd_soc_read(codec, ADC_APC_CTRL);
		reg_val |= (0x1<<MBIASEN);
		snd_soc_write(codec, ADC_APC_CTRL, reg_val);

		/*select the phone out source*/
		reg_val = snd_soc_read(codec, LOUT_CTRL);
		reg_val &= ~(0xf<<LINEOUTS3);
		reg_val |= (0x1<<LINEOUTS0);
		snd_soc_write(codec, LOUT_CTRL, reg_val);
		codec_analog_headsetmic_en = 0;
	} else {
		/*select the phone out source*/
		reg_val = snd_soc_read(codec, LOUT_CTRL);
		//reg_val &= ~(0xf<<LINEOUTS3);
		reg_val &= ~(0x1<<LINEOUTS0);
		snd_soc_write(codec, LOUT_CTRL, reg_val);

		/*Master microphone BIAS disable*/
		reg_val = snd_soc_read(codec, ADC_APC_CTRL);
		reg_val &= ~(0x1<<MBIASEN);
		//reg_val |= (0x1<<MBIASEN);
		snd_soc_write(codec, ADC_APC_CTRL, reg_val);

		/*close mic1 routeway*/
		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val &= ~(0x1<<MIC1AMPEN);
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

		/*MIC1 boost amplifier Gain control(default value)*/
	 	 reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
	 	 reg_val |= (0x4<<ADC_MIC1G);
	 	 snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);
	}

	return 0;
}

static int codec_analog_get_mainmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_analog_mainmic_en;
	return 0;
}

/*
*	codec_voice_record_en == 1, set status.
*	codec_voice_record_en == 0, set status.
*/
static int codec_set_voicerecord(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_voice_record_en = ucontrol->value.integer.value[0];
	return 0;
}

static int codec_get_voicerecord(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_voice_record_en;
	return 0;
}

/*
*	codec_analog_headsetmic_en == 1, open mic2.
*	codec_analog_headsetmic_en == 0, close mic2.
*/
static int codec_analog_set_headsetmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg_val = 0;
	codec_analog_headsetmic_en = ucontrol->value.integer.value[0];

	if (codec_analog_headsetmic_en) {
		/*close  mic1 routeway*/
		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val &= ~(0x1<<MIC1AMPEN);
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

		/*open mic2 routeway*/
		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val |= 0x1<<MIC2AMPEN;
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

		/*MIC2 Source select*/
		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val &= ~( 0x1<<MIC2SLT);
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

		/*MIC2boost amplifier Gain control*/
		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val |= 0x4<<ADC_MIC2G;
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

		/*Headset microphoneBIAS Enable*/
	 	reg_val = snd_soc_read(codec, ADC_APC_CTRL);
		reg_val |= (0x1<<HBIASEN);
		snd_soc_write(codec, ADC_APC_CTRL, reg_val);

		/*Headset microphoneBIAS Current sensor & ADC Enable*/
	 	reg_val = snd_soc_read(codec, ADC_APC_CTRL);
		reg_val |= (0x1<<HBIASADCEN);
		snd_soc_write(codec, ADC_APC_CTRL, reg_val);

		/*select the phone out source*/
		reg_val = snd_soc_read(codec, LOUT_CTRL);
		reg_val &= ~(0xf<<LINEOUTS3);
		reg_val |= (0x1<<LINEOUTS1);
		snd_soc_write(codec, LOUT_CTRL, reg_val);

		codec_analog_mainmic_en = 0;
	} else {
		/*select the phone out source*/
		reg_val = snd_soc_read(codec, LOUT_CTRL);
		reg_val &= ~(0xf<<LINEOUTS3);
		//reg_val |= (0x1<<LINEOUTS1);
		snd_soc_write(codec, LOUT_CTRL, reg_val);

		/*close mic2 routeway*/
		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val &= ~(0x1<<MIC2AMPEN);
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);
	}
	return 0;
}

static int codec_analog_get_headsetmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_analog_headsetmic_en;
	return 0;
}

/*
*	close all phone routeway
*/
static int codec_set_endcall(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int reg_val;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	/*set headphone volume to 0*/
	reg_val = snd_soc_read(codec, HPOUT_CTRL);
	reg_val &= ~(0x3f<<HP_VOL);
	snd_soc_write(codec, HPOUT_CTRL, reg_val);

	/*disable pa*/
	reg_val = snd_soc_read(codec, HPOUT_CTRL);
	reg_val &= ~(0x1<<HPPA_EN);
	snd_soc_write(codec, HPOUT_CTRL, reg_val);

	/*hardware xzh support*/
	reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
	reg_val &= ~(0xf<<HPOUTPUTENABLE);
	snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

	/*unmute l/r headphone pa*/
	reg_val = snd_soc_read(codec, HPOUT_CTRL);
	reg_val &= ~((0x1<<RHPPA_MUTE)|(0x1<<LHPPA_MUTE));
	snd_soc_write(codec, HPOUT_CTRL, reg_val);
	write_power_item_value(POWER_SPK_EN,0);
	//gpio_set_value(item.gpio.gpio, 0);
	usleep_range(2000, 3000);

	/*zero cross*/
	reg_val = snd_soc_read(codec, ADDA_TUNE1);
	reg_val |= (0x1<<8);
	snd_soc_write(codec, ADDA_TUNE1, reg_val);

	reg_val = 0xE880;
	snd_soc_write(codec, SPKOUT_CTRL, reg_val);

	/*earpiece input source select default value*/
	//reg_val = snd_soc_read(codec, ESPKOUT_CTRL);
	reg_val = 0x8200;
	snd_soc_write(codec, ESPKOUT_CTRL, reg_val);

	/*clear LOUT_CTRL (default value)*/
	reg_val = 0x8060;
	snd_soc_write(codec, LOUT_CTRL, reg_val);

	/*clear OMIXER_SR (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, OMIXER_SR, reg_val);

	/*clear OMIXER_DACA_CTRL (default value)*/
	reg_val = 0x0f80;
	snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

	/*clear ADC_SRCBST_CTRL (default value)*/
	reg_val = 0x4444;
	snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

	/*clear ADC_APC_CTRL*/
	reg_val = snd_soc_read(codec, ADC_APC_CTRL);
	reg_val &= ~(0x1<<15);
	reg_val &= ~(0x1<<11);
	snd_soc_write(codec, ADC_APC_CTRL, reg_val);

	/*clear ADC_SRC (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, ADC_SRC, reg_val);

	/*clear DAC_MXR_SRC (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, DAC_MXR_SRC, reg_val);

	/*clear DAC_DIG_CTRL (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, DAC_DIG_CTRL, reg_val);

	/*clear ADC_DIG_CTRL (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, ADC_DIG_CTRL, reg_val);

	/*clear AIF3_SGP_CTRL (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, AIF3_SGP_CTRL, reg_val);

	/*clear AIF3_CLK_CTRL (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, AIF3_CLK_CTRL, reg_val);

	/*clear AIF2_MXR_SRC (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, AIF2_MXR_SRC, reg_val);

	/*clear AIF2_DACDAT_CTRL (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, AIF2_DACDAT_CTRL, reg_val);

	/*clear AIF2_ADCDAT_CTRL (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, AIF2_ADCDAT_CTRL, reg_val);

	/*clear AIF2_CLK_CTRL (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, AIF2_CLK_CTRL, reg_val);

	/*clear AIF2_CLK_CTRL (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, AIF2_CLK_CTRL, reg_val);

	/*clear AIF1_MXR_SRC (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, AIF1_MXR_SRC, reg_val);

	/*clear AIF1_DACDAT_CTRL (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, AIF1_DACDAT_CTRL, reg_val);

	/*clear AIF1_ADCDAT_CTRL (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, AIF1_ADCDAT_CTRL, reg_val);

	/*clear AIF1_CLK_CTRL (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, AIF1_CLK_CTRL, reg_val);

	/*clear AIF_SR_CTRL (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, AIF_SR_CTRL, reg_val);

	/*clear MOD_RST_CTRL (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, MOD_RST_CTRL, reg_val);

	/*clear MOD_CLK_ENA (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, MOD_CLK_ENA, reg_val);

	/*clear SYSCLK_CTRL (default value)*/
	reg_val = 0x0000;
	snd_soc_write(codec, SYSCLK_CTRL, reg_val);

	codec_analog_phonein_en		   	= 0;
	codec_analog_mainmic_en         = 0;
	codec_analog_phoneout_en        = 0;
	codec_lineinin_en          		= 0;
	codec_lineincap_en         		= 0;
	codec_analog_headsetmic_en      = 0;
	codec_speakerout_en        		= 0;
	codec_earpieceout_en       		= 0;
	codec_voice_record_en      		= 0;
	codec_headphoneout_en      		= 0;
	codec_phonein_left_en	   		= 0;
	codec_speakerout_lntor_en  		= 0;
	codec_dacphoneout_reduced_en	= 0;
	codec_noise_reduced_adcin_en	= 0;

	codec_digital_headsetmic_en 	= 0;
	codec_digital_mainmic_en		= 0;
	codec_digital_phoneout_en		= 0;
	codec_digital_output_en			= 0;
	codec_digital_phonein_en 		= 0;
	codec_digital_bb_clk_format_init= 0;

	codec_bt_clk_format 			= 0;
	codec_bt_out_en 				= 0;
	bt_bb_button_voice 				= 0;

	codec_analog_btmic_en 			= 0;
	codec_analog_btphonein_en 		= 0;

	codec_digital_btmic_en 			= 0;
	codec_digital_btphonein_en 		= 0;
	codec_digital_bb_bt_clk_format 	= 0;
#if 0
	/*cldo3:config voltage for aif3*/
	if (aif3_cldo3) {
		regulator_disable(aif3_cldo3);
		regulator_put(aif3_cldo3);
		aif3_cldo3 = NULL;
	}
#endif
	return 0;
}

static int codec_get_endcall(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

/*
*	codec_digital_mainmic_en == 1, open mic1 for digital bb.
*	codec_digital_mainmic_en == 0, close mic1 for digital bb.
*/

static int codec_digital_set_mainmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg_val;
	codec_digital_mainmic_en = ucontrol->value.integer.value[0];

	if (codec_digital_mainmic_en) {
		/*open mic1 routeway*/
		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val |= 0x1<<MIC1AMPEN;
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val &= ~(0x7<<12);
		reg_val |= 0x0<<12;
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

		/*MIC1 boost amplifier Gain control*/
	 	reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
	 	reg_val |= (0x4<<ADC_MIC1G);
	 	snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

		/*Master microphone BIAS Enable*/
		reg_val = snd_soc_read(codec, ADC_APC_CTRL);
		reg_val |= (0x1<<MBIASEN);
		snd_soc_write(codec, ADC_APC_CTRL, reg_val);

		/*left ADC Mixer unMute Control*/
		reg_val = snd_soc_read(codec, ADC_SRC);
		reg_val |= (0x1<<LADCMIXMUTEMIC1BOOST);
		snd_soc_write(codec, ADC_SRC, reg_val);

		/*enable  analog  adc*/
		reg_val = snd_soc_read(codec, ADC_APC_CTRL);
		reg_val |= (0x1<<ADCLEN);
		snd_soc_write(codec, ADC_APC_CTRL, reg_val);

		/*enable  digital adc*/
		reg_val = snd_soc_read(codec, ADC_DIG_CTRL);
		reg_val &= ~((0x1<<ENAD)|(0x1<<ENDM));
		reg_val |= (0x1<<ENAD)|(0x0<<ENDM);
		snd_soc_write(codec, ADC_DIG_CTRL, reg_val);

		/*AIF2_ADCL/R_MXR_SRC*/
		reg_val = snd_soc_read(codec, AIF2_MXR_SRC);
		reg_val |= (0x1<<AIF2_ADCL_MXR_SRC);
		snd_soc_write(codec, AIF2_MXR_SRC, reg_val);
	} else {
		/*disable mic1 routeway*/
		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val &= ~(0x1<<MIC1AMPEN);
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

		/*disable Master microphone BIAS*/
		reg_val = snd_soc_read(codec, ADC_APC_CTRL);
		reg_val &= ~(0x1<<MBIASEN);
		snd_soc_write(codec, ADC_APC_CTRL, reg_val);

		/*left/Right ADC Mixer Mute Control*/
		reg_val = snd_soc_read(codec, ADC_SRC);
		reg_val &= ~(0x1<<LADCMIXMUTEMIC1BOOST);
		snd_soc_write(codec, ADC_SRC, reg_val);

		/*disable  analog  adc*/
		reg_val = snd_soc_read(codec, ADC_APC_CTRL);
		reg_val &= ~(0x1<<ADCLEN);
		snd_soc_write(codec, ADC_APC_CTRL, reg_val);

		 /*disable  digital adc*/
		reg_val = snd_soc_read(codec, ADC_DIG_CTRL);
		reg_val &= ~((0x1<<ENAD)|(0x1<<ENDM));
		snd_soc_write(codec, ADC_DIG_CTRL, reg_val);

		 /*AIF2_ADCL/R_MXR_SRC*/
		reg_val = snd_soc_read(codec, AIF2_MXR_SRC);
		reg_val &= ~(0x1<<AIF2_ADCL_MXR_SRC);
		snd_soc_write(codec, AIF2_MXR_SRC, reg_val);
	}

	return 0;
}

static int codec_digital_get_mainmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_digital_mainmic_en;
	return 0;
}

/*
*	codec_digital_headsetmic_en == 1, open mic2 for digital bb.
*	codec_digital_headsetmic_en == 0, close mic2 for digital bb.
*/
static int codec_digital_set_headsetmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{

	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg_val;
	codec_digital_headsetmic_en = ucontrol->value.integer.value[0];

	if (codec_digital_headsetmic_en) {
		/*open mic2 routeway*/
		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val |= 0x1<<MIC2AMPEN;
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

		/*MIC2 Source select*/
		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val &= ~( 0x1<<MIC2SLT);
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

		/*MIC2boost amplifier Gain control*/
		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val &= ~(0x7<<ADC_MIC2G);
		reg_val |= 0x4<<ADC_MIC2G;
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

		/*Headset microphoneBIAS Enable*/
	 	reg_val = snd_soc_read(codec, ADC_APC_CTRL);
		reg_val |= (0x1<<HBIASEN);
		snd_soc_write(codec, ADC_APC_CTRL, reg_val);

		/*Headset microphoneBIAS Current sensor & ADC Enable*/
	 	reg_val = snd_soc_read(codec, ADC_APC_CTRL);
		reg_val |= (0x1<<HBIASADCEN);
		snd_soc_write(codec, ADC_APC_CTRL, reg_val);

		/*left/Right ADC Mixer unMute Control*/
		reg_val = snd_soc_read(codec, ADC_SRC);
		reg_val |= (0x1<<LADCMIXMUTEMIC2BOOST);
		snd_soc_write(codec, ADC_SRC, reg_val);

		/*enable  analog  adc*/
		reg_val = snd_soc_read(codec, ADC_APC_CTRL);
		reg_val |= (0x1<<ADCLEN);
		snd_soc_write(codec, ADC_APC_CTRL, reg_val);

		/*enable  digital adc*/
		reg_val = snd_soc_read(codec, ADC_DIG_CTRL);
		reg_val &= ~((0x1<<ENAD)|(0x1<<ENDM));
		reg_val |= (0x1<<ENAD)|(0x0<<ENDM);
		snd_soc_write(codec, ADC_DIG_CTRL, reg_val);

		 /*AIF2_ADCL/R_MXR_SRC*/
		reg_val = snd_soc_read(codec, AIF2_MXR_SRC);
		reg_val |= (0x1<<AIF2_ADCL_MXR_SRC);
		snd_soc_write(codec, AIF2_MXR_SRC, reg_val);
	} else {
		/*close mic2 routeway*/
		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val &= ~(0x1<<MIC2AMPEN);
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

		/*left ADC Mixer Mute Control*/
		reg_val = snd_soc_read(codec, ADC_SRC);
		reg_val &= ~(0x1<<LADCMIXMUTEMIC2BOOST);
		snd_soc_write(codec, ADC_SRC, reg_val);

		/*disable  analog  adc*/
		reg_val = snd_soc_read(codec, ADC_APC_CTRL);
		reg_val &= ~(0x1<<ADCLEN);
		snd_soc_write(codec, ADC_APC_CTRL, reg_val);

		 /*disable  digital adc*/
		reg_val = snd_soc_read(codec, ADC_DIG_CTRL);
		reg_val &= ~((0x1<<ENAD)|(0x1<<ENDM));
		snd_soc_write(codec, ADC_DIG_CTRL, reg_val);

		 /*AIF2_ADCL_MXR_SRC*/
		reg_val = snd_soc_read(codec, AIF2_MXR_SRC);
		reg_val &= ~(0x1<<AIF2_ADCL_MXR_SRC);
		snd_soc_write(codec, AIF2_MXR_SRC, reg_val);
	}
	return 0;
}

static int codec_digital_get_headsetmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_digital_headsetmic_en;
	return 0;
}

/*
*	codec_digital_phoneout_en == 1, enable phoneout for digital bb.
*	codec_digital_phoneout_en == 0, disable phoneout for digital bb.
*/
static int codec_digital_set_phoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg_val;
	codec_digital_phoneout_en = ucontrol->value.integer.value[0];

	if (codec_digital_phoneout_en) {
		/*AIF2 ADC left/right channel data source select*/
		reg_val = snd_soc_read(codec, AIF2_ADCDAT_CTRL);
		reg_val &= ~(0x3<<AIF2_ADCL_SRC);
		snd_soc_write(codec, AIF2_ADCDAT_CTRL, reg_val);
		/*hardware add reason*/
		snd_soc_write(codec, AIF2_VOL_CTRL1, 0x9f9f);

		/*AIF2 ADC left/right channel enable*/
		reg_val = snd_soc_read(codec, AIF2_ADCDAT_CTRL);
		reg_val |= (0x1<<AIF2_ADCL_EN);
		snd_soc_write(codec, AIF2_ADCDAT_CTRL, reg_val);
	} else {
		/*AIF2 ADC left/right channel data source select(default value)*/
		reg_val = snd_soc_read(codec, AIF2_ADCDAT_CTRL);
		reg_val &= ~(0x3<<AIF2_ADCL_SRC);
		snd_soc_write(codec, AIF2_ADCDAT_CTRL, reg_val);

		/*disable AIF2 ADC left/right channel */
		reg_val = snd_soc_read(codec, AIF2_ADCDAT_CTRL);
		reg_val &= ~(0x1<<AIF2_ADCL_EN);
		snd_soc_write(codec, AIF2_ADCDAT_CTRL, reg_val);
	}
	return 0;
}

static int codec_digital_get_phoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_digital_phoneout_en;
	return 0;
}

/*
*	codec_digital_output_en == 1,
*	codec_digital_output_en == 0.
*/
static int codec_digital_set_output(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg_val;
	codec_digital_output_en = ucontrol->value.integer.value[0];

	if (codec_digital_output_en) {
		/*open aif1 DAC channel slot0 switch*/
		reg_val = snd_soc_read(codec, AIF1_DACDAT_CTRL);
		reg_val |= (0x1<<AIF1_DA0L_ENA);
		snd_soc_write(codec, AIF1_DACDAT_CTRL,reg_val);

		/*AIF1 DAC Timeslot0 left channel data source select*/
		reg_val = snd_soc_read(codec, AIF1_DACDAT_CTRL);
		reg_val &= ~(0x3<<AIF1_DA0L_SRC);
		snd_soc_write(codec, AIF1_DACDAT_CTRL, reg_val);

		 /*AIF2_ADCL_MXR_SRC*/
		reg_val = snd_soc_read(codec, AIF2_MXR_SRC);
		reg_val &= ~(0xf<<AIF2_ADCL_MXR_SRC);
		reg_val |= (0x8<<AIF2_ADCL_MXR_SRC);
		snd_soc_write(codec, AIF2_MXR_SRC, reg_val);
	} else {
		/*close aif1 DAC channel slot0 switch*/
		reg_val = snd_soc_read(codec, AIF1_DACDAT_CTRL);
		reg_val &= ~(0x1<<AIF1_DA0L_ENA);
		snd_soc_write(codec, AIF1_DACDAT_CTRL,reg_val);

		/*AIF1 DAC Timeslot0 left channel data source select(default value)*/
		reg_val = snd_soc_read(codec, AIF1_DACDAT_CTRL);
		reg_val &= ~(0x3<<AIF1_DA0L_SRC);
		snd_soc_write(codec, AIF1_DACDAT_CTRL, reg_val);

		 /*AIF2_ADCL_MXR_SRC(default value)*/
		reg_val = snd_soc_read(codec, AIF2_MXR_SRC);
		reg_val &= ~(0xf<<AIF2_ADCL_MXR_SRC);
		snd_soc_write(codec, AIF2_MXR_SRC, reg_val);
	}
	return 0;
}

static int codec_digital_get_output(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_digital_output_en;
	return 0;
}

/*
*	codec_digital_phonein_en == 1, enble phonein for digital bb .
*	codec_digital_phonein_en == 0. disable phonein for digital bb.
*/
static int codec_set_digital_phonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg_val;
	codec_digital_phonein_en = ucontrol->value.integer.value[0];

	if (codec_digital_phonein_en) {
		/*AIF2 DAC left channel enble*/
		reg_val =  snd_soc_read(codec, AIF2_DACDAT_CTRL);
		reg_val |= (0x1<<AIF2_DACL_ENA);
		snd_soc_write(codec, AIF2_DACDAT_CTRL, reg_val);

		/*select source */
		reg_val =  snd_soc_read(codec, AIF2_DACDAT_CTRL);
		reg_val &= ~(0x3<<AIF2_DACL_SRC);
		snd_soc_write(codec, AIF2_DACDAT_CTRL, reg_val);

		/*AIF2 DAC input source select*/
		reg_val = snd_soc_read(codec, AIF3_SGP_CTRL);
		reg_val &= ~(0x3<<AIF2_DAC_SRC);
		snd_soc_write(codec, AIF3_SGP_CTRL, reg_val);

		/*select  DAC mixer source*/
		reg_val = snd_soc_read(codec, DAC_MXR_SRC);
		//reg_val &= ~(0xf<<DACL_MXR_SRC);
		reg_val |= (0x1<<DACL_MXR_AIF2_DACL);
		snd_soc_write(codec, DAC_MXR_SRC, reg_val);

		/*enable digital DAC*/
		reg_val = snd_soc_read(codec, DAC_DIG_CTRL);
		reg_val |= (0x1<<ENDA);
		snd_soc_write(codec, DAC_DIG_CTRL, reg_val);

		/*enable analog DAC*/
		reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
		reg_val |= (0x1<<DACALEN);
		snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

		/*select source */
		reg_val =  snd_soc_read(codec, OMIXER_SR);
		reg_val |= (0x1<<LMIXMUTEDACL)|(0x1<<RMIXMUTEDACL);
		snd_soc_write(codec, OMIXER_SR, reg_val);

		/*enble l/r Analog Output Mixer */
		reg_val =  snd_soc_read(codec, OMIXER_DACA_CTRL);
		reg_val |= (0x1<<LMIXEN)|(0x1<<RMIXEN);
		snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);
	} else {
		/*disable AIF2 DAC left channel */
		reg_val =  snd_soc_read(codec, AIF2_DACDAT_CTRL);
		reg_val &= ~(0x1<<AIF2_DACL_ENA);
		snd_soc_write(codec, AIF2_DACDAT_CTRL, reg_val);

		/*select source(default value) */
		reg_val =  snd_soc_read(codec, AIF2_DACDAT_CTRL);
		reg_val &= ~(0x3<<AIF2_DACL_SRC);
		snd_soc_write(codec, AIF2_DACDAT_CTRL, reg_val);

		/*AIF2_ADCL/R_MXR_SRC(for blue tooth)*/
		reg_val = snd_soc_read(codec, AIF2_MXR_SRC);
		reg_val &= ~(0xf<<AIF2_ADCR_MXR_SRC);
		snd_soc_write(codec, AIF2_MXR_SRC, reg_val);

		/*select  DAC mixer source(default value)*/
		reg_val = snd_soc_read(codec, DAC_MXR_SRC);
		//reg_val &= ~(0xf<<DACL_MXR_SRC);
		reg_val &= ~(0x1<<DACL_MXR_AIF2_DACL);
		snd_soc_write(codec, DAC_MXR_SRC, reg_val);

		/*disable digital DAC*/
		reg_val = snd_soc_read(codec, DAC_DIG_CTRL);
		reg_val &= ~(0x1<<ENDA);
		snd_soc_write(codec, DAC_DIG_CTRL, reg_val);

		/*disable analog DAC*/
		reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
		reg_val &= ~(0x1<<DACALEN);
		snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

		/*select source(default value) */
		reg_val =  snd_soc_read(codec, OMIXER_SR);
		reg_val &= ~((0x1<<LMIXMUTEDACL)|(0x1<<RMIXMUTEDACL));
		snd_soc_write(codec, OMIXER_SR, reg_val);

		/*disble l/r Analog Output Mixer */
		reg_val =  snd_soc_read(codec, OMIXER_DACA_CTRL);
		reg_val &= ~((0x1<<LMIXEN)|(0x1<<RMIXEN));
		snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

	}
	return 0;
}

static int codec_get_digital_phonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_digital_phonein_en;
	return 0;
}

static int codec_set_digital_bb_clk_format(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int reg_val;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	codec_digital_bb_clk_format_init = ucontrol->value.integer.value[0];
	if (codec_digital_bb_clk_format_init) {
		printk("%s,line:%d\n",__func__,__LINE__);
		/*enble pll2*/
		reg_val = 0x0;
		reg_val |= (0x1<<PLL_EN);
		reg_val |= (0x24<<PLL_PREDIV_NI);
		snd_soc_write(codec, PLL_CTRL2,reg_val);

		reg_val = 0x0;
		reg_val |= (0x1<<PLLCLK_ENA) ;/*enble pllclk*/
		reg_val |= (0x3<<PLLCLK_SRC) ;/*select pll source from bclk2*/
		reg_val |= (0x1<<AIF2CLK_ENA) ;/*enble aif2clk*/
		reg_val |= (0x3<<AIF2CLK_SRC) ;/*select pllclk from pll*/
		reg_val |= (0x1<<SYSCLK_ENA) ;/*enble sys clk*/
		reg_val |= (0x1<<SYSCLK_SRC) ;/*select sys clk from aif2*/
		snd_soc_write(codec, SYSCLK_CTRL,reg_val);

		/* enable  aif2 module clk*/
		reg_val = 0x0;
		reg_val |= (0x3<<MOD_CLK_AIF2)|(0x1<<MOD_CLK_ADC_DIG)|(0x1<<MOD_CLK_DAC_DIG);
		snd_soc_write(codec, MOD_CLK_ENA,reg_val);

		/*reset aif1 aif2 module*/
		reg_val = 0x0;
		reg_val |= (0x3<<MOD_RESET_AIF2)|(0x1<<MOD_RESET_ADC_DIG)|(0x1<<MOD_RESET_DAC_DIG);
		snd_soc_write(codec, MOD_RST_CTRL,reg_val);

		reg_val = 0x0;
		reg_val |= (0x0<<AIF1_FS)|(0x0<<AIF2_FS);
		snd_soc_write(codec, AIF_SR_CTRL,reg_val);

		reg_val = 0x0;
		reg_val |= (0x1<<AIF2_MSTR_MOD);/*slave mode*/
		reg_val |= (0x4<<AIF2_LRCK_DIV);/*bclk/lrck = 256*/
		reg_val |= (0x1<<AIF2_WORD_SIZ);/*ws = 16*/
		reg_val |= (0x3<<AIF2_DATA_FMT);/*slave mode*/
		reg_val |= (0x1<<AIF2_MONO_PCM);/*mono mode*/
		snd_soc_write(codec, AIF2_CLK_CTRL, reg_val);

		reg_val = snd_soc_read(codec, AIF2_CLK_CTRL);
		reg_val &= ~(0x1<<AIF2_LRCK_INV);
		reg_val |= (0x1<<AIF2_BCLK_INV);
		snd_soc_write(codec, AIF2_CLK_CTRL, reg_val);

		if (digital_bb_cap_keytone_used == 1) {
			/*
			*In the digital bb phone scenario, if need to config
			*the key tone and recording function, you should config aif1 clock first, and open
			*the SRC function.
			*/
			/***************************example*************************************************/
		#if 0
			/*config sysclk 22.5792M*/
			sunxi_daudio_set_rate(22579200);

			/*SUNXI_DAUDIOCTL*/
			reg_val = readl((void __iomem *)0xf8006000);
			/*Global disable Digital Audio Interface*/
			reg_val |= (1<<0);
			/*enable lrck bclk output*/
			reg_val |= (3<<17);
			/*config i2s format*/
			reg_val &= ~(3<<4);
			reg_val |= (1<<4);
			writel(reg_val,(void __iomem *)0xf8006000);

			/*SUNXI_DAUDIOTX0CHSEL*/
			reg_val = readl((void __iomem *)0xf8006034);
			/*config i2s fmt*/
			reg_val |= (1<<12);
			writel(reg_val, (void __iomem *)0xf8006034);

			/*SUNXI_DAUDIOFAT0*/
			reg_val = readl((void __iomem *)0xf8006004);
			reg_val &= ~(1<<7);
			reg_val &= ~(1<<19);
			writel(reg_val, (void __iomem *)0xf8006004);

			/*SUNXI_DAUDIOCLKD*/
			reg_val = readl((void __iomem *)0xf8006024);
			/*config mclk = 22.5792M and enble mclk*/
			//reg_val |= (0x1<<8)|(0x1<<0);
			/*lrck:44.1,bclk=44.1*64*/
			reg_val |= (5<<4);
			writel(reg_val, (void __iomem *)0xf8006024);

			/*SUNXI_DAUDIOFAT0*/
			reg_val = readl((void __iomem *)0xf8006004);
			reg_val &= ~(0x3ff<<20);
			reg_val &= ~(0x3ff<<8);
			reg_val |= (31)<<8;
			/*slot_width:16*/
			reg_val |= (3<<0);
			/*sr =6*/
			reg_val |= (3<<4);
			writel(reg_val, (void __iomem *)0xf8006004);
		#endif
		/********************example**************************************************/
			reg_val = snd_soc_read(codec, AIF1_CLK_CTRL);
			reg_val &= ~(0xf<<AIF1_BCLK_DIV);
			reg_val &= ~(0x7<<AIF1_LRCK_DIV);
			reg_val &= ~(0x3<<AIF1_WORK_SIZ);
			reg_val |= ((0x4<<AIF1_BCLK_DIV)|(0x2<<AIF1_LRCK_DIV)|(0x1<<AIF1_WORK_SIZ));
			snd_soc_write(codec, AIF1_CLK_CTRL, reg_val);

			reg_val = snd_soc_read(codec, AIF1_CLK_CTRL);
			reg_val &= ~(0x1<<AIF1_MSTR_MOD);
			reg_val |= (0x1<<AIF1_MSTR_MOD);
			reg_val |= (0x0<<AIF1_DATA_FMT);
			reg_val &= ~(0x1<<AIF1_LRCK_INV);
			reg_val &= ~(0x1<<AIF1_BCLK_INV);
			snd_soc_write(codec, AIF1_CLK_CTRL, reg_val);
			/*
			*In the phone scenario with digital bb, the src1,src2 function
			*should be opened in record and key tone.
			*/
			/*enble aif1,aif1 src from pll*/
			reg_val = snd_soc_read(codec, SYSCLK_CTRL);
			reg_val |= (0x3<<AIF1CLK_SRC);
			reg_val |= (0x1<<AIF1CLK_ENA);
			snd_soc_write(codec, SYSCLK_CTRL, reg_val);

			reg_val = snd_soc_read(codec, AIF1_ADCDAT_CTRL);
			reg_val |= (0x1<<AIF1_AD0L_ENA);
			snd_soc_write(codec, AIF1_ADCDAT_CTRL, reg_val);

			reg_val = snd_soc_read(codec, AIF1_DACDAT_CTRL);
			reg_val |= (0x1<<AIF1_DA0L_ENA);
			snd_soc_write(codec, AIF1_DACDAT_CTRL, reg_val);

			/*enble module src1,src2*/
			reg_val = snd_soc_read(codec, MOD_CLK_ENA);
			reg_val |= (0x1<<MOD_CLK_SRC1)|(0x1<<MOD_CLK_SRC2);
			snd_soc_write(codec, MOD_CLK_ENA, reg_val);
			/*reset src1,src2*/
			reg_val = snd_soc_read(codec, MOD_RST_CTRL);
			reg_val |= (0x1<<MOD_RESET_SRC2)|(0x1<<MOD_RESET_SRC1);
			snd_soc_write(codec, MOD_RST_CTRL, reg_val);

			reg_val = snd_soc_read(codec, AIF_SR_CTRL);
			/*select src2 source from aif1*/
			reg_val &= ~(0x1<<SRC2_SRC);
			/*enable src2*/
			reg_val |= (0x1<<SRC2_ENA);
			/*select src1 source from ap*/
			reg_val &= ~(0x1<<SRC1_SRC);
			/*enable src1*/
			reg_val |= (0x1<<SRC1_ENA);
			snd_soc_write(codec, AIF_SR_CTRL, reg_val);
		}

	} else {
		reg_val = 0x0141;
		snd_soc_write(codec, PLL_CTRL1,reg_val);

		reg_val = 0x0;
		snd_soc_write(codec, PLL_CTRL2,reg_val);

		reg_val = 0x0;
		snd_soc_write(codec, SYSCLK_CTRL,reg_val);

		/* enable  aif2 module clk*/
		reg_val = 0x0;
		snd_soc_write(codec, MOD_CLK_ENA,reg_val);

		/*reset aif1 aif2 module*/
		reg_val = 0x0;
		snd_soc_write(codec, MOD_RST_CTRL,reg_val);

		reg_val = 0x0;
		snd_soc_write(codec, AIF2_CLK_CTRL, reg_val);

		reg_val = 0x0;
		snd_soc_write(codec, AIF_SR_CTRL, reg_val);
	}

	return 0;
}

static int codec_get_digital_bb_clk_format(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_digital_bb_clk_format_init;
	return 0;
}

static int codec_set_bt_clk_format(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg_val;
	codec_bt_clk_format = ucontrol->value.integer.value[0];

	if (codec_bt_clk_format) {
		#if 0
		/*cldo3:config voltage for aif3*/
		aif3_cldo3 = regulator_get(NULL, aif3_voltage.str);
		if (!aif3_cldo3) {
			printk("get audio aif3_cldo3 failed\n");
			return -EFAULT;
		}
		regulator_set_voltage(aif3_cldo3, 3000000, 3000000);
		regulator_enable(aif3_cldo3);
		#endif
		/*
		*Need to open the external clock, provide the codec clock source for analog
		*bb bluetooth phone scenario.
		*/
		/***********************example******************************************/
#if 0
		/*config sysclk 24.576M*/
		sunxi_daudio_set_rate(24576000);

		/*config mclk = 24.576M and enble mclk*/
		reg_val = 0;
		reg_val |= (0x1<<8)|(0x1<<0);
		writel(reg_val, (void __iomem *)0xf8006024);
#endif
		/***********************example******************************************/

		/*enable AIF2CLK,systemclk*/
		reg_val = snd_soc_read(codec, SYSCLK_CTRL);
		/*enble sysclk,aif2clk*/
		reg_val |= (0x1<<SYSCLK_ENA)|(0x1<<AIF2CLK_ENA);
		/*select aif2clk from mclk1*/
		reg_val &= ~(0x3<<AIF2CLK_SRC);
		/*select sysclk from aif2*/
		reg_val |= (0x1<<SYSCLK_SRC);
		snd_soc_write(codec, SYSCLK_CTRL, reg_val);

		/*enable module aif2,aif3,DAC,ADC*/
		reg_val = snd_soc_read(codec, MOD_CLK_ENA);
		reg_val |= (0x1<<MOD_CLK_DAC_DIG)|(0x1<<MOD_CLK_ADC_DIG)|(0x1<<MOD_CLK_AIF2)|(0x1<<MOD_CLK_AIF3);
		snd_soc_write(codec, MOD_CLK_ENA, reg_val);

		/*reset module aif2,aif3, DAC,ADC*/
		reg_val = snd_soc_read(codec, MOD_RST_CTRL);
		reg_val |= (0x1<<MOD_RESET_DAC_DIG)|(0x1<<MOD_RESET_ADC_DIG)|(0x1<<MOD_RESET_AIF2)|(0x1<<MOD_CLK_AIF3);
		snd_soc_write(codec, MOD_RST_CTRL, reg_val);

		/*aif2:sr=8k*/
		reg_val = snd_soc_read(codec, AIF_SR_CTRL);
		reg_val &=~(0xf<<AIF2_FS);
		reg_val &=~(0xf<<AIF1_FS);
		snd_soc_write(codec, AIF_SR_CTRL, reg_val);

		/*aif2:master*/
		reg_val = snd_soc_read(codec, AIF2_CLK_CTRL);
		reg_val &= ~(0x1<<AIF2_MSTR_MOD);
		snd_soc_write(codec, AIF2_CLK_CTRL, reg_val);

		/*aif2clk/bclk = 48*/
		reg_val = snd_soc_read(codec, AIF2_CLK_CTRL);
		reg_val &= ~(0xf<<AIF2_BCLK_DIV);
		reg_val |= (0x9<<AIF2_BCLK_DIV);
		snd_soc_write(codec, AIF2_CLK_CTRL, reg_val);

		/*bclk/lrck=64*/
		reg_val = snd_soc_read(codec, AIF2_CLK_CTRL);
		reg_val &= ~(0x7<<AIF2_LRCK_DIV);
		reg_val |= (0x2<<AIF2_LRCK_DIV);
		snd_soc_write(codec, AIF2_CLK_CTRL, reg_val);

		/*SR=16 */
		reg_val = snd_soc_read(codec, AIF2_CLK_CTRL);
		reg_val &= ~(0x3<<AIF2_WORD_SIZ);
		reg_val |= (0x1<<AIF2_WORD_SIZ);
		snd_soc_write(codec, AIF2_CLK_CTRL, reg_val);

		/*DSP mode*/
		reg_val = snd_soc_read(codec, AIF2_CLK_CTRL);
		reg_val &= ~(0x3<<AIF2_DATA_FMT);
		reg_val |= (0x3<<AIF2_DATA_FMT);
		snd_soc_write(codec, AIF2_CLK_CTRL, reg_val);

		/*Mono mode select*/
		reg_val = snd_soc_read(codec, AIF2_CLK_CTRL);
		reg_val |= (0x1<<AIF2_MONO_PCM);
		snd_soc_write(codec, AIF2_CLK_CTRL, reg_val);

		/*config aif3(clk source come from aif2)*/
		/* invert bclk + nor frm */
		reg_val = snd_soc_read(codec, AIF3_CLK_CTRL);
		reg_val &= ~(0x1<<AIF3_LRCK_INV);
		reg_val |= 0x1<<AIF3_BCLK_INV;
		snd_soc_write(codec, AIF3_CLK_CTRL, reg_val);

		/*Mono mode select*/
		reg_val = snd_soc_read(codec, AIF3_CLK_CTRL);
		reg_val &= ~(0x3<<AIF3_WORD_SIZ);
		reg_val |= (0x1<<AIF3_WORD_SIZ);
		snd_soc_write(codec, AIF3_CLK_CTRL, reg_val);

		/*BCLK/LRCK Come from AIF2*/
		reg_val = snd_soc_read(codec, AIF3_CLK_CTRL);
		reg_val &= ~(0x3<<AIF3_CLOC_SRC);
		reg_val |= (0x1<<AIF3_CLOC_SRC);
		snd_soc_write(codec, AIF3_CLK_CTRL, reg_val);
	} else {
		/*disable AIF2CLK,sysclk*/
		reg_val = 0;
		snd_soc_write(codec, SYSCLK_CTRL, reg_val);

		/*disable module AIF1,aif2,aif3,DAC,ADC*/
		reg_val = snd_soc_read(codec, MOD_CLK_ENA);
		reg_val &= ~((0x1<<MOD_CLK_DAC_DIG)|(0x1<<MOD_CLK_ADC_DIG)|(0x1<<MOD_CLK_AIF2)|(0x1<<MOD_CLK_AIF3));
		snd_soc_write(codec, MOD_CLK_ENA, reg_val);

		/*reset module AIF1, DAC,ADC*/
		reg_val = snd_soc_read(codec, MOD_RST_CTRL);
		reg_val &= ~((0x1<<MOD_RESET_DAC_DIG)|(0x1<<MOD_RESET_ADC_DIG)|(0x1<<MOD_RESET_AIF2)|(0x1<<MOD_CLK_AIF3));
		snd_soc_write(codec, MOD_RST_CTRL, reg_val);

		/*AIF2_CLK_CTRL:default value*/
		reg_val = 0;
		snd_soc_write(codec, AIF2_CLK_CTRL, reg_val);

		/*AIF3_CLK_CTRL:default value*/
		reg_val = 0;
		snd_soc_write(codec, AIF3_CLK_CTRL, reg_val);
		#if 0
		/*cldo3:config voltage for aif3*/
		if (aif3_cldo3) {
			regulator_disable(aif3_cldo3);
			regulator_put(aif3_cldo3);
			aif3_cldo3 = NULL;
		}
		#endif
	}

	return 0;
}

static int codec_get_bt_clk_format(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_bt_clk_format;
	return 0;
}

static int codec_set_bt_out(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg_val;
	codec_bt_out_en = ucontrol->value.integer.value[0];
	if (codec_bt_out_en) {
		/*AIF2 ADC right channel enable*/
		reg_val = snd_soc_read(codec, AIF2_ADCDAT_CTRL);
		reg_val |= 0x1<<AIF2_ADCR_EN;
		snd_soc_write(codec, AIF2_ADCDAT_CTRL, reg_val);

		/*AIF3 PCM output source select*/
		reg_val = snd_soc_read(codec, AIF3_SGP_CTRL);
		reg_val &= ~(0x3<<AIF3_ADC_SRC);
		reg_val |= (0x2<<AIF3_ADC_SRC);
		snd_soc_write(codec, AIF3_SGP_CTRL, reg_val);
	} else {
		reg_val = snd_soc_read(codec, AIF3_SGP_CTRL);
		reg_val &= ~(0x3<<AIF3_ADC_SRC);
		snd_soc_write(codec, AIF3_SGP_CTRL, reg_val);

		reg_val = snd_soc_read(codec, AIF2_ADCDAT_CTRL);
		reg_val &= ~(0x1<<AIF2_ADCR_EN);
		snd_soc_write(codec, AIF2_ADCDAT_CTRL, reg_val);
	}

	return 0;
}

static int codec_get_bt_out(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_bt_out_en;
	return 0;
}

static int codec_set_digital_btmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg_val;
	codec_digital_btmic_en = ucontrol->value.integer.value[0];

	if (codec_digital_btmic_en) {
		/*AIF2 DAC input source select*/
		reg_val = snd_soc_read(codec, AIF3_SGP_CTRL);
		reg_val &= ~(0x3<<AIF2_DAC_SRC);
		reg_val |= 0x2<<AIF2_DAC_SRC;
		snd_soc_write(codec, AIF3_SGP_CTRL, reg_val);

		/*AIF2 DAC left channel data source select*/
		reg_val = snd_soc_read(codec, AIF2_DACDAT_CTRL);
		reg_val |= (0x1<<AIF2_DACR_ENA);
		snd_soc_write(codec, AIF2_DACDAT_CTRL, reg_val);

		/*AIF2 ADC left channel mixer source select*/
		reg_val = snd_soc_read(codec, AIF2_MXR_SRC);
		reg_val &= ~(0xf<<AIF2_ADCL_MXR_SRC);
		reg_val |= 0x2<<AIF2_ADCL_MXR_SRC;
		snd_soc_write(codec, AIF2_MXR_SRC, reg_val);
	} else {
		/*AIF2 DAC input source select*/
		reg_val = snd_soc_read(codec, AIF3_SGP_CTRL);
		reg_val &= ~(0x3<<AIF2_DAC_SRC);
		snd_soc_write(codec, AIF3_SGP_CTRL, reg_val);

		/*AIF2 DAC left channel data source select*/
		reg_val = snd_soc_read(codec, AIF2_DACDAT_CTRL);
		reg_val &= ~(0x1<<AIF2_DACR_ENA);
		snd_soc_write(codec, AIF2_DACDAT_CTRL, reg_val);

		/*AIF2 ADC left channel mixer source select*/
		reg_val = snd_soc_read(codec, AIF2_MXR_SRC);
		reg_val &= ~(0xf<<AIF2_ADCL_MXR_SRC);
		snd_soc_write(codec, AIF2_MXR_SRC, reg_val);
	}
	return 0;
}

static int codec_get_digital_btmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_digital_btmic_en;
	return 0;
}

static int codec_set_digital_btphonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg_val;
	codec_digital_btphonein_en = ucontrol->value.integer.value[0];

	if (codec_digital_btphonein_en) {
		/*AIF2 DAC left channel enable*/
		reg_val = snd_soc_read(codec, AIF2_DACDAT_CTRL);
		reg_val |= 0x1<<AIF2_DACL_ENA;
		snd_soc_write(codec, AIF2_DACDAT_CTRL, reg_val);

		/*AIF2 DAC left channel data source select*/
		reg_val = snd_soc_read(codec, AIF2_DACDAT_CTRL);
		reg_val &= ~(0x3<<AIF2_DACL_SRC);
		snd_soc_write(codec, AIF2_DACDAT_CTRL, reg_val);

		/*AIF2 DAC left channel data source select*/
		reg_val = snd_soc_read(codec, AIF2_MXR_SRC);
		reg_val &= ~(0xf<<AIF2_ADCR_MXR_SRC);
		reg_val |= 0x2<<AIF2_ADCR_MXR_SRC;
		snd_soc_write(codec, AIF2_MXR_SRC, reg_val);
	} else {
		/*AIF2 DAC left channel enable*/
		reg_val = snd_soc_read(codec, AIF2_DACDAT_CTRL);
		reg_val &= ~(0x1<<AIF2_DACL_ENA);
		snd_soc_write(codec, AIF2_DACDAT_CTRL, reg_val);

		/*AIF2 DAC left channel data source select(default value)*/
		reg_val = snd_soc_read(codec, AIF2_DACDAT_CTRL);
		reg_val &= ~(0x3<<AIF2_DACL_SRC);
		snd_soc_write(codec, AIF2_DACDAT_CTRL, reg_val);

		/*AIF2 DAC left channel data source select*/
		reg_val = snd_soc_read(codec, AIF2_MXR_SRC);
		reg_val &= ~(0xf<<AIF2_ADCR_MXR_SRC);
		snd_soc_write(codec, AIF2_MXR_SRC, reg_val);
	}
	return 0;
}

static int codec_get_digital_btphonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_digital_btphonein_en;
	return 0;
}

static int codec_set_digital_bb_bt_clk_format(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg_val;
	codec_digital_bb_bt_clk_format  = ucontrol->value.integer.value[0];

	if (codec_digital_bb_bt_clk_format) {
#if 0
		/*cldo3:config voltage for aif3*/
		aif3_cldo3 = regulator_get(NULL,aif3_voltage.str);
		if (!aif3_cldo3) {
			printk("get audio aif3_cldo3 failed\n");
			return -EFAULT;
		}
		regulator_set_voltage(aif3_cldo3, 3000000, 3000000);
		regulator_enable(aif3_cldo3);
#endif
		/* enable  aif3 module clk*/
		reg_val = snd_soc_read(codec, MOD_CLK_ENA);
		reg_val |= (0x1<<MOD_CLK_AIF3);
		snd_soc_write(codec, MOD_CLK_ENA,reg_val);

		/*reset aif3 module*/
		reg_val = snd_soc_read(codec, MOD_RST_CTRL);
		reg_val |= (0x1<<MOD_RESET_AIF3);
		snd_soc_write(codec, MOD_RST_CTRL,reg_val);

		/*
		* config aif3(clk source from aif2)
		*/
		reg_val = snd_soc_read(codec, AIF3_CLK_CTRL);
		reg_val &= ~(0x1<<AIF3_LRCK_INV);
		reg_val |= 0x1<<AIF3_BCLK_INV;
		snd_soc_write(codec, AIF3_CLK_CTRL, reg_val);

		/*Mono mode select*/
		reg_val = snd_soc_read(codec, AIF3_CLK_CTRL);
		reg_val &= ~(0x3<<AIF3_WORD_SIZ);
		reg_val |= (0x1<<AIF3_WORD_SIZ);
		snd_soc_write(codec, AIF3_CLK_CTRL, reg_val);

		/*BCLK/LRCK Come from AIF2*/
		reg_val = snd_soc_read(codec, AIF3_CLK_CTRL);
		reg_val &= ~(0x3<<AIF3_CLOC_SRC);
		reg_val |= (0x1<<AIF3_CLOC_SRC);
		snd_soc_write(codec, AIF3_CLK_CTRL, reg_val);
	} else {
		reg_val = 0;
		snd_soc_write(codec, AIF3_CLK_CTRL, reg_val);
#if 0
		/*cldo3:config voltage for aif3*/
		if (aif3_cldo3) {
			regulator_disable(aif3_cldo3);
			regulator_put(aif3_cldo3);
			aif3_cldo3 = NULL;
		}
#endif
	}

	return 0;
}

static int codec_get_digital_bb_bt_clk_format(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_digital_bb_bt_clk_format;
	return 0;
}

static int codec_set_analog_btmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg_val;
	codec_analog_btmic_en = ucontrol->value.integer.value[0];

	if (codec_analog_btmic_en) {
		/*AIF2 DAC input source select*/
		reg_val = snd_soc_read(codec, AIF3_SGP_CTRL);
		reg_val &= ~(0x3<<AIF2_DAC_SRC);
		reg_val |= 0x1<<AIF2_DAC_SRC;
		snd_soc_write(codec, AIF3_SGP_CTRL, reg_val);

		/*select  DAC mixer source*/
		reg_val = snd_soc_read(codec, DAC_MXR_SRC);
		reg_val |= (0x1<<DACL_MXR_AIF2_DACL);
		snd_soc_write(codec, DAC_MXR_SRC, reg_val);

		/*enable digital DAC*/
		reg_val = snd_soc_read(codec, DAC_DIG_CTRL);
		reg_val |= (0x1<<ENDA);
		snd_soc_write(codec, DAC_DIG_CTRL, reg_val);

		/*enable L internal analog channel*/
		reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
		reg_val |= (0x1<<DACALEN);
		snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

		/*Left Output Mixer Mute Control*/
		reg_val = snd_soc_read(codec, OMIXER_SR);
		reg_val = 0x0;
		reg_val |= (0x1<<LMIXMUTEDACL);
		snd_soc_write(codec, OMIXER_SR, reg_val);

		/*Left Analog Output Mixer Enable*/
		reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
		reg_val |= (0x1<<LMIXEN);
		snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

		/*select the phone out source*/
		reg_val = snd_soc_read(codec, LOUT_CTRL);
		reg_val &= ~(0xf<<LINEOUTS3);
		reg_val |= (0x1<<LINEOUTS3);
		snd_soc_write(codec, LOUT_CTRL, reg_val);
	} else {
		/*AIF2 DAC input source select(default value)*/
		reg_val = snd_soc_read(codec, AIF3_SGP_CTRL);
		reg_val &= ~(0x3<<AIF2_DAC_SRC);
		snd_soc_write(codec, AIF3_SGP_CTRL, reg_val);

		/*select  DAC mixer source*/
		reg_val = snd_soc_read(codec, DAC_MXR_SRC);
		reg_val &= ~(0x1<<DACL_MXR_AIF2_DACL);
		snd_soc_write(codec, DAC_MXR_SRC, reg_val);

		/*enable digital DAC*/
		reg_val = snd_soc_read(codec, DAC_DIG_CTRL);
		reg_val &= ~(0x1<<ENDA);
		snd_soc_write(codec, DAC_DIG_CTRL, reg_val);

		/*enable L internal analog channel*/
		reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
		reg_val &= ~(0x1<<DACALEN);
		snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

		/*Left Output Mixer Mute Control*/
		reg_val = snd_soc_read(codec, OMIXER_SR);
		reg_val &= ~(0x1<<LMIXMUTEDACL);
		snd_soc_write(codec, OMIXER_SR, reg_val);

		/*Left Analog Output Mixer Enable*/
		reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
		reg_val &= ~(0x1<<LMIXEN);
		snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

		/*select the phone out source*/
		reg_val = snd_soc_read(codec, LOUT_CTRL);
		reg_val &= ~(0xf<<LINEOUTS3);
		snd_soc_write(codec, LOUT_CTRL, reg_val);
	}

	return 0;
}

static int codec_get_analog_btmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_analog_btmic_en;
	return 0;
}

static int codec_set_analog_btphonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int reg_val;
	codec_analog_btphonein_en = ucontrol->value.integer.value[0];

	if (codec_analog_btphonein_en) {
		/*select the phonein source*/
		reg_val = snd_soc_read(codec, ADC_SRC);
		reg_val |= (0x1<<RADCMIXMUTELINEINLR);
		snd_soc_write(codec, ADC_SRC, reg_val);

		/*ADC R channel Enable*/
		reg_val = snd_soc_read(codec, ADC_APC_CTRL);
		reg_val &= ~(0x1<<ADCREN);
		reg_val |= (0x1<<ADCREN);
		snd_soc_write(codec, ADC_APC_CTRL, reg_val);

		/*enable ADC Digital part & ENBLE Analog ADC mode */
		reg_val = snd_soc_read(codec, ADC_DIG_CTRL);
		reg_val &= ~((0x1<<ENAD)|(0x1<<ENDM));
		reg_val |= (0x1<<ENAD)|(0x0<<ENDM);
		snd_soc_write(codec, ADC_DIG_CTRL, reg_val);

		/*AIF2 ADC right channel mixer source select */
		reg_val = snd_soc_read(codec, AIF2_MXR_SRC);
		reg_val &= ~(0x1<<AIF2_ADCR_MXR_SRC);
		reg_val |= (0x1<<AIF2_ADCR_MXR_SRC);
		snd_soc_write(codec, AIF2_MXR_SRC, reg_val);
	} else {
		/*select the phonein source*/
		reg_val = snd_soc_read(codec, ADC_SRC);
		reg_val &= ~(0x1<<RADCMIXMUTELINEINLR);
		snd_soc_write(codec, ADC_SRC, reg_val);

		/*ADC R channel Enable*/
		reg_val = snd_soc_read(codec, ADC_APC_CTRL);
		reg_val &= ~(0x1<<ADCREN);
		snd_soc_write(codec, ADC_APC_CTRL, reg_val);

		/*enable ADC Digital part & ENBLE Analog ADC mode(default value) */
		reg_val = snd_soc_read(codec, ADC_DIG_CTRL);
		reg_val &= ~((0x1<<ENAD)|(0x1<<ENDM));
		snd_soc_write(codec, ADC_DIG_CTRL, reg_val);

		/*AIF2 ADC right channel mixer source select */
		reg_val = snd_soc_read(codec, AIF2_MXR_SRC);
		reg_val &= ~(0x1<<AIF2_ADCR_MXR_SRC);
		snd_soc_write(codec, AIF2_MXR_SRC, reg_val);
	}

	return 0;
}

static int codec_get_analog_btphonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_analog_btphonein_en;
	return 0;
}


//	the system voice (such as music, movie sound...) come out from speaker.

static int codec_spk_play_open(struct snd_soc_codec *codec)
{
	int reg_val;

	printk("=======enter %s========\n",__func__);
//	msleep(3000);
//	printk("11111111enter111 %s\n",__func__);
	
	/*open aif1 DAC channel slot0 switch*/
	reg_val = snd_soc_read(codec, AIF1_DACDAT_CTRL);
	reg_val |= (0x1<<AIF1_DA0L_ENA)|(0x1<<AIF1_DA0R_ENA);
	snd_soc_write(codec, AIF1_DACDAT_CTRL,reg_val);

	/*select aif1 da0 source*/
	reg_val = snd_soc_read(codec, AIF1_DACDAT_CTRL);
	reg_val &= ~(0x3<<AIF1_DA0L_SRC);
	reg_val &= ~(0x3<<AIF1_DA0R_SRC);
	snd_soc_write(codec, AIF1_DACDAT_CTRL, reg_val);

	/*select  DAC mixer source*/
	reg_val = snd_soc_read(codec, DAC_MXR_SRC);
	reg_val |= (0x1<<DACL_MXR_AIF1_DA0L)|(0x1<<DACR_MXR_AIF1_DA0R);
	snd_soc_write(codec, DAC_MXR_SRC, reg_val);

	/*enable digital DAC*/
	reg_val = snd_soc_read(codec, DAC_DIG_CTRL);
	reg_val |= (0x1<<ENDA);
	snd_soc_write(codec, DAC_DIG_CTRL, reg_val);

	/*enable analog DAC*/
	reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
	reg_val |= (0x1<<DACALEN)|(0X1<<DACAREN);
	snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

	/*enable output mixer*/
	reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
	reg_val |= (0x1<<LMIXEN)|(0X1<<RMIXEN);
	snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

	/*select output mixer source select*/
	reg_val =  snd_soc_read(codec, OMIXER_SR);
	reg_val |= (0x1<<RMIXMUTEDACR)|(0x1<<LMIXMUTEDACL);
	snd_soc_write(codec, OMIXER_SR, reg_val);

	/*disable headphone power amplifier*/
	reg_val = snd_soc_read(codec, HPOUT_CTRL);
	reg_val &= ~(0x1<<HPPA_EN);
	snd_soc_write(codec, HPOUT_CTRL, reg_val);
	/*hardware xzh support*/
	reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
	reg_val &= ~(0xf<<HPOUTPUTENABLE);
	snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);
	/*mute l/r headphone pa*/
	reg_val = snd_soc_read(codec, HPOUT_CTRL);
	reg_val &= ~((0x1<<RHPPA_MUTE)|(0x1<<LHPPA_MUTE));
	snd_soc_write(codec, HPOUT_CTRL, reg_val);

	usleep_range(2000, 3000);
//	printk("2222222222enter22222222 %s\n",__func__);

	/*select spk source and  volume*/
	reg_val = snd_soc_read(codec, SPKOUT_CTRL);
	reg_val &= ~((0x1<<RSPKS)|(0x1<<LSPKS));
	reg_val |= speaker_val<<SPK_VOL;
	snd_soc_write(codec, SPKOUT_CTRL, reg_val);

	//printk("########enter111 %s ########\n",__func__);
	//msleep(2000);
	msleep(50);
	/*speaker enable*/
	reg_val = snd_soc_read(codec, SPKOUT_CTRL);
	reg_val |= (0x1<<RSPK_EN)|(0x1<<LSPK_EN);
	snd_soc_write(codec, SPKOUT_CTRL, reg_val);

	usleep_range(2000, 3000);
	if (play_running) {
		//enable external pa for speaker
		//gpio_set_value(item.gpio.gpio, 1);
		msleep(62);
	}
//	msleep(2000);
	printk("leave %s\n",__func__);
	return 0;
}


//	The system voice (such as music, movie sound...) come out from headphone.

static int codec_headphone_play_open(struct snd_soc_codec *codec)
{
	int reg_val;
	//disable pa_ctrl. turn off speaker's external pa.
	//gpio_set_value(item.gpio.gpio, 0);
	//msleep(62);
	/*disable speaker*/
	printk("enter %s\n",__func__);
	reg_val = snd_soc_read(codec, SPKOUT_CTRL);
	reg_val &= ~((0x1<<RSPK_EN)|(0x1<<LSPK_EN));
	snd_soc_write(codec, SPKOUT_CTRL, reg_val);

	/*enble l/r Analog Output Mixer */
	reg_val =  snd_soc_read(codec, OMIXER_DACA_CTRL);
	reg_val &= ~((0x1<<LMIXEN)|(0x1<<RMIXEN));
	snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

	/*open aif1 DAC channel slot0 switch*/
	reg_val= snd_soc_read(codec, AIF1_DACDAT_CTRL);
	reg_val|=(0x1<<AIF1_DA0L_ENA)|(0x1<<AIF1_DA0R_ENA);
	snd_soc_write(codec, AIF1_DACDAT_CTRL,reg_val);

	/*select aif1 da0 source*/
	reg_val = snd_soc_read(codec, AIF1_DACDAT_CTRL);
	reg_val &=~(0x3<<AIF1_DA0L_SRC);
	reg_val &=~(0x3<<AIF1_DA0R_SRC);
	snd_soc_write(codec, AIF1_DACDAT_CTRL, reg_val);
	/*enable digital DAC*/
	reg_val = snd_soc_read(codec, DAC_DIG_CTRL);
	reg_val |= (0x1<<ENDA);
	snd_soc_write(codec, DAC_DIG_CTRL, reg_val);

	if (codec_headphoneout_en) {/*phone on:keypad tone*/
			/*select  DAC mixer source*/
		reg_val = snd_soc_read(codec, DAC_MXR_SRC);
		reg_val |= (0x1<<DACL_MXR_AIF1_DA0L);
		snd_soc_write(codec, DAC_MXR_SRC, reg_val);
			/*enable L/R internal analog channel*/
		reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
		reg_val |= (0x1<<DACALEN);
		snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

		/*select the ladcl/adcr source */
		reg_val =  snd_soc_read(codec, OMIXER_SR);
		reg_val |= (0x1<<LMIXMUTEDACL)|(0x1<<RMIXMUTEDACL);
		snd_soc_write(codec, OMIXER_SR, reg_val);

	} else {/*normal mode:keypad tone*/
		/*select  DAC mixer source*/
		reg_val = snd_soc_read(codec, DAC_MXR_SRC);
		reg_val |= (0x1<<DACL_MXR_AIF1_DA0L)|(0x1<<DACR_MXR_AIF1_DA0R);
		snd_soc_write(codec, DAC_MXR_SRC, reg_val);
		/*enable L/R internal analog channel*/
		reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
		reg_val |= (0x1<<DACAREN);
		reg_val |= (0x1<<DACALEN);
		snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

		reg_val = snd_soc_read(codec, HPOUT_CTRL);
		reg_val &= ~((0x1<<RHPS)|(0x1<<LHPS));
		snd_soc_write(codec, HPOUT_CTRL, reg_val);
		/*config headphone volume */
		reg_val = snd_soc_read(codec, HPOUT_CTRL);
		reg_val &=~(0x3f<<HP_VOL);
		reg_val |=(headset_val<<HP_VOL);
		snd_soc_write(codec, HPOUT_CTRL, reg_val);
	}

	if (play_running) {
		reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
		reg_val |= (0xf<<HPOUTPUTENABLE);
		snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

		reg_val = snd_soc_read(codec, HPOUT_CTRL);
		reg_val |= (0x1<<HPPA_EN);
		snd_soc_write(codec, HPOUT_CTRL, reg_val);
		msleep(10);
		/*unmute l/r headphone pa*/
		reg_val = snd_soc_read(codec, HPOUT_CTRL);
		reg_val |= (0x1<<RHPPA_MUTE)|(0x1<<LHPPA_MUTE);
		snd_soc_write(codec, HPOUT_CTRL, reg_val);
	}
  
	return 0;
}

/*
*	the system voice(such as music, movie sound...) come out from speaker && headphone
*/

static int codec_spk_headset_play_open(struct snd_soc_codec *codec)
{
	int reg_val;
	/*open aif1 DAC channel slot0 switch*/
	reg_val = snd_soc_read(codec, AIF1_DACDAT_CTRL);
	reg_val |= (0x1<<AIF1_DA0L_ENA)|(0x1<<AIF1_DA0R_ENA);
	snd_soc_write(codec, AIF1_DACDAT_CTRL,reg_val);

	/*select aif1 da0 source*/
	reg_val = snd_soc_read(codec, AIF1_DACDAT_CTRL);
	reg_val &= ~(0x3<<AIF1_DA0L_SRC);
	reg_val &= ~(0x3<<AIF1_DA0R_SRC);
	snd_soc_write(codec, AIF1_DACDAT_CTRL, reg_val);

	/*select  DAC mixer source*/
	reg_val = snd_soc_read(codec, DAC_MXR_SRC);
	reg_val |= (0x1<<DACL_MXR_AIF1_DA0L)|(0x1<<DACR_MXR_AIF1_DA0R);
	snd_soc_write(codec, DAC_MXR_SRC, reg_val);

	/*enable digital DAC*/
	reg_val = snd_soc_read(codec, DAC_DIG_CTRL);
	reg_val |= (0x1<<ENDA);
	snd_soc_write(codec, DAC_DIG_CTRL, reg_val);

	/*enable L/R internal analog channel*/
	reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
	reg_val |= (0x1<<DACAREN);
	reg_val |= (0x1<<DACALEN);
	snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

	/*set headphone volume 0x3b=59*/
	reg_val = snd_soc_read(codec, HPOUT_CTRL);
	reg_val &= ~(0x3f<<HP_VOL);
	reg_val |= (headset_val<<HP_VOL);
	snd_soc_write(codec, HPOUT_CTRL, reg_val);
	if (play_running) {
		reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
		reg_val |= (0xf<<HPOUTPUTENABLE);
		snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

		reg_val = snd_soc_read(codec, HPOUT_CTRL);
		reg_val |= (0x1<<HPPA_EN);
		snd_soc_write(codec, HPOUT_CTRL, reg_val);
		msleep(10);
		/*unmute l/r headphone pa*/
		reg_val = snd_soc_read(codec, HPOUT_CTRL);
		reg_val |= (0x1<<RHPPA_MUTE)|(0x1<<LHPPA_MUTE);
		snd_soc_write(codec, HPOUT_CTRL, reg_val);
	}
	/*enable output mixer*/
	reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
	reg_val |= (0x1<<LMIXEN)|(0X1<<RMIXEN);
	snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

	/*select output mixer source select*/
	reg_val =  snd_soc_read(codec, OMIXER_SR);
	reg_val |= (0x1<<RMIXMUTEDACR)|(0x1<<LMIXMUTEDACL);
	snd_soc_write(codec, OMIXER_SR, reg_val);

	/*select spk source and  volume*/
	reg_val = snd_soc_read(codec, SPKOUT_CTRL);
	reg_val &= ~((0x1<<RSPKS)|(0x1<<LSPKS));
	reg_val |= speaker_val<<SPK_VOL;
	snd_soc_write(codec, SPKOUT_CTRL, reg_val);

	/*speaker enable*/
	reg_val = snd_soc_read(codec, SPKOUT_CTRL);
	reg_val |= (0x1<<RSPK_EN)|(0x1<<LSPK_EN);
	snd_soc_write(codec, SPKOUT_CTRL, reg_val);
	usleep_range(2000, 3000);
	if (play_running) {
		//config gpio info of audio_pa_ctrl open, turn on external pa for speaker.
		//gpio_set_value(item.gpio.gpio, 1);
		//msleep(62);

	}
	
	return 0;
}

/*
*	the system voice(such as music, movie sound...) come out from earpiece.
*/

static int codec_earpiece_play_open(struct snd_soc_codec *codec)
{
	int reg_val;
	/*enable timeslot 0 DAC l/r channel*/
	reg_val = snd_soc_read(codec, AIF1_DACDAT_CTRL);
	reg_val &= ~(0x1<<AIF1_DA0L_ENA);
	reg_val |= (0x1<<AIF1_DA0L_ENA);
	snd_soc_write(codec, AIF1_DACDAT_CTRL, reg_val);

	/*select aif1 da0 left source*/
	reg_val = snd_soc_read(codec, AIF1_DACDAT_CTRL);
	reg_val &= ~(0x3<<AIF1_DA0L_SRC);
	snd_soc_write(codec, AIF1_DACDAT_CTRL, reg_val);

	/*set timeslot 0 DAC l/r channel volume*/
	reg_val = snd_soc_read(codec, AIF1_VOL_CTRL3);
	reg_val &= ~(0xff<<AIF1_DA0L_VOL);
	reg_val &= ~(0xff<<AIF1_DA0R_VOL);
	reg_val |= (0xa0<<AIF1_DA0L_VOL);
	reg_val |= (0xa0<<AIF1_DA0R_VOL);
	snd_soc_write(codec, AIF1_VOL_CTRL3, reg_val);

	/*set dac l/r channel volume*/
	reg_val = snd_soc_read(codec, DAC_VOL_CTRL);
	reg_val &= ~(0xff<<DAC_VOL_L);
	reg_val &= ~(0xff<<DAC_VOL_R);
	reg_val |= (0xa0<<DAC_VOL_L);
	reg_val |= (0xa0<<DAC_VOL_R);
	snd_soc_write(codec, DAC_VOL_CTRL, reg_val);

	/*enable L/R internal analog channel*/
	reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
	reg_val |= (0x1<<DACALEN);
	snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

	/*select  DAC mixer source*/
	reg_val = snd_soc_read(codec, DAC_MXR_SRC);
	reg_val |= (0x1<<DACL_MXR_AIF1_DA0L);
	snd_soc_write(codec, DAC_MXR_SRC, reg_val);

	/*enable digital DAC*/
	reg_val = snd_soc_read(codec, DAC_DIG_CTRL);
	reg_val |= (0x1<<ENDA);
	snd_soc_write(codec, DAC_DIG_CTRL, reg_val);
	if (codec_earpieceout_en) {
		/*enable L/R analog mixer output*/
		reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
		reg_val |= (0x1<<LMIXEN);
		snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

		/*select output mixer source select*/
		reg_val =  snd_soc_read(codec, OMIXER_SR);
		reg_val |= (0x1<<LMIXMUTEDACL);
		snd_soc_write(codec, OMIXER_SR, reg_val);

		/*earpiece input source select DACL*/
		reg_val = snd_soc_read(codec, ESPKOUT_CTRL);
		reg_val &= ~(0x3<<ESPSR);
		reg_val |= (0x3<<ESPSR);
		snd_soc_write(codec, ESPKOUT_CTRL, reg_val);
	} else {
		/*earpiece input source select DACL*/
		reg_val = snd_soc_read(codec, ESPKOUT_CTRL);
		reg_val &= ~(0x3<<ESPSR);
		reg_val |= (0x1<<ESPSR);
		snd_soc_write(codec, ESPKOUT_CTRL, reg_val);
	}

	/*set earpiece volume 0x3b=59*/
	reg_val = snd_soc_read(codec, ESPKOUT_CTRL);
	reg_val &= ~(0x1f<<ESP_VOL);
	reg_val |= (0x1a<<ESP_VOL);
	snd_soc_write(codec, ESPKOUT_CTRL, reg_val);

	return 0;
}


static int codec_bt_play_open(struct snd_soc_codec *codec)
{
	int reg_val;
	printk("%s,line:%d\n",__func__,__LINE__);
	/*open aif1 DAC channel slot0 switch*/
	reg_val = snd_soc_read(codec, AIF1_DACDAT_CTRL);
	reg_val |= (0x1<<AIF1_DA0R_ENA);
	snd_soc_write(codec, AIF1_DACDAT_CTRL,reg_val);

	/*AIF2 ADC right channel mixer source select */
	reg_val = snd_soc_read(codec, AIF2_MXR_SRC);
	reg_val |= (0x1<<11);
	snd_soc_write(codec, AIF2_MXR_SRC, reg_val);
	return 0;
}

/*
*	codec_speaker_headset_earpiece_en == 4, AIF1_DA0R & AIF2 ADC right channel mixer source select AIF1_DA0R
*	codec_speaker_headset_earpiece_en == 3, earpiece is open,speaker and headphone is close.
*	codec_speaker_headset_earpiece_en == 2, speaker is open, headphone is open.
*	codec_speaker_headset_earpiece_en == 1, speaker is open, headphone is close.
*	codec_speaker_headset_earpiece_en == 0, speaker is closed, headphone is open.
*	this function just used for the system voice(such as music and moive sound and so on),
*	no the phone call.
*/
static int codec_set_spk_headset_earpiece(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	codec_speaker_headset_earpiece_en = ucontrol->value.integer.value[0];

      printk("enter %s:codec_speaker_headset_earpiece_en=%d\n",__func__,codec_speaker_headset_earpiece_en);

	if (codec_speaker_headset_earpiece_en == 1) {
		codec_spk_play_open(codec);
	} else if (codec_speaker_headset_earpiece_en == 2) {
		codec_spk_headset_play_open(codec);
	} else if (codec_speaker_headset_earpiece_en == 3) {
		codec_earpiece_play_open(codec);
	} else if (codec_speaker_headset_earpiece_en == 0) {
		codec_headphone_play_open(codec);
	}else if(codec_speaker_headset_earpiece_en == 4) {
		codec_bt_play_open(codec);
	}

	return 0;
}

static int codec_get_spk_headset_earpiece(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_speaker_headset_earpiece_en;
	printk("enter %s:codec_speaker_headset_earpiece_en=%d\n",__func__,codec_speaker_headset_earpiece_en);
	return 0;
}

static const char *spk_headset_earpiece_function[] = {"headset", "spk", "spk_headset", "earpiece",
													  "bt_button_voice"};
static const struct soc_enum spk_headset_earpiece_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(spk_headset_earpiece_function), spk_headset_earpiece_function),
};

static int codec_get_bt_button_voice(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = bt_bb_button_voice ;
	return 0;
}

static int codec_set_bt_button_voice(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	bt_bb_button_voice =  ucontrol->value.integer.value[0];
	return 0;
}

static int codec_set_system_bt_capture_flag(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_system_bt_capture_en =  ucontrol->value.integer.value[0];
	return 0;
}

static int codec_get_system_bt_capture_flag(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0]=codec_system_bt_capture_en;
	return 0;
}

static int codec_set_headsetmic_voicerecord(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_headsetmic_voicerecord =  ucontrol->value.integer.value[0];
	return 0;
}

static int codec_get_headsetmic_voicerecord(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0]=codec_headsetmic_voicerecord;
	return 0;
}

/*
*	phone record from main mic + phone in(digital bb) .
* 	or
*	phone record from sub mic + phone in(digital bb) .
*	mic1 uses as main mic. mic2 uses as sub mic
*/
static int codec_digital_voice_mic_bb_capture_open(struct snd_soc_codec *codec)
{
	int reg_val;
	/* select AIF1 output  mixer source(phone in from digital_bb)*/
	printk("enter %s\n",__func__);
	reg_val = snd_soc_read(codec, AIF1_MXR_SRC);
	reg_val |= (0x1<<AIF1_AD0L_AIF2_DACL_MXR);
	snd_soc_write(codec, AIF1_MXR_SRC,reg_val);

	/* select AIF1 output  mixer source(mic1)*/
	reg_val = snd_soc_read(codec, AIF1_MXR_SRC);
	reg_val |= (0x1<<AIF1_AD0L_ADCL_MXR);
	snd_soc_write(codec, AIF1_MXR_SRC,reg_val);

	/*AIF1 ADC Timeslot0 left channel data source select*/
	reg_val = snd_soc_read(codec, AIF1_ADCDAT_CTRL);
	reg_val &= ~(0x3<<AIF1_AD0L_SRC);
	snd_soc_write(codec, AIF1_ADCDAT_CTRL,reg_val);

	/*open ADC channel slot0 switch*/
	reg_val = snd_soc_read(codec, AIF1_ADCDAT_CTRL);
	reg_val |= (0x1<<AIF1_AD0L_ENA);
	snd_soc_write(codec, AIF1_ADCDAT_CTRL,reg_val);
	return 0;
}

static int codec_digital_voice_bb_bt_capture_open(struct snd_soc_codec *codec)
{
	int reg_val;
	/* select AIF1 output  mixer source(phone in from digital_bb) */
	printk("enter %s\n",__func__);
	reg_val = snd_soc_read(codec, AIF1_MXR_SRC);
	reg_val |= (0x1<<AIF1_AD0L_AIF2_DACL_MXR);
	snd_soc_write(codec, AIF1_MXR_SRC,reg_val);

	/* select AIF1 output  mixer source(mic1)*/
	reg_val = snd_soc_read(codec, AIF1_MXR_SRC);
	reg_val |= (0x1<<AIF1_AD0L_AIF2_DACR_MXR);
	snd_soc_write(codec, AIF1_MXR_SRC,reg_val);

	/*AIF1 ADC Timeslot0 left channel data source select*/
	reg_val = snd_soc_read(codec, AIF1_ADCDAT_CTRL);
	reg_val &= ~(0x3<<AIF1_AD0L_SRC);
	snd_soc_write(codec, AIF1_ADCDAT_CTRL,reg_val);

	/*open ADC channel slot0 switch*/
	reg_val = snd_soc_read(codec, AIF1_ADCDAT_CTRL);
	reg_val |= (0x1<<AIF1_AD0L_ENA);
	snd_soc_write(codec, AIF1_ADCDAT_CTRL,reg_val);
	return 0;
}
/*
*	use for phone record from main mic + phone in.
*	mic1 is use as main mic.
*/
static int codec_analog_voice_main_mic_capture_open(struct snd_soc_codec *codec)
{
	int reg_val;
	/*select mic1/2 boost control*/

	printk("enter %s\n",__func__);
	
	reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
	reg_val &= ~((0x1<<MIC1AMPEN)|(0x1<<MIC2AMPEN));
	reg_val |= (0x1<<MIC1AMPEN)|(0x0<<MIC2AMPEN);
	snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

	/*setup mic1 boost amplifier Gain control*/
	reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
	reg_val |= (0x7<<ADC_MIC1G);
	snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

	/*select adc mixer source*/
	reg_val = snd_soc_read(codec, ADC_SRC);
	reg_val |= (0x1<<LADCMIXMUTEMIC1BOOST);
	snd_soc_write(codec, ADC_SRC, reg_val);

	/*select the phonein source*/
	reg_val = snd_soc_read(codec, ADC_SRC);
	reg_val |= (0x1<<LADCMIXMUTELINEINLR);
	snd_soc_write(codec, ADC_SRC, reg_val);

	/*ADC L/R channel Enable*/
	reg_val = snd_soc_read(codec, ADC_APC_CTRL);
	reg_val &= ~(0x1<<ADCLEN);
	reg_val |= (0x1<<ADCLEN);
	snd_soc_write(codec, ADC_APC_CTRL, reg_val);

	/*mic1 BIAS Enable*/
	reg_val = snd_soc_read(codec, ADC_APC_CTRL);
	reg_val &= ~(0x1<<MBIASEN);
	reg_val |= (0x1<<MBIASEN);
	snd_soc_write(codec, ADC_APC_CTRL, reg_val);

	/*enable ADC Digital part & enable Analog ADC mode */
	reg_val = snd_soc_read(codec, ADC_DIG_CTRL);
	reg_val &= ~((0x1<<ENAD)|(0x1<<ENDM));
	reg_val |= (0x1<<ENAD)|(0x0<<ENDM);
	snd_soc_write(codec, ADC_DIG_CTRL, reg_val);

	/* select AIF1 output  mixer source*/
	reg_val = snd_soc_read(codec, AIF1_MXR_SRC);
	reg_val |= (0x1<<AIF1_AD0L_ADCL_MXR);
	snd_soc_write(codec, AIF1_MXR_SRC,reg_val);

	/*open ADC channel slot0 switch*/
	reg_val= snd_soc_read(codec, AIF1_ADCDAT_CTRL);
	//reg_val&=~(0x3<<AIF1_AD0R_ENA);
	reg_val |= (0x1<<AIF1_AD0L_ENA);
	snd_soc_write(codec, AIF1_ADCDAT_CTRL,reg_val);
	return 0;
}

/*
*	use for phone record from sub mic + phone in.
*	mic2 is use as sub mic.
* 	mic2 is the headset mic.
*/
static int codec_analog_voice_headset_mic_capture_open(struct snd_soc_codec *codec)
{
	int reg_val;

	/*select  mic1/2 boost control*/
	printk("enter %s\n",__func__);
	reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
	reg_val &= ~((0x1<<MIC1AMPEN)|(0x1<<MIC2AMPEN));
	reg_val |= (0x0<<MIC1AMPEN)|(0x1<<MIC2AMPEN);
	snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

	/*setup mic2 boost amplifier Gain control*/
	reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
	reg_val |= (0x4<<ADC_MIC2G);
	snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

	/*select  adc mixer source*/
	reg_val = snd_soc_read(codec, ADC_SRC);
	reg_val |= (0x1<<LADCMIXMUTEMIC2BOOST);
	snd_soc_write(codec, ADC_SRC, reg_val);

	/*select the phonein source*/
	reg_val = snd_soc_read(codec, ADC_SRC);
	reg_val |= (0x1<<LADCMIXMUTELINEINLR);
	snd_soc_write(codec, ADC_SRC, reg_val);

	/*ADC L/R channel Enable*/
	reg_val = snd_soc_read(codec, ADC_APC_CTRL);
	reg_val &= ~(0x1<<ADCLEN);
	reg_val |= (0x1<<ADCLEN);
	snd_soc_write(codec, ADC_APC_CTRL, reg_val);

	/*Headset microphone BIAS enable*/
	reg_val = snd_soc_read(codec, ADC_APC_CTRL);
	reg_val &= ~(0x1<<HBIASEN);
	reg_val |= (0x1<<HBIASEN);
	snd_soc_write(codec, ADC_APC_CTRL, reg_val);

	/*enable ADC Digital part & enable Analog ADC mode */
	reg_val = snd_soc_read(codec, ADC_DIG_CTRL);
	reg_val &= ~((0x1<<ENAD)|(0x1<<ENDM));
	reg_val |= (0x1<<ENAD)|(0x0<<ENDM);
	snd_soc_write(codec, ADC_DIG_CTRL, reg_val);

	/* select AIF1 output  mixer source*/
	reg_val = snd_soc_read(codec, AIF1_MXR_SRC);
	reg_val |= (0x1<<AIF1_AD0L_ADCL_MXR);
	snd_soc_write(codec, AIF1_MXR_SRC,reg_val);

	/*open ADC channel slot0 switch*/
	reg_val = snd_soc_read(codec, AIF1_ADCDAT_CTRL);
	reg_val &= ~(0x3<<AIF1_AD0R_ENA);
	reg_val |= (0x1<<AIF1_AD0L_ENA);
	snd_soc_write(codec, AIF1_ADCDAT_CTRL,reg_val);

	return 0;
}

static int codec_analog_voice_bb_bt_capture_open(struct snd_soc_codec *codec)
{
	int reg_val;
	/* select AIF1 output  mixer source*/
	printk("enter %s\n",__func__);
	reg_val = snd_soc_read(codec, AIF1_MXR_SRC);
	reg_val |= (0x1<<AIF1_AD0R_ADCR_MXR) |(0x1<<AIF1_AD0R_AIF2_DACL_MXR) ;
	snd_soc_write(codec, AIF1_MXR_SRC,reg_val);

	/*AIF1 ADC Timeslot0 left channel data source select*/
	reg_val = snd_soc_read(codec, AIF1_ADCDAT_CTRL);
	reg_val &= ~(0x3<<AIF1_AD0L_SRC);
	reg_val |= (0x1<<AIF1_AD0L_SRC);
	snd_soc_write(codec, AIF1_ADCDAT_CTRL,reg_val);

	/*open ADC channel slot0 switch*/
	reg_val = snd_soc_read(codec, AIF1_ADCDAT_CTRL);
	reg_val &= ~(0x3<<AIF1_AD0R_ENA);
	reg_val |= (0x1<<AIF1_AD0L_ENA)|(0x1<<AIF1_AD0R_ENA);
	snd_soc_write(codec, AIF1_ADCDAT_CTRL,reg_val);

	return 0;
}

static int codec_set_digitalbb_capture_mic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int reg_val;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	codec_digitalbb_capture_mic =  ucontrol->value.integer.value[0];
	if (codec_digitalbb_capture_mic) {
		codec_digital_voice_mic_bb_capture_open(codec);
	} else {
		reg_val = snd_soc_read(codec, AIF1_MXR_SRC);
		/* select AIF1 output  mixer source(phone in from digital_bb)*/
		reg_val &= ~(0x1<<AIF1_AD0L_AIF2_DACL_MXR);
		/* select AIF1 output  mixer source(mic1)*/
		reg_val &= ~(0x1<<AIF1_AD0L_ADCL_MXR);
		snd_soc_write(codec, AIF1_MXR_SRC,reg_val);

	}
	return 0;
}

static int codec_get_digitalbb_capture_mic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0]=codec_digitalbb_capture_mic;
	return 0;
}


static int codec_set_digitalbb_capture_bt(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int reg_val;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	codec_digitalbb_capture_bt =  ucontrol->value.integer.value[0];
	if (codec_digitalbb_capture_bt) {
		codec_digital_voice_bb_bt_capture_open(codec);
	} else {
		/* select AIF1 output  mixer source(phone in from digital_bb)*/
		reg_val = snd_soc_read(codec, AIF1_MXR_SRC);
		reg_val &= ~(0x1<<AIF1_AD0L_AIF2_DACL_MXR);
		reg_val &= ~(0x1<<AIF1_AD0L_AIF2_DACR_MXR);
		snd_soc_write(codec, AIF1_MXR_SRC,reg_val);
	}
	return 0;
}

static int codec_get_digitalbb_capture_bt(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0]=codec_digitalbb_capture_bt;
	return 0;
}


static int codec_set_analogbb_capture_mainmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int reg_val;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	codec_analogbb_capture_mainmic =  ucontrol->value.integer.value[0];
	if (codec_analogbb_capture_mainmic) {
		codec_analog_voice_main_mic_capture_open(codec);
	} else {

		/*select adc mixer source*/
		reg_val = snd_soc_read(codec, ADC_SRC);
		reg_val &= ~(0x1<<LADCMIXMUTEMIC1BOOST);
		reg_val &= ~(0x1<<LADCMIXMUTELINEINLR);
		snd_soc_write(codec, ADC_SRC, reg_val);
		/* select AIF1 output  mixer source*/
		reg_val = snd_soc_read(codec, AIF1_MXR_SRC);
		reg_val &= ~(0x1<<AIF1_AD0L_ADCL_MXR);
		snd_soc_write(codec, AIF1_MXR_SRC,reg_val);

		/*open ADC channel slot0 switch*/
		reg_val = snd_soc_read(codec, AIF1_ADCDAT_CTRL);
		//reg_val&=~(0x3<<AIF1_AD0R_ENA);
		reg_val &= ~(0x1<<AIF1_AD0L_ENA);
		snd_soc_write(codec, AIF1_ADCDAT_CTRL,reg_val);
	}
	return 0;
}

static int codec_get_analogbb_capture_mainmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0]=codec_analogbb_capture_mainmic;
	return 0;
}

static int codec_set_analogbb_capture_headsetmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int reg_val;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	codec_analogbb_capture_headsetmic =  ucontrol->value.integer.value[0];
	if (codec_analogbb_capture_headsetmic) {
		codec_analog_voice_headset_mic_capture_open(codec);
	} else {
		/*select  adc mixer source*/
		reg_val = snd_soc_read(codec, ADC_SRC);
		reg_val &= ~(0x1<<LADCMIXMUTEMIC2BOOST);
		reg_val &= ~(0x1<<LADCMIXMUTELINEINLR);
		snd_soc_write(codec, ADC_SRC, reg_val);

		/* select AIF1 output  mixer source*/
		reg_val = snd_soc_read(codec, AIF1_MXR_SRC);
		reg_val &= ~(0x1<<AIF1_AD0L_ADCL_MXR);
		snd_soc_write(codec, AIF1_MXR_SRC,reg_val);

		/*open ADC channel slot0 switch*/
		reg_val = snd_soc_read(codec, AIF1_ADCDAT_CTRL);
		reg_val &= ~(0x3<<AIF1_AD0R_ENA);
		//reg_val|=(0x1<<AIF1_AD0L_ENA);
		snd_soc_write(codec, AIF1_ADCDAT_CTRL,reg_val);
	}
	return 0;
}

static int codec_get_analogbb_capture_headsetmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_analogbb_capture_headsetmic;
	return 0;
}

static int codec_set_analogbb_capture_bt(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int reg_val;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	codec_analogbb_capture_bt =  ucontrol->value.integer.value[0];
	if (codec_analogbb_capture_bt) {
		codec_analog_voice_bb_bt_capture_open(codec);

	} else {
		/* select AIF1 output  mixer source*/
		reg_val = snd_soc_read(codec, AIF1_MXR_SRC);
		reg_val &= ~((0x1<<AIF1_AD0R_ADCR_MXR)|(0x1<<AIF1_AD0R_AIF2_DACL_MXR));
		snd_soc_write(codec, AIF1_MXR_SRC,reg_val);

		/*AIF1 ADC Timeslot0 left channel data source select*/
		reg_val = snd_soc_read(codec, AIF1_ADCDAT_CTRL);
		reg_val &= ~(0x1<<AIF1_AD0L_SRC);
		snd_soc_write(codec, AIF1_ADCDAT_CTRL,reg_val);

		/*open ADC channel slot0 switch*/
		reg_val = snd_soc_read(codec, AIF1_ADCDAT_CTRL);
		reg_val &= ~(0x3<<AIF1_AD0R_ENA);
		//reg_val|=(0x1<<AIF1_AD0L_ENA)|(0x1<<AIF1_AD0R_ENA);
		snd_soc_write(codec, AIF1_ADCDAT_CTRL,reg_val);

	}
	return 0;
}

static int codec_get_analogbb_capture_bt(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0]=codec_analogbb_capture_bt;
	return 0;
}


static const struct snd_kcontrol_new sndvir_audio_controls[] = {
	/*AIF1*/
	SOC_SINGLE("AIF1 ADC timeslot 0 left channel volume", AIF1_VOL_CTRL1, AIF1_AD0L_VOL, 0xff, 0),
	SOC_SINGLE("AIF1 ADC timeslot 0 right channel volume", AIF1_VOL_CTRL1, AIF1_AD0R_VOL, 0xff, 0),
	SOC_SINGLE("AIF1 ADC timeslot 1 left channel volume", AIF1_VOL_CTRL2, AIF1_AD1L_VOL, 0xff, 0),
	SOC_SINGLE("AIF1 ADC timeslot 1 right channel volume", AIF1_VOL_CTRL2, AIF1_AD1R_VOL, 0xff, 0),

	SOC_SINGLE("AIF1 DAC timeslot 0 left channel volume", AIF1_VOL_CTRL3, AIF1_DA0L_VOL, 0xff, 0),
	SOC_SINGLE("AIF1 DAC timeslot 0 right channel volume", AIF1_VOL_CTRL3, AIF1_DA0R_VOL, 0xff, 0),
	SOC_SINGLE("AIF1 DAC timeslot 1 left channel volume", AIF1_VOL_CTRL4, AIF1_DA1L_VOL, 0xff, 0),
	SOC_SINGLE("AIF1 DAC timeslot 1 right channel volume", AIF1_VOL_CTRL4, AIF1_DA1R_VOL, 0xff, 0),

	SOC_SINGLE("AIF1 ADC t_0_l_channel mixer gain control", AIF1_MXR_GAIN, AIF1_AD0L_MXR_GAIN, 0xf, 0),
	SOC_SINGLE("AIF1 ADC t_0_r_channel mixer gain control", AIF1_MXR_GAIN, AIF1_AD0R_MXR_GAIN, 0xf, 0),
	SOC_SINGLE("AIF1 ADC t_1_l_channel mixer gain control", AIF1_MXR_GAIN, AIF1_AD1L_MXR_GAIN, 0x3, 0),
	SOC_SINGLE("AIF1 ADC t_1_r_channel mixer gain control", AIF1_MXR_GAIN, AIF1_AD1R_MXR_GAIN, 0x3, 0),

	/*AIF2*/
	SOC_SINGLE("AIF2 ADC left channel volume", AIF2_VOL_CTRL1, AIF2_ADCL_VOL, 0xff, 0),
	SOC_SINGLE("AIF2 ADC right channel volume", AIF2_VOL_CTRL1, AIF2_ADCR_VOL, 0xff, 0),

	SOC_SINGLE("AIF2 DAC left channel volume", AIF2_VOL_CTRL2, AIF2_DACL_VOL, 0xff, 0),
	SOC_SINGLE("AIF2 DAC right channel volume", AIF2_VOL_CTRL2, AIF2_DACR_VOL, 0xff, 0),

	SOC_SINGLE("AIF2 ADC left channel mixer gain control", AIF2_MXR_GAIN, AIF2_ADCL_MXR_GAIN, 0xf, 0),
	SOC_SINGLE("AIF2 ADC right channel mixer gain control", AIF2_MXR_GAIN, AIF2_ADCR_MXR_GAIN, 0xf, 0),

	/*ADC*/
	SOC_SINGLE("ADC left channel volume", ADC_VOL_CTRL, ADC_VOL_L, 0xff, 0),
	SOC_SINGLE("ADC right channel volume", ADC_VOL_CTRL, ADC_VOL_R, 0xff, 0),

	/*DAC*/
	SOC_SINGLE("DAC left channel volume", DAC_VOL_CTRL, DAC_VOL_L, 0xff, 0),
	SOC_SINGLE("DAC right channel volume", DAC_VOL_CTRL, DAC_VOL_R, 0xff, 0),

	SOC_SINGLE("digital volume control", DAC_DBG_CTRL, DVC, 0x3f, 0),

	SOC_SINGLE("DAC left channel mixer gain control", DAC_MXR_GAIN, DACL_MXR_GAIN, 0xf, 0),
	SOC_SINGLE("DAC right channel mixer gain control", DAC_MXR_GAIN, DACR_MXR_GAIN, 0xf, 0),

	/*ADC*/
	SOC_SINGLE("ADC left channel input gain control", ADC_APC_CTRL, ADCLG, 0x7, 0),
	SOC_SINGLE("ADC right channel input gain control", ADC_APC_CTRL, ADCRG, 0x7, 0),

	SOC_SINGLE("MIC1 boost amplifier gain control", ADC_SRCBST_CTRL, ADC_MIC1G, 0x7, 0),
	SOC_SINGLE("MIC2 boost amplifier gain control", ADC_SRCBST_CTRL, ADC_MIC2G, 0x7, 0),
	SOC_SINGLE("LINEINL-LINEINR pre-amplifier gain control", ADC_SRCBST_CTRL, LINEIN_PREG, 0x7, 0),
	SOC_SINGLE("AUXI pre-amplifier gain control", ADC_SRCBST_CTRL, AUXI_PREG, 0x7, 0),

	SOC_SINGLE("AXin to L_R output mixer gain ctr", OMIXER_BST1_CTRL, AXG, 0x7, 0),
	SOC_SINGLE("MIC1 BST stage to L_R outp mixer gain ctr", OMIXER_BST1_CTRL, OMIXER_MIC1G, 0x7, 0),
	SOC_SINGLE("MIC2 BST stage to L_R outp mixer gain ctr", OMIXER_BST1_CTRL, OMIXER_MIC2G, 0x7, 0),
	SOC_SINGLE("LINEINL/R to L_R output mixer gain ctr", OMIXER_BST1_CTRL, LINEING, 0x7, 0),

	SOC_SINGLE("headphone volume control", HPOUT_CTRL, HP_VOL, 0x3f, 0),

	SOC_SINGLE("earpiece volume control", ESPKOUT_CTRL, ESP_VOL, 0x1f, 0),

	SOC_SINGLE("speaker_volume_control", SPKOUT_CTRL, SPK_VOL, 0x1f, 0),
//	SOC_SINGLE("speaker volume control", SPKOUT_CTRL, SPK_VOL, 23, 0),  // max=31

	SOC_SINGLE("line out gain control", LOUT_CTRL, LINEOUTG, 0x7, 0),
	/*enable phoneout*/
	SOC_SINGLE_BOOL_EXT("Audio phone out", 		0, codec_analog_get_phoneout, 	codec_analog_set_phoneout),
	/*open the phone in call*/
	SOC_SINGLE_BOOL_EXT("Audio phone in", 		0, codec_analog_get_phonein, 	codec_analog_set_phonein),
	/*set the phone in call voice through earpiece out*/
	SOC_SINGLE_BOOL_EXT("Audio earpiece out", 	0, codec_get_earpieceout, 	codec_set_earpieceout),
	/*set the phone in call voice through headphone out*/
	SOC_SINGLE_BOOL_EXT("Audio headphone out", 	0, codec_get_headphoneout, 	codec_set_headphoneout),
	/*set the phone in call voice through speaker out*/
	SOC_SINGLE_BOOL_EXT("Audio speaker out", 	0, codec_get_speakerout, 	codec_set_speakerout),
	/*set main mic(mic1)*/
	SOC_SINGLE_BOOL_EXT("Audio_analog_main_mic", 		0, codec_analog_get_mainmic, codec_analog_set_mainmic),
	/*set headset mic(mic2)*/
	SOC_SINGLE_BOOL_EXT("Audio analog headsetmic", 	0, codec_analog_get_headsetmic, codec_analog_set_headsetmic),
	/*set voicerecord status*/
	SOC_SINGLE_BOOL_EXT("Audio_phone_voicerecord", 	0, codec_get_voicerecord, 	codec_set_voicerecord),
	/*set voicerecord status*/
	SOC_SINGLE_BOOL_EXT("Audio phone endcall", 	0, codec_get_endcall, 	codec_set_endcall),

	SOC_SINGLE_BOOL_EXT("Audio linein record", 	0, codec_get_lineincap, codec_set_lineincap),
	SOC_SINGLE_BOOL_EXT("Audio linein in", 		0, codec_get_lineinin, 	codec_set_lineinin),
	SOC_ENUM_EXT("Speaker_Function", spk_headset_earpiece_enum[0], codec_get_spk_headset_earpiece, codec_set_spk_headset_earpiece),

	/*audio digital interface for phone case*/
	/*set mic1 for digital bb*/
	SOC_SINGLE_BOOL_EXT("Audio digital main mic", 	0, codec_digital_get_mainmic, codec_digital_set_mainmic),
	/*set mic2 for digital bb*/
	SOC_SINGLE_BOOL_EXT("Audio digital headset mic", 	0, codec_digital_get_headsetmic, codec_digital_set_headsetmic),
	/*set phoneout for digital bb*/
	SOC_SINGLE_BOOL_EXT("Audio digital phone out",	0, codec_digital_get_phoneout, codec_digital_set_phoneout),
	/*use for clear noise */
	SOC_SINGLE_BOOL_EXT("Audio digital dac out",	0, codec_digital_get_output, codec_digital_set_output),
	/*set phonein for digtal bb*/
	SOC_SINGLE_BOOL_EXT("Audio digital phonein",	0, codec_get_digital_phonein, codec_set_digital_phonein),
	/*set clk,format for digtal bb*/
	SOC_SINGLE_BOOL_EXT("Audio digital clk format status",	0, codec_get_digital_bb_clk_format, codec_set_digital_bb_clk_format),

	/*bluetooth*/
	/*set clk,format for bt*/
	SOC_SINGLE_BOOL_EXT("Audio bt clk format status",	0, codec_get_bt_clk_format, codec_set_bt_clk_format),
	/*set bt out*/
	SOC_SINGLE_BOOL_EXT("Audio bt out",	0, codec_get_bt_out, codec_set_bt_out),
	/*set analog bt mic*/
	SOC_SINGLE_BOOL_EXT("Audio analog bt mic",	0, codec_get_analog_btmic, codec_set_analog_btmic),
	/*set analog bt phonein*/
	SOC_SINGLE_BOOL_EXT("Audio analog bt phonein",	0, codec_get_analog_btphonein, codec_set_analog_btphonein),
	/* set bt mic for dbb*/
	SOC_SINGLE_BOOL_EXT("Audio digital bt mic",	0, codec_get_digital_btmic, codec_set_digital_btmic),
	/*set bt phonein for dbb*/
	SOC_SINGLE_BOOL_EXT("Audio digital bt phonein",	0, codec_get_digital_btphonein, codec_set_digital_btphonein),
	/*bt_bb_out*/
	SOC_SINGLE_BOOL_EXT("Audio bt button voice",	0, codec_get_bt_button_voice, codec_set_bt_button_voice),
	/*set bt phonein for dbb*/
	SOC_SINGLE_BOOL_EXT("Audio digital bb bt clk format", 0, codec_get_digital_bb_bt_clk_format, codec_set_digital_bb_bt_clk_format),
	/*set bt phonein for dbb*/
	SOC_SINGLE_BOOL_EXT("Audio system bt capture flag", 0, codec_get_system_bt_capture_flag, codec_set_system_bt_capture_flag),
	/*set voicerecord status for headphone mic*/
	SOC_SINGLE_BOOL_EXT("Audio headsetmic voicerecord", 	0, codec_get_headsetmic_voicerecord, 	codec_set_headsetmic_voicerecord),

	/*set voicerecord routway for call*/
	SOC_SINGLE_BOOL_EXT("Audio digital bb capture mic", 	0, codec_get_digitalbb_capture_mic, 	codec_set_digitalbb_capture_mic),
	SOC_SINGLE_BOOL_EXT("Audio digital bb capture bt", 	0, codec_get_digitalbb_capture_bt, 	codec_set_digitalbb_capture_bt),

	SOC_SINGLE_BOOL_EXT("Audio analog bb capture mainmic", 	0, codec_get_analogbb_capture_mainmic, 	codec_set_analogbb_capture_mainmic),
	SOC_SINGLE_BOOL_EXT("Audio analog bb capture headsetmic", 	0, codec_get_analogbb_capture_headsetmic, 	codec_set_analogbb_capture_headsetmic),
	SOC_SINGLE_BOOL_EXT("Audio analog bb capture bt", 	0, codec_get_analogbb_capture_bt, 	codec_set_analogbb_capture_bt),

};

static int current_substream = -1;
static int sndvir_audio_umute(struct snd_soc_dai *codec_dai, int mute)
{
	int reg_val;

	struct snd_soc_codec *codec = codec_dai->codec;
	if (current_substream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (mute == 0) {/*enable switch*/
			switch (codec_speaker_headset_earpiece_en) {
			case 0:
				/*hardware:xzh*/
				reg_val = snd_soc_read(codec, SPKOUT_CTRL);
				reg_val |= (0x7<<13);
				snd_soc_write(codec, SPKOUT_CTRL, reg_val);

				reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
				reg_val |= (0xf<<HPOUTPUTENABLE);
				snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

				reg_val = snd_soc_read(codec, HPOUT_CTRL);
				reg_val |= (0x1<<HPPA_EN);
				snd_soc_write(codec, HPOUT_CTRL, reg_val);
				msleep(10);
				/*unmute l/r headphone pa*/
				reg_val = snd_soc_read(codec, HPOUT_CTRL);
				reg_val |= (0x1<<RHPPA_MUTE)|(0x1<<LHPPA_MUTE);
				snd_soc_write(codec, HPOUT_CTRL, reg_val);
				break;
			case 1:
				msleep(100);
				write_power_item_value(POWER_SPK_EN,1);
				//gpio_set_value(item.gpio.gpio, 1);
				msleep(62);
				break;
			case 2:
				reg_val = snd_soc_read(codec, SPKOUT_CTRL);
				reg_val |= (0x7<<13);
				snd_soc_write(codec, SPKOUT_CTRL, reg_val);

				reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
				reg_val |= (0xf<<HPOUTPUTENABLE);
				snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

				reg_val = snd_soc_read(codec, HPOUT_CTRL);
				reg_val |= (0x1<<HPPA_EN);
				snd_soc_write(codec, HPOUT_CTRL, reg_val);
				write_power_item_value(POWER_SPK_EN,1);
				//gpio_set_value(item.gpio.gpio, 1);
				msleep(10);
				/*unmute l/r headphone pa*/
				reg_val = snd_soc_read(codec, HPOUT_CTRL);
				reg_val |= (0x1<<RHPPA_MUTE)|(0x1<<LHPPA_MUTE);
				snd_soc_write(codec, HPOUT_CTRL, reg_val);
				msleep(50);
				break;
			case 3:
				/*unmute earpiece line*/
				reg_val = snd_soc_read(codec, ESPKOUT_CTRL);
				reg_val |= (0x1<<ESPPA_MUTE);
				snd_soc_write(codec, ESPKOUT_CTRL, reg_val);

				/*enable earpiece power amplifier*/
				reg_val = snd_soc_read(codec, ESPKOUT_CTRL);
				reg_val |= (0x1<<ESPPA_EN);
				snd_soc_write(codec, ESPKOUT_CTRL, reg_val);
				break;
			default:
				break;
			}
		} else {
			write_power_item_value(POWER_SPK_EN,0);
		}

	} else if (current_substream == SNDRV_PCM_STREAM_CAPTURE) {
	}

	return 0;
}

static int sndvir_audio_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *codec_dai)
{
	int reg_val = 0;
	struct snd_soc_codec *codec = codec_dai->codec;
	
	/*enable AIF1CLK,*/
	reg_val = snd_soc_read(codec, SYSCLK_CTRL);
	/*aif1 clk source from MCLK1*/
	//reg_val &= ~(0x3<<AIF1CLK_SRC);

	if (codec_digital_bb_clk_format_init) {
		/*system clk from aif2*/
		reg_val |= (0x1<<SYSCLK_SRC);
		reg_val |= (0x1<<SYSCLK_ENA);
	} else {
		/*system clk from aif1*/
		reg_val &= ~(0x1<<SYSCLK_SRC);
		/*enable AIF1CLK,systemclk*/
		reg_val |= (0x1<<AIF1CLK_ENA)|(0x1<<SYSCLK_ENA);
	}
	snd_soc_write(codec, SYSCLK_CTRL, reg_val);
	
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_running = 1;
		current_substream = SNDRV_PCM_STREAM_PLAYBACK;
		/*enable module AIF1,DAC*/
		reg_val = snd_soc_read(codec, MOD_CLK_ENA);
		reg_val |= (0x1<<MOD_CLK_AIF1)|(0x1<<MOD_CLK_DAC_DIG);
		snd_soc_write(codec, MOD_CLK_ENA, reg_val);

		/*reset module AIF1, DAC*/
		reg_val = snd_soc_read(codec, MOD_RST_CTRL);
		reg_val |= (0x1<<MOD_RESET_AIF1)|(0x1<<MOD_RESET_DAC_DIG);
		snd_soc_write(codec, MOD_RST_CTRL, reg_val);
	} else {
		if (!codec_voice_record_en) {
			cap_running = 1;
		}

		current_substream = SNDRV_PCM_STREAM_CAPTURE;
		/*enable module AIF1,ADC*/
		reg_val = snd_soc_read(codec, MOD_CLK_ENA);
		reg_val |= (0x1<<MOD_CLK_AIF1)|(0x1<<MOD_CLK_ADC_DIG);
		snd_soc_write(codec, MOD_CLK_ENA, reg_val);

		/*reset module AIF1, ADC*/
		reg_val = snd_soc_read(codec, MOD_RST_CTRL);
		reg_val |= (0x1<<MOD_RESET_AIF1)|(0x1<<MOD_RESET_ADC_DIG);
		snd_soc_write(codec, MOD_RST_CTRL, reg_val);
	}
	
	return 0;
}

static void sndvir_audio_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *codec_dai)
{
	int reg_val = 0;
	struct snd_soc_codec *codec = codec_dai->codec;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if ( !(codec_speakerout_en || codec_headphoneout_en || codec_earpieceout_en ||
					codec_lineinin_en || codec_voice_record_en || codec_bt_clk_format || codec_digital_bb_bt_clk_format) ) {
			switch (codec_speaker_headset_earpiece_en){
			case  0:
				/*set headphone volume to 0*/
				reg_val = snd_soc_read(codec, HPOUT_CTRL);
				reg_val &= ~(0x3f<<HP_VOL);
				snd_soc_write(codec, HPOUT_CTRL, reg_val);

				/*disable pa*/
				reg_val = snd_soc_read(codec, HPOUT_CTRL);
				reg_val &= ~(0x1<<HPPA_EN);
				snd_soc_write(codec, HPOUT_CTRL, reg_val);

				/*hardware xzh support*/
				reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
				reg_val &= ~(0xf<<HPOUTPUTENABLE);
				snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

				/*unmute l/r headphone pa*/
				reg_val = snd_soc_read(codec, HPOUT_CTRL);
				reg_val &= ~((0x1<<RHPPA_MUTE)|(0x1<<LHPPA_MUTE));
				snd_soc_write(codec, HPOUT_CTRL, reg_val);

				reg_val = snd_soc_read(codec, HPOUT_CTRL);
				reg_val &= ~((0x1<<RHPS)|(0x1<<LHPS));
				snd_soc_write(codec, HPOUT_CTRL, reg_val);
				//turn off speaker's external pa.
				//gpio_set_value(item.gpio.gpio, 0);
				msleep(62);
				/*disable speaker*/
				reg_val = snd_soc_read(codec, SPKOUT_CTRL);
				reg_val &= ~((0x1<<RSPK_EN)|(0x1<<LSPK_EN));
				snd_soc_write(codec, SPKOUT_CTRL, reg_val);
				break;
			case 1:
				/*set spk volume to 0*/
				reg_val = snd_soc_read(codec, SPKOUT_CTRL);
				reg_val &= ~(0x1f<<SPK_VOL);
				snd_soc_write(codec, SPKOUT_CTRL, reg_val);

				/*disable pa_ctrl, turn off speaker's external pa.*/
				//gpio_set_value(item.gpio.gpio, 0);
				msleep(62);
				/*disable speaker*/
				reg_val = snd_soc_read(codec, SPKOUT_CTRL);
				reg_val &= ~((0x1<<RSPK_EN)|(0x1<<LSPK_EN));
				snd_soc_write(codec, SPKOUT_CTRL, reg_val);

				//select spk source and  volume
				reg_val = snd_soc_read(codec, SPKOUT_CTRL);
				reg_val &= ~((0x1<<RSPKS)|(0x1<<LSPKS)|(0x1f<<SPK_VOL));
				snd_soc_write(codec, SPKOUT_CTRL, reg_val);
				break;
			case 2:
				/*set headphone volume to 0*/
				reg_val = snd_soc_read(codec, HPOUT_CTRL);
				reg_val &= ~(0x3f<<HP_VOL);
				snd_soc_write(codec, HPOUT_CTRL, reg_val);

				/*disable pa*/
				reg_val = snd_soc_read(codec, HPOUT_CTRL);
				reg_val &= ~(0x1<<HPPA_EN);
				snd_soc_write(codec, HPOUT_CTRL, reg_val);

				/*hardware xzh support*/
				reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
				reg_val &= ~(0xf<<HPOUTPUTENABLE);
				snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

				/*unmute l/r headphone pa*/
				reg_val = snd_soc_read(codec, HPOUT_CTRL);
				reg_val &= ~((0x1<<RHPPA_MUTE)|(0x1<<LHPPA_MUTE));
				snd_soc_write(codec, HPOUT_CTRL, reg_val);

				/*set spk volume to 0*/
				reg_val = snd_soc_read(codec, SPKOUT_CTRL);
				reg_val &= ~(0x1f<<SPK_VOL);
				snd_soc_write(codec, SPKOUT_CTRL, reg_val);

				/*disable pa_ctrl, turn off speaker's external pa.*/
				//gpio_set_value(item.gpio.gpio, 0);
				msleep(62);
				/*disable speaker*/
				reg_val = snd_soc_read(codec, SPKOUT_CTRL);
				reg_val &= ~((0x1<<RSPK_EN)|(0x1<<LSPK_EN));
				snd_soc_write(codec, SPKOUT_CTRL, reg_val);

				//select spk source and  volume
				reg_val = snd_soc_read(codec, SPKOUT_CTRL);
				reg_val &= ~((0x1<<RSPKS)|(0x1<<LSPKS)|(0x1f<<SPK_VOL));
				snd_soc_write(codec, SPKOUT_CTRL, reg_val);
				break;
			case  3:
				/*set earpiece volume:0*/
				reg_val = snd_soc_read(codec, ESPKOUT_CTRL);
				reg_val &= ~(0x1f<<ESP_VOL);
				snd_soc_write(codec, ESPKOUT_CTRL, reg_val);

				/*disable earpiece power amplifier*/
				reg_val = snd_soc_read(codec, ESPKOUT_CTRL);
				reg_val &= ~(0x1<<ESPPA_EN);
				snd_soc_write(codec, ESPKOUT_CTRL, reg_val);

				/*mute earpiece line*/
				reg_val = snd_soc_read(codec, ESPKOUT_CTRL);
				reg_val &= ~(0x1<<ESPPA_MUTE);
				snd_soc_write(codec, ESPKOUT_CTRL, reg_val);

				/*earpiece input source select*/
				reg_val = snd_soc_read(codec, ESPKOUT_CTRL);
				reg_val &= ~(0x3<<ESPSR);
				snd_soc_write(codec, ESPKOUT_CTRL, reg_val);
				break;
			default:
				break;
			}
			/*disable L/R analog mixer output*/
			reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
			reg_val &= ~(0x1<<RMIXEN);
			reg_val &= ~(0x1<<LMIXEN);
			snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

			reg_val = snd_soc_read(codec, OMIXER_SR);
			reg_val &= ~(0x3fff<<LMIXMUTEDACR);
			snd_soc_write(codec, OMIXER_SR, reg_val);

			/*disable analog DAC*/
			reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
			reg_val &= ~((0x1<<DACALEN)|(0X1<<DACAREN));
			snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

			reg_val = snd_soc_read(codec, DAC_DIG_CTRL);
			reg_val &= ~(0x1<<ENDA);
			snd_soc_write(codec, DAC_DIG_CTRL, reg_val);

			reg_val= snd_soc_read(codec, AIF1_DACDAT_CTRL);
			reg_val &= ~((0x1<<AIF1_DA0L_ENA)|(0x1<<AIF1_DA0R_ENA));
			snd_soc_write(codec, AIF1_DACDAT_CTRL,reg_val);

			/*diable clk parts*/
			if (1 == cap_running ) {
				printk("capturing is running!!!!\n");
			} else {
				/*disable module AIF1,DAC*/
				reg_val = snd_soc_read(codec, MOD_CLK_ENA);
				reg_val &= ~(0x1<<MOD_CLK_AIF1);
				snd_soc_write(codec, MOD_CLK_ENA, reg_val);

				/*rereset module AIF1, DAC*/
				reg_val = snd_soc_read(codec, MOD_RST_CTRL);
				reg_val &= ~(0x1<<MOD_RESET_AIF1);
				snd_soc_write(codec, MOD_RST_CTRL, reg_val);
				/*disable aif1clk,sysclk*/
				reg_val = snd_soc_read(codec, SYSCLK_CTRL);
				reg_val &= ~((0x1<<AIF1CLK_ENA)|(0x1<<SYSCLK_ENA));
				snd_soc_write(codec, SYSCLK_CTRL, reg_val);
			}

			/*disable module DAC*/
			reg_val = snd_soc_read(codec, MOD_CLK_ENA);
			reg_val &= ~(0x1<<MOD_CLK_DAC_DIG);
			snd_soc_write(codec, MOD_CLK_ENA, reg_val);

			/*rereset module AIF1, DAC*/
			reg_val = snd_soc_read(codec, MOD_RST_CTRL);
			reg_val &= ~(0x1<<MOD_RESET_DAC_DIG);
			snd_soc_write(codec, MOD_RST_CTRL, reg_val);

			/*close phone out*/
			reg_val = snd_soc_read(codec, LOUT_CTRL);
			reg_val &= ~(0xf<<LINEOUTS3);
			snd_soc_write(codec, LOUT_CTRL, reg_val);
		}
		play_running = 0;
	} else {
		if ( !(codec_speakerout_en || codec_headphoneout_en || codec_earpieceout_en ||
					codec_lineinin_en || codec_voice_record_en || codec_bt_clk_format) )
		{
			/*disable  mic1/2 boost control*/
			reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
			reg_val &= ~((0x1<<MIC1AMPEN)|(0x1<<MIC2AMPEN));
			snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

			/*select  adc mixer source*/
			reg_val = snd_soc_read(codec, ADC_SRC);
			reg_val &= ~((0x1<<LADCMIXMUTEMIC1BOOST)|(0x1<<RADCMIXMUTEMIC1BOOST));
			snd_soc_write(codec, ADC_SRC, reg_val);

			reg_val = snd_soc_read(codec, ADC_SRC);
			reg_val |= (0x1<<LADCMIXMUTEMIC2BOOST)|(0x1<<RADCMIXMUTEMIC2BOOST);
			snd_soc_write(codec, ADC_SRC, reg_val);

			reg_val = snd_soc_read(codec, ADC_SRC);
			reg_val &= ~((0x1<<LADCMIXMUTEAUXINL)|(0x1<<RADCMIXMUTEAUXINR));
			snd_soc_write(codec, ADC_SRC, reg_val);

			/*ADC L/R channel Enable*/
			reg_val = snd_soc_read(codec, ADC_APC_CTRL);
			reg_val &= ~((0x1<<ADCREN)|(0x1<<ADCLEN));
			snd_soc_write(codec, ADC_APC_CTRL, reg_val);

			/*disable mic1 BIAS*/
			reg_val = snd_soc_read(codec, ADC_APC_CTRL);
			reg_val &= ~(0x1<<MBIASEN);
			snd_soc_write(codec, ADC_APC_CTRL, reg_val);

			/*enable ADC Digital part & enable analog ADC mode */
			reg_val = snd_soc_read(codec, ADC_DIG_CTRL);
			reg_val &= ~((0x1<<ENAD)|(0x1<<ENDM));
			snd_soc_write(codec, ADC_DIG_CTRL, reg_val);

			if (play_running == 1 ) {
				printk("play is running!!!!\n");
			} else {
				/*disable module AIF1*/
				reg_val = snd_soc_read(codec, MOD_CLK_ENA);
				reg_val &= ~(0x1<<MOD_CLK_AIF1);
				snd_soc_write(codec, MOD_CLK_ENA, reg_val);

				/*rereset module AIF1*/
				reg_val = snd_soc_read(codec, MOD_RST_CTRL);
				reg_val &= ~(0x1<<MOD_RESET_AIF1);
				snd_soc_write(codec, MOD_RST_CTRL, reg_val);
				/*disable Aif1clk, SYSCLK*/
				reg_val = snd_soc_read(codec, SYSCLK_CTRL);
				reg_val &= ~((0x1<<AIF1CLK_ENA)|(0x1<<SYSCLK_ENA));
				snd_soc_write(codec, SYSCLK_CTRL, reg_val);
			}

			/*disable module ADC*/
			reg_val = snd_soc_read(codec, MOD_CLK_ENA);
			reg_val &= ~(0x1<<MOD_CLK_ADC_DIG);
			snd_soc_write(codec, MOD_CLK_ENA, reg_val);

			/*rereset module ADC*/
			reg_val = snd_soc_read(codec, MOD_RST_CTRL);
			reg_val &= ~(0x1<<MOD_RESET_ADC_DIG);
			snd_soc_write(codec, MOD_RST_CTRL, reg_val);
		}

		/* select AIF1 output  mixer source*/
		reg_val = snd_soc_read(codec, AIF1_MXR_SRC);
		reg_val &= ~((0x1<<AIF1_AD0R_ADCR_MXR)|(0x1<<AIF1_AD0L_ADCL_MXR));
		snd_soc_write(codec, AIF1_MXR_SRC,reg_val);

		/*close ADC channel slot0 switch*/
		reg_val = snd_soc_read(codec, AIF1_ADCDAT_CTRL);
		reg_val &= ~(0x3<<AIF1_AD0R_ENA);
		snd_soc_write(codec, AIF1_ADCDAT_CTRL,reg_val);

		cap_running = 0;
	}
}

static int codec_capture_open(struct snd_soc_codec *codec)
{
	int reg_val;
	printk("enter %s\n",__func__);
	if (codec_headsetmic_voicerecord) {
		/*select  mic1/2 boost control*/
		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val &= ~((0x1<<MIC1AMPEN)|(0x1<<MIC2AMPEN));
		reg_val |= (0x0<<MIC1AMPEN)|(0x1<<MIC2AMPEN);
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);
		/* setup mic2 boost amplifier Gain control*/
		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val |= (0x4<<ADC_MIC2G);
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);
		/*select  adc mixer source*/
		reg_val = snd_soc_read(codec, ADC_SRC);
		reg_val |= (0x1<<LADCMIXMUTEMIC2BOOST)|(0x1<<RADCMIXMUTEMIC2BOOST);
		snd_soc_write(codec, ADC_SRC, reg_val);

		/*ADC L/R channel enable*/
		reg_val = snd_soc_read(codec, ADC_APC_CTRL);
		reg_val &= ~((0x1<<ADCREN)|(0x1<<ADCLEN));
		reg_val |= (0x1<<ADCREN)|(0x1<<ADCLEN);
		snd_soc_write(codec, ADC_APC_CTRL, reg_val);

		/*Headset microphone BIAS enable*/
		reg_val = snd_soc_read(codec, ADC_APC_CTRL);
		reg_val &= ~(0x1<<HBIASEN);
		reg_val |= (0x1<<HBIASEN);
		snd_soc_write(codec, ADC_APC_CTRL, reg_val);
	} else {
		/*select  mic1/2 boost control*/
		reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		reg_val &= ~((0x1<<MIC1AMPEN)|(0x1<<MIC2AMPEN));
		reg_val |= (0x1<<MIC1AMPEN)|(0x0<<MIC2AMPEN);
		snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

		/* setup mic1 boost amplifier Gain control*/
		 reg_val = snd_soc_read(codec, ADC_SRCBST_CTRL);
		 reg_val |= (0x6<<ADC_MIC1G);
		 snd_soc_write(codec, ADC_SRCBST_CTRL, reg_val);

		/*select  adc mixer source*/
		reg_val = snd_soc_read(codec, ADC_SRC);
		reg_val |= (0x1<<LADCMIXMUTEMIC1BOOST)|(0x1<<RADCMIXMUTEMIC1BOOST);
		snd_soc_write(codec, ADC_SRC, reg_val);

		/*ADC L/R channel enable*/
		reg_val = snd_soc_read(codec, ADC_APC_CTRL);
		reg_val &= ~((0x1<<ADCREN)|(0x1<<ADCLEN));
		reg_val |= (0x1<<ADCREN)|(0x1<<ADCLEN);
		snd_soc_write(codec, ADC_APC_CTRL, reg_val);

		/* mic1 BIAS enable*/
		reg_val = snd_soc_read(codec, ADC_APC_CTRL);
		reg_val &= ~(0x1<<MBIASEN);
		reg_val |= (0x1<<MBIASEN);
		snd_soc_write(codec, ADC_APC_CTRL, reg_val);
	}

	/*enable ADC Digital part & enable analog ADC mode */
	reg_val = snd_soc_read(codec, ADC_DIG_CTRL);
	reg_val &= ~((0x1<<ENAD)|(0x1<<ENDM));
	reg_val |= (0x1<<ENAD)|(0x0<<ENDM);
	snd_soc_write(codec, ADC_DIG_CTRL, reg_val);

	/* select AIF1 output  mixer source*/
	reg_val = snd_soc_read(codec, AIF1_MXR_SRC);
	reg_val |= (0x1<<AIF1_AD0R_ADCR_MXR)|(0x1<<AIF1_AD0L_ADCL_MXR);
	snd_soc_write(codec, AIF1_MXR_SRC,reg_val);

	/*open ADC channel slot0 switch*/
	reg_val = snd_soc_read(codec, AIF1_ADCDAT_CTRL);
	reg_val &= ~(0x3<<AIF1_AD0R_ENA);
	reg_val |= (0x1<<AIF1_AD0L_ENA)|(0x1<<AIF1_AD0R_ENA);
	snd_soc_write(codec, AIF1_ADCDAT_CTRL,reg_val);
	msleep(200);
	return 0;
}

/*
*	use for the line_in record
*/
static int codec_voice_linein_capture_open(struct snd_soc_codec *codec)
{
	int reg_val;

	/*select R/L ADC_MIXMUTE*/
	printk("enter %s\n",__func__);
	reg_val = snd_soc_read(codec, ADC_SRC);
	reg_val |= (0x1<<LADCMIXMUTEAUXINL)|(0x1<<RADCMIXMUTEAUXINR);
	snd_soc_write(codec, ADC_SRC, reg_val);

	/*ADC L/R channel Enable*/
	reg_val = snd_soc_read(codec, ADC_APC_CTRL);
	reg_val &= ~((0x1<<ADCREN)|(0x1<<ADCLEN));
	reg_val |= (0x1<<ADCREN)|(0x1<<ADCLEN);
	snd_soc_write(codec, ADC_APC_CTRL, reg_val);

	/*enable ADC Digital part & ENBLE Analog ADC mode */
	reg_val = snd_soc_read(codec, ADC_DIG_CTRL);
	reg_val &= ~((0x1<<ENAD)|(0x1<<ENDM));
	reg_val |= (0x1<<ENAD)|(0x0<<ENDM);
	snd_soc_write(codec, ADC_DIG_CTRL, reg_val);

	/* select AIF1_AD0L/R_MXR_SRC*/
	reg_val = snd_soc_read(codec, AIF1_MXR_SRC);
	reg_val |= (0x1<<AIF1_AD0R_ADCR_MXR)|(0x1<<AIF1_AD0L_ADCL_MXR);
	snd_soc_write(codec, AIF1_MXR_SRC,reg_val);

	/*open ADC channel slot0 switch*/
	reg_val = snd_soc_read(codec, AIF1_ADCDAT_CTRL);
	reg_val &= ~(0x3<<AIF1_AD0R_ENA);
	reg_val |= (0x1<<AIF1_AD0L_ENA)|(0x1<<AIF1_AD0R_ENA);
	snd_soc_write(codec, AIF1_ADCDAT_CTRL,reg_val);
	return 0;
}

static int sndvir_audio_prepare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *codec_dai)
{
	int  play_ret;
	struct snd_soc_codec *codec = codec_dai->codec;

	printk("enter %s:codec_speaker_headset_earpiece_en=%d\n",__func__,codec_speaker_headset_earpiece_en);
	printk("enter %s:stream=%d\n",__func__,substream->stream);
	printk("enter %s:state=%d\n",__func__,substream->runtime->status->state);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if(substream->runtime->status->state == SNDRV_PCM_STATE_XRUN){
			if (codec_speaker_headset_earpiece_en == 1) {
				play_ret = codec_spk_play_open(codec);
			} else if (codec_speaker_headset_earpiece_en == 2) {
				play_ret = codec_spk_headset_play_open(codec);
			} else if (codec_speaker_headset_earpiece_en == 3) {
				play_ret = codec_earpiece_play_open(codec);
			} else if (codec_speaker_headset_earpiece_en == 0) {
				play_ret = codec_headphone_play_open(codec);
			}
			play_running = 1;
		}

	} else {
		if (codec_voice_record_en && (codec_digital_mainmic_en ||codec_digital_headsetmic_en)) {
			codec_digital_voice_mic_bb_capture_open(codec);
		} else if (codec_voice_record_en && codec_digital_btmic_en) {
			codec_digital_voice_bb_bt_capture_open(codec);
		} else if (codec_voice_record_en && codec_analog_btmic_en) {
			codec_analog_voice_bb_bt_capture_open(codec);
		} else if (codec_voice_record_en && codec_analog_mainmic_en) {
			codec_analog_voice_main_mic_capture_open(codec);
		} else if (codec_voice_record_en && codec_analog_headsetmic_en) {
			codec_analog_voice_headset_mic_capture_open(codec);
		} else if (codec_lineinin_en && codec_lineincap_en) {
			codec_voice_linein_capture_open(codec);
		} else if (!codec_voice_record_en) {
			codec_capture_open(codec);
		}
	}

	return 0;
}

static int sndvir_audio_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *codec_dai)
{
	int reg_val;
	struct snd_soc_codec *codec = codec_dai->codec;
	int samplerate;

	samplerate = params_rate(params);

      printk("enter %s:rate=%d\n",__func__,samplerate);

	switch (samplerate) {
		case 44100:
			reg_val = snd_soc_read(codec, AIF_SR_CTRL);
			reg_val &= ~(0xf<<AIF1_FS);
			reg_val |= (0x7<<AIF1_FS);
			snd_soc_write(codec, AIF_SR_CTRL, reg_val);
			break;
		case 8000:
			reg_val = snd_soc_read(codec, AIF_SR_CTRL);
			reg_val &= ~(0xf<<AIF1_FS);
			reg_val |= (0x0<<AIF1_FS);
			snd_soc_write(codec, AIF_SR_CTRL, reg_val);
			break;
		case 48000:
			reg_val = snd_soc_read(codec, AIF_SR_CTRL);
			reg_val &= ~(0xf<<AIF1_FS);
			reg_val |= (0x8<<AIF1_FS);
			snd_soc_write(codec, AIF_SR_CTRL, reg_val);
			break;
		case 11025:
			reg_val = snd_soc_read(codec, AIF_SR_CTRL);
			reg_val &= ~(0xf<<AIF1_FS);
			reg_val |= (0x1<<AIF1_FS);
			snd_soc_write(codec, AIF_SR_CTRL, reg_val);
			break;
		case 12000:
			reg_val = snd_soc_read(codec, AIF_SR_CTRL);
			reg_val &= ~(0xf<<AIF1_FS);
			reg_val |= (0x2<<AIF1_FS);
			snd_soc_write(codec, AIF_SR_CTRL, reg_val);
			break;
		case 16000:
			reg_val = snd_soc_read(codec, AIF_SR_CTRL);
			reg_val &= ~(0xf<<AIF1_FS);
			reg_val |= (0x3<<AIF1_FS);
			snd_soc_write(codec, AIF_SR_CTRL, reg_val);
			break;
		case 22050:
			reg_val = snd_soc_read(codec, AIF_SR_CTRL);
			reg_val &= ~(0xf<<AIF1_FS);
			reg_val |= (0x4<<AIF1_FS);
			snd_soc_write(codec, AIF_SR_CTRL, reg_val);
			break;
		case 24000:
			reg_val = snd_soc_read(codec, AIF_SR_CTRL);
			reg_val &= ~(0xf<<AIF1_FS);
			reg_val |= (0x5<<AIF1_FS);
			snd_soc_write(codec, AIF_SR_CTRL, reg_val);
			break;
		case 32000:
			reg_val = snd_soc_read(codec, AIF_SR_CTRL);
			reg_val &= ~(0xf<<AIF1_FS);
			reg_val |= (0x6<<AIF1_FS);
			snd_soc_write(codec, AIF_SR_CTRL, reg_val);
			break;
		case 96000:
			reg_val = snd_soc_read(codec, AIF_SR_CTRL);
			reg_val &= ~(0xf<<AIF1_FS);
			reg_val |= (0x9<<AIF1_FS);
			snd_soc_write(codec, AIF_SR_CTRL, reg_val);
			break;
		case 192000:
			reg_val = snd_soc_read(codec, AIF_SR_CTRL);
			reg_val &= ~(0xf<<AIF1_FS);
			reg_val |= (0xa<<AIF1_FS);
			snd_soc_write(codec, AIF_SR_CTRL, reg_val);
		break;
		default:
			break;
	}

	return 0;
}


//according to different MCLK1, MCLK2, BCLK1, BCLK2 input clock, use pll for config Aif1 clock
static int sndvir_audio_set_dai_pll(struct snd_soc_dai *codec_dai, int pll_id, int source, 
									unsigned int freq_in, unsigned int freq_out)
{
	int reg_val;
	int m, l_decimal,r_decimal;
	struct snd_soc_codec *codec = codec_dai->codec;

	printk("%s:source=%d,f_in=%d,f_out=%d\n",__func__,source,freq_in,freq_out);
	
	if (!freq_out)
		return 0;
	if ((freq_in < 128000) || (freq_in > 24576000)){
		return -EINVAL;
	} else if ((freq_in == 24576000) || (freq_in == 22579200)) {
		/*aif1 clk source from MCLK1*/
		reg_val = snd_soc_read(codec, SYSCLK_CTRL);
		reg_val &= ~(0x3<<AIF1CLK_SRC);
		snd_soc_write(codec, SYSCLK_CTRL, reg_val);
		return 0;
	}

	switch (source) {
		case AC100_MCLK1:
			/*pll source from MCLK1*/
			reg_val = snd_soc_read(codec, SYSCLK_CTRL);
			reg_val &= ~(0x3<<PLLCLK_SRC);
			snd_soc_write(codec, SYSCLK_CTRL, reg_val);
			break;
		case AC100_MCLK2:
			/*pll source from MCLK2*/
			reg_val = snd_soc_read(codec, SYSCLK_CTRL);
			reg_val &= ~(0x3<<PLLCLK_SRC);
			reg_val |= (0x1<<PLLCLK_SRC);
			snd_soc_write(codec, SYSCLK_CTRL, reg_val);
			break;
		case AC100_BCLK1:
			/*pll source from BCLK1*/
			reg_val = snd_soc_read(codec, SYSCLK_CTRL);
			reg_val &= ~(0x3<<PLLCLK_SRC);
			reg_val |= (0x2<<PLLCLK_SRC);
			snd_soc_write(codec, SYSCLK_CTRL, reg_val);
			break;
		case AC100_BCLK2:
			/*pll source from BCLK2*/
			reg_val = snd_soc_read(codec, SYSCLK_CTRL);
			reg_val &= ~(0x3<<PLLCLK_SRC);
			reg_val |= (0x3<<PLLCLK_SRC);
			snd_soc_write(codec, SYSCLK_CTRL, reg_val);
			break;
		default:
			return -EINVAL;
	}
	/* freq_out = freq_in * n/(m*(2k+1)) , k=1 */
	switch (freq_out) {
	case 22579200:
		switch (freq_in) {
			case 19200000:
				m = 25;
				l_decimal =88;
				r_decimal =1;
				break;
			case 13000000:
				m = 19;
				l_decimal =99;
				r_decimal =0;
				break;
			case 6000000:
				m = 38;
				l_decimal =429 ;
				r_decimal =0 ;
				break;
			case 384000:
				m = 1;
				l_decimal =176;
				r_decimal =2;
				break;
			case 256000:
				m = 1;
				l_decimal =264;
				r_decimal =3;
				break;
			case 192000:
				m = 1;
				l_decimal =352;
				r_decimal =4;
				break;
			case 128000:
				m = 1;
				l_decimal =529;
				r_decimal =1;
				break;
				
			case 11289600:
				printk("use 22579200 11289600\n");
				m = 1;
				l_decimal =2;
				r_decimal =0;
				break;
			default:
				return -EINVAL;

		}
		break;
	case 24576000:
		switch (freq_in) {
			case 24000000:
				m = 25;
				l_decimal =76;
				r_decimal =4;
				break;
			case 19200000:
				m = 25;
				l_decimal =88;
				r_decimal =1;
				break;
			case 13000000:
				m = 42;
				l_decimal =238;
				r_decimal =1;
				break;
			case 11289600:
				printk("use 24576000 11289600\n");
				m = 49;
				l_decimal =320;
				r_decimal =1;
				break;
			case 12000000:
				m = 25;
				l_decimal =153;
				r_decimal =3;
				break;
			case 6000000:
				m = 25;
				l_decimal =307 ;
				r_decimal =1 ;
				break;
			case 384000:
				m = 1;
				l_decimal =192;
				r_decimal =0;
				break;
			case 256000:
				m = 1;
				l_decimal =288;
				r_decimal =0;
				break;
			case 192000:
				m = 1;
				l_decimal =384;
				r_decimal =0;
				break;
			case 128000:
				m = 1;
				l_decimal =576;
				r_decimal =0;
				break;
			default:
				return -EINVAL;

		}

		break;
	default:
		return -EINVAL;
	}
	/*config pll m*/
	reg_val = snd_soc_read(codec, PLL_CTRL1);
	reg_val &= ~(0x3f<<PLL_POSTDIV_M);
	reg_val |= (m<<PLL_POSTDIV_M);
	snd_soc_write(codec, PLL_CTRL1, reg_val);

	/*config pll n*/
	reg_val = snd_soc_read(codec, PLL_CTRL2);
	reg_val &= ~(0x3ff<<PLL_PREDIV_NI);
	reg_val |= (l_decimal<<PLL_PREDIV_NI);
	reg_val &= ~(0x7<<PLL_POSTDIV_NF);
	reg_val |= (r_decimal<<PLL_POSTDIV_NF);
	reg_val |= (1<<PLL_EN);
	snd_soc_write(codec, PLL_CTRL2, reg_val);

	/*enable pll_enable and select AIF1CLK source from pll*/
	reg_val = snd_soc_read(codec, SYSCLK_CTRL);
	reg_val |= (0x1<<PLLCLK_ENA)|(0x2<<AIF1CLK_SRC);
	snd_soc_write(codec, SYSCLK_CTRL, reg_val);

	return 0;
}

static int sndvir_audio_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	printk("%s:clk_id=%d,freq=%d,dir=%d\n",__func__,clk_id,freq,dir);

	//sndvir_audio_set_dai_pll(codec_dai,0,AC100_MCLK1,freq,22579200);
	//sndvir_audio_set_dai_clkdiv(codec_dai,0,44100);
	
	return 0;
}

/*
*	for codec chip
*
*	SYSCLK=MCLK=pll2=24.576M/22.5792M

*	LRCK_freq = samplerate
*	BCLK_freq = sample_rate*(LRCK_DIV)
*	word_select_size = 32
*	LRCK_DIV = 2*word_select_size

*/
static int sndvir_audio_set_dai_clkdiv(struct snd_soc_dai *codec_dai, int div_id, int sample_rate)
{
	int reg_val = 0;
	u32 aif_bclk_div = 0;
	u32 bclk_lrck_div = 0;
	struct snd_soc_codec *codec = codec_dai->codec;
	int aif1_word_size = 16;
	bclk_lrck_div = aif1_word_size *2;
	printk("%s:div_id=%d,rate=%d\n",__func__,div_id,sample_rate);
	/*calculate bclk_lrck_div Ratio*/
	switch(bclk_lrck_div)
	{
		case 16: bclk_lrck_div = 0;
				 break;
		case 32: bclk_lrck_div = 1;
				 break;
		case 64: bclk_lrck_div = 2;
				 break;
		case 128: bclk_lrck_div = 3;
				 break;
		case 256: bclk_lrck_div = 4;
				 break;
		default:
				 break;
	}

	/*calculate word select bit*/
	switch (aif1_word_size)
	{
		case 16: aif1_word_size = 0x1;
				break;
		case 8: aif1_word_size = 0x0;
				break;
		case 20: aif1_word_size = 0x2;
				break;
		case 24: aif1_word_size = 0x3;
				break;
		default:
				break;
	}
	switch (sample_rate)
	{
		case 44100:
			/*aif1/bclk = 8*/
			aif_bclk_div =	0x4;
			break;
		case 8000:
			/*aif1/bclk = 48*/
			aif_bclk_div =0x9;
			break;
		case 48000:
			/*aif1/bclk = 8*/
			aif_bclk_div =0x4;
			break;
		case 11025:
			/*aif1/bclk = 32*/
			aif_bclk_div =0x8;
			break;
		case 12000:
			/*aif1/bclk = 32*/
			aif_bclk_div =0x8;
			break;
		case 16000:
			/*aif1/bclk = 24*/
			aif_bclk_div =0x7;
			break;
		case 22050:
			/*aif1/bclk = 16*/
			aif_bclk_div =0x6;
			break;
		case 24000:
			/*aif1/bclk = 16*/
			aif_bclk_div =0x6;
			break;
		case 32000:
			/*aif1/bclk = 12*/
			aif_bclk_div =0x5;
			break;
		case 96000:
			/*aif1/bclk = 4*/
			aif_bclk_div =0x2;
			break;
		case 192000:
			/*aif1/bclk = 2*/
			aif_bclk_div =0x1;
			break;
		default:
			break;

	}
	reg_val = snd_soc_read(codec, AIF1_CLK_CTRL);
	reg_val &= ~(0xf<<AIF1_BCLK_DIV);
	reg_val &= ~(0x7<<AIF1_LRCK_DIV);
	reg_val &= ~(0x3<<AIF1_WORK_SIZ);
	reg_val |= ((aif_bclk_div<<AIF1_BCLK_DIV)|(bclk_lrck_div<<AIF1_LRCK_DIV)|(aif1_word_size<<AIF1_WORK_SIZ));
	snd_soc_write(codec, AIF1_CLK_CTRL, reg_val);
	return 0;
}

static int sndvir_audio_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	int reg_val;
	struct snd_soc_codec *codec = codec_dai->codec;

       printk("enter %s\n",__func__);
	/*
	* 	master or slave selection
	*	0 = Master mode
	*	1 = Slave mode
	*/
	reg_val = snd_soc_read(codec, AIF1_CLK_CTRL);
	printk("enter %s:reg=%d\n",__func__,reg_val);
	switch(fmt & SND_SOC_DAIFMT_MASTER_MASK){
		case SND_SOC_DAIFMT_CBM_CFM:   /* codec clk & frm master, ap is slave*/
			reg_val &=~(0x1<<AIF1_MSTR_MOD);
			reg_val |= (0x0<<AIF1_MSTR_MOD);
			printk("codec clk & frm master, ap is slave\n");
			break;
		case SND_SOC_DAIFMT_CBS_CFS:   /* codec clk & frm slave,ap is master*/
			reg_val &=~(0x1<<AIF1_MSTR_MOD);
			reg_val |= (0x1<<AIF1_MSTR_MOD);
			printk("codec clk & frm slave,ap is master\n");
			break;
		default:
			printk("unknwon master/slave format\n");
			return -EINVAL;
	}
	snd_soc_write(codec, AIF1_CLK_CTRL, reg_val);

	/* i2s mode selection */
	reg_val = snd_soc_read(codec, AIF1_CLK_CTRL);
	reg_val&=~(3<<AIF1_DATA_FMT);
	switch(fmt & SND_SOC_DAIFMT_FORMAT_MASK){
		case SND_SOC_DAIFMT_I2S:        /* I2S1 mode */
			reg_val |= (0x0<<AIF1_DATA_FMT);
			break;
		case SND_SOC_DAIFMT_RIGHT_J:    /* Right Justified mode */
			reg_val |= (0x2<<AIF1_DATA_FMT);
			break;
		case SND_SOC_DAIFMT_LEFT_J:     /* Left Justified mode */
			reg_val |= (0x1<<AIF1_DATA_FMT);
			break;
		case SND_SOC_DAIFMT_DSP_A:      /* L reg_val msb after FRM LRC */
			reg_val |= (0x3<<AIF1_DATA_FMT);
			break;
		default:
			printk("%s, line:%d\n", __func__, __LINE__);
			return -EINVAL;
	}
	snd_soc_write(codec, AIF1_CLK_CTRL, reg_val);

	/* DAI signal inversions */
	reg_val = snd_soc_read(codec, AIF1_CLK_CTRL);
	switch(fmt & SND_SOC_DAIFMT_INV_MASK){
		case SND_SOC_DAIFMT_NB_NF:     /* normal bit clock + nor frame */
			printk("enter %s nn\n",__func__);
			reg_val &= ~(0x1<<AIF1_LRCK_INV);
			reg_val &= ~(0x1<<AIF1_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_NB_IF:     /* normal bclk + inv frm */
			printk("enter %s ni\n",__func__);
			reg_val |= (0x1<<AIF1_LRCK_INV);
			reg_val &= ~(0x1<<AIF1_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_IB_NF:     /* invert bclk + nor frm */
			printk("enter %s in\n",__func__);
			reg_val &= ~(0x1<<AIF1_LRCK_INV);
			reg_val |= (0x1<<AIF1_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_IB_IF:     /* invert bclk + inv frm */
			printk("enter %s ii\n",__func__);
			reg_val |= (0x1<<AIF1_LRCK_INV);
			reg_val |= (0x1<<AIF1_BCLK_INV);
			break;
	}
	printk("enter %s:11reg=%d,v=%d\n",__func__,AIF1_CLK_CTRL,reg_val);
	snd_soc_write(codec, AIF1_CLK_CTRL, reg_val);
	printk("enter %s:22reg=%d,v=%d\n",__func__,AIF1_CLK_CTRL,snd_soc_read(codec, AIF1_CLK_CTRL));

	printk("enter %s:c2reg=%d,v=%d\n",__func__,AIF1_CLK_CTRL,snd_soc_read(codec, 0xc2));
	printk("enter %s:c3reg=%d,v=%d\n",__func__,AIF1_CLK_CTRL,snd_soc_read(codec, 0xc3));

	return 0;
}

static struct snd_soc_dai_ops sndvir_audio_dai_ops = {
	.startup = sndvir_audio_startup,
	.shutdown = sndvir_audio_shutdown,
	.prepare  =	sndvir_audio_prepare,
	.hw_params = sndvir_audio_hw_params,
	.digital_mute = sndvir_audio_umute,
	.set_sysclk = sndvir_audio_set_dai_sysclk,
	.set_clkdiv = sndvir_audio_set_dai_clkdiv,
	.set_fmt = sndvir_audio_set_dai_fmt,
	.set_pll = sndvir_audio_set_dai_pll,
};

static struct snd_soc_dai_driver sndvir_audio_dai = {
	.name = "ac100_audio",
	/* playback capabilities */
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = sndvir_audio_RATES,
		.formats = sndvir_audio_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = sndvir_audio_RATES,
		.formats = sndvir_audio_FORMATS,
	},
	/* pcm operations */
	.ops = &sndvir_audio_dai_ops,
};
EXPORT_SYMBOL(sndvir_audio_dai);

/*1=headphone in slot, else 0*/
static int headphone_state = 0;
/* key define */
#define KEY_HEADSETHOOK         226

static volatile int irq_flag = 0;
static struct snd_soc_codec *codec_switch = NULL;
//static script_item_u item_eint;
static struct workqueue_struct *switch_detect_queue;
static struct workqueue_struct *codec_irq_queue;

enum headphone_mode_u {
	HEADPHONE_IDLE,
	FOUR_HEADPHONE_PLUGIN,
	THREE_HEADPHONE_PLUGIN,
};

/*struct for headset*/
struct headset_data {
	struct switch_dev sdev;
	int state;
	int check_count;
	int reset_flag;

	enum headphone_mode_u mode;
	struct work_struct work;
	struct work_struct clear_codec_irq;
	struct work_struct codec_resume;
	struct work_struct state_work;

	struct semaphore sem;
	struct timer_list timer;

	struct input_dev *key;
	struct snd_soc_codec *codec;
};

static ssize_t switch_gpio_print_state(struct switch_dev *sdev, char *buf)
{
	struct headset_data	*hs_data =
		container_of(sdev, struct headset_data, sdev);
	return sprintf(buf, "%d\n", hs_data->state);
}

static ssize_t print_headset_name(struct switch_dev *sdev, char *buf)
{
	struct headset_data	*hs_data =
		container_of(sdev, struct headset_data, sdev);
	return sprintf(buf, "%s\n", hs_data->sdev.name);
}


/*
**switch_status_update: update the switch state.
*/
static void switch_status_update(struct headset_data *para)
{
	struct headset_data *hs_data = para;
	printk("%s,line:%d,hs_data->state:%d\n",__func__, __LINE__, hs_data->state);
	down(&hs_data->sem);
	switch_set_state(&hs_data->sdev, hs_data->state);
	up(&hs_data->sem);
}
/*
**clear_codec_irq_work: clear audiocodec pending and Record the interrupt.
*/
static void clear_codec_irq_work(struct work_struct *work)
{
	int reg_val = 0;
	struct headset_data *hs_data = container_of(work, struct headset_data, clear_codec_irq);
	struct snd_soc_codec *codec = hs_data->codec;
	irq_flag--;
	reg_val = snd_soc_read(codec, HMIC_STS);
	if ((0x1<<4)&reg_val) {
		reset_flag++;
		printk("---///----reset_flag:%d--------//\n",reset_flag);
	}
	reg_val |= (0x1f<<0);
	snd_soc_write(codec, HMIC_STS, reg_val);
	reg_val = snd_soc_read(codec, HMIC_STS);
	if((reg_val&0x1f) != 0){
		reg_val |= (0x1f<<0);
		snd_soc_write(codec, HMIC_STS, reg_val);
	}

	cancel_work_sync(&hs_data->work);

	if (0 == queue_work(switch_detect_queue, &hs_data->work)) {
		printk("[clear_codec_irq_work]add work struct failed!\n");
	}
}

/*
**earphone_switch_work: judge the status of the headphone
*/
static void earphone_switch_work(struct work_struct *work)
{
	int reg_val = 0;
	int tmp  = 0;
	unsigned int temp_value[8];
	struct headset_data *hs_data = container_of(work, struct headset_data, work);
	struct snd_soc_codec *codec = hs_data->codec;
	hs_data->check_count = 0;
	/*read HMIC_DATA */
	tmp = snd_soc_read(codec, HMIC_STS);
	reg_val = tmp;
	tmp = (tmp>>HMIC_DATA);
	tmp &= 0x1f;

	if ((tmp>=0xb) && (hs_data->mode== FOUR_HEADPHONE_PLUGIN)) {
		tmp = snd_soc_read(codec, HMIC_STS);
		tmp = (tmp>>HMIC_DATA);
		tmp &= 0x1f;
		if(tmp>=0x19){
			msleep(150);
			tmp = snd_soc_read(codec, HMIC_STS);
			tmp = (tmp>>HMIC_DATA);
			tmp &= 0x1f;
			if(((tmp<0xb && tmp>=0x1) || tmp>=0x19)&&(reset_flag == 0)){
				input_report_key(hs_data->key, KEY_HEADSETHOOK, 1);
				input_sync(hs_data->key);
				printk("%s,line:%d,KEY_HEADSETHOOK1\n",__func__,__LINE__);
				if(hook_flag1 != hook_flag2){
					hook_flag1 = hook_flag2 = 0;
				}
				hook_flag1++;
			}
			if(reset_flag)
				reset_flag--;
		}else if(tmp<0x19 && tmp>=0x17){
			msleep(80);
			tmp = snd_soc_read(codec, HMIC_STS);
			tmp = (tmp>>HMIC_DATA);
			tmp &= 0x1f;
			if(tmp<0x19 && tmp>=0x17 &&(reset_flag == 0)) {
				KEY_VOLUME_FLAG = 1;
				input_report_key(hs_data->key, KEY_VOLUMEUP, 1);
				input_sync(hs_data->key);
				input_report_key(hs_data->key, KEY_VOLUMEUP, 0);
				input_sync(hs_data->key);
				printk("%s,line:%d,tmp:%d,KEY_VOLUMEUP\n",__func__,__LINE__,tmp);
			}
			if(reset_flag)
				reset_flag--;
		}else if(tmp<0x17 && tmp>=0x13){
			msleep(80);
			tmp = snd_soc_read(codec, HMIC_STS);
			tmp = (tmp>>HMIC_DATA);
			tmp &= 0x1f;
			if(tmp<0x17 && tmp>=0x13  && (reset_flag == 0)){
				KEY_VOLUME_FLAG = 1;
				input_report_key(hs_data->key, KEY_VOLUMEDOWN, 1);
				input_sync(hs_data->key);
				input_report_key(hs_data->key, KEY_VOLUMEDOWN, 0);
				input_sync(hs_data->key);
				printk("%s,line:%d,KEY_VOLUMEDOWN\n",__func__,__LINE__);
			}
			if(reset_flag)
				reset_flag--;
		}
	} else if ((tmp<0xb && tmp>=0x2) && (hs_data->mode== FOUR_HEADPHONE_PLUGIN)) {
		/*read HMIC_DATA */
		tmp = snd_soc_read(codec, HMIC_STS);
		tmp = (tmp>>HMIC_DATA);
		tmp &= 0x1f;
		if (tmp<0xb && tmp>=0x2) {
			if(KEY_VOLUME_FLAG) {
				KEY_VOLUME_FLAG = 0;
			}
			if(hook_flag1 == (++hook_flag2)) {
				hook_flag1 = hook_flag2 = 0;
				input_report_key(hs_data->key, KEY_HEADSETHOOK, 0);
				input_sync(hs_data->key);
				printk("%s,line:%d,KEY_HEADSETHOOK0\n",__func__,__LINE__);
			}
		}
	} else {
		while (irq_flag == 0) {
			msleep(40);
			/*read HMIC_DATA */
			tmp = snd_soc_read(codec, HMIC_STS);
			tmp = (tmp>>HMIC_DATA);
			tmp &= 0x1f;
			if (hs_data->check_count <= HEADSET_CHECKCOUNT){
				temp_value[hs_data->check_count] = tmp;
				hs_data->check_count++;
				if(hs_data->check_count >= 2){
					if( !(temp_value[hs_data->check_count - 1] == temp_value[(hs_data->check_count) - 2])){
						hs_data->check_count = 0;
					}
				}

			}else{
				if (temp_value[hs_data->check_count -2] >= 0xb) {

					hs_data->state		= 2;
					hs_data->mode = THREE_HEADPHONE_PLUGIN;
					switch_status_update(hs_data);
					hs_data->check_count = 0;
					reset_flag = 0;
					break;
				} else if(temp_value[hs_data->check_count - 2]>=0x1 && temp_value[hs_data->check_count -2]<0xb) {
					hs_data->mode = FOUR_HEADPHONE_PLUGIN;
					hs_data->state		= 1;
					switch_status_update(hs_data);
					hs_data->check_count = 0;
					reset_flag = 0;
					break;

				} else {
					hs_data->mode = HEADPHONE_IDLE;
					hs_data->state = 0;
					switch_status_update(hs_data);
					hs_data->check_count = 0;
					reset_flag = 0;
					break;
				}
			}
		}
	}

}
/*
**audio_hmic_irq:  the interrupt handlers
*/
static irqreturn_t audio_hmic_irq(int irq, void *para)
{
	struct headset_data *hs_data = (struct headset_data *)para;
	if (hs_data == NULL) {
		return -EINVAL;
	}

	irq_flag = irq_flag+1;
	/**
	*here, you should clear audio-irq(gpio) pending bit
	*
	*/
	/**************example**************************************/
	#if 0
	/*clear audio-irq pending bit*/
	writel((0x1<<7), ((void __iomem *)0xf8002c00+0x214));
	#endif
	/*************example***************************************/
	if(codec_irq_queue == NULL)
		printk("------------codec_irq_queue is null!!----------");
	if(&hs_data->clear_codec_irq == NULL)
		printk("------------hs_data->clear_codec_irq is null!!----------");

	if(0 == queue_work(codec_irq_queue, &hs_data->clear_codec_irq)){
		irq_flag--;
		printk("[audio_hmic_irq]add work struct failed!\n");
	}
	return 0;
}

/*
**switch_hw_config:config the AC100 register for headphone detection.
*/
static void switch_hw_config(struct snd_soc_codec *codec)
{
	int reg_val;
	reg_val = snd_soc_read(codec, OMIXER_BST1_CTRL);
	reg_val |= (0xf<<12);
	snd_soc_write(codec, OMIXER_BST1_CTRL, reg_val);
	
	/*debounce when Key down or keyup*/
	reg_val = snd_soc_read(codec, HMIC_CTRL1);
	reg_val &= (0xf<<HMIC_M);
	reg_val |= (0x0<<HMIC_M);
	snd_soc_write(codec, HMIC_CTRL1, reg_val);

	/*debounce when earphone plugin or pullout*/
	reg_val = snd_soc_read(codec, HMIC_CTRL1);
	reg_val &= (0xf<<HMIC_N);
	reg_val |= (0x0<<HMIC_N);
	snd_soc_write(codec, HMIC_CTRL1, reg_val);

	/*Down Sample Setting Select/11:Downby 8,16Hz*/
	reg_val = snd_soc_read(codec, HMIC_CTRL2);
	reg_val &= ~(0x3<<HMIC_SAMPLE_SELECT);
	snd_soc_write(codec, HMIC_CTRL2, reg_val);

	/*Hmic_th2 for detecting Keydown or Keyup.*/
	reg_val = snd_soc_read(codec, HMIC_CTRL2);
	reg_val &= ~(0x1f<<HMIC_TH2);
	reg_val |= (0x8<<HMIC_TH2);
	snd_soc_write(codec, HMIC_CTRL2, reg_val);

	/*Hmic_th1[4:0],detecting eraphone plugin or pullout*/
	reg_val = snd_soc_read(codec, HMIC_CTRL2);
	reg_val &= ~(0x1f<<HMIC_TH1);
	reg_val |= (0x1<<HMIC_TH1);
	snd_soc_write(codec, HMIC_CTRL2, reg_val);

	/*Headset microphone BIAS Enable*/
	reg_val = snd_soc_read(codec, ADC_APC_CTRL);
	reg_val |= (0x1<<HBIASEN);
	snd_soc_write(codec, ADC_APC_CTRL, reg_val);

	/*Headset microphone BIAS Current sensor & ADC Enable*/
	reg_val = snd_soc_read(codec, ADC_APC_CTRL);
	reg_val |= (0x1<<HBIASADCEN);
	snd_soc_write(codec, ADC_APC_CTRL, reg_val);

	/*Earphone Plugin/out Irq Enable*/
	reg_val = snd_soc_read(codec, HMIC_CTRL1);
	reg_val |= (0x1<<HMIC_PULLOUT_IRQ);
	reg_val |= (0x1<<HMIC_PLUGIN_IRQ);
	snd_soc_write(codec, HMIC_CTRL1, reg_val);

	/*Hmic KeyUp/key down Irq Enable*/
	reg_val = snd_soc_read(codec, HMIC_CTRL1);
	reg_val |= (0x1<<HMIC_KEYDOWN_IRQ);
	reg_val |= (0x1<<HMIC_KEYUP_IRQ);
	snd_soc_write(codec, HMIC_CTRL1, reg_val);
}


static void codec_resume_work(struct work_struct *work)
{
	int reg_val = 0;
	struct headset_data *hs_data = container_of(work, struct headset_data, codec_resume);
	struct snd_soc_codec *codec = hs_data->codec;
	msleep(10);
	/*disable this bit to prevent leakage from ldoin*/
	reg_val = snd_soc_read(codec, ADDA_TUNE3);
	reg_val |= (0x1<<OSCEN);
	snd_soc_write(codec, ADDA_TUNE3, reg_val);
	switch_hw_config(codec);
}

static int virq;
static int sndvir_audio_soc_probe(struct snd_soc_codec *codec)
{
	int ret = 0;
	int reg_val1 = 0;

	struct headset_data *hs_data;
	struct device *dev = codec->dev;

	debug_codec = codec;

	ret = snd_soc_codec_set_cache_io(codec, 8, 16, SND_SOC_I2C);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to set cache I/O: %d\n", ret);
		return ret;
	}
	pr_debug("AUDIO_I2C_BUS:%s, line:%d\n", __func__, __LINE__);

	codec_switch = codec;

	/********************initialize the hs_data*******************/
	hs_data = kzalloc(sizeof(struct headset_data), GFP_KERNEL);
	if (hs_data == NULL) {
		return -ENOMEM;
	}
	hs_data->codec = codec;

	hs_data->sdev.state 		= 0;
	hs_data->state				= -1;
	hs_data->check_count	=0;
	hs_data->sdev.name 			= "h2w222";  // not use
	hs_data->sdev.print_name 	= print_headset_name;
	hs_data->sdev.print_state 	= switch_gpio_print_state;
	snd_soc_codec_set_drvdata(codec, hs_data);
	ret = switch_dev_register(&hs_data->sdev);
	if (ret < 0) {
		goto err_switch_dev_register;
	}

	/*use for judge the state of switch*/
	INIT_WORK(&hs_data->work, earphone_switch_work);
	INIT_WORK(&hs_data->clear_codec_irq,clear_codec_irq_work);
	INIT_WORK(&hs_data->codec_resume, codec_resume_work);

	/********************create input device************************/
	hs_data->key = input_allocate_device();
	if (!hs_data->key) {
	     printk(KERN_ERR "input_allocate_device: not enough memory for input device\n");
	     ret = -ENOMEM;
	     goto err_input_allocate_device;
	}

	hs_data->key->name          = "headset";
	hs_data->key->phys          = "headset/input0";
	hs_data->key->id.bustype    = BUS_HOST;
	hs_data->key->id.vendor     = 0x0001;
	hs_data->key->id.product    = 0xffff;
	hs_data->key->id.version    = 0x0100;

	hs_data->key->evbit[0] = BIT_MASK(EV_KEY);

	set_bit(KEY_HEADSETHOOK, hs_data->key->keybit);
	set_bit(KEY_VOLUMEUP, hs_data->key->keybit);
	set_bit(KEY_VOLUMEDOWN, hs_data->key->keybit);

	ret = input_register_device(hs_data->key);
	if (ret) {
	    printk(KERN_ERR "input_register_device: input_register_device failed\n");
	    goto err_input_register_device;
	}

	sema_init(&hs_data->sem, 1);
	headphone_state = 0;
	hs_data->mode = HEADPHONE_IDLE;
	
	// get aif3 voltage and audio interrupt pin from cpu
	/************************example***********************************************/
	#if 0

	if(script_get_item("audio0", "aif3_voltage", &aif3_voltage) != SCIRPT_ITEM_VALUE_TYPE_STR){
		printk("[aif3_voltage]script_get_item return type err!!!!!!!!!!!!\n");
		return -EFAULT;
	}

	type = script_get_item("audio0", "audio_int_ctrl", &item_eint);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		printk("script_get_item return type err\n");
		return -EFAULT;
	}
	#endif
	/*************************example**********************************************/
	codec_irq_queue = create_singlethread_workqueue("codec_irq");

	switch_detect_queue = create_singlethread_workqueue("codec_resume");
	if (switch_detect_queue == NULL) {
		printk("[AC100] try to create workqueue for codec failed!\n");
		ret = -ENOMEM;
		goto err_switch_work_queue;
	}

	/*
	*here ,you should register the gpio irq for headphone detection.
	*
	*the follow is the example:
	*/
	/**********************************example*****************************************/
#if 0
	/*
	* map the virq of gpio
	* headphone gpio irq pin is PL7
	* item_eint.gpio.gpio = GPIOL(7);
	*/
	virq = gpio_to_irq(item_eint.gpio.gpio);
	if (IS_ERR_VALUE(virq)) {
		pr_warn("map gpio [%d] to virq failed, errno = %d\n",
		GPIOA(0), virq);
		return -EINVAL;
	}

	pr_debug("gpio [%d] map to virq [%d] ok\n", item_eint.gpio.gpio, virq);
	/* request virq, set virq type to high level trigger */
	ret = devm_request_irq(dev, virq, audio_hmic_irq, IRQF_TRIGGER_FALLING, "PL7_EINT", hs_data);
	if (IS_ERR_VALUE(ret)) {
		pr_warn("request virq %d failed, errno = %d\n", virq, ret);
        	return -EINVAL;
	}

	/*
	* item_eint.gpio.gpio = GPIOL(7);
	* select HOSC 24Mhz(PIO Interrupt Clock Select)
	*/
	req_status = gpio_request(item_eint.gpio.gpio, NULL);
	if (0 != req_status) {
		pr_warn("request gpio[%d] failed!\n", item_eint.gpio.gpio);
		return -EINVAL;
	}
	gpio_set_debounce(item_eint.gpio.gpio, 1);
#endif
/*********************************************example************************************************/
	/* initialize the codec register for headphone detection*/
	switch_hw_config(codec);

	reg_val1 = snd_soc_read(codec, ADDA_TUNE3);
	reg_val1 |= (0x1<<OSCEN);

	printk("===%s:write reg=%d,v=%d===\n",__func__,ADDA_TUNE3,reg_val1);
	snd_soc_write(codec, ADDA_TUNE3, reg_val1);
	printk("===%s:read reg=%d,v=%d===\n",__func__,ADDA_TUNE3,snd_soc_read(codec, ADDA_TUNE3));
	printk("===%s:read reg=%d,v=%d===\n",__func__,SPKOUT_CTRL,snd_soc_read(codec, SPKOUT_CTRL));

	/* RTC 32.768 KHz clock output */
	snd_soc_write(codec, 0xc2, 0x01);
	snd_soc_write(codec, 0xc3, 0x01);
	/**/
	// request and initially turn on speaker's external pa enable pin.
	/*************************************example************************************/
	#if 0
	/*get the default pa val(close)*/
	type = script_get_item("audio0", "audio_pa_ctrl", &item);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		printk("script_get_item return type err\n");
		return -EFAULT;
	}

	/*request pa gpio*/
	req_status = gpio_request(item.gpio.gpio, NULL);
	if (0 != req_status) {
		printk("request gpio failed!\n");
	}
	/*
	* config gpio info of audio_pa_ctrl, the default pa config is close(check pa sys_config1.fex)
	*/
	gpio_direction_output(item.gpio.gpio, 1);
	gpio_set_value(item.gpio.gpio, 0);
	#endif
	/*********************************example*****************************************/
	return 0;
err_switch_work_queue:
	devm_free_irq(dev,virq,NULL);

err_input_register_device:
	if(hs_data->key){
		input_free_device(hs_data->key);
	}

err_input_allocate_device:
	switch_dev_unregister(&hs_data->sdev);

err_switch_dev_register:
	kfree(hs_data);

	return ret;
}

/* power down chip */
static int sndvir_audio_soc_remove(struct snd_soc_codec *codec)
{
	struct headset_data *hs_data = snd_soc_codec_get_drvdata(codec);
	struct device *dev = codec->dev;

	devm_free_irq(dev,virq,NULL);
	if (hs_data->key) {
		input_unregister_device(hs_data->key);
		input_free_device(hs_data->key);
   	}
 	switch_dev_unregister(&hs_data->sdev);

	kfree(hs_data);
	return 0;
}

static int sndvir_audio_suspend(struct snd_soc_codec *codec, pm_message_t state )
{
	int reg_val;

	printk("[AC100 codec]:suspend\n");
	/* check if called in talking standby */
/*	if (check_scene_locked(SCENE_TALKING_STANDBY) == 0) {
		printk("In talking standby, audio codec do not suspend!!\n");
		return 0;
	} */
	/*Headset microphone BIAS Enable*/
	reg_val = snd_soc_read(codec, ADC_APC_CTRL);
	reg_val &= ~(0x1<<HBIASEN);
	snd_soc_write(codec, ADC_APC_CTRL, reg_val);

	/*Headset microphone BIAS Current sensor & ADC Enable*/
	reg_val = snd_soc_read(codec, ADC_APC_CTRL);
	reg_val &= ~(0x1<<HBIASADCEN);
	snd_soc_write(codec, ADC_APC_CTRL, reg_val);

	/*disable pa_ctrl, turn off speaker's external pa. */
	//gpio_set_value(item.gpio.gpio, 0);
	msleep(20);

	/*disable pa*/
	reg_val = snd_soc_read(codec, HPOUT_CTRL);
	reg_val &= ~(0x1<<HPPA_EN);
	snd_soc_write(codec, HPOUT_CTRL, reg_val);

	/*hardware xzh support*/
	reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
	reg_val &= ~(0xf<<HPOUTPUTENABLE);
	snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

	/*unmute l/r headphone pa*/
	reg_val = snd_soc_read(codec, HPOUT_CTRL);
	reg_val &= ~((0x1<<RHPPA_MUTE)|(0x1<<LHPPA_MUTE));
	snd_soc_write(codec, HPOUT_CTRL, reg_val);

/***** disable avcc *****/


	/*disable this bit to prevent leakage from ldoin*/
	reg_val = snd_soc_read(codec, ADDA_TUNE3);
	reg_val &= ~(0x1<<OSCEN);
	snd_soc_write(codec, ADDA_TUNE3, reg_val);
	return 0;
}

static int sndvir_audio_resume(struct snd_soc_codec *codec)
{
	int reg_val;
	struct headset_data *hs_data = snd_soc_codec_get_drvdata(codec);
	printk("[codec]:resume");
	hs_data->mode = HEADPHONE_IDLE;
	headphone_state = 0;
	hs_data->state	= 0;
	/*process for normal standby*/
//	if (NORMAL_STANDBY == standby_type) {
	/*process for super standby*/
//	} else if(SUPER_STANDBY == standby_type) {
//		schedule_work(&hs_data->codec_resume);

//	}

/********* enable avcc **********/


	reg_val = snd_soc_read(codec, ADDA_TUNE3);
	reg_val |=(0x1<<OSCEN);
	snd_soc_write(codec, ADDA_TUNE3, reg_val);
	
	switch_hw_config(codec);

	return 0;
}


static struct snd_soc_codec_driver soc_codec_dev_sndvir_audio = {
	.probe 		=	sndvir_audio_soc_probe,
	.remove 	=   sndvir_audio_soc_remove,
	.suspend 	= 	sndvir_audio_suspend,
	.resume 	=	sndvir_audio_resume,
	.controls 	= 	sndvir_audio_controls,
	.num_controls = ARRAY_SIZE(sndvir_audio_controls),
};

static int sunxi_codec_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

	if (twi_id == adapter->nr) {
		strlcpy(info->type, SUNXI_CODEC_NAME, I2C_NAME_SIZE);
		return 0;
	} else {
		return -ENODEV;
	}
}

static int __init sndvir_audio_codec_probe(struct i2c_client *i2c,
				      const struct i2c_device_id *id)
{
	return snd_soc_register_codec(&i2c->dev, &soc_codec_dev_sndvir_audio, &sndvir_audio_dai, 1);
}

static int __exit sndvir_audio_codec_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	return 0;
}

static const unsigned short normal_i2c[] = {0x1a, I2C_CLIENT_END};

static const struct i2c_device_id sunxi_codec_id[] = {
	{"ac100", 0},
	{}
};

static struct i2c_driver sunxi_codec_driver = {
	.class 		= I2C_CLASS_HWMON,
	.id_table 	= sunxi_codec_id,
	.probe 		= sndvir_audio_codec_probe,
	.remove 	= __exit_p(sndvir_audio_codec_remove),
	.driver 	= {
		.owner 	= THIS_MODULE,
		.name 	= "ac100",
	},
	.address_list = normal_i2c,
};

static ssize_t ac100_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	static int val = 0, flag = 0;
	u8 reg,num,i=0;
	u16 value_w,value_r[128];
	val = simple_strtol(buf, NULL, 16);
	flag = (val >> 24) & 0xF;
	if(flag) {//write
		reg = (val >> 16) & 0xFF;
		value_w =  val & 0xFFFF;
		snd_soc_write(debug_codec, reg, value_w);
		printk("write 0x%x to reg:0x%x\n",value_w,reg);
	} else {
		reg =(val>>8)& 0xFF;
		num=val&0xff;
		printk("read:start add:0x%x,count:0x%x\n",reg,num);
		do{
			value_r[i] = snd_soc_read(debug_codec, reg);
			printk("	%04x",value_r[i]);
			reg+=1;
			i++;
			if(i%4==0)
				printk("\n");
		}while(i<num);
	}
	return count;
}
static ssize_t ac100_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	printk("echo flag|reg|val > ac100\n");
	printk("eg read star addres=0x06,count 0x10:echo 0610 >ac100\n");
	printk("eg write value:0x13fe to address:0x06 :echo 10613fe > ac100\n");
    return 0;
}
static struct device ac100_device;
static DEVICE_ATTR(ac100, S_IWUSR | S_IRUGO, ac100_show, ac100_store);
static int ac100_init(void)
{
        int ret;
	dev_set_name(&ac100_device, "ac100");

	if (device_register(&ac100_device))
                printk("error device_register()\n");

	ret = device_create_file(&ac100_device, &dev_attr_ac100);
	if (ret)
                printk("device_create_file error\n");

	printk("axp device register ok and device_create_file ok\n");
        return 0;
}

static int __init sndvir_audio_codec_init(void)
{
	int ret;
	if(strcmp(g_selected_codec,"ac100"))
		return 0;
        ac100_init();
	sunxi_codec_driver.detect = sunxi_codec_detect;
	ret = i2c_add_driver(&sunxi_codec_driver);
	return ret;
}
module_init(sndvir_audio_codec_init);

static void __exit sndvir_audio_codec_exit(void)
{
	if(strcmp(g_selected_codec,"ac100"))
		return ;
	i2c_del_driver(&sunxi_codec_driver);
}
module_exit(sndvir_audio_codec_exit);

MODULE_DESCRIPTION("SNDVIR_AUDIO ALSA soc codec driver");
MODULE_AUTHOR("huangxin");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-vir_audio-codec");
