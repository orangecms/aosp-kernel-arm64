/*
 * drivers/amlogic/media/di_multi_v3/nr_downscale.c
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
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/dma-contiguous.h>
#include <linux/amlogic/iomap.h>
#include <linux/amlogic/media/canvas/canvas.h>
#include <linux/amlogic/media/canvas/canvas_mgr.h>
#include "register.h"
#include "nr_downscale.h"
#include "deinterlace.h"

#include "di_data_l.h"
#include "di_sys.h"
#include "di_api.h"

static struct nr_ds_s nrds_dev;

static void nr_ds_hw_init(unsigned int width, unsigned int height)
{
	unsigned char h_step = 0, v_step = 0;
	unsigned int width_out, height_out;

	width_out = NR_DS_WIDTH;
	height_out = NR_DS_HEIGHT;

	h_step = width / width_out;
	v_step = height / height_out;

	/*Switch MIF to NR_DS*/
	dimv3_RDMA_WR_BITS(VIUB_MISC_CTRL0, 3, 5, 2);
	/* config dsbuf_ocol*/
	dimv3_RDMA_WR_BITS(NR_DS_BUF_SIZE_REG, width_out, 0, 8);
	/* config dsbuf_orow*/
	dimv3_RDMA_WR_BITS(NR_DS_BUF_SIZE_REG, height_out, 8, 8);

	dimv3_RDMA_WR_BITS(NRDSWR_X, (width_out - 1), 0, 13);
	dimv3_RDMA_WR_BITS(NRDSWR_Y, (height_out - 1), 0, 13);

	dimv3_RDMA_WR_BITS(NRDSWR_CAN_SIZE, (height_out - 1), 0, 13);
	dimv3_RDMA_WR_BITS(NRDSWR_CAN_SIZE, (width_out - 1), 16, 13);
	/* little endian */
	dimv3_RDMA_WR_BITS(NRDSWR_CAN_SIZE, 1, 13, 1);

	dimv3_RDMA_WR_BITS(NR_DS_CTRL, v_step, 16, 6);
	dimv3_RDMA_WR_BITS(NR_DS_CTRL, h_step, 24, 6);
}

/*
 * init nr ds buffer
 */
void dimv3_nr_ds_buf_init(unsigned int cma_flag, unsigned long mem_start,
			  struct device *dev)
{
	unsigned int i = 0;
	bool ret;
	struct dim_mm_s omm;

	if (cma_flag == 0) {
		nrds_dev.nrds_addr = mem_start;
	} else {
		#ifdef HIS_V3
		nrds_dev.nrds_pages = dma_alloc_from_contiguous(dev,
			NR_DS_PAGE_NUM, 0);
		if (nrds_dev.nrds_pages)
			nrds_dev.nrds_addr = page_to_phys(nrds_dev.nrds_pages);
		else
			PR_ERR("DI: alloc nr ds mem error.\n");
		#else
		ret = dimv3_mm_alloc_api(cma_flag, NR_DS_PAGE_NUM, &omm);
		if (ret) {
			nrds_dev.nrds_pages = omm.ppage;
			nrds_dev.nrds_addr = omm.addr;
		} else {
			PR_ERR("alloc nr ds mem error.\n");
		}

		#endif
	}
	for (i = 0; i < NR_DS_BUF_NUM; i++)
		nrds_dev.buf[i] = nrds_dev.nrds_addr + (NR_DS_BUF_SIZE * i);
	nrds_dev.cur_buf_idx = 0;
}

void dimv3_nr_ds_buf_uninit(unsigned int cma_flag, struct device *dev)
{
	unsigned int i = 0;

	if (cma_flag == 0) {
		nrds_dev.nrds_addr = 0;
	} else {
		if (nrds_dev.nrds_pages) {
			#ifdef HIS_V3
			dma_release_from_contiguous(dev,
						    nrds_dev.nrds_pages,
						    NR_DS_PAGE_NUM);
			#else
			dimv3_mm_release_api(cma_flag,
					     nrds_dev.nrds_pages,
					     NR_DS_PAGE_NUM,
					     nrds_dev.nrds_addr);
			#endif
			nrds_dev.nrds_addr = 0;
			nrds_dev.nrds_pages = NULL;
		} else
			PR_INF("no release nr ds mem.\n");
	}
	for (i = 0; i < NR_DS_BUF_NUM; i++)
		nrds_dev.buf[i] = 0;
	nrds_dev.cur_buf_idx = 0;
}

/*
 * hw config, alloc canvas
 */
void dimv3_nr_ds_init(unsigned int width, unsigned int height)
{
	nr_ds_hw_init(width, height);
	nrds_dev.field_num = 0;

	if (nrds_dev.canvas_idx != 0)
		return;

	if (extv3_ops.cvs_alloc_table("nr_ds",
				      &nrds_dev.canvas_idx, 1,
				      CANVAS_MAP_TYPE_1)) {
		PR_ERR("%s alloc nrds canvas error.\n", __func__);
		return;
	}
	PR_INF("%s alloc nrds canvas %u.\n", __func__, nrds_dev.canvas_idx);
}

/*
 * config nr ds mif, switch buffer
 */
void dimv3_nr_ds_mif_config(void)
{
	unsigned long mem_addr = 0;

	mem_addr = nrds_dev.buf[nrds_dev.cur_buf_idx];
	canvas_config(nrds_dev.canvas_idx, mem_addr,
		      NR_DS_WIDTH, NR_DS_HEIGHT, 0, 0);
	dimv3_RDMA_WR_BITS(NRDSWR_CTRL,
			   nrds_dev.canvas_idx, 0, 8);
	dimv3_nr_ds_hw_ctrl(true);
}

/*
 * enable/disable nr ds mif&hw
 */
void dimv3_nr_ds_hw_ctrl(bool enable)
{
	/*Switch MIF to NR_DS*/
	dimv3_RDMA_WR_BITS(VIUB_MISC_CTRL0, enable ? 3 : 2, 5, 2);
	dimv3_RDMA_WR_BITS(NRDSWR_CTRL, enable ? 1 : 0, 12, 1);
	dimv3_RDMA_WR_BITS(NR_DS_CTRL, enable ? 1 : 0, 30, 1);
}

/*
 * process in irq
 */
void dimv3_nr_ds_irq(void)
{
	dimv3_nr_ds_hw_ctrl(false);
	nrds_dev.field_num++;
	nrds_dev.cur_buf_idx++;
	if (nrds_dev.cur_buf_idx >= NR_DS_BUF_NUM)
		nrds_dev.cur_buf_idx = 0;
}

/*
 * get buf addr&size for dump
 */
void dimv3_get_nr_ds_buf(unsigned long *addr, unsigned long *size)
{
	*addr = nrds_dev.nrds_addr;
	*size =	NR_DS_BUF_SIZE;
	PR_INF("%s addr 0x%lx, size 0x%lx.\n", __func__, *addr, *size);
}

/*
 * 0x37f9 ~ 0x37fc 0x3740 ~ 0x3743 8 regs
 */
void dimv3_dump_nrds_reg(unsigned int base_addr)
{
	unsigned int i = 0x37f9;

	pr_info("-----nrds reg start-----\n");
	pr_info("[0x%x][0x%x]=0x%x\n",
		base_addr + (0x2006 << 2), i, dimv3_RDMA_RD(0x2006));
	for (i = 0x37f9; i < 0x37fd; i++)
		pr_info("[0x%x][0x%x]=0x%x\n",
			base_addr + (i << 2), i, dimv3_RDMA_RD(i));
	for (i = 0x3740; i < 0x3744; i++)
		pr_info("[0x%x][0x%x]=0x%x\n",
			base_addr + (i << 2), i, dimv3_RDMA_RD(i));
	pr_info("-----nrds reg end-----\n");
}
