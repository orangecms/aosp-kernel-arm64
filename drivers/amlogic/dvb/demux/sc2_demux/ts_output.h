/*
 * drivers/amlogic/dvb/demux/sc2_demux/ts_output.h
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

#ifndef _TS_OUTPUT_H_
#define _TS_OUTPUT_H_

#include <linux/types.h>
#include "sc2_control.h"
#include <linux/dvb/dmx.h>

struct pts_dts {
	int pid;
	char pts_dts_flag;
	int es_bytes_from_beginning;
	u64 pts;
	u64 dts;
};

struct out_elem;

typedef int (*ts_output_cb) (struct out_elem *pout,
			     char *buf, int count, void *udata,
				 int req_len, int *req_ret);
enum content_type {
	NONE_TYPE,
	VIDEO_TYPE,
	AUDIO_TYPE,
	SUB_TYPE,
	TTX_TYPE,
	SEC_TYPE,
	OTHER_TYPE
};

/**
 * ts_output_init
 * \param sid_num
 * \param sid_info
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_output_init(int sid_num, int *sid_info);

/**
 * ts_output_destroy
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_output_destroy(void);

/**
 * remap pid
 * \param sid: stream id
 * \param pid: orginal pid
 * \param new_pid: replace pid
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_output_remap_pid(int sid, int pid, int new_pid);

/**
 * set pid pcr
 * \param sid: stream id
 * \param pcrpid
 * \param pcr_num
 * \retval 0:success.
 * \retval -1:fail.
 * \note:pcrpid == -1, it will close
 */
int ts_output_set_pcr(int sid, int pcrpid, int pcr_num);

/**
 * get pcr value
 * \param pcrpid
 * \param pcr_num
 * \param pcr:pcr value
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_output_get_pcr(int pcr_num, uint64_t *pcr);

struct out_elem *ts_output_find_same_section_pid(int sid, int pid);

/**
 * open one output pipeline
 * \param dmx_id:demux id.
 * \param sid:stream id.
 * \param format:output format.
 * \param type:input content type.
 * \param media_type:aucpu support format
 * \param output_mode:1 will output raw mode,just for ES.
 * \retval return out_elem.
 * \retval NULL:fail.
 */
struct out_elem *ts_output_open(int sid, u8 dmx_id, u8 format,
				enum content_type type, int media_type,
				int output_mode);

/**
 * close openned index
 * \param pout
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_output_close(struct out_elem *pout);

/**
 * add pid in stream
 * \param pout
 * \param pid:
 * \param pid_mask:0,matched all bits; 0x1FFF matched any PID
 * \param dmx_id: dmx_id
 * \param cb_id:same pid ref
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_output_add_pid(struct out_elem *pout, int pid, int pid_mask, int dmx_id,
		      int *cb_id);
/**
 * remove pid in stream
 * \param pout
 * \param pid
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_output_remove_pid(struct out_elem *pout, int pid);

/**
 * set out elem mem
 * \param memsize
 * \param sec_level
 * \param pts_memsize
 * \param pts_level
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_output_set_mem(struct out_elem *pout, int memsize,
	int sec_level, int pts_memsize, int pts_level);

int ts_output_set_sec_mem(struct out_elem *pout,
	unsigned int buf, unsigned int size);

int ts_output_get_mem_info(struct out_elem *pout,
			   unsigned int *total_size,
			   unsigned int *buf_phy_start,
			   unsigned int *free_size, unsigned int *wp_offset,
			   __u64 *newest_pts);

/**
 * reset index pipeline, clear the buf
 * \param pout
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_output_reset(struct out_elem *pout);

/**
 * set callback for getting data
 * \param pout
 * \param cb
 * \param udata:private data
 * \param dmx_id:dmx_id
 * \param format:format
 * \param is_sec: is section callback
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_output_add_cb(struct out_elem *pout, ts_output_cb cb, void *udata,
		     u8 dmx_id, u8 format, bool is_sec);

/**
 * remove callback for getting data
 * \param pout
 * \param cb
 * \param udata:private data
 * \param cb_id:cb_id
 * \param is_sec: is section callback
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_output_remove_cb(struct out_elem *pout, ts_output_cb cb, void *udata,
			u8 cb_id, bool is_sec);

struct out_elem *ts_output_find_dvr(int sid);

int ts_output_sid_debug(void);
int ts_output_dump_info(char *buf);
#endif
