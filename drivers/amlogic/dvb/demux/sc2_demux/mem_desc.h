/*
 * drivers/amlogic/dvb/demux/sc2_demux/mem_desc.h
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

#ifndef _MEM_DESC_H_
#define _MEM_DESC_H_

#ifndef u8
#define u8 unsigned char
#endif

union mem_desc {
	u64 data;
	struct {
		u64 byte_length:27;	// 26:0
		u64 irq:1;	// 27 not used
		u64 eoc:1;	// 28 not used
		u64 loop:1;	// 29
		u64 error:1;	// 30 not used
		u64 owner:1;	// 31 not used
		u64 address:32;	// 63:32
	} bits;
};

struct chan_id {
	u8 used;
	u8 id;
	u8 is_es;
	u8 mode;
	u8 enable;
	unsigned long mem;
	unsigned long mem_phy;
	unsigned int mem_size;
	int sec_level;
	union mem_desc *memdescs;
	unsigned int memdescs_phy;
	unsigned int r_offset;
	unsigned long memdescs_map;
	unsigned long last_w_addr;
	unsigned int tee_handle;

	/*just for DVR sec direct mem*/
	unsigned int sec_mem;
	unsigned int sec_size;
};

enum bufferid_mode {
	INPUT_MODE,
	OUTPUT_MODE
};

 /**/ struct bufferid_attr {
	u8 is_es;
	enum bufferid_mode mode;
	u8 req_id;
};

/**
 * chan init
 * \retval 0: success
 * \retval -1:fail.
 */
int SC2_bufferid_init(void);

/**
 * alloc chan
 * \param attr
 * \param pchan,if ts/es, return this channel
 * \param pchan1, if es, return this channel for pcr
 * \retval success: 0
 * \retval -1:fail.
 */
int SC2_bufferid_alloc(struct bufferid_attr *attr,
		       struct chan_id **pchan, struct chan_id **pchan1);

/**
 * dealloc chan & free mem
 * \param pchan:struct chan_id handle
 * \retval success: 0
 * \retval -1:fail.
 */
int SC2_bufferid_dealloc(struct chan_id *pchan);

/**
 * chan mem
 * \param pchan:struct chan_id handle
 * \param mem_size:used memory
 * \param sec_level: memory security level
 * \retval success: 0
 * \retval -1:fail.
 */
int SC2_bufferid_set_mem(struct chan_id *pchan,
			 unsigned int mem_size, int sec_level);

/**
 * chan mem
 * \param pchan:struct chan_id handle
 * \param sec_mem:direct memory
 * \param sec_size: memory size
 * \retval success: 0
 * \retval -1:fail.
 */
int SC2_bufferid_set_sec_mem(struct chan_id *pchan,
			 unsigned int sec_mem, unsigned int sec_size);

/**
 * set enable
 * \param pchan:struct chan_id handle
 * \param enable: 1/0
 * \retval success: 0
 * \retval -1:fail.
 */
int SC2_bufferid_set_enable(struct chan_id *pchan, int enable);

/**
 * recv data
 * \param pchan:struct chan_id handle
 * \retval 0: no data
 * \retval 1: recv data, it can call SC2_bufferid_read
 */
int SC2_bufferid_recv_data(struct chan_id *pchan);

/**
 * chan read
 * \param pchan:struct chan_id handle
 * \param pread: if is secure will return physical addr, otherwise virtual addr
 * \param plen:data size addr
 * \param is_sec: 1 is secure, 0 is normal
 * \retval >=0:read cnt.
 * \retval -1:fail.
 */
int SC2_bufferid_read(struct chan_id *pchan, char **pread, unsigned int len,
		int is_sec);

/**
 * write to channel
 * \param pchan:struct chan_id handle
 * \param buf: data addr
 * \param  count: write size
 * \param  isphybuf: isphybuf
 * \retval -1:fail
 * \retval written size
 */
int SC2_bufferid_write(struct chan_id *pchan, const char __user *buf,
		       unsigned int count, int isphybuf);

unsigned int SC2_bufferid_get_free_size(struct chan_id *pchan);
unsigned int SC2_bufferid_get_wp_offset(struct chan_id *pchan);

int _alloc_buff(unsigned int len, int sec_level,
		unsigned long *vir_mem, unsigned long *phy_mem,
		unsigned int *handle);
void _free_buff(unsigned long buf, unsigned int len, int sec_level,
		unsigned int handle);

#endif
