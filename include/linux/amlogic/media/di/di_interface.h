/*
 * include/linux/amlogic/media/di/di_interface.h
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

#ifndef __DI_INTERLACE_H__
#define __DI_INTERLACE_H__

enum di_work_mode {
	WORK_MODE_PRE = 0,
	WORK_MODE_POST,
	WORK_MODE_PRE_POST,
	WORK_MODE_MAX
};

enum di_buffer_mode {
	BUFFER_MODE_ALLOC_BUF = 0, /*di alloc output buffer*/
	BUFFER_MODE_USE_BUF,
	/* BUFFER_MODE_USE_BUF
	 * di not alloc output buffer, need caller alloc output buffer
	 ***************************/
	BUFFER_MODE_MAX
};

/**********************************************************
 * If flag is DI_BUFFERFLAG_ENDOFFRAME,
 * di_buffer is invalid,
 * only notify di that there will be no more input
 *********************************************************/
#define DI_BUFFERFLAG_ENDOFFRAME 0x00000001

struct ins_mng_s {	/*ary add*/
	unsigned int	code;
	unsigned int	ch;
	unsigned int	index;

	unsigned int	type;
	struct page	*pages;
};

struct di_buffer {
	/*header*/
	struct ins_mng_s mng;
	void *private_data;
	u32 size;
	unsigned long phy_addr;
	struct vframe_s *vf;
	void *caller_data; /*from di_init_parm.caller_data*/
	u32 flag;
};

enum DI_FLAG {
	DI_FLAG_P               = 0x1,
	DI_FLAG_I               = 0x2,
	DI_FLAG_EOS		= 0x4,
	DI_FLAG_BUF_BY_PASS	= 0x8,
	DI_FLAG_MAX = 0x7FFFFFFF,
};

enum DI_ERRORTYPE {
	DI_ERR_NONE = 0,
	DI_ERR_UNDEFINED = 0x80000001,
	DI_ERR_REG_NO_IDLE_CH,
	DI_ERR_INDEX_OVERFLOW,
	DI_ERR_INDEX_NOT_ACTIVE,
	DI_ERR_IN_NO_SPACE,
	DI_ERR_UNSUPPORT,
	DI_ERR_MAX = 0x7FFFFFFF,
};

struct di_operations_s {
	/*The input data has been processed and returned to the caller*/
	enum DI_ERRORTYPE (*empty_input_done)(struct di_buffer *buf);
	/* The output buffer has filled in the data and returned it
	 * to the caller.
	 */
	enum DI_ERRORTYPE (*fill_output_done)(struct di_buffer *buf);
};

enum di_output_format {
	DI_OUTPUT_422 = 0,
	DI_OUTPUT_NV12 = 1,
	DI_OUTPUT_NV21 = 2,
	DI_OUTPUT_MAX = 0x7FFFFFFF,
};

struct di_init_parm {
	enum di_work_mode work_mode;
	enum di_buffer_mode buffer_mode;
	struct di_operations_s ops;
	void *caller_data;
	enum di_output_format output_format;
};

struct di_status {
	unsigned int status;
};

struct composer_dst {
	unsigned int h;
};

/**
 * @brief  di_create_instance  creat di instance
 *
 * @param[in]  parm    Pointer of parm structure
 *
 * @return      di index for success, or fail type if < 0
 */
int di_create_instance(struct di_init_parm parm);

/**
 * @brief  di_set_parameter  set parameter to di for init
 *
 * @param[in]  index   instance index
 *
 * @return      0 for success, or fail type if < 0
 */

int di_destroy_instance(int index);

/**
 * @brief  di_empty_input_buffer  send input date to di
 *
 * @param[in]  index   instance index
 * @param[in]  buffer  Pointer of buffer structure
 *
 * @return      Success or fail type
 */
enum DI_ERRORTYPE di_empty_input_buffer(int index, struct di_buffer *buffer);

/**
 * @brief  di_fill_output_buffer  send output buffer to di
 *
 * @param[in]  index   instance index
 * @param[in]  buffer  Pointer of buffer structure
 * @ 2019-12-30:discuss with jintao
 * @ when pre + post + local mode,
 * @	use this function put post buf to di
 * @return      Success or fail type
 */

enum DI_ERRORTYPE di_fill_output_buffer(int index, struct di_buffer *buffer);

/**
 * @brief  di_get_state  Get the state of di by instance index
 *
 * @param[in]   index   instance index
 * @param[out]  status  Pointer of di status structure
 *
 * @return      0 for success, or fail type if < 0
 */
int di_get_state(int index, struct di_status *status);

/**
 * @brief  di_write  use di pre buffer to do post
 *
 * @param[in]  buffer        di pre output buffer
 * @param[in]  composer_dst  Pointer of dst buffer and composer para structure
 *
 * @return      0 for success, or fail type if < 0
 */
int di_write(struct di_buffer *buffer, struct composer_dst *dst);

/**
 * @brief  di_release_keep_buf  release buf after instance destroy
 *
 * @param[in]  buffer        di output buffer
 *
 * @return      0 for success, or fail type if < 0
 */
//int di_release_keep_buf(struct di_buffer *buffer);

#endif	/*__DI_INTERLACE_H__*/
