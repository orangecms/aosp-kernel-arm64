/*
 * drivers/amlogic/drm/vpu-hw/meson_vpu_osd_mif.c
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#ifdef CONFIG_AMLOGIC_MEDIA_CANVAS
#include <linux/amlogic/media/canvas/canvas.h>
#include <linux/amlogic/media/canvas/canvas_mgr.h>
#endif
#include "meson_vpu_pipeline.h"
#include "meson_crtc.h"
#include "meson_vpu_reg.h"
#include "meson_vpu_util.h"

static struct osd_mif_reg_s osd_mif_reg[HW_OSD_MIF_NUM] = {
	{
		VIU_OSD1_CTRL_STAT,
		VIU_OSD1_CTRL_STAT2,
		VIU_OSD1_COLOR_ADDR,
		VIU_OSD1_COLOR,
		VIU_OSD1_TCOLOR_AG0,
		VIU_OSD1_TCOLOR_AG1,
		VIU_OSD1_TCOLOR_AG2,
		VIU_OSD1_TCOLOR_AG3,
		VIU_OSD1_BLK0_CFG_W0,
		VIU_OSD1_BLK0_CFG_W1,
		VIU_OSD1_BLK0_CFG_W2,
		VIU_OSD1_BLK0_CFG_W3,
		VIU_OSD1_BLK0_CFG_W4,
		VIU_OSD1_BLK1_CFG_W4,
		VIU_OSD1_BLK2_CFG_W4,
		VIU_OSD1_FIFO_CTRL_STAT,
		VIU_OSD1_TEST_RDDATA,
		VIU_OSD1_PROT_CTRL,
		VIU_OSD1_MALI_UNPACK_CTRL,
		VIU_OSD1_DIMM_CTRL,
	},
	{
		VIU_OSD2_CTRL_STAT,
		VIU_OSD2_CTRL_STAT2,
		VIU_OSD2_COLOR_ADDR,
		VIU_OSD2_COLOR,
		VIU_OSD2_TCOLOR_AG0,
		VIU_OSD2_TCOLOR_AG1,
		VIU_OSD2_TCOLOR_AG2,
		VIU_OSD2_TCOLOR_AG3,
		VIU_OSD2_BLK0_CFG_W0,
		VIU_OSD2_BLK0_CFG_W1,
		VIU_OSD2_BLK0_CFG_W2,
		VIU_OSD2_BLK0_CFG_W3,
		VIU_OSD2_BLK0_CFG_W4,
		VIU_OSD2_BLK1_CFG_W4,
		VIU_OSD2_BLK2_CFG_W4,
		VIU_OSD2_FIFO_CTRL_STAT,
		VIU_OSD2_TEST_RDDATA,
		VIU_OSD2_PROT_CTRL,
		VIU_OSD2_MALI_UNPACK_CTRL,
		VIU_OSD2_DIMM_CTRL,
	},
	{
		VIU_OSD3_CTRL_STAT,
		VIU_OSD3_CTRL_STAT2,
		VIU_OSD3_COLOR_ADDR,
		VIU_OSD3_COLOR,
		VIU_OSD3_TCOLOR_AG0,
		VIU_OSD3_TCOLOR_AG1,
		VIU_OSD3_TCOLOR_AG2,
		VIU_OSD3_TCOLOR_AG3,
		VIU_OSD3_BLK0_CFG_W0,
		VIU_OSD3_BLK0_CFG_W1,
		VIU_OSD3_BLK0_CFG_W2,
		VIU_OSD3_BLK0_CFG_W3,
		VIU_OSD3_BLK0_CFG_W4,
		VIU_OSD3_BLK1_CFG_W4,
		VIU_OSD3_BLK2_CFG_W4,
		VIU_OSD3_FIFO_CTRL_STAT,
		VIU_OSD3_TEST_RDDATA,
		VIU_OSD3_PROT_CTRL,
		VIU_OSD3_MALI_UNPACK_CTRL,
		VIU_OSD3_DIMM_CTRL,
	}
};

static unsigned int osd_canvas[3][2] = {
	{0x41, 0x42}, {0x43, 0x44}, {0x45, 0x46} };
static u32 osd_canvas_index[3] = {0, 0, 0};

/*
 * Internal function to query information for a given format. See
 * meson_drm_format_info() for the public API.
 */
const struct meson_drm_format_info *__meson_drm_format_info(u32 format)
{
	static const struct meson_drm_format_info formats[] = {
		{ .format = DRM_FORMAT_XRGB8888,
			.hw_blkmode = BLOCK_MODE_32BIT,
			.hw_colormat = COLOR_MATRIX_ARGB8888,
			.alpha_replace = 1 },
		{ .format = DRM_FORMAT_XBGR8888,
			.hw_blkmode = BLOCK_MODE_32BIT,
			.hw_colormat = COLOR_MATRIX_ABGR8888,
			.alpha_replace = 1 },
		{ .format = DRM_FORMAT_RGBX8888,
			.hw_blkmode = BLOCK_MODE_32BIT,
			.hw_colormat = COLOR_MATRIX_RGBA8888,
			.alpha_replace = 1 },
		{ .format = DRM_FORMAT_BGRX8888,
			.hw_blkmode = BLOCK_MODE_32BIT,
			.hw_colormat = COLOR_MATRIX_BGRA8888,
			.alpha_replace = 1 },
		{ .format = DRM_FORMAT_ARGB8888,
			.hw_blkmode = BLOCK_MODE_32BIT,
			.hw_colormat = COLOR_MATRIX_ARGB8888,
			.alpha_replace = 0 },
		{ .format = DRM_FORMAT_ABGR8888,
			.hw_blkmode = BLOCK_MODE_32BIT,
			.hw_colormat = COLOR_MATRIX_ABGR8888,
			.alpha_replace = 0 },
		{ .format = DRM_FORMAT_RGBA8888,
			.hw_blkmode = BLOCK_MODE_32BIT,
			.hw_colormat = COLOR_MATRIX_RGBA8888,
			.alpha_replace = 0 },
		{ .format = DRM_FORMAT_BGRA8888,
			.hw_blkmode = BLOCK_MODE_32BIT,
			.hw_colormat = COLOR_MATRIX_BGRA8888,
			.alpha_replace = 0 },
		{ .format = DRM_FORMAT_RGB888,
			.hw_blkmode = BLOCK_MODE_24BIT,
			.hw_colormat = COLOR_MATRIX_RGB888,
			.alpha_replace = 0 },
		{ .format = DRM_FORMAT_RGB565,
			.hw_blkmode = BLOCK_MODE_16BIT,
			.hw_colormat = COLOR_MATRIX_565,
			.alpha_replace = 0 },
	};

	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(formats); ++i) {
		if (formats[i].format == format)
			return &formats[i];
	}

	return NULL;
}

const struct meson_drm_format_info *__meson_drm_afbc_format_info(u32 format)
{
	static const struct meson_drm_format_info formats[] = {
		{ .format = DRM_FORMAT_XRGB8888,
			.hw_blkmode = BLOCK_MODE_RGBA8888,
			.alpha_replace = 1 },
		{ .format = DRM_FORMAT_XBGR8888,
			.hw_blkmode = BLOCK_MODE_RGBA8888,
			.alpha_replace = 1 },
		{ .format = DRM_FORMAT_RGBX8888,
			.hw_blkmode = BLOCK_MODE_RGBA8888,
			.alpha_replace = 1 },
		{ .format = DRM_FORMAT_BGRX8888,
			.hw_blkmode = BLOCK_MODE_RGBA8888,
			.alpha_replace = 1 },
		{ .format = DRM_FORMAT_ARGB8888,
			.hw_blkmode = BLOCK_MODE_RGBA8888,
			.alpha_replace = 0 },
		{ .format = DRM_FORMAT_ABGR8888,
			.hw_blkmode = BLOCK_MODE_RGBA8888,
			.alpha_replace = 0 },
		{ .format = DRM_FORMAT_RGBA8888,
			.hw_blkmode = BLOCK_MODE_RGBA8888,
			.alpha_replace = 0 },
		{ .format = DRM_FORMAT_BGRA8888,
			.hw_blkmode = BLOCK_MODE_RGBA8888,
			.alpha_replace = 0 },
		{ .format = DRM_FORMAT_RGB888,
			.hw_blkmode = BLOCK_MODE_RGB888,
			.alpha_replace = 0 },
		{ .format = DRM_FORMAT_RGB565,
			.hw_blkmode = BLOCK_MODE_RGB565,
			.alpha_replace = 0 },
		{ .format = DRM_FORMAT_RGBA1010102,
			.hw_blkmode = BLOCK_MODE_RGBA1010102,
			.alpha_replace = 0 },
	};

	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(formats); ++i) {
		if (formats[i].format == format)
			return &formats[i];
	}

	return NULL;
}

/**
 * meson_drm_format_info - query information for a given format
 * @format: pixel format (DRM_FORMAT_*)
 *
 * The caller should only pass a supported pixel format to this function.
 * Unsupported pixel formats will generate a warning in the kernel log.
 *
 * Returns:
 * The instance of struct meson_drm_format_info that describes the
 * pixel format, or NULL if the format is unsupported.
 */
const struct meson_drm_format_info *meson_drm_format_info(u32 format,
							  bool afbc_en)
{
	const struct meson_drm_format_info *info;

	if (afbc_en)
		info = __meson_drm_afbc_format_info(format);
	else
		info = __meson_drm_format_info(format);
	WARN_ON(!info);
	return info;
}

/**
 * meson_drm_format_hw_blkmode - get the hw_blkmode for format
 * @format: pixel format (DRM_FORMAT_*)
 *
 * Returns:
 * The hw_blkmode match the specified pixel format.
 */
static u8 meson_drm_format_hw_blkmode(u32 format, bool afbc_en)
{
	const struct meson_drm_format_info *info;

	info = meson_drm_format_info(format, afbc_en);
	return info ? info->hw_blkmode : 0;
}

/**
 * meson_drm_format_hw_colormat - get the hw_colormat for format
 * @format: pixel format (DRM_FORMAT_*)
 *
 * Returns:
 * The hw_colormat match the specified pixel format.
 */
static u8 meson_drm_format_hw_colormat(u32 format, bool afbc_en)
{
	const struct meson_drm_format_info *info;

	info = meson_drm_format_info(format, afbc_en);
	return info ? info->hw_colormat : 0;
}

/**
 * meson_drm_format_alpha_replace - get the alpha replace for format
 * @format: pixel format (DRM_FORMAT_*)
 *
 * Returns:
 * The alpha_replace match the specified pixel format.
 */
static u8 meson_drm_format_alpha_replace(u32 format, bool afbc_en)
{
	const struct meson_drm_format_info *info;

	info = meson_drm_format_info(format, afbc_en);
	return info ? info->alpha_replace : 0;
}

/*osd input size config*/
void osd_input_size_config(struct osd_mif_reg_s *reg, struct osd_scope_s scope)
{
	meson_vpu_write_reg(reg->viu_osd_blk0_cfg_w1,
			    (scope.h_end << 16) | /*x_end pixels[13bits]*/
		scope.h_start/*x_start pixels[13bits]*/);
	meson_vpu_write_reg(reg->viu_osd_blk0_cfg_w2,
			    (scope.v_end << 16) | /*y_end pixels[13bits]*/
		scope.v_start/*y_start pixels[13bits]*/);
}

/*osd canvas config*/
void osd_canvas_config(struct osd_mif_reg_s *reg, u32 canvas_index)
{
	meson_vpu_write_reg_bits(reg->viu_osd_blk0_cfg_w0,
				 canvas_index, 16, 8);
}

/*osd mali afbc src en
 * 1: read data from mali afbcd;0: read data from DDR directly
 */
void osd_mali_src_en(struct osd_mif_reg_s *reg, u8 osd_index, bool flag)
{
	meson_vpu_write_reg_bits(reg->viu_osd_blk0_cfg_w0, flag, 30, 1);
	meson_vpu_write_reg_bits(OSD_PATH_MISC_CTRL, flag, (osd_index + 4), 1);
}

/*osd endian mode
 * 1: little endian;0: big endian[for mali afbc input]
 */
void osd_endian_mode(struct osd_mif_reg_s *reg, bool flag)
{
	meson_vpu_write_reg_bits(reg->viu_osd_blk0_cfg_w0, flag, 15, 1);
}

/*osd mif enable*/
void osd_block_enable(struct osd_mif_reg_s *reg, bool flag)
{
	meson_vpu_write_reg_bits(reg->viu_osd_ctrl_stat, flag, 0, 1);
}

/*osd mem mode
 * 0: canvas_addr;1:linear_addr[for mali-afbc-mode]
 */
void osd_mem_mode(struct osd_mif_reg_s *reg, bool mode)
{
	meson_vpu_write_reg_bits(reg->viu_osd_ctrl_stat, mode, 2, 1);
}

/*osd alpha_div en
 *if input is premult,alpha_div=1,else alpha_div=0
 */
void osd_alpha_div_enable(struct osd_mif_reg_s *reg, bool flag)
{
	meson_vpu_write_reg_bits(reg->viu_osd_mali_unpack_ctrl, flag, 28, 1);
}

/*osd x reverse en
 *reverse read in X direction
 */
void osd_reverse_x_enable(struct osd_mif_reg_s *reg, bool flag)
{
	meson_vpu_write_reg_bits(reg->viu_osd_blk0_cfg_w0, flag, 28, 1);
}

/*osd y reverse en
 *reverse read in Y direction
 */
void osd_reverse_y_enable(struct osd_mif_reg_s *reg, bool flag)
{
	meson_vpu_write_reg_bits(reg->viu_osd_blk0_cfg_w0, flag, 29, 1);
}

/*osd mali unpack en
 * 1: osd will unpack mali_afbc_src;0:osd will unpack normal src
 */
void osd_mali_unpack_enable(struct osd_mif_reg_s *reg, bool flag)
{
	meson_vpu_write_reg_bits(reg->viu_osd_mali_unpack_ctrl, flag, 31, 1);
}

static void osd_fifo_hold_line_config(struct osd_mif_reg_s *reg,
				      u32 hold_line)
{
	meson_vpu_write_reg_bits(reg->viu_osd_fifo_ctrl_stat,
				 hold_line & 0x1f, 5, 5);
}

/*osd ctrl config*/
void osd_ctrl_set(struct osd_mif_reg_s *reg)
{
	meson_vpu_write_reg(reg->viu_osd_ctrl_stat,
			    (0 << 31) | /*osd_cfg_sync_en*/
			     (0 << 30) | /*Enable free_clk*/
			     (0x100 << 12) | /*global alpha*/
			     (0 << 11) | /*TEST_RD_EN*/
			     (0 << 2) | /*osd_mem_mode 0:canvas_addr*/
			     (0 << 1) | /*premult_en*/
			     (0 << 0)/*OSD_BLK_ENABLE*/);
	meson_vpu_write_reg(reg->viu_osd_ctrl_stat2,
			    (1 << 14) | /*replaced_alpha_en*/
			     (0xff << 6) | /*replaced_alpha*/
			     (0 << 4) | /*hold fifo lines 2bit*/
			     (0 << 3) | /*output fullrange enable 1bit*/
			     (0 << 2)/*alpha 9bit mode 1bit*/);
	meson_vpu_write_reg(reg->viu_osd_tcolor_ag0,
			    (0xff << 24) | /*Y/R*/
			     (0xff << 16) | /*CB/G*/
			     (0xff << 24) | /*CR/B*/
			     (0xff << 24)/*ALPHA*/);
	meson_vpu_write_reg(reg->viu_osd_tcolor_ag1,
			    (0xff << 24) | /*Y/R*/
			     (0xff << 16) | /*CB/G*/
			     (0xff << 24) | /*CR/B*/
			     (0xff << 24)/*ALPHA*/);
	meson_vpu_write_reg(reg->viu_osd_tcolor_ag2,
			    (0xff << 24) | /*Y/R*/
			     (0xff << 16) | /*CB/G*/
			     (0xff << 24) | /*CR/B*/
			     (0xff << 24)/*ALPHA*/);
	meson_vpu_write_reg(reg->viu_osd_tcolor_ag3,
			    (0xff << 24) | /*Y/R*/
			     (0xff << 16) | /*CB/G*/
			     (0xff << 24) | /*CR/B*/
			     (0xff << 24)/*ALPHA*/);
	meson_vpu_write_reg(reg->viu_osd_blk0_cfg_w0,
			    (0 << 30) | /*read from ddr[0]/afbc[1]*/
			     (0 << 29) | /*y reverse disable*/
			     (0 << 28) | /*x reverse disable*/
			     (osd_canvas[0][0] << 16) | /*canvas index*/
			     (1 << 15) | /*little endian in ddr*/
			     (0 << 14) | /*no repeat display y pre line*/
			     (0 << 12) | /*no interpolation per pixel*/
			     (5 << 8) | /*read from ddr 32bit mode*/
			     (0 << 6) | /*TC_ALPHA_EN*/
			     (1 << 2) | /*ARGB format for 32bit mode*/
			     (0 << 1) | /*interlace en*/
			     (0 << 0)/*output odd/even lines sel*/);
	meson_vpu_write_reg(reg->viu_osd_blk0_cfg_w1,
			    (1919 << 16) | /*x_end pixels[13bits]*/
			     (0 << 0)/*x_start pixels[13bits]*/);
	meson_vpu_write_reg(reg->viu_osd_blk0_cfg_w2,
			    (1079 << 16) | /*y_end pixels[13bits]*/
			     (0 << 0)/*y_start pixels[13bits]*/);
	/*frame addr in linear addr*/
	meson_vpu_write_reg(reg->viu_osd_blk1_cfg_w4, 0);
	/*line_stride in linear addr*/
	meson_vpu_write_reg(reg->viu_osd_blk2_cfg_w4, 0);
	meson_vpu_write_reg(reg->viu_osd_fifo_ctrl_stat,
			    (1 << 31) | /*BURSET_LEN_SEL[2]*/
			     (0 << 30) | /*no swap*/
			     (0 << 29) | /*div swap*/
			     (2 << 24) | /*Fifo_lim 5bits*/
			     (2 << 22) | /*Fifo_ctrl 2bits*/
			     (0x20 << 12) | /*FIFO_DEPATH_VAL 7bits*/
			     (1 << 10) | /*BURSET_LEN_SEL[1:0]*/
			     (4 << 5) | /*hold fifo lines 5bits*/
			     (0 << 4) | /*CLEAR_ERR*/
			     (0 << 3) | /*fifo_sync_rst*/
			     (0 << 1) | /*ENDIAN:no conversion*/
			     (1 << 0)/*urgent enable*/);
	meson_vpu_write_reg(reg->viu_osd_mali_unpack_ctrl,
			    (0 << 31) | /*unpack normal src*/
			     (0 << 28) | /*alpha div en*/
			     (0 << 26) | /*dividor gating clk*/
			     (1 << 24) | /*alpha mapping mode 2bits*/
			     (0 << 16) | /*afbc swap 64bit 1bits*/
			     (1 << 12) | /*afbcd_r_reorder r 4bits*/
			     (2 << 8) | /*afbcd_r_reorder g 4bits*/
			     (3 << 4) | /*afbcd_r_reorder b 4bits*/
			     (4 << 0)/*afbcd_r_reorder alpha 4bits*/);
}

static void osd_color_config(struct osd_mif_reg_s *reg,
			     u32 pixel_format, bool afbc_en)
{
	u8 blk_mode, colormat, alpha_replace;

	blk_mode = meson_drm_format_hw_blkmode(pixel_format, afbc_en);
	colormat = meson_drm_format_hw_colormat(pixel_format, afbc_en);
	alpha_replace = meson_drm_format_alpha_replace(pixel_format, afbc_en);
	meson_vpu_write_reg_bits(reg->viu_osd_blk0_cfg_w0,
				 blk_mode, 8, 4);
	meson_vpu_write_reg_bits(reg->viu_osd_blk0_cfg_w0,
				 colormat, 2, 4);
	meson_vpu_write_reg_bits(reg->viu_osd_ctrl_stat2,
				 alpha_replace, 14, 1);
}

static void osd_afbc_config(struct osd_mif_reg_s *reg,
			    u8 osd_index, bool afbc_en)
{
	osd_mali_unpack_enable(reg, afbc_en);
	osd_mali_src_en(reg, osd_index, afbc_en);
	osd_endian_mode(reg, !afbc_en);
	osd_mem_mode(reg, afbc_en);
}

u8 *meson_drm_vmap(ulong addr, u32 size, bool *bflg)
{
	u8 *vaddr = NULL;
	ulong phys = addr;
	u32 offset = phys & ~PAGE_MASK;
	u32 npages = PAGE_ALIGN(size) / PAGE_SIZE;
	struct page **pages = NULL;
	pgprot_t pgprot;
	int i;

	if (!PageHighMem(phys_to_page(phys)))
		return phys_to_virt(phys);

	if (offset)
		npages++;

	pages = vmalloc(sizeof(struct page *) * npages);
	if (!pages)
		return NULL;

	for (i = 0; i < npages; i++) {
		pages[i] = phys_to_page(phys);
		phys += PAGE_SIZE;
	}

	/*nocache*/
	pgprot = pgprot_writecombine(PAGE_KERNEL);

	vaddr = vmap(pages, npages, VM_MAP, pgprot);
	if (!vaddr) {
		pr_err("the phy(%lx) vmaped fail, size: %d\n",
		       addr - offset, npages << PAGE_SHIFT);
		vfree(pages);
		return NULL;
	}

	vfree(pages);

	DRM_DEBUG("map high mem pa(%lx) to va(%p), size: %d\n",
		  addr, vaddr + offset, npages << PAGE_SHIFT);
	*bflg = true;

	return vaddr + offset;
}

void meson_drm_unmap_phyaddr(u8 *vaddr)
{
	void *addr = (void *)(PAGE_MASK & (ulong)vaddr);

	vunmap(addr);
}

static int osd_check_state(struct meson_vpu_block *vblk,
			   struct meson_vpu_block_state *state,
		struct meson_vpu_pipeline_state *mvps)
{
	struct meson_vpu_osd_layer_info *plane_info;
	struct meson_vpu_osd *osd = to_osd_block(vblk);
	struct meson_vpu_osd_state *mvos = to_osd_state(state);

	if (state->checked)
		return 0;

	state->checked = true;

	if (!mvos || mvos->plane_index >= MESON_MAX_OSDS) {
		DRM_INFO("mvos is NULL!\n");
		return -1;
	}
	DRM_DEBUG("%s check_state called.\n", osd->base.name);
	plane_info = &mvps->plane_info[vblk->index];
	mvos->src_x = plane_info->src_x;
	mvos->src_y = plane_info->src_y;
	mvos->src_w = plane_info->src_w;
	mvos->src_h = plane_info->src_h;
	mvos->byte_stride = plane_info->byte_stride;
	mvos->phy_addr = plane_info->phy_addr;
	mvos->pixel_format = plane_info->pixel_format;
	mvos->fb_size = plane_info->fb_size;
	mvos->premult_en = plane_info->premult_en;
	mvos->rotation = plane_info->rotation;
	mvos->afbc_en = plane_info->afbc_en;
	mvos->blend_bypass = plane_info->blend_bypass;
	return 0;
}

static void osd_set_state(struct meson_vpu_block *vblk,
			  struct meson_vpu_block_state *state)
{
	struct file *fp;
	mm_segment_t fs;
	loff_t pos;
	char name_buf[64];
	struct drm_crtc *crtc;
	struct am_meson_crtc *amc;
	struct meson_vpu_osd *osd = to_osd_block(vblk);
	struct meson_vpu_osd_state *mvos = to_osd_state(state);
	u32 pixel_format, canvas_index, src_h, byte_stride;
	struct osd_scope_s scope_src = {0, 1919, 0, 1079};
	struct osd_mif_reg_s *reg = osd->reg;
	bool alpha_div_en, reverse_x, reverse_y, afbc_en;
	bool bflg = false;
	void *buff = NULL;
	u64 phy_addr;
	u32 hold_line;

	crtc = vblk->pipeline->crtc;
	amc = to_am_meson_crtc(crtc);

	if (!vblk) {
		DRM_DEBUG("set_state break for NULL.\n");
		return;
	}
	alpha_div_en = (mvos->premult_en && !mvos->blend_bypass) ? 1 : 0;
	afbc_en = mvos->afbc_en ? 1 : 0;
	src_h = mvos->src_h;
	byte_stride = mvos->byte_stride;
	phy_addr = mvos->phy_addr;
	scope_src.h_start = mvos->src_x;
	scope_src.h_end = mvos->src_x + mvos->src_w - 1;
	scope_src.v_start = mvos->src_y;
	scope_src.v_end = mvos->src_y + mvos->src_h - 1;
	pixel_format = mvos->pixel_format;
	canvas_index = osd_canvas[vblk->index][osd_canvas_index[vblk->index]];
	/*Toto: need to separate*/
	if (0)
		osd_ctrl_set(osd->reg);
	reverse_x = (mvos->rotation & DRM_REFLECT_X) ? 1 : 0;
	reverse_y = (mvos->rotation & DRM_REFLECT_Y) ? 1 : 0;
	osd_reverse_x_enable(reg, reverse_x);
	osd_reverse_y_enable(reg, reverse_y);
	canvas_config(canvas_index, phy_addr, byte_stride, src_h,
		      CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
	osd_canvas_index[vblk->index] ^= 1;
	osd_canvas_config(reg, canvas_index);
	osd_input_size_config(reg, scope_src);
	osd_color_config(reg, pixel_format, afbc_en);
	osd_afbc_config(reg, vblk->index, afbc_en);
	osd_alpha_div_enable(reg, alpha_div_en);
	if (crtc->index == 0)
		hold_line = VIU1_DEFAULT_HOLD_LINE;
	else
		hold_line = VIU2_DEFAULT_HOLD_LINE;
	osd_fifo_hold_line_config(reg, hold_line);
	DRM_DEBUG("plane_index=%d,HW-OSD=%d\n",
		  mvos->plane_index, vblk->index);
	DRM_DEBUG("canvas_index[%d]=0x%x,phy_addr=0x%pa\n",
		  osd_canvas_index[vblk->index], canvas_index, &phy_addr);
	DRM_DEBUG("scope h/v start/end:[%d/%d/%d/%d]\n",
		  scope_src.h_start, scope_src.h_end,
		scope_src.v_start, scope_src.v_end);
	DRM_DEBUG("%s set_state done.\n", osd->base.name);

	if (amc->dump_enable) {
		DRM_DEBUG("start to dump gem buff %d.\n", amc->dump_index);
		memset(name_buf, 0, sizeof(name_buf));
		amc->dump_index %= amc->dump_counts;
		snprintf(name_buf, sizeof(name_buf), "%s/plane%d.dump.%d",
			 amc->osddump_path, mvos->plane_index,
					amc->dump_index++);

		if (amc->dump_index >= amc->dump_counts)
			amc->dump_index = 0;

		fs = get_fs();
		set_fs(KERNEL_DS);
		pos = 0;
		fp = filp_open(name_buf, O_CREAT | O_RDWR, 0644);
		if (IS_ERR(fp)) {
			DRM_ERROR("create %s osd_dump fail.\n", name_buf);
		} else {
			buff = meson_drm_vmap(phy_addr, mvos->fb_size, &bflg);
			vfs_write(fp, buff, mvos->fb_size, &pos);
			filp_close(fp, NULL);
		}
		set_fs(fs);
		DRM_DEBUG("low_mem: %d.\n", bflg);
		if (bflg)
			meson_drm_unmap_phyaddr(buff);
	}
}

static void osd_hw_enable(struct meson_vpu_block *vblk)
{
	struct meson_vpu_osd *osd = to_osd_block(vblk);
	struct osd_mif_reg_s *reg = osd->reg;

	if (!vblk) {
		DRM_DEBUG("enable break for NULL.\n");
		return;
	}
	osd_block_enable(reg, 1);
	DRM_DEBUG("%s enable done.\n", osd->base.name);
}

static void osd_hw_disable(struct meson_vpu_block *vblk)
{
	struct meson_vpu_osd *osd = to_osd_block(vblk);
	struct osd_mif_reg_s *reg = osd->reg;
	u8 version = vblk->pipeline->osd_version;

	if (!vblk) {
		DRM_DEBUG("disable break for NULL.\n");
		return;
	}
	/*G12B should always enable,avoid afbc decoder error*/
	if ((version != OSD_V2) && (version != OSD_V3))
		osd_block_enable(reg, 0);
	DRM_DEBUG("%s disable done.\n", osd->base.name);
}

static void osd_dump_register(struct meson_vpu_block *vblk,
			      struct seq_file *seq)
{
	int osd_index;
	u32 value;
	char buff[8];
	struct meson_vpu_osd *osd;
	struct osd_mif_reg_s *reg;

	osd_index = vblk->index;
	osd = to_osd_block(vblk);
	reg = osd->reg;

	snprintf(buff, 8, "OSD%d", osd_index + 1);

	value = meson_drm_read_reg(reg->viu_osd_fifo_ctrl_stat);
	seq_printf(seq, "%s_%-35s\t0x%08X\n", buff, "FIFO_CTRL_STAT:", value);

	value = meson_drm_read_reg(reg->viu_osd_ctrl_stat);
	seq_printf(seq, "%s_%-35s\t0x%08X\n", buff, "CTRL_STAT:", value);

	value = meson_drm_read_reg(reg->viu_osd_ctrl_stat2);
	seq_printf(seq, "%s_%-35s\t0x%08X\n", buff, "CTRL_STAT2:", value);

	value = meson_drm_read_reg(reg->viu_osd_blk0_cfg_w0);
	seq_printf(seq, "%s_%-35s\t0x%08X\n", buff, "BLK0_CFG_W0:", value);

	value = meson_drm_read_reg(reg->viu_osd_blk0_cfg_w1);
	seq_printf(seq, "%s_%-35s\t0x%08X\n", buff, "BLK0_CFG_W1:", value);

	value = meson_drm_read_reg(reg->viu_osd_blk0_cfg_w2);
	seq_printf(seq, "%s_%-35s\t0x%08X\n", buff, "BLK0_CFG_W2:", value);

	value = meson_drm_read_reg(reg->viu_osd_blk0_cfg_w3);
	seq_printf(seq, "%s_%-35s\t0x%08X\n", buff, "BLK0_CFG_W3:", value);

	value = meson_drm_read_reg(reg->viu_osd_blk0_cfg_w4);
	seq_printf(seq, "%s_%-35s\t0x%08X\n", buff, "BLK0_CFG_W4:", value);

	value = meson_drm_read_reg(reg->viu_osd_blk1_cfg_w4);
	seq_printf(seq, "%s_%-35s\t0x%08X\n", buff, "BLK1_CFG_W4:", value);

	value = meson_drm_read_reg(reg->viu_osd_blk2_cfg_w4);
	seq_printf(seq, "%s_%-35s\t0x%08X\n", buff, "BLK2_CFG_W4:", value);

	value = meson_drm_read_reg(reg->viu_osd_prot_ctrl);
	seq_printf(seq, "%s_%-35s\t0x%08X\n", buff, "PROT_CTRL:", value);

	value = meson_drm_read_reg(reg->viu_osd_mali_unpack_ctrl);
	seq_printf(seq, "%s_%-35s\t0x%08X\n", buff, "MALI_UNPACK_CTRL:", value);

	value = meson_drm_read_reg(reg->viu_osd_dimm_ctrl);
	seq_printf(seq, "%s_%-35s\t0x%08X\n", buff, "DIMM_CTRL:", value);
}

static void osd_hw_init(struct meson_vpu_block *vblk)
{
	struct meson_vpu_osd *osd = to_osd_block(vblk);

	if (!vblk || !osd) {
		DRM_DEBUG("hw_init break for NULL.\n");
		return;
	}
	osd->reg = &osd_mif_reg[vblk->index];
	DRM_DEBUG("%s hw_init done.\n", osd->base.name);
}

struct meson_vpu_block_ops osd_ops = {
	.check_state = osd_check_state,
	.update_state = osd_set_state,
	.enable = osd_hw_enable,
	.disable = osd_hw_disable,
	.dump_register = osd_dump_register,
	.init = osd_hw_init,
};
