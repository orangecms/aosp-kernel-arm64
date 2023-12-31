/*
 * sound/soc/amlogic/auge/pdm.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/gpio/consumer.h>
#include <linux/pinctrl/consumer.h>

#include <sound/soc.h>
#include <sound/tlv.h>

#include <linux/amlogic/pm.h>

#include "pdm.h"
#include "pdm_hw.h"
#include "pdm_match_table.c"
#include "audio_io.h"
#include "iomap.h"
#include "regs.h"
#include "ddr_mngr.h"
#include "vad.h"

/*#define __PTM_PDM_CLK__*/

static struct snd_pcm_hardware aml_pdm_hardware = {
	.info			=
					SNDRV_PCM_INFO_MMAP |
					SNDRV_PCM_INFO_MMAP_VALID |
					SNDRV_PCM_INFO_INTERLEAVED |
					SNDRV_PCM_INFO_PAUSE |
					SNDRV_PCM_INFO_RESUME,

	.formats		=	SNDRV_PCM_FMTBIT_S16 |
					SNDRV_PCM_FMTBIT_S24 |
					SNDRV_PCM_FMTBIT_S32,

	.rate_min			=	8000,
	.rate_max			=	96000,

	.channels_min		=	PDM_CHANNELS_MIN,
	.channels_max		=	PDM_CHANNELS_LB_MAX,

	.buffer_bytes_max	=	512 * 1024,
	.period_bytes_max	=	256 * 1024,
	.period_bytes_min	=	32,
	.periods_min		=	2,
	.periods_max		=	1024,
	.fifo_size		=	0,
};

static unsigned int period_sizes[] = {
	64, 128, 256, 512, 1024, 2048, 4096,
	8192, 16384, 32768, 65536, 65536 * 2, 65536 * 4
};

static struct snd_pcm_hw_constraint_list hw_constraints_period_sizes = {
	.count = ARRAY_SIZE(period_sizes),
	.list = period_sizes,
	.mask = 0
};

static const char *const pdm_filter_mode_texts[] = {
	"Filter Mode 0",
	"Filter Mode 1",
	"Filter Mode 2",
	"Filter Mode 3",
	"Filter Mode 4"
};

static const struct soc_enum pdm_filter_mode_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, ARRAY_SIZE(pdm_filter_mode_texts),
			pdm_filter_mode_texts);

static struct aml_pdm *s_pdm;

static struct aml_pdm *get_pdm(void)
{
	if (!s_pdm) {
		pr_err("Not init pdm\n");
		return NULL;
	}

	return s_pdm;
}

int pdm_get_train_sample_count_from_dts(void)
{
	struct aml_pdm *p_pdm = get_pdm();

	if (p_pdm)
		return  p_pdm->train_sample_count;
	else
		return -1;
}

int pdm_get_train_version(void)
{
	struct aml_pdm *p_pdm = get_pdm();

	if (p_pdm && p_pdm->chipinfo)
		return p_pdm->chipinfo->train_version;

	return 0;
}

static int aml_pdm_filter_mode_get_enum(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct aml_pdm *p_pdm = dev_get_drvdata(component->dev);

	if (!p_pdm)
		return 0;

	ucontrol->value.enumerated.item[0] = p_pdm->filter_mode;

	return 0;
}

static int aml_pdm_filter_mode_set_enum(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct aml_pdm *p_pdm = dev_get_drvdata(component->dev);

	if (!p_pdm)
		return 0;

	p_pdm->filter_mode = ucontrol->value.enumerated.item[0];

	return 0;
}

int pdm_hcic_shift_gain;

static const char *const pdm_hcic_shift_gain_texts[] = {
	"keep with coeff",
	"shift with -0x4",
};

static const struct soc_enum pdm_hcic_shift_gain_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, ARRAY_SIZE(pdm_hcic_shift_gain_texts),
			pdm_hcic_shift_gain_texts);

static int pdm_hcic_shift_gain_get_enum(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.enumerated.item[0] = pdm_hcic_shift_gain;

	return 0;
}

static int pdm_hcic_shift_gain_set_enum(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	pdm_hcic_shift_gain = ucontrol->value.enumerated.item[0];

	return 0;
}

static const char *const pdm_dclk_texts[] = {
	"PDM Dclk 3.072m, support 8k/16k/32k/48k/64k/96k",
	"PDM Dclk 1.024m, support 8k/16k",
	"PDM Dclk   768k, support 8k/16k",
};

static const struct soc_enum pdm_dclk_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, ARRAY_SIZE(pdm_dclk_texts),
			pdm_dclk_texts);

static int pdm_dclk_get_enum(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct aml_pdm *p_pdm = dev_get_drvdata(component->dev);

	if (!p_pdm)
		return 0;

	ucontrol->value.enumerated.item[0] = p_pdm->dclk_idx;

	return 0;
}

static int pdm_dclk_set_enum(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct aml_pdm *p_pdm = dev_get_drvdata(component->dev);

	if (!p_pdm)
		return 0;

	p_pdm->dclk_idx = ucontrol->value.enumerated.item[0];

	return 0;
}


static const char *const pdm_bypass_texts[] = {
	"PCM Data",
	"Raw Data/Bypass Data",
};

static const struct soc_enum pdm_bypass_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, ARRAY_SIZE(pdm_bypass_texts),
			pdm_bypass_texts);

static int pdm_bypass_get_enum(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct aml_pdm *p_pdm = dev_get_drvdata(component->dev);

	if (!p_pdm)
		return 0;

	ucontrol->value.enumerated.item[0] = p_pdm->bypass;

	return 0;
}

static int pdm_bypass_set_enum(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct aml_pdm *p_pdm = dev_get_drvdata(component->dev);

	if (!p_pdm)
		return 0;

	p_pdm->bypass = ucontrol->value.enumerated.item[0];

	if (p_pdm->clk_on)
		pdm_set_bypass_data((bool)p_pdm->bypass);

	return 0;
}

static const char *const pdm_lowpower_texts[] = {
	"PDM Normal Mode",
	"PDM Low Power Mode",
};

static const struct soc_enum pdm_lowpower_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, ARRAY_SIZE(pdm_lowpower_texts),
			pdm_lowpower_texts);

static void pdm_set_lowpower_mode(struct aml_pdm *p_pdm, bool isLowPower)
{
	if (p_pdm->isLowPower == isLowPower)
		return;

	p_pdm->isLowPower = isLowPower;

	if (p_pdm->clk_on) {
		int osr, filter_mode, dclk_idx;

		if (p_pdm->isLowPower) {
			/* dclk for 768k */
			dclk_idx = 2;

			pr_info("%s force pdm sysclk to 24m, dclk 768k\n",
				__func__);
		} else
			dclk_idx = p_pdm->dclk_idx;

		clk_set_rate(p_pdm->clk_pdm_dclk,
			pdm_dclkidx2rate(dclk_idx));

		/* filter for pdm */
		osr = pdm_get_ors(dclk_idx, p_pdm->rate);
		if (!osr)
			osr = 192;

		filter_mode = p_pdm->isLowPower ? 4 : p_pdm->filter_mode;
		aml_pdm_filter_ctrl(osr, filter_mode);

		/* update sample count */
		pdm_set_channel_ctrl(
			pdm_get_sample_count(
				p_pdm->isLowPower,
				dclk_idx)
			);

		/* check to set pdm sysclk */
		pdm_force_sysclk_to_oscin(p_pdm->isLowPower);

		pr_info("\n%s, pdm_sysclk:%lu pdm_dclk:%lu, dclk_srcpll:%lu\n",
			__func__,
			clk_get_rate(p_pdm->clk_pdm_sysclk),
			clk_get_rate(p_pdm->clk_pdm_dclk),
			clk_get_rate(p_pdm->dclk_srcpll));

		/* Check to set vad for Low Power */
		if (vad_pdm_is_running())
			vad_set_lowerpower_mode(p_pdm->isLowPower);
	}
}

static int pdm_lowpower_get_enum(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct aml_pdm *p_pdm = dev_get_drvdata(component->dev);

	if (!p_pdm)
		return 0;

	ucontrol->value.enumerated.item[0] = p_pdm->isLowPower;

	return 0;
}


static int pdm_lowpower_set_enum(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct aml_pdm *p_pdm = dev_get_drvdata(component->dev);
	bool isLowPower;

	if (!p_pdm)
		return 0;

	isLowPower = (bool)ucontrol->value.enumerated.item[0];
	pdm_set_lowpower_mode(p_pdm, isLowPower);

	return 0;
}

static const char *const pdm_train_texts[] = {
	"Disabled",
	"Enable",
};

static const struct soc_enum pdm_train_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, ARRAY_SIZE(pdm_train_texts),
			pdm_train_texts);

static int pdm_train_get_enum(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct aml_pdm *p_pdm = dev_get_drvdata(component->dev);

	if (!p_pdm)
		return 0;

	ucontrol->value.enumerated.item[0] = p_pdm->train_en;

	return 0;
}

static int pdm_train_set_enum(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct aml_pdm *p_pdm = dev_get_drvdata(component->dev);

	if (!p_pdm ||
		!p_pdm->chipinfo ||
		!p_pdm->chipinfo->train ||
		(p_pdm->train_en == ucontrol->value.enumerated.item[0]))
		return 0;

	p_pdm->train_en = ucontrol->value.enumerated.item[0];

	if (p_pdm->clk_on)
		pdm_train_en(p_pdm->train_en);

	return 0;
}

static const struct snd_kcontrol_new snd_pdm_controls[] = {
	/* which set */
	SOC_ENUM_EXT("PDM Filter Mode",
		     pdm_filter_mode_enum,
		     aml_pdm_filter_mode_get_enum,
		     aml_pdm_filter_mode_set_enum),

	/* fix HCIC shift gain according current dmic */
	SOC_ENUM_EXT("PDM HCIC shift gain from coeff",
		     pdm_hcic_shift_gain_enum,
		     pdm_hcic_shift_gain_get_enum,
		     pdm_hcic_shift_gain_set_enum),

	SOC_ENUM_EXT("PDM Dclk",
		     pdm_dclk_enum,
		     pdm_dclk_get_enum,
		     pdm_dclk_set_enum),

	SOC_ENUM_EXT("PDM Low Power mode",
			pdm_lowpower_enum,
			pdm_lowpower_get_enum,
			pdm_lowpower_set_enum),

	SOC_ENUM_EXT("PDM Train",
		     pdm_train_enum,
		     pdm_train_get_enum,
		     pdm_train_set_enum),

	SOC_ENUM_EXT("PDM Bypass",
		     pdm_bypass_enum,
		     pdm_bypass_get_enum,
		     pdm_bypass_set_enum),
};

#if 0
static int pdm_mute_val_info(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 0xffffffff;
	uinfo->count = 1;

	return 0;
}

static int pdm_mute_val_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	int val;

	val = pdm_get_mute_value();
	ucontrol->value.integer.value[0] = val;

	pr_info("%s, get mute_val:0x%x\n",
		__func__,
		val);

	return 0;
}

static int pdm_mute_val_set(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	int val  = (int)ucontrol->value.integer.value[0];

	pr_info("%s, set mute_val:0x%x\n",
		__func__,
		val);

	pdm_set_mute_value(val);

	return 0;
}

static struct snd_kcontrol_new mute_val_control = {
	.iface = SNDRV_CTL_ELEM_IFACE_PCM,
	.name = "PDM Mute Value",
	.info = pdm_mute_val_info,
	.get = pdm_mute_val_get,
	.put = pdm_mute_val_set,
	.private_value = 0,
};

static int pdm_mute_chmask_info(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 0xff;
	uinfo->count = 1;

	return 0;
}

static int pdm_mute_chmask_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	int val;

	val = pdm_get_mute_channel();

	ucontrol->value.integer.value[0] = val;

	pr_info("%s, get pdm channel mask val:0x%x\n",
		__func__,
		val);

	return 0;
}

static int pdm_mute_chmask_set(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	int val  = (int)ucontrol->value.integer.value[0];

	if (val < 0)
		val = 0;
	if (val > 255)
		val = 255;

	pr_info("%s, set pdm channel mask val:0x%x\n",
		__func__,
		val);

	pdm_set_mute_channel(val);

	return 0;
}

static struct snd_kcontrol_new mute_chmask_control = {
	.iface = SNDRV_CTL_ELEM_IFACE_PCM,
	.name = "PDM Mute Channel Mask",
	.info = pdm_mute_chmask_info,
	.get = pdm_mute_chmask_get,
	.put = pdm_mute_chmask_set,
	.private_value = 0,
};

static const struct snd_kcontrol_new *snd_pdm_chipinfo_controls[PDM_RUN_MAX] = {
	&mute_val_control,
	&mute_chmask_control,
};

static void pdm_running_destroy_controls(struct snd_card *card,
	struct aml_pdm *p_pdm)
{
	int i;

	for (i = 0; i < PDM_RUN_MAX; i++)
		if (p_pdm->controls[i])
			snd_ctl_remove(card, p_pdm->controls[i]);
}

static int pdm_running_create_controls(struct snd_card *card,
	struct aml_pdm *p_pdm)
{
	int i, err = 0;

	memset(p_pdm->controls, 0, sizeof(p_pdm->controls));

	for (i = 0; i < PDM_RUN_MAX; i++) {
		p_pdm->controls[i] =
			snd_ctl_new1(snd_pdm_chipinfo_controls[i], NULL);
		err = snd_ctl_add(card, p_pdm->controls[i]);
		if (err < 0)
			goto __error;
	}

	return 0;

__error:
	pdm_running_destroy_controls(card, p_pdm);

	return err;
}
#endif
static irqreturn_t aml_pdm_isr_handler(int irq, void *data)
{
	struct snd_pcm_substream *substream =
		(struct snd_pcm_substream *)data;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->platform->dev;
	struct aml_pdm *p_pdm = (struct aml_pdm *)dev_get_drvdata(dev);
	unsigned int status;
	int train_sts = pdm_train_sts();

	pr_debug("%s\n", __func__);

	if (!snd_pcm_running(substream))
		return IRQ_NONE;

	status = aml_toddr_get_status(p_pdm->tddr) & MEMIF_INT_MASK;
	if (status & MEMIF_INT_COUNT_REPEAT) {
		snd_pcm_period_elapsed(substream);

		aml_toddr_ack_irq(p_pdm->tddr, MEMIF_INT_COUNT_REPEAT);
	} else
		dev_dbg(dev, "unexpected irq - STS 0x%02x\n",
			status);

	if (train_sts) {
		pr_debug("%s train result:0x%x\n",
			__func__,
			train_sts);
		pdm_train_clr();
	}

	return !status ? IRQ_NONE : IRQ_HANDLED;
}

static int aml_pdm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->platform->dev;
	struct aml_pdm *p_pdm = (struct aml_pdm *)
							dev_get_drvdata(dev);
	int ret;

	pr_debug("%s, stream:%d\n",
		__func__, substream->stream);

	snd_soc_set_runtime_hwparams(substream, &aml_pdm_hardware);

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	/* Ensure that period size is a multiple of 32bytes */
	ret = snd_pcm_hw_constraint_list(runtime, 0,
					   SNDRV_PCM_HW_PARAM_PERIOD_BYTES,
					   &hw_constraints_period_sizes);
	if (ret < 0) {
		dev_err(substream->pcm->card->dev,
			"%s() setting constraints failed: %d\n",
			__func__, ret);
		return -EINVAL;
	}

	/* Ensure that buffer size is a multiple of period size */
	ret = snd_pcm_hw_constraint_integer(runtime,
						SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0) {
		dev_err(substream->pcm->card->dev,
			"%s() setting constraints failed: %d\n",
			__func__, ret);
		return -EINVAL;
	}

	runtime->private_data = p_pdm;

	p_pdm->tddr = aml_audio_register_toddr
		(dev, p_pdm->actrl, aml_pdm_isr_handler, substream);
	if (!p_pdm->tddr) {
		dev_err(dev, "failed to claim to ddr\n");
		return -ENXIO;
	}

	return 0;
}

static int aml_pdm_close(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->platform->dev;
	struct aml_pdm *p_pdm = (struct aml_pdm *)dev_get_drvdata(dev);

	pr_debug("enter %s type: %d\n",
		__func__, substream->stream);

	aml_audio_unregister_toddr(p_pdm->dev, substream);

	return 0;
}


static int aml_pdm_hw_params(
	struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int ret = 0;

	pr_debug("enter %s\n", __func__);

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	runtime->dma_bytes = params_buffer_bytes(params);
	memset(runtime->dma_area, 0, runtime->dma_bytes);

	return ret;
}

static int aml_pdm_hw_free(struct snd_pcm_substream *substream)
{
	pr_debug("%s\n", __func__);
	snd_pcm_lib_free_pages(substream);

	return 0;
}

static int aml_pdm_prepare(
	struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct aml_pdm *p_pdm = runtime->private_data;

	struct toddr *to = p_pdm->tddr;
	unsigned int start_addr, end_addr, int_addr;

	start_addr = runtime->dma_addr;
	end_addr = start_addr + runtime->dma_bytes - 8;
	int_addr = frames_to_bytes(runtime, runtime->period_size) / 8;

	aml_toddr_set_buf(to, start_addr, end_addr);
	aml_toddr_set_intrpt(to, int_addr);

	return 0;
}

static int aml_pdm_trigger(
	struct snd_pcm_substream *substream, int cmd)
{
	return 0;
}

static snd_pcm_uframes_t aml_pdm_pointer(
	struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct aml_pdm *p_pdm = runtime->private_data;
	unsigned int addr = 0, start_addr = 0;

	start_addr = runtime->dma_addr;

	addr = aml_toddr_get_position(p_pdm->tddr);

	return bytes_to_frames(runtime, addr - start_addr);
}

static int aml_pdm_mmap(
	struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	return dma_mmap_coherent(substream->pcm->card->dev, vma,
			runtime->dma_area, runtime->dma_addr,
			runtime->dma_bytes);
}

static int aml_pdm_silence(
	struct snd_pcm_substream *substream,
	int channel, snd_pcm_uframes_t pos, snd_pcm_uframes_t count)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned char *ppos = NULL;
	ssize_t n;

	pr_debug("%s\n", __func__);

	n = frames_to_bytes(runtime, count);
	ppos = runtime->dma_area + frames_to_bytes(runtime, pos);
	memset(ppos, 0, n);

	return 0;
}

static int aml_pdm_pcm_new(struct snd_soc_pcm_runtime *soc_runtime)
{
	struct snd_pcm *pcm = soc_runtime->pcm;
	struct snd_pcm_substream *substream;
	struct snd_soc_dai *dai = soc_runtime->cpu_dai;
	int size = aml_pdm_hardware.buffer_bytes_max;
	int ret = -EINVAL;

	pr_debug("%s dai->name: %s dai->id: %d\n",
		__func__, dai->name, dai->id);

	/* only capture for PDM */
	substream = pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream;
	if (substream) {
		ret = snd_dma_alloc_pages(SNDRV_DMA_TYPE_DEV,
					soc_runtime->platform->dev,
					size, &substream->dma_buffer);
		if (ret) {
			dev_err(soc_runtime->dev, "Cannot allocate buffer(s)\n");
			return ret;
		}
	}

	return 0;
}

static void aml_pdm_pcm_free(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;

	pr_debug("%s\n", __func__);

	substream = pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream;
	if (substream) {
		snd_dma_free_pages(&substream->dma_buffer);
		substream->dma_buffer.area = NULL;
		substream->dma_buffer.addr = 0;
	}
}

static struct snd_pcm_ops aml_pdm_ops = {
	.open = aml_pdm_open,
	.close = aml_pdm_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = aml_pdm_hw_params,
	.hw_free = aml_pdm_hw_free,
	.prepare = aml_pdm_prepare,
	.trigger = aml_pdm_trigger,
	.pointer = aml_pdm_pointer,
	.mmap = aml_pdm_mmap,
	.silence = aml_pdm_silence,
};

static int aml_pdm_probe(struct snd_soc_platform *platform)
{
	pr_debug("%s\n", __func__);

	return 0;
}

struct snd_soc_platform_driver aml_soc_platform_pdm = {
	.pcm_new = aml_pdm_pcm_new,
	.pcm_free = aml_pdm_pcm_free,
	.ops = &aml_pdm_ops,
	.probe = aml_pdm_probe,
};
EXPORT_SYMBOL_GPL(aml_soc_platform_pdm);

static int aml_pdm_dai_hw_params(
	struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *cpu_dai)
{
	return 0;
}

static int aml_pdm_dai_set_fmt(
	struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	return 0;
}

static int aml_pdm_dai_prepare(
	struct snd_pcm_substream *substream,
	struct snd_soc_dai *cpu_dai)
{
	struct aml_pdm *p_pdm = snd_soc_dai_get_drvdata(cpu_dai);
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned int bitwidth;
	unsigned int toddr_type, lsb;
	struct toddr *to = p_pdm->tddr;
	struct toddr_fmt fmt;
	unsigned int osr = 192, filter_mode, dclk_idx;
	struct pdm_info info;

	if (vad_pdm_is_running()
		&& pm_audio_is_suspend())
		return 0;

	p_pdm->rate = runtime->rate;

	/* set bclk */
	bitwidth = snd_pcm_format_width(runtime->format);
	lsb = 32 - bitwidth;

	switch (bitwidth) {
	case 16:
	case 32:
		toddr_type = 0;
		break;
	case 24:
		toddr_type = 4;
		break;
	default:
		pr_err("invalid bit_depth: %d\n",
				bitwidth);
		return -1;
	}

	pr_debug("%s rate:%d, bits:%d, channels:%d\n",
		__func__,
		runtime->rate,
		bitwidth,
		runtime->channels);

	/* to ddr pdmin */
	fmt.type      = toddr_type;
	fmt.msb       = 31;
	fmt.lsb       = lsb;
	fmt.endian    = 0;
	fmt.bit_depth = bitwidth;
	fmt.ch_num    = runtime->channels;
	fmt.rate      = runtime->rate;
	aml_toddr_select_src(to, PDMIN);
	aml_toddr_set_format(to, &fmt);
	aml_toddr_set_fifos(to, 0x40);

	/* force pdm sysclk to 24m */
	if (p_pdm->isLowPower) {
		/* dclk for 768k */
		dclk_idx = 2;
		filter_mode = 4;
		pdm_force_sysclk_to_oscin(true);
		if (vad_pdm_is_running())
			vad_set_lowerpower_mode(true);

	} else {
		dclk_idx = p_pdm->dclk_idx;
		filter_mode = p_pdm->filter_mode;
	}

	/* filter for pdm */
	osr = pdm_get_ors(dclk_idx, runtime->rate);
	if (!osr)
		return -EINVAL;

	pr_info("%s, pdm_dclk:%d, osr:%d, rate:%d filter mode:%d\n",
		__func__,
		pdm_dclkidx2rate(dclk_idx),
		osr,
		runtime->rate,
		p_pdm->filter_mode);

	info.bitdepth   = bitwidth;
	info.channels   = runtime->channels;
	info.lane_masks = p_pdm->lane_mask_in;
	info.dclk_idx   = dclk_idx;
	info.bypass     = p_pdm->bypass;
	info.sample_count = pdm_get_sample_count(p_pdm->isLowPower,
							dclk_idx);

	aml_pdm_ctrl(&info);
	aml_pdm_filter_ctrl(osr, filter_mode);

	if (p_pdm->chipinfo && p_pdm->chipinfo->truncate_data)
		pdm_init_truncate_data(runtime->rate);

	return 0;
}

static int aml_pdm_dai_trigger(
	struct snd_pcm_substream *substream, int cmd,
		struct snd_soc_dai *cpu_dai)
{
	struct aml_pdm *p_pdm = snd_soc_dai_get_drvdata(cpu_dai);
	bool toddr_stopped = false;

	pr_debug("%s\n", __func__);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:

		if (vad_pdm_is_running()
			&& pm_audio_is_suspend()) {
			pm_audio_set_suspend(false);
			/* VAD switch to alsa buffer */
			vad_update_buffer(0);
			audio_toddr_irq_enable(p_pdm->tddr, true);
			break;
		}

		pdm_fifo_reset();

		dev_info(substream->pcm->card->dev, "PDM Capture start\n");
		aml_toddr_enable(p_pdm->tddr, 1);
		pdm_enable(1);

		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (vad_pdm_is_running() && pm_audio_is_suspend()) {
			/* switch to VAD buffer */
			vad_update_buffer(1);
			audio_toddr_irq_enable(p_pdm->tddr, false);
			break;
		}
		pdm_enable(0);
		dev_info(substream->pcm->card->dev, "pdm capture stop\n");

		toddr_stopped = aml_toddr_burst_finished(p_pdm->tddr);
		if (toddr_stopped)
			aml_toddr_enable(p_pdm->tddr, false);
		else
			pr_err("%s(), toddr may be stuck\n", __func__);
	break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int aml_pdm_dai_set_sysclk(struct snd_soc_dai *cpu_dai,
				int clk_id, unsigned int freq, int dir)
{
	struct aml_pdm *p_pdm = snd_soc_dai_get_drvdata(cpu_dai);
	unsigned int sysclk_srcpll_freq, dclk_srcpll_freq;
	unsigned int dclk_idx = p_pdm->dclk_idx;

	/* lowpower, force dclk to 768k */
	if (p_pdm->isLowPower)
		dclk_idx = 2;

	sysclk_srcpll_freq = clk_get_rate(p_pdm->sysclk_srcpll);
	dclk_srcpll_freq = clk_get_rate(p_pdm->dclk_srcpll);

#ifdef __PTM_PDM_CLK__
	clk_set_rate(p_pdm->clk_pdm_sysclk, 133333351);
	clk_set_rate(p_pdm->dclk_srcpll, 24576000 * 15); /* 350m */
	clk_set_rate(p_pdm->clk_pdm_dclk, 3072000);
#else
	clk_set_rate(p_pdm->clk_pdm_sysclk, 133333351);

	if (dclk_srcpll_freq == 0)
		clk_set_rate(p_pdm->dclk_srcpll, 24576000);
#endif
	clk_set_rate(p_pdm->clk_pdm_dclk,
		pdm_dclkidx2rate(dclk_idx));

	pr_info("\n%s, pdm_sysclk:%lu pdm_dclk:%lu, dclk_srcpll:%lu\n",
		__func__,
		clk_get_rate(p_pdm->clk_pdm_sysclk),
		clk_get_rate(p_pdm->clk_pdm_dclk),
		clk_get_rate(p_pdm->dclk_srcpll));

	return 0;
}

static int aml_pdm_dai_probe(struct snd_soc_dai *cpu_dai)
{
	return 0;
}

int aml_pdm_dai_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *cpu_dai)
{
	struct aml_pdm *p_pdm = snd_soc_dai_get_drvdata(cpu_dai);
	int ret;

	/* enable clock gate */
	ret = clk_prepare_enable(p_pdm->clk_gate);

	/* enable clock */
	ret = clk_prepare_enable(p_pdm->sysclk_srcpll);
	if (ret) {
		pr_err("Can't enable pcm sysclk_srcpll clock: %d\n", ret);
		goto err;
	}

	ret = clk_prepare_enable(p_pdm->dclk_srcpll);
	if (ret) {
		pr_err("Can't enable pcm dclk_srcpll clock: %d\n", ret);
		goto err;
	}

	ret = clk_prepare_enable(p_pdm->clk_pdm_sysclk);
	if (ret) {
		pr_err("Can't enable pcm clk_pdm_sysclk clock: %d\n", ret);
		goto err;
	}

	ret = clk_prepare_enable(p_pdm->clk_pdm_dclk);
	if (ret) {
		pr_err("Can't enable pcm clk_pdm_dclk clock: %d\n", ret);
		goto err;
	}
#if 0
	if (p_pdm->chipinfo && p_pdm->chipinfo->mute_fn) {
		struct snd_card *card = cpu_dai->component->card->snd_card;

		pdm_running_create_controls(card, p_pdm);
	}
#endif

	p_pdm->clk_on = true;

	return 0;
err:
	pr_err("failed enable clock\n");
	return -EINVAL;
}

void aml_pdm_dai_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *cpu_dai)
{
	struct aml_pdm *p_pdm = snd_soc_dai_get_drvdata(cpu_dai);
#if 0
	if (p_pdm->chipinfo && p_pdm->chipinfo->mute_fn) {
		struct snd_card *card = cpu_dai->component->card->snd_card;

		pdm_running_destroy_controls(card, p_pdm);
	}
#endif

	p_pdm->clk_on = false;
	p_pdm->rate = 0;

	if (p_pdm->isLowPower) {
		pdm_force_sysclk_to_oscin(false);
		vad_set_lowerpower_mode(false);
	}

	/* disable clock and gate */
	clk_disable_unprepare(p_pdm->clk_pdm_dclk);
	clk_disable_unprepare(p_pdm->clk_pdm_sysclk);
	clk_disable_unprepare(p_pdm->sysclk_srcpll);
	clk_disable_unprepare(p_pdm->dclk_srcpll);
	clk_disable_unprepare(p_pdm->clk_gate);
}

static struct snd_soc_dai_ops aml_pdm_dai_ops = {
	.set_fmt        = aml_pdm_dai_set_fmt,
	.hw_params      = aml_pdm_dai_hw_params,
	.prepare        = aml_pdm_dai_prepare,
	.trigger        = aml_pdm_dai_trigger,
	.set_sysclk     = aml_pdm_dai_set_sysclk,
	.startup = aml_pdm_dai_startup,
	.shutdown = aml_pdm_dai_shutdown,
};

struct snd_soc_dai_driver aml_pdm_dai[] = {
	{
		.name = "PDM",
		.capture = {
			.channels_min =	PDM_CHANNELS_MIN,
			.channels_max = PDM_CHANNELS_LB_MAX,
			.rates        = PDM_RATES,
			.formats      = PDM_FORMATS,
		},
		.probe            = aml_pdm_dai_probe,
		.ops     = &aml_pdm_dai_ops,
	},
};
EXPORT_SYMBOL_GPL(aml_pdm_dai);

static const struct snd_soc_component_driver aml_pdm_component = {
	.name = DRV_NAME,
	.controls = snd_pdm_controls,
	.num_controls = ARRAY_SIZE(snd_pdm_controls),
};

static int snd_soc_of_get_slot_mask(
	struct device_node *np,
	const char *prop_name,
	unsigned int *mask)
{
	u32 val;
	const __be32 *of_slot_mask = of_get_property(np, prop_name, &val);
	int i;

	if (!of_slot_mask)
		return -EINVAL;

	val /= sizeof(u32);
	for (i = 0; i < val; i++)
		if (be32_to_cpup(&of_slot_mask[i]))
			*mask |= (1 << i);

	return val;
}

static int aml_pdm_platform_probe(struct platform_device *pdev)
{
	struct aml_pdm *p_pdm;
	struct device_node *node = pdev->dev.of_node;
	struct device_node *node_prt = NULL;
	struct platform_device *pdev_parent;
	struct aml_audio_controller *actrl = NULL;
	struct device *dev = &pdev->dev;
	struct pdm_chipinfo *p_chipinfo;

	int ret;

	p_pdm = devm_kzalloc(&pdev->dev,
			sizeof(struct aml_pdm),
			GFP_KERNEL);
	if (!p_pdm) {
		/*dev_err(&pdev->dev, "Can't allocate pcm_p\n");*/
		ret = -ENOMEM;
		goto err;
	}

	/* match data */
	p_chipinfo = (struct pdm_chipinfo *)
		of_device_get_match_data(dev);
	if (!p_chipinfo)
		dev_warn_once(dev, "check whether to update pdm chipinfo\n");

	p_pdm->chipinfo = p_chipinfo;

	/* get audio controller */
	node_prt = of_get_parent(node);
	if (node_prt == NULL)
		return -ENXIO;

	pdev_parent = of_find_device_by_node(node_prt);
	of_node_put(node_prt);
	actrl = (struct aml_audio_controller *)
				platform_get_drvdata(pdev_parent);
	p_pdm->actrl = actrl;
	/* clock gate */
	p_pdm->clk_gate = devm_clk_get(&pdev->dev, "gate");
	if (IS_ERR(p_pdm->clk_gate)) {
		dev_err(&pdev->dev,
			"Can't get pdm gate\n");
		return PTR_ERR(p_pdm->clk_gate);
	}

	/* pinmux */
	p_pdm->pdm_pins = devm_pinctrl_get_select(&pdev->dev, "pdm_pins");
	if (IS_ERR(p_pdm->pdm_pins)) {
		p_pdm->pdm_pins = NULL;
		dev_err(&pdev->dev,
			"Can't get pdm pinmux\n");
		//return PTR_ERR(p_pdm->pdm_pins);
	}

	p_pdm->sysclk_srcpll = devm_clk_get(&pdev->dev, "sysclk_srcpll");
	if (IS_ERR(p_pdm->sysclk_srcpll)) {
		dev_err(&pdev->dev,
			"Can't retrieve pll clock\n");
		ret = PTR_ERR(p_pdm->sysclk_srcpll);
		goto err;
	}

	p_pdm->dclk_srcpll = devm_clk_get(&pdev->dev, "dclk_srcpll");
	if (IS_ERR(p_pdm->dclk_srcpll)) {
		dev_err(&pdev->dev,
			"Can't retrieve data src clock\n");
		ret = PTR_ERR(p_pdm->dclk_srcpll);
		goto err;
	}

	p_pdm->clk_pdm_sysclk = devm_clk_get(&pdev->dev, "pdm_sysclk");
	if (IS_ERR(p_pdm->clk_pdm_sysclk)) {
		dev_err(&pdev->dev,
			"Can't retrieve clk_pdm_sysclk clock\n");
		ret = PTR_ERR(p_pdm->clk_pdm_sysclk);
		goto err;
	}

	p_pdm->clk_pdm_dclk = devm_clk_get(&pdev->dev, "pdm_dclk");
	if (IS_ERR(p_pdm->clk_pdm_dclk)) {
		dev_err(&pdev->dev,
			"Can't retrieve clk_pdm_dclk clock\n");
		ret = PTR_ERR(p_pdm->clk_pdm_dclk);
		goto err;
	}

	ret = clk_set_parent(p_pdm->clk_pdm_sysclk, p_pdm->sysclk_srcpll);
	if (ret) {
		dev_err(&pdev->dev,
			"Can't set clk_pdm_sysclk parent clock\n");
		ret = PTR_ERR(p_pdm->clk_pdm_sysclk);
		goto err;
	}

	ret = clk_set_parent(p_pdm->clk_pdm_dclk, p_pdm->dclk_srcpll);
	if (ret) {
		dev_err(&pdev->dev,
			"Can't set clk_pdm_dclk parent clock\n");
		ret = PTR_ERR(p_pdm->clk_pdm_dclk);
		goto err;
	}

	ret = snd_soc_of_get_slot_mask(node, "lane-mask-in",
			&p_pdm->lane_mask_in);
	if (ret < 0) {
		pr_warn("default set lane_mask_in as all lanes.\n");
		p_pdm->lane_mask_in = 0xf;
	}

	ret = of_property_read_u32(node, "filter_mode",
			&p_pdm->filter_mode);
	if (ret < 0) {
		/* defulat set 1 */
		p_pdm->filter_mode = 1;
	}
	pr_info("%s pdm filter mode from dts:%d\n",
		__func__, p_pdm->filter_mode);

	ret = of_property_read_u32(node, "train_sample_count",
			&p_pdm->train_sample_count);
	if (ret < 0)
		p_pdm->train_sample_count = -1;
	pr_info("%s pdm train sample count from dts:%d\n",
		__func__, p_pdm->train_sample_count);

	s_pdm = p_pdm;

	p_pdm->dev = dev;
	dev_set_drvdata(&pdev->dev, p_pdm);

	/*config ddr arb */
	aml_pdm_arb_config(p_pdm->actrl);

	ret = snd_soc_register_component(&pdev->dev,
				&aml_pdm_component,
				aml_pdm_dai,
				ARRAY_SIZE(aml_pdm_dai));

	if (ret) {
		dev_err(&pdev->dev,
			"snd_soc_register_component failed\n");
		goto err;
	}

	pr_info("%s, register soc platform\n", __func__);

	return snd_soc_register_platform(&pdev->dev, &aml_soc_platform_pdm);

err:
	return ret;

}

static int aml_pdm_platform_remove(struct platform_device *pdev)
{
	struct aml_pdm *p_pdm = dev_get_drvdata(&pdev->dev);

	clk_disable_unprepare(p_pdm->sysclk_srcpll);
	clk_disable_unprepare(p_pdm->clk_pdm_dclk);

	snd_soc_unregister_component(&pdev->dev);

	snd_soc_unregister_platform(&pdev->dev);

	return 0;
}

static int pdm_platform_suspend(
	struct platform_device *pdev, pm_message_t state)
{
	struct aml_pdm *p_pdm = dev_get_drvdata(&pdev->dev);

	/* whether in freeze */
	if (is_pm_freeze_mode()
		&& vad_pdm_is_running()) {
		pr_info("%s, Entry in freeze\n", __func__);
		pdm_set_lowpower_mode(p_pdm, true);
	}

	return 0;
}

static int pdm_platform_resume(
	struct platform_device *pdev)
{
	struct aml_pdm *p_pdm = dev_get_drvdata(&pdev->dev);

	/* whether in freeze mode */
	if (is_pm_freeze_mode()
		&& vad_pdm_is_running()) {
		pr_info("%s, Exist from freeze\n", __func__);
		pdm_set_lowpower_mode(p_pdm, false);
	}

	return 0;
}


struct platform_driver aml_pdm_driver = {
	.driver  = {
		.name           = DRV_NAME,
		.owner          = THIS_MODULE,
		.of_match_table = of_match_ptr(aml_pdm_device_id),
	},
	.probe   = aml_pdm_platform_probe,
	.remove  = aml_pdm_platform_remove,
	.suspend = pdm_platform_suspend,
	.resume  = pdm_platform_resume,
};
module_platform_driver(aml_pdm_driver);

MODULE_AUTHOR("AMLogic, Inc.");
MODULE_DESCRIPTION("Amlogic PDM ASoc driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);
