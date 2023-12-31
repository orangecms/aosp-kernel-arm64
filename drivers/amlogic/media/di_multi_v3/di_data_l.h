/*
 * drivers/amlogic/media/di_multi_v3/di_data_l.h
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

#define DI_CHANNEL_NUB	(2)
#define DI_CHANNEL_MAX  (4)
#define DIM_K_VFM_NUM	(10)
#define DIM_K_VFM_IN_LIMIT		(2)
#define DIM_K_BUF_IN_LIMIT		(8)
#define DIM_K_BUF_OUT_LIMIT		(8)

#define DIM_K_VFM_LIMIT_PRE_READY	(3)

/*ei mode for first frame*/
#define DIM_EI_CNT	(3)

#define DIM_K_CODE_DVFM	(0x12345678)
#define DIM_K_CODE_OLD	(0x87654321)
#define DIM_K_CODE_MASK (0xffaa5500)
#define DIM_K_CODE_LOCAL	(DIM_K_CODE_MASK|02)
#define DIM_K_CODE_POST		(DIM_K_CODE_MASK|03)
#define DIM_K_CODE_INS_POST		(DIM_K_CODE_MASK|04)

#define TABLE_FLG_END	(0xfffffffe)
#define TABLE_LEN_MAX	(1000)
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

/*****************************************
 * cma mode
 * this is keep order with dts define
 *****************************************/
enum eDI_MEM_M {
	eDI_MEM_M_rev = 0,
	eDI_MEM_M_cma = 1,
	eDI_MEM_M_cma_all = 2,
	eDI_MEM_M_codec_a = 3,
	eDI_MEM_M_codec_b = 4,
	eDI_MEM_M_max	/**/
};
#if 0
enum EDPST_OUT_MODE {
	EDPST_OUT_MODE_DEF,
	EDPST_OUT_MODE_NV21,
	EDPST_OUT_MODE_NV12,
};
#endif

enum EDPST_MODE {
	EDPST_MODE_NV21_8BIT, /*NV21 NV12*/
	EDPST_MODE_422_10BIT_PACK,
	EDPST_MODE_422_10BIT,
	EDPST_MODE_422_8BIT,
};

struct di_win_s {
	unsigned int x_size;
	unsigned int y_size;
	unsigned int x_st;
	unsigned int y_st;
};

/* ************************************** */
/* *************** cfg top ************** */
/* ************************************** */
/* also see: di_cfg_top_ctr*/
enum eDI_CFG_TOP_IDX {
	/* cfg for top */
	EDI_CFG_BEGIN,
	EDI_CFG_CH_NUB,
	EDI_CFG_mem_flg,
	EDI_CFG_first_bypass,
	EDI_CFG_ref_2,
	EDI_CFG_PMODE,
	/* EDI_CFG_PMODE
	 * 0:as p;
	 * 1:as i;
	 * 2:use 2 i buf
	 */
	EDI_CFG_KEEP_CLEAR_AUTO,
	EDI_CFG_PRE_RST_OLD_FLOW,
	EDI_CFG_TMODE_1,	/*EDIM_TMODE_1_PW_VFM*/
	EDI_CFG_TMODE_2,	/*EDIM_TMODE_2_PW_OUT*/
	EDI_CFG_TMODE_3,	/*EDIM_TMODE_3_PW_LOCAL*/
	EDI_CFG_DBG,
	EDI_CFG_END,

};

#define cfgeq(a, b) ((div3_cfg_top_get(EDI_CFG_##a) == (b)) ? true : false)
#define cfgnq(a, b) ((div3_cfg_top_get(EDI_CFG_##a) != (b)) ? true : false)
#define cfgg(a) div3_cfg_top_get(EDI_CFG_##a)
#define cfgs(a, b) div3_cfg_top_set(EDI_CFG_##a, b)

#define K_DI_CFG_NUB	(EDI_CFG_END - EDI_CFG_BEGIN + 1)
/* top cfg table struct */
#define K_DI_CFG_T_FLG_NOTHING	(0x00)
#define K_DI_CFG_T_FLG_DTS	(0x01)
#define K_DI_CFG_T_FLG_NONE	(0x80)
/*used for variable*/
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

/*used for init table*/
struct di_cfg_ctr_s {
	char *dts_name;

	enum eDI_CFG_TOP_IDX id;
	unsigned char	default_val;
	unsigned char	flg;
};

/* ************************************** */
/* *************** cfg x *************** */
/* ************************************** */
/*also see di_cfgx_ctr*/
enum eDI_CFGX_IDX {
	/* cfg channel x*/
	eDI_CFGX_BEGIN,
	eDI_CFGX_BYPASS_ALL,	/*bypass_all*/
	EDI_CFGX_HOLD_VIDEO,	/*hold_video*/
	eDI_CFGX_END,

	/* debug cfg x */
	eDI_DBG_CFGX_BEGIN,
	eDI_DBG_CFGX_IDX_VFM_IN,
	eDI_DBG_CFGX_IDX_VFM_OT,
	eDI_DBG_CFGX_END,
};

#define K_DI_CFGX_NUB	(eDI_DBG_CFGX_END - eDI_CFGX_BEGIN + 1)

struct di_cfgx_ctr_s {
	char *name;
	enum eDI_CFGX_IDX id;
	bool	default_val;
};

/* ****************************** */
enum eDI_SUB_ID {
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

#ifdef DIM_DEBUG_QUE_ERR
union dim_dbg_que_u {
	unsigned int u32;
	struct {
	unsigned int	buf_type	:2,
			buf_index	:6,
			que_index	:4,
			que_api		:4,
			omx_index	:8,
			cnt		:8;/*hight*/
	} b;
};

enum EDIM_QUE_API {
	DIM_QUE_IN	= 1,
	DIM_QUE_OUT,
	DIM_QUE_OUT_NOT_FIFO,
	DIM_QUE_PEEK,
	DIM_O_IN,
	DIM_O_OUT,
	DIM_O_PEEK,
};

#define DIM_DBG_QUE_SIZE	25

struct di_dbg_que_s {
	union dim_dbg_que_u que_inf[DIM_DBG_QUE_SIZE];
	unsigned int pos;
	unsigned int en;
	unsigned int cnt;
};

#define DIM_DBG_MARK	(0x55550000)
#define DIM_PST_A	(DIM_DBG_MARK | 0x0000c000)

#endif

struct di_dbg_datax_s {
	struct vframe_s vfm_input;	/*debug input vframe*/
	struct vframe_s *pfm_out;	/*debug di_get vframe*/
#ifdef DIM_DEBUG_QUE_ERR
	struct di_dbg_que_s que;
#endif
};

/*debug function*/
enum eDI_DBG_F {
	eDI_DBG_F_00,
	eDI_DBG_F_01,
	eDI_DBG_F_02,
	eDI_DBG_F_03,
	eDI_DBG_F_04,
	eDI_DBG_F_05,
	eDI_DBG_F_06,
	eDI_DBG_F_07,
	eDI_DBG_F_08,
	eDI_DBG_F_09,
};

struct di_dbg_func_s {
	enum eDI_DBG_F index;
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

enum eDI_TOUT_CONTR {
/*	eDI_TOUT_CONTR_INT,*/
	eDI_TOUT_CONTR_EN,
	eDI_TOUT_CONTR_FINISH,
	eDI_TOUT_CONTR_CHECK,

	eDI_TOUT_CONTR_CLEAR,
	eDI_TOUT_CONTR_RESET,
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

enum eDO_TABLE_CMD {
	eDO_TABLE_CMD_NONE,
	eDO_TABLE_CMD_STOP,
	eDO_TABLE_CMD_START,
	eDO_TABLE_CMD_PAUSE,
	eDO_TABLE_CMD_STEP,
	eDO_TABLE_CMD_STEP_BACK,
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
#if 0 /*move to deinterlace.h*/

/*2019-10-14*/
struct di_in_inf_s {
	/*use this for judge type change or not */
	unsigned int ch;
	unsigned int vtype_ori;	/*only debug*/
	unsigned int src_type;
	unsigned int trans_fmt;
	unsigned int sig_fmt;/*use for mtn*/
	unsigned int h;
	unsigned int w;
};
struct dim_wmode_s;

struct dim_wmode_s {
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
	 *prog_proc_config:
	 *1: process p from decoder as field
	 *0: process p from decoder as frame
	 ***************************************/
		is_invert_tp		:1,
		p_as_i			:1,
		is_angle		:1,
		reserved		:19;
	unsigned int vtype;
	unsigned int h;
	unsigned int w;
	unsigned int seq;
};

//struct dim_vmode_s;

enum EDIM_DISP_TYPE {
	EDIM_DISP_T_NONE,
	EDIM_DISP_T_IN,		/*need bypass*/
	EDIM_DISP_T_PRE,
	EDIM_DISP_T_NR,
	EDIM_DISP_T_PST,
};

struct dim_dvfm_s {
//	unsigned int		code_name;/*0x12345678*/
	unsigned int		index;

	enum EDIM_DISP_TYPE	etype;
	struct di_in_inf_s	in_inf;
	struct dim_wmode_s	wmode;
	struct dim_vmode_s	vmode;
	struct vframe_s		vframe;/*input cp*/
	void	*vfm_in;
	void	*di_buf;/*vfm out is in di_buf*/
};
#endif


/**************************************/
/* PRE */
/**************************************/
enum eDI_PRE_ST {
	eDI_PRE_ST_EXIT,
	eDI_PRE_ST_IDLE,	/*swith to next channel?*/
	eDI_PRE_ST_CHECK,
	eDI_PRE_ST_SET,
	eDI_PRE_ST_WAIT_INT,
	eDI_PRE_ST_TIMEOUT,
};

enum eDI_PRE_ST4 {	/*use this for co work with do table*/
	eDI_PRE_ST4_EXIT,
	eDI_PRE_ST4_IDLE,	/*swith to next channel?*/
	eDI_PRE_ST4_CHECK,	/*check mode do_table and set*/
	eDI_PRE_ST4_DO_TABLE,	/* do table statue;*/
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
struct di_hpre_s;

struct dim_hpre_ops_s {
	struct di_pre_stru_s *pres;	/*tmp for ch*/
/**************************************
 *prob_hw: not depend channel info
 **************************************/
	void (*prob_hw)(struct di_hpre_s *hpre);
	void (*reg_var)(struct di_hpre_s *hpre);
	void (*reg_hw)(struct di_hpre_s *hpre);
	void (*remove_hw)(struct di_hpre_s *hpre);
	void (*unreg_var)(struct di_hpre_s *hpre);
	void (*unreg_hw)(struct di_hpre_s *hpre);
};

struct di_hpre_s {
	enum eDI_PRE_ST4 pre_st;
	unsigned int curr_ch;
	/*set when have vframe in; clear when int have get*/
	bool hw_flg_busy_pre;
/*	bool trig_unreg;*/	/*add for unreg flow;*/
/*	enum eDI_SUB_ID hw_owner_pre;*/
	bool flg_wait_int;
	struct di_pre_stru_s *pres;
	struct di_post_stru_s *psts;
	struct di_time_out_s tout;	/*for time out*/
	bool flg_int_done;
	unsigned int check_recycle_buf_cnt;

	struct di_pre_set_s set_lst;
	struct di_pre_set_s set_curr;

	struct di_in_inf_s vinf_lst;
	struct di_in_inf_s vinf_curr;

	/* use do table to swith mode*/
	struct do_table_s sdt_mode;

	unsigned int idle_cnt;	/*use this avoid repeat idle <->check*/
	/*dbg flow:*/
	bool dbg_f_en;
	unsigned int dbg_f_lstate;
	unsigned int dbg_f_cnt;

	/*canvans*/
	unsigned int di_inp_idx[3];
	struct dim_hpre_ops_s ops;
};

/**************************************/
/* POST */
/**************************************/
enum eDI_PST_ST {
	eDI_PST_ST_EXIT,
	eDI_PST_ST_IDLE,	/*swith to next channel?*/
	eDI_PST_ST_CHECK,
	eDI_PST_ST_SET,
	eDI_PST_ST_WAIT_INT,
	eDI_PST_ST_TIMEOUT,
	eDI_PST_ST_DONE,	/*use for bypass_all*/
};

struct di_hpst_s {
	enum eDI_PST_ST state;
	unsigned int curr_ch;
	/*set when have vframe in; clear when int have get*/
	bool hw_flg_busy_post;
	struct di_pre_stru_s *pres;
	struct di_post_stru_s *psts;
	struct di_time_out_s tout;	/*for time out*/
	bool flg_int_done;

	/*dbg flow:*/
	bool dbg_f_en;
	unsigned int dbg_f_lstate;
	unsigned int dbg_f_cnt;
	struct vframe_s		vf_post;

};

/**************************************/
/* channel status */
/**************************************/
enum EDI_TOP_STATE {
	eDI_TOP_STATE_NOPROB,
	EDI_TOP_STATE_IDLE,	/*idle not work*/
	/* STEP1
	 * till peek vframe and set irq;before this state, event reg finish
	 */
	eDI_TOP_STATE_REG_STEP1,
	eDI_TOP_STATE_REG_STEP1_P1,	/*2019-05-21*/
	eDI_TOP_STATE_REG_STEP2,	/*till alloc and ready*/
	EDI_TOP_STATE_READY,		/*can do DI*/
	eDI_TOP_STATE_BYPASS,		/*complet bypass*/
	eDI_TOP_STATE_UNREG_STEP1,	/*till pre/post is finish;*/
	/* do unreg and to IDLE.
	 * no need to wait cma release after  this unreg event finish
	 */
	eDI_TOP_STATE_UNREG_STEP2,

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
#if 1	/*not use cmd*/

	/*local event*/
	struct kfifo	fifo_cmd;
	spinlock_t     lock_cmd;
	bool flg_cmd;
	unsigned int err_cmd_cnt;
#endif
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

enum eCMD_LOCAL {
	eCMD_NONE,
	eCMD_REG,
	eCMD_UNREG,
	eCMD_READY,
	eCMD_CHG,
	ECMD_RL_KEEP,
	ECMD_RL_KEEP_ALL,
	NR_FINISH,
};

/**************************************
 *QUE
 *keep same order with di_name_new_que
 **************************************/
enum QUE_TYPE {	/*mast start from 0 */
	QUE_IN_FREE,	/*5*/
	QUE_PRE_READY,	/*6*/
	QUE_POST_FREE,	/*7*/
	QUE_POST_READY,	/*8*/
	QUE_POST_BACK,		/*new*/
	QUE_POST_DOING,
	QUE_POST_NOBUF,
	QUE_POST_KEEP,	/*below use pw_queue_in*/
	QUE_POST_KEEP_BACK,
	/*----------------*/
	QUE_DBG,
	QUE_NUB,
};

/**************************************
 *QUED
 *keep same order with qued_name
 **************************************/
enum QUED_TYPE {
	/*fifo:*/
	QUED_T_FREE,
	QUED_T_IN,
	QUED_T_PRE,
	QUED_T_PRE_READY,
	QUED_T_PST,
	QUED_T_PST_READY,
	QUED_T_BACK,
	QUED_T_RECYCL,
	QUED_T_IS_IN,	/*input for instance mode*/
	QUED_T_IS_FREE,
	QUED_T_IS_PST_FREE, /*use for post buf out*/
	QUED_T_IS_PST_DOBEF,/*before post doing*/
	//QUED_T_IS_PST_NOBUF,
	QUED_T_PST_DOING,/*?*/
	QUED_T_DBG,
	/*bit map*/
	QUED_T_SPLIT, /*not use, only split*/
	QUED_T_DIS,
	QUED_T_NUB,
};

enum QUED_KIND {
	QUED_K_FIFO,
	QUED_K_N, /*bit map*/
};

#define DIM_MAX_QUE_N		(32)

struct que_n_s {
	int		nub;
	struct mutex	mtex;
	unsigned int	marsk;
};

struct que_f_s {
	bool		flg;	/*1: have reg*/
	struct kfifo	fifo;
};

struct dim_que_s {
	enum QUED_TYPE	index;
	enum QUED_KIND	type;
	const char *name;
	union {
		struct que_n_s n;
		struct que_f_s f;
	};
};

/*#define QUE_NUB  (5)*/
enum eDI_BUF_TYPE {
	eDI_BUF_T_IN = 1,	/*VFRAME_TYPE_IN*/
	eDI_BUF_T_LOCAL,	/*VFRAME_TYPE_LOCAL*/
	eDI_BUF_T_POST,		/*VFRAME_TYPE_POST*/
};

#define MAX_FIFO_SIZE	(32)

/**************************************
 *
 * summmary variable
 * also see:di_sum_name_tab
 **************************************/

enum eDI_SUM {
	eDI_SUM_O_PEEK_CNT,	/*video_peek_cnt*/
	eDI_SUM_REG_UNREG_CNT,	/*di_reg_unreg_cnt*/
	eDI_SUM_NUB,
};

struct di_sum_s {
	char *name;
	enum eDI_SUM	index;
	unsigned int	default_val;
};

/**************************************
 *
 * module para
 *	int
 *	eDI_MP_SUB_DI_B
 *	eDI_MP_SUB_NR_B
 *	eDI_MP_SUB_PD_B
 *	eDI_MP_SUB_MTN_B
 *	eDI_MP_SUB_3D_B
 **************************************/
enum eDI_MP_UI_T {
	/*keep same order with di_mp_ui_top*/
	eDI_MP_UI_T_BEGIN,
	/**************************************/
	eDI_MP_SUB_DI_B,

	eDI_MP_force_prog,	/*force_prog bool*/
	edi_mp_combing_fix_en,	/*combing_fix_en bool*/
	eDI_MP_cur_lev,		/*cur_lev*/
	eDI_MP_pps_dstw,	/*pps_dstw*/
	eDI_MP_pps_dsth,	/*pps_dsth*/
	eDI_MP_pps_en,		/*pps_en*/
	eDI_MP_pps_position,	/*pps_position*/
	eDI_MP_pre_enable_mask,	/*pre_enable_mask*/
	eDI_MP_post_refresh,	/*post_refresh*/
	eDI_MP_nrds_en,		/*nrds_en*/
	eDI_MP_bypass_3d,	/*bypass_3d*/
	eDI_MP_bypass_trick_mode,	/*bypass_trick_mode*/
	eDI_MP_invert_top_bot,	/*invert_top_bot */
	eDI_MP_skip_top_bot,
	eDI_MP_force_width,
	eDI_MP_force_height,
	eDI_MP_prog_proc_config,
	eDI_MP_start_frame_drop_count,
	eDI_MP_same_field_top_count,	/*long?*/
	eDI_MP_same_field_bot_count,	/*long?*/
	eDI_MP_vpp_3d_mode,
	eDI_MP_force_recovery_count,
	eDI_MP_pre_process_time,	/*no use?*/
	eDI_MP_bypass_post,
	eDI_MP_post_wr_en,
	eDI_MP_post_wr_support,
	eDI_MP_bypass_post_state,
	eDI_MP_use_2_interlace_buff,
	eDI_MP_debug_blend_mode,
	eDI_MP_nr10bit_support,
	eDI_MP_di_stop_reg_flag,
	eDI_MP_mcpre_en,
	eDI_MP_check_start_drop_prog,
	eDI_MP_overturn,			/*? in init*/
	eDI_MP_full_422_pack,
	eDI_MP_cma_print,
	eDI_MP_pulldown_enable,
	eDI_MP_di_force_bit_mode,
	eDI_MP_calc_mcinfo_en,
	eDI_MP_colcfd_thr,
	eDI_MP_post_blend,
	eDI_MP_post_ei,
	eDI_MP_post_cnt,
	eDI_MP_di_log_flag,
	eDI_MP_di_debug_flag,
	eDI_MP_buf_state_log_threshold,
	eDI_MP_di_vscale_skip_enable,
	eDI_MP_di_vscale_skip_count,
	eDI_MP_di_vscale_skip_count_real,
	eDI_MP_det3d_en,
	eDI_MP_post_hold_line,
	eDI_MP_post_urgent,
	eDI_MP_di_printk_flag,
	eDI_MP_force_recovery,
/*	eDI_MP_debug_blend_mode,*/
	eDI_MP_di_dbg_mask,
	eDI_MP_nr_done_check_cnt,
	eDI_MP_pre_hsc_down_en,
	eDI_MP_pre_hsc_down_width,
	eDI_MP_show_nrwr,
	/********deinterlace_hw.c*********/
	eDI_MP_pq_load_dbg,
	eDI_MP_lmv_lock_win_en,
	eDI_MP_lmv_dist,
	eDI_MP_pr_mcinfo_cnt,
	eDI_MP_offset_lmv,
	eDI_MP_post_ctrl,
	eDI_MP_if2_disable,
	eDI_MP_pre_flag,
	eDI_MP_pre_mif_gate,
	eDI_MP_pre_urgent,
	eDI_MP_pre_hold_line,
	eDI_MP_pre_ctrl,
	eDI_MP_line_num_post_frst,
	eDI_MP_line_num_pre_frst,
	eDI_MP_pd22_flg_calc_en,
	eDI_MP_mcen_mode,
	eDI_MP_mcuv_en,
	eDI_MP_mcdebug_mode,
	eDI_MP_pldn_ctrl_rflsh,
	eDI_MP_4k_test,

	eDI_MP_SUB_DI_E,
	/**************************************/
	eDI_MP_SUB_NR_B,
	EDI_MP_SUB_DELAY,
	EDI_MP_SUB_DBG_MODE,
	eDI_MP_cue_en,
	eDI_MP_invert_cue_phase,
	eDI_MP_cue_pr_cnt,
	eDI_MP_cue_glb_mot_check_en,
	eDI_MP_glb_fieldck_en,
	eDI_MP_dnr_pr,
	eDI_MP_dnr_dm_en,
	eDI_MP_SUB_NR_E,
	/**************************************/
	eDI_MP_SUB_PD_B,
	eDI_MP_flm22_ratio,
	eDI_MP_pldn_cmb0,
	eDI_MP_pldn_cmb1,
	eDI_MP_flm22_sure_num,
	eDI_MP_flm22_glbpxlnum_rat,
	eDI_MP_flag_di_weave,
	eDI_MP_flm22_glbpxl_maxrow,
	eDI_MP_flm22_glbpxl_minrow,
	eDI_MP_cmb_3point_rnum,
	eDI_MP_cmb_3point_rrat,
	/******film_fw1.c**/
	eDI_MP_pr_pd,
	eDI_MP_prt_flg,
	eDI_MP_flmxx_maybe_num,
	eDI_MP_flm32_mim_frms,
	eDI_MP_flm22_dif01a_flag,
	eDI_MP_flm22_mim_frms,
	eDI_MP_flm22_mim_smfrms,
	eDI_MP_flm32_f2fdif_min0,
	eDI_MP_flm32_f2fdif_min1,
	eDI_MP_flm32_chk1_rtn,
	eDI_MP_flm32_ck13_rtn,
	eDI_MP_flm32_chk2_rtn,
	eDI_MP_flm32_chk3_rtn,
	eDI_MP_flm32_dif02_ratio,
	eDI_MP_flm22_chk20_sml,
	eDI_MP_flm22_chk21_sml,
	eDI_MP_flm22_chk21_sm2,
	eDI_MP_flm22_lavg_sft,
	eDI_MP_flm22_lavg_lg,
	eDI_MP_flm22_stl_sft,
	eDI_MP_flm22_chk5_avg,
	eDI_MP_flm22_chk6_max,
	eDI_MP_flm22_anti_chk1,
	eDI_MP_flm22_anti_chk3,
	eDI_MP_flm22_anti_chk4,
	eDI_MP_flm22_anti_ck140,
	eDI_MP_flm22_anti_ck141,
	eDI_MP_flm22_frmdif_max,
	eDI_MP_flm22_flddif_max,
	eDI_MP_flm22_minus_cntmax,
	eDI_MP_flagdif01chk,
	eDI_MP_dif01_ratio,
	/*******vof_soft_top*****/
	eDI_MP_cmb32_blw_wnd,
	eDI_MP_cmb32_wnd_ext,
	eDI_MP_cmb32_wnd_tol,
	eDI_MP_cmb32_frm_nocmb,
	eDI_MP_cmb32_min02_sft,
	eDI_MP_cmb32_cmb_tol,
	eDI_MP_cmb32_avg_dff,
	eDI_MP_cmb32_smfrm_num,
	eDI_MP_cmb32_nocmb_num,
	eDI_MP_cmb22_gcmb_rnum,
	eDI_MP_flmxx_cal_lcmb,
	eDI_MP_flm2224_stl_sft,
	eDI_MP_SUB_PD_E,
	/**************************************/
	eDI_MP_SUB_MTN_B,
	eDI_MP_force_lev,
	eDI_MP_dejaggy_flag,
	eDI_MP_dejaggy_enable,
	eDI_MP_cmb_adpset_cnt,
	eDI_MP_cmb_num_rat_ctl4,
	eDI_MP_cmb_rat_ctl4_minthd,
	eDI_MP_small_local_mtn,
	eDI_MP_di_debug_readreg,
	eDI_MP_SUB_MTN_E,
	/**************************************/
	eDI_MP_SUB_3D_B,
	eDI_MP_chessbd_vrate,
	eDI_MP_det3d_debug,

	eDI_MP_SUB_3D_E,
	/**************************************/
	eDI_MP_UI_T_END,
};

#define K_DI_MP_UIT_NUB (eDI_MP_UI_T_END - eDI_MP_UI_T_BEGIN + 1)

struct di_mp_uit_s {
	char *name;
	enum eDI_MP_UI_T	id;
	int	default_val;
};

/*also see: div3_mpx*/
enum eDI_MP_UIX_T {
	eDI_MP_UIX_BEGIN,
	eDI_MP_UIX_RUN_FLG, /*run_flag*/
	eDI_MP_UIX_END,
};

#define K_DI_MP_UIX_NUB (eDI_MP_UIX_END - eDI_MP_UIX_BEGIN + 1)

struct di_mp_uix_s {
	char *name;
	enum eDI_MP_UIX_T	id;
	unsigned int	default_val;
};

/**************************************/
/* DI WORKING MODE */
/**************************************/
enum eDI_WORK_MODE {
	eDI_WORK_MODE_NONE,
	eDI_WORK_MODE_bypass_complet,
	eDI_WORK_MODE_bypass_all,	/*dim_is_bypass*/
	eDI_WORK_MODE_bypass_pre,
	eDI_WORK_MODE_bypass_post,
	eDI_WORK_MODE_i,
	eDI_WORK_MODE_p_as_i,
	eDI_WORK_MODE_p_as_p,
	eDI_WORK_MODE_p_use_ibuf,
	eDI_WORK_MODE_all,

};

/**************************************/
/* vframe                             */
/**************************************/
struct dev_vfram_t {
	#if 0
	/*same*/
	unsigned int tmode;
	const char *name;
	unsigned int ch;
	/*status:*/
	bool bypass_complete;
	bool reg;	/*use this for vframe reg/unreg*/
	#endif
	/*receiver:*/
	struct vframe_receiver_s di_vf_recv;
	/*provider:*/
	struct vframe_provider_s di_vf_prov;

/*	unsigned int data[32]; */	/*null*/
//	struct vframe_operations_s	pre_ops;
};


/**********************************************************
 * instance
 *	same as vframe
 **********************************************************/
struct dev_instance {
	struct di_init_parm parm;
	struct di_buffer *in[DIM_K_BUF_IN_LIMIT];
	struct di_buffer *out[DIM_K_BUF_OUT_LIMIT];
};

#if 0
union dim_inter_s {
	struct dev_vfram_t	dvfm;
	struct dev_instance	dinst;
};
#else
struct di_ch_s;

struct dim_itf_ops_s {
	void *(*peek)(struct di_ch_s *pch);
	void *(*get)(struct di_ch_s *pch);
	void (*put)(void *data, struct di_ch_s *pch);
};

struct dim_inter_s {
	unsigned int tmode;
	const char *name;
	unsigned int ch;
	/*status:*/
	bool bypass_complete;
	bool reg;
	struct dim_itf_ops_s opsi;
	struct dim_itf_ops_s opso;
	void (*op_post_done)(struct di_ch_s *pch);
	struct dim_dvfm_s *(*op_dvfm_fill)(struct di_ch_s *pch);
	void (*op_ins_2_doing)(struct di_ch_s *pch,
			       bool bypass,
			       struct di_buf_s *di_buf);
	union {
		struct dev_vfram_t	dvfm;
		struct dev_instance	dinst;
	} u;
};
#endif

struct di_ores_s {
	/* same as ori */
	struct di_pre_stru_s di_pre_stru;
	struct di_post_stru_s di_post_stru;

	struct di_buf_s di_buf_local[MAX_LOCAL_BUF_NUM * 2];
	struct di_buf_s di_buf_in[MAX_IN_BUF_NUM];
	struct di_buf_s di_buf_post[MAX_POST_BUF_NUM];
	/* ins_post: add for EDIM_TMODE_3_PW_LOCAL*/
	struct di_buffer ins_post[MAX_POST_BUF_NUM];
	struct queue_s queue[QUEUE_NUM];
	struct di_buf_pool_s di_buf_pool[VFRAME_TYPE_NUM];

	//struct vframe_s *vframe_in[MAX_IN_BUF_NUM];
	void *vframe_in[MAX_IN_BUF_NUM];
	struct vframe_s vframe_in_dup[MAX_IN_BUF_NUM];
	struct vframe_s vframe_local[MAX_LOCAL_BUF_NUM * 2];
	struct vframe_s vframe_post[MAX_POST_BUF_NUM];
	/* ********** */
};

/*keep same order with di_cma_state_name*/
enum eDI_CMA_ST {
	EDI_CMA_ST_IDL,
	EDI_CMA_ST_ALLOC,	/*do*/
	EDI_CMA_ST_READY,
	EDI_CMA_ST_RELEASE,	/*do*/
	EDI_CMA_ST_PART,
};

union ens_u {
	unsigned int en_32;
	struct {
		unsigned int
		h_sc_down		:1,
		pps_enable		:1,
		reserved		:30;
	} b;
};

enum EDIM_TMODE;

struct dim_cfg_s {
	enum EDIM_TMODE		tmode;
	unsigned int w_rdup;	/*width_roundup*/
	unsigned int f_w;	/*force_w: force input*/
	unsigned int f_h;	/*force_h*/

	unsigned int vfm_bitdepth;
	union ens_u ens;
};

/**********************************
 * mem
 *********************************/
struct di_mm_cfg_s {
	/*support max input interlace size*/
	unsigned int di_h;
	unsigned int di_w;
	/**/
	unsigned int num_local;
	unsigned int num_post;

	unsigned int size_local;
	unsigned int size_post;

	int nr_size;
	int count_size;
	int mcinfo_size;
	int mv_size;
	int mtn_size;

	/* alloc di buf as p or i;0: alloc buf as i;
	 * 1: alloc buf as p;
	 */
	unsigned char buf_alloc_mode;
};

/*use for reserved and alloc all*/
struct dim_mm_t_s {
	unsigned long	mem_start;
	unsigned int	mem_size;
	struct page	*total_pages;
};

struct di_mm_st_s {
	/* use for reserved and alloc all*/
	unsigned long	mem_start;
	unsigned int	mem_size;
	/* struct page	*total_pages;*/

	/*unsigned int	flag_cma;*/

//no use	unsigned int	size_local;	/*?*/
//no use	unsigned int	size_post;	/*?*/
	int	num_local;
	int	num_post;	/*for alloc nub*/
};

struct di_mm_s {
	struct di_mm_cfg_s cfg;
	struct di_mm_st_s sts;
};

struct dim_pres_s {
	/*bad_frame_throw_count*/
	unsigned int bad_frame;
};
/********************************************************************
 * for err
 ********************************************************************/
#define DIM_ERR_QUE_FIFO	DI_BIT0

/********************************************************************
 * for sum
 ********************************************************************/
struct dim_sum_s {
	/*buf*/
	unsigned int b_pre_free;
	unsigned int b_pst_ready;
	unsigned int b_recyc;
	unsigned int b_pre_ready;
	unsigned int b_pst_free;
	unsigned int b_display;
};

struct di_ch_s {
	/*struct di_cfgx_s dbg_cfg;*/
	bool cfgx_en[K_DI_CFGX_NUB];
	unsigned int mp_uix[K_DI_MP_UIX_NUB];/*module para x*/

	struct di_dbg_datax_s dbg_data;

	//struct dev_vfram_t vfm;
	struct dim_inter_s	interf;
	struct dentry *dbg_rootx;	/*dbg_fs*/

	unsigned int ch_id;
	struct di_ores_s rse_ori;
	struct dim_dvfm_s	dvfm[DIM_K_VFM_NUM];
	struct dim_que_s	qued[QUED_T_NUB];

	struct kfifo	fifo[QUE_NUB];
	bool flg_fifo[QUE_NUB];	/*have ini: 1; else 0*/
/*	bool sub_act_flg;*/
	/************************/
	/*old glob*/
	/************************/
	/*bypass_state*/
	bool bypass_state;

	struct dim_pres_s pret;
	/*video_peek_cnt*/
	unsigned int sum[eDI_SUM_NUB + 1];
	unsigned int sum_get;
	unsigned int sum_put;
	unsigned int sum_pst_done;
	struct dim_sum_s	sumx;
	struct dim_cfg_s	cfgt;
	struct dim_dvfm_s	lst_dvfm;
//	unsigned int pause;	/* 2020-05-14 */
	unsigned int err;
};

struct di_meson_data {
	const char *name;
	/*struct ic_ver icver;*/
	/*struct ddemod_reg_off regoff;*/
};

struct dim_wq_s {
	char *name;
	unsigned int ch;
	struct workqueue_struct *wq_cma;
	struct work_struct wq_work;
};
/*****************************
 * hw timer
 *****************************/
#define DIM_HTM_W_INPUT		DI_BIT16
#define DIM_HTM_W_LOCAL		DI_BIT17
#define DIM_HTM_W_POST		DI_BIT18
#define DIM_HTM_W_INT		DI_BIT19

#define DIM_HTM_WK		(DI_BIT0 | DI_BIT1 | DI_BIT2 | DI_BIT3)
#define DIM_HTM_REG		(DI_BIT4 | DI_BIT5 | DI_BIT6 | DI_BIT7)

#define DIM_HTM_WK_BIT		(DI_BIT0)
#define DIM_HTM_REG_BIT		(DI_BIT4)

#define DIM_HTM_CONDITION	(DIM_HTM_W_INPUT	|	\
				 DIM_HTM_W_LOCAL	|	\
				 DIM_HTM_W_POST		|	\
				 DIM_HTM_W_INT)

/*keep same order with dim_htr_sts_name*/
enum EHTM_STS {
	EHTM_STS_NONE,
	EHTM_STS_PROB,
	EHTM_STS_WRK,
	EHTM_STS_CANCEL,
};

struct di_timer_s {
	struct hrtimer		hrt;
	unsigned int	con;
	enum EHTM_STS	sts;
#if 0
	u64		lst_ht;
	u64		lst_tsk_st;
	u64		lst_tsk_end;
	unsigned int	pre_ready_cnt;
	unsigned int	pre_get_cnt;
#endif
};

/*****************************
 * manager:
 *****************************/
struct di_mng_s {
	/*workqueue*/
	struct dim_wq_s		wq;

	/*use enum eDI_CMA_ST*/
	atomic_t cma_mem_state[DI_CHANNEL_NUB];
	/*1:alloc cma, 0:release cma set by mng, read by work que*/
	unsigned char cma_reg_cmd[DI_CHANNEL_NUB];
	enum EDIM_TMODE		tmode_pre[DI_CHANNEL_MAX];

	/*task:*/
	struct di_task		tsk;
	/*timer*/
	struct di_timer_s	tim;

	/*channel state: use enum eDI_TOP_STATE */
	atomic_t ch_state[DI_CHANNEL_NUB];

	bool in_flg[DI_CHANNEL_NUB];
#if 0
	unsigned long	   mem_start[DI_CHANNEL_NUB];
	unsigned int	   mem_size[DI_CHANNEL_NUB];
#endif
	bool sub_act_flg[DI_CHANNEL_NUB];
	/*struct	mutex	   event_mutex[DI_CHANNEL_NUB];*/
	bool init_flg[DI_CHANNEL_NUB];	/*init_flag*/
	/*bool reg_flg[DI_CHANNEL_NUB];*/	/*reg_flag*/
	unsigned int reg_flg_ch;	/*for x ch reg/unreg flg*/
	bool trig_unreg[DI_CHANNEL_NUB];
	bool hw_reg_flg;	/*for di_reg_setting/di_unreg_setting*/
	bool act_flg		;/*active_flag*/

	bool flg_hw_int;	/*only once*/

	struct dim_mm_t_s mmt;
	struct di_mm_s	mm[DI_CHANNEL_NUB];
};

/*************************
 *debug register:
 *************************/
#define K_DI_SIZE_REG_LOG	(1000)
#define K_DI_LAB_MOD		(0xf001)
/*also see: dbg_mode_name*/
enum eDI_DBG_MOD {
	eDI_DBG_MOD_REGB,	/* 0 */
	eDI_DBG_MOD_REGE,	/* 1 */
	eDI_DBG_MOD_UNREGB,	/* 2 */
	eDI_DBG_MOD_UNREGE,	/* 3 */
	eDI_DBG_MOD_PRE_SETB,	/* 4 */
	eDI_DBG_MOD_PRE_SETE,	/* 5 */
	eDI_DBG_MOD_PRE_DONEB,	/* 6 */
	eDI_DBG_MOD_PRE_DONEE,	/* 7 */
	eDI_DBG_MOD_POST_SETB,	/* 8 */
	eDI_DBG_MOD_POST_SETE,	/* 9 */
	eDI_DBG_MOD_POST_IRQB,	/* a */
	eDI_DBG_MOD_POST_IRQE,	/* b */
	eDI_DBG_MOD_POST_DB,	/* c */
	eDI_DBG_MOD_POST_DE,	/* d */
	eDI_DBG_MOD_POST_CH_CHG,	/* e */
	eDI_DBG_MOD_POST_TIMEOUT,	/* F */

	eDI_DBG_MOD_RVB,	/*10 */
	eDI_DBG_MOD_RVE,	/*11 */

	eDI_DBG_MOD_POST_RESIZE, /*0x12 */
	eDI_DBG_MOD_END,

};

enum eDI_LOG_TYPE {
	eDI_LOG_TYPE_ALL = 1,
	eDI_LOG_TYPE_REG,
	eDI_LOG_TYPE_MOD,
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

struct di_dbg_data {
	unsigned int vframe_type;	/*use for type info*/
	unsigned int cur_channel;
	struct di_dbg_reg_log reg_log;

	unsigned int delay_cnt;/*use for regiter delay*/
};

struct qued_ops_s {
	bool (*prob)(struct di_ch_s *pch);
	void (*remove)(struct di_ch_s *pch);
	bool (*reg)(struct di_ch_s *pch);
	bool (*unreg)(struct di_ch_s *pch);
	bool (*in)(struct di_ch_s *pch, enum QUED_TYPE qtype,
		   unsigned int buf_in);
	bool (*out)(struct di_ch_s *pch, enum QUED_TYPE qtype,
		    unsigned int *buf_o);
	bool (*peek)(struct di_ch_s *pch, enum QUED_TYPE qtype,
		    unsigned int *buf_o);
	bool (*list)(struct di_ch_s *pch, enum QUED_TYPE qtype,
		     unsigned int *outbuf,
		     unsigned int *rsize);
	unsigned int (*listv3_count)(struct di_ch_s *pch, enum QUED_TYPE qtype);
	bool (*is_in)(struct di_ch_s *pch, enum QUED_TYPE qtype,
			  unsigned int buf_index);
	bool (*is_empty)(struct di_ch_s *pch, enum QUED_TYPE qtype);
	const char *(*get_name)(struct di_ch_s *pch, enum QUED_TYPE qtype);
	bool (*move)(struct di_ch_s *pch, enum QUED_TYPE qtypef,
	       enum QUED_TYPE qtypet, unsigned int *buf_index);
};

struct di_data_l_s {
	/*bool cfg_en[K_DI_CFG_NUB];*/	/*cfg_top*/
	union di_cfg_tdata_u cfg_en[K_DI_CFG_NUB];
	/*use for debug*/
	unsigned int cfg_sel;
	unsigned int cfg_dbg_mode; /*val or item*/
	/**************/
	int mp_uit[K_DI_MP_UIT_NUB];	/*eDI_MP_UI_T*/
	struct di_ch_s ch_data[DI_CHANNEL_NUB];
	int plane[DI_CHANNEL_NUB];	/*use for debugfs*/
	//struct dim_cfg_s	cfg_sys[DI_CHANNEL_NUB];	/**/
	struct di_dbg_data dbg_data;
	struct di_mng_s mng;
	struct di_hpre_s hw_pre;
	struct di_hpst_s hw_pst;
	struct dentry *dbg_root_top;	/* dbg_fs*/
	/*pq_ops*/
	const struct pulldown_op_s *ops_pd;	/* pulldown */
	const struct detect3d_op_s *ops_3d;	/* detect_3d */
	const struct nr_op_s *ops_nr;	/* nr */
	const struct mtn_op_s *ops_mtn;	/* deinterlace_mtn */
	/*di ops for other module */
	/*struct di_ext_ops *di_api; */
	const struct di_meson_data *mdata;
	/*const struct qued_ops_s	*que_op;*/
};

enum EDI_ST {
	EDI_ST_PRE		= DI_BIT0,
	EDI_ST_P_U2I		= DI_BIT1,
	EDI_ST_BYPASS		= DI_BIT2,
	EDI_ST_P_T		= DI_BIT3,
	EDI_ST_P_B		= DI_BIT4,
	EDI_ST_I		= DI_BIT5,
	EDI_ST_DUMMY		= DI_BIT6,
	EDI_ST_AS_LINKA		= DI_BIT7,
	EDI_ST_AS_LINKB		= DI_BIT8,
	EDI_ST_AS_LINK_ERR1	= DI_BIT9,
	EDI_ST_PRE_DONE		= DI_BIT10,
	EDI_ST_VFM_I		= DI_BIT11,
	EDI_ST_VFM_P_ASI_T	= DI_BIT12,
	EDI_ST_VFM_P_ASI_B	= DI_BIT13,
	EDI_ST_VFM_P_U2I	= DI_BIT14,
	EDI_ST_VFM_RECYCLE	= DI_BIT15,
	EDI_ST_DIS		= DI_BIT16,
	EDI_ST_RECYCLE		= DI_BIT17,
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
#define DBG_M_QUED		DI_BIT12
#define DBG_M_HTM		DI_BIT13	/*hw timer*/
#define DBG_M_WQ		DI_BIT14	/*work que*/


extern unsigned int div3_dbg;

#define dbg_m(mark, fmt, args ...)		\
	do {					\
		if (div3_dbg & DBG_M_C_ALL)	\
			break;			\
		if ((div3_dbg & DBG_M_O_ALL) ||	\
		    (div3_dbg & (mark))) {		\
			pr_info("dimv3:"fmt, ##args); \
		}				\
	} while (0)

#define PR_ERR(fmt, args ...)		pr_err("dimv3:err:"fmt, ## args)
#define PR_WARN(fmt, args ...)		pr_err("dimv3:warn:"fmt, ## args)
#define PR_INF(fmt, args ...)		pr_info("dimv3:"fmt, ## args)

#define dbg_dt(fmt, args ...)		dbg_m(DBG_M_DT, fmt, ##args)
#define dbg_reg(fmt, args ...)		dbg_m(DBG_M_REG, fmt, ##args)
#define dbg_post_ref(fmt, args ...)	dbg_m(DBG_M_POST_REF, fmt, ##args)
#define dbg_poll(fmt, args ...)		dbg_m(DBG_M_POLLING, fmt, ##args)
#define dbg_tsk(fmt, args ...)		dbg_m(DBG_M_TSK, fmt, ##args)

#define dbg_init(fmt, args ...)		dbg_m(DBG_M_INIT, fmt, ##args)
#define dbg_ev(fmt, args ...)		dbg_m(DBG_M_EVENT, fmt, ##args)
#define dbgv3_first_frame(fmt, args ...) dbg_m(DBG_M_FIRSTFRAME, fmt, ##args)
#define dbg_dbg(fmt, args ...)		dbg_m(DBG_M_DBG, fmt, ##args)
#define dbg_once(fmt, args ...)		dbg_m(DBG_M_ONCE, fmt, ##args)
#define dbg_qued(fmt, args ...)		dbg_m(DBG_M_QUED, fmt, ##args)
#define dbg_mem(fmt, args ...)		dbg_m(DBG_M_MEM, fmt, ##args)
#define dbg_keep(fmt, args ...)		dbg_m(DBG_M_KEEP, fmt, ##args)
#define dbg_htm(fmt, args ...)		dbg_m(DBG_M_HTM, fmt, ##args)
#define dbg_wq(fmt, args ...)		dbg_m(DBG_M_WQ, fmt, ##args)

char *div3_cfgx_get_name(enum eDI_CFGX_IDX idx);
bool div3_cfgx_get(unsigned int ch, enum eDI_CFGX_IDX idx);
void div3_cfgx_set(unsigned int ch, enum eDI_CFGX_IDX idx, bool en);

static inline struct di_data_l_s *get_datal(void)
{
	if (getv3_dim_de_devp())
		return (struct di_data_l_s *)getv3_dim_de_devp()->data_l;

	PR_ERR("get_datal:xxx\n");
	return NULL;
}

static inline struct di_ch_s *get_chdata(unsigned int ch)
{
	return &get_datal()->ch_data[ch];
}

static inline struct di_mng_s *get_bufmng(void)
{
	return &get_datal()->mng;
}

static inline struct di_dbg_data *get_dbg_data(void)
{
	return &get_datal()->dbg_data;
}

static inline struct di_hpre_s  *get_hw_pre(void)
{
	return &get_datal()->hw_pre;
}

static inline struct di_hpst_s  *get_hw_pst(void)
{
	return &get_datal()->hw_pst;
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

#ifdef DIM_DEBUG_QUE_ERR
static inline struct di_dbg_que_s *di_get_dbg_que(unsigned int ch)
{
	return &get_datal()->ch_data[ch].dbg_data.que;
}
#endif

/**********************
 *
 *	flg_wait_int
 *********************/
static inline void di_pre_wait_irq_set(bool on)
{
	get_hw_pre()->flg_wait_int = on;
}

static inline bool di_pre_wait_irq_get(void)
{
	return get_hw_pre()->flg_wait_int;
}

static inline struct di_ores_s *get_orsc(unsigned int ch)
{
	return &get_datal()->ch_data[ch].rse_ori;
}

//static inline struct vframe_s **get_vframe_in(unsigned int ch)
//static inline union dim_itf_u **get_vframe_in(unsigned int ch)
static inline void **getv3_vframe_in(unsigned int ch)
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

static inline struct di_buffer *get_post_ins(unsigned int ch)
{
	return &get_orsc(ch)->ins_post[0];
}

static inline enum eDI_SUB_ID get_current_channel(void)
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

static inline void set_init_flag(unsigned char ch, bool on)
{
	get_bufmng()->init_flg[ch] =  on;
}

extern const unsigned int div3_ch2mask_table[DI_CHANNEL_MAX];
/******************************************
 *
 *	reg / unreg
 *
 *****************************************/
static inline bool get_reg_flag(unsigned char ch)
{
	unsigned int flg = get_bufmng()->reg_flg_ch;
	bool ret = false;

	if (div3_ch2mask_table[ch] & flg)
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
		get_bufmng()->reg_flg_ch = flg | div3_ch2mask_table[ch];
	else
		get_bufmng()->reg_flg_ch = flg & (~div3_ch2mask_table[ch]);
	/*dim_print("%s:%d\n", __func__, get_bufmng()->reg_flg_ch);*/
}

static inline struct dim_inter_s *get_dev_intf(unsigned int ch)
{
	if (ch < DI_CHANNEL_NUB)
		//return &get_datal()->ch_data[ch].vfm;
		return &get_datal()->ch_data[ch].interf;

	PR_ERR("%s ch overflow %d\n", __func__, ch);
	return &get_datal()->ch_data[0].interf;
}

/******************************************
 *
 *	trig unreg:
 *		when unreg: set 1
 *		when reg: set 0
 *****************************************/

static inline bool get_flag_trig_unreg(unsigned char ch)
{
	return get_bufmng()->trig_unreg[ch];
}

#if 0
static inline unsigned int get_reg_flag_all(void)
{
	return get_bufmng()->reg_flg_ch;
}
#endif

static inline void set_flag_trig_unreg(unsigned char ch, bool on)
{
	get_bufmng()->trig_unreg[ch] =  on;
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

/*sum*/
static inline void di_sum_set_l(unsigned int ch, enum eDI_SUM id,
				unsigned int val)
{
	get_chdata(ch)->sum[id] = val;
}

static inline unsigned int di_sum_inc_l(unsigned int ch, enum eDI_SUM id)
{
	get_chdata(ch)->sum[id]++;
	return get_chdata(ch)->sum[id];
}

static inline unsigned int di_sum_get_l(unsigned int ch, enum eDI_SUM id)
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

#if 0
static inline struct semaphore *get_sema(void)
{
	return &getv3_dim_de_devp()->sema;
}
#endif

static inline struct di_task *get_task(void)
{
	return &get_bufmng()->tsk;
}

static inline struct di_timer_s *get_htimer(void)
{
	return &get_bufmng()->tim;
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

#if 0
static inline struct di_ext_ops *get_ops_api(void)
{
	return get_datal()->di_api;
}
#endif

#define di_mpr(x) dimp_get(edi_mp_##x)

/******************************************
 *	module para for di
 *****************************************/

static inline int dimp_get(enum eDI_MP_UI_T idx)
{
	return get_datal()->mp_uit[idx];
}

static inline void dimp_set(enum eDI_MP_UI_T idx, int val)
{
	get_datal()->mp_uit[idx] = val;
}

static inline int dimp_inc(enum eDI_MP_UI_T idx)
{
	get_datal()->mp_uit[idx]++;
	return get_datal()->mp_uit[idx];
}

static inline int dimp_dec(enum eDI_MP_UI_T idx)
{
	get_datal()->mp_uit[idx]--;
	return get_datal()->mp_uit[idx];
}

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

#if 1
/****************************************
 * pre hw ops
 ****************************************/
static inline struct di_pre_stru_s *get_h_ppre(void)
{
	return get_hw_pre()->ops.pres;
}

static inline void set_h_ppre(unsigned int ch)
{
	get_hw_pre()->ops.pres = get_pre_stru(ch);
}

#endif

static inline unsigned int dim_get_timerms(u64 check_timer)
{
	return (unsigned int)(jiffies_to_msecs(jiffies_64 - check_timer));
}

/**/
void div3_tout_int(struct di_time_out_s *tout, unsigned int thd);
bool div3_tout_contr(enum eDI_TOUT_CONTR cmd, struct di_time_out_s *tout);
extern const struct qued_ops_s qued_ops;

#endif	/*__DI_DATA_L_H__*/
