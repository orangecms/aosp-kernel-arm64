/*
 * drivers/amlogic/dvb/demux/sc2_demux/ts_input.c
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
#include <linux/string.h>
#include <linux/module.h>

#include "ts_input.h"
#include "mem_desc.h"
#include "../dmx_log.h"

struct in_elem {
	__u8 used;
	__u8 mem_level;
	struct chan_id *pchan;
};

#define MAX_INPUT_NUM			32
static struct in_elem ts_input_table[MAX_INPUT_NUM];

#define TS_INPUT_DESC_MAX_SIZE	(400 * 188)
#define TS_INPUT_BUFF_SIZE		(400 * 188)

#define dprint(fmt, args...)   \
	dprintk(LOG_ERROR, debug_input, "ts_input:" fmt, ## args)
#define pr_dbg(fmt, args...)   \
	dprintk(LOG_DBG, debug_input, "ts_input:" fmt, ## args)

MODULE_PARM_DESC(debug_input, "\n\t\t Enable demux input information");
static int debug_input;
module_param(debug_input, int, 0644);

/**
 * ts_input_init
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_input_init(void)
{
	memset(&ts_input_table, 0, sizeof(ts_input_table));
	return 0;
}

/**
 * ts_input_destroy
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_input_destroy(void)
{
	int i = 0;

	for (i = 0; i < MAX_INPUT_NUM; i++) {
		struct in_elem *pelem = &ts_input_table[i];

		if (pelem->used && pelem->pchan) {
			SC2_bufferid_dealloc(pelem->pchan);
			pelem->pchan = NULL;
		}
		pelem->used = 0;
	}
	return 0;
}

/**
 * ts_input_open
 * \param id:
 * \param sec_level:
 * \retval in_elem:success.
 * \retval NULL:fail.
 */
struct in_elem *ts_input_open(int id, int sec_level)
{
	int ret = 0;
	struct bufferid_attr attr;
	struct in_elem *elem;

	pr_dbg("%s line:%d\n", __func__, __LINE__);

	if (id >= MAX_INPUT_NUM) {
		dprint("%s id:%d invalid\n", __func__, id);
		return NULL;
	}
	attr.mode = INPUT_MODE;
	attr.req_id = id;
	ret = SC2_bufferid_alloc(&attr, &ts_input_table[id].pchan, NULL);
	if (ret != 0)
		return NULL;

	ts_input_table[id].mem_level = sec_level;
	ts_input_table[id].used = 1;
	elem = &ts_input_table[id];

	if (SC2_bufferid_set_mem(elem->pchan,
				 TS_INPUT_BUFF_SIZE, sec_level) != 0)
		dprint("input id:%d, malloc fail\n", id);

	pr_dbg("%s line:%d\n", __func__, __LINE__);
	return elem;
}

/**
 * ts_input_close
 * \param elem:
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_input_close(struct in_elem *elem)
{
	if (elem && elem->pchan)
		SC2_bufferid_dealloc(elem->pchan);
	elem->used = 0;
	return 0;
}

/**
 * ts_input_write
 * \param elem:
 * \param buf:
 * \param count:
 * \retval size:written count
 * \retval -1:fail.
 */
int ts_input_write(struct in_elem *elem, const char *buf, int count)
{
	int ret = 0;

	pr_dbg("%s line:%d\n", __func__, __LINE__);

	if (elem && elem->mem_level && count > TS_INPUT_DESC_MAX_SIZE)
		return -1;

	pr_dbg("%s line:%d\n", __func__, __LINE__);

	if (!elem->pchan || !buf) {
		pr_dbg("%s invalid parameter line:%d\n", __func__, __LINE__);
		return 0;
	}
	ret = SC2_bufferid_write(elem->pchan,
				 buf, count, elem->mem_level ? 1 : 0);
	return ret;
}
