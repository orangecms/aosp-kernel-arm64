/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * drivers/amlogic/media/di_multi/di_data_l.h
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

#ifndef __DI_DATA_L_H__
#define __DI_DATA_L_H__

#include <linux/amlogic/media/vfm/vframe.h>
#include <linux/amlogic/media/vfm/vframe_provider.h>
#include <linux/amlogic/media/vfm/vframe_receiver.h>

#include <linux/kfifo.h>	/*ary add*/

#include "../deinterlace/di_pqa.h"
//#include "di_pqa.h"

#define DI_CHANNEL_NUB	(2)
#define DI_CHANNEL_MAX  (4)

/* for vfm mode limit input vf */
#define DIM_K_VFM_IN_LIMIT		(2)

#define TABLE_FLG_END	(0xfffffffe)
#define TABLE_LEN_MAX (1000)
#define F_IN(x, a, b)	(((x) > (a)) && ((x) < (b)))
#define COM_M(m, a, b)	(((a) & (m)) == ((b) & (m)))
#define COM_MV(a, m, v)	(((a) & (m)) == (v))
#define COM_ME(a, m)	(((a) & (m)) == (m))

#define DI_BIT0		0x00000001
#define DI_BIT1		0x00000002
#define DI_BIT2		0x00000004
#define DI_BIT3		0x00000008
#define DI_BIT4		0x00000010
#define DI_BIT5		0x00000020
#define DI_BIT6		0x00000040
#define DI_BIT7		0x00000080
#define DI_BIT8		0x00000100
#define DI_BIT9		0x00000200
#define DI_BIT10	0x00000400
#define DI_BIT11	0x00000800
#define DI_BIT12	0x00001000
#define DI_BIT13	0x00002000
#define DI_BIT14	0x00004000
#define DI_BIT15	0x00008000
#define DI_BIT16	0x00010000
#define DI_BIT17	0x00020000
#define DI_BIT18	0x00040000
#define DI_BIT19	0x00080000
#define DI_BIT20	0x00100000
#define DI_BIT21	0x00200000
#define DI_BIT22	0x00400000
#define DI_BIT23	0x00800000
#define DI_BIT24	0x01000000
#define DI_BIT25	0x02000000
#define DI_BIT26	0x04000000
#define DI_BIT27	0x08000000
#define DI_BIT28	0x10000000
#define DI_BIT29	0x20000000
#define DI_BIT30	0x40000000
#define DI_BIT31	0x80000000

/*****************************************
 *
 * vframe mask
 *
 *****************************************/

#define DI_VFM_T_MASK_CHANGE		\
	(VIDTYPE_VIU_422		\
	| VIDTYPE_VIU_SINGLE_PLANE	\
	| VIDTYPE_VIU_444		\
	| VIDTYPE_INTERLACE		\
	| VIDTYPE_COMPRESS		\
	| VIDTYPE_MVC)

enum EDI_MEM_M {
	EDI_MEM_M_REV = 0,
	EDI_MEM_M_CMA = 1,
	EDI_MEM_M_CMA_ALL = 2,
	EDI_MEM_M_CODEC_A = 3,
	EDI_MEM_M_CODEC_B = 4,
	EDI_MEM_M_MAX	/**/
};

enum EDPST_OUT_MODE {
	EDPST_OUT_MODE_DEF,
	EDPST_OUT_MODE_NV21,
	EDPST_OUT_MODE_NV12,
};

enum DIME_REG_MODE {
	DIME_REG_MODE_VFM,	/* vframe */
	DIME_REG_MODE_NEW,	/* new infterface */
};

/* ************************************** */
/* *************** cfg top ************** */
/* ************************************** */
/* also see: di_cfg_top_ctr*/

enum EDI_CFG_TOP_IDX {
	/* cfg for top */
	EDI_CFG_BEGIN,
	EDI_CFG_MEM_FLAG,
	EDI_CFG_FIRST_BYPASS,
	EDI_CFG_REF_2,
	EDI_CFG_PMODE,
	/* EDI_CFG_PMODE
	 * 0:as p;
	 * 1:as i;
	 * 2:use 2 i buf
	 */
	EDI_CFG_KEEP_CLEAR_AUTO,
	EDI_CFG_MEM_RELEASE_BLOCK_MODE,
	EDI_CFG_FIX_BUF,
	EDI_CFG_PAUSE_SRC_CHG,
	EDI_CFG_4K,
	EDI_CFG_POUT_FMT,
	EDI_CFG_DAT,
	EDI_CFG_ALLOC_WAIT, /* alloc wait */
	EDI_CFG_KEEP_DEC_VF,
	EDI_CFG_POST_NUB,
	EDI_CFG_BYPASS_MEM,
	EDI_CFG_END,
};

#define cfgeq(a, b) ((di_cfg_top_get(EDI_CFG_##a) == (b)) ? true : false)
#define cfgnq(a, b) ((di_cfg_top_get(EDI_CFG_##a) != (b)) ? true : false)
#define cfgg(a) di_cfg_top_get(EDI_CFG_##a)
#define cfgs(a, b) di_cfg_top_set(EDI_CFG_##a, b)
#define K_DI_CFG_NUB	(EDI_CFG_END - EDI_CFG_BEGIN + 1)
#define K_DI_CFG_T_FLG_NOTHING	(0x00)
#define K_DI_CFG_T_FLG_DTS	(0x01)
#define K_DI_CFG_T_FLG_NONE	(0x80)
union di_cfg_tdata_u {
	unsigned int d32;
	struct {
		unsigned int val_df:4,/**/
		val_dts:4,
		val_dbg:4,
		val_c:4,
		dts_en:1,
		dts_have:1,
		dbg_have:1,
		rev_en:1,
		en_update:4,
		reserved:8;
	} b;
};

struct di_cfg_ctr_s {
	char *dts_name;
	enum EDI_CFG_TOP_IDX id;
	unsigned char	default_val;
	unsigned char	flg;
};

/* ************************************** */
/* *************** cfg x *************** */
/* ************************************** */
/*also see di_cfgx_ctr*/
enum EDI_CFGX_IDX {
	/* cfg channel x*/
	EDI_CFGX_BEGIN,
	EDI_CFGX_BYPASS_ALL,	/*bypass_all*/
	EDI_CFGX_END,

	/* debug cfg x */
	EDI_DBG_CFGX_BEGIN,
	EDI_DBG_CFGX_IDX_VFM_IN,
	EDI_DBG_CFGX_IDX_VFM_OT,
	EDI_DBG_CFGX_END,
};

#define K_DI_CFGX_NUB	(EDI_DBG_CFGX_END - EDI_CFGX_BEGIN + 1)

struct di_cfgx_ctr_s {
	char *name;
	enum EDI_CFGX_IDX id;
	bool	default_val;
};

/* ****************************** */
enum EDI_SUB_ID {
	DI_SUB_ID_S0,	/*DI_SUB_ID_MARST,*/
	DI_SUB_ID_S1,
	DI_SUB_ID_S2,
	DI_SUB_ID_S3,
	/*DI_SUB_ID_NUB,*/
};

/*debug vframe type */
struct di_vframe_type_info {
	char *name;
	unsigned int mask;
	char *other;
};

struct di_dbg_datax_s {
	struct vframe_s vfm_input;	/*debug input vframe*/
	struct vframe_s *pfm_out;	/*debug di_get vframe*/
	/*timer:*/
	u64 us_reg_begin;
	u64 us_reg_end;
	u64 us_unreg_begin;
	u64 us_unreg_end;
	u64 us_first_get;
	u64 us_first_ready;

};

/*debug function*/
enum EDI_DBG_F {
	EDI_DBG_F_00,
	EDI_DBG_F_01,
	EDI_DBG_F_02,
	EDI_DBG_F_03,
	EDI_DBG_F_04,
	EDI_DBG_F_05,
	EDI_DBG_F_06,
	EDI_DBG_F_07,
	EDI_DBG_F_08,
};

struct di_dbg_func_s {
	enum EDI_DBG_F index;
	void (*func)(unsigned int para);
	char *name;
	char *info;
};

/*register*/
struct reg_t {
	unsigned int add;
	unsigned int bit;
	unsigned int wid;
/*	unsigned int id;*/
	unsigned int df_val;
	char *name;
	char *bname;
	char *info;
};

#ifdef MARK_SC2
struct reg_acc {
	void (*wr)(unsigned int adr, unsigned int val);
	unsigned int (*rd)(unsigned int adr);
	unsigned int (*bwr)(unsigned int adr, unsigned int val,
			    unsigned int start, unsigned int len);
	unsigned int (*brd)(unsigned int adr, unsigned int start,
			    unsigned int len);
};
#endif
/**************************************/
/* time out */
/**************************************/

enum EDI_TOUT_CONTR {
/*	eDI_TOUT_CONTR_INT,*/
	EDI_TOUT_CONTR_EN,
	EDI_TOUT_CONTR_FINISH,
	EDI_TOUT_CONTR_CHECK,

	EDI_TOUT_CONTR_CLEAR,
	EDI_TOUT_CONTR_RESET,
};

struct di_time_out_s {
	bool en;
	unsigned long	timer_start;
	unsigned int	timer_thd;
	unsigned int	over_flow_cnt;
	bool		flg_over;
/*	bool (*do_func)(void);*/
};

struct di_func_tab_s {
	unsigned int index;
	bool (*func)(void);
};

/****************************************/
/* do_table				*/
/****************************************/

/*for do_table_ops_s id*/
#define K_DO_TABLE_ID_PAUSE		0
#define K_DO_TABLE_ID_STOP		1
#define K_DO_TABLE_ID_START		2

/*for mark of do_table_ops_s id*/
#define K_DO_TABLE_CAN_STOP		0x01

/*for op ret*/
#define K_DO_TABLE_R_B_FINISH		0x01/*bit 0: 0:not finish, 1:finish*/
/*bit 1: 0: to other index; 1: to next*/
#define K_DO_TABLE_R_B_NEXT		0x02
#define K_DO_TABLE_R_B_OTHER		0xf0	/*bit [7:4]: other index*/
#define K_DO_TABLE_R_B_OTHER_SHIFT	4	/*bit [7:4]: other index*/

#define K_DO_R_FINISH	(K_DO_TABLE_R_B_FINISH | K_DO_TABLE_R_B_NEXT)
#define K_DO_R_NOT_FINISH	(0)
#define K_DO_R_JUMP(a)	(K_DO_TABLE_R_B_FINISH |	\
	(((a) << K_DO_TABLE_R_B_OTHER_SHIFT) & K_DO_TABLE_R_B_OTHER))

enum EDO_TABLE_CMD {
	EDO_TABLE_CMD_NONE,
	EDO_TABLE_CMD_STOP,
	EDO_TABLE_CMD_START,
	EDO_TABLE_CMD_PAUSE,
	EDO_TABLE_CMD_STEP,
	EDO_TABLE_CMD_STEP_BACK,
};

struct do_table_ops_s {
	/*bool stop_mark;*/
	unsigned int id;
	unsigned int mark;	/*stop / pause*/
	bool (*con)(void *data);/*condition*/
	unsigned int (*do_op)(void *data);
	unsigned int (*do_stop_op)(void *data);
	char *name;

};

struct do_table_s {
	const struct do_table_ops_s *ptab;
	unsigned int size;
	unsigned int op_lst;
	unsigned int op_crr;
	void *data;
	bool do_stop;	/*need do stops	*/
	bool flg_stop;	/*have stop	*/

	bool do_pause;
	bool do_step;	/*step mode*/

	bool flg_repeat;
	char *name;

};

/*************************************/

/**********************/
/* vframe info */
/**********************/
struct di_vinfo_s {
	/*use this for judge type change or not */
	unsigned int ch;
	unsigned int vtype;
	unsigned int src_type;
	unsigned int trans_fmt;
	unsigned int h;
	unsigned int v;
};

/**************************************/
/* PRE */
/**************************************/
enum EDI_PRE_ST {
	EDI_PRE_ST_EXIT,
	EDI_PRE_ST_IDLE,	/*switch to next channel?*/
	EDI_PRE_ST_CHECK,
	EDI_PRE_ST_SET,
	EDI_PRE_ST_WAIT_INT,
	EDI_PRE_ST_TIMEOUT,
};

enum EDI_PRE_ST4 {	/*use this for co work with do table*/
	EDI_PRE_ST4_EXIT,
	EDI_PRE_ST4_IDLE,	/*switch to next channel?*/
	EDI_PRE_ST4_CHECK,	/*check mode do_table and set*/
	EDI_PRE_ST4_DO_TABLE,	/* do table statue;*/
};

struct di_pre_set_s {
	/*use to remember last hw pre setting;*/
	/*cfg:	*/
	bool cfg_mcpre_en;	/*mcpre_en*/

	unsigned int in_w;
	unsigned int in_h;
	unsigned int in_type;
	unsigned int src_type;
};

struct di_hpre_s {
	enum EDI_PRE_ST4 pre_st;
	unsigned int curr_ch;
	/*set when have vframe in; clear when int have get*/
	bool hw_flg_busy_pre;
/*	bool trig_unreg;*/	/*add for unreg flow;*/
/*	enum EDI_SUB_ID hw_owner_pre;*/
	/*bool flg_wait_int;*/
	atomic_t flg_wait_int;
	struct di_pre_stru_s *pres;
	struct di_post_stru_s *psts;
	struct di_time_out_s tout;	/*for time out*/
	bool flg_int_done;
	unsigned int check_recycle_buf_cnt;

	struct di_pre_set_s set_lst;
	struct di_pre_set_s set_curr;

	struct di_vinfo_s vinf_lst;
	struct di_vinfo_s vinf_curr;

	/* use do table to switch mode*/
	struct do_table_s sdt_mode;

	unsigned int idle_cnt;	/*use this avoid repeat idle <->check*/
	/*dbg flow:*/
	bool dbg_f_en;
	unsigned int dbg_f_lstate;
	unsigned int dbg_f_cnt;
	union hw_sc2_ctr_pre_s pre_top_cfg;

};

/**************************************/
/* POST */
/**************************************/
enum EDI_PST_ST {
	EDI_PST_ST_EXIT,
	EDI_PST_ST_IDLE,	/*switch to next channel?*/
	EDI_PST_ST_CHECK,
	EDI_PST_ST_SET,
	EDI_PST_ST_WAIT_INT,
	EDI_PST_ST_TIMEOUT,
	EDI_PST_ST_DONE,	/*use for bypass_all*/
};

struct di_hpst_s {
	enum EDI_PST_ST state;
	unsigned int curr_ch;
	/*set when have vframe in; clear when int have get*/
	bool hw_flg_busy_post;
	struct di_pre_stru_s *pres;
	struct di_post_stru_s *psts;
	struct di_time_out_s tout;	/*for time out*/
	bool flg_int_done;

	/*dbg flow:*/
	bool dbg_f_en;
	bool flg_have_set;
	unsigned int dbg_f_lstate;
	unsigned int dbg_f_cnt;
	struct vframe_s		vf_post;
	union hw_sc2_ctr_pst_s pst_top_cfg;
	unsigned int last_pst_size;
	/****************/
	unsigned int cfg_cp	: 1;
	unsigned int cfg_rot	: 2;
	unsigned int cfg_afbce	: 1; /*wr use afbce*/
	unsigned int cfg_from	: 4;	/* enum DI_SRC_ID */

	/*****/
	unsigned int pst_copy	: 1;
	unsigned int pst_tst_use: 2;
	unsigned int cfg_out_enc: 1;

	unsigned int cfg_out_fmt: 2;
	unsigned int cfg_out_bit: 2; /**/
	/*****/
	unsigned int cfg_pip_nub: 4;
	unsigned int cfg_rev2	: 4;
	/*****/
	unsigned int cfg_rev3	: 8;
	/****************/
};

/**************************************/
/* channel status */
/**************************************/
enum EDI_TOP_STATE {
	EDI_TOP_STATE_NOPROB,
	EDI_TOP_STATE_IDLE,	/*idle not work*/
	/* STEP1
	 * till peek vframe and set irq;before this state, event reg finish
	 */
	EDI_TOP_STATE_REG_STEP1,	/* wait vf */
	EDI_TOP_STATE_REG_STEP1_P1,	/* have vf input */
	EDI_TOP_STATE_REG_STEP2,	/*till alloc and ready*/
	EDI_TOP_STATE_READY,		/*can do DI*/
	EDI_TOP_STATE_BYPASS,		/*complet bypass*/
	EDI_TOP_STATE_UNREG_STEP1,	/*till pre/post is finish;*/
	/* do unreg and to IDLE.
	 * no need to wait cma release after  this unreg event finish
	 */
	EDI_TOP_STATE_UNREG_STEP2,
};

/**************************************/
/* thread and cmd */
/**************************************/
struct di_task {
	bool flg_init;
	struct semaphore sem;
	wait_queue_head_t wait_queue;
	struct task_struct *thread;
	unsigned int status;

	unsigned int wakeup;
	unsigned int delay;
	bool exit;
	/*not use cmd*/
	/*local event*/
	struct kfifo	fifo_cmd;
	spinlock_t     lock_cmd; /*spinlock*/
	bool flg_cmd;
	struct kfifo	fifo_cmd2[DI_CHANNEL_NUB];
	spinlock_t     lock_cmd2[DI_CHANNEL_NUB]; /*spinlock*/
	bool flg_cmd2[DI_CHANNEL_NUB];
	unsigned int err_cmd_cnt;
};

struct dim_fcmd_s {
	struct kfifo	fifo;
	spinlock_t     lock_w; /*spinlock*/
	spinlock_t     lock_r; /*spinlock*/
	unsigned int flg_lock;
	bool flg; /* flg for kfifo */

	bool alloc_cmd;
	bool release_cmd;
	unsigned int reg_nub;
	unsigned int reg_page; /*size >> page_shift*/
	int doing; /* inc in send_cmd, and set 0 when thread done*/
	int	sum_alloc; /* alloc ++, releas -- */
};

struct di_mtask {
	bool flg_init;
	struct semaphore sem;
	wait_queue_head_t wait_queue;
	struct task_struct *thread;
	unsigned int status;

	unsigned int wakeup;
	unsigned int delay;
	bool exit;

	struct dim_fcmd_s fcmd[DI_CHANNEL_NUB];
	unsigned int err_res; /* 0: no err; other have error */
	unsigned int err_cmd_cnt;
};

#define MAX_KFIFO_L_CMD_NUB	32

union   DI_L_CMD_BITS {
	unsigned int cmd32;
	struct {
		unsigned int id:8,	/*low bit*/
			     ch:8,	/*channel*/
			     p2:8,
			     p3:8;
	} b;
};

#define LCMD1(id, ch)	((id) | ((ch) << 8))
#define LCMD2(id, ch, p2)	((id) | ((ch) << 8) | ((p2) << 16))

enum ECMD_LOCAL {
	ECMD_NONE,
	ECMD_REG,
	ECMD_UNREG,
	ECMD_READY,
	ECMD_CHG,
	ECMD_RL_KEEP,
	ECMD_BLK,
	NR_FINISH,
};

/* for mem task */

#ifdef MARK_SC2
union   DI_L_CMD_BLK_BITS {
	unsigned int cmd32;
	struct {
		unsigned int cmd:4,	/*low bit*/
			     nub:4,	/**/
			     page:24;
	} b;
};

#define LCMD_BLK(cmd, nub, page)	((cmd) | ((nub) << 4) | ((page) << 8))
#endif
enum ECMD_BLK {
	ECMD_BLK_NONE,
	ECMD_BLK_ALLOC,
	ECMD_BLK_RELEASE,
	ECMD_BLK_RELEASE_ALL, /* ready to release */
};

enum EDIM_BLK_TYP {
	EDIM_BLK_TYP_PST_TEST,
	EDIM_BLK_TYP_OLDI,
	EDIM_BLK_TYP_OLDP,
	EDIM_BLK_TYP_BUFI,
	EDIM_BLK_TYP_BUFP,
	EDIM_BLK_TYP_DATI,
	EDIM_BLK_TYP_PAFBCT,
};

struct blk_flg_s {
	union {
		unsigned int d32;
		struct {
		unsigned int tvp	: 1;
		unsigned int afbc	: 1;
		unsigned int is_i	: 1;
		unsigned int dw		: 1;
		//2020-10-04 unsigned int rev1	: 4;
		unsigned int typ	: 4;
		unsigned int page: 24;
		} b;
	};
};

struct mtsk_cmd_s {
	unsigned int cmd	: 4;
	unsigned int rev1	: 4;
	unsigned int nub	: 8;
	unsigned int rev2	: 16;
	struct blk_flg_s	flg;
};

/**************************************
 * QUE
 * keep same order as di_name_new_que
 **************************************/
enum QUE_TYPE {	/*mast start from 0 */
	QUE_IN_FREE,	/*5*/
	QUE_PRE_READY,	/*6*/
	QUE_POST_FREE,	/*7*/
	QUE_POST_READY,	/*8*/
	QUE_POST_BACK,		/*new*/
	QUE_POST_DOING,
	QUE_POST_KEEP,	/*below use pw_queue_in*/
	QUE_POST_KEEP_BACK,
	QUE_POST_KEEP_RE_ALLOC, /*need*/
	QUE_PRE_NO_BUF_WAIT,	//
	QUE_PST_NO_BUF_WAIT,	//
	QUE_PRE_NO_BUF, /*ary add before local_free*/
	QUE_PST_NO_BUF,
	/*----------------*/
	QUE_DBG,
	QUE_NUB,
};

/*#define QUE_NUB  (5)*/
enum EDI_BUF_TYPE {
	EDI_BUF_T_IN = 1,	/*VFRAME_TYPE_IN*/
	EDI_BUF_T_LOCAL,	/*VFRAME_TYPE_LOCAL*/
	EDI_BUF_T_POST,		/*VFRAME_TYPE_POST*/
};

#define MAX_FIFO_SIZE	(32)

/**************************************
 *
 * summmary variable
 * also see:di_sum_name_tab
 **************************************/

enum EDI_SUM {
	EDI_SUM_O_PEEK_CNT,	/*video_peek_cnt*/
	EDI_SUM_REG_UNREG_CNT,	/*di_reg_unreg_cnt*/
	EDI_SUM_NUB,
};

struct di_sum_s {
	char *name;
	enum EDI_SUM	index;
	unsigned int	default_val;
};

/**************************************
 *
 * module para
 *	int
 *	EDI_MP_SUB_DI_B
 *	EDI_MP_SUB_NR_B
 *	EDI_MP_SUB_PD_B
 *	EDI_MP_SUB_MTN_B
 *	EDI_MP_SUB_3D_B
 **************************************/
enum EDI_MP_UI_T {
	/*keep same order with di_mp_ui_top*/
	EDI_MP_UI_T_BEGIN,
	/**************************************/
	EDI_MP_SUB_DI_B,

	edi_mp_force_prog,	/*force_prog bool*/
	edi_mp_combing_fix_en,	/*combing_fix_en bool*/
	edi_mp_cur_lev,		/*cur_lev*/
	edi_mp_pps_dstw,	/*pps_dstw*/
	edi_mp_pps_dsth,	/*pps_dsth*/
	edi_mp_pps_en,		/*pps_en*/
	edi_mp_pps_position,	/*pps_position*/
	edi_mp_pre_enable_mask,	/*pre_enable_mask*/
	edi_mp_post_refresh,	/*post_refresh*/
	edi_mp_nrds_en,		/*nrds_en*/
	edi_mp_bypass_3d,	/*bypass_3d*/
	edi_mp_bypass_trick_mode,	/*bypass_trick_mode*/
	edi_mp_invert_top_bot,	/*invert_top_bot */
	edi_mp_skip_top_bot,
	edi_mp_force_width,
	edi_mp_force_height,
	edi_mp_prog_proc_config,
	edi_mp_start_frame_drop_count,
	edi_mp_same_field_top_count,	/*long?*/
	edi_mp_same_field_bot_count,	/*long?*/
	edi_mp_vpp_3d_mode,
	edi_mp_force_recovery_count,
	edi_mp_pre_process_time,	/*no use?*/
	edi_mp_bypass_post,
	edi_mp_post_wr_en,
	edi_mp_post_wr_support,
	edi_mp_bypass_post_state,
	edi_mp_use_2_interlace_buff,
	edi_mp_debug_blend_mode,
	edi_mp_nr10bit_support,
	edi_mp_di_stop_reg_flag,
	edi_mp_mcpre_en,
	edi_mp_check_start_drop, /* eDI_MP_check_start_drop_prog */
	edi_mp_overturn,			/*? in init*/
	edi_mp_full_422_pack,
	edi_mp_cma_print,
	edi_mp_pulldown_enable,
	edi_mp_di_force_bit_mode,
	edi_mp_calc_mcinfo_en,
	edi_mp_colcfd_thr,
	edi_mp_post_blend,
	edi_mp_post_ei,
	edi_mp_post_cnt,
	edi_mp_di_log_flag,
	edi_mp_di_debug_flag,
	edi_mp_buf_state_log_threshold,
	edi_mp_di_vscale_skip_enable,
	edi_mp_di_vscale_skip_count,
	edi_mp_di_vscale_skip_real, /* eDI_MP_di_vscale_skip_count_real */
	edi_mp_det3d_en,
	edi_mp_post_hold_line,
	edi_mp_post_urgent,
	edi_mp_di_printk_flag,
	edi_mp_force_recovery,
/*	edi_mp_debug_blend_mode,*/
	edi_mp_di_dbg_mask,
	edi_mp_nr_done_check_cnt,
	edi_mp_pre_hsc_down_en,
	edi_mp_pre_hsc_down_width,
	edi_mp_show_nrwr,
	/********deinterlace_hw.c*********/
	edi_mp_pq_load_dbg,
	edi_mp_lmv_lock_win_en,
	edi_mp_lmv_dist,
	edi_mp_pr_mcinfo_cnt,
	edi_mp_offset_lmv,
	edi_mp_post_ctrl,
	edi_mp_if2_disable,
	edi_mp_pre, /* eDI_MP_pre_flag */
	edi_mp_pre_mif_gate,
	edi_mp_pre_urgent,
	edi_mp_pre_hold_line,
	edi_mp_pre_ctrl,
	edi_mp_line_num_post_frst,
	edi_mp_line_num_pre_frst,
	edi_mp_pd22_flg_calc_en,
	edi_mp_mcen_mode,
	edi_mp_mcuv_en,
	edi_mp_mcdebug_mode,
	edi_mp_pldn_ctrl_rflsh,

	EDI_MP_SUB_DI_E,
	/**************************************/
	EDI_MP_SUB_NR_B,
	edi_mp_dnr_en,
	edi_mp_nr2_en,
	edi_mp_cue_en,
	edi_mp_invert_cue_phase,
	edi_mp_cue_pr_cnt,
	edi_mp_cue_glb_mot_check_en,
	edi_mp_glb_fieldck_en,
	edi_mp_dnr_pr,
	edi_mp_dnr_dm_en,
	EDI_MP_SUB_NR_E,
	/**************************************/
	EDI_MP_SUB_PD_B,
	edi_mp_flm22_ratio,
	edi_mp_pldn_cmb0,
	edi_mp_pldn_cmb1,
	edi_mp_flm22_sure_num,
	edi_mp_flm22_glbpxlnum_rat,
	edi_mp_flag_di_weave,
	edi_mp_flm22_glbpxl_maxrow,
	edi_mp_flm22_glbpxl_minrow,
	edi_mp_cmb_3point_rnum,
	edi_mp_cmb_3point_rrat,
	/******film_fw1.c**/
	edi_mp_pr_pd,
	edi_mp_prt_flg,
	edi_mp_flmxx_maybe_num,
	edi_mp_flm32_mim_frms,
	edi_mp_flm22_dif01a_flag,
	edi_mp_flm22_mim_frms,
	edi_mp_flm22_mim_smfrms,
	edi_mp_flm32_f2fdif_min0,
	edi_mp_flm32_f2fdif_min1,
	edi_mp_flm32_chk1_rtn,
	edi_mp_flm32_ck13_rtn,
	edi_mp_flm32_chk2_rtn,
	edi_mp_flm32_chk3_rtn,
	edi_mp_flm32_dif02_ratio,
	edi_mp_flm22_chk20_sml,
	edi_mp_flm22_chk21_sml,
	edi_mp_flm22_chk21_sm2,
	edi_mp_flm22_lavg_sft,
	edi_mp_flm22_lavg_lg,
	edi_mp_flm22_stl_sft,
	edi_mp_flm22_chk5_avg,
	edi_mp_flm22_chk6_max,
	edi_mp_flm22_anti_chk1,
	edi_mp_flm22_anti_chk3,
	edi_mp_flm22_anti_chk4,
	edi_mp_flm22_anti_ck140,
	edi_mp_flm22_anti_ck141,
	edi_mp_flm22_frmdif_max,
	edi_mp_flm22_flddif_max,
	edi_mp_flm22_minus_cntmax,
	edi_mp_flagdif01chk,
	edi_mp_dif01_ratio,
	/*******vof_soft_top*****/
	edi_mp_cmb32_blw_wnd,
	edi_mp_cmb32_wnd_ext,
	edi_mp_cmb32_wnd_tol,
	edi_mp_cmb32_frm_nocmb,
	edi_mp_cmb32_min02_sft,
	edi_mp_cmb32_cmb_tol,
	edi_mp_cmb32_avg_dff,
	edi_mp_cmb32_smfrm_num,
	edi_mp_cmb32_nocmb_num,
	edi_mp_cmb22_gcmb_rnum,
	edi_mp_flmxx_cal_lcmb,
	edi_mp_flm2224_stl_sft,
	EDI_MP_SUB_PD_E,
	/**************************************/
	EDI_MP_SUB_MTN_B,
	edi_mp_force_lev,
	edi_mp_dejaggy_flag,
	edi_mp_dejaggy_enable,
	edi_mp_cmb_adpset_cnt,
	edi_mp_cmb_num_rat_ctl4,
	edi_mp_cmb_rat_ctl4_minthd,
	edi_mp_small_local_mtn,
	edi_mp_di_debug_readreg,
	EDI_MP_SUB_MTN_E,
	/**************************************/
	EDI_MP_SUB_3D_B,
	edi_mp_chessbd_vrate,
	edi_mp_det3d_debug,

	EDI_MP_SUB_3D_E,
	/**************************************/
	EDI_MP_UI_T_END,
};

#define K_DI_MP_UIT_NUB (EDI_MP_UI_T_END - EDI_MP_UI_T_BEGIN + 1)

struct di_mp_uit_s {
	char *name;
	enum EDI_MP_UI_T	id;
	int	default_val;
};

/*also see: di_mpx*/
enum EDI_MP_UIX_T {
	EDI_MP_UIX_BEGIN,
	EDI_MP_UIX_RUN_FLG, /*run_flag*/
	EDI_MP_UIX_END,
};

#define K_DI_MP_UIX_NUB (EDI_MP_UIX_END - EDI_MP_UIX_BEGIN + 1)

struct di_mp_uix_s {
	char *name;
	enum EDI_MP_UIX_T	id;
	unsigned int	default_val;
};

/**************************************/
/* DI WORKING MODE */
/**************************************/
enum EDI_WORK_MODE {
	EDI_WORK_MODE_NONE,
	EDI_WORK_MODE_BYPASS_COMPLET,
	EDI_WORK_MODE_BYPASS_ALL,	/*dim_is_bypass*/
	EDI_WORK_MODE_BYPASS_PRE,
	EDI_WORK_MODE_BYPASS_POST,
	EDI_WORK_MODE_I,
	EDI_WORK_MODE_P_AS_I,
	EDI_WORK_MODE_P_AS_P,
	EDI_WORK_MODE_P_USE_IBUF,
	EDI_WORK_MODE_NEW_2020, /* ary for test */
	EDI_WORK_MODE_ALL,

};

/**************************************/
/* vframe                             */
/**************************************/
struct dev_vfram_t {
	const char *name;
	/*receiver:*/
	struct vframe_receiver_s di_vf_recv;
	/*provider:*/
	struct vframe_provider_s di_vf_prov;

	unsigned int indx;
	/*status:*/
	bool bypass_complete;
	bool reg;	/*use this for vframe reg/unreg*/
/*	unsigned int data[32]; */	/*null*/

};

struct di_dat_s {
//	struct dim_mm_blk_s *blk_buf;
	void	*virt;
	bool	flg_alloc;

	struct blk_flg_s flg;
	unsigned long addr_st;
	unsigned long addr_end;
};

struct di_ores_s {
	/* same as ori */
	struct di_pre_stru_s di_pre_stru;
	struct di_post_stru_s di_post_stru;

	struct di_buf_s di_buf_local[MAX_LOCAL_BUF_NUM * 2];
	struct di_buf_s di_buf_in[MAX_IN_BUF_NUM];
	struct di_buf_s di_buf_post[MAX_POST_BUF_NUM];

	struct queue_s queue[QUEUE_NUM];
	struct di_buf_pool_s di_buf_pool[VFRAME_TYPE_NUM];

	struct vframe_s *vframe_in[MAX_IN_BUF_NUM];
	struct vframe_s vframe_in_dup[MAX_IN_BUF_NUM];
	struct vframe_s vframe_local[MAX_LOCAL_BUF_NUM * 2];
	struct vframe_s vframe_post[MAX_POST_BUF_NUM];
	/* ********** */
	struct di_dat_s	dat_i;
	struct di_dat_s dat_p_afbct;
};

enum EDI_CMA_ST {
	EDI_CMA_ST_IDL,
	EDI_CMA_ST_ALLOC,	/*do*/
	EDI_CMA_ST_READY,
	EDI_CMA_ST_RELEASE,	/*do*/
	EDI_CMA_ST_PART,
};

/**********************************
 * mem
 *********************************/
struct di_mm_cfg_s {
	/*support di size*/
	unsigned int di_h;
	unsigned int di_w;
	/**/
	unsigned int num_local;
	unsigned int num_post;
	unsigned int num_step1_post;

	unsigned int size_local;
	unsigned int size_post;
	unsigned int size_local_page;/*2020 for blk*/
	unsigned int size_post_page;/*2020 for blk*/
	unsigned int nr_size;
	unsigned int count_size;
	unsigned int mcinfo_size;
	unsigned int mv_size;
	unsigned int mtn_size;
	unsigned int di_size;	/* no afbc info size */
	unsigned int afbci_size;	/* afbc info size */
	unsigned int afbct_size;
	unsigned int afbct_local_max_size;
	unsigned int dw_size;
	unsigned int pst_buf_size;
	unsigned int pst_afbci_size;	/*07-28*/
	unsigned int pst_afbct_size;
	unsigned int pst_buf_uv_size;	/*07-28*/
	unsigned int pst_buf_y_size;	/*07-28*/
	unsigned int pst_cvs_w;		/*07-28*/
	unsigned int pst_cvs_h;		/*07-28*/
	unsigned int pst_mode;		/*07-28*/
	unsigned char buf_alloc_mode; /* 0: for i; 1:for prog*/
	/* 2020-06-19 */
	unsigned int canvas_width[3];
	int nr_size_p;
	unsigned int canvas_height; /* for i*/
	unsigned int canvas_height_mc;
	struct blk_flg_s ibuf_flg;
	struct blk_flg_s pbuf_flg;
	/*2020-10-04*/
	struct blk_flg_s dat_idat_flg;
	struct blk_flg_s dat_pafbci_flg;
	struct blk_flg_s dat_pafbct_flg;
	//unsigned int size_iafbc_all;
	unsigned int size_pafbct_all;
	unsigned int size_pafbct_one;

	unsigned int nub_pafbct;
	//unsigned int size_idat;
	unsigned int size_idat_all; /* 2020-10-12 */
	unsigned int size_idat_one;
	unsigned int nub_idat;

	unsigned int fix_buf	: 1;
	unsigned int dis_afbce	: 1;
	unsigned int rev1	: 30;
	unsigned int pre_inser_size;
};
struct dim_mm_t_s {
	/* use for reserved and alloc all*/
	unsigned long	mem_start;
	unsigned int	mem_size;
	struct page	*total_pages;
};

struct di_mm_st_s {
	unsigned long	mem_start;
	unsigned int	mem_size;
	int	num_local;
	int	num_post;	/*ppost*/
	unsigned int	flg_tvp;
	unsigned int	flg_realloc;
	unsigned int	num_pst_alloc;
	unsigned int	flg_release;
	int	cnt_alloc; /* debug only */
};

struct di_mm_s {
	struct di_mm_cfg_s cfg; /* clear in dip_init_value */
	struct di_mm_st_s sts;
};

struct dim_sum_s {
	/*buf*/
	unsigned int b_pre_free;
	unsigned int b_pst_ready;
	unsigned int b_recyc;
	unsigned int b_pre_ready;
	unsigned int b_pst_free;
	unsigned int b_display;
};

struct dim_bypass_s {
	union {
		unsigned int d32;
		struct {
			unsigned int need_bypass	: 1,
					is_bypass	: 1,
					lst_n		: 1,
					lst_i		: 1,
					rev1		: 4,
					reason_n	: 8,
					reason_i	: 8,
					rev2		: 8;
		} b;
	};
};

/************************************************
 * di_que_buf
 ***********************************************/
typedef uintptr_t ud;

#define tst_que_ele	(sizeof(unsigned int))//(sizeof(ud))//
#define tst_quep_ele	(sizeof(ud))//

#define DIM_DATA_MASK	(0xf1230000)
#define CODE_BLK	(DIM_DATA_MASK | 0x01)
#define CODE_INS	(DIM_DATA_MASK | 0x02)
#define CODE_HW		(DIM_DATA_MASK | 0x03)
#define CODE_LL		(DIM_DATA_MASK | 0x04)
#define CODE_PST	(DIM_DATA_MASK | 0x05)
#define CODE_MEMN	(DIM_DATA_MASK | 0x06)
#define CODE_PAT	(DIM_DATA_MASK | 0x07)
#define CODE_IAT	(DIM_DATA_MASK | 0x08)

#define CODE_OUT	(0xff123402)
#define CODE_OUT_MODE2	(0xff123403)
#define CODE_BYPASS	(0xff123405)

#define NONE_QUE	(0xff)

#define QS_ERR_LOG_SIZE		10
enum QS_FUNC_E {
	QS_FUNC_N_IN,
	QS_FUNC_N_O,
	QS_FUNC_N_EMPTY,
	QS_FUNC_N_FULL,
	QS_FUNC_N_LIST,
	QS_FUNC_N_PEEK,
	QS_FUNC_N_SOME,

	QS_FUNC_F_IN,// = 0x100,
	QS_FUNC_F_O,
	QS_FUNC_F_EMPTY,
	QS_FUNC_F_FULL,
	QS_FUNC_F_LIST,
	QS_FUNC_F_PEEK,
	QS_FUNC_F_SOME,
};

enum QS_ERR_E {
	QS_ERR_INDEX_OVERFLOW = 0x80000001,
	QS_ERR_BIT_CHECK,
	QS_ERR_FIFO_IN,
	QS_ERR_FIFO_O,
	QS_ERR_FIFO_PEEK,
	QS_ERR_FIFO_EMPTY,
	QS_ERR_FIFO_FULL,
	QS_ERR_FIFO_ALLOC,
	QS_ERR_FIFO_O_OVERFLOW,
};

/*use this as fix heard*/
struct qs_buf_s {
	unsigned int code;
	unsigned int ch;
	unsigned int index;
	unsigned int dbg_id;
};

union q_buf_u {
	struct qs_buf_s *qbc; /*qbuf control*/
};

struct qs_err_msg_s {
	enum QS_FUNC_E	func_id;
	enum QS_ERR_E	err_id;
	const char	*qname;
	unsigned int	index1;
	unsigned int	index2;
};

struct qs_err_log_s {
	unsigned int nub;
	unsigned int pos;
	struct qs_err_msg_s msg[QS_ERR_LOG_SIZE];
};

/********************************/

/*que tst*/
enum Q_TYPE {
	Q_T_FIFO,
	Q_T_N, /*bit map*/
	Q_T_FIFO_2,
	Q_T_N_2,
};

struct qs_n_s {
	int		nub;
	//struct mutex	mtex;
	//spinlock_t     lock_rw; /*read and write*/
	unsigned int	marsk;
	union q_buf_u	lst[MAX_FIFO_SIZE];
	//unsigned int	flg_lock;
};

struct qs_f_s {
	bool		flg;	/*1: have reg*/
	struct kfifo	fifo;
	//spinlock_t     lock_wr;
	//spinlock_t     lock_rd;
	//unsigned int	flg_lock;
};

struct qs_cls_s;

struct buf_que_s;

struct qsp_ops_s {
	bool (*reset)(struct buf_que_s *pqb, struct qs_cls_s *q);
	bool (*in)(struct buf_que_s *pqb, struct qs_cls_s *q,
		   union q_buf_u ubuf);
	bool (*is_empty)(struct buf_que_s *pqb, struct qs_cls_s *q);
	bool (*is_full)(struct buf_que_s *pqb, struct qs_cls_s *q);
	unsigned int (*count)(struct buf_que_s *pqb, struct qs_cls_s *q);
	bool (*list)(struct buf_que_s *pqb, struct qs_cls_s *q,
		     unsigned int *rsize);
	/*option*/
	bool (*out)(struct buf_que_s *pqb, struct qs_cls_s *q,
		    union q_buf_u *pbuf);
	bool (*peek)(struct buf_que_s *pqb, struct qs_cls_s *q,
		     union q_buf_u *pbuf);
	bool (*out_some)(struct buf_que_s *pqb, struct qs_cls_s *q,
			 union q_buf_u pbuf);
};

struct qs_cls_s {
	enum Q_TYPE	type;
	//unsigned int list[MAX_FIFO_SIZE];
	const char *name;
	bool	flg;
	struct qsp_ops_s ops;
	spinlock_t     lock_wr;/*spinlock*/
	spinlock_t     lock_rd;/*spinlock*/
	unsigned int	flg_lock;
	union {
		struct qs_n_s n;
		struct qs_f_s f;
	};
	struct qs_err_log_s *plog;
};

struct buf_que_s;

struct qb_ops_s {
	bool (*bufs_reset)(void *pbuf);
	bool (*bufs_init)(void *pbuf);
	bool (*list)(struct buf_que_s *pqb, unsigned int qindex,
		     unsigned int *rsize);
};

struct buf_que_s {
	//struct buf_que_s	*pqb;/*self*/
	unsigned int	type;
	unsigned int	list_id[MAX_FIFO_SIZE];
	ud		list_ud[MAX_FIFO_SIZE];
	//void	*pbuf[MAX_FIFO_SIZE];
	union q_buf_u	pbuf[MAX_FIFO_SIZE];/*point*/
	struct qs_cls_s	*pque[MAX_FIFO_SIZE];/**/
	bool	rflg;	/*resource flg*/
	char	*name;
	unsigned int	nub_que;
	unsigned int	nub_buf;
	struct qs_err_log_s log;
	struct qb_ops_s opsb;
	struct qs_f_s tmp_kfifo;

};

#define DIM_QUE_LOCK_RD	(DI_BIT0)
#define DIM_QUE_LOCK_WR	(DI_BIT1)
#define DIM_QUE_LOCK_RD_WR (DI_BIT0 | DI_BIT1)

struct que_creat_s {
	char		*name;
	enum Q_TYPE	type;
	/*need protect or not*/
	unsigned int	lock;
};

struct qbuf_creat_s {
	char *name;
	unsigned int nub_que;
	unsigned int nub_buf;
	unsigned int code;
};

/* di_que_buf end */
/************************************************/
/* que buf block */
enum QBF_BLK_Q_TYPE {
	QBF_BLK_Q_IDLE,
	QBF_BLK_Q_READY, /* multi wr, multi rd */
	QBF_BLK_Q_RCYCLE,
	QBF_BLK_Q_RCYCLE2, /*from mem */
	QBF_BLK_Q_NUB,
};

#define DIM_BLK_NUB	20 /* buf number*/
struct dim_mm_blk_s {
	struct qs_buf_s	header;

	unsigned long	mem_start;
//	unsigned int	size_page; /*size >> page_shift */
	struct page	*pages;
	struct blk_flg_s flg;
	bool	flg_alloc; /* alloc or release*/
	unsigned int	reg_cnt;
	//unsigned long jiff;
//	bool	tvp;
};

/*que buf block end*/
/************************************************/
/* que buf post afbc table */
enum QBF_PAT_Q_TYPE {
	QBF_PAT_Q_IDLE,
	QBF_PAT_Q_READY, /* multi wr, multi rd */
	QBF_PAT_Q_IN_USED,
	QBF_PAT_Q_NUB,
};

#define DIM_PAT_NUB	16 /* buf number*/
struct dim_pat_s {
	struct qs_buf_s	header;

	unsigned long	mem_start;
};

/*que post afbc tabl end*/
/************************************************/
/* que buf loacal buffer exit data */
enum QBF_IAT_Q_TYPE {
	QBF_IAT_Q_IDLE,
	QBF_IAT_Q_READY, /* multi wr, multi rd */
	QBF_IAT_Q_IN_USED,
	QBF_IAT_Q_NUB,
};

#define DIM_IAT_NUB	(MAX_LOCAL_BUF_NUM * 2) /* buf number*/
struct dim_iat_s {
	struct qs_buf_s	header;

//	unsigned long	start_idat;
	unsigned long	start_afbct;
	//unsigned long	start_mc;
	unsigned short	*mcinfo_adr_v;/**/
	bool		mcinfo_alloc_flg;
};

/*que loacal buffer exit data end*/

/************************************************/
/* que buf mem config */
enum QBF_MEM_Q_TYPE {
	QBF_MEM_Q_GET_PRE,	/*tmp*/
	QBF_MEM_Q_GET_PST,	/*tmp*/
	QBF_MEM_Q_IN_USED,
	QBF_MEM_Q_RECYCLE,
	QBF_MEM_Q_NUB,
};

/************************************************/

/************************************************
 * 2020-06-16 test
 ************************************************/

struct dim_wmode_s {
	//enum EDIM_TMODE		tmode;
	unsigned int	buf_type;	/*add this to split kinds */
	unsigned int	is_afbc		:1,
		is_vdin			:1,
		is_i			:1,
		need_bypass		:1,
		is_bypass		:1,
		pre_bypass		:1,
		post_bypass		:1,
		flg_keep		:1, /*keep buf*/

		trick_mode		:1,
		prog_proc_config	:1, /*debug only: proc*/
	/**************************************
	 *prog_proc_config:	same as p_as_i?
	 *1: process p from decoder as field
	 *0: process p from decoder as frame
	 ***************************************/
		is_invert_tp		:1,
		p_as_i			:1,
		p_as_p			:1,
		p_use_2i		:1,
		is_angle		:1,
		is_top			:1, /*include */
		is_eos			:1,
		is_eos_insert		:1, /* this is internal eos */
		bypass			:1, /* is_bypass | need_bypass*/
		reserved		:13;
	unsigned int vtype;	/*vfm->type*/
	//unsigned int h;	/*taget h*/
	//unsigned int w;	/*taget w*/
	unsigned int src_h;
	unsigned int src_w;
	unsigned int tgt_h;
	unsigned int tgt_w;
	unsigned int o_h;
	unsigned int o_w;
	unsigned int seq;
	unsigned int seq_sgn;
};

enum EDIM_TMODE {
	EDIM_TMODE_NONE,
	EDIM_TMODE_1_PW_VFM,
	/* EDIM_TMODE_1_PW_LOCAL ******
	 * pre + post write
	 * all buf alloc by di
	 * use vframe event
	 ******************************/
	EDIM_TMODE_2_PW_OUT,
	/* EDIM_TMODE_2_PRE_OUT ******
	 * pre + post write
	 * post buf alloc by other module
	 * not use vframe path
	 * add 2019-11-26 for zhouzhi
	 ******************************/
	EDIM_TMODE_3_PW_LOCAL,
	/* EDIM_TMODE_2_PRE_OUT ******
	 * pre + post write
	 * post buf alloc by self
	 * not use vframe path
	 * add 2019-12-04 for test
	 ******************************/
};

/************************************************/
struct di_ch_s {
	/*struct di_cfgx_s dbg_cfg;*/
	bool cfgx_en[K_DI_CFGX_NUB];
	unsigned int mp_uix[K_DI_MP_UIX_NUB];/*module para x*/

	struct di_dbg_datax_s dbg_data;

	struct dev_vfram_t vfm;
	enum vframe_source_type_e	src_type;
	struct dentry *dbg_rootx;	/*dbg_fs*/

	unsigned int ch_id;
	struct di_ores_s rse_ori;
	struct kfifo	fifo[QUE_NUB];
	bool flg_fifo[QUE_NUB];	/*have ini: 1; else 0*/
/*	bool sub_act_flg;*/
	/************************/
	/*old glob*/
	/************************/
	/*bypass_state*/
	bool bypass_state;

	/*video_peek_cnt*/
	unsigned int sum[EDI_SUM_NUB + 1];
	unsigned int sum_get;
	unsigned int sum_put;
	unsigned int sum_reg_cnt;
	unsigned int sum_pst_get;
	unsigned int sum_pst_put;
	struct dim_sum_s	sumx;
	struct dim_bypass_s	bypass; /*state only*/
	enum EDPST_MODE mode;

	/* qb: blk */
	struct buf_que_s blk_qb;
	struct qs_cls_s	blk_q[QBF_BLK_Q_NUB];
	struct dim_mm_blk_s	blk_bf[DIM_BLK_NUB];
	/* blk end */
	/* qb:mem */
	struct buf_que_s mem_qb;
	struct qs_cls_s	mem_q[QBF_MEM_Q_NUB];
	/* qb: post afbct 2020-10-05 */
	struct buf_que_s pat_qb;
	struct qs_cls_s pat_q[QBF_PAT_Q_NUB];
	struct dim_pat_s	pat_bf[DIM_PAT_NUB];

	/* qb: local 2020-10-12 */
	struct buf_que_s iat_qb;
	struct qs_cls_s iat_q[QBF_IAT_Q_NUB];
	struct dim_iat_s	iat_bf[DIM_IAT_NUB];
	unsigned int		is_tvp	:2;
	//0: unknown, 1: non tvp, 2: tvp
	unsigned int		is_secure_pre	:2;
	//0: unknown, 1: non secure, 2: secure
	unsigned int		is_secure_pst	:2;
	//0: unknown, 1: non secure, 2: secure
};

struct dim_policy_s {
	unsigned int std;
	unsigned int ch[DI_CHANNEL_NUB];
	unsigned int order_i;
	unsigned int idle; /*no use*/
	union {
		unsigned int cfg_d32;
		struct {
			unsigned int bypass_all_p	: 1,
					i_first		: 1,
					rev1		: 30;
		} cfg_b;
	};
};

struct di_meson_data {
	const char *name;
	unsigned int ic_id;
	/*struct ic_ver icver;*/
	/*struct ddemod_reg_off regoff;*/
};

struct dim_wq_s {
	char *name;
	unsigned int ch;
	unsigned int cmd;
	unsigned int cnt;
	struct workqueue_struct *wq_cma;
	struct work_struct wq_work;
};

struct di_mng_s {
	/*workqueue*/
	struct dim_wq_s		wq;

	/*use enum EDI_CMA_ST*/
	atomic_t cma_mem_state[DI_CHANNEL_NUB];
	/*1:alloc cma, 0:release cma set by mng, read by work que*/
	unsigned int cma_wqsts[DI_CHANNEL_NUB];
	unsigned int cma_wqcnt;
	unsigned int cma_flg_run;
	/*task:*/
	struct di_task		tsk;
	struct di_mtask		mtsk;

	/*channel state: use enum eDI_TOP_STATE */
	atomic_t ch_state[DI_CHANNEL_NUB];

	bool in_flg[DI_CHANNEL_NUB];
	unsigned long	   mem_start[DI_CHANNEL_NUB];
	unsigned int	   mem_size[DI_CHANNEL_NUB];

	bool sub_act_flg[DI_CHANNEL_NUB];
	/*struct	mutex	   event_mutex[DI_CHANNEL_NUB];*/
	bool init_flg[DI_CHANNEL_NUB];	/*init_flag*/
	/*bool reg_flg[DI_CHANNEL_NUB];*/	/*reg_flag*/
	unsigned int reg_flg_ch;	/*for x ch reg/unreg flg*/
	unsigned int reg_setting_ch;	/*ary 2020-10-10*/
	bool trig_unreg_l[DI_CHANNEL_NUB];
	bool hw_reg_flg;	/*for di_reg_setting/di_unreg_setting*/
	bool act_flg		;/*active_flag*/

	bool flg_hw_int;	/*only once*/
	struct dim_policy_s	policy;
	struct dim_mm_t_s mmt;
	struct di_mm_s	mm[DI_CHANNEL_NUB];
	/*new reg/unreg*/
	atomic_t trig_reg[DI_CHANNEL_NUB];
	atomic_t trig_unreg[DI_CHANNEL_NUB];
};

/*************************
 *debug register:
 *************************/
#define K_DI_SIZE_REG_LOG	(1000)
#define K_DI_LAB_MOD		(0xf001)
/*also see: dbg_mode_name*/
enum EDI_DBG_MOD {
	EDI_DBG_MOD_REGB,	/* 0 */
	EDI_DBG_MOD_REGE,	/* 1 */
	EDI_DBG_MOD_UNREGB,	/* 2 */
	EDI_DBG_MOD_UNREGE,	/* 3 */
	EDI_DBG_MOD_PRE_SETB,	/* 4 */
	EDI_DBG_MOD_PRE_SETE,	/* 5 */
	EDI_DBG_MOD_PRE_DONEB,	/* 6 */
	EDI_DBG_MOD_PRE_DONEE,	/* 7 */
	EDI_DBG_MOD_POST_SETB,	/* 8 */
	EDI_DBG_MOD_POST_SETE,	/* 9 */
	EDI_DBG_MOD_POST_IRQB,	/* a */
	EDI_DBG_MOD_POST_IRQE,	/* b */
	EDI_DBG_MOD_POST_DB,	/* c */
	EDI_DBG_MOD_POST_DE,	/* d */
	EDI_DBG_MOD_POST_CH_CHG,	/* e */
	EDI_DBG_MOD_POST_TIMEOUT,	/* F */

	EDI_DBG_MOD_RVB,	/*10 */
	EDI_DBG_MOD_RVE,	/*11 */

	EDI_DBG_MOD_POST_RESIZE, /*0x12 */
	EDI_DBG_MOD_END,

};

enum EDI_LOG_TYPE {
	EDI_LOG_TYPE_ALL = 1,
	EDI_LOG_TYPE_REG,
	EDI_LOG_TYPE_MOD,
};

struct di_dbg_reg {
	unsigned int addr;
	unsigned int val;
	unsigned int st_bit:8,
		b_w:8,
		res:16;
};

struct di_dbg_mod {
	unsigned int lable;	/*0xf001: mean dbg mode*/
	unsigned int ch:8,
		mod:8,
		res:16;
	unsigned int cnt;/*frame cnt*/
};

union udbg_data {
	struct di_dbg_reg reg;
	struct di_dbg_mod mod;
};

struct di_dbg_reg_log {
	bool en;
	bool en_reg;
	bool en_mod;
	bool en_all;
	bool en_notoverwrite;

	union udbg_data log[K_DI_SIZE_REG_LOG];
	unsigned int pos;
	unsigned int wsize;
	bool overflow;
};

enum EDBG_TIMER {
	EDBG_TIMER_REG_B,
	EDBG_TIMER_REG_E,
	EDBG_TIMER_UNREG_B,
	EDBG_TIMER_UNREG_E,
	EDBG_TIMER_FIRST_GET,
	EDBG_TIMER_FIRST_READY,
};
struct di_dbg_data {
	unsigned int vframe_type;	/*use for type info*/
	unsigned int cur_channel;
	struct di_dbg_reg_log reg_log;
};

#define DI_CVS_EN_PRE	DI_BIT0
#define DI_CVS_EN_PST	DI_BIT1
#define DI_CVS_EN_PRE2	DI_BIT2
#define DI_CVS_EN_PST2	DI_BIT3
#define DI_CVS_EN_INP	DI_BIT4
#define DI_CVS_EN_DS	DI_BIT5

struct di_cvs_s {
	unsigned int post_idx[2][6];
	unsigned int pre_idx[2][10];
	unsigned int inp_idx[3];
	unsigned int nr_ds_idx;
	unsigned int pre_num;
	unsigned int post_num;
	unsigned int en;/*bit0:pre,bit1,post; bit2,pre_2*/
	unsigned int err_cnt;
};

struct di_data_l_s {
	/*bool cfg_en[K_DI_CFG_NUB];*/	/*cfg_top*/
	union di_cfg_tdata_u cfg_en[K_DI_CFG_NUB];
	unsigned int cfg_sel;
	unsigned int cfg_dbg_mode; /*val or item*/
	int mp_uit[K_DI_MP_UIT_NUB];	/*EDI_MP_UI_T*/
	struct di_ch_s ch_data[DI_CHANNEL_NUB];
	int plane[DI_CHANNEL_NUB];	/*use for debugfs*/

	struct di_dbg_data dbg_data;
	struct di_mng_s mng;
	struct di_hpre_s hw_pre;
	struct di_hpst_s hw_pst;
	const struct dim_hw_opsv_s *hop_l1; /* from sc2 */
	struct afd_s di_afd;
	const struct hw_ops_s *hop_l2;

	const struct afd_ops_s *afds;
	struct di_cvs_s cvs;
	struct dentry *dbg_root_top;	/* dbg_fs*/
	/*pq_ops*/
	const struct pulldown_op_s *ops_pd;	/* pulldown */
	const struct detect3d_op_s *ops_3d;	/* detect_3d */
	const struct nr_op_s *ops_nr;	/* nr */
	const struct mtn_op_s *ops_mtn;	/* deinterlace_mtn */
	const struct ext_ops_s	*ops_ext;
	/*di ops for other module */
	/*struct di_ext_ops *di_api; */
	const struct di_meson_data *mdata;
};

/**************************************
 *
 * DEBUG infor
 *
 *************************************/

#define DBG_M_C_ALL		DI_BIT30	/*all debug close*/
#define DBG_M_O_ALL		DI_BIT31	/*all debug open*/

#define DBG_M_DT		DI_BIT0	/*do table work*/
#define DBG_M_REG		DI_BIT1	/*reg/unreg*/
#define DBG_M_POST_REF		DI_BIT2
#define DBG_M_TSK		DI_BIT3
#define DBG_M_INIT		DI_BIT4
#define DBG_M_EVENT		DI_BIT5
#define DBG_M_FIRSTFRAME	DI_BIT6
#define DBG_M_DBG		DI_BIT7

#define DBG_M_POLLING		DI_BIT8
#define DBG_M_ONCE		DI_BIT9
#define DBG_M_KEEP		DI_BIT10	/*keep buf*/
#define DBG_M_MEM		DI_BIT11	/*mem alloc release*/
#define DBG_M_MEM2		DI_BIT12	/* 2nd task mem*/
#define DBG_M_WQ		DI_BIT14	/*work que*/
#define DBG_M_PL		DI_BIT15
#define DBG_M_AFBC		DI_BIT16
#define DBG_M_COPY		DI_BIT17

#define DBG_M_RESET_PRE		DI_BIT29
extern unsigned int di_dbg;

#define dbg_m(mark, fmt, args ...)		\
	do {					\
		if (di_dbg & DBG_M_C_ALL)	\
			break;			\
		if ((di_dbg & DBG_M_O_ALL) ||	\
		    (di_dbg & (mark))) {		\
			pr_info("dim:" fmt, ##args); \
		}				\
	} while (0)

#define PR_ERR(fmt, args ...)		pr_err("dim:err:" fmt, ##args)
#define PR_WARN(fmt, args ...)		pr_err("dim:warn:" fmt, ##args)
#define PR_INF(fmt, args ...)		pr_info("dim:" fmt, ##args)

#define dbg_dt(fmt, args ...)		dbg_m(DBG_M_DT, fmt, ##args)
#define dbg_reg(fmt, args ...)		dbg_m(DBG_M_REG, fmt, ##args)
#define dbg_post_ref(fmt, args ...)	dbg_m(DBG_M_POST_REF, fmt, ##args)
#define dbg_poll(fmt, args ...)		dbg_m(DBG_M_POLLING, fmt, ##args)
#define dbg_tsk(fmt, args ...)		dbg_m(DBG_M_TSK, fmt, ##args)

#define dbg_init(fmt, args ...)		dbg_m(DBG_M_INIT, fmt, ##args)
#define dbg_ev(fmt, args ...)		dbg_m(DBG_M_EVENT, fmt, ##args)
#define dbg_first_frame(fmt, args ...)	dbg_m(DBG_M_FIRSTFRAME, fmt, ##args)
#define dbg_dbg(fmt, args ...)		dbg_m(DBG_M_DBG, fmt, ##args)
#define dbg_once(fmt, args ...)		dbg_m(DBG_M_ONCE, fmt, ##args)
#define dbg_mem(fmt, args ...)		dbg_m(DBG_M_MEM, fmt, ##args)
#define dbg_keep(fmt, args ...)		dbg_m(DBG_M_KEEP, fmt, ##args)
#define dbg_wq(fmt, args ...)		dbg_m(DBG_M_WQ, fmt, ##args)
#define dbg_pl(fmt, args ...)		dbg_m(DBG_M_PL, fmt, ##args)
#define dbg_mem2(fmt, args ...)		dbg_m(DBG_M_MEM2, fmt, ##args)
#define dbg_afbc(fmt, args ...)		dbg_m(DBG_M_AFBC, fmt, ##args)
#define dbg_copy(fmt, args ...)		dbg_m(DBG_M_COPY, fmt, ##args)

char *di_cfgx_get_name(enum EDI_CFGX_IDX idx);
bool di_cfgx_get(unsigned int ch, enum EDI_CFGX_IDX idx);
void di_cfgx_set(unsigned int ch, enum EDI_CFGX_IDX idx, bool en);

static inline struct di_data_l_s *get_datal(void)
{
	return (struct di_data_l_s *)get_dim_de_devp()->data_l;
}

static inline struct di_ch_s *get_chdata(unsigned int ch)
{
	return &get_datal()->ch_data[ch];
}

static inline struct di_mng_s *get_bufmng(void)
{
	return &get_datal()->mng;
}

static inline struct di_hpre_s  *get_hw_pre(void)
{
	return &get_datal()->hw_pre;
}

static inline struct di_hpst_s  *get_hw_pst(void)
{
	return &get_datal()->hw_pst;
}

static inline const struct dim_hw_opsv_s  *opl1(void)
{
	return get_datal()->hop_l1;
}

static inline const struct hw_ops_s  *opl2(void)
{
	return get_datal()->hop_l2;
}

static inline struct di_dat_s *get_pst_afbct(struct di_ch_s *pch)
{
	return &pch->rse_ori.dat_p_afbct;
}

static inline struct di_dat_s *get_idat(struct di_ch_s *pch)
{
	return &pch->rse_ori.dat_i;
}

/****************************************
 * flg_hw_int
 *	for hw set once
 ****************************************/
static inline bool di_get_flg_hw_int(void)
{
	return get_datal()->mng.flg_hw_int;
}

static inline void di_set_flg_hw_int(bool on)
{
	get_datal()->mng.flg_hw_int = on;
}

/**********************
 *
 *	reg log:
 *********************/
static inline struct di_dbg_reg_log *get_dbg_reg_log(void)
{
	return &get_datal()->dbg_data.reg_log;
}

static inline struct di_ores_s *get_orsc(unsigned int ch)
{
	return &get_datal()->ch_data[ch].rse_ori;
}

static inline struct vframe_s **get_vframe_in(unsigned int ch)
{
	return &get_orsc(ch)->vframe_in[0];
}

static inline struct vframe_s *get_vframe_in_dup(unsigned int ch)
{
	return &get_orsc(ch)->vframe_in_dup[0];
}

static inline struct vframe_s *get_vframe_local(unsigned int ch)
{
	return &get_orsc(ch)->vframe_local[0];
}

static inline struct vframe_s *get_vframe_post(unsigned int ch)
{
	return &get_orsc(ch)->vframe_post[0];
}

static inline struct di_buf_s *get_buf_local(unsigned int ch)
{
	return &get_orsc(ch)->di_buf_local[0];
}

static inline struct di_buf_s *get_buf_in(unsigned int ch)
{
	return &get_orsc(ch)->di_buf_in[0];
}

static inline struct di_buf_s *get_buf_post(unsigned int ch)
{
	return &get_orsc(ch)->di_buf_post[0];
}

static inline struct queue_s *get_queue(unsigned int ch)
{
	return &get_orsc(ch)->queue[0];
}

static inline struct di_buf_pool_s *get_buf_pool(unsigned int ch)
{
	return &get_orsc(ch)->di_buf_pool[0];
}

static inline struct di_pre_stru_s *get_pre_stru(unsigned int ch)
{
	return &get_orsc(ch)->di_pre_stru;
}

static inline struct di_post_stru_s *get_post_stru(unsigned int ch)
{
	return &get_orsc(ch)->di_post_stru;
}

static inline enum EDI_SUB_ID get_current_channel(void)
{
	return get_datal()->dbg_data.cur_channel;
}

static inline void set_current_channel(unsigned int channel)
{
	get_datal()->dbg_data.cur_channel = channel;
}

static inline bool get_init_flag(unsigned char ch)
{
	return get_bufmng()->init_flg[ch];
}

static inline struct dim_policy_s *get_dpolicy(void)
{
	return &get_bufmng()->policy;
}

static inline void set_init_flag(unsigned char ch, bool on)
{
	get_bufmng()->init_flg[ch] =  on;
}

extern const unsigned int di_ch2mask_table[DI_CHANNEL_MAX];
/******************************************
 *
 *	reg / unreg
 *
 *****************************************/
static inline bool get_reg_flag(unsigned char ch)
{
	unsigned int flg = get_bufmng()->reg_flg_ch;
	bool ret = false;

	if (di_ch2mask_table[ch] & flg)
		ret = true;

	/*dim_print("%s:%d\n", __func__, ret);*/
	return ret;
}

static inline unsigned int get_reg_flag_all(void)
{
	return get_bufmng()->reg_flg_ch;
}

static inline void set_reg_flag(unsigned char ch, bool on)
{
	unsigned int flg = get_bufmng()->reg_flg_ch;

	if (on)
		get_bufmng()->reg_flg_ch = flg | di_ch2mask_table[ch];
	else
		get_bufmng()->reg_flg_ch = flg & (~di_ch2mask_table[ch]);
	/*dim_print("%s:%d\n", __func__, get_bufmng()->reg_flg_ch);*/
}

static inline unsigned int get_reg_setting_all(void)
{
	return get_bufmng()->reg_setting_ch;
}

static inline void set_reg_setting(unsigned char ch, bool on)
{
	unsigned int flg = get_bufmng()->reg_setting_ch;

	if (on)
		get_bufmng()->reg_setting_ch = flg | di_ch2mask_table[ch];
	else
		get_bufmng()->reg_setting_ch = flg & (~di_ch2mask_table[ch]);
	/*dim_print("%s:%d\n", __func__, get_bufmng()->reg_flg_ch);*/
}

/******************************************
 *
 *	trig unreg:
 *		when unreg: set 1
 *		when reg: set 0
 *****************************************/

static inline bool get_flag_trig_unreg(unsigned char ch)
{
	return get_bufmng()->trig_unreg_l[ch];
}

#ifdef MARK_HIS
static inline unsigned int get_reg_flag_all(void)
{
	return get_bufmng()->reg_flg_ch;
}
#endif

static inline void set_flag_trig_unreg(unsigned char ch, bool on)
{
	get_bufmng()->trig_unreg_l[ch] =  on;
}

static inline bool get_hw_reg_flg(void)
{
	return get_bufmng()->hw_reg_flg;
}

static inline void set_hw_reg_flg(bool on)
{
	get_bufmng()->hw_reg_flg =  on;
}

static inline bool get_or_act_flag(void)
{
	return get_bufmng()->act_flg;
}

static inline void set_or_act_flag(bool on)
{
	get_bufmng()->act_flg =  on;
}

static inline unsigned int get_flag_tvp(unsigned char ch)
{
	return get_datal()->ch_data[ch].is_tvp;
}

static inline void set_flag_tvp(unsigned char ch, unsigned int data)
{
	get_datal()->ch_data[ch].is_tvp = data;
}

static inline unsigned int get_flag_secure_pre(unsigned char ch)
{
	return get_datal()->ch_data[ch].is_secure_pre;
}

static inline void set_flag_secure_pre(unsigned char ch, unsigned int data)
{
	get_datal()->ch_data[ch].is_secure_pre = data;
}

static inline unsigned int get_flag_secure_pst(unsigned char ch)
{
	return get_datal()->ch_data[ch].is_secure_pst;
}

static inline void set_flag_secure_pst(unsigned char ch, unsigned int data)
{
	get_datal()->ch_data[ch].is_secure_pst = data;
}

/*sum*/
static inline void di_sum_set_l(unsigned int ch, enum EDI_SUM id,
				unsigned int val)
{
	get_chdata(ch)->sum[id] = val;
}

static inline unsigned int di_sum_inc_l(unsigned int ch, enum EDI_SUM id)
{
	get_chdata(ch)->sum[id]++;
	return get_chdata(ch)->sum[id];
}

static inline unsigned int di_sum_get_l(unsigned int ch, enum EDI_SUM id)
{
	return get_chdata(ch)->sum[id];
}

/*sum get and put*/
static inline unsigned int get_sum_g(unsigned int ch)
{
	return get_datal()->ch_data[ch].sum_get;
}

static inline void sum_g_inc(unsigned int ch)
{
	get_datal()->ch_data[ch].sum_get++;
}

static inline void sum_g_clear(unsigned int ch)
{
	get_datal()->ch_data[ch].sum_get = 0;
}

static inline unsigned int get_sum_p(unsigned int ch)
{
	return get_datal()->ch_data[ch].sum_put;
}

static inline void sum_p_inc(unsigned int ch)
{
	get_datal()->ch_data[ch].sum_put++;
}

static inline void sum_p_clear(unsigned int ch)
{
	get_datal()->ch_data[ch].sum_put = 0;
}

/*********************/
static inline unsigned int get_sum_pst_g(unsigned int ch)
{
	return get_datal()->ch_data[ch].sum_pst_get;
}

static inline void sum_pst_g_inc(unsigned int ch)
{
	get_datal()->ch_data[ch].sum_pst_get++;
}

static inline void sum_pst_g_clear(unsigned int ch)
{
	get_datal()->ch_data[ch].sum_pst_get = 0;
}

static inline unsigned int get_sum_pst_p(unsigned int ch)
{
	return get_datal()->ch_data[ch].sum_pst_put;
}

static inline void sum_pst_p_inc(unsigned int ch)
{
	get_datal()->ch_data[ch].sum_pst_put++;
}

static inline void sum_pst_p_clear(unsigned int ch)
{
	get_datal()->ch_data[ch].sum_pst_put = 0;
}

/*********************/

static inline struct dim_sum_s *get_sumx(unsigned int ch)
{
	return &get_datal()->ch_data[ch].sumx;
}

/*bypass_state*/

static inline bool di_bypass_state_get(unsigned int ch)
{
	return get_chdata(ch)->bypass_state;
}

static inline void di_bypass_state_set(unsigned int ch, bool on)
{
	get_chdata(ch)->bypass_state =  on;
}

#ifdef MARK_HIS
static inline struct semaphore *get_sema(void)
{
	return &get_dim_de_devp()->sema;
}
#endif

static inline struct di_task *get_task(void)
{
	return &get_bufmng()->tsk;
}

static inline struct di_mtask *get_mtask(void)
{
	return &get_bufmng()->mtsk;
}

/******************************************
 *	pq ops
 *****************************************/

static inline const struct pulldown_op_s *get_ops_pd(void)
{
	return get_datal()->ops_pd;
}

static inline const struct detect3d_op_s *get_ops_3d(void)
{
	return get_datal()->ops_3d;
}

static inline const struct nr_op_s *get_ops_nr(void)
{
	return get_datal()->ops_nr;
}

static inline const struct mtn_op_s *get_ops_mtn(void)
{
	return get_datal()->ops_mtn;
}

static inline const struct ext_ops_s *ops_ext(void)
{
	return get_datal()->ops_ext;
}

#ifdef MARK_HIS
static inline struct di_ext_ops *get_ops_api(void)
{
	return get_datal()->di_api;
}
#endif

/******************************************
 *	module para for di
 *****************************************/

static inline int dimp_get(enum EDI_MP_UI_T idx)
{
	return get_datal()->mp_uit[idx];
}

static inline void dimp_set(enum EDI_MP_UI_T idx, int val)
{
	get_datal()->mp_uit[idx] = val;
}

static inline int dimp_inc(enum EDI_MP_UI_T idx)
{
	get_datal()->mp_uit[idx]++;
	return get_datal()->mp_uit[idx];
}

static inline int dimp_dec(enum EDI_MP_UI_T idx)
{
	get_datal()->mp_uit[idx]--;
	return get_datal()->mp_uit[idx];
}

#define di_mpr(x) dimp_get(edi_mp_##x)

/******************************************
 *	mm
 *****************************************/
static inline struct di_mm_s *dim_mm_get(unsigned int ch)
{
	return &get_datal()->mng.mm[ch];
}

static inline struct dim_mm_t_s *dim_mmt_get(void)
{
	return &get_datal()->mng.mmt;
}

static inline unsigned long di_get_mem_start(unsigned int ch)
{
	return get_datal()->mng.mm[ch].sts.mem_start;
}

static inline void di_set_mem_info(unsigned int ch,
				   unsigned long mstart,
				   unsigned int size)
{
	get_datal()->mng.mm[ch].sts.mem_start = mstart;
	get_datal()->mng.mm[ch].sts.mem_size = size;
}

static inline unsigned int di_get_mem_size(unsigned int ch)
{
	return get_datal()->mng.mm[ch].sts.mem_size;
}

void di_tout_int(struct di_time_out_s *tout, unsigned int thd);
bool di_tout_contr(enum EDI_TOUT_CONTR cmd, struct di_time_out_s *tout);

#ifdef MARK_SC2
/*cpu_after_eq*/
static inline bool is_ic_after_eq(unsigned int ic_id)
{
	if (get_datal()->mdata->ic_id >= ic_id)
		return true;
	return false;
}

static inline bool is_ic_before(unsigned int ic_id)
{
	if (get_datal()->mdata->ic_id < ic_id)
		return true;
	return false;
}
#endif
static inline bool is_ic_between(unsigned int ic_min, unsigned int ic_max)
{
	unsigned int id = get_datal()->mdata->ic_id;

	if ((id >= ic_min) && (id <= ic_max))
		return true;
	return false;
}

#define DIM_IS_IC(cc)		is_ic_named((get_datal()->mdata->ic_id), \
					DI_IC_ID_##cc)
#define DIM_IS_IC_EF(cc)	is_ic_after_eq((get_datal()->mdata->ic_id), \
					DI_IC_ID_##cc)
#define DIM_IS_IC_BF(cc)	is_ic_before((get_datal()->mdata->ic_id), \
					DI_IC_ID_##cc)
#define DIM_IS_IC_BT(cc1, cc2)	is_ic_between(DI_IC_ID_##cc1, DI_IC_ID_##cc2)
#endif	/*__DI_DATA_L_H__*/
