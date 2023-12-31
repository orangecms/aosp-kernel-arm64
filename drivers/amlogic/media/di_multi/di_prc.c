// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * drivers/amlogic/media/di_multi/di_prc.c
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
#include <linux/err.h>
#include <linux/seq_file.h>

#include <linux/amlogic/media/vfm/vframe.h>
#include "deinterlace.h"

#include "di_data_l.h"
#include "di_data.h"
#include "di_dbg.h"
#include "di_sys.h"
#include "di_vframe.h"
#include "di_que.h"
#include "di_task.h"

#include "di_prc.h"
#include "di_pre.h"
#include "di_post.h"
#include "di_api.h"
#include "sc2/di_hw_v3.h"

#include <linux/amlogic/media/di/di.h>
//#include "../deinterlace/di_pqa.h"

/************************************************
 * dim_cfg
 *	[0] bypass_all_p
 ***********************************************/
#ifdef TEST_DISABLE_BYPASS_P
static unsigned int dim_cfg;
#else
static unsigned int dim_cfg = 1;
#endif
module_param_named(dim_cfg, dim_cfg, uint, 0664);

/**************************************
 *
 * cfg ctr top
 *	bool
 **************************************/

const struct di_cfg_ctr_s di_cfg_top_ctr[K_DI_CFG_NUB] = {
	/*same order with enum eDI_DBG_CFG*/
	/* cfg for top */
	[EDI_CFG_BEGIN]  = {"cfg top begin ", EDI_CFG_BEGIN, 0,
			K_DI_CFG_T_FLG_NONE},
	[EDI_CFG_MEM_FLAG]  = {"flag_cma", EDI_CFG_MEM_FLAG,
				EDI_MEM_M_CMA,
				K_DI_CFG_T_FLG_DTS},
	[EDI_CFG_FIRST_BYPASS]  = {"first_bypass",
				EDI_CFG_FIRST_BYPASS,
				0,
				K_DI_CFG_T_FLG_DTS},
	[EDI_CFG_REF_2]  = {"ref_2",
		EDI_CFG_REF_2, 0, K_DI_CFG_T_FLG_NOTHING},
	[EDI_CFG_PMODE]  = {"pmode:0:as p;1:as i;2:use 2 i buf",
			EDI_CFG_PMODE,
			1,
			K_DI_CFG_T_FLG_NOTHING},
	[EDI_CFG_KEEP_CLEAR_AUTO]  = {"keep_buf clear auto",
			EDI_CFG_KEEP_CLEAR_AUTO,
			0,
			K_DI_CFG_T_FLG_NOTHING},
	[EDI_CFG_MEM_RELEASE_BLOCK_MODE]  = {"release mem use block mode",
			EDI_CFG_MEM_RELEASE_BLOCK_MODE,
			1,
			K_DI_CFG_T_FLG_NOTHING},
	[EDI_CFG_FIX_BUF]  = {"fix buf",
			EDI_CFG_FIX_BUF,
			0,
			K_DI_CFG_T_FLG_NOTHING},
	[EDI_CFG_PAUSE_SRC_CHG]  = {"pause when source chang",
			EDI_CFG_PAUSE_SRC_CHG,
			0,
			K_DI_CFG_T_FLG_NOTHING},
	[EDI_CFG_4K]  = {"en_4k",
			EDI_CFG_4K,
			/* 0: not support 4K;	*/
			/* 1: enable 4K		*/
			/* 2: dynamic: vdin: 4k enable, other, disable	*/
			0,
			K_DI_CFG_T_FLG_DTS},
	[EDI_CFG_POUT_FMT]  = {"po_fmt",
	/* 0:default; 1: nv21; 2: nv12; 3:afbce */
	/* 4: dynamic change p out put;	 4k:afbce, other:yuv422 10*/
	/* 5: dynamic: 4k: nv21, other yuv422 10bit */
			EDI_CFG_POUT_FMT,
			3,
			K_DI_CFG_T_FLG_DTS},
	[EDI_CFG_DAT]  = {"en_dat",/*bit 0: pst dat; bit 1: idat */
			EDI_CFG_DAT,
			0,
			K_DI_CFG_T_FLG_DTS},
	[EDI_CFG_ALLOC_WAIT]  = {"alloc waite", /* 0:not wait; 1: wait */
			EDI_CFG_ALLOC_WAIT,
			0, //1, //
			K_DI_CFG_T_FLG_NOTHING},
	[EDI_CFG_KEEP_DEC_VF]  = {"keep_dec_vf",
			/* 0:not keep; 1: keep dec vf for p; 2: dynamic*/
			EDI_CFG_KEEP_DEC_VF,
			1,
			K_DI_CFG_T_FLG_DTS},
	[EDI_CFG_POST_NUB]  = {"post_nub",
			/* 0:not config post nub;*/
			EDI_CFG_POST_NUB,
			0,
			K_DI_CFG_T_FLG_DTS},
	[EDI_CFG_BYPASS_MEM]  = {"bypass_mem",
			/* 0:not bypass;	*/
			/* 1:bypass;		*/
			/* 2: when 4k bypass	*/
			EDI_CFG_BYPASS_MEM,
			0,
			K_DI_CFG_T_FLG_DTS},
	[EDI_CFG_END]  = {"cfg top end ", EDI_CFG_END, 0,
			K_DI_CFG_T_FLG_NONE},

};

void di_cfg_set(enum ECFG_DIM idx, unsigned int data)
{
	if ((idx == ECFG_DIM_BYPASS_P) && (!data))
		dim_cfg &= (~DI_BIT0);
}

char *di_cfg_top_get_name(enum EDI_CFG_TOP_IDX idx)
{
	return di_cfg_top_ctr[idx].dts_name;
}

void di_cfg_top_get_info(unsigned int idx, char **name)
{
	if (di_cfg_top_ctr[idx].id != idx)
		PR_ERR("%s:err:idx not map [%d->%d]\n", __func__,
		       idx, di_cfg_top_ctr[idx].id);

	*name = di_cfg_top_ctr[idx].dts_name;
}

bool di_cfg_top_check(unsigned int idx)
{
	unsigned int tsize;

	tsize = ARRAY_SIZE(di_cfg_top_ctr);
	if (idx >= tsize) {
		PR_ERR("%s:err:overflow:%d->%d\n",
		       __func__, idx, tsize);
		return false;
	}
	if (di_cfg_top_ctr[idx].flg & K_DI_CFG_T_FLG_NONE)
		return false;
	if (idx != di_cfg_top_ctr[idx].id) {
		PR_ERR("%s:%s:err:not map:%d->%d\n",
		       __func__,
		       di_cfg_top_ctr[idx].dts_name,
		       idx,
		       di_cfg_top_ctr[idx].id);
		return false;
	}
	return true;
}

void di_cfg_top_init_val(void)
{
	int i;
	union di_cfg_tdata_u *pd;
	const struct di_cfg_ctr_s *pt;

	PR_INF("%s:\n", __func__);
	for (i = EDI_CFG_BEGIN; i < EDI_CFG_END; i++) {
		if (!di_cfg_top_check(i))
			continue;
		pd = &get_datal()->cfg_en[i];
		pt = &di_cfg_top_ctr[i];
		/*di_cfg_top_set(i, di_cfg_top_ctr[i].default_val);*/
		pd->d32 = 0;/*clear*/
		pd->b.val_df = pt->default_val;
		pd->b.val_c = pd->b.val_df;
	}
	PR_INF("%s:finish\n", __func__);
}

void di_cfg_top_dts(void)
{
	struct platform_device *pdev = get_dim_de_devp()->pdev;
	int i;
	union di_cfg_tdata_u *pd;
	const struct di_cfg_ctr_s *pt;
	int ret;
	unsigned int uval;

	if (!pdev) {
		PR_ERR("%s:no pdev\n", __func__);
		return;
	}
	PR_INF("%s\n", __func__);
	for (i = EDI_CFG_BEGIN; i < EDI_CFG_END; i++) {
		if (!di_cfg_top_check(i))
			continue;
		if (!(di_cfg_top_ctr[i].flg & K_DI_CFG_T_FLG_DTS))
			continue;
		pd = &get_datal()->cfg_en[i];
		pt = &di_cfg_top_ctr[i];
		pd->b.dts_en = 1;
		ret = of_property_read_u32(pdev->dev.of_node,
					   pt->dts_name,
					   &uval);
		if (ret)
			continue;
		PR_INF("\t%s:%d\n", pt->dts_name, uval);
		pd->b.dts_have = 1;
		pd->b.val_dts = uval;
		pd->b.val_c = pd->b.val_dts;
	}
	//PR_INF("%s end\n", __func__);

	if (cfgg(4K) && DIM_IS_IC_BF(TM2)) {
		cfgs(4K, 0);
		PR_INF("not support 4k\n");
	}
	/* dat */
	/*bit 0: pst dat; bit 1: idat */
	pd = &get_datal()->cfg_en[EDI_CFG_DAT];
	pt = &di_cfg_top_ctr[EDI_CFG_DAT];
	if (DIM_IS_IC(TM2B)	||
	    DIM_IS_IC(SC2)) {//	    DIM_IS_IC(T5)
		if (!pd->b.dts_have) {
			pd->b.val_c = 0x3;
			//pd->b.val_c = 0x0;//test
		}
	} else {
		pd->b.val_c = 0;
	}
	PR_INF("%s:%s:0x%x\n", __func__, pt->dts_name, pd->b.val_c);
}

static void di_cfgt_show_item_one(struct seq_file *s, unsigned int index)
{
	union di_cfg_tdata_u *pd;
	const struct di_cfg_ctr_s *pt;

	if (!di_cfg_top_check(index))
		return;
	pd = &get_datal()->cfg_en[index];
	pt = &di_cfg_top_ctr[index];
	seq_printf(s, "id:%2d:%-10s\n", index, pt->dts_name);
	seq_printf(s, "\t%-5s:0x%2x[%d]\n",
		   "tdf", pt->default_val, pt->default_val);
	seq_printf(s, "\t%-5s:%d\n",
		   "tdts", pt->flg & K_DI_CFG_T_FLG_DTS);
	seq_printf(s, "\t%-5s:0x%-4x\n", "d32", pd->d32);
	seq_printf(s, "\t%-5s:0x%2x[%d]\n",
		   "vdf", pd->b.val_df, pd->b.val_df);
	seq_printf(s, "\t%-5s:0x%2x[%d]\n",
		   "vdts", pd->b.val_dts, pd->b.val_dts);
	seq_printf(s, "\t%-5s:0x%2x[%d]\n",
		   "vdbg", pd->b.val_dbg, pd->b.val_dbg);
	seq_printf(s, "\t%-5s:0x%2x[%d]\n", "vc", pd->b.val_c, pd->b.val_c);
	seq_printf(s, "\t%-5s:%d\n", "endts", pd->b.dts_en);
	seq_printf(s, "\t%-5s:%d\n", "hdts", pd->b.dts_have);
	seq_printf(s, "\t%-5s:%d\n", "hdbg", pd->b.dbg_have);
}

void di_cfgt_show_item_sel(struct seq_file *s)
{
	int i = get_datal()->cfg_sel;

	di_cfgt_show_item_one(s, i);
}

void di_cfgt_set_sel(unsigned int dbg_mode, unsigned int id)
{
	if (!di_cfg_top_check(id)) {
		PR_ERR("%s:%d is overflow\n", __func__, id);
		return;
	}
	get_datal()->cfg_sel = id;
	get_datal()->cfg_dbg_mode = dbg_mode;
}

void di_cfgt_show_item_all(struct seq_file *s)
{
	int i;

	for (i = EDI_CFG_BEGIN; i < EDI_CFG_END; i++)
		di_cfgt_show_item_one(s, i);
}

static void di_cfgt_show_val_one(struct seq_file *s, unsigned int index)
{
	union di_cfg_tdata_u *pd;
	const struct di_cfg_ctr_s *pt;

	if (!di_cfg_top_check(index))
		return;
	pd = &get_datal()->cfg_en[index];
	pt = &di_cfg_top_ctr[index];
	seq_printf(s, "id:%2d:%-10s\n", index, pt->dts_name);
	seq_printf(s, "\t%-5s:0x%-4x\n", "d32", pd->d32);
	seq_printf(s, "\t%-5s:0x%2x[%d]\n", "vc", pd->b.val_c, pd->b.val_c);
}

void di_cfgt_show_val_sel(struct seq_file *s)
{
	unsigned int i = get_datal()->cfg_sel;

	di_cfgt_show_val_one(s, i);
}

void di_cfgt_show_val_all(struct seq_file *s)
{
	int i;

	for (i = EDI_CFG_BEGIN; i < EDI_CFG_END; i++)
		di_cfgt_show_val_one(s, i);
}

unsigned int di_cfg_top_get(enum EDI_CFG_TOP_IDX id)
{
	union di_cfg_tdata_u *pd;

	pd = &get_datal()->cfg_en[id];
	return pd->b.val_c;
}

void di_cfg_top_set(enum EDI_CFG_TOP_IDX id, unsigned int val)
{
	union di_cfg_tdata_u *pd;

	pd = &get_datal()->cfg_en[id];
	pd->b.val_dbg = val;
	pd->b.dbg_have = 1;
	pd->b.val_c = val;
}

/**************************************
 *
 * cfg ctr x
 *	bool
 **************************************/

const struct di_cfgx_ctr_s di_cfgx_ctr[K_DI_CFGX_NUB] = {
	/*same order with enum eDI_DBG_CFG*/

	/* cfg channel x*/
	[EDI_CFGX_BEGIN]  = {"cfg x begin ", EDI_CFGX_BEGIN, 0},
	/* bypass_all */
	[EDI_CFGX_BYPASS_ALL]  = {"bypass_all", EDI_CFGX_BYPASS_ALL, 0},
	[EDI_CFGX_END]  = {"cfg x end ", EDI_CFGX_END, 0},

	/* debug cfg x */
	[EDI_DBG_CFGX_BEGIN]  = {"cfg dbg begin ", EDI_DBG_CFGX_BEGIN, 0},
	[EDI_DBG_CFGX_IDX_VFM_IN] = {"vfm_in", EDI_DBG_CFGX_IDX_VFM_IN, 1},
	[EDI_DBG_CFGX_IDX_VFM_OT] = {"vfm_out", EDI_DBG_CFGX_IDX_VFM_OT, 1},
	[EDI_DBG_CFGX_END]    = {"cfg dbg end", EDI_DBG_CFGX_END, 0},
};

char *di_cfgx_get_name(enum EDI_CFGX_IDX idx)
{
	return di_cfgx_ctr[idx].name;
}

void di_cfgx_get_info(enum EDI_CFGX_IDX idx, char **name)
{
	if (di_cfgx_ctr[idx].id != idx)
		PR_ERR("%s:err:idx not map [%d->%d]\n", __func__,
		       idx, di_cfgx_ctr[idx].id);

	*name = di_cfgx_ctr[idx].name;
}

bool di_cfgx_check(unsigned int idx)
{
	unsigned int tsize;

	tsize = ARRAY_SIZE(di_cfgx_ctr);
	if (idx >= tsize) {
		PR_ERR("%s:err:overflow:%d->%d\n",
		       __func__, idx, tsize);
		return false;
	}
	if (idx != di_cfgx_ctr[idx].id) {
		PR_ERR("%s:err:not map:%d->%d\n",
		       __func__, idx, di_cfgx_ctr[idx].id);
		return false;
	}
	pr_info("\t%-15s=%d\n", di_cfgx_ctr[idx].name,
		di_cfgx_ctr[idx].default_val);
	return true;
}

void di_cfgx_init_val(void)
{
	int i, ch;

	for (ch = 0; ch < DI_CHANNEL_NUB; ch++) {
		for (i = EDI_CFGX_BEGIN; i < EDI_DBG_CFGX_END; i++)
			di_cfgx_set(ch, i, di_cfgx_ctr[i].default_val);
	}
}

bool di_cfgx_get(unsigned int ch, enum EDI_CFGX_IDX idx)
{
	return get_datal()->ch_data[ch].cfgx_en[idx];
}

void di_cfgx_set(unsigned int ch, enum EDI_CFGX_IDX idx, bool en)
{
	get_datal()->ch_data[ch].cfgx_en[idx] = en;
}

/**************************************
 *
 * module para top
 *	int
 **************************************/

const struct di_mp_uit_s di_mp_ui_top[] = {
	/*same order with enum eDI_MP_UI*/
	/* for top */
	[EDI_MP_UI_T_BEGIN]  = {"module para top begin ",
			EDI_MP_UI_T_BEGIN, 0},
	/**************************************/
	[EDI_MP_SUB_DI_B]  = {"di begin ",
			EDI_MP_SUB_DI_B, 0},
	[edi_mp_force_prog]  = {"bool:force_prog:1",
			edi_mp_force_prog, 1},
	[edi_mp_combing_fix_en]  = {"bool:combing_fix_en,def:1",
			edi_mp_combing_fix_en, 1},
	[edi_mp_cur_lev]  = {"int cur_lev,def:2",
			edi_mp_cur_lev, 2},
	[edi_mp_pps_dstw]  = {"pps_dstw:int",
			edi_mp_pps_dstw, 0},
	[edi_mp_pps_dsth]  = {"pps_dsth:int",
			edi_mp_pps_dsth, 0},
	[edi_mp_pps_en]  = {"pps_en:bool",
			edi_mp_pps_en, 0},
	[edi_mp_pps_position]  = {"pps_position:uint:def:1",
			edi_mp_pps_position, 1},
	[edi_mp_pre_enable_mask]  = {"pre_enable_mask:bit0:ma;bit1:mc:def:3",
			edi_mp_pre_enable_mask, 3},
	[edi_mp_post_refresh]  = {"post_refresh:bool",
			edi_mp_post_refresh, 0},
	[edi_mp_nrds_en]  = {"nrds_en:bool",
			edi_mp_nrds_en, 0},
	[edi_mp_bypass_3d]  = {"bypass_3d:int:def:1",
			edi_mp_bypass_3d, 1},
	[edi_mp_bypass_trick_mode]  = {"bypass_trick_mode:int:def:1",
			edi_mp_bypass_trick_mode, 1},
	[edi_mp_invert_top_bot]  = {"invert_top_bot:int",
			edi_mp_invert_top_bot, 0},
	[edi_mp_skip_top_bot]  = {"skip_top_bot:int:",
			/*1or2: may affect atv when bypass di*/
			edi_mp_skip_top_bot, 0},
	[edi_mp_force_width]  = {"force_width:int",
			edi_mp_force_width, 0},
	[edi_mp_force_height]  = {"force_height:int",
			edi_mp_force_height, 0},
	[edi_mp_prog_proc_config]  = {"prog_proc_config:int:def:0x",
/* prog_proc_config,
 * bit[2:1]: when two field buffers are used,
 * 0 use vpp for blending ,
 * 1 use post_di module for blending
 * 2 debug mode, bob with top field
 * 3 debug mode, bot with bot field
 * bit[0]:
 * 0 "prog vdin" use two field buffers,
 * 1 "prog vdin" use single frame buffer
 * bit[4]:
 * 0 "prog frame from decoder/vdin" use two field buffers,
 * 1 use single frame buffer
 * bit[5]:
 * when two field buffers are used for decoder (bit[4] is 0):
 * 1,handle prog frame as two interlace frames
 * bit[6]:(bit[4] is 0,bit[5] is 0,use_2_interlace_buff is 0): 0,
 * process progress frame as field,blend by post;
 * 1, process progress frame as field,process by normal di
 */
			edi_mp_prog_proc_config, ((1 << 5) | (1 << 1) | 1)},
	[edi_mp_start_frame_drop_count]  = {"start_frame_drop_count:int:2",
			edi_mp_start_frame_drop_count, 0},
	[edi_mp_same_field_top_count]  = {"same_field_top_count:long?",
			edi_mp_same_field_top_count, 0},
	[edi_mp_same_field_bot_count]  = {"same_field_bot_count:long?",
			edi_mp_same_field_bot_count, 0},
	[edi_mp_vpp_3d_mode]  = {"vpp_3d_mode:int",
			edi_mp_vpp_3d_mode, 0},
	[edi_mp_force_recovery_count]  = {"force_recovery_count:uint",
			edi_mp_force_recovery_count, 0},
	[edi_mp_pre_process_time]  = {"pre_process_time:int",
			edi_mp_pre_process_time, 0},
	[edi_mp_bypass_post]  = {"bypass_post:int",
			edi_mp_bypass_post, 0},
	[edi_mp_post_wr_en]  = {"post_wr_en:bool:1",
			edi_mp_post_wr_en, 1},
	[edi_mp_post_wr_support]  = {"post_wr_support:uint",
			edi_mp_post_wr_support, 0},
	[edi_mp_bypass_post_state]  = {"bypass_post_state:int",
/* 0, use di_wr_buf still;
 * 1, call dim_pre_de_done_buf_clear to clear di_wr_buf;
 * 2, do nothing
 */
			edi_mp_bypass_post_state, 0},
	[edi_mp_use_2_interlace_buff]  = {"use_2_interlace_buff:int",
			edi_mp_use_2_interlace_buff, 0},
	[edi_mp_debug_blend_mode]  = {"debug_blend_mode:int:-1",
			edi_mp_debug_blend_mode, -1},
	[edi_mp_nr10bit_support]  = {"nr10bit_support:uint",
		/* 0: not support nr10bit, 1: support nr10bit */
			edi_mp_nr10bit_support, 0},
	[edi_mp_di_stop_reg_flag]  = {"di_stop_reg_flag:uint",
			edi_mp_di_stop_reg_flag, 0},
	[edi_mp_mcpre_en]  = {"mcpre_en:bool:true",
			edi_mp_mcpre_en, 1},
	[edi_mp_check_start_drop]  = {"check_start_drop_prog:bool",
			edi_mp_check_start_drop, 0},
	[edi_mp_overturn]  = {"overturn:bool:?",
			edi_mp_overturn, 0},
	[edi_mp_full_422_pack]  = {"full_422_pack:bool",
			edi_mp_full_422_pack, 0},
	[edi_mp_cma_print]  = {"cma_print:bool:1",
			edi_mp_cma_print, 0},
	[edi_mp_pulldown_enable]  = {"pulldown_enable:bool:1",
			edi_mp_pulldown_enable, 1},
	[edi_mp_di_force_bit_mode]  = {"di_force_bit_mode:uint:10",
			edi_mp_di_force_bit_mode, 10},
	[edi_mp_calc_mcinfo_en]  = {"calc_mcinfo_en:bool:1",
			edi_mp_calc_mcinfo_en, 1},
	[edi_mp_colcfd_thr]  = {"colcfd_thr:uint:128",
			edi_mp_colcfd_thr, 128},
	[edi_mp_post_blend]  = {"post_blend:uint",
			edi_mp_post_blend, 0},
	[edi_mp_post_ei]  = {"post_ei:uint",
			edi_mp_post_ei, 0},
	[edi_mp_post_cnt]  = {"post_cnt:uint",
			edi_mp_post_cnt, 0},
	[edi_mp_di_log_flag]  = {"di_log_flag:uint",
			edi_mp_di_log_flag, 0},
	[edi_mp_di_debug_flag]  = {"di_debug_flag:uint",
			edi_mp_di_debug_flag, 0},
	[edi_mp_buf_state_log_threshold]  = {"buf_state_log_threshold:unit:16",
			edi_mp_buf_state_log_threshold, 16},
	[edi_mp_di_vscale_skip_enable]  = {"di_vscale_skip_enable:int",
/*
 * bit[2]: enable bypass all when skip
 * bit[1:0]: enable bypass post when skip
 */
			edi_mp_di_vscale_skip_enable, 0},
	[edi_mp_di_vscale_skip_count]  = {"di_vscale_skip_count:int",
			edi_mp_di_vscale_skip_count, 0},
	[edi_mp_di_vscale_skip_real]  = {"di_vscale_skip_count_real:int",
			edi_mp_di_vscale_skip_real, 0},
	[edi_mp_det3d_en]  = {"det3d_en:bool",
			edi_mp_det3d_en, 0},
	[edi_mp_post_hold_line]  = {"post_hold_line:int:8",
			edi_mp_post_hold_line, 8},
	[edi_mp_post_urgent]  = {"post_urgent:int:1",
			edi_mp_post_urgent, 1},
	[edi_mp_di_printk_flag]  = {"di_printk_flag:uint",
			edi_mp_di_printk_flag, 0},
	[edi_mp_force_recovery]  = {"force_recovery:uint:1",
			edi_mp_force_recovery, 1},
#ifdef MARK_HIS
	[edi_mp_debug_blend_mode]  = {"debug_blend_mode:int:-1",
			edi_mp_debug_blend_mode, -1},
#endif
	[edi_mp_di_dbg_mask]  = {"di_dbg_mask:uint:0x02",
			edi_mp_di_dbg_mask, 2},
	[edi_mp_nr_done_check_cnt]  = {"nr_done_check_cnt:uint:5",
			edi_mp_nr_done_check_cnt, 5},
	[edi_mp_pre_hsc_down_en]  = {"pre_hsc_down_en:bool:0",
			edi_mp_pre_hsc_down_en, 0},
	[edi_mp_pre_hsc_down_width]  = {"pre_hsc_down_width:int:480",
				edi_mp_pre_hsc_down_width, 480},
	[edi_mp_show_nrwr]  = {"show_nrwr:bool:0",
				edi_mp_show_nrwr, 0},

	/******deinterlace_hw.c**********/
	[edi_mp_pq_load_dbg]  = {"pq_load_dbg:uint",
			edi_mp_pq_load_dbg, 0},
	[edi_mp_lmv_lock_win_en]  = {"lmv_lock_win_en:bool",
			edi_mp_lmv_lock_win_en, 0},
	[edi_mp_lmv_dist]  = {"lmv_dist:short:5",
			edi_mp_lmv_dist, 5},
	[edi_mp_pr_mcinfo_cnt]  = {"pr_mcinfo_cnt:ushort",
			edi_mp_pr_mcinfo_cnt, 0},
	[edi_mp_offset_lmv]  = {"offset_lmv:short:100",
			edi_mp_offset_lmv, 100},
	[edi_mp_post_ctrl]  = {"post_ctrl:uint",
			edi_mp_post_ctrl, 0},
	[edi_mp_if2_disable]  = {"if2_disable:bool",
			edi_mp_if2_disable, 0},
	[edi_mp_pre]  = {"pre_flag:ushort:2",
			edi_mp_pre, 2},
	[edi_mp_pre_mif_gate]  = {"pre_mif_gate:bool",
			edi_mp_pre_mif_gate, 0},
	[edi_mp_pre_urgent]  = {"pre_urgent:ushort",
			edi_mp_pre_urgent, 0},
	[edi_mp_pre_hold_line]  = {"pre_hold_line:ushort:10",
			edi_mp_pre_hold_line, 10},
	[edi_mp_pre_ctrl]  = {"pre_ctrl:uint",
			edi_mp_pre_ctrl, 0},
	[edi_mp_line_num_post_frst]  = {"line_num_post_frst:ushort:5",
			edi_mp_line_num_post_frst, 5},
	[edi_mp_line_num_pre_frst]  = {"line_num_pre_frst:ushort:5",
			edi_mp_line_num_pre_frst, 5},
	[edi_mp_pd22_flg_calc_en]  = {"pd22_flg_calc_en:bool:true",
			edi_mp_pd22_flg_calc_en, 1},
	[edi_mp_mcen_mode]  = {"mcen_mode:ushort:1",
			edi_mp_mcen_mode, 1},
	[edi_mp_mcuv_en]  = {"mcuv_en:ushort:1",
			edi_mp_mcuv_en, 1},
	[edi_mp_mcdebug_mode]  = {"mcdebug_mode:ushort",
			edi_mp_mcdebug_mode, 0},
	[edi_mp_pldn_ctrl_rflsh]  = {"pldn_ctrl_rflsh:uint:1",
			edi_mp_pldn_ctrl_rflsh, 1},

	[EDI_MP_SUB_DI_E]  = {"di end-------",
				EDI_MP_SUB_DI_E, 0},
	/**************************************/
	[EDI_MP_SUB_NR_B]  = {"nr begin",
			EDI_MP_SUB_NR_B, 0},
	[edi_mp_dnr_en]  = {"dnr_en:bool:true",
			edi_mp_dnr_en, 1},
	[edi_mp_nr2_en]  = {"nr2_en:uint:1",
			edi_mp_nr2_en, 1},
	[edi_mp_cue_en]  = {"cue_en:bool:true",
			edi_mp_cue_en, 1},
	[edi_mp_invert_cue_phase]  = {"invert_cue_phase:bool",
			edi_mp_invert_cue_phase, 0},
	[edi_mp_cue_pr_cnt]  = {"cue_pr_cnt:uint",
			edi_mp_cue_pr_cnt, 0},
	[edi_mp_cue_glb_mot_check_en]  = {"cue_glb_mot_check_en:bool:true",
			edi_mp_cue_glb_mot_check_en, 1},
	[edi_mp_glb_fieldck_en]  = {"glb_fieldck_en:bool:true",
			edi_mp_glb_fieldck_en, 1},
	[edi_mp_dnr_pr]  = {"dnr_pr:bool",
			edi_mp_dnr_pr, 0},
	[edi_mp_dnr_dm_en]  = {"dnr_dm_en:bool",
			edi_mp_dnr_dm_en, 0},
	[EDI_MP_SUB_NR_E]  = {"nr end-------",
			EDI_MP_SUB_NR_E, 0},
	/**************************************/
	[EDI_MP_SUB_PD_B]  = {"pd begin",
			EDI_MP_SUB_PD_B, 0},
	[edi_mp_flm22_ratio]  = {"flm22_ratio:uint:200",
			edi_mp_flm22_ratio, 200},
	[edi_mp_pldn_cmb0]  = {"pldn_cmb0:uint:1",
			edi_mp_pldn_cmb0, 1},
	[edi_mp_pldn_cmb1]  = {"pldn_cmb1:uint",
			edi_mp_pldn_cmb1, 0},
	[edi_mp_flm22_sure_num]  = {"flm22_sure_num:uint:100",
			edi_mp_flm22_sure_num, 100},
	[edi_mp_flm22_glbpxlnum_rat]  = {"flm22_glbpxlnum_rat:uint:4",
			edi_mp_flm22_glbpxlnum_rat, 4},
	[edi_mp_flag_di_weave]  = {"flag_di_weave:int:1",
			edi_mp_flag_di_weave, 1},
	[edi_mp_flm22_glbpxl_maxrow]  = {"flm22_glbpxl_maxrow:uint:16",
			edi_mp_flm22_glbpxl_maxrow, 16},
	[edi_mp_flm22_glbpxl_minrow]  = {"flm22_glbpxl_minrow:uint:3",
			edi_mp_flm22_glbpxl_minrow, 3},
	[edi_mp_cmb_3point_rnum]  = {"cmb_3point_rnum:uint",
			edi_mp_cmb_3point_rnum, 0},
	[edi_mp_cmb_3point_rrat]  = {"cmb_3point_rrat:unit:32",
			edi_mp_cmb_3point_rrat, 32},
	/********************************/
	[edi_mp_pr_pd]  = {"pr_pd:uint",
			edi_mp_pr_pd, 0},
	[edi_mp_prt_flg]  = {"prt_flg:bool",
			edi_mp_prt_flg, 0},
	[edi_mp_flmxx_maybe_num]  = {"flmxx_maybe_num:uint:15",
	/* if flmxx level > flmxx_maybe_num */
	/* mabye flmxx: when 2-2 3-2 not detected */
			edi_mp_flmxx_maybe_num, 15},
	[edi_mp_flm32_mim_frms]  = {"flm32_mim_frms:int:6",
			edi_mp_flm32_mim_frms, 6},
	[edi_mp_flm22_dif01a_flag]  = {"flm22_dif01a_flag:int:1",
			edi_mp_flm22_dif01a_flag, 1},
	[edi_mp_flm22_mim_frms]  = {"flm22_mim_frms:int:60",
			edi_mp_flm22_mim_frms, 60},
	[edi_mp_flm22_mim_smfrms]  = {"flm22_mim_smfrms:int:40",
			edi_mp_flm22_mim_smfrms, 40},
	[edi_mp_flm32_f2fdif_min0]  = {"flm32_f2fdif_min0:int:11",
			edi_mp_flm32_f2fdif_min0, 11},
	[edi_mp_flm32_f2fdif_min1]  = {"flm32_f2fdif_min1:int:11",
			edi_mp_flm32_f2fdif_min1, 11},
	[edi_mp_flm32_chk1_rtn]  = {"flm32_chk1_rtn:int:25",
			edi_mp_flm32_chk1_rtn, 25},
	[edi_mp_flm32_ck13_rtn]  = {"flm32_ck13_rtn:int:8",
			edi_mp_flm32_ck13_rtn, 8},
	[edi_mp_flm32_chk2_rtn]  = {"flm32_chk2_rtn:int:16",
			edi_mp_flm32_chk2_rtn, 16},
	[edi_mp_flm32_chk3_rtn]  = {"flm32_chk3_rtn:int:16",
			edi_mp_flm32_chk3_rtn, 16},
	[edi_mp_flm32_dif02_ratio]  = {"flm32_dif02_ratio:int:8",
			edi_mp_flm32_dif02_ratio, 8},
	[edi_mp_flm22_chk20_sml]  = {"flm22_chk20_sml:int:6",
			edi_mp_flm22_chk20_sml, 6},
	[edi_mp_flm22_chk21_sml]  = {"flm22_chk21_sml:int:6",
			edi_mp_flm22_chk21_sml, 6},
	[edi_mp_flm22_chk21_sm2]  = {"flm22_chk21_sm2:int:10",
			edi_mp_flm22_chk21_sm2, 10},
	[edi_mp_flm22_lavg_sft]  = {"flm22_lavg_sft:int:4",
			edi_mp_flm22_lavg_sft, 4},
	[edi_mp_flm22_lavg_lg]  = {"flm22_lavg_lg:int:24",
			edi_mp_flm22_lavg_lg, 24},
	[edi_mp_flm22_stl_sft]  = {"flm22_stl_sft:int:7",
			edi_mp_flm22_stl_sft, 7},
	[edi_mp_flm22_chk5_avg]  = {"flm22_chk5_avg:int:50",
			edi_mp_flm22_chk5_avg, 50},
	[edi_mp_flm22_chk6_max]  = {"flm22_chk6_max:int:20",
			edi_mp_flm22_chk6_max, 20},
	[edi_mp_flm22_anti_chk1]  = {"flm22_anti_chk1:int:61",
			edi_mp_flm22_anti_chk1, 61},
	[edi_mp_flm22_anti_chk3]  = {"flm22_anti_chk3:int:140",
			edi_mp_flm22_anti_chk3, 140},
	[edi_mp_flm22_anti_chk4]  = {"flm22_anti_chk4:int:128",
			edi_mp_flm22_anti_chk4, 128},
	[edi_mp_flm22_anti_ck140]  = {"flm22_anti_ck140:int:32",
			edi_mp_flm22_anti_ck140, 32},
	[edi_mp_flm22_anti_ck141]  = {"flm22_anti_ck141:int:80",
			edi_mp_flm22_anti_ck141, 80},
	[edi_mp_flm22_frmdif_max]  = {"flm22_frmdif_max:int:50",
			edi_mp_flm22_frmdif_max, 50},
	[edi_mp_flm22_flddif_max]  = {"flm22_flddif_max:int:100",
			edi_mp_flm22_flddif_max, 100},
	[edi_mp_flm22_minus_cntmax]  = {"flm22_minus_cntmax:int:2",
			edi_mp_flm22_minus_cntmax, 2},
	[edi_mp_flagdif01chk]  = {"flagdif01chk:int:1",
			edi_mp_flagdif01chk, 1},
	[edi_mp_dif01_ratio]  = {"dif01_ratio:int:10",
			edi_mp_dif01_ratio, 10},
	/*************vof_soft_top**************/
	[edi_mp_cmb32_blw_wnd]  = {"cmb32_blw_wnd:int:180",
			edi_mp_cmb32_blw_wnd, 180},
	[edi_mp_cmb32_wnd_ext]  = {"cmb32_wnd_ext:int:11",
			edi_mp_cmb32_wnd_ext, 11},
	[edi_mp_cmb32_wnd_tol]  = {"cmb32_wnd_tol:int:4",
			edi_mp_cmb32_wnd_tol, 4},
	[edi_mp_cmb32_frm_nocmb]  = {"cmb32_frm_nocmb:int:40",
			edi_mp_cmb32_frm_nocmb, 40},
	[edi_mp_cmb32_min02_sft]  = {"cmb32_min02_sft:int:7",
			edi_mp_cmb32_min02_sft, 7},
	[edi_mp_cmb32_cmb_tol]  = {"cmb32_cmb_tol:int:10",
			edi_mp_cmb32_cmb_tol, 10},
	[edi_mp_cmb32_avg_dff]  = {"cmb32_avg_dff:int:48",
			edi_mp_cmb32_avg_dff, 48},
	[edi_mp_cmb32_smfrm_num]  = {"cmb32_smfrm_num:int:4",
			edi_mp_cmb32_smfrm_num, 4},
	[edi_mp_cmb32_nocmb_num]  = {"cmb32_nocmb_num:int:20",
			edi_mp_cmb32_nocmb_num, 20},
	[edi_mp_cmb22_gcmb_rnum]  = {"cmb22_gcmb_rnum:int:8",
			edi_mp_cmb22_gcmb_rnum, 8},
	[edi_mp_flmxx_cal_lcmb]  = {"flmxx_cal_lcmb:int:1",
			edi_mp_flmxx_cal_lcmb, 1},
	[edi_mp_flm2224_stl_sft]  = {"flm2224_stl_sft:int:7",
			edi_mp_flm2224_stl_sft, 7},
	[EDI_MP_SUB_PD_E]  = {"pd end------",
			EDI_MP_SUB_PD_E, 0},
	/**************************************/
	[EDI_MP_SUB_MTN_B]  = {"mtn begin",
			EDI_MP_SUB_MTN_B, 0},
	[edi_mp_force_lev]  = {"force_lev:int:0xff",
			edi_mp_force_lev, 0xff},
	[edi_mp_dejaggy_flag]  = {"dejaggy_flag:int:-1",
			edi_mp_dejaggy_flag, -1},
	[edi_mp_dejaggy_enable]  = {"dejaggy_enable:int:1",
			edi_mp_dejaggy_enable, 1},
	[edi_mp_cmb_adpset_cnt]  = {"cmb_adpset_cnt:int",
			edi_mp_cmb_adpset_cnt, 0},
	[edi_mp_cmb_num_rat_ctl4]  = {"cmb_num_rat_ctl4:int:64:0~255",
			edi_mp_cmb_num_rat_ctl4, 64},
	[edi_mp_cmb_rat_ctl4_minthd]  = {"cmb_rat_ctl4_minthd:int:64",
			edi_mp_cmb_rat_ctl4_minthd, 64},
	[edi_mp_small_local_mtn]  = {"small_local_mtn:uint:70",
			edi_mp_small_local_mtn, 70},
	[edi_mp_di_debug_readreg]  = {"di_debug_readreg:int",
			edi_mp_di_debug_readreg, 0},
	[EDI_MP_SUB_MTN_E]  = {"mtn end----",
			EDI_MP_SUB_MTN_E, 0},
	/**************************************/
	[EDI_MP_SUB_3D_B]  = {"3d begin",
			EDI_MP_SUB_3D_B, 0},
	[edi_mp_chessbd_vrate]  = {"chessbd_vrate:int:29",
				edi_mp_chessbd_vrate, 29},
	[edi_mp_det3d_debug]  = {"det3d_debug:bool:0",
				edi_mp_det3d_debug, 0},
	[EDI_MP_SUB_3D_E]  = {"3d begin",
				EDI_MP_SUB_3D_E, 0},

	/**************************************/
	[EDI_MP_UI_T_END]  = {"module para top end ", EDI_MP_UI_T_END, 0},
};

bool di_mp_uit_check(unsigned int idx)
{
	unsigned int tsize;

	tsize = ARRAY_SIZE(di_mp_ui_top);
	if (idx >= tsize) {
		PR_ERR("%s:err:overflow:%d->%d\n",
		       __func__, idx, tsize);
		return false;
	}
	if (idx != di_mp_ui_top[idx].id) {
		PR_ERR("%s:err:not map:%d->%d\n",
		       __func__, idx, di_mp_ui_top[idx].id);
		return false;
	}
	dbg_init("\t%-15s=%d\n", di_mp_ui_top[idx].name,
		 di_mp_ui_top[idx].default_val);
	return true;
}

char *di_mp_uit_get_name(enum EDI_MP_UI_T idx)
{
	return di_mp_ui_top[idx].name;
}

void di_mp_uit_init_val(void)
{
	int i;

	for (i = EDI_MP_UI_T_BEGIN; i < EDI_MP_UI_T_END; i++) {
		if (!di_mp_uit_check(i))
			continue;
		di_mp_uit_set(i, di_mp_ui_top[i].default_val);
	}
}

int di_mp_uit_get(enum EDI_MP_UI_T idx)
{
	return get_datal()->mp_uit[idx];
}

void di_mp_uit_set(enum EDI_MP_UI_T idx, int val)
{
	get_datal()->mp_uit[idx] = val;
}

/************************************************
 * asked by pq tune
 ************************************************/
static bool pulldown_enable = true;
module_param_named(pulldown_enable, pulldown_enable, bool, 0664);

static bool mcpre_en = true;
module_param_named(mcpre_en, mcpre_en, bool, 0664);

static unsigned int mcen_mode = 1;
module_param_named(mcen_mode, mcen_mode, uint, 0664);
/************************************************/

void dim_mp_update_reg(void)
{
	int val;

	val = dimp_get(edi_mp_pulldown_enable);
	if (pulldown_enable != val) {
		PR_INF("mp:pulldown_enable: %d -> %d\n",
		       val, pulldown_enable);
		dimp_set(edi_mp_pulldown_enable, pulldown_enable);
	}

	val = dimp_get(edi_mp_mcpre_en);
	if (mcpre_en != val) {
		PR_INF("mp:mcpre_en: %d -> %d\n",
		       val, mcpre_en);
		dimp_set(edi_mp_mcpre_en, mcpre_en);
	}

	val = dimp_get(edi_mp_mcen_mode);
	if (mcen_mode != val) {
		PR_INF("mp:mcen_mode: %d -> %d\n",
		       val, mcen_mode);
		dimp_set(edi_mp_mcen_mode, mcen_mode);
	}
}

void dim_mp_update_post(void)
{
	int val;

	val = dimp_get(edi_mp_mcen_mode);
	if (mcen_mode != val) {
		PR_INF("mp:mcen_mode: %d -> %d\n",
		       val, mcen_mode);
		dimp_set(edi_mp_mcen_mode, mcen_mode);
	}
}

/**************************************
 *
 * module para x
 *	unsigned int
 **************************************/
const struct di_mp_uix_s di_mpx[] = {
	/*same order with enum eDI_MP_UI*/

	/* module para for channel x*/
	[EDI_MP_UIX_BEGIN]  = {"module para x begin ", EDI_MP_UIX_BEGIN, 0},
	/*debug:	run_flag*/
	[EDI_MP_UIX_RUN_FLG]  = {"run_flag(0:run;1:pause;2:step)",
				EDI_MP_UIX_RUN_FLG, DI_RUN_FLAG_RUN},
	[EDI_MP_UIX_END]  = {"module para x end ", EDI_MP_UIX_END, 0},

};

bool di_mp_uix_check(unsigned int idx)
{
	unsigned int tsize;

	tsize = ARRAY_SIZE(di_mpx);
	if (idx >= tsize) {
		PR_ERR("%s:err:overflow:%d->%d\n",
		       __func__, idx, tsize);
		return false;
	}
	if (idx != di_mpx[idx].id) {
		PR_ERR("%s:err:not map:%d->%d\n",
		       __func__, idx, di_mpx[idx].id);
		return false;
	}
	dbg_init("\t%-15s=%d\n", di_mpx[idx].name, di_mpx[idx].default_val);

	return true;
}

char *di_mp_uix_get_name(enum EDI_MP_UIX_T idx)
{
	return di_mpx[idx].name;
}

void di_mp_uix_init_val(void)
{
	int i, ch;

	for (ch = 0; ch < DI_CHANNEL_NUB; ch++) {
		dbg_init("%s:ch[%d]\n", __func__, ch);
		for (i = EDI_MP_UIX_BEGIN; i < EDI_MP_UIX_END; i++) {
			if (ch == 0) {
				if (!di_mp_uix_check(i))
					continue;
			}
			di_mp_uix_set(ch, i, di_mpx[i].default_val);
		}
	}
}

unsigned int di_mp_uix_get(unsigned int ch, enum EDI_MP_UIX_T idx)
{
	return get_datal()->ch_data[ch].mp_uix[idx];
}

void di_mp_uix_set(unsigned int ch, enum EDI_MP_UIX_T idx, unsigned int val)
{
	get_datal()->ch_data[ch].mp_uix[idx] = val;
}

bool di_is_pause(unsigned int ch)
{
	unsigned int run_flag;

	run_flag = di_mp_uix_get(ch, EDI_MP_UIX_RUN_FLG);

	if (run_flag == DI_RUN_FLAG_PAUSE	||
	    run_flag == DI_RUN_FLAG_STEP_DONE)
		return true;

	return false;
}

void di_pause_step_done(unsigned int ch)
{
	unsigned int run_flag;

	run_flag = di_mp_uix_get(ch, EDI_MP_UIX_RUN_FLG);
	if (run_flag == DI_RUN_FLAG_STEP) {
		di_mp_uix_set(ch, EDI_MP_UIX_RUN_FLG,
			      DI_RUN_FLAG_STEP_DONE);
	}
}

void di_pause(unsigned int ch, bool on)
{
	pr_info("%s:%d\n", __func__, on);
	if (on)
		di_mp_uix_set(ch, EDI_MP_UIX_RUN_FLG,
			      DI_RUN_FLAG_PAUSE);
	else
		di_mp_uix_set(ch, EDI_MP_UIX_RUN_FLG,
			      DI_RUN_FLAG_RUN);
}

/**************************************
 *
 * summmary variable
 *
 **************************************/
const struct di_sum_s di_sum_tab[] = {
	/*video_peek_cnt*/
	[EDI_SUM_O_PEEK_CNT] = {"o_peek_cnt", EDI_SUM_O_PEEK_CNT, 0},
	/*di_reg_unreg_cnt*/
	[EDI_SUM_REG_UNREG_CNT] = {
			"di_reg_unreg_cnt", EDI_SUM_REG_UNREG_CNT, 100},

	[EDI_SUM_NUB] = {"end", EDI_SUM_NUB, 0},
};

unsigned int di_sum_get_tab_size(void)
{
	return ARRAY_SIZE(di_sum_tab);
}

bool di_sum_check(unsigned int ch, enum EDI_SUM id)
{
	unsigned int tsize;

	tsize = ARRAY_SIZE(di_sum_tab);

	if (id >= tsize) {
		PR_ERR("%s:err:overflow:tsize[%d],id[%d]\n",
		       __func__, tsize, id);
		return false;
	}
	if (di_sum_tab[id].index != id) {
		PR_ERR("%s:err:table:id[%d],tab_id[%d]\n",
		       __func__, id, di_sum_tab[id].index);
		return false;
	}
	return true;
}

void di_sum_reg_init(unsigned int ch)
{
	unsigned int tsize;
	int i;

	tsize = ARRAY_SIZE(di_sum_tab);

	dbg_init("%s:ch[%d]\n", __func__, ch);
	for (i = 0; i < tsize; i++) {
		if (!di_sum_check(ch, i))
			continue;
		dbg_init("\t:%d:name:%s,%d\n", i, di_sum_tab[i].name,
			 di_sum_tab[i].default_val);
		di_sum_set_l(ch, i, di_sum_tab[i].default_val);
	}
}

void di_sum_get_info(unsigned int ch,  enum EDI_SUM id, char **name,
		     unsigned int *pval)
{
	*name = di_sum_tab[id].name;
	*pval = di_sum_get(ch, id);
}

void di_sum_set(unsigned int ch, enum EDI_SUM id, unsigned int val)
{
	if (!di_sum_check(ch, id))
		return;

	di_sum_set_l(ch, id, val);
}

unsigned int di_sum_inc(unsigned int ch, enum EDI_SUM id)
{
	if (!di_sum_check(ch, id))
		return 0;
	return di_sum_inc_l(ch, id);
}

unsigned int di_sum_get(unsigned int ch, enum EDI_SUM id)
{
	if (!di_sum_check(ch, id))
		return 0;
	return di_sum_get_l(ch, id);
}

void dim_sumx_clear(unsigned int ch)
{
	struct dim_sum_s *psumx = get_sumx(ch);

	memset(psumx, 0, sizeof(*psumx));
}

void dim_sumx_set(unsigned int ch)
{
	struct dim_sum_s *psumx = get_sumx(ch);

	psumx->b_pre_free	= list_count(ch, QUEUE_LOCAL_FREE);
	psumx->b_pre_ready	= di_que_list_count(ch, QUE_PRE_READY);
	psumx->b_pst_free	= di_que_list_count(ch, QUE_POST_FREE);
	psumx->b_pst_ready	= di_que_list_count(ch, QUE_POST_READY);
	psumx->b_recyc		= list_count(ch, QUEUE_RECYCLE);
	psumx->b_display	= list_count(ch, QUEUE_DISPLAY);
}

/****************************/
/*call by event*/
/****************************/
void dip_even_reg_init_val(unsigned int ch)
{
}

void dip_even_unreg_val(unsigned int ch)
{
}

/****************************/
static void dip_cma_init_val(void)
{
	unsigned int ch;
	struct di_mng_s *pbm = get_bufmng();

	for (ch = 0; ch < DI_CHANNEL_NUB; ch++) {
		/* CMA state */
		atomic_set(&pbm->cma_mem_state[ch], EDI_CMA_ST_IDL);

		/* CMA reg/unreg cmd */
		/* pbm->cma_reg_cmd[ch] = 0; */
		pbm->cma_wqsts[ch] = 0;
	}
	pbm->cma_flg_run = 0;
}

void dip_cma_close(void)
{
	unsigned int ch;
	struct di_mng_s *pbm = get_bufmng();

	if (dip_cma_st_is_idl_all())
		return;

	for (ch = 0; ch < DI_CHANNEL_NUB; ch++) {
#ifdef MARK_SC2
		if (!dip_cma_st_is_idle(ch)) {
			dim_cma_top_release(ch);
			pr_info("%s:force release ch[%d]", __func__, ch);
			atomic_set(&pbm->cma_mem_state[ch], EDI_CMA_ST_IDL);

			/*pbm->cma_reg_cmd[ch] = 0;*/
			pbm->cma_wqsts[ch] = 0;
		}
#endif
	}
	pbm->cma_flg_run = 0;
}

#ifdef MARK_SC2
static void dip_wq_cma_handler(struct work_struct *work)
{
	struct di_mng_s *pbm = get_bufmng();
	enum EDI_CMA_ST cma_st;
	bool do_flg;
	struct dim_wq_s *wq;
	ulong flags = 0;

	unsigned int ch;
	unsigned int cma_cmd;

	spin_lock_irqsave(&plist_lock, flags);
	pbm->cma_flg_run |= DI_BIT0;
	wq = container_of(work, struct dim_wq_s, wq_work);
	do_flg = false;
	ch = wq->ch;
	cma_cmd = wq->cmd;
	spin_unlock_irqrestore(&plist_lock, flags);
	cma_st = dip_cma_get_st(ch);

	dbg_wq("%s:ch[%d],cmd[%d],st[%d][%d]\n",
	       "k:h:", ch, cma_cmd, cma_st, wq->cnt);

	/* release alloc one ******************************/
	if (cma_cmd == ECMA_CMD_ONE_RE_AL) {
		dpst_cma_re_alloc_re_alloc(ch);

		spin_lock_irqsave(&plist_lock, flags);
		pbm->cma_flg_run &= (~DI_BIT0);
		pbm->cma_flg_run &= (~DI_BIT3);
		task_send_ready();
		spin_unlock_irqrestore(&plist_lock, flags);
		return;
	} else if (cma_cmd == ECMA_CMD_ONE_RELEAS) {
		dpst_cma_r_back_unreg(ch);

		spin_lock_irqsave(&plist_lock, flags);
		pbm->cma_flg_run &= (~DI_BIT0);
		pbm->cma_flg_run &= (~DI_BIT3);
		task_send_ready();
		spin_unlock_irqrestore(&plist_lock, flags);
		return;
	}
	/**************************************************/
	switch (cma_st) {
	case EDI_CMA_ST_IDL:
		if (cma_cmd == ECMA_CMD_ALLOC) {
			do_flg = true;
			/*set:alloc:*/
			atomic_set(&pbm->cma_mem_state[ch], EDI_CMA_ST_ALLOC);
			if (dim_cma_top_alloc(ch)) {
				atomic_set(&pbm->cma_mem_state[ch],
					   EDI_CMA_ST_READY);
			}
		}
		break;
	case EDI_CMA_ST_READY:
		if ((cma_cmd == ECMA_CMD_RELEASE) ||
		    (cma_cmd == ECMA_CMD_BACK)) {
			do_flg = true;
			atomic_set(&pbm->cma_mem_state[ch],
				   EDI_CMA_ST_RELEASE);
			dim_cma_top_release(ch);
			if (di_que_is_empty(ch, QUE_POST_KEEP))
				atomic_set(&pbm->cma_mem_state[ch],
					   EDI_CMA_ST_IDL);
			else
				atomic_set(&pbm->cma_mem_state[ch],
					   EDI_CMA_ST_PART);
			dpst_cma_re_alloc_unreg(ch);
		}
		break;
	case EDI_CMA_ST_PART:
		if (cma_cmd == ECMA_CMD_ALLOC) {
			do_flg = true;
			/*set:alloc:*/
			atomic_set(&pbm->cma_mem_state[ch], EDI_CMA_ST_ALLOC);
			if (dim_cma_top_alloc(ch)) {
				atomic_set(&pbm->cma_mem_state[ch],
					   EDI_CMA_ST_READY);
			}
		} else if ((cma_cmd == ECMA_CMD_RELEASE) ||
			   (cma_cmd == ECMA_CMD_BACK)) {
			do_flg = true;
			atomic_set(&pbm->cma_mem_state[ch],
				   EDI_CMA_ST_RELEASE);
			dim_cma_top_release(ch);
			if (di_que_is_empty(ch, QUE_POST_KEEP))
				atomic_set(&pbm->cma_mem_state[ch],
					   EDI_CMA_ST_IDL);
			else
				atomic_set(&pbm->cma_mem_state[ch],
					   EDI_CMA_ST_PART);
		}

		break;
	case EDI_CMA_ST_ALLOC:	/*do*/
	case EDI_CMA_ST_RELEASE:/*do*/
	default:
		break;
	}
	if (!do_flg)
		PR_INF("\tch[%d],do nothing[%d]\n", ch, cma_st);

	dbg_wq("%s:end\n", __func__);
	spin_lock_irqsave(&plist_lock, flags);
	pbm->cma_flg_run &= (~DI_BIT0);
	pbm->cma_flg_run &= (~DI_BIT3);
	task_send_ready();
	spin_unlock_irqrestore(&plist_lock, flags);
}

static void dip_wq_prob(void)
{
	struct di_mng_s *pbm = get_bufmng();

	pbm->wq.wq_cma = create_singlethread_workqueue("deinterlace");
	INIT_WORK(&pbm->wq.wq_work, dip_wq_cma_handler);
}

static void dip_wq_ext(void)
{
	struct di_mng_s *pbm = get_bufmng();

	cancel_work_sync(&pbm->wq.wq_work);
	destroy_workqueue(pbm->wq.wq_cma);
	pr_info("%s:finish\n", __func__);
}
#endif
#ifdef MARK_SC2
static void dip_wq_check(unsigned int ch)
{
	struct di_mng_s *pbm = get_bufmng();
	enum EDI_CMA_ST cma_st;

	/* for reg */
	if (pbm->cma_wqsts[ch] & DI_BIT0) {
		PR_WARN("k:cma_check:ch[%d] release lost\n", ch);
		pbm->cma_wqsts[ch] &= (~DI_BIT0);
	}
	if (pbm->cma_wqsts[ch] & DI_BIT1) {
		pbm->cma_wqsts[ch] &= (~DI_BIT1);
		cma_st = dip_cma_get_st(ch);
		if (cma_st == EDI_CMA_ST_IDL ||
		    cma_st == EDI_CMA_ST_PART) {
			dip_wq_cma_run(ch, ECMA_CMD_ALLOC);
			PR_WARN("k:cma_re:ch[%d]\n", ch);
		}
	}
}

void dip_wq_check_unreg(unsigned int ch)
{
	struct di_mng_s *pbm = get_bufmng();

	if (pbm->cma_wqsts[ch] & DI_BIT0) {
		PR_WARN("k:unreg\n");
		/* after unreg */
		cancel_work_sync(&pbm->wq.wq_work);
		pbm->cma_wqsts[ch] &= (~DI_BIT0);
		dip_wq_cma_run(ch, ECMA_CMD_RELEASE);
	}
}

void dip_wq_cma_run(unsigned char ch, unsigned int reg_cmd)
{
	struct di_mng_s *pbm = get_bufmng();
	bool ret = true;

	dbg_wq("%s:ch[%d] [%d][%d]\n",
	       "k:u", ch, reg_cmd, pbm->cma_wqcnt);
	pbm->cma_flg_run |= DI_BIT2;
	if (pbm->cma_flg_run & DI_BIT0) {
		if (reg_cmd == ECMA_CMD_ALLOC)
			pbm->cma_wqsts[ch] |= DI_BIT1;
		else if (reg_cmd == ECMA_CMD_RELEASE)
			pbm->cma_wqsts[ch] |= DI_BIT0;
		else if (reg_cmd == ECMA_CMD_ONE_RE_AL)
			pbm->cma_wqsts[ch] |= DI_BIT3;
		else if (reg_cmd == ECMA_CMD_ONE_RELEAS)
			pbm->cma_wqsts[ch] |= DI_BIT4;
		PR_WARN("k:cma:2 ch[%d][%d]\n", ch, reg_cmd);
		pbm->cma_flg_run &= (~DI_BIT2);
		return;
	}

	pbm->wq.cmd = reg_cmd;

	pbm->wq.ch = ch;
	pbm->wq.cnt = pbm->cma_wqcnt;
	ret = queue_work(pbm->wq.wq_cma, &pbm->wq.wq_work);
	if (!ret) {
		PR_WARN("k:cma:ch[%d] [%d]\n", ch, reg_cmd);
		if (reg_cmd == ECMA_CMD_ALLOC)
			pbm->cma_wqsts[ch] |= DI_BIT1;
		else if (reg_cmd == ECMA_CMD_RELEASE)
			pbm->cma_wqsts[ch] |= DI_BIT0;
		else if (reg_cmd == ECMA_CMD_ONE_RE_AL)
			pbm->cma_wqsts[ch] |= DI_BIT3;
		else if (reg_cmd == ECMA_CMD_ONE_RELEAS)
			pbm->cma_wqsts[ch] |= DI_BIT4;
	} else {
		pbm->cma_flg_run |= DI_BIT3;
	}
	pbm->cma_wqcnt++;
	pbm->cma_flg_run &= (~DI_BIT2);
	dbg_wq("%s:end\n", "k:u");
}

bool dip_cma_st_is_ready(unsigned int ch)
{
	struct di_mng_s *pbm = get_bufmng();
	bool ret = false;

	if (atomic_read(&pbm->cma_mem_state[ch]) == EDI_CMA_ST_READY)
		ret = true;

	return ret;
}
#endif

bool dip_cma_st_is_idle(unsigned int ch)
{
	struct di_mng_s *pbm = get_bufmng();
	bool ret = false;

	if (atomic_read(&pbm->cma_mem_state[ch]) == EDI_CMA_ST_IDL)
		ret = true;

	return ret;
}

bool dip_cma_st_is_idl_all(void)
{
	unsigned int ch;
	struct di_mng_s *pbm = get_bufmng();
	bool ret = true;

	for (ch = 0; ch < DI_CHANNEL_NUB; ch++) {
		if (atomic_read(&pbm->cma_mem_state[ch]) != EDI_CMA_ST_IDL) {
			ret = true;
			break;
		}
	}
	return ret;
}

enum EDI_CMA_ST dip_cma_get_st(unsigned int ch)
{
	struct di_mng_s *pbm = get_bufmng();

	return atomic_read(&pbm->cma_mem_state[ch]);
}

const char * const di_cma_state_name[] = {
	"IDLE",
	"do_alloc",
	"READY",
	"do_release",
	"PART",
};

const char *di_cma_dbg_get_st_name(unsigned int ch)
{
	enum EDI_CMA_ST st = dip_cma_get_st(ch);
	const char *p = "overflow";

	if (st < ARRAY_SIZE(di_cma_state_name))
		p = di_cma_state_name[st];
	return p;
}

void dip_cma_st_set_ready_all(void)
{
	unsigned int ch;
	struct di_mng_s *pbm = get_bufmng();

	for (ch = 0; ch < DI_CHANNEL_NUB; ch++)
		atomic_set(&pbm->cma_mem_state[ch], EDI_CMA_ST_READY);
}

/****************************/
/*channel STATE*/
/****************************/
void dip_chst_set(unsigned int ch, enum EDI_TOP_STATE chst)
{
	struct di_mng_s *pbm = get_bufmng();

	atomic_set(&pbm->ch_state[ch], chst);
}

enum EDI_TOP_STATE dip_chst_get(unsigned int ch)
{
	struct di_mng_s *pbm = get_bufmng();

	return atomic_read(&pbm->ch_state[ch]);
}

void dip_chst_init(void)
{
	unsigned int ch;

	for (ch = 0; ch < DI_CHANNEL_NUB; ch++)
		dip_chst_set(ch, EDI_TOP_STATE_IDLE);
}

static void dip_process_reg_after(struct di_ch_s *pch);
//ol dim_process_reg(struct di_ch_s *pch);
//ol dim_process_unreg(struct di_ch_s *pch);

void dip_chst_process_ch(void)
{
	unsigned int ch;
	unsigned int chst;
	struct vframe_s *vframe;
	struct di_pre_stru_s *ppre;// = get_pre_stru(ch);
//	struct di_mng_s *pbm = get_bufmng();
	ulong flags = 0;
	struct di_ch_s *pch;
	struct di_mng_s *pbm = get_bufmng();

	for (ch = 0; ch < DI_CHANNEL_NUB; ch++) {
		pch = get_chdata(ch);
		if (atomic_read(&pbm->trig_reg[ch]))
			dim_process_reg(pch);
		if (atomic_read(&pbm->trig_unreg[ch]))
			dim_process_unreg(pch);

		dip_process_reg_after(pch);

		chst = dip_chst_get(ch);
		ppre = get_pre_stru(ch);
		task_polling_cmd_keep(ch, chst);

		switch (chst) {
		case EDI_TOP_STATE_REG_STEP2:

			break;
		case EDI_TOP_STATE_UNREG_STEP1:

			break;
		case EDI_TOP_STATE_UNREG_STEP2:

			break;
		case EDI_TOP_STATE_READY:
			spin_lock_irqsave(&plist_lock, flags);
			dim_post_keep_back_recycle(ch);
			spin_unlock_irqrestore(&plist_lock, flags);
			dim_sumx_set(ch);
#ifdef SC2_NEW_FLOW
			ins_in_vf(pch);
#endif
			break;
		case EDI_TOP_STATE_BYPASS:
			vframe = pw_vf_peek(ch);
			if (!vframe)
				break;
			if (dim_need_bypass(ch, vframe))
				break;

			di_reg_variable(ch, vframe);
			if (!ppre->bypass_flag) {
				set_bypass2_complete(ch, false);
				/*this will cause first local buf not alloc*/
				/*dim_bypass_first_frame(ch);*/
				dip_chst_set(ch, EDI_TOP_STATE_REG_STEP2);
			}
			break;

		default:
			break;
		}
	}
}

#ifdef MARK_SC2
bool dip_chst_change_2unreg(void)
{
	unsigned int ch;
	unsigned int chst;
	bool ret = false;

	for (ch = 0; ch < DI_CHANNEL_NUB; ch++) {
		chst = dip_chst_get(ch);
		//dbg_poll("[%d]%d\n", ch, chst);
		if (chst == EDI_TOP_STATE_UNREG_STEP1) {
			//dbg_reg("%s:ch[%d]to UNREG_STEP2\n", __func__, ch);
			set_reg_flag(ch, false);
			dip_chst_set(ch, EDI_TOP_STATE_UNREG_STEP2);
			ret = true;
		}
	}
	return ret;
}
#endif
/************************************************
 * new reg and unreg
 ***********************************************/
/************************************************
 * dim_api_reg
 *	block
 ***********************************************/

bool dim_api_reg(enum DIME_REG_MODE rmode, struct di_ch_s *pch)
{
	unsigned int ch;
	struct di_mng_s *pbm = get_bufmng();
	bool ret = false;
	unsigned int cnt;
	struct di_dev_s *de_devp = get_dim_de_devp();

	if (!pch) {
#ifdef PRINT_BASIC
		PR_ERR("%s:no pch\n", __func__);
#endif
		return false;
	}
	ch = pch->ch_id;

	dip_even_reg_init_val(ch);
	if (de_devp->flags & DI_SUSPEND_FLAG) {
#ifdef PRINT_BASIC
		PR_ERR("reg event device hasn't resumed\n");
#endif
		return false;
	}
	if (get_reg_flag(ch)) {
		PR_ERR("no muti instance.\n");
		return false;
	}
	task_delay(50);

	di_bypass_state_set(ch, false);

	atomic_set(&pbm->trig_reg[ch], 1);

	dbg_timer(ch, EDBG_TIMER_REG_B);
	task_send_ready();

	cnt = 0; /* 500us x 10 = 5ms */
	while (atomic_read(&pbm->trig_reg[ch]) && cnt < 10) {
		usleep_range(500, 501);
		cnt++;
	}

	task_send_ready();

	cnt = 0; /* 3ms x 2000 = 6s */
	while (atomic_read(&pbm->trig_reg[ch]) && cnt < 2000) {
		usleep_range(3000, 3001);
		cnt++;
	}

	if (!atomic_read(&pbm->trig_reg[ch]))
		ret = true;
	else
		PR_ERR("%s:ch[%d]:failed\n", __func__, ch);

	dbg_timer(ch, EDBG_TIMER_REG_E);
	dbg_ev("ch[%d]:reg end\n", ch);
	return ret;
}

void dim_trig_unreg(unsigned int ch)
{
	struct di_mng_s *pbm = get_bufmng();

	atomic_set(&pbm->trig_unreg[ch], 1);
}

bool dim_api_unreg(enum DIME_REG_MODE rmode, struct di_ch_s *pch)
{
	unsigned int ch;
	struct di_mng_s *pbm = get_bufmng();
	bool ret = false;
	unsigned int cnt;

	if (!pch) {
		//PR_ERR("%s:no pch\n", __func__);
		return false;
	}
	ch = pch->ch_id;

	/*dbg_ev("%s:unreg\n", __func__);*/
	dip_even_unreg_val(ch);	/*new*/
	di_bypass_state_set(ch, true);

	dbg_timer(ch, EDBG_TIMER_UNREG_B);
	task_delay(100);

	task_send_ready();

	cnt = 0; /* 500us x 10 = 5ms */
	while (atomic_read(&pbm->trig_unreg[ch]) && cnt < 10) {
		usleep_range(500, 501);
		cnt++;
	}

	task_send_ready();

	cnt = 0; /* 3ms x 2000 = 6s */
	while (atomic_read(&pbm->trig_unreg[ch]) && cnt < 2000) {
		usleep_range(3000, 3001);
		cnt++;
	}

	if (!atomic_read(&pbm->trig_unreg[ch]))
		ret = true;
	else
		PR_ERR("%s:ch[%d]:failed\n", __func__, ch);

	dbg_timer(ch, EDBG_TIMER_UNREG_E);
	//dbg_ev("ch[%d]unreg end\n", ch);

	return ret;
}

/* from dip_event_reg_chst */
bool dim_process_reg(struct di_ch_s *pch)
{
	enum EDI_TOP_STATE chst;
	unsigned int ch = pch->ch_id;
//	struct di_pre_stru_s *ppre = get_pre_stru(ch);
	bool ret = false;
	struct di_mng_s *pbm = get_bufmng();

	dbg_dbg("%s:ch[%d]\n", __func__, ch);

	chst = dip_chst_get(ch);
	//trace_printk("%s,%d\n", __func__, chst);

	switch (chst) {
	case EDI_TOP_STATE_IDLE:
		queue_init2(ch);
		di_que_init(ch);
		//move to event di_vframe_reg(ch);

		dip_chst_set(ch, EDI_TOP_STATE_REG_STEP1);
		task_send_cmd(LCMD1(ECMD_REG, ch));
		ret = true;
		dbg_dbg("reg ok\n");
		break;
	case EDI_TOP_STATE_REG_STEP1:
	case EDI_TOP_STATE_REG_STEP1_P1:
	case EDI_TOP_STATE_REG_STEP2:
	case EDI_TOP_STATE_READY:
	case EDI_TOP_STATE_BYPASS:
		PR_WARN("have reg\n");
		ret = true;
		break;
	case EDI_TOP_STATE_UNREG_STEP1:
	case EDI_TOP_STATE_UNREG_STEP2:
		PR_ERR("%s:in UNREG_STEP1/2\n", __func__);
		ret = false;
		break;
	case EDI_TOP_STATE_NOPROB:
	default:
		ret = false;
		PR_ERR("err: not prob[%d]\n", chst);

		break;
	}

	atomic_set(&pbm->trig_reg[ch], 0);
	//trace_printk("%s end\n", __func__);
	return ret;
}

/*from: dip_event_unreg_chst*/
bool dim_process_unreg(struct di_ch_s *pch)
{
	enum EDI_TOP_STATE chst, chst2;
	unsigned int ch = pch->ch_id;
	struct di_pre_stru_s *ppre = get_pre_stru(ch);
	bool ret = false;
	struct di_mng_s *pbm = get_bufmng();

	chst = dip_chst_get(ch);
	dbg_reg("%s:ch[%d]:%s\n", __func__, ch, dip_chst_get_name(chst));

	if (chst > EDI_TOP_STATE_NOPROB)
		set_flag_trig_unreg(ch, true);

	//trace_printk("%s:%d\n", __func__, chst);
	switch (chst) {
	case EDI_TOP_STATE_READY:
		//move di_vframe_unreg(ch);
		/*trig unreg*/
		dip_chst_set(ch, EDI_TOP_STATE_UNREG_STEP1);
		//task_send_cmd(LCMD1(ECMD_UNREG, ch));
		/*debug only di_dbg = di_dbg|DBG_M_TSK;*/

		/*wait*/
		ppre->unreg_req_flag_cnt = 0;
		chst2 = dip_chst_get(ch);

		/*debug only di_dbg = di_dbg & (~DBG_M_TSK);*/
		dbg_reg("%s:ch[%d] ready end\n", __func__, ch);
		task_delay(100);

		break;
	case EDI_TOP_STATE_BYPASS:
		/*from bypass complet to unreg*/
		//move di_vframe_unreg(ch);
		di_unreg_variable(ch);

		set_reg_flag(ch, false);
		set_reg_setting(ch, false);
		if ((!get_reg_flag_all()) &&
		    (!get_reg_setting_all())) {
			dbg_pl("ch[%d]:unreg1,bypass:\n", ch);
			di_unreg_setting();
			dpre_init();
			dpost_init();
		}
		dip_chst_set(ch, EDI_TOP_STATE_IDLE);
		ret = true;

		break;
	case EDI_TOP_STATE_IDLE:
		PR_WARN("have unreg\n");
		ret = true;
		break;
	case EDI_TOP_STATE_REG_STEP2:
		di_unreg_variable(ch);
		set_reg_flag(ch, false);
		set_reg_setting(ch, false);
		if ((!get_reg_flag_all()) &&
		    (!get_reg_setting_all())) {
			dbg_pl("ch[%d]:unreg2,step2:\n", ch);
			di_unreg_setting();
			dpre_init();
			dpost_init();
		}
		/*note: no break;*/
	case EDI_TOP_STATE_REG_STEP1:
	case EDI_TOP_STATE_REG_STEP1_P1:
		dbg_dbg("%s:in reg step1\n", __func__);
		//move di_vframe_unreg(ch);
		set_reg_flag(ch, false);
		dip_chst_set(ch, EDI_TOP_STATE_IDLE);

		ret = true;
		break;
	case EDI_TOP_STATE_UNREG_STEP1:
		if (dpre_can_exit(ch) && dpst_can_exit(ch)) {
			dip_chst_set(ch, EDI_TOP_STATE_UNREG_STEP2);
			set_reg_flag(ch, false);
			set_reg_setting(ch, false);
			//reflesh = true;
		}
		break;
	case EDI_TOP_STATE_UNREG_STEP2:
		di_unreg_variable(ch);
		if ((!get_reg_flag_all()) &&
		    (!get_reg_setting_all())) {
			dbg_pl("ch[%d]:unreg3,step2:\n", ch);
			di_unreg_setting();
			dpre_init();
			dpost_init();
		}

		dip_chst_set(ch, EDI_TOP_STATE_IDLE);
		ret = true;
		break;
	case EDI_TOP_STATE_NOPROB:
	default:
		PR_ERR("err: not prob[%d]\n", chst);
		ret = true;
		break;
	}

	if (ret) {
		task_delay(1);
		//trace_printk("%s end\n", __func__);
		dbg_pl("unreg:%s\n", dip_chst_get_name(chst));
		atomic_set(&pbm->trig_unreg[ch], 0);
	} else {
		//trace_printk("%s step end\n", __func__);
	}

	return ret;
}

/* from dip_chst_process_reg */
static void dip_process_reg_after(struct di_ch_s *pch)
{
	enum EDI_TOP_STATE chst;
	struct vframe_s *vframe;
	unsigned int ch = pch->ch_id;
	struct di_pre_stru_s *ppre = get_pre_stru(ch);
	bool reflesh = true;
	struct di_mng_s *pbm = get_bufmng();
//	ulong flags = 0;

	while (reflesh) {
		reflesh = false;

	chst = dip_chst_get(ch);

	//dbg_reg("%s:ch[%d]%s\n", __func__, ch, dip_chst_get_name(chst));

	switch (chst) {
	case EDI_TOP_STATE_NOPROB:
	case EDI_TOP_STATE_IDLE:
		break;
	case EDI_TOP_STATE_REG_STEP1:/*wait peek*/
		vframe = pw_vf_peek(ch);

		if (vframe) {
			dbg_timer(ch, EDBG_TIMER_FIRST_GET);
			dim_tr_ops.pre_get(vframe->index_disp);

			set_flag_trig_unreg(ch, false);

			dip_chst_set(ch, EDI_TOP_STATE_REG_STEP1_P1);

			reflesh = true;
			//trace_printk("step1:peek\n");
		}
		break;
	case EDI_TOP_STATE_REG_STEP1_P1:
		vframe = pw_vf_peek(ch);
		if (!vframe) {
			PR_ERR("%s:p1 vfm nop\n", __func__);
			dip_chst_set(ch, EDI_TOP_STATE_REG_STEP1);
			reflesh = true;
			break;
		}
		if (pbm->cma_flg_run & DI_BIT0)
			break;

		di_reg_variable(ch, vframe);
		/*di_reg_process_irq(ch);*/ /*check if bypass*/
		set_reg_setting(ch, true);

		/*?how  about bypass ?*/
		if (ppre->bypass_flag) {
			/* complete bypass */
			set_bypass2_complete(ch, true);
			if (!get_reg_flag_all()) {
				/*first channel reg*/
				dpre_init();
				dpost_init();
				di_reg_setting(ch, vframe);
			}
			dip_chst_set(ch, EDI_TOP_STATE_BYPASS);
			set_reg_flag(ch, true);
		} else {
			set_bypass2_complete(ch, false);
			if (!get_reg_flag_all()) {
				/*first channel reg*/
				dpre_init();
				dpost_init();
				di_reg_setting(ch, vframe);
			}
			/*this will cause first local buf not alloc*/
			/*dim_bypass_first_frame(ch);*/
			dip_chst_set(ch, EDI_TOP_STATE_REG_STEP2);
			/*set_reg_flag(ch, true);*/
		}
		dbg_reg("%s:p1 end\n", __func__);
		reflesh = true;
		break;
	case EDI_TOP_STATE_REG_STEP2:/**/

		pch = get_chdata(ch);
#ifdef	SC2_NEW_FLOW
		if (memn_get(pch)) {
#else
		//if (mem_cfg(pch)) {
		mem_cfg_pre(pch);
		if (di_pst_afbct_check(pch) &&
		    di_i_dat_check(pch)		&&
		    mem_alloc_check(pch)) {
			mem_cfg(pch);
#endif
			mem_cfg_realloc_wait(pch);
			if (di_cfg_top_get(EDI_CFG_FIRST_BYPASS)) {
				if (get_sum_g(ch) == 0)
					dim_bypass_first_frame(ch);
				else
					PR_INF("ch[%d],g[%d]\n",
					       ch, get_sum_g(ch));
			}
			dip_chst_set(ch, EDI_TOP_STATE_READY);
			set_reg_flag(ch, true);
		}

		break;
	case EDI_TOP_STATE_READY:

		break;
	case EDI_TOP_STATE_BYPASS:
	case EDI_TOP_STATE_UNREG_STEP1:
	case EDI_TOP_STATE_UNREG_STEP2:
		/*do nothing;*/
		break;
	}
	}
}

void dip_hw_process(void)
{
	di_dbg_task_flg = 5;
	dpre_process();
	di_dbg_task_flg = 6;
	pre_mode_setting();
	di_dbg_task_flg = 7;
	dpst_process();
	di_dbg_task_flg = 8;
}

const char * const di_top_state_name[] = {
	"NOPROB",
	"IDLE",
	"REG_STEP1",
	"REG_P1",
	"REG_STEP2",
	"READY",
	"BYPASS",
	"UNREG_STEP1",
	"UNREG_STEP2",
};

const char *dip_chst_get_name_curr(unsigned int ch)
{
	const char *p = "";
	enum EDI_TOP_STATE chst;

	chst = dip_chst_get(ch);

	if (chst < ARRAY_SIZE(di_top_state_name))
		p = di_top_state_name[chst];

	return p;
}

const char *dip_chst_get_name(enum EDI_TOP_STATE chst)
{
	const char *p = "";

	if (chst < ARRAY_SIZE(di_top_state_name))
		p = di_top_state_name[chst];

	return p;
}

static const struct di_mm_cfg_s c_mm_cfg_normal = {
	.di_h	=	1088,
	.di_w	=	1920,
	.num_local	=	MAX_LOCAL_BUF_NUM,
	.num_post	=	POST_BUF_NUM,
	.num_step1_post = 1,
};

static const struct di_mm_cfg_s c_mm_cfg_4k = {
	.di_h	=	2160,
	.di_w	=	3840,
	.num_local	=	0,
	.num_post	=	POST_BUF_NUM,
	.num_step1_post = 1,
};

static const struct di_mm_cfg_s c_mm_cfg_fix_4k = {
	.di_h	=	2160,
	.di_w	=	3840,
	.num_local	=	MAX_LOCAL_BUF_NUM,
	.num_post	=	POST_BUF_NUM,
	.num_step1_post = 1,
};

static const struct di_mm_cfg_s c_mm_cfg_bypass = {
	.di_h	=	2160,
	.di_w	=	3840,
	.num_local	=	0,
	.num_post	=	0,
	.num_step1_post = 0,
};

const struct di_mm_cfg_s *di_get_mm_tab(unsigned int is_4k)
{
	if (is_4k)
		return &c_mm_cfg_4k;
	else
		return &c_mm_cfg_normal;
}

/**********************************/
/* TIME OUT CHEKC api*/
/**********************************/

void di_tout_int(struct di_time_out_s *tout, unsigned int thd)
{
	tout->en = false;
	tout->timer_start = 0;
	tout->timer_thd = thd;
}

bool di_tout_contr(enum EDI_TOUT_CONTR cmd, struct di_time_out_s *tout)
{
	unsigned long ctimer;
	unsigned long diff;
	bool ret = false;

	ctimer = cur_to_msecs();

	switch (cmd) {
	case EDI_TOUT_CONTR_EN:
		tout->en = true;
		tout->timer_start = ctimer;
		break;
	case EDI_TOUT_CONTR_FINISH:
		if (tout->en) {
			diff = ctimer - tout->timer_start;

			if (diff > tout->timer_thd) {
				tout->over_flow_cnt++;

				if (tout->over_flow_cnt > 0xfffffff0) {
					tout->over_flow_cnt = 0;
					tout->flg_over = 1;
				}
		#ifdef MARK_HIS
				if (tout->do_func)
					tout->do_func();

		#endif
				ret = true;
			}
			tout->en = false;
		}
		break;

	case EDI_TOUT_CONTR_CHECK:	/*if time is overflow, disable timer*/
		if (tout->en) {
			diff = ctimer - tout->timer_start;

			if (diff > tout->timer_thd) {
				tout->over_flow_cnt++;

				if (tout->over_flow_cnt > 0xfffffff0) {
					tout->over_flow_cnt = 0;
					tout->flg_over = 1;
				}
				#ifdef MARK_HIS
				if (tout->do_func)
					tout->do_func();

				#endif
				ret = true;
				tout->en = false;
			}
		}
		break;
	case EDI_TOUT_CONTR_CLEAR:
		tout->en = false;
		tout->timer_start = ctimer;
		break;
	case EDI_TOUT_CONTR_RESET:
		tout->en = true;
		tout->timer_start = ctimer;
		break;
	}

	return ret;
}

const unsigned int di_ch2mask_table[DI_CHANNEL_MAX] = {
	DI_BIT0,
	DI_BIT1,
	DI_BIT2,
	DI_BIT3,
};

/****************************************
 *bit control
 ****************************************/
static const unsigned int bit_tab[32] = {
	DI_BIT0,
	DI_BIT1,
	DI_BIT2,
	DI_BIT3,
	DI_BIT4,
	DI_BIT5,
	DI_BIT6,
	DI_BIT7,
	DI_BIT8,
	DI_BIT9,
	DI_BIT10,
	DI_BIT11,
	DI_BIT12,
	DI_BIT13,
	DI_BIT14,
	DI_BIT15,
	DI_BIT16,
	DI_BIT17,
	DI_BIT18,
	DI_BIT19,
	DI_BIT20,
	DI_BIT21,
	DI_BIT22,
	DI_BIT23,
	DI_BIT24,
	DI_BIT25,
	DI_BIT26,
	DI_BIT27,
	DI_BIT28,
	DI_BIT29,
	DI_BIT30,
	DI_BIT31,
};

void bset(unsigned int *p, unsigned int bitn)
{
	*p = *p | bit_tab[bitn];
}

void bclr(unsigned int *p, unsigned int bitn)
{
	*p = *p & (~bit_tab[bitn]);
}

bool bget(unsigned int *p, unsigned int bitn)
{
	return (*p & bit_tab[bitn]) ? true : false;
}

/****************************************/
/* do_table				*/
/****************************************/

/*for do_table_working*/
#define K_DO_TABLE_LOOP_MAX	15

const struct do_table_s do_table_def = {
	.ptab = NULL,
	.data = NULL,
	.size = 0,
	.op_lst = K_DO_TABLE_ID_STOP,
	.op_crr = K_DO_TABLE_ID_STOP,
	.do_stop = 0,
	.flg_stop = 0,
	.do_pause = 0,
	.do_step = 0,
	.flg_repeat = 0,

};

void do_table_init(struct do_table_s *pdo,
		   const struct do_table_ops_s *ptable,
		   unsigned int size_tab)
{
	memcpy(pdo, &do_table_def, sizeof(struct do_table_s));

	if (ptable) {
		pdo->ptab = ptable;
		pdo->size = size_tab;
	}
}

/*if change to async?*/
/* now only call in same thread */
void do_talbe_cmd(struct do_table_s *pdo, enum EDO_TABLE_CMD cmd)
{
	switch (cmd) {
	case EDO_TABLE_CMD_NONE:
		pr_info("test:%s\n", __func__);
		break;
	case EDO_TABLE_CMD_STOP:
		pdo->do_stop = true;
		break;
	case EDO_TABLE_CMD_START:
		if (pdo->op_crr == K_DO_TABLE_ID_STOP) {
			pdo->op_lst = pdo->op_crr;
			pdo->op_crr = K_DO_TABLE_ID_START;
			pdo->do_stop = false;
			pdo->flg_stop = false;
		} else if (pdo->op_crr == K_DO_TABLE_ID_PAUSE) {
			pdo->op_crr = pdo->op_lst;
			pdo->op_lst = K_DO_TABLE_ID_PAUSE;
			pdo->do_pause = false;
		} else {
			pr_info("crr is [%d], not start\n", pdo->op_crr);
		}
		break;
	case EDO_TABLE_CMD_PAUSE:
		if (pdo->op_crr <= K_DO_TABLE_ID_STOP) {
			/*do nothing*/
		} else {
			pdo->op_lst = pdo->op_crr;
			pdo->op_crr = K_DO_TABLE_ID_PAUSE;
			pdo->do_pause = true;
		}
		break;
	case EDO_TABLE_CMD_STEP:
		pdo->do_step = true;
		break;
	case EDO_TABLE_CMD_STEP_BACK:
		pdo->do_step = false;
		break;
	default:
		break;
	}
}

bool do_table_is_crr(struct do_table_s *pdo, unsigned int state)
{
	if (pdo->op_crr == state)
		return true;
	return false;
}

void do_table_working(struct do_table_s *pdo)
{
	const struct do_table_ops_s *pcrr;
	unsigned int ret = 0;
	unsigned int next;
	bool flash = false;
	unsigned int cnt = 0;	/*proction*/
	unsigned int lst_id;	/*dbg only*/
	char *name = "";	/*dbg only*/
	bool need_pr = false;	/*dbg only*/

	if (!pdo)
		return;

	if (!pdo->ptab		||
	    pdo->op_crr >= pdo->size) {
		PR_ERR("di:err:%s:ovflow:0x%p,0x%p,crr=%d,size=%d\n",
		       __func__,
		       pdo, pdo->ptab,
		       pdo->op_crr,
		       pdo->size);
		return;
	}

	pcrr = pdo->ptab + pdo->op_crr;

	if (pdo->name)
		name = pdo->name;
	/*stop ?*/
	if (pdo->do_stop &&
	    (pcrr->mark & K_DO_TABLE_CAN_STOP)) {
		dbg_dt("%s:do stop\n", name);

		/*do stop*/
		if (pcrr->do_stop_op)
			pcrr->do_stop_op(pdo->data);
		/*set status*/
		pdo->op_lst = pdo->op_crr;
		pdo->op_crr = K_DO_TABLE_ID_STOP;
		pdo->flg_stop = true;
		pdo->do_stop = false;

		return;
	}

	/*pause?*/
	if (pdo->op_crr == K_DO_TABLE_ID_STOP	||
	    pdo->op_crr == K_DO_TABLE_ID_PAUSE)
		return;

	do {
		flash = false;
		cnt++;
		if (cnt > K_DO_TABLE_LOOP_MAX) {
			PR_ERR("di:err:%s:loop more %d\n", name, cnt);
			break;
		}

		/*stop again? */
		if (pdo->do_stop &&
		    (pcrr->mark & K_DO_TABLE_CAN_STOP)) {
			/*do stop*/
			dbg_dt("%s: do stop in loop\n", name);
			if (pcrr->do_stop_op)
				pcrr->do_stop_op(pdo->data);
			/*set status*/
			pdo->op_lst = pdo->op_crr;
			pdo->op_crr = K_DO_TABLE_ID_STOP;
			pdo->flg_stop = true;
			pdo->do_stop = false;

			break;
		}

		/*debug:*/
		lst_id = pdo->op_crr;
		need_pr = true;

		if (pcrr->con) {
			if (pcrr->con(pdo->data))
				ret = pcrr->do_op(pdo->data);
			else
				break;

		} else {
			ret = pcrr->do_op(pdo->data);
			dbg_dt("do_table:do:%d:ret=0x%x\n", pcrr->id, ret);
		}

		/*not finish, keep current status*/
		if ((ret & K_DO_TABLE_R_B_FINISH) == 0) {
			dbg_dt("%s:not finish,wait\n", __func__);
			break;
		}

		/*fix to next */
		if (ret & K_DO_TABLE_R_B_NEXT) {
			pdo->op_lst = pdo->op_crr;
			pdo->op_crr++;
			if (pdo->op_crr >= pdo->size) {
				pdo->op_crr = pdo->flg_repeat ?
					K_DO_TABLE_ID_START
					: K_DO_TABLE_ID_STOP;
				dbg_dt("%s:to end,%d\n", __func__,
				       pdo->op_crr);
				break;
			}
			/*return;*/
			flash = true;
		} else {
			next = ((ret & K_DO_TABLE_R_B_OTHER) >>
					K_DO_TABLE_R_B_OTHER_SHIFT);
			if (next < pdo->size) {
				pdo->op_lst = pdo->op_crr;
				pdo->op_crr = next;
				if (next > K_DO_TABLE_ID_STOP)
					flash = true;
				else
					flash = false;
			} else {
				PR_ERR("%s: next[%d] err:\n",
				       __func__, next);
			}
		}
		/*debug 1:*/
		need_pr = false;
		if (lst_id != pdo->op_crr) {
			dbg_dt("do_table:%s:%s->%s\n", pdo->name,
			       pdo->ptab[lst_id].name,
			       pdo->ptab[pdo->op_crr].name);
		}

		pcrr = pdo->ptab + pdo->op_crr;
	} while (flash && !pdo->do_step);

	/*debug 2:*/
	if (need_pr) {
		if (lst_id != pdo->op_crr) {
			dbg_dt("do_table2:%s:%s->%s\n", pdo->name,
			       pdo->ptab[lst_id].name,
			       pdo->ptab[pdo->op_crr].name);
		}
	}
}

/***********************************************/
void dim_bypass_st_clear(struct di_ch_s *pch)
{
	pch->bypass.d32 = 0;
}

void dim_bypass_set(struct di_ch_s *pch, bool which, unsigned int reason)
{
	bool on = false;
	struct dim_policy_s *pp = get_dpolicy();

	if (reason)
		on = true;

	if (!which) {
		if (pch->bypass.b.lst_n != on) {
			dbg_pl("ch[%d]:bypass change:n:%d->%d\n",
			       pch->ch_id,
			       pch->bypass.b.lst_n,
			       on);
			pch->bypass.b.lst_n = on;
		}
		if (on) {
			pch->bypass.b.need_bypass = 1;
			pch->bypass.b.reason_n = reason;
			pp->ch[pch->ch_id] = 0;
		} else {
			pch->bypass.b.need_bypass = 0;
			pch->bypass.b.reason_n = 0;
		}
	} else {
		if (pch->bypass.b.lst_i != on) {
			dbg_pl("ch[%d]:bypass change:i:%d->%d\n",
			       pch->ch_id,
			       pch->bypass.b.lst_i,
			       on);
			pch->bypass.b.lst_i = on;
		}
		if (on) {
			pch->bypass.b.is_bypass = 1;
			pch->bypass.b.reason_i = reason;
			pp->ch[pch->ch_id] = 0;
		} else {
			pch->bypass.b.is_bypass = 0;
			pch->bypass.b.reason_i = 0;
		}
	}
}

#define DIM_POLICY_STD_OLD	(125)
#define DIM_POLICY_STD		(250)
#define DIM_POLICY_NOT_LIMIT	(2000)
#define DIM_POLICY_SHIFT_H	(7)
#define DIM_POLICY_SHIFT_W	(6)

void dim_polic_cfg_local(unsigned int cmd, bool on)
{
	struct dim_policy_s *pp;

	if (dil_get_diffver_flag() != 1)
		return;

	pp = get_dpolicy();
	switch (cmd) {
	case K_DIM_BYPASS_CLEAR_ALL:
		pp->cfg_d32 = 0x0;
		break;
	case K_DIM_I_FIRST:
		pp->cfg_b.i_first = on;
		break;
	case K_DIM_BYPASS_ALL_P:
#ifdef TEST_DISABLE_BYPASS_P
		PR_INF("%s:get bypass p cmd, do nothing\n", __func__);
#else
		pp->cfg_b.bypass_all_p = on;
#endif
		break;
	default:
		PR_WARN("%s:cmd is overflow[%d]\n", __func__, cmd);
		break;
	}
}

//EXPORT_SYMBOL(dim_polic_cfg);

void dim_polic_prob(void)
{
	struct dim_policy_s *pp = get_dpolicy();
	struct di_dev_s *de_devp = get_dim_de_devp();

	if (DIM_IS_IC_EF(SC2) || DIM_IS_IC(TM2B) || DIM_IS_IC(T5)) {
		if (de_devp->clkb_max_rate >= 340000000)
			pp->std = DIM_POLICY_NOT_LIMIT;
		else if (de_devp->clkb_max_rate > 300000000)
			pp->std = DIM_POLICY_STD;
		else
			pp->std = DIM_POLICY_STD_OLD;
	} else if (cpu_after_eq(MESON_CPU_MAJOR_ID_G12A)) {
#ifdef TEST_DISABLE_BYPASS_P
		pp->std = DIM_POLICY_NOT_LIMIT;
#else
		if (de_devp->clkb_max_rate >= 340000000)
			pp->std = DIM_POLICY_STD;
		else
			pp->std = DIM_POLICY_STD_OLD;
#endif
	} else {
		pp->std = DIM_POLICY_STD_OLD;
	}

	pp->cfg_b.i_first = 1;
}

void dim_polic_unreg(struct di_ch_s *pch)
{
	struct dim_policy_s *pp	= get_dpolicy();
	unsigned int ch;

	ch = pch->ch_id;

	pp->ch[ch] = 0;
	pp->order_i &= ~(1 << ch);
}

unsigned int dim_polic_is_bypass(struct di_ch_s *pch, struct vframe_s *vf)
{
	struct dim_policy_s *pp	= get_dpolicy();
	unsigned int reason = 0;
	unsigned int ptt, pcu, i;
	unsigned int ch;

	ch = pch->ch_id;

	/* cfg */
	if (pp->cfg_d32) {
		if (!VFMT_IS_I(vf->type)) {
			if (dim_cfg & DI_BIT0) {
				reason = 0x60;
			} else if (pp->cfg_b.bypass_all_p) {
				reason = 0x61;
			} else if (pp->cfg_b.i_first && pp->order_i) {
				reason = 0x62;
				dbg_pl("ch[%d],bapass for order\n",
				       ch);
			}
		}
	}
	if (reason)
		return reason;

	if (!vf) {
		pr_info("no_vf\n");
		dump_stack();
		return reason;
	}
	/*count total*/
	ptt = 0;
	for (i = 0; i < DI_CHANNEL_NUB; i++)
		ptt += pp->ch[i];

	ptt -= pp->ch[ch];

	/*count current*/
	pcu = (vf->height >> DIM_POLICY_SHIFT_H) *
		(vf->width >> DIM_POLICY_SHIFT_W);
	if (VFMT_IS_I(vf->type))
		pcu >>= 1;

	/*check bypass*/
	if ((ptt + pcu) > pp->std) {
		/* bypass */
		reason = 0x63;
		pp->ch[ch] = 0;
	} else {
		pp->ch[ch] = pcu;
	}

	if (pp->cfg_b.i_first	&&
	    VFMT_IS_I(vf->type)	&&
	    reason) {
		pp->order_i |= (1 << ch);
		dbg_pl("ch[%d],bapass order[1]\n", ch);
	} else {
		pp->order_i &= ~(1 << ch);
	}
	return reason;
}

unsigned int dim_get_trick_mode(void)
{
	unsigned int trick_mode;

	if (dimp_get(edi_mp_bypass_trick_mode)) {
		int trick_mode_fffb = 0;
		int trick_mode_i = 0;

		if (dimp_get(edi_mp_bypass_trick_mode) & 0x1)
			query_video_status(0, &trick_mode_fffb);
		if (dimp_get(edi_mp_bypass_trick_mode) & 0x2)
			query_video_status(1, &trick_mode_i);
		trick_mode =
			trick_mode_fffb | (trick_mode_i << 1);

		return trick_mode;
	}
	return 0;
}

static enum EDPST_MODE dim_cnt_mode(struct di_ch_s *pch)
{
	enum EDPST_MODE mode;

	if (dim_cfg_nv21()) {
		mode = EDPST_MODE_NV21_8BIT;
	} else {
		if (dimp_get(edi_mp_nr10bit_support)) {
			if (dimp_get(edi_mp_full_422_pack))
				mode = EDPST_MODE_422_10BIT_PACK;
			else
				mode = EDPST_MODE_422_10BIT;
		} else {
			mode = EDPST_MODE_422_8BIT;
		}
	}
	return mode;
}

/****************************/
bool dip_is_support_4k(unsigned int ch)
{
	struct di_ch_s *pch = get_chdata(ch);

	if ((cfgg(4K) == 1) ||
	    ((cfgg(4K) == 2) && IS_VDIN_SRC(pch->src_type)))
		return true;
	return false;
}

void dip_init_value_reg(unsigned int ch, struct vframe_s *vframe)
{
	struct di_post_stru_s *ppost;
	struct di_pre_stru_s *ppre = get_pre_stru(ch);
	struct di_ch_s *pch = get_chdata(ch);
	struct di_mm_s *mm;
	enum EDI_SGN sgn;
	unsigned int post_nub;

	dbg_reg("%s:ch[%d]\n", __func__, ch);

	/*post*/
	ppost = get_post_stru(ch);
	/*keep buf:*/
	/*keep_post_buf = ppost->keep_buf_post;*/

	memset(ppost, 0, sizeof(struct di_post_stru_s));
	ppost->next_canvas_id = 1;

	/*pre*/
	memset(ppre, 0, sizeof(struct di_pre_stru_s));

	/* bypass state */
	dim_bypass_st_clear(pch);
	/* dw */
	dw_int();

	if (cfgg(PMODE) == 2) {
		//prog_proc_config = 3;
		dimp_set(edi_mp_prog_proc_config, 3);
		//use_2_interlace_buff = 2;
		dimp_set(edi_mp_use_2_interlace_buff, 2);

	} else {
		//prog_proc_config = 0x23;
		//use_2_interlace_buff = 1;
		dimp_set(edi_mp_prog_proc_config, 0x23);
		dimp_set(edi_mp_use_2_interlace_buff, 1);
	}
	pch->src_type = vframe->source_type;
	if (dim_afds())
		di_set_default(ch);

	mm = dim_mm_get(ch);

	sgn = di_vframe_2_sgn(vframe);
	ppre->sgn_lv = sgn;

	if (cfgg(FIX_BUF)) {
		if (dim_afds()	&&
		    cfgg(4K)) {
			memcpy(&mm->cfg, &c_mm_cfg_fix_4k,
			       sizeof(struct di_mm_cfg_s));
		} else {
			memcpy(&mm->cfg, &c_mm_cfg_normal,
			       sizeof(struct di_mm_cfg_s));
		}
		mm->cfg.fix_buf = 1;
	} else if (!cfgg(4K)) {
		memcpy(&mm->cfg, &c_mm_cfg_normal, sizeof(struct di_mm_cfg_s));
	} else if (sgn <= EDI_SGN_HD) {
		memcpy(&mm->cfg, &c_mm_cfg_normal, sizeof(struct di_mm_cfg_s));
		if (cfgg(POUT_FMT) == 4)
			mm->cfg.dis_afbce = 1;
	} else if ((sgn <= EDI_SGN_4K)	&&
		 dim_afds()		&&
		 dip_is_support_4k(ch)) {
		memcpy(&mm->cfg, &c_mm_cfg_4k, sizeof(struct di_mm_cfg_s));
	} else {
		memcpy(&mm->cfg, &c_mm_cfg_bypass, sizeof(struct di_mm_cfg_s));
	}

	if (cfgg(FIX_BUF))
		mm->cfg.fix_buf = 1;
	else
		mm->cfg.fix_buf = 0;

	post_nub = cfgg(POST_NUB);
	if ((post_nub) && (post_nub < POST_BUF_NUM))
		mm->cfg.num_post = post_nub;

	PR_INF("%s:ch[%d]:fix_buf:%d\n", __func__, ch, mm->cfg.fix_buf);

	pch->mode = dim_cnt_mode(pch);
}

enum EDI_SGN di_vframe_2_sgn(struct vframe_s *vframe)
{
	unsigned int h, v;
	enum EDI_SGN sgn;

	if (IS_COMP_MODE(vframe->type)) {
		h = vframe->compWidth;
		v = vframe->compHeight;
	} else {
		h = vframe->width;
		v = vframe->height;
	}

	if (h <= 1280 && v <= 720) {
		sgn = EDI_SGN_SD;
	} else if (h <= 1920 && v <= 1088) {
		sgn = EDI_SGN_HD;
	} else if (h <= 3840 && v <= 2660 &&
		   IS_PROG(vframe->type)) {
		sgn = EDI_SGN_4K;
	} else {
		sgn = EDI_SGN_OTHER;
	}

	return sgn;
}

static bool dip_init_value(void)
{
	unsigned int ch;
	struct di_post_stru_s *ppost;
	struct di_mm_s *mm;
	struct dim_mm_t_s *mmt = dim_mmt_get();
	struct di_ch_s *pch;
	bool ret = false;

	for (ch = 0; ch < DI_CHANNEL_NUB; ch++) {
		pch = get_chdata(ch);
		pch->ch_id = ch;
		ppost = get_post_stru(ch);
		memset(ppost, 0, sizeof(struct di_post_stru_s));
		ppost->next_canvas_id = 1;

		/*que*/
		ret = di_que_alloc(ch);
		if (ret) {
			pw_queue_clear(ch, QUE_POST_KEEP);
			pw_queue_clear(ch, QUE_POST_KEEP_BACK);
		}
		mm = dim_mm_get(ch);
		memcpy(&mm->cfg, &c_mm_cfg_normal, sizeof(struct di_mm_cfg_s));
	}
	mmt->mem_start = 0;
	mmt->mem_size = 0;
	mmt->total_pages = NULL;
	set_current_channel(0);

	return ret;
}

/******************************************
 *	pq ops
 *****************************************/
void dip_init_pq_ops(void)
{
	unsigned  int ic_id;
	struct di_dev_s *di_devp = get_dim_de_devp();

	ic_id = get_datal()->mdata->ic_id;
	di_attach_ops_pulldown(&get_datal()->ops_pd);
	di_attach_ops_3d(&get_datal()->ops_3d);
	di_attach_ops_nr(&get_datal()->ops_nr);
	di_attach_ops_mtn(&get_datal()->ops_mtn);
	dil_attch_ext_api(&get_datal()->ops_ext);

	dim_attach_to_local();

	/*pd_device_files_add*/
	get_ops_pd()->prob(di_devp->dev);

	get_ops_nr()->nr_drv_init(di_devp->dev);

	di_attach_ops_afd_v3(&get_datal()->afds);
	if (dim_afds()) {
		get_datal()->di_afd.top_cfg_pre =
			&get_datal()->hw_pre.pre_top_cfg;
		get_datal()->di_afd.top_cfg_pst =
			&get_datal()->hw_pst.pst_top_cfg;
		dim_afds()->prob(ic_id,
				 &get_datal()->di_afd);
	} else {
		PR_ERR("%s:no afds\n", __func__);
	}

	/* hw l1 ops*/
	if (IS_IC_EF(ic_id, SC2)) {
		get_datal()->hop_l1 = &dim_ops_l1_v3;
		di_attach_ops_v3(&get_datal()->hop_l2);
	#ifdef MARK_SC2
		if (get_datal()->hop_l2)
			PR_INF("%s\n", opl2()->info.name);
		else
			PR_INF("%s\n", "op12 failed");
	#endif
	}
}

/**********************************/
void dip_clean_value(void)
{
	unsigned int ch;

	for (ch = 0; ch < DI_CHANNEL_NUB; ch++) {
		/*que*/
		di_que_release(ch);
	}
}

bool dip_prob(void)
{
	bool ret = true;
	int i;
	struct di_ch_s *pch;

	ret = dip_init_value();
	for (i = 0; i < DI_CHANNEL_NUB; i++) {
		pch = get_chdata(i);
		bufq_blk_int(pch);
		bufq_mem_int(pch);
		bufq_pat_int(pch);
		bufq_iat_int(pch);
	}

	di_cfgx_init_val();
	di_cfg_top_init_val();
	di_mp_uit_init_val();
	di_mp_uix_init_val();

	dev_vframe_init();
	didbg_fs_init();
//	dip_wq_prob();
	dip_cma_init_val();
	dip_chst_init();

	dpre_init();
	dpost_init();

	//dip_init_pq_ops();
	/*dim_polic_prob();*/

	return ret;
}

void dip_exit(void)
{
	int i;
	struct di_ch_s *pch;

	for (i = 0; i < DI_CHANNEL_NUB; i++) {
		pch = get_chdata(i);
		bufq_blk_exit(pch);
		bufq_mem_exit(pch);
		bufq_pat_exit(pch);
		bufq_iat_exit(pch);
	}
	dim_release_canvas();
//	dip_wq_ext();
	dev_vframe_exit();
	dip_clean_value();
	didbg_fs_exit();
}

/**********************************************/
/**********************************************************
 * cvsi_cfg
 *	set canvas_config_config by config | planes | index
 *
 **********************************************************/
void cvsi_cfg(struct dim_cvsi_s	*pcvsi)
{
	int i;
	unsigned char *canvas_index = &pcvsi->cvs_id[0];
	//unsigned int shift;
	struct canvas_config_s *cfg = &pcvsi->cvs_cfg[0];
	u32 planes = pcvsi->plane_nub;

	if (planes > 3) {
		//PR_ERR("%s:planes overflow[%d]\n", __func__, planes);
		return;
	}
	//dim_print("%s:p[%d]\n", __func__, planes);

	for (i = 0; i < planes; i++, canvas_index++, cfg++) {
		canvas_config_config(*canvas_index, cfg);
		dim_print("\tw[%d],h[%d],cid[%d]\n",
			  cfg->width, cfg->height, *canvas_index);
	}
}

#ifdef TEST_PIP

#define src0_fmt          2   //0:444 1:422 2:420
#define src0_bits         2   //0:8bit 1:9bit 2:10bit
#define comb444           0   //need set 1,when hsize>2k, fmt444,8bit,
#define PIP_FMT           2   //
#ifdef ARY_TEST //ary want change background color, but failed
#define DEF_COLOR0        0x0
#define DEF_COLOR1        0x200
#define DEF_COLOR2        0x200
#else
#define DEF_COLOR0        0x3ff
#define DEF_COLOR1        0x200
#define DEF_COLOR2        0x200

#endif
#define PIP_BIT           10  //
//int32_t PIP_BIT_ENC = PIP_BIT==8 ? 0 : PIP_BIT==9 ? 1 : 2;
#define PIP_HSIZE         (2160 / 2) //1/2 rot shrink
#define PIP_VSIZE         (4096 / 2) //1/2 rot shrink

struct AFBCD_S di_afbcd_din_def = {
	.index		= 0,
	.hsize		= 1920,
	.vsize		= 1080,
	.head_baddr	= 0,
	/* index, hsize, vsize, head_baddr, body_baddr*/
	.body_baddr	= 0,
	.compbits	= 0,
	.fmt_mode	= 0,
	.ddr_sz_mode	= 1,
	.fmt444_comb	= comb444,
	/* compbits, fmt_mode, ddr_sz_mode, fmt444_comb, dos_uncomp*/
	.dos_uncomp	= 0,
	.rot_en		= 0,
	.rot_hbgn	= 0,
	.rot_vbgn	= 0,
	.h_skip_en	= 0,
	/* rot_en, rot_hbgn, rot_vbgn, h_skip_en, v_skip_en*/
	.v_skip_en	= 0,
	.rev_mode	= 0,
	/* rev_mode,lossy_en,*/
	.lossy_en	= 0,
	/*def_color_y,def_color_u,def_color_v*/
	.def_color_y	= DEF_COLOR0,
	.def_color_u	= DEF_COLOR1,
	.def_color_v	= DEF_COLOR2,
	.win_bgn_h	= 0,
	.win_end_h	= 1919,
	.win_bgn_v	= 0,
	/* win_bgn_h, win_end_h, win_bgn_v, win_end_v*/
	.win_end_v	= 1079,
	.rot_vshrk	= 0,
	.rot_hshrk	= 0,
	.rot_drop_mode	= 0,
	.rot_ofmt_mode	= PIP_FMT,
	.rot_ocompbit	= 2,//PIP_BIT_ENC,
	.pip_src_mode	= 0,
	.hold_line_num	= 8,
};     // rot_vshrk

struct AFBCE_S di_afbce_out_def = {
	.head_baddr		= 0,
	.mmu_info_baddr		= 0,
	.reg_init_ctrl		= 0,
	.reg_pip_mode		= 0,
	.reg_ram_comb		= 0,
	/* reg_init_ctrl, reg_pip_mode, reg_ram_comb, reg_format_mode */
	.reg_format_mode	= PIP_FMT,
	.reg_compbits_y		= PIP_BIT,
	/* reg_compbits_y, reg_compbits_c */
	.reg_compbits_c		= PIP_BIT,
	.hsize_in		= 1920,
	.vsize_in		= 1080,
	.hsize_bgnd		= PIP_HSIZE,
	/* hsize_in, vsize_in, hsize_bgnd, vsize_bgnd,*/
	.vsize_bgnd		= PIP_VSIZE,
	.enc_win_bgn_h		= 0,
	.enc_win_end_h		= 1919,
	.enc_win_bgn_v		= 0,
	/* enc_win_bgn_h,enc_win_end_h,enc_win_bgn_v,enc_win_end_v,*/
	.enc_win_end_v		= 1079,
	.loosy_mode		= 0,
	/* loosy_mode,rev_mode,*/
	.rev_mode		= 0,
	/* def_color_0,def_color_1,def_color_2,def_color_3 */
	.def_color_0		= DEF_COLOR0,
	.def_color_1		= DEF_COLOR1,
	.def_color_2		= DEF_COLOR2,
	.def_color_3		= 0,
	.force_444_comb		= comb444,
	.rot_en			= 0,
};                    // force_444_comb, rot_en

int dim_post_process_copy_input(struct di_ch_s *pch,
				struct vframe_s *vfm_in,
				struct di_buf_s *di_buf)
{
	struct di_hpst_s  *pst = get_hw_pst();
	unsigned int ch;
	struct mem_cpy_s	*cfg_cpy;
	struct di_post_stru_s *ppost;
	//struct vframe_s		*vfm_in;
	struct dim_wmode_s	wmode;
	struct dim_wmode_s	*pwm;
	//struct di_buf_s		*di_buf;

	//struct dim_fmt_s	*fmt_in;
	//struct dim_fmt_s	*fmt_out;
	//struct dim_cvsi_s	*cvsi_in;
	//struct dim_cvsi_s	*cvsi_out;
	struct AFBCD_S		*in_afbcd;
	struct AFBCE_S		*o_afbce;
	//struct DI_SIM_MIF_s	*o_mif;
	unsigned int h, v;
	//bool err = false;
	//bool flg_in_mif = false, flg_out_mif = false;
	//struct pst_cfg_afbc_s *acfg;
	//struct di_buf_s		*in_buf;
//	struct di_cvs_s *cvss;
	enum DI_MIF0_ID mif_index;
	enum EAFBC_DEC	dec_index = EAFBC_DEC_IF1;
//	unsigned int cvsw, cvsh;
	union hw_sc2_ctr_pre_s cfg;
	bool is_4k = false;

	ch	= pch->ch_id;
	ppost	= &pch->rse_ori.di_post_stru;
	cfg_cpy = &ppost->cfg_cpy;
	pwm	= &wmode;
	pst->cfg_from = DI_SRC_ID_AFBCD_IF1;
	cfg_cpy->afbcd_vf	= NULL;
	//pst->cfg_rot	= 1;

	memset(pwm, 0, sizeof(*pwm));
	pwm->src_h = vfm_in->height;
	pwm->src_w = vfm_in->width;

	if (IS_COMP_MODE(vfm_in->type)) {
		pwm->is_afbc = 1;
		pwm->src_h = vfm_in->compHeight;
		pwm->src_w = vfm_in->compWidth;
	}
	if ((pwm->src_w > 1920) || (pwm->src_h > 1080))
		is_4k = true;

	if (IS_PROG(vfm_in->type))
		pwm->is_i = 0;
	else
		pwm->is_i = 1;
	/**********************/
	/* afbcd */
	in_afbcd = &ppost->in_afbcd;
	cfg_cpy->in_afbcd = in_afbcd;
	cfg_cpy->in_rdmif = NULL;

	memcpy(in_afbcd, &di_afbcd_din_def, sizeof(*in_afbcd));
	in_afbcd->hsize = pwm->src_w;
	in_afbcd->vsize = pwm->src_h;
	in_afbcd->head_baddr = vfm_in->compHeadAddr >> 4;
	in_afbcd->body_baddr = vfm_in->compBodyAddr >> 4;
	if (pst->cfg_rot) {
		if (pst->cfg_rot == 1) {
			/*180*/
			in_afbcd->rev_mode	= 3;
			in_afbcd->rot_en	= 0;
		} else if (pst->cfg_rot == 2) {
			/*90*/
			in_afbcd->rev_mode	= 2;
			in_afbcd->rot_en	= 1;
		} else if (pst->cfg_rot == 3) {
			/*270*/
			in_afbcd->rev_mode	= 1;
			in_afbcd->rot_en	= 1;
		}
	} else {
		in_afbcd->rot_en	= 0;
		in_afbcd->rev_mode	= 0;
	}
	dbg_copy("%s:rev_mode =%d\n", __func__, in_afbcd->rev_mode);
	//in_afbcd->rot_en	= pst->cfg_rot;
	in_afbcd->dos_uncomp	= 1; //tmp for

	if (vfm_in->type & VIDTYPE_VIU_422)
		in_afbcd->fmt_mode = 1;
	else if (vfm_in->type & VIDTYPE_VIU_444)
		in_afbcd->fmt_mode = 0;
	else
		in_afbcd->fmt_mode = 2;/* 420 ?*/

	if (vfm_in->bitdepth & BITDEPTH_Y10)
		in_afbcd->compbits = 2;	// 10 bit
	else
		in_afbcd->compbits = 0; // 8bit

	if (vfm_in->bitdepth & BITDEPTH_SAVING_MODE) {
		//r |= (1 << 28); /* mem_saving_mode */
		in_afbcd->blk_mem_mode = 1;
	} else {
		in_afbcd->blk_mem_mode = 0;
	}
	if (vfm_in->type & VIDTYPE_SCATTER) {
		//r |= (1 << 29);
		in_afbcd->ddr_sz_mode = 1;
	} else {
		in_afbcd->ddr_sz_mode = 0;
	}

	#ifdef ARY_TEST
	if (IS_420P_SRC(vfm_in->type)) {
		cvfmt_en = 1;
		vt_ini_phase = 0xc;
		cvfm_h = out_height >> 1;
		rpt_pix = 1;
		phase_step = 8;
	} else {
		cvfm_h = out_height;
		rpt_pix = 0;
	}
	#endif
	in_afbcd->win_bgn_h = 0;
	in_afbcd->win_end_h = in_afbcd->win_bgn_h + in_afbcd->hsize - 1;
	in_afbcd->win_bgn_v = 0;
	in_afbcd->win_end_v = in_afbcd->win_bgn_v + in_afbcd->vsize - 1;
	in_afbcd->index = dec_index;

	/**********************/
	/* afbce */
	/*wr afbce */
	//flg_out_mif = false;
	o_afbce = &ppost->out_afbce;
	cfg_cpy->out_afbce = o_afbce;

	//di_buf->di_buf_post = di_que_out_to_di_buf(ch, QUE_POST_FREE);

	memcpy(o_afbce, &di_afbce_out_def, sizeof(*o_afbce));
	o_afbce->reg_init_ctrl = 0;
	o_afbce->head_baddr = di_buf->afbc_adr;
	o_afbce->mmu_info_baddr = di_buf->afbct_adr;
	o_afbce->reg_format_mode = 1; /* 422 tmp */
	//o_afbce->reg_compbits_y = 8;
	//o_afbce->reg_compbits_c = 8; /* 8 bit */
	if (pst->cfg_rot == 2 || pst->cfg_rot == 3) {
		h = pwm->src_h;
		v = pwm->src_w;
		//di_cnt_post_buf_rotation(pch, &cvsw, &cvsh);
		//dim_print("rotation:cvsw[%d],cvsh[%d]\n", cvsw, cvsh);
		//di_buf->canvas_width[0] = cvsw;
		//di_buf->canvas_height	= cvsh;
		o_afbce->rot_en = 1;
	} else {
		h = pwm->src_w;
		v = pwm->src_h;
		o_afbce->rot_en = 0;
	}
//		v = v / 2;
	o_afbce->hsize_in = h;
	o_afbce->vsize_in = v;
	o_afbce->enc_win_bgn_h = 0;
	o_afbce->enc_win_end_h = o_afbce->enc_win_bgn_h + h - 1;
	o_afbce->enc_win_bgn_v = 0;
	o_afbce->enc_win_end_v = o_afbce->enc_win_bgn_v + v - 1;

	//o_afbce->rot_en = pst->cfg_rot;

	cfg_cpy->afbc_en = 1;
	cfg_cpy->out_wrmif = NULL;

	//sync with afbce
	if (o_afbce->rot_en) {
		/*sync with afbce 422*/
		in_afbcd->rot_ofmt_mode = o_afbce->reg_format_mode;
		if (o_afbce->reg_compbits_y == 8)
			in_afbcd->rot_ocompbit	= 0;
		else if (o_afbce->reg_compbits_y == 9)
			in_afbcd->rot_ocompbit	= 1;
		else/* if (o_afbce->reg_compbits_y == 10) */
			in_afbcd->rot_ocompbit	= 2;
	}

	/* set vf */
	memcpy(di_buf->vframe, vfm_in, sizeof(struct vframe_s));
	di_buf->vframe->private_data = di_buf;
	di_buf->vframe->compHeight	= v;
	di_buf->vframe->compWidth	= h;
	di_buf->vframe->height	= v;
	di_buf->vframe->width	= h;
	di_buf->vframe->compHeadAddr = di_buf->afbc_adr;
	di_buf->vframe->compBodyAddr = di_buf->nr_adr;
	#ifdef ARY_TEST
	di_buf->vframe->bitdepth |= (BITDEPTH_U10 | BITDEPTH_V10);
	di_buf->vframe->type |= (VIDTYPE_COMPRESS	|
				 VIDTYPE_SCATTER	|
				 VIDTYPE_VIU_422);
	#else
		di_buf->vframe->bitdepth &= ~(BITDEPTH_MASK);
		di_buf->vframe->bitdepth &= ~(FULL_PACK_422_MODE);
		#ifdef ARY_TEST
		di_buf->vframe->bitdepth |= (BITDEPTH_Y8	|
				  BITDEPTH_U8	|
				  BITDEPTH_V8);
		#else
		di_buf->vframe->bitdepth |= (BITDEPTH_Y10	|
				  BITDEPTH_U10	|
				  BITDEPTH_V10	|
				  FULL_PACK_422_MODE);
		#endif
		/*clear*/
		di_buf->vframe->type &= ~(VIDTYPE_VIU_NV12	|
			       VIDTYPE_VIU_444	|
			       VIDTYPE_VIU_NV21	|
			       VIDTYPE_VIU_422	|
			       VIDTYPE_VIU_SINGLE_PLANE	|
			       VIDTYPE_COMPRESS	|
			       VIDTYPE_PRE_INTERLACE);
		di_buf->vframe->type |= (VIDTYPE_COMPRESS	|
					 VIDTYPE_SCATTER	|
					 VIDTYPE_VIU_422	|
					 VIDTYPE_VIU_SINGLE_PLANE);
	#endif

	#ifdef ARY_TEST
	dimpst_fill_outvf(&pst->vf_post, di_buf, EDPST_OUT_MODE_DEF);

	pst->vf_post.compWidth = h;
	pst->vf_post.compHeight = v;
	pst->vf_post.type |= (VIDTYPE_COMPRESS | VIDTYPE_SCATTER);
	pst->vf_post.compHeadAddr = di_buf->afbc_adr;
	pst->vf_post.compBodyAddr = di_buf->nr_adr;
	pst->vf_post.bitdepth |= (BITDEPTH_U10 | BITDEPTH_V10);
	#endif
	di_buf->vframe->early_process_fun = dim_do_post_wr_fun;
	di_buf->vframe->process_fun = NULL;

	/* 2019-04-22 Suggestions from brian.zhu*/
	di_buf->vframe->mem_handle = NULL;
	di_buf->vframe->type |= VIDTYPE_DI_PW;
	di_buf->in_buf	= NULL;
	memcpy(&pst->vf_post, di_buf->vframe, sizeof(pst->vf_post));

	//dim_print("%s:v[%d]\n", __func__, v);
	dim_print("\tblk[%d]:head[0x%lx], body[0x%lx]\n",
		  di_buf->blk_buf->header.index,
		  di_buf->afbct_adr,
		  di_buf->nr_adr);

	/**********************/

	if (pst->cfg_from == DI_SRC_ID_MIF_IF0 ||
	    pst->cfg_from == DI_SRC_ID_AFBCD_IF0) {
		//in_buf = acfg->buf_mif[0];
		mif_index = DI_MIF0_ID_IF0;
		dec_index = EAFBC_DEC_IF0;
		cfg_cpy->port = 0x81;
	} else if (pst->cfg_from == DI_SRC_ID_MIF_IF1	||
		 pst->cfg_from == DI_SRC_ID_AFBCD_IF1) {
		//in_buf = acfg->buf_mif[1];
		mif_index = DI_MIF0_ID_IF1;
		dec_index = EAFBC_DEC_IF1;
		cfg_cpy->port = 0x92;
	} else {
		//in_buf = acfg->buf_mif[2];
		mif_index = DI_MIF0_ID_IF2;
		dec_index = EAFBC_DEC_IF2;
		cfg_cpy->port = 0xa3;
	}

	cfg_cpy->hold_line = 4;
	cfg_cpy->opin	= &di_pre_regset;

	dbg_copy("is_4k[%d]\n", is_4k);
	#if 1
	if (is_4k)
		dim_sc2_4k_set(2);
	else
		dim_sc2_4k_set(0);
	#endif
	cfg.d32 = 0;
	dim_sc2_contr_pre(&cfg);
	if (cfg_cpy->in_afbcd)
		dim_print("1:afbcd:0x%px\n", cfg_cpy->in_afbcd);
	if (cfg_cpy->in_rdmif)
		dim_print("2:in_rdmif:0x%px\n", cfg_cpy->in_rdmif);
	if (cfg_cpy->out_afbce)
		dim_print("3:out_afbce:0x%px\n", cfg_cpy->out_afbce);
	if (cfg_cpy->out_wrmif)
		dim_print("4:out_wrmif:0x%px\n", cfg_cpy->out_wrmif);

	//dbg_sc2_4k_set(1);
	opl2()->memcpy_rot(cfg_cpy);
	//dbg_sc2_4k_set(2);
	pst->pst_tst_use	= 1;
	pst->flg_int_done	= false;

	return 0;
}

/*
 * input use use vframe set
 * out use pst cfg
 */
int dim_post_process_copy_only(struct di_ch_s *pch,
			       struct vframe_s *vfm_in,
			       struct di_buf_s *di_buf)
{
	struct di_hpst_s  *pst = get_hw_pst();
	unsigned int ch;
	struct mem_cpy_s	*cfg_cpy;
	struct di_post_stru_s *ppost;
	struct dim_wmode_s	wmode;
	struct dim_wmode_s	*pwm;
	struct dim_cvsi_s	*cvsi_out;
	struct AFBCD_S		*in_afbcd	= NULL;
	struct AFBCE_S		*o_afbce	= NULL;
	struct di_buf_s		*in_buf		= NULL;
	struct DI_MIF_S		*i_mif		= NULL;
	struct DI_SIM_MIF_s	*o_mif;
	unsigned int h, v;
	//bool err = false;
	//bool flg_in_mif = false, flg_out_mif = false;
	//struct pst_cfg_afbc_s *acfg;
	//struct di_buf_s		*in_buf;
	struct di_cvs_s *cvss;
	enum DI_MIF0_ID mif_index;
	enum EAFBC_DEC	dec_index = EAFBC_DEC_IF1;
//	unsigned int cvsw, cvsh;
	union hw_sc2_ctr_pre_s cfg;
	bool is_4k = false;
//	unsigned int tmp_mask;
	ch	= pch->ch_id;
	ppost	= &pch->rse_ori.di_post_stru;
	cfg_cpy = &ppost->cfg_cpy;
	pwm	= &wmode;
	pst->cfg_from = DI_SRC_ID_AFBCD_IF1;
	cfg_cpy->afbcd_vf	= NULL;
	cvsi_out = &ppost->cvsi_out;
	cvss	= &get_datal()->cvs;

	memset(pwm, 0, sizeof(*pwm));
	pwm->src_h = vfm_in->height;
	pwm->src_w = vfm_in->width;

	if (IS_COMP_MODE(vfm_in->type)) {
		pwm->is_afbc = 1;
		pwm->src_h = vfm_in->compHeight;
		pwm->src_w = vfm_in->compWidth;
	}
	if ((pwm->src_w > 1920) || (pwm->src_h > 1920))
		is_4k = true;

	if (IS_PROG(vfm_in->type))
		pwm->is_i = 0;
	else
		pwm->is_i = 1;
	/**********************/
	if (pwm->is_afbc) {
		/* afbcd */
		in_afbcd = &ppost->in_afbcd;
		cfg_cpy->in_afbcd = in_afbcd;
		cfg_cpy->in_rdmif = NULL;

		memcpy(in_afbcd, &di_afbcd_din_def, sizeof(*in_afbcd));
		in_afbcd->hsize = pwm->src_w;
		in_afbcd->vsize = pwm->src_h;
		in_afbcd->head_baddr = vfm_in->compHeadAddr >> 4;
		in_afbcd->body_baddr = vfm_in->compBodyAddr >> 4;
		if (pst->cfg_rot) {
			if (pst->cfg_rot == 1) {
				/*180*/
				in_afbcd->rev_mode	= 3;
				in_afbcd->rot_en	= 0;
			} else if (pst->cfg_rot == 2) {
				/*90*/
				in_afbcd->rev_mode	= 2;
				in_afbcd->rot_en	= 1;
			} else if (pst->cfg_rot == 3) {
				/*270*/
				in_afbcd->rev_mode	= 1;
				in_afbcd->rot_en	= 1;
			}
		} else {
			in_afbcd->rot_en	= 0;
			in_afbcd->rev_mode	= 0;
		}
		dbg_copy("%s:rev_mode =%d\n", __func__, in_afbcd->rev_mode);
		//in_afbcd->rot_en	= pst->cfg_rot;
		in_afbcd->dos_uncomp	= 1; //tmp for

		if (vfm_in->type & VIDTYPE_VIU_422)
			in_afbcd->fmt_mode = 1;
		else if (vfm_in->type & VIDTYPE_VIU_444)
			in_afbcd->fmt_mode = 0;
		else
			in_afbcd->fmt_mode = 2;/* 420 ?*/

		if (vfm_in->bitdepth & BITDEPTH_Y10)
			in_afbcd->compbits = 2;	// 10 bit
		else
			in_afbcd->compbits = 0; // 8bit

		if (vfm_in->bitdepth & BITDEPTH_SAVING_MODE) {
			//r |= (1 << 28); /* mem_saving_mode */
			in_afbcd->blk_mem_mode = 1;
		} else {
			in_afbcd->blk_mem_mode = 0;
		}
		if (vfm_in->type & VIDTYPE_SCATTER) {
			//r |= (1 << 29);
			in_afbcd->ddr_sz_mode = 1;
		} else {
			in_afbcd->ddr_sz_mode = 0;
		}

		in_afbcd->win_bgn_h = 0;
		in_afbcd->win_end_h = in_afbcd->win_bgn_h + in_afbcd->hsize - 1;
		in_afbcd->win_bgn_v = 0;
		in_afbcd->win_end_v = in_afbcd->win_bgn_v + in_afbcd->vsize - 1;
		in_afbcd->index = dec_index;
	} else {
		i_mif = &ppost->di_buf1_mif;
		cfg_cpy->in_afbcd = NULL;
		cfg_cpy->in_rdmif = i_mif;

		in_buf = &ppost->in_buf;
		memset(in_buf, 0, sizeof(*in_buf));

		in_buf->vframe = &ppost->in_buf_vf;
		memcpy(in_buf->vframe, vfm_in, sizeof(struct vframe_s));
		in_buf->vframe->private_data = in_buf;

		pre_cfg_cvs(in_buf->vframe);
		config_di_mif_v3(i_mif, DI_MIF0_ID_IF1, in_buf, ch);
		i_mif->mif_index = DI_MIF0_ID_IF1;
	}

	/* set vf */
	memcpy(di_buf->vframe, vfm_in, sizeof(struct vframe_s));
	di_buf->vframe->private_data = di_buf;
	di_buf->vframe->compHeight	= pwm->src_h;
	di_buf->vframe->compWidth	= pwm->src_w;
	di_buf->vframe->height	= pwm->src_h;
	di_buf->vframe->width	= pwm->src_w;
	di_buf->vframe->bitdepth &= ~(BITDEPTH_MASK);
	di_buf->vframe->bitdepth &= ~(FULL_PACK_422_MODE);
	/*clear*/
	di_buf->vframe->type &= ~(VIDTYPE_VIU_NV12	|
		       VIDTYPE_VIU_444	|
		       VIDTYPE_VIU_NV21 |
		       VIDTYPE_VIU_422	|
		       VIDTYPE_VIU_SINGLE_PLANE |
		       VIDTYPE_COMPRESS |
		       VIDTYPE_PRE_INTERLACE);

	//dbg_copy("di:typ[0x%x]:\n", di_buf->vframe->type);
	if (pst->cfg_out_enc)
		di_buf->vframe->type |= (VIDTYPE_COMPRESS	|
					 VIDTYPE_SCATTER	|
					 VIDTYPE_VIU_SINGLE_PLANE);
	if (pst->cfg_out_fmt == 0) {
		di_buf->vframe->type |= (VIDTYPE_VIU_444 |
					 VIDTYPE_VIU_SINGLE_PLANE);
	} else if (pst->cfg_out_fmt == 1) {
		di_buf->vframe->type |= (VIDTYPE_VIU_422 |
					 VIDTYPE_VIU_SINGLE_PLANE);
	} else {
		if (!pst->cfg_out_enc)
			di_buf->vframe->type |= VIDTYPE_VIU_NV21;
		else
			di_buf->vframe->type |= VIDTYPE_VIU_SINGLE_PLANE;
	}
	//dbg_copy("di:typ[0x%x]:\n", di_buf->vframe->type);
	if (pst->cfg_out_bit == 0)
		di_buf->vframe->bitdepth |= (BITDEPTH_Y8	|
				  BITDEPTH_U8	|
				  BITDEPTH_V8);
	else if (pst->cfg_out_bit == 2)
		di_buf->vframe->bitdepth |= (BITDEPTH_Y10	|
				  BITDEPTH_U10	|
				  BITDEPTH_V10	|
				  FULL_PACK_422_MODE);

	if (pst->cfg_out_enc) {
		/**********************/
		/* afbce */
		/*wr afbce */
		//flg_out_mif = false;
		o_afbce = &ppost->out_afbce;
		cfg_cpy->out_afbce = o_afbce;
		cfg_cpy->afbc_en = 1;
		cfg_cpy->out_wrmif = NULL;

		memcpy(o_afbce, &di_afbce_out_def, sizeof(*o_afbce));
		o_afbce->reg_init_ctrl = 0;
		o_afbce->head_baddr = di_buf->afbc_adr;
		o_afbce->mmu_info_baddr = di_buf->afbct_adr;
		o_afbce->reg_format_mode = 1; /* 422 tmp */
		//o_afbce->reg_compbits_y = 8;
		//o_afbce->reg_compbits_c = 8; /* 8 bit */
		if (pst->cfg_rot == 2 || pst->cfg_rot == 3) {
			h = pwm->src_h;
			v = pwm->src_w;
			//di_cnt_post_buf_rotation(pch, &cvsw, &cvsh);
			//dim_print("rotation:cvsw[%d],cvsh[%d]\n", cvsw, cvsh);
			//di_buf->canvas_width[0] = cvsw;
			//di_buf->canvas_height = cvsh;
			o_afbce->rot_en = 1;
		} else {
			h = pwm->src_w;
			v = pwm->src_h;
			o_afbce->rot_en = 0;
		}
	//		v = v / 2;
		o_afbce->hsize_in = h;
		o_afbce->vsize_in = v;
		o_afbce->enc_win_bgn_h = 0;
		o_afbce->enc_win_end_h = o_afbce->enc_win_bgn_h + h - 1;
		o_afbce->enc_win_bgn_v = 0;
		o_afbce->enc_win_end_v = o_afbce->enc_win_bgn_v + v - 1;

		//o_afbce->rot_en = pst->cfg_rot;

		//sync with afbce
		if (o_afbce->rot_en && cfg_cpy->in_afbcd) {
			/*sync with afbce 422*/
			in_afbcd->rot_ofmt_mode = o_afbce->reg_format_mode;
			if (o_afbce->reg_compbits_y == 8)
				in_afbcd->rot_ocompbit	= 0;
			else if (o_afbce->reg_compbits_y == 9)
				in_afbcd->rot_ocompbit	= 1;
			else/* if (o_afbce->reg_compbits_y == 10)*/
				in_afbcd->rot_ocompbit	= 2;
		}
		di_buf->vframe->compHeight	= v;
		di_buf->vframe->compWidth	= h;
		di_buf->vframe->height	= v;
		di_buf->vframe->width	= h;
		di_buf->vframe->compHeadAddr = di_buf->afbc_adr;
		di_buf->vframe->compBodyAddr = di_buf->nr_adr;

		//dim_print("%s:v[%d]\n", __func__, v);
		dim_print("\tblk[%d]:head[0x%lx], body[0x%lx]\n",
			  di_buf->blk_buf->header.index,
			  di_buf->afbct_adr,
			  di_buf->nr_adr);
	} else {
		o_mif = &ppost->di_diwr_mif;
		cfg_cpy->out_wrmif = o_mif;
		cfg_cpy->out_afbce = NULL;
		cvsi_out->cvs_id[0] = (unsigned char)cvss->post_idx[1][0];
		cvsi_out->plane_nub = 1;
		cvsi_out->cvs_cfg[0].phy_addr = di_buf->nr_adr;
		cvsi_out->cvs_cfg[0].width	= di_buf->canvas_width[0];
		cvsi_out->cvs_cfg[0].height	= di_buf->canvas_height;
		cvsi_out->cvs_cfg[0].block_mode	= 0;

		cvsi_out->cvs_cfg[0].endian	= 0;

		o_mif->canvas_num	= (unsigned short)cvsi_out->cvs_id[0];
		memcpy(&di_buf->vframe->canvas0_config[0],
		       &cvsi_out->cvs_cfg[0],
		       sizeof(di_buf->vframe->canvas0_config[0]));
		di_buf->vframe->plane_num = cvsi_out->plane_nub;
		di_buf->vframe->canvas0Addr = (u32)o_mif->canvas_num;

		/* set cvs */
		cvsi_cfg(cvsi_out);

		opl1()->wr_cfg_mif(o_mif, EDI_MIFSM_WR, di_buf->vframe, NULL);
	}

	di_buf->vframe->early_process_fun = dim_do_post_wr_fun;
	di_buf->vframe->process_fun = NULL;

	/* 2019-04-22 Suggestions from brian.zhu*/
	di_buf->vframe->mem_handle = NULL;
	di_buf->vframe->type |= VIDTYPE_DI_PW;
	di_buf->in_buf	= NULL;
	memcpy(&pst->vf_post, di_buf->vframe, sizeof(pst->vf_post));

	/**********************/

	if (pst->cfg_from == DI_SRC_ID_MIF_IF0 ||
	    pst->cfg_from == DI_SRC_ID_AFBCD_IF0) {
		//in_buf = acfg->buf_mif[0];
		mif_index = DI_MIF0_ID_IF0;
		dec_index = EAFBC_DEC_IF0;
		cfg_cpy->port = 0x81;
	} else if (pst->cfg_from == DI_SRC_ID_MIF_IF1	||
		 pst->cfg_from == DI_SRC_ID_AFBCD_IF1) {
		//in_buf = acfg->buf_mif[1];
		mif_index = DI_MIF0_ID_IF1;
		dec_index = EAFBC_DEC_IF1;
		cfg_cpy->port = 0x92;
	} else {
		//in_buf = acfg->buf_mif[2];
		mif_index = DI_MIF0_ID_IF2;
		dec_index = EAFBC_DEC_IF2;
		cfg_cpy->port = 0xa3;
	}

	cfg_cpy->hold_line = 4;
	cfg_cpy->opin	= &di_pre_regset;

	dbg_copy("is_4k[%d]\n", is_4k);
	#if 1
	if (is_4k)
		dim_sc2_4k_set(2);
	else
		dim_sc2_4k_set(0);
	#endif
	cfg.d32 = 0;
	dim_sc2_contr_pre(&cfg);
	if (cfg_cpy->in_afbcd)
		dim_print("1:afbcd:0x%px\n", cfg_cpy->in_afbcd);
	if (cfg_cpy->in_rdmif)
		dim_print("2:in_rdmif:0x%px\n", cfg_cpy->in_rdmif);
	if (cfg_cpy->out_afbce)
		dim_print("3:out_afbce:0x%px\n", cfg_cpy->out_afbce);
	if (cfg_cpy->out_wrmif)
		dim_print("4:out_wrmif:0x%px\n", cfg_cpy->out_wrmif);

	opl2()->memcpy(cfg_cpy);

	pst->pst_tst_use	= 1;
	pst->flg_int_done	= false;

	return 0;
}

void dim_post_copy_update(struct di_ch_s *pch,
			  struct vframe_s *vfm_in,
			  struct di_buf_s *di_buf)
{
	struct di_hpst_s  *pst = get_hw_pst();
//	const struct reg_acc *op = di_pre_regset;

	memcpy(di_buf->vframe, &pst->vf_post, sizeof(struct vframe_s));
	di_buf->vframe->private_data = di_buf;

	di_buf->vframe->compHeadAddr = di_buf->afbc_adr;
	di_buf->vframe->compBodyAddr = di_buf->nr_adr;
	//di_buf->vframe->early_process_fun = dim_do_post_wr_fun;
	//di_buf->vframe->process_fun = NULL;

	/* 2019-04-22 Suggestions from brian.zhu*/
	di_buf->vframe->mem_handle = NULL;
	di_buf->vframe->type |= VIDTYPE_DI_PW;
	di_buf->in_buf	= NULL;

	dbg_afbc_update_level1(vfm_in, EAFBC_DEC_IF1);
	dbg_afbce_update_level1(di_buf->vframe, &di_pre_regset, EAFBC_ENC1);
	pst->pst_tst_use	= 1;
	pst->flg_int_done	= false;
	opl1()->pst_set_flow(1, EDI_POST_FLOW_STEP4_CP_START);
}

void dbg_cp_4k(struct di_ch_s *pch, unsigned int mode)
{
	struct vframe_s	*vfm_in = NULL;
	struct di_buf_s	*di_buf;
	unsigned int ch;
	struct di_hpst_s  *pst = get_hw_pst();
	unsigned int time_cnt = 0;
	u64 t_st, t_c;
	unsigned int us_diff[5], diffa;
	int i;
	unsigned int flg_mode, mode_rotation, mode_copy;
	char *mname = "nothing";

//	struct di_hpst_s  *pst = get_hw_pst();
	if (mode & 0x0f) {
		flg_mode = 1; /*rotation*/
		mode_rotation	= mode & 0xf;
		mode_copy	= 0;

	} else if (mode & 0xf0) {
		flg_mode = 2; /*mem copy only */
		mode_rotation	= 0;
		mode_copy	= mode & 0xf0;
	} else {
		mode_copy	= 0;
		mode_rotation	= 0;
		//PR_ERR("mode[%d] is overflow?\n", mode);
		return;
	}

	ch	= pch->ch_id;
	/**********************/
	pst->cfg_rot = (mode_rotation & (DI_BIT1 | DI_BIT2)) >> 1;
	/**********************/
	/* vfm */
	vfm_in = pw_vf_peek(ch);
	if (!vfm_in) {
		dbg_copy("no input");
		return;
	}
	dbg_copy("vf:%px\n", vfm_in);
	/**********************/
	if (di_que_is_empty(ch, QUE_POST_FREE))
		return;
	/**********************/
	t_st = cur_to_usecs();
	vfm_in	= pw_vf_get(ch);
	di_buf	= di_que_out_to_di_buf(ch, QUE_POST_FREE);
	t_c = cur_to_usecs();
	us_diff[0] = (unsigned int)(t_c - t_st);
	/**********************/
	/* rotation */
	if (mode_rotation) {
		mname = "rotation";
		if (mode_rotation & DI_BIT0)
			dim_post_process_copy_input(pch, vfm_in, di_buf);
		else
			dim_post_copy_update(pch, vfm_in, di_buf);
	} else if (mode_copy == 0x10) {
		mname = "copy:2 mif";
		pst->cfg_out_bit = 2;
		pst->cfg_out_enc = 0;
		pst->cfg_out_fmt = 1;
		dim_post_process_copy_only(pch, vfm_in, di_buf);
	} else if (mode_copy == 0x20) {
		mname = "copy:2 afbc";
		pst->cfg_out_bit = 2;
		pst->cfg_out_enc = 1;
		pst->cfg_out_fmt = 1;
		dim_post_process_copy_only(pch, vfm_in, di_buf);
	}

	t_c = cur_to_usecs();
	us_diff[1] = (unsigned int)(t_c - t_st);

	/* wait finish */
	while (time_cnt < 50) {
		if (pst->flg_int_done)
			break;
		//msleep(3);
		usleep_range(2000, 2001);
		time_cnt++;
	}
	if (!pst->flg_int_done) {
		//PR_ERR("%s:copy failed\n", __func__);
		pw_vf_put(vfm_in, ch);
		return;
	}
	t_c = cur_to_usecs();
	us_diff[2] = (unsigned int)(t_c - t_st);

	di_que_in(ch, QUE_POST_READY, di_buf);
	dbg_copy("di:typ2[0x%x]:\n", di_buf->vframe->type);
	pw_vf_put(vfm_in, ch);

	dbg_copy("%s:timer:%d:%px:%s\n", __func__, time_cnt, vfm_in, mname);
	diffa = 0;
	for (i = 0; i < 5; i++) {
		diffa = us_diff[i] - diffa;
		dbg_copy("\t:[%d]:%d:%d\n", i, us_diff[i], diffa);
		diffa = us_diff[i];
	}
}

#define DIM_PIP_WIN_NUB (16)
static const struct di_win_s win_pip[DIM_PIP_WIN_NUB] = {
	[0] = {
	.x_st = 0,
	.y_st = 0,
	},
	[1] = {
	.x_st = 1920,
	.y_st = 0,
	},
	[2] = {
	.x_st = 960,
	.y_st = 540,
	},
	[3] = {
	.x_st = 1920,
	.y_st = 1080,
	},
};

static const struct di_win_s win_pip_16[DIM_PIP_WIN_NUB] = { /*960 x 540 */
	[0] = {
	.x_st = 0,
	.y_st = 0,
	},
	[1] = {
	.x_st = 960,
	.y_st = 0,
	},
	[2] = {
	.x_st = 1920,
	.y_st = 0,
	},
	[3] = {
	.x_st = 2880,
	.y_st = 0,
	},
	[4] = { /*line 1*/
	.x_st = 0,
	.y_st = 540,
	},
	[5] = {
	.x_st = 960,
	.y_st = 540,
	},
	[6] = {
	.x_st = 1920,
	.y_st = 540,
	},
	[7] = {
	.x_st = 2880,
	.y_st = 540,
	},
	[8] = { /*line 2*/
	.x_st = 0,
	.y_st = 1080,
	},
	[9] = {
	.x_st = 960,
	.y_st = 1080,
	},
	[10] = {
	.x_st = 1920,
	.y_st = 1080,
	},
	[11] = {
	.x_st = 2880,
	.y_st = 1080,
	},
	[12] = { /*line 3*/
	.x_st = 0,
	.y_st = 1620,
	},
	[13] = {
	.x_st = 960,
	.y_st = 1620,
	},
	[14] = {
	.x_st = 1920,
	.y_st = 1620,
	},
	[15] = {
	.x_st = 2880,
	.y_st = 1620,
	},
};

int dim_post_copy_pip(struct di_ch_s *pch,
		      struct vframe_s *vfm_in,
		      struct di_buf_s *di_buf,
		      unsigned int winmode)
{
	struct di_hpst_s  *pst = get_hw_pst();
	unsigned int ch;
	struct mem_cpy_s	*cfg_cpy;
	struct di_post_stru_s *ppost;
	struct dim_wmode_s	wmode;
	struct dim_wmode_s	*pwm;
	struct dim_cvsi_s	*cvsi_out;
	struct AFBCD_S		*in_afbcd	= NULL;
	struct AFBCE_S		*o_afbce	= NULL;
	struct di_buf_s		*in_buf		= NULL;
	struct DI_MIF_S		*i_mif		= NULL;
	struct DI_SIM_MIF_s	*o_mif;
	unsigned int h, v;
	struct di_cvs_s *cvss;
	enum DI_MIF0_ID mif_index;
	enum EAFBC_DEC	dec_index = EAFBC_DEC_IF1;
	union hw_sc2_ctr_pre_s cfg;
	bool is_4k = false;

	struct di_win_s	win[DIM_PIP_WIN_NUB];
	struct di_win_s *pwin;
	unsigned int pip_nub = pst->cfg_pip_nub + 1;
	int i;
	unsigned int time_cnt = 0;
	u64 t_st, t_c;
	unsigned int us_diff[DIM_PIP_WIN_NUB + 2], diffa;
	unsigned int bgrh, bgrv;
	static unsigned int pstiong_shift, shif_cnt;

	ch	= pch->ch_id;
	ppost	= &pch->rse_ori.di_post_stru;
	cfg_cpy = &ppost->cfg_cpy;
	pwm	= &wmode;
	pst->cfg_from = DI_SRC_ID_AFBCD_IF1;
	cfg_cpy->afbcd_vf	= NULL;
	cvsi_out = &ppost->cvsi_out;
	cvss	= &get_datal()->cvs;
	dbg_copy("%s:pip_nub[%d]\n", __func__, pip_nub);
	memset(pwm, 0, sizeof(*pwm));
	pwm->src_h = vfm_in->height;
	pwm->src_w = vfm_in->width;

	if (winmode & DI_BIT3) {
		bgrh = 1920;
		bgrv = 1080;
	} else {
		bgrh = 3840;
		bgrv = 2160;
	}

	if (IS_PROG(vfm_in->type))
		pwm->is_i = 0;
	else
		pwm->is_i = 1;

	if (IS_COMP_MODE(vfm_in->type)) {
		pwm->is_afbc = 1;
		pwm->src_h = vfm_in->compHeight;
		pwm->src_w = vfm_in->compWidth;
	}

	if (pwm->is_i)
		pwm->src_h = pwm->src_h >> 1;
	if ((pwm->src_w > 1920) || (pwm->src_h > 1920))
		is_4k = true;

	/**********************/
	if (pwm->is_afbc) {
		/* afbcd */
		in_afbcd = &ppost->in_afbcd;
		cfg_cpy->in_afbcd = in_afbcd;
		cfg_cpy->in_rdmif = NULL;

		memcpy(in_afbcd, &di_afbcd_din_def, sizeof(*in_afbcd));
		in_afbcd->hsize = pwm->src_w;
		in_afbcd->vsize = pwm->src_h;
		in_afbcd->head_baddr = vfm_in->compHeadAddr >> 4;
		in_afbcd->body_baddr = vfm_in->compBodyAddr >> 4;
		if (pst->cfg_rot) {
			if (pst->cfg_rot == 1) {
				/*180*/
				in_afbcd->rev_mode	= 3;
				in_afbcd->rot_en	= 0;
			} else if (pst->cfg_rot == 2) {
				/*90*/
				in_afbcd->rev_mode	= 2;
				in_afbcd->rot_en	= 1;
			} else if (pst->cfg_rot == 3) {
				/*270*/
				in_afbcd->rev_mode	= 1;
				in_afbcd->rot_en	= 1;
			}
		} else {
			in_afbcd->rot_en	= 0;
			in_afbcd->rev_mode	= 0;
		}
		dbg_copy("%s:rev_mode =%d\n", __func__, in_afbcd->rev_mode);
		//in_afbcd->rot_en	= pst->cfg_rot;
		in_afbcd->dos_uncomp	= 1; //tmp for

		if (vfm_in->type & VIDTYPE_VIU_422)
			in_afbcd->fmt_mode = 1;
		else if (vfm_in->type & VIDTYPE_VIU_444)
			in_afbcd->fmt_mode = 0;
		else
			in_afbcd->fmt_mode = 2;/* 420 ?*/

		if (vfm_in->bitdepth & BITDEPTH_Y10)
			in_afbcd->compbits = 2;	// 10 bit
		else
			in_afbcd->compbits = 0; // 8bit

		if (vfm_in->bitdepth & BITDEPTH_SAVING_MODE) {
			//r |= (1 << 28); /* mem_saving_mode */
			in_afbcd->blk_mem_mode = 1;
		} else {
			in_afbcd->blk_mem_mode = 0;
		}
		if (vfm_in->type & VIDTYPE_SCATTER) {
			//r |= (1 << 29);
			in_afbcd->ddr_sz_mode = 1;
		} else {
			in_afbcd->ddr_sz_mode = 0;
		}

		in_afbcd->win_bgn_h = 0;
		in_afbcd->win_end_h = in_afbcd->win_bgn_h + in_afbcd->hsize - 1;
		in_afbcd->win_bgn_v = 0;
		in_afbcd->win_end_v = in_afbcd->win_bgn_v + in_afbcd->vsize - 1;
		in_afbcd->index = dec_index;
	} else {
		i_mif = &ppost->di_buf1_mif;
		cfg_cpy->in_afbcd = NULL;
		cfg_cpy->in_rdmif = i_mif;

		in_buf = &ppost->in_buf;
		memset(in_buf, 0, sizeof(*in_buf));

		in_buf->vframe = &ppost->in_buf_vf;
		memcpy(in_buf->vframe, vfm_in, sizeof(struct vframe_s));
		in_buf->vframe->private_data = in_buf;

		pre_cfg_cvs(in_buf->vframe);
		config_di_mif_v3(i_mif, DI_MIF0_ID_IF1, in_buf, ch);
		i_mif->mif_index = DI_MIF0_ID_IF1;
	}

	/* set vf */
	memcpy(di_buf->vframe, vfm_in, sizeof(struct vframe_s));
	di_buf->vframe->private_data = di_buf;
	di_buf->vframe->compHeight	= bgrv;	//pwm->src_h;
	di_buf->vframe->compWidth	= bgrh;	//pwm->src_w;
	di_buf->vframe->height	= bgrv;	//pwm->src_h;
	di_buf->vframe->width	= bgrh;	//pwm->src_w;
	di_buf->vframe->bitdepth &= ~(BITDEPTH_MASK);
	di_buf->vframe->bitdepth &= ~(FULL_PACK_422_MODE);
	/*clear*/
	di_buf->vframe->type &= ~(VIDTYPE_VIU_NV12	|
		       VIDTYPE_VIU_444	|
		       VIDTYPE_VIU_NV21 |
		       VIDTYPE_VIU_422	|
		       VIDTYPE_VIU_SINGLE_PLANE |
		       VIDTYPE_COMPRESS |
		       VIDTYPE_PRE_INTERLACE);

	//dbg_copy("di:typ[0x%x]:\n", di_buf->vframe->type);
	if (pst->cfg_out_enc)
		di_buf->vframe->type |= (VIDTYPE_COMPRESS	|
					 VIDTYPE_SCATTER	|
					 VIDTYPE_VIU_SINGLE_PLANE);
	if (pst->cfg_out_fmt == 0) {
		di_buf->vframe->type |= (VIDTYPE_VIU_444 |
					 VIDTYPE_VIU_SINGLE_PLANE);
	} else if (pst->cfg_out_fmt == 1) {
		di_buf->vframe->type |= (VIDTYPE_VIU_422 |
					 VIDTYPE_VIU_SINGLE_PLANE);
	} else {
		if (!pst->cfg_out_enc)
			di_buf->vframe->type |= VIDTYPE_VIU_NV21;
		else
			di_buf->vframe->type |= VIDTYPE_VIU_SINGLE_PLANE;
	}
	//dbg_copy("di:typ[0x%x]:\n", di_buf->vframe->type);
	if (pst->cfg_out_bit == 0)
		di_buf->vframe->bitdepth |= (BITDEPTH_Y8	|
				  BITDEPTH_U8	|
				  BITDEPTH_V8);
	else if (pst->cfg_out_bit == 2)
		di_buf->vframe->bitdepth |= (BITDEPTH_Y10	|
				  BITDEPTH_U10	|
				  BITDEPTH_V10	|
				  FULL_PACK_422_MODE);

	if (pst->cfg_out_enc) {
		/**********************/
		/* afbce */
		/*wr afbce */
		//flg_out_mif = false;
		o_afbce = &ppost->out_afbce;
		cfg_cpy->out_afbce = o_afbce;
		cfg_cpy->afbc_en = 1;
		cfg_cpy->out_wrmif = NULL;

		memcpy(o_afbce, &di_afbce_out_def, sizeof(*o_afbce));
		o_afbce->reg_init_ctrl = 0;
		o_afbce->head_baddr = di_buf->afbc_adr;
		o_afbce->mmu_info_baddr = di_buf->afbct_adr;
		o_afbce->reg_format_mode = 1; /* 422 tmp */
		//o_afbce->reg_compbits_y = 8;
		//o_afbce->reg_compbits_c = 8; /* 8 bit */
		if (pst->cfg_rot == 2 || pst->cfg_rot == 3) {
			h = pwm->src_h;
			v = pwm->src_w;
			//di_cnt_post_buf_rotation(pch, &cvsw, &cvsh);
			//dim_print("rotation:cvsw[%d],cvsh[%d]\n", cvsw, cvsh);
			//di_buf->canvas_width[0] = cvsw;
			//di_buf->canvas_height = cvsh;
			o_afbce->rot_en = 1;
		} else {
			h = pwm->src_w;
			v = pwm->src_h;
			o_afbce->rot_en = 0;
		}
	//		v = v / 2;
		o_afbce->hsize_in = h;
		o_afbce->vsize_in = v;
		o_afbce->enc_win_bgn_h = 0;
		o_afbce->enc_win_end_h = o_afbce->enc_win_bgn_h + h - 1;
		o_afbce->enc_win_bgn_v = 0;
		o_afbce->enc_win_end_v = o_afbce->enc_win_bgn_v + v - 1;

		//o_afbce->rot_en = pst->cfg_rot;
		#if 1
		//sync with afbce
		if (o_afbce->rot_en && cfg_cpy->in_afbcd) {
			/*sync with afbce 422*/
			in_afbcd->rot_ofmt_mode = o_afbce->reg_format_mode;
			if (o_afbce->reg_compbits_y == 8)
				in_afbcd->rot_ocompbit	= 0;
			else if (o_afbce->reg_compbits_y == 9)
				in_afbcd->rot_ocompbit	= 1;
			else/* if (o_afbce->reg_compbits_y == 10) */
				in_afbcd->rot_ocompbit	= 2;
		}
		#endif
		//di_buf->vframe->compHeight	= v;
		//di_buf->vframe->compWidth	= h;
		//di_buf->vframe->height	= v;
		//di_buf->vframe->width	= h;
		di_buf->vframe->compHeadAddr = di_buf->afbc_adr;
		di_buf->vframe->compBodyAddr = di_buf->nr_adr;

		//dim_print("%s:v[%d]\n", __func__, v);
		dim_print("\tblk[%d]:head[0x%lx], body[0x%lx]\n",
			  di_buf->blk_buf->header.index,
			  di_buf->afbct_adr,
			  di_buf->nr_adr);
	} else {
		o_mif = &ppost->di_diwr_mif;
		cfg_cpy->out_wrmif = o_mif;
		cfg_cpy->out_afbce = NULL;
		cvsi_out->cvs_id[0] = (unsigned char)cvss->post_idx[1][0];
		cvsi_out->plane_nub = 1;
		cvsi_out->cvs_cfg[0].phy_addr = di_buf->nr_adr;
		cvsi_out->cvs_cfg[0].width	= di_buf->canvas_width[0];
		cvsi_out->cvs_cfg[0].height	= di_buf->canvas_height;
		cvsi_out->cvs_cfg[0].block_mode	= 0;

		cvsi_out->cvs_cfg[0].endian	= 0;

		o_mif->canvas_num	= (unsigned short)cvsi_out->cvs_id[0];
		memcpy(&di_buf->vframe->canvas0_config[0],
		       &cvsi_out->cvs_cfg[0],
		       sizeof(di_buf->vframe->canvas0_config[0]));
		di_buf->vframe->plane_num = cvsi_out->plane_nub;
		di_buf->vframe->canvas0Addr = (u32)o_mif->canvas_num;

		/* set cvs */
		cvsi_cfg(cvsi_out);

		opl1()->wr_cfg_mif(o_mif, EDI_MIFSM_WR, di_buf->vframe, NULL);
	}

	di_buf->vframe->early_process_fun = dim_do_post_wr_fun;
	di_buf->vframe->process_fun = NULL;

	/* 2019-04-22 Suggestions from brian.zhu*/
	di_buf->vframe->mem_handle = NULL;
	di_buf->vframe->type |= VIDTYPE_DI_PW;
	di_buf->in_buf	= NULL;
	memcpy(&pst->vf_post, di_buf->vframe, sizeof(pst->vf_post));

	/**********************/

	if (pst->cfg_from == DI_SRC_ID_MIF_IF0 ||
	    pst->cfg_from == DI_SRC_ID_AFBCD_IF0) {
		//in_buf = acfg->buf_mif[0];
		mif_index = DI_MIF0_ID_IF0;
		dec_index = EAFBC_DEC_IF0;
		cfg_cpy->port = 0x81;
	} else if (pst->cfg_from == DI_SRC_ID_MIF_IF1	||
		 pst->cfg_from == DI_SRC_ID_AFBCD_IF1) {
		//in_buf = acfg->buf_mif[1];
		mif_index = DI_MIF0_ID_IF1;
		dec_index = EAFBC_DEC_IF1;
		cfg_cpy->port = 0x92;
	} else {
		//in_buf = acfg->buf_mif[2];
		mif_index = DI_MIF0_ID_IF2;
		dec_index = EAFBC_DEC_IF2;
		cfg_cpy->port = 0xa3;
	}

	cfg_cpy->hold_line = 4;
	cfg_cpy->opin	= &di_pre_regset;

	dbg_copy("is_4k[%d]\n", is_4k);
	#if 1
	if (is_4k)
		dim_sc2_4k_set(2);
	else
		dim_sc2_4k_set(0);
	#endif
	cfg.d32 = 0;
	dim_sc2_contr_pre(&cfg);
	if (cfg_cpy->in_afbcd)
		dim_print("1:afbcd:0x%px\n", cfg_cpy->in_afbcd);
	if (cfg_cpy->in_rdmif)
		dim_print("2:in_rdmif:0x%px\n", cfg_cpy->in_rdmif);
	if (cfg_cpy->out_afbce)
		dim_print("3:out_afbce:0x%px\n", cfg_cpy->out_afbce);
	if (cfg_cpy->out_wrmif)
		dim_print("4:out_wrmif:0x%px\n", cfg_cpy->out_wrmif);

	/*************************************/
	t_st = cur_to_usecs();
	o_afbce->hsize_bgnd = bgrh;
	o_afbce->vsize_bgnd = bgrv;
	di_buf->vframe->compWidth	= bgrh;
	di_buf->vframe->compHeight	= bgrv;
	dbg_copy("%s:pip_nub[%d]\n", __func__, pip_nub);
	for (i = 0; i < pip_nub; i++) {
		time_cnt = 0;
		if (winmode & DI_BIT1)
			memcpy(&win[i], &win_pip_16[i], sizeof(win[i]));
		else
			memcpy(&win[i], &win_pip[i], sizeof(win[i]));
		win[i].x_size = pwm->src_w;
		win[i].y_size = pwm->src_h;
		if ((winmode & DI_BIT0) && (i == 1)) {
			if ((pstiong_shift % 30) == 0) {
				shif_cnt = pstiong_shift / 30;

				if ((win[i].x_st - (shif_cnt * 4)) <
				    win_pip[3].x_st)
					win[i].x_st -= shif_cnt * 4;
				else
					pstiong_shift = 0;
				if ((win[i].y_st + (shif_cnt * 4)) <
				    win_pip[3].y_st)
					win[i].y_st += shif_cnt * 4;
				else
					pstiong_shift = 0;
			} else {
				win[i].x_st -= shif_cnt * 4;
				win[i].y_st += shif_cnt * 4;
			}
			pstiong_shift++;
			PR_INF("x_st[%d], y_st[%d], %d\n",
			       win[i].x_st, win[i].y_st, pstiong_shift);
		}
		pwin  = &win[i];
		if (o_afbce) {
			o_afbce->enc_win_bgn_h = pwin->x_st;
			o_afbce->enc_win_end_h = o_afbce->enc_win_bgn_h + h - 1;
			o_afbce->enc_win_bgn_v = pwin->y_st;
			o_afbce->enc_win_end_v = o_afbce->enc_win_bgn_v + v - 1;
			if (i == 0)
				o_afbce->reg_init_ctrl = 1;
			else
				o_afbce->reg_init_ctrl = 0;
			o_afbce->reg_pip_mode = 1;
		}
		dbg_copy("%s:%d <%d:%d:%d:%d>\n", __func__, i,
			 o_afbce->enc_win_bgn_h,
			 o_afbce->enc_win_end_h,
			 o_afbce->enc_win_bgn_v,
			 o_afbce->enc_win_end_v);
		/********************/
		opl2()->memcpy(cfg_cpy);

		pst->pst_tst_use	= 1;
		pst->flg_int_done	= false;
		/* wait finish */
		while (time_cnt < 50) {
			if (pst->flg_int_done)
				break;
			//msleep(3);
			usleep_range(1000, 1001);
			time_cnt++;
		}
		if (!pst->flg_int_done) {
			PR_ERR("%s:copy failed\n", __func__);
			pw_vf_put(vfm_in, ch);
			break;
		}
		t_c = cur_to_usecs();
		us_diff[i] = (unsigned int)(t_c - t_st);
	}
	diffa = 0;
	for (i = 0; i < DIM_PIP_WIN_NUB; i++) {
		diffa = us_diff[i] - diffa;
		dbg_copy("\t:[%d]:%d:%d\n", i, us_diff[i], diffa);
		diffa = us_diff[i];
	}

	return 0;
}

/************************************************
 * mode:
 *	[16]: still or not;;
 *	[3:0]: nub
 *	[7:4]:
 *		[4]:pip mode pic 1 move;
 *		[5]:select pip window;
 *		[7]:select background size (4k or 1080)
 ************************************************/
void dbg_pip_func(struct di_ch_s *pch, unsigned int mode)
{
	struct vframe_s		*vfm_in = NULL;
	struct di_buf_s		*di_buf;
	unsigned int ch;
	struct di_hpst_s  *pst = get_hw_pst();

	ch	= pch->ch_id;
	/**********************/
	pst->cfg_rot = 0;
	pst->cfg_out_bit = 2;
	pst->cfg_out_enc = 1;
	pst->cfg_out_fmt = 1;
	/**********************/
	/* vfm */
	vfm_in = pw_vf_peek(ch);
	if (!vfm_in) {
		dbg_copy("no input");
		return;
	}
	//dbg_copy("%s:vf:%px\n", __func__, vfm_in);
	/**********************/
	if (di_que_is_empty(ch, QUE_POST_FREE))
		return;
	/**********************/
	vfm_in	= pw_vf_get(ch);
	di_buf	= di_que_out_to_di_buf(ch, QUE_POST_FREE);

	/**********************/
	pst->cfg_pip_nub = (mode & 0xf);
	dim_post_copy_pip(pch, vfm_in, di_buf, (mode & 0xf0) >> 4);

	di_que_in(ch, QUE_POST_READY, di_buf);
	dbg_copy("di:typ2[0x%x]:\n", di_buf->vframe->type);
	pw_vf_put(vfm_in, ch);
}
#else

void dbg_cp_4k(struct di_ch_s *pch, unsigned int mode)
{
}

void dbg_pip_func(struct di_ch_s *pch, unsigned int mode)
{
}
#endif
