/*
 * drivers/amlogic/dvb/demux/sc2_demux/ts_output.c
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
#include <linux/vmalloc.h>
#include <linux/of.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/dvb/dmx.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>

#include "sc2_control.h"
#include "ts_output.h"
#include "mem_desc.h"
#include "../../aucpu/aml_aucpu.h"
#include "../aml_dvb.h"
#include "../dmx_log.h"
#include <linux/fs.h>

#define MAX_TS_PID_NUM              1024
#define MAX_SID_NUM                 64
#define MAX_REMAP_NUM               32
#define MAX_ES_NUM                  64
#define MAX_OUT_ELEM_NUM            128
#define MAX_PCR_NUM                 16

#define MAX_READ_BUF_LEN			(64 * 1024)
#define MAX_DVR_READ_BUF_LEN		(2 * 1024 * 1024)

#define MAX_FEED_NUM				32
static int ts_output_max_pid_num_per_sid = 16;
/*protect cb_list/ts output*/
static struct mutex *ts_output_mutex;
static struct mutex es_output_mutex;

struct es_params_t {
	struct dmx_non_sec_es_header header;
	char last_header[16];
	u8 have_header;
	u8 have_send_header;
	unsigned long data_start;
	unsigned int data_len;
	int es_overflow;
};

struct ts_out {
	struct out_elem *pout;
	struct es_params_t *es_params;
	struct ts_out *pnext;
};

struct ts_out_task {
	int running;
	wait_queue_head_t wait_queue;
	struct task_struct *out_task;
	u16 flush_time_ms;
	struct timer_list out_timer;
	struct ts_out *ts_out_list;
};

struct pid_entry {
	u8 used;
	u16 id;
	u16 pid;
	u16 pid_mask;
	u8 dmx_id;
	u8 ref;
	struct out_elem *pout;
	struct pid_entry *pnext;
};

struct cb_entry {
	u8 id;
	u8 format;
	u8 ref;
	ts_output_cb cb;
	void *udata[MAX_FEED_NUM];
	struct cb_entry *next;
};

struct dump_file {
	loff_t file_pos;
	struct file *file_fp;
};

struct out_elem {
	u8 used;
	u8 sid;
	u8 enable;
	u8 ref;
	u8 dmx_id;
	enum output_format format;
	enum content_type type;
	int media_type;

	struct pid_entry *pid_list;
	struct es_entry *es_pes;
	struct chan_id *pchan;
	struct chan_id *pchan1;
	struct pid_entry *pcrpid_list;

	char *cache;
	u16 cache_len;
	u16 remain_len;
	struct cb_entry *cb_sec_list;
	struct cb_entry *cb_ts_list;
	u16 flush_time_ms;
	int running;
	u8 output_mode;

	s32 aucpu_handle;
	u8 aucpu_start;
	unsigned long aucpu_mem_phy;
	unsigned long aucpu_mem;
	unsigned int aucpu_mem_size;
	unsigned int aucpu_read_offset;
	__u64 newest_pts;

	/*pts/dts for aucpu*/
	s32 aucpu_pts_handle;
	u8 aucpu_pts_start;
	unsigned long aucpu_pts_mem_phy;
	unsigned long aucpu_pts_mem;
	unsigned int aucpu_pts_mem_size;
	unsigned int aucpu_pts_r_offset;

	struct dump_file dump_file;
};

struct sid_entry {
	int used;
	int pid_entry_begin;
	int pid_entry_num;
};

struct remap_entry {
	int status;
	u8 stream_id;
	int remap_flag;
	int pid_entry;
	int pid;
	int pid_new;
};

struct es_entry {
	u8 used;
	u8 buff_id;
	u8 id;
	u8 dmx_id;
	int status;		//-1:off;
	int pid;
	struct out_elem *pout;
};

struct pcr_entry {
	u8 turn_on;
	u8 stream_id;
	int pcr_pid;
};

static struct pid_entry *pid_table;
static struct sid_entry sid_table[MAX_SID_NUM];
static struct remap_entry remap_table[MAX_REMAP_NUM];
static struct es_entry es_table[MAX_ES_NUM];
static struct out_elem *out_elem_table;
static struct pcr_entry pcr_table[MAX_PCR_NUM];
static struct ts_out_task ts_out_task_tmp;
static struct ts_out_task es_out_task_tmp;
static int timer_wake_up;
static int timer_es_wake_up;

#define dprint(fmt, args...) \
	dprintk(LOG_ERROR, debug_ts_output, "ts_output:" fmt, ## args)
#define dprintk_info(fmt, args...) \
	dprintk(LOG_ERROR, debug_ts_output, fmt, ## args)
#define pr_dbg(fmt, args...) \
	dprintk(LOG_DBG, debug_ts_output, "ts_output:" fmt, ## args)

MODULE_PARM_DESC(debug_ts_output, "\n\t\t Enable demux debug information");
static int debug_ts_output;
module_param(debug_ts_output, int, 0644);

MODULE_PARM_DESC(drop_dup, "\n\t\t drop duplicate packet");
static int drop_dup;
module_param(drop_dup, int, 0644);

MODULE_PARM_DESC(dump_video_es, "\n\t\t dump video es packet");
static int dump_video_es;
module_param(dump_video_es, int, 0644);

MODULE_PARM_DESC(dump_audio_es, "\n\t\t dump audio es packet");
static int dump_audio_es;
module_param(dump_audio_es, int, 0644);

MODULE_PARM_DESC(dump_dvr_ts, "\n\t\t dump dvr ts packet");
static int dump_dvr_ts;
module_param(dump_dvr_ts, int, 0644);

struct dump_file dvr_dump_file;

#define VIDEOES_DUMP_FILE   "/data/video_dump"
#define AUDIOES_DUMP_FILE   "/data/audio_dump"
#define DVR_DUMP_FILE       "/data/dvr_dump"

#define READ_CACHE_SIZE      (188)

static int out_flush_time = 10;
static int out_es_flush_time = 10;

static int _handle_es(struct out_elem *pout, struct es_params_t *es_params);
static int start_aucpu_non_es(struct out_elem *pout);
static int aucpu_bufferid_read(struct out_elem *pout,
			       char **pread, unsigned int len, int is_pts);

static void dump_file_open(char *path, struct dump_file *dump_file_fp,
	int sid, int pid, int is_ts)
{
	int i = 0;
	char whole_path[255];
	struct file *file_fp;

	if (dump_file_fp->file_fp)
		return;

	//find new file name
	while (i < 999) {
		if (is_ts)
			snprintf((char *)&whole_path, sizeof(whole_path),
			"%s_%03d.ts", path, i);
		else
			snprintf((char *)&whole_path, sizeof(whole_path),
			"%s_0x%0x_0x%0x_%03d.es", path, sid, pid, i);

		file_fp = filp_open(whole_path, O_RDONLY, 0666);
		if (IS_ERR(file_fp))
			break;
		filp_close(file_fp, current->files);
		i++;
	}
	dump_file_fp->file_fp = filp_open(whole_path,
		O_CREAT | O_RDWR | O_APPEND, 0666);
	if (IS_ERR(dump_file_fp->file_fp)) {
		pr_err("create video dump [%s] file failed [%d]\n",
			whole_path, (int)PTR_ERR(dump_file_fp->file_fp));
		dump_file_fp->file_fp = NULL;
	} else {
		dprint("create dump [%s] success\n", whole_path);
	}
}

static void dump_file_write(
	char *buf, size_t count, struct dump_file *dump_file_fp)
{
	mm_segment_t old_fs;

	if (!dump_file_fp->file_fp) {
		pr_err("Failed to write video dump file fp is null\n");
		return;
	}
	if (count == 0)
		return;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	if (count != vfs_write(dump_file_fp->file_fp, buf, count,
			&dump_file_fp->file_pos))
		pr_err("Failed to write dump file\n");

	set_fs(old_fs);
}

static void dump_file_close(struct dump_file *dump_file_fp)
{
	if (dump_file_fp->file_fp) {
		vfs_fsync(dump_file_fp->file_fp, 0);
		filp_close(dump_file_fp->file_fp, current->files);
		dump_file_fp->file_fp = NULL;
	}
}

struct out_elem *_find_free_elem(void)
{
	int i = 0;

	for (i = 0; i < MAX_OUT_ELEM_NUM; i++) {
		struct out_elem *pout = &out_elem_table[i];

		if (!pout->used)
			return pout;
	}
	return NULL;
}

static struct pid_entry *_malloc_pid_entry_slot(int sid, int pid)
{
	int i = 0;
	int start = 0;
	int end = 0;
	struct pid_entry *pid_slot;
	int j = 0;
	int jump = 0;
	int row_start = 0;

	if (sid >= MAX_SID_NUM) {
		pr_dbg("%s error sid:%d\n", __func__, sid);
		return NULL;
	}

	start = sid_table[sid].pid_entry_begin;
	end = sid_table[sid].pid_entry_begin + sid_table[sid].pid_entry_num;

	for (i = start; i < end; i++) {
		if (i >= MAX_TS_PID_NUM) {
			pr_dbg("err sid:%d,pid start:%d, num:%d\n", sid,
			       sid_table[sid].pid_entry_begin,
			       sid_table[sid].pid_entry_num);
			return NULL;
		}

		pid_slot = &pid_table[i];

		if (!pid_slot->used) {
			/*check same pid, not at same row(there are 4 pid) */
			row_start = i / 4 * 4;
			for (j = row_start; j < row_start + 4; j++) {
				if (pid_table[j].used &&
				    pid_table[j].pid == pid) {
					jump = 1;
					break;
				}
			}
			if (jump) {
				pr_dbg("sid:%d at pos:%d, find pid:%d\n",
				       sid, j, pid);
				jump = 0;
				continue;
			}
			pr_dbg("sid:%d start:%d, end:%d, pid_entry:%d\n", sid,
			       start, end, pid_slot->id);
			return pid_slot;
		}
	}
	pr_dbg("err sid:%d,pid start:%d, num:%d\n", sid,
	       sid_table[sid].pid_entry_begin, sid_table[sid].pid_entry_num);

	return NULL;
}

static int _free_pid_entry_slot(struct pid_entry *pid_slot)
{
	if (pid_slot) {
		pid_slot->pid = -1;
		pid_slot->pid_mask = 0;
		pid_slot->used = 0;
	}
	return 0;
}

static struct es_entry *_malloc_es_entry_slot(void)
{
	int i = 0;

	for (i = 0; i < MAX_ES_NUM; i++) {
		struct es_entry *p = &es_table[i];

		if (p->used == 0) {
			p->used = 1;
			return p;
		}
	}

	return NULL;
}

static int _free_es_entry_slot(struct es_entry *es)
{
	if (es) {
		es->pid = 0;
		es->status = -1;
		es->used = 0;
		es->buff_id = 0;
	}
	return 0;
}

static int _check_timer_wakeup(void)
{
	if (timer_wake_up) {
		timer_wake_up = 0;
		return 1;
	}
	return 0;
}

static void _timer_ts_out_func(unsigned long arg)
{
//      dprint("wakeup ts_out_timer\n");
	if (ts_out_task_tmp.ts_out_list) {
		timer_wake_up = 1;
		wake_up_interruptible(&ts_out_task_tmp.wait_queue);
		mod_timer(&ts_out_task_tmp.out_timer,
		  jiffies + msecs_to_jiffies(out_flush_time));
	}
}

static int _check_timer_es_wakeup(void)
{
	if (timer_es_wake_up) {
		timer_es_wake_up = 0;
		return 1;
	}
	return 0;
}

static void _timer_es_out_func(unsigned long arg)
{
//      dprint("wakeup ts_out_timer\n");
	if (es_out_task_tmp.ts_out_list) {
		timer_es_wake_up = 1;
		wake_up_interruptible(&es_out_task_tmp.wait_queue);
		mod_timer(&es_out_task_tmp.out_timer,
		  jiffies + msecs_to_jiffies(out_es_flush_time));
	}
}

static int out_sec_cb_list(struct out_elem *pout, char *buf, int size)
{
	int w_size = 0;
	int last_w_size = -1;
	struct cb_entry *ptmp = NULL;

	if (pout->running != TASK_RUNNING)
		return 0;

	ptmp = pout->cb_sec_list;
	while (ptmp && ptmp->cb) {
		w_size = ptmp->cb(pout, buf, size, ptmp->udata[0], 0, 0);
		if (last_w_size != -1 && w_size != last_w_size) {
			dprint("ref:%d ", pout->ref);
			dprint("add pid:%d cache for filter ",
			       pout->pid_list->pid);
			dprint("w:%d,last_w:%d\n", w_size, last_w_size);
		}
		last_w_size = w_size;
		ptmp = ptmp->next;
	}

	return w_size;
}

static int out_ts_cb_list(struct out_elem *pout, char *buf, int size,
	int req_len, int *req_ret)
{
	int w_size = 0;
	struct cb_entry *ptmp = NULL;

	if (pout->running != TASK_RUNNING)
		return 0;

	ptmp = pout->cb_ts_list;
	while (ptmp && ptmp->cb) {
		w_size = ptmp->cb(pout, buf, size, ptmp->udata[0],
				req_len, req_ret);
		if (req_ret && *req_ret == 1)
			return 0;
		ptmp = ptmp->next;
	}

	return w_size;
}

static int section_process(struct out_elem *pout)
{
	int ret = 0;
	int len = 0, w_size;
	char *pread;

	if (pout->pchan->sec_level)
		start_aucpu_non_es(pout);

	if (pout->remain_len == 0) {
		len = MAX_READ_BUF_LEN;
		if (pout->pchan->sec_level)
			ret = aucpu_bufferid_read(pout, &pread, len, 0);
		else
			ret = SC2_bufferid_read(pout->pchan, &pread, len, 0);
		if (ret != 0) {
			if (pout->cb_sec_list) {
				w_size = out_sec_cb_list(pout, pread, ret);
				pr_dbg("%s send:%d, w:%d wwwwww\n", __func__,
				       ret, w_size);
				pout->remain_len = ret - w_size;
				if (pout->remain_len) {
					if (pout->remain_len >=
							READ_CACHE_SIZE) {
						dprint("len:%d lost data\n",
						       pout->remain_len);
						pout->remain_len = 0;
					} else {
						memcpy(pout->cache,
						       pread + w_size,
						       pout->remain_len);
					}
				}
			}
			if (pout->cb_ts_list)
				out_ts_cb_list(pout, pread, ret, 0, 0);
		}
	} else {
		len = READ_CACHE_SIZE - pout->remain_len;
		if (pout->pchan->sec_level)
			ret = aucpu_bufferid_read(pout, &pread, len, 0);
		else
			ret = SC2_bufferid_read(pout->pchan, &pread, len, 0);
		if (ret != 0) {
			memcpy(pout->cache + pout->remain_len, pread, ret);
			pout->remain_len += ret;
			if (ret == len) {
				ret = pout->remain_len;
				w_size =
				    out_sec_cb_list(pout, pout->cache, ret);
				pr_dbg("%s send:%d, w:%d\n", __func__, ret,
				       w_size);
				pout->remain_len = ret - w_size;
				if (pout->remain_len)
					memmove(pout->cache,
						pout->cache + w_size,
						pout->remain_len);
			}
			if (pout->cb_ts_list)
				out_ts_cb_list(pout, pread, ret, 0, 0);
		}
	}
	return 0;
}

static void write_sec_ts_data(struct out_elem *pout, char *buf, int size)
{
	struct dmx_sec_ts_data sec_ts_data;

	sec_ts_data.buf_start = pout->pchan->mem_phy;
	sec_ts_data.buf_end = sec_ts_data.buf_start + pout->pchan->mem_size;
	sec_ts_data.data_start = (unsigned long)buf;
	sec_ts_data.data_end = (unsigned long)buf + size;

	out_ts_cb_list(pout, (char *)&sec_ts_data,
		       sizeof(struct dmx_sec_ts_data), 0, 0);
}

static int dvr_process(struct out_elem *pout)
{
	int ret = 0;
	int len = 0;
	char *pread;
	int flag = 0;

	if (pout->pchan->sec_level)
		flag = 1;

	len = MAX_DVR_READ_BUF_LEN;
	ret = SC2_bufferid_read(pout->pchan, &pread, len, flag);
	if (ret != 0) {
		if (pout->cb_ts_list && flag == 0) {
//                      dprint("%s w:%d wwwwww\n", __func__, len);
			out_ts_cb_list(pout, pread, ret, 0, 0);
			if (dump_dvr_ts == 1) {
				dump_file_open(DVR_DUMP_FILE, &dvr_dump_file,
					0, 0, 1);
				dump_file_write(pread, ret, &dvr_dump_file);
			} else {
				dump_file_close(&dvr_dump_file);
			}
		} else if (pout->cb_ts_list && flag == 1) {
			write_sec_ts_data(pout, pread, ret);
		}
	}

	return 0;
}

static int _task_out_func(void *data)
{
	int timeout = 0;
	int ret = 0;
	int len = 0;
	struct ts_out *ptmp;
	char *pread = NULL;

	while (ts_out_task_tmp.running == TASK_RUNNING) {
		timeout =
		    wait_event_interruptible(ts_out_task_tmp.wait_queue,
						     _check_timer_wakeup());

		if (ts_out_task_tmp.running != TASK_RUNNING)
			break;

		mutex_lock(ts_output_mutex);

		ptmp = ts_out_task_tmp.ts_out_list;
		while (ptmp) {
			if (!ptmp->pout->enable) {
				ptmp = ptmp->pnext;
				continue;
			}
			if (ptmp->pout->format == SECTION_FORMAT) {
				section_process(ptmp->pout);
			} else if (ptmp->pout->format == DVR_FORMAT) {
				dvr_process(ptmp->pout);
			} else {
				len = MAX_READ_BUF_LEN;
				if (ptmp->pout->pchan->sec_level) {
					start_aucpu_non_es(ptmp->pout);
					ret = aucpu_bufferid_read(
						ptmp->pout, &pread, len, 0);
				} else {
					ret = SC2_bufferid_read(
					ptmp->pout->pchan, &pread, len, 0);
				}
				if (ret != 0)
					out_ts_cb_list(ptmp->pout, pread,
							ret, 0, 0);
			}
			ptmp = ptmp->pnext;
		}
		mutex_unlock(ts_output_mutex);
	}
	ts_out_task_tmp.running = TASK_IDLE;
	return 0;
}

static int _task_es_out_func(void *data)
{
	int timeout = 0;
	struct ts_out *ptmp;
	int ret = 0;

	while (es_out_task_tmp.running == TASK_RUNNING) {
		timeout =
		    wait_event_interruptible(es_out_task_tmp.wait_queue,
						     _check_timer_es_wakeup());

		if (es_out_task_tmp.running != TASK_RUNNING)
			break;

		mutex_lock(&es_output_mutex);

		ptmp = es_out_task_tmp.ts_out_list;
		while (ptmp) {
			if (!ptmp->pout->enable) {
				ptmp = ptmp->pnext;
				continue;
			}
			if (ptmp->pout->format == ES_FORMAT) {
				pr_dbg("get %s data\n",
				       ptmp->pout->type == AUDIO_TYPE ?
				       "audio" : "video");
				do {
					ret =
					    _handle_es(ptmp->pout,
						       ptmp->es_params);
				} while (ret == 0);
				pr_dbg("get %s data done\n",
				       ptmp->pout->type == AUDIO_TYPE ?
				       "audio" : "video");
			}
			ptmp = ptmp->pnext;
		}
		mutex_unlock(&es_output_mutex);
	}
	es_out_task_tmp.running = TASK_IDLE;
	return 0;
}

//> 0: have dirty data
//0: success
//-1: exit
//-2: pid not same
static int get_non_sec_es_header(struct out_elem *pout, char *last_header,
				 char *cur_header,
				 struct dmx_non_sec_es_header *pheader)
{
	char *pts_dts = NULL;
	int ret = 0;
	int header_len = 0;
	int offset = 0;
	int pid = 0;
	unsigned int cur_es_bytes = 0;
	unsigned int last_es_bytes = 0;

//      pr_dbg("%s enter\n", __func__);

	header_len = 16;
	offset = 0;

	if (!pout->aucpu_pts_start &&
		 pout->aucpu_pts_handle >= 0) {
		if (wdma_get_active(pout->pchan1->id)) {
			ret = aml_aucpu_strm_start(pout->aucpu_pts_handle);
			if (ret >= 0) {
				pr_dbg("aucpu pts start success\n");
				pout->aucpu_pts_start = 1;
			} else {
				pr_dbg("aucpu start fail ret:%d\n",
					   ret);
				return -1;
			}
		} else {
			return -1;
		}
	}

	while (header_len) {
		if (pout->aucpu_pts_start)
			ret = aucpu_bufferid_read(pout, &pts_dts,
					header_len, 1);
		else
			ret = SC2_bufferid_read(pout->pchan1,
					&pts_dts, header_len, 0);
		if (ret != 0) {
			memcpy((char *)(cur_header + offset), pts_dts, ret);
			header_len -= ret;
			offset += ret;
		} else {
			return -3;
		}
		if (pout->running == TASK_DEAD)
			return -1;
	}
	pid = (cur_header[1] & 0x1f) << 8 | cur_header[0];
	if (pout->es_pes->pid != pid) {
		dprint("%s pid diff req pid %d, ret pid:%d\n",
		       __func__, pout->es_pes->pid, pid);
		return -2;
	}
	pr_dbg("cur header: 0x%0x 0x%0x 0x%0x 0x%0x\n",
	       *(unsigned int *)cur_header,
	       *((unsigned int *)cur_header + 1),
	       *((unsigned int *)cur_header + 2),
	       *((unsigned int *)cur_header + 3));

	pr_dbg("last header: 0x%0x 0x%0x 0x%0x 0x%0x\n",
	       *(unsigned int *)last_header,
	       *((unsigned int *)last_header + 1),
	       *((unsigned int *)last_header + 2),
	       *((unsigned int *)last_header + 3));

	cur_es_bytes = cur_header[7] << 24 |
	    cur_header[6] << 16 | cur_header[5] << 8 | cur_header[4];

	//when start, es_bytes not equal 0, there are dirty data
	if (last_header[0] == 0xff && last_header[1] == 0xff) {
		pr_dbg("%s dirty data:%d\n", __func__, cur_es_bytes);
		if (cur_es_bytes != 0)
			return cur_es_bytes;
	}

	pheader->pts_dts_flag = last_header[2] & 0x3;
	pheader->dts = last_header[3] & 0x1;
	pheader->dts <<= 32;
	pheader->dts |= last_header[11] << 24
	    | last_header[10] << 16 | last_header[9] << 8 | last_header[8];
	pheader->pts = last_header[3] >> 1 & 0x1;
	pheader->pts <<= 32;
	pheader->pts |= last_header[15] << 24
	    | last_header[14] << 16 | last_header[13] << 8 | last_header[12];

	pheader->pts &= 0x1FFFFFFFF;
	last_es_bytes = last_header[7] << 24
	    | last_header[6] << 16 | last_header[5] << 8 | last_header[4];
	if (cur_es_bytes < last_es_bytes)
		pheader->len = 0xffffffff - last_es_bytes + cur_es_bytes;
	else
		pheader->len = cur_es_bytes - last_es_bytes;

	pr_dbg("%s len:%d,cur_es_bytes:0x%0x, last_es_bytes:0x%0x\n",
	       __func__, pheader->len, cur_es_bytes, last_es_bytes);
	pr_dbg("%s exit\n", __func__);
	return 0;
}

static int write_es_data(struct out_elem *pout, struct es_params_t *es_params)
{
	int ret;
	char *ptmp;
	int len;
	int h_len = sizeof(struct dmx_non_sec_es_header);
	int es_len = 0;

	if (es_params->have_header == 0)
		return -1;

	if (es_params->have_send_header == 0) {
		pr_dbg("%s pdts_flag:%d, pts:0x%lx, dts:0x%lx, len:%d\n",
		       pout->type == AUDIO_TYPE ? "audio" : "video",
		       es_params->header.pts_dts_flag,
		       (unsigned long)es_params->header.pts,
		       (unsigned long)es_params->header.dts,
		       es_params->header.len);
		if (es_params->header.pts_dts_flag & 0x2)
			pout->newest_pts = es_params->header.pts;
		out_ts_cb_list(pout, (char *)&es_params->header, h_len,
			(h_len + es_params->header.len),
			&es_params->es_overflow);
		es_params->have_send_header = 1;
	}

	es_len = es_params->header.len;
	len = es_len - es_params->data_len;
	ret = SC2_bufferid_read(pout->pchan, &ptmp, len, 0);
	if (ret) {
		if (!es_params->es_overflow)
			out_ts_cb_list(pout, ptmp, ret, 0, 0);
		else
			pr_dbg("audio data lost\n");
		if (((dump_audio_es & 0xFFFF)  == pout->es_pes->pid &&
			((dump_audio_es >> 16) & 0xFFFF) == pout->sid) ||
			dump_audio_es == 0xFFFFFFFF)
			dump_file_open(AUDIOES_DUMP_FILE, &pout->dump_file,
				pout->sid, pout->es_pes->pid, 0);
		if (pout->dump_file.file_fp && dump_audio_es == 0)
			dump_file_close(&pout->dump_file);
		if (pout->dump_file.file_fp)
			dump_file_write(ptmp, ret, &pout->dump_file);

		es_params->data_len += ret;
		pr_dbg("%s total len:%d, remain:%d\n",
		       pout->type == AUDIO_TYPE ? "audio" : "video",
		       es_len,
		       es_len - es_params->data_len);

		if (ret != len) {
			if (pout->pchan->r_offset != 0)
				return -1;
			/*loop back ,read one time*/
			len = es_params->header.len - es_params->data_len;
			ret = SC2_bufferid_read(pout->pchan, &ptmp, len, 0);
			if (ret) {
				if (!es_params->es_overflow)
					out_ts_cb_list(pout, ptmp, ret, 0, 0);
				else
					pr_dbg("audio data lost\n");
				if (pout->dump_file.file_fp)
					dump_file_write(
						ptmp, ret, &pout->dump_file);
				es_params->data_len += ret;
				pr_dbg("%s total len:%d, remain:%d\n",
					   pout->type == AUDIO_TYPE ?
					   "audio" : "video",
					   es_len,
					   es_len - es_params->data_len);
				if (ret != len)
					return -1;
				else
					return 0;
			}
		} else {
			return 0;
		}
	}
	return -1;
}

static int clean_es_data(struct out_elem *pout, struct chan_id *pchan,
			 unsigned int len)
{
	int ret;
	char *ptmp;
	while (len) {
		ret = SC2_bufferid_read(pout->pchan, &ptmp, len, 0);
		if (ret != 0) {
			len -= ret;
		}
		if (pout->running == TASK_DEAD)
			return -1;
	}
	return 0;
}

static int start_aucpu_non_es(struct out_elem *pout)
{
	int ret;

	if (!pout->aucpu_start &&
		pout->aucpu_handle >= 0 &&
		wdma_get_active(pout->pchan->id)) {
		ret = aml_aucpu_strm_start(pout->aucpu_handle);
		if (ret >= 0) {
			pr_dbg("aucpu start success\n");
			pout->aucpu_start = 1;
		} else {
			pr_dbg("aucpu start fail ret:%d\n", ret);
			return -1;
		}
	}
	return 0;
}

static void create_aucpu_pts(struct out_elem *pout)
{
	struct aml_aucpu_strm_buf src;
	struct aml_aucpu_strm_buf dst;
	struct aml_aucpu_inst_config cfg;
	int ret;

	if (pout->aucpu_pts_handle < 0 && pout->pchan1->sec_level) {
		src.phy_start = pout->pchan1->mem_phy;
		src.buf_size = pout->pchan1->mem_size;
		src.buf_flags = 0;

		pr_dbg("%s src aucpu pts phy:0x%lx, size:0x%0x\n",
		       __func__, src.phy_start, src.buf_size);

		pout->aucpu_pts_mem_size = pout->pchan1->mem_size;
		ret =
		    _alloc_buff(pout->aucpu_pts_mem_size, 0,
					&pout->aucpu_pts_mem,
					&pout->aucpu_pts_mem_phy, 0);
		if (ret != 0)
			return;
		pr_dbg("%s dst aucpu pts mem:0x%lx, phy:0x%lx\n",
		       __func__, pout->aucpu_pts_mem, pout->aucpu_pts_mem_phy);

		dst.phy_start = pout->aucpu_pts_mem_phy;
		dst.buf_size = pout->aucpu_pts_mem_size;
		dst.buf_flags = 0;
		cfg.media_type = MEDIA_PTS_PACK;
		cfg.dma_chn_id = (pout->es_pes->pid << 8) |
			pout->pchan1->id;
		cfg.config_flags = 0;
		pout->aucpu_pts_handle = aml_aucpu_strm_create(&src,
				&dst, &cfg);
		if (pout->aucpu_pts_handle < 0)
			dprint("%s create aucpu pts fail, ret:%d\n",
			       __func__, pout->aucpu_pts_handle);
		else
			dprint("%s create aucpu pts inst success\n", __func__);

		pout->aucpu_pts_r_offset = 0;
		pout->aucpu_pts_start = 0;
	}
}

static void create_aucpu_inst(struct out_elem *pout)
{
	struct aml_aucpu_strm_buf src;
	struct aml_aucpu_strm_buf dst;
	struct aml_aucpu_inst_config cfg;
	int ret;

	if (pout->type == NONE_TYPE)
		return;

	if (pout->type == VIDEO_TYPE) {
		create_aucpu_pts(pout);
		return;
	} else if (pout->type == AUDIO_TYPE) {
		create_aucpu_pts(pout);
	}
	/*now except the video, others will pass by aucpu */
	if (pout->aucpu_handle < 0) {
		src.phy_start = pout->pchan->mem_phy;
		src.buf_size = pout->pchan->mem_size;
		src.buf_flags = 0;

		pr_dbg("%s src aucpu phy:0x%lx, size:0x%0x\n",
		       __func__, src.phy_start, src.buf_size);

		pout->aucpu_mem_size = pout->pchan->mem_size;
		ret =
		    _alloc_buff(pout->aucpu_mem_size, 0, &pout->aucpu_mem,
				&pout->aucpu_mem_phy, 0);
		if (ret != 0)
			return;
		pr_dbg("%s dst aucpu mem:0x%lx, phy:0x%lx\n",
		       __func__, pout->aucpu_mem, pout->aucpu_mem_phy);

		dst.phy_start = pout->aucpu_mem_phy;
		dst.buf_size = pout->aucpu_mem_size;
		dst.buf_flags = 0;
		cfg.media_type = pout->media_type;	/*MEDIA_MPX; */
		cfg.dma_chn_id = pout->pchan->id;
		cfg.config_flags = 0;
		pout->aucpu_handle = aml_aucpu_strm_create(&src, &dst, &cfg);
		if (pout->aucpu_handle < 0)
			dprint("%s create aucpu fail, ret:%d\n",
			       __func__, pout->aucpu_handle);
		else
			dprint("%s create aucpu inst success\n", __func__);

		pout->aucpu_read_offset = 0;
		pout->aucpu_start = 0;
	}
}

static unsigned int aucpu_read_pts_process(struct out_elem *pout,
				       unsigned int w_offset,
				       char **pread, unsigned int len)
{
	unsigned int buf_len = len;
	unsigned int data_len = 0;

	pr_dbg("%s pts w:0x%0x, r:0x%0x\n", __func__,
	       (u32)w_offset, (u32)(pout->aucpu_pts_r_offset));
	if (w_offset > pout->aucpu_pts_r_offset) {
		data_len = min((w_offset - pout->aucpu_pts_r_offset), buf_len);
		if (data_len < 16)
			return 0;
		dma_sync_single_for_cpu(aml_get_device(),
					(dma_addr_t)(pout->aucpu_pts_mem_phy +
						      pout->aucpu_pts_r_offset),
					data_len, DMA_FROM_DEVICE);
		*pread = (char *)(pout->aucpu_pts_mem +
				pout->aucpu_pts_r_offset);
		pout->aucpu_pts_r_offset += data_len;
	} else {
		unsigned int part1_len = 0;

		part1_len = pout->aucpu_pts_mem_size - pout->aucpu_pts_r_offset;
		if (part1_len == 0) {
			data_len = min(w_offset, buf_len);
			if (data_len < 16)
				return 0;
			pout->aucpu_pts_r_offset = 0;
			*pread = (char *)(pout->aucpu_pts_mem +
				pout->aucpu_pts_r_offset);
			dma_sync_single_for_cpu(aml_get_device(), (dma_addr_t)
						(pout->aucpu_pts_mem_phy +
						 pout->aucpu_pts_r_offset),
						data_len, DMA_FROM_DEVICE);
			pout->aucpu_pts_r_offset += data_len;
			return data_len;
		}
		data_len = min(part1_len, buf_len);
		if (data_len < 16)
			return 0;

		*pread = (char *)(pout->aucpu_pts_mem +
				pout->aucpu_pts_r_offset);
		if (data_len < part1_len) {
			dma_sync_single_for_cpu(aml_get_device(), (dma_addr_t)
						(pout->aucpu_pts_mem_phy +
						 pout->aucpu_pts_r_offset),
						data_len, DMA_FROM_DEVICE);
			pout->aucpu_pts_r_offset += data_len;
		} else {
			data_len = part1_len;
			dma_sync_single_for_cpu(aml_get_device(), (dma_addr_t)
						(pout->aucpu_pts_mem_phy +
						 pout->aucpu_pts_r_offset),
						data_len, DMA_FROM_DEVICE);
			pout->aucpu_pts_r_offset = 0;
		}
	}
	pr_dbg("%s pts request:%d, ret:%d\n", __func__, len, data_len);
	return data_len;
}

static unsigned int aucpu_read_process(struct out_elem *pout,
				       unsigned int w_offset,
				       char **pread, unsigned int len,
					   int is_pts)
{
	unsigned int buf_len = len;
	unsigned int data_len = 0;

	if (is_pts)
		return aucpu_read_pts_process(pout, w_offset, pread, len);

	pr_dbg("%s w:0x%0x, r:0x%0x\n", __func__,
	       (u32)w_offset, (u32)(pout->aucpu_read_offset));
	if (w_offset > pout->aucpu_read_offset) {
		data_len = min((w_offset - pout->aucpu_read_offset), buf_len);
		dma_sync_single_for_cpu(aml_get_device(),
					(dma_addr_t)(pout->aucpu_mem_phy +
						      pout->aucpu_read_offset),
					data_len, DMA_FROM_DEVICE);
		*pread = (char *)(pout->aucpu_mem + pout->aucpu_read_offset);
		pout->aucpu_read_offset += data_len;
	} else {
		unsigned int part1_len = 0;

		part1_len = pout->aucpu_mem_size - pout->aucpu_read_offset;
		if (part1_len == 0) {
			data_len = min(w_offset, buf_len);
			pout->aucpu_read_offset = 0;
			*pread = (char *)(pout->aucpu_mem +
					pout->aucpu_read_offset);
			dma_sync_single_for_cpu(aml_get_device(), (dma_addr_t)
						(pout->aucpu_mem_phy +
						 pout->aucpu_read_offset),
						data_len, DMA_FROM_DEVICE);
			pout->aucpu_read_offset += data_len;
			return data_len;
		}
		data_len = min(part1_len, buf_len);
		*pread = (char *)(pout->aucpu_mem + pout->aucpu_read_offset);
		if (data_len < part1_len) {
			dma_sync_single_for_cpu(aml_get_device(), (dma_addr_t)
						(pout->aucpu_mem_phy +
						 pout->aucpu_read_offset),
						data_len, DMA_FROM_DEVICE);
			pout->aucpu_read_offset += data_len;
		} else {
			data_len = part1_len;
			dma_sync_single_for_cpu(aml_get_device(), (dma_addr_t)
						(pout->aucpu_mem_phy +
						 pout->aucpu_read_offset),
						data_len, DMA_FROM_DEVICE);
			pout->aucpu_read_offset = 0;
		}
	}
	pr_dbg("%s request:%d, ret:%d\n", __func__, len, data_len);
	return data_len;
}

static int aucpu_bufferid_read(struct out_elem *pout,
			       char **pread, unsigned int len, int is_pts)
{
	struct aml_aucpu_buf_upd upd;
	unsigned int w_offset = 0;
	s32 handle;
	unsigned long mem_phy;
	unsigned int r_offset;

	if (is_pts) {
		handle = pout->aucpu_pts_handle;
		mem_phy = pout->aucpu_pts_mem_phy;
		r_offset = pout->aucpu_pts_r_offset;
	} else {
		handle = pout->aucpu_handle;
		mem_phy = pout->aucpu_mem_phy;
		r_offset = pout->aucpu_read_offset;
	}

	if (aml_aucpu_strm_get_dst(handle, &upd)
	    >= 0) {
		w_offset = upd.phy_cur_ptr - mem_phy;
		if (r_offset != w_offset)
			return aucpu_read_process(pout,
					w_offset, pread, len, is_pts);
	}
	return 0;
}

static int write_aucpu_es_data(struct out_elem *pout,
	struct es_params_t *es_params, unsigned int isdirty)
{
	int ret;
	char *ptmp;
	int h_len = sizeof(struct dmx_non_sec_es_header);
	int es_len = 0;
	int len = 0;

	pr_dbg("%s chan id:%d, isdirty:%d\n", __func__,
	       pout->pchan->id, isdirty);

	if (es_params->have_header == 0)
		return -1;

	if (!pout->aucpu_start &&
		pout->format == ES_FORMAT &&
		pout->type == AUDIO_TYPE && pout->aucpu_handle >= 0) {
		if (wdma_get_active(pout->pchan->id)) {
			ret = aml_aucpu_strm_start(pout->aucpu_handle);
			if (ret >= 0) {
				pr_dbg("aucpu start success\n");
				pout->aucpu_start = 1;
			} else {
				pr_dbg("aucpu start fail ret:%d\n",
					   ret);
				return -1;
			}
		} else {
			return -1;
		}
	}

	if (es_params->have_send_header == 0) {
		pr_dbg("%s pdts_flag:%d, pts:0x%lx, dts:0x%lx, len:%d\n",
		       pout->type == AUDIO_TYPE ? "audio" : "video",
		       es_params->header.pts_dts_flag,
		       (unsigned long)es_params->header.pts,
		       (unsigned long)es_params->header.dts,
		       es_params->header.len);
		if (es_params->header.pts_dts_flag & 0x2)
			pout->newest_pts = es_params->header.pts;
		out_ts_cb_list(pout, (char *)&es_params->header, h_len,
			(h_len + es_params->header.len),
			&es_params->es_overflow);
		es_params->have_send_header = 1;
	}

	es_len = es_params->header.len;
	len = es_len - es_params->data_len;
	ret = aucpu_bufferid_read(pout, &ptmp, len, 0);
	if (ret) {
		if (!es_params->es_overflow)
			out_ts_cb_list(pout, ptmp, ret, 0, 0);
		else
			pr_dbg("audio data lost\n");
		if (((dump_audio_es & 0xFFFF)  == pout->es_pes->pid &&
			((dump_audio_es >> 16) & 0xFFFF) == pout->sid) ||
			dump_audio_es == 0xFFFFFFFF)
			dump_file_open(AUDIOES_DUMP_FILE, &pout->dump_file,
				pout->sid, pout->es_pes->pid, 0);

		if (pout->dump_file.file_fp && dump_audio_es == 0)
			dump_file_close(&pout->dump_file);

		if (pout->dump_file.file_fp)
			dump_file_write(ptmp, ret, &pout->dump_file);

		es_params->data_len += ret;
		pr_dbg("%s total len:%d, remain:%d\n",
		       pout->type == AUDIO_TYPE ? "audio" : "video",
		       es_len,
		       es_len - es_params->data_len);

		if (ret != len)
			return -1;
		else
			return 0;
	}
	return -1;
}

static int write_aucpu_sec_es_data(struct out_elem *pout,
				   struct es_params_t *es_params)
{
	unsigned int len = es_params->header.len;
	struct dmx_sec_es_data sec_es_data;
	char *ptmp;
	int ret;

	if (es_params->header.len == 0)
		return -1;

	if (!pout->aucpu_start &&
		pout->format == ES_FORMAT &&
		pout->type == AUDIO_TYPE && pout->aucpu_handle >= 0) {
		if (wdma_get_active(pout->pchan->id)) {
			ret = aml_aucpu_strm_start(pout->aucpu_handle);
			if (ret >= 0) {
				pr_dbg("aucpu start success\n");
				pout->aucpu_start = 1;
			} else {
				pr_dbg("aucpu start fail ret:%d\n",
					   ret);
				return -1;
			}
		} else {
			return -1;
		}
	}

	len = es_params->header.len - es_params->data_len;
	ret = aucpu_bufferid_read(pout, &ptmp, len, 0);
	if (es_params->data_start == 0)
		es_params->data_start = (unsigned long)ptmp;

	es_params->data_len += ret;
	if (ret != len)
		return -1;

	memset(&sec_es_data, 0, sizeof(struct dmx_sec_es_data));
	sec_es_data.pts_dts_flag = es_params->header.pts_dts_flag;
	sec_es_data.dts = es_params->header.dts;
	sec_es_data.pts = es_params->header.pts;
	sec_es_data.buf_start = pout->pchan->mem;
	sec_es_data.buf_end = pout->pchan->mem + pout->pchan->mem_size;
	sec_es_data.data_start = es_params->data_start;
	sec_es_data.data_end = (unsigned long)ptmp + len;

	pr_dbg("video data start:0x%x, end:0x%x\n",
		sec_es_data.data_start, sec_es_data.data_end);
	pr_dbg("video pdts_flag:%d, pts:0x%lx, dts:0x%lx, offset:0x%lx\n",
	       sec_es_data.pts_dts_flag, (unsigned long)sec_es_data.pts,
	       (unsigned long)sec_es_data.dts,
	       (unsigned long)(sec_es_data.data_start - sec_es_data.buf_start));

	out_ts_cb_list(pout, (char *)&sec_es_data,
		       sizeof(struct dmx_sec_es_data), 0, 0);

	es_params->data_start = 0;
	es_params->data_len = 0;
	return 0;
}

static int clean_aucpu_data(struct out_elem *pout, unsigned int len)
{
	int ret;
	char *ptmp;

	if (pout->aucpu_handle < 0)
		return -1;

	if (!pout->aucpu_start &&
		pout->format == ES_FORMAT &&
		pout->type == AUDIO_TYPE && pout->aucpu_handle >= 0) {
		if (wdma_get_active(pout->pchan->id)) {
			ret = aml_aucpu_strm_start(pout->aucpu_handle);
			if (ret >= 0) {
				pr_dbg("aucpu start success\n");
				pout->aucpu_start = 1;
			} else {
				pr_dbg("aucpu start fail ret:%d\n",
					   ret);
				return -1;
			}
		}
	}

	while (len) {
		ret = aucpu_bufferid_read(pout, &ptmp, len, 0);
		if (ret != 0)
			len -= ret;

		if (pout->running == TASK_DEAD)
			return -1;
	}
	return 0;
}

static void enforce_flush_cache(char *addr, unsigned int len)
{
	if (len == 0)
		return;
	dma_sync_single_for_cpu(
			aml_get_device(),
			(dma_addr_t)(addr),
			len, DMA_FROM_DEVICE);
}

static int write_sec_video_es_data(struct out_elem *pout,
				   struct es_params_t *es_params)
{
	unsigned int len = es_params->header.len;
	struct dmx_sec_es_data sec_es_data;
	char *ptmp;
	int ret;
	int flag = 0;

	if (es_params->header.len == 0)
		return -1;

	if (pout->pchan->sec_level)
		flag = 1;
	len = es_params->header.len - es_params->data_len;
	ret = SC2_bufferid_read(pout->pchan, &ptmp, len, flag);
	if (ret == 0)
		return -1;
	if (es_params->data_start == 0)
		es_params->data_start = (unsigned long)ptmp;

	es_params->data_len += ret;

	if (((dump_video_es & 0xFFFF)  == pout->es_pes->pid &&
		((dump_video_es >> 16) & 0xFFFF) == pout->sid) ||
		dump_video_es == 0XFFFFFFFF)
		dump_file_open(VIDEOES_DUMP_FILE, &pout->dump_file,
			pout->sid, pout->es_pes->pid, 0);

	if (pout->dump_file.file_fp && dump_video_es == 0)
		dump_file_close(&pout->dump_file);

	if (pout->dump_file.file_fp) {
		if (flag) {
			enforce_flush_cache(ptmp, ret);
			dump_file_write(
				ptmp - pout->pchan->mem_phy +
				pout->pchan->mem, ret,
				&pout->dump_file);
		} else {
			dump_file_write(ptmp, ret, &pout->dump_file);
		}
	}

	if (ret != len) {
		if (pout->pchan->r_offset != 0)
			return -1;
		/*if loop back , read one time */
		len = es_params->header.len - es_params->data_len;
		ret = SC2_bufferid_read(pout->pchan, &ptmp, len, flag);
		es_params->data_len += ret;

		if (pout->dump_file.file_fp) {
			if (flag) {
				enforce_flush_cache(ptmp, ret);
				dump_file_write(
					ptmp - pout->pchan->mem_phy +
					pout->pchan->mem, ret,
					&pout->dump_file);
			} else {
				dump_file_write(ptmp, ret,
					&pout->dump_file);
			}
		}

		if (ret != len)
			return -1;
	}
	memset(&sec_es_data, 0, sizeof(struct dmx_sec_es_data));
	sec_es_data.pts_dts_flag = es_params->header.pts_dts_flag;
	sec_es_data.dts = es_params->header.dts;
	sec_es_data.pts = es_params->header.pts;
	sec_es_data.buf_start = pout->pchan->mem_phy;
	sec_es_data.buf_end = pout->pchan->mem_phy + pout->pchan->mem_size;
	if (flag) {
		sec_es_data.data_start = es_params->data_start;
		sec_es_data.data_end = (unsigned long)ptmp + len;
	} else {
		sec_es_data.data_start = es_params->data_start -
			pout->pchan->mem + pout->pchan->mem_phy;
		sec_es_data.data_end = (unsigned long)ptmp -
			pout->pchan->mem + pout->pchan->mem_phy + len;
	}
	if (sec_es_data.data_end > sec_es_data.data_start)
		pr_dbg("video data start:0x%x, end:0x%x len:0x%x\n",
			sec_es_data.data_start, sec_es_data.data_end,
			(sec_es_data.data_end - sec_es_data.data_start));
	else
		pr_err("video data start:0x%x,data end:0x%x\n",
				sec_es_data.data_start, sec_es_data.data_end);

	if (sec_es_data.data_start > sec_es_data.buf_end)
		pr_err("video data start:0x%x, buf end:0x%x\n",
			sec_es_data.data_start, sec_es_data.buf_end);

	pr_dbg("video pdts_flag:%d, pts:0x%lx, dts:0x%lx, offset:0x%lx\n",
			sec_es_data.pts_dts_flag,
			(unsigned long)sec_es_data.pts,
			(unsigned long)sec_es_data.dts,
			(unsigned long)(sec_es_data.data_start -
				sec_es_data.buf_start));

	if (es_params->header.pts_dts_flag & 0x2)
		pout->newest_pts = sec_es_data.pts;
	out_ts_cb_list(pout, (char *)&sec_es_data,
			sizeof(struct dmx_sec_es_data), 0, 0);

	es_params->data_start = 0;
	es_params->data_len = 0;
	return 0;
}

static int _handle_es(struct out_elem *pout, struct es_params_t *es_params)
{
	int ret = 0;
	unsigned int dirty_len = 0;
	char cur_header[16];
	char *pcur_header;
	char *plast_header;
	struct dmx_non_sec_es_header *pheader = &es_params->header;

	memset(&cur_header, 0, sizeof(cur_header));
	pcur_header = (char *)&cur_header;
	plast_header = (char *)&es_params->last_header;

//      pr_dbg("enter es %s\n", pout->type ? "audio" : "video");
//      pr_dbg("%s enter,line:%d\n", __func__, __LINE__);

	if (pout->running != TASK_RUNNING)
		return -1;

	if (es_params->have_header == 0) {
		ret =
		    get_non_sec_es_header(pout, plast_header, pcur_header,
					  pheader);
		if (ret < 0) {
			return -1;
		} else if (ret > 0) {
			dirty_len = ret;
			if (pout->pchan->sec_level &&
					pout->type != VIDEO_TYPE)
				ret = clean_aucpu_data(pout, dirty_len);
			else
				ret = clean_es_data(pout,
						pout->pchan, dirty_len);
			memcpy(&es_params->last_header, pcur_header,
					sizeof(es_params->last_header));
			return 0;
		}
		if (pheader->len == 0) {
			memcpy(&es_params->last_header, pcur_header,
					sizeof(es_params->last_header));
			pr_dbg("header.len is 0, jump\n");
			return 0;
		}
		memcpy(&es_params->last_header, pcur_header,
				sizeof(es_params->last_header));
		es_params->have_header = 1;
	}

	if (pout->output_mode || pout->pchan->sec_level) {
		if (pout->type == VIDEO_TYPE) {
			ret = write_sec_video_es_data(pout, es_params);
		} else {
			//to do for use aucpu to handle non-video data
			if (pout->output_mode) {
				ret =
				    write_aucpu_sec_es_data(pout, es_params);
			} else {
				ret = write_aucpu_es_data(pout, es_params, 0);
			}
		}
		if (ret == 0) {
			es_params->have_header = 0;
			es_params->have_send_header = 0;
			es_params->data_len = 0;
			es_params->data_start = 0;
			es_params->es_overflow = 0;
			return 0;
		} else {
			return -1;
		}
	} else {
		if (es_params->have_header) {
			ret = write_es_data(pout, es_params);
			if (ret == 0) {
				es_params->have_header = 0;
				es_params->have_send_header = 0;
				es_params->data_len = 0;
				es_params->data_start = 0;
				es_params->es_overflow = 0;
				return 0;
			} else {
				return -1;
			}
		}
	}
	return -1;
}

static void add_ts_out_list(struct ts_out_task *head, struct ts_out *ts_out_tmp)
{
	struct ts_out *cur_tmp = NULL;

	cur_tmp = head->ts_out_list;
	while (cur_tmp) {
		if (!cur_tmp->pnext)
			break;
		cur_tmp = cur_tmp->pnext;
	}
	if (cur_tmp)
		cur_tmp->pnext = ts_out_tmp;
	else
		head->ts_out_list = ts_out_tmp;
}

static void remove_ts_out_list(struct out_elem *pout, struct ts_out_task *head)
{
	struct ts_out *ts_out_pre = NULL;
	struct ts_out *cur_tmp = NULL;

	cur_tmp = head->ts_out_list;
	ts_out_pre = cur_tmp;
	while (cur_tmp) {
		if (cur_tmp->pout == pout) {
			if (cur_tmp == head->ts_out_list)
				head->ts_out_list = cur_tmp->pnext;
			else
				ts_out_pre->pnext = cur_tmp->pnext;

			kfree(cur_tmp->es_params);
			kfree(cur_tmp);
			break;
		}
		ts_out_pre = cur_tmp;
		cur_tmp = cur_tmp->pnext;
	}
}

/**
 * ts_output_init
 * \param sid_num
 * \param sid_info
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_output_init(int sid_num, int *sid_info)
{
	int i = 0;
	struct sid_entry *psid = NULL;
	int times = 0;
	struct aml_dvb *advb = aml_get_dvb_device();

	do {
	} while (!tsout_get_ready() && times++ < 20);

	memset(&sid_table, 0, sizeof(sid_table));

	ts_output_max_pid_num_per_sid = MAX_TS_PID_NUM / (sid_num * 4) * 4;

	for (i = 0; i < sid_num; i++) {
		psid = &sid_table[sid_info[i]];
		psid->used = 1;
		psid->pid_entry_begin = ts_output_max_pid_num_per_sid * i;
		psid->pid_entry_num = ts_output_max_pid_num_per_sid;
		dprint("%s sid:%d,pid start:%d, len:%d\n",
		       __func__, sid_info[i],
		       psid->pid_entry_begin, psid->pid_entry_num);
		tsout_config_sid_table(sid_info[i],
				       psid->pid_entry_begin / 4,
				       psid->pid_entry_num / 4);
	}

	pid_table = vmalloc(sizeof(*pid_table) * MAX_TS_PID_NUM);
	if (!pid_table) {
		dprint("%s malloc fail\n", __func__);
		return -1;
	}

	memset(pid_table, 0, sizeof(struct pid_entry) * MAX_TS_PID_NUM);
	for (i = 0; i < MAX_TS_PID_NUM; i++) {
		struct pid_entry *pid_slot = &pid_table[i];

		pid_slot->id = i;
	}

	memset(&es_table, 0, sizeof(es_table));
	for (i = 0; i < MAX_ES_NUM; i++) {
		struct es_entry *es_slot = &es_table[i];

		es_slot->id = i;
	}

	memset(&remap_table, 0, sizeof(remap_table));

	out_elem_table = vmalloc(sizeof(*out_elem_table)
				 * MAX_OUT_ELEM_NUM);
	if (!out_elem_table) {
		dprint("%s malloc fail\n", __func__);
		vfree(pid_table);
		return -1;
	}
	memset(out_elem_table, 0, sizeof(struct out_elem) * MAX_OUT_ELEM_NUM);
//      memset(&out_elem_table, 0, sizeof(out_elem_table));
	memset(&pcr_table, 0, sizeof(pcr_table));

	ts_out_task_tmp.running = TASK_RUNNING;
	ts_out_task_tmp.flush_time_ms = out_flush_time;
	ts_out_task_tmp.ts_out_list = NULL;
	ts_output_mutex = &advb->mutex;

	init_waitqueue_head(&ts_out_task_tmp.wait_queue);
	ts_out_task_tmp.out_task =
	    kthread_run(_task_out_func, (void *)NULL, "ts_out_task");
	if (!ts_out_task_tmp.out_task)
		dprint("create ts_out_task fail\n");

	init_timer(&ts_out_task_tmp.out_timer);
	ts_out_task_tmp.out_timer.function = _timer_ts_out_func;
	ts_out_task_tmp.out_timer.data = 0;
	add_timer(&ts_out_task_tmp.out_timer);

	es_out_task_tmp.running = TASK_RUNNING;
	es_out_task_tmp.flush_time_ms = out_es_flush_time;
	mutex_init(&es_output_mutex);
	es_out_task_tmp.ts_out_list = NULL;

	init_waitqueue_head(&es_out_task_tmp.wait_queue);
	es_out_task_tmp.out_task =
	    kthread_run(_task_es_out_func, (void *)NULL, "es_out_task");
	if (!es_out_task_tmp.out_task)
		dprint("create es_out_task fail\n");

	init_timer(&es_out_task_tmp.out_timer);
	es_out_task_tmp.out_timer.function = _timer_es_out_func;
	es_out_task_tmp.out_timer.data = 0;
	add_timer(&es_out_task_tmp.out_timer);

	return 0;
}

int ts_output_sid_debug(void)
{
	int i = 0;
	int dmxdev_num;
	struct sid_entry *psid = NULL;

	memset(&sid_table, 0, sizeof(sid_table));
	dmxdev_num = 32;
	/*for every dmx dev, it will use 2 sids,
	 * one is demod, another is local
	 */
	ts_output_max_pid_num_per_sid =
	    MAX_TS_PID_NUM / (2 * dmxdev_num * 4) * 4;

	for (i = 0; i < dmxdev_num; i++) {
		psid = &sid_table[i];
		psid->used = 1;
		psid->pid_entry_begin = ts_output_max_pid_num_per_sid * 2 * i;
		psid->pid_entry_num = ts_output_max_pid_num_per_sid;
		pr_dbg("%s sid:%d,pid start:%d, len:%d\n",
		       __func__, i, psid->pid_entry_begin, psid->pid_entry_num);
		tsout_config_sid_table(i,
				       psid->pid_entry_begin / 4,
				       psid->pid_entry_num / 4);

		psid = &sid_table[i + 32];
		psid->used = 1;
		psid->pid_entry_begin =
		    ts_output_max_pid_num_per_sid * (2 * i + 1);
		psid->pid_entry_num = ts_output_max_pid_num_per_sid;

		pr_dbg("%s sid:%d, pid start:%d, len:%d\n", __func__,
		       i + 32, psid->pid_entry_begin, psid->pid_entry_num);
		tsout_config_sid_table(i + 32,
				       psid->pid_entry_begin / 4,
				       psid->pid_entry_num / 4);
	}

	return 0;
}

int ts_output_destroy(void)
{
	if (out_elem_table)
		vfree(out_elem_table);

	out_elem_table = NULL;

	if (pid_table)
		vfree(pid_table);

	pid_table = NULL;
	return 0;
}

/**
 * remap pid
 * \param sid: stream id
 * \param pid: orginal pid
 * \param new_pid: replace pid
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_output_remap_pid(int sid, int pid, int new_pid)
{
	return 0;
}

/**
 * set pid pcr
 * \param sid: stream id
 * \param pcr_num
 * \param pcrpid
 * \retval 0:success.
 * \retval -1:fail.
 * \note:pcrpid == -1, it will close
 */
int ts_output_set_pcr(int sid, int pcr_num, int pcrpid)
{
	pr_dbg("%s pcr_num:%d,pid:%d\n", __func__, pcr_num, pcrpid);
	if (pcr_num >= MAX_PCR_NUM) {
		dprint("%s num:%d invalid\n", __func__, pcr_num);
		return -1;
	}
	if (pcrpid == -1) {
		pcr_table[pcr_num].turn_on = 0;
		pcr_table[pcr_num].stream_id = -1;
		pcr_table[pcr_num].pcr_pid = -1;
		tsout_config_pcr_table(pcr_num, -1, sid);
	} else {
		pcr_table[pcr_num].turn_on = 1;
		pcr_table[pcr_num].stream_id = sid;
		pcr_table[pcr_num].pcr_pid = pcrpid;
		tsout_config_pcr_table(pcr_num, pcrpid, sid);
	}
	return 0;
}

/**
 * get pcr value
 * \param pcr_num
 * \param pcr:pcr value
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_output_get_pcr(int pcr_num, uint64_t *pcr)
{
	if (pcr_num >= MAX_PCR_NUM) {
		dprint("%s num:%d invalid\n", __func__, pcr_num);
		return -1;
	}
	if (!pcr_table[pcr_num].turn_on) {
		dprint("%s num:%d close\n", __func__, pcr_num);
		return -1;
	}
	tsout_config_get_pcr(pcr_num, pcr);
	return 0;
}

struct out_elem *ts_output_find_same_section_pid(int sid, int pid)
{
	int i = 0;

	for (i = 0; i < MAX_OUT_ELEM_NUM; i++) {
		struct out_elem *pout = &out_elem_table[i];

		if (pout->used &&
		    pout->sid == sid &&
		    pout->format == SECTION_FORMAT &&
			pout->es_pes->pid == pid) {
			return pout;
		}
	}
	return NULL;
}

struct out_elem *ts_output_find_dvr(int sid)
{
	int i = 0;

	for (i = 0; i < MAX_OUT_ELEM_NUM; i++) {
		struct out_elem *pout = &out_elem_table[i];

		if (pout->used &&
		    pout->sid == sid && pout->format == DVR_FORMAT) {
			return pout;
		}
	}
	return NULL;
}

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
				int output_mode)
{
	struct bufferid_attr attr;
	int ret = 0;
	struct out_elem *pout;
	struct ts_out *ts_out_tmp = NULL;

	pr_dbg("%s sid:%d, format:%d, type:%d ", __func__, sid, format, type);
	pr_dbg("media_type:%d, output_mode:%d\n", media_type, output_mode);

	if (sid >= MAX_SID_NUM) {
		dprint("%s sid:%d fail\n", __func__, sid);
		return NULL;
	}
	pout = _find_free_elem();
	if (!pout) {
		dprint("%s find free elem sid:%d fail\n", __func__, sid);
		return NULL;
	}
	pout->dmx_id = dmx_id;
	pout->format = format;
	pout->sid = sid;
	pout->pid_list = NULL;
	pout->output_mode = output_mode;
	pout->type = type;
	pout->media_type = media_type;
	pout->ref = 0;
	pout->newest_pts = 0;

	memset(&attr, 0, sizeof(struct bufferid_attr));
	attr.mode = OUTPUT_MODE;

	if (format == ES_FORMAT) {
		attr.is_es = 1;
		ret = SC2_bufferid_alloc(&attr, &pout->pchan, &pout->pchan1);
		if (ret != 0) {
			dprint("%s sid:%d SC2_bufferid_alloc fail\n",
			       __func__, sid);
			return NULL;
		}
		pout->enable = 0;
		pout->remain_len = 0;
		pout->cache_len = 0;
		pout->cache = NULL;
		pout->aucpu_handle = -1;
		pout->aucpu_start = 0;
		pout->aucpu_pts_handle = -1;
		pout->aucpu_pts_start = 0;
	} else {
		if (format == DVR_FORMAT && dump_dvr_ts)
			dump_file_open(DVR_DUMP_FILE, &dvr_dump_file, 0, 0, 1);

		ret = SC2_bufferid_alloc(&attr, &pout->pchan, NULL);
		if (ret != 0) {
			dprint("%s sid:%d SC2_bufferid_alloc fail\n",
			       __func__, sid);
			return NULL;
		}
		pout->aucpu_handle = -1;
		pout->aucpu_start = 0;
		pout->aucpu_pts_handle = -1;
		pout->aucpu_pts_start = 0;
		pout->enable = 0;
		pout->remain_len = 0;
		pout->cache_len = READ_CACHE_SIZE;
		pout->cache = kmalloc(pout->cache_len, GFP_KERNEL);
		if (!pout->cache) {
			SC2_bufferid_dealloc(pout->pchan);
			dprint("%s sid:%d kmalloc cache fail\n", __func__, sid);
			return NULL;
		}
	}

	ts_out_tmp = kmalloc(sizeof(*ts_out_tmp), GFP_KERNEL);
	if (!ts_out_tmp) {
		dprint("ts out list fail\n");
		SC2_bufferid_dealloc(pout->pchan);
		kfree(pout->cache);
		return NULL;
	}

	if (format == ES_FORMAT) {
		ts_out_tmp->es_params =
			kmalloc(sizeof(struct es_params_t), GFP_KERNEL);
		if (!ts_out_tmp->es_params) {
			dprint("ts out es_params fail\n");
			SC2_bufferid_dealloc(pout->pchan);
			kfree(pout->cache);
			kfree(ts_out_tmp);
			return NULL;
		}
		memset(ts_out_tmp->es_params, 0,
				sizeof(struct es_params_t));
		ts_out_tmp->es_params->last_header[0] = 0xff;
		ts_out_tmp->es_params->last_header[1] = 0xff;
	} else {
		ts_out_tmp->es_params = NULL;
	}
	ts_out_tmp->pout = pout;
	ts_out_tmp->pnext = NULL;

	if (format == ES_FORMAT) {
		mutex_lock(&es_output_mutex);
		add_ts_out_list(&es_out_task_tmp, ts_out_tmp);
		mutex_unlock(&es_output_mutex);
	} else {
		add_ts_out_list(&ts_out_task_tmp, ts_out_tmp);
	}
	pout->running = TASK_RUNNING;
	pout->used = 1;
	pr_dbg("%s exit\n", __func__);

	return pout;
}

/**
 * close openned index
 * \param pout
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_output_close(struct out_elem *pout)
{
	if (pout->ref)
		return -1;

	pout->running = TASK_DEAD;

	if (pout->format == ES_FORMAT) {
		if (pout->dump_file.file_fp)
			dump_file_close(&pout->dump_file);

		mutex_lock(&es_output_mutex);
		remove_ts_out_list(pout, &es_out_task_tmp);
		mutex_unlock(&es_output_mutex);
	} else {
		if (pout->format == DVR_FORMAT &&
			dvr_dump_file.file_fp)
			dump_file_close(&dvr_dump_file);
		remove_ts_out_list(pout, &ts_out_task_tmp);
	}

	if (pout->aucpu_handle >= 0) {
		s32 ret;

		if (pout->aucpu_start) {
			ret = aml_aucpu_strm_stop(pout->aucpu_handle);
			if (ret >= 0)
				pr_dbg("aml_aucpu_strm_stop success\n");
			else
				pr_dbg("aucpu_stop fail ret:%d\n", ret);
			pout->aucpu_start = 0;
		}
		ret = aml_aucpu_strm_remove(pout->aucpu_handle);
		if (ret >= 0)
			pr_dbg("aucpu_strm_remove success\n");
		else
			pr_dbg("aucpu_strm_remove fail ret:%d\n", ret);
		pout->aucpu_handle = -1;

		_free_buff(pout->aucpu_mem_phy,
				pout->aucpu_mem_size, 0, 0);
		pout->aucpu_mem = 0;
	}

	if (pout->aucpu_pts_handle >= 0) {
		s32 ret;

		if (pout->aucpu_pts_start) {
			ret = aml_aucpu_strm_stop(pout->aucpu_pts_handle);
			if (ret >= 0)
				pr_dbg("aml_aucpu_strm_stop pts success\n");
			else
				pr_dbg("aucpu_stop fail ret:%d\n", ret);
			pout->aucpu_pts_start = 0;
		}
		ret = aml_aucpu_strm_remove(pout->aucpu_pts_handle);
		if (ret >= 0)
			pr_dbg("aucpu_strm_remove pts success\n");
		else
			pr_dbg("aucpu_strm_remove pts fail ret:%d\n", ret);
		pout->aucpu_pts_handle = -1;

		_free_buff(pout->aucpu_pts_mem_phy,
				pout->aucpu_pts_mem_size, 0, 0);
		pout->aucpu_pts_mem = 0;
	}

	if (pout->pchan) {
		SC2_bufferid_set_enable(pout->pchan, 0);
		SC2_bufferid_dealloc(pout->pchan);
		pout->pchan = NULL;
	}
	if (pout->pchan1) {
		SC2_bufferid_set_enable(pout->pchan1, 0);
		SC2_bufferid_dealloc(pout->pchan1);
		pout->pchan1 = NULL;
	}

	pout->used = 0;
	pr_dbg("%s end\n", __func__);
	return 0;
}

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
		      int *cb_id)
{
	struct pid_entry *pid_slot = NULL;
	struct es_entry *es_pes = NULL;

	if (cb_id)
		*cb_id = 0;

	if (pout->pchan)
		SC2_bufferid_set_enable(pout->pchan, 1);
	if (pout->pchan1)
		SC2_bufferid_set_enable(pout->pchan1, 1);

	pr_dbg("%s pout:0x%lx pid:%d, pid_mask:%d\n",
	       __func__, (unsigned long)pout, pid, pid_mask);
	if (pout->format == ES_FORMAT ||
	    pout->format == PES_FORMAT || pout->format == SECTION_FORMAT) {
		es_pes = _malloc_es_entry_slot();
		if (!es_pes) {
			dprint("get es entry slot error\n");
			return -1;
		}
		es_pes->buff_id = pout->pchan->id;
		es_pes->pid = pid;
		es_pes->status = pout->format;
		es_pes->dmx_id = dmx_id;
		es_pes->pout = pout;
		pout->es_pes = es_pes;

		/*before pid filter enable */
		if (pout->pchan->sec_level)
			create_aucpu_inst(pout);
		if (pout->type == VIDEO_TYPE) {
			if (((dump_video_es & 0xFFFF)  == pout->es_pes->pid &&
			((dump_video_es >> 16) & 0xFFFF) == pout->sid) ||
				dump_video_es == 0XFFFFFFFF)
			dump_file_open(VIDEOES_DUMP_FILE, &pout->dump_file,
				pout->sid, pout->es_pes->pid, 0);
		}
		if (pout->type == AUDIO_TYPE) {
			if (((dump_audio_es & 0xFFFF)  == pout->es_pes->pid &&
			((dump_audio_es >> 16) & 0xFFFF) ==	pout->sid) ||
			   dump_audio_es == 0xFFFFFFFF)
			dump_file_open(AUDIOES_DUMP_FILE, &pout->dump_file,
				pout->sid, pout->es_pes->pid, 0);
		}
		tsout_config_es_table(es_pes->buff_id, es_pes->pid,
				      pout->sid, 1, !drop_dup, pout->format);
	} else {
		pid_slot = pout->pid_list;
		while (pid_slot) {
			if (pid_slot->pid == pid) {
				pid_slot->ref++;
				if (cb_id)
					*cb_id = dmx_id;
				if (pout->enable == 0)
					pr_err("!!!!!pout enable is 0, this is danger error!!!!!!\n");
				return 0;
			}
			pid_slot = pid_slot->pnext;
		}

		pid_slot = _malloc_pid_entry_slot(pout->sid, pid);
		if (!pid_slot) {
			pr_dbg("malloc pid entry fail\n");
			return -1;
		}
		pid_slot->pid = pid;
		pid_slot->pid_mask = pid_mask;
		pid_slot->used = 1;
		pid_slot->dmx_id = dmx_id;
		pid_slot->ref = 1;
		pid_slot->pout = pout;

		pid_slot->pnext = pout->pid_list;
		pout->pid_list = pid_slot;
		pr_dbg("sid:%d, pid:0x%0x, mask:0x%0x\n",
				pout->sid, pid_slot->pid, pid_slot->pid_mask);
		tsout_config_ts_table(pid_slot->pid, pid_slot->pid_mask,
				pid_slot->id, pout->pchan->id);
	}
	pout->enable = 1;
	return 0;
}

/**
 * remove pid in stream
 * \param pout
 * \param pid
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_output_remove_pid(struct out_elem *pout, int pid)
{
	struct pid_entry *cur_pid;
	struct pid_entry *prev_pid;

	pr_dbg("%s pout:0x%lx\n", __func__, (unsigned long)pout);
	if (pout->format == ES_FORMAT ||
		pout->format == PES_FORMAT || pout->format == SECTION_FORMAT) {
		if (pout->format != pout->es_pes->status) {
			pr_err("rm pid error pout fmt:%d es_pes fmt:%d\r\n",
			pout->format,
			pout->es_pes->status);
			return 0;
		} else if (pout->ref <= 1) {
			tsout_config_es_table(pout->es_pes->buff_id, -1,
					pout->sid, 1, !drop_dup, pout->format);
			_free_es_entry_slot(pout->es_pes);
//          pout->es_pes = NULL;
		} else {
			return 0;
		}
	} else if (pout->format == DVR_FORMAT) {
		cur_pid = pout->pid_list;
		prev_pid = cur_pid;
		while (cur_pid) {
			if (cur_pid->pid == pid) {
				if (cur_pid->ref > 1) {
					cur_pid->ref--;
					return 0;
				} else if (cur_pid->ref == 1) {
					if (cur_pid == pout->pid_list)
						pout->pid_list = cur_pid->pnext;
					else
						prev_pid->pnext =
						    cur_pid->pnext;
					break;
				}
			}
			prev_pid = cur_pid;
			cur_pid = cur_pid->pnext;
		}
		if (cur_pid) {
			tsout_config_ts_table(-1, cur_pid->pid_mask,
					      cur_pid->id, pout->pchan->id);
			_free_pid_entry_slot(cur_pid);
		}
		if (pout->pid_list)
			return 0;
	}
	pout->enable = 0;
	pr_dbg("%s line:%d\n", __func__, __LINE__);
	return 0;
}

int ts_output_set_mem(struct out_elem *pout, int memsize,
	int sec_level, int pts_memsize, int pts_level)
{
	pr_dbg("%s mem size:0x%0x, pts_memsize:0x%0x, sec_level:%d\n",
		__func__, memsize, pts_memsize, sec_level);

	if (pout && pout->pchan)
		SC2_bufferid_set_mem(pout->pchan, memsize, sec_level);

	if (pout && pout->pchan1)
		SC2_bufferid_set_mem(pout->pchan1, pts_memsize, pts_level);

	return 0;
}

int ts_output_set_sec_mem(struct out_elem *pout,
	unsigned int buf, unsigned int size)
{
	pr_dbg("%s size:0x%0x\n", __func__, size);

	if (pout && pout->pchan)
		SC2_bufferid_set_sec_mem(pout->pchan, buf, size);
	return 0;
}

int ts_output_get_mem_info(struct out_elem *pout,
			   unsigned int *total_size,
			   unsigned int *buf_phy_start,
			   unsigned int *free_size, unsigned int *wp_offset,
			   __u64 *newest_pts)
{
	*total_size = pout->pchan->mem_size;
	*buf_phy_start = pout->pchan->mem_phy;
	*wp_offset = SC2_bufferid_get_wp_offset(pout->pchan);
	*free_size = SC2_bufferid_get_free_size(pout->pchan);
	if (newest_pts)
		*newest_pts = pout->newest_pts;
	return 0;
}

/**
 * reset index pipeline, clear the buf
 * \param pout
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_output_reset(struct out_elem *pout)
{
	return 0;
}

/**
 * set callback for getting data
 * \param pout
 * \param cb
 * \param udata:private data
 * \param cb_id:cb_id
 * \param format:format
 * \param is_sec: is section callback
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_output_add_cb(struct out_elem *pout, ts_output_cb cb, void *udata,
		     u8 cb_id, u8 format, bool is_sec)
{
	struct cb_entry *tmp_cb = NULL;

	if (format == DVR_FORMAT) {
		tmp_cb = pout->cb_ts_list;
		while (tmp_cb) {
			if (tmp_cb->id == cb_id &&
				tmp_cb->format == DVR_FORMAT) {
				tmp_cb->ref++;
				if (tmp_cb->ref < MAX_FEED_NUM)
					tmp_cb->udata[tmp_cb->ref] = udata;
				return 0;
			}
			tmp_cb = tmp_cb->next;
		}
	}

	tmp_cb = vmalloc(sizeof(*tmp_cb));
	if (!tmp_cb) {
		dprint("%s malloc fail\n", __func__);
		return -1;
	}
	tmp_cb->cb = cb;
	tmp_cb->udata[0] = udata;
	tmp_cb->next = NULL;
	tmp_cb->format = format;
	tmp_cb->ref = 0;
	tmp_cb->id = cb_id;

	if (is_sec) {
		tmp_cb->next = pout->cb_sec_list;
		pout->cb_sec_list = tmp_cb;
	} else {
		if (pout->type == VIDEO_TYPE || pout->type == AUDIO_TYPE)
			mutex_lock(&es_output_mutex);

		tmp_cb->next = pout->cb_ts_list;
		pout->cb_ts_list = tmp_cb;

		if (pout->type == VIDEO_TYPE || pout->type == AUDIO_TYPE)
			mutex_unlock(&es_output_mutex);
	}

	pout->ref++;

	if (pout->type == VIDEO_TYPE || pout->type == AUDIO_TYPE)
		mod_timer(&es_out_task_tmp.out_timer,
		  jiffies + msecs_to_jiffies(out_es_flush_time));
	else
		mod_timer(&ts_out_task_tmp.out_timer,
		  jiffies + msecs_to_jiffies(out_flush_time));
	return 0;
}

static void remove_udata(struct cb_entry *tmp_cb, void *udata)
{
	int i, j;

	/*remove the free feed*/
	for (i = 0; i <= tmp_cb->ref && i < MAX_FEED_NUM; i++) {
		if (tmp_cb->udata[i] == udata) {
			for (j = i; j < tmp_cb->ref && j < MAX_FEED_NUM; j++)
				tmp_cb->udata[j] = tmp_cb->udata[j + 1];
		}
	}
}

/**
 * remove callback for getting data
 * \param pout
 * \param cb
 * \param udata:private data
 * \param cb_id:dmx_id
 * \param is_sec: is section callback
 * \retval 0:success.
 * \retval -1:fail.
 */
int ts_output_remove_cb(struct out_elem *pout, ts_output_cb cb, void *udata,
			u8 cb_id, bool is_sec)
{
	struct cb_entry *tmp_cb = NULL;
	struct cb_entry *pre_cb = NULL;

	if (pout->format == DVR_FORMAT) {
		tmp_cb = pout->cb_ts_list;
		while (tmp_cb) {
			if (tmp_cb->id == cb_id &&
				tmp_cb->format == DVR_FORMAT) {
				if (tmp_cb->ref == 0) {
					if (tmp_cb == pout->cb_ts_list)
						pout->cb_ts_list = tmp_cb->next;
					else
						pre_cb->next = tmp_cb->next;
					vfree(tmp_cb);
					pout->ref--;
					return 0;
				}
				remove_udata(tmp_cb, udata);
				tmp_cb->ref--;
				return 0;
			}
			pre_cb = tmp_cb;
			tmp_cb = tmp_cb->next;
		}
		return 0;
	}
	if (is_sec) {
		tmp_cb = pout->cb_sec_list;
		while (tmp_cb) {
			if (tmp_cb->cb == cb && tmp_cb->udata[0] == udata) {
				if (tmp_cb == pout->cb_sec_list)
					pout->cb_sec_list = tmp_cb->next;
				else
					pre_cb->next = tmp_cb->next;

				vfree(tmp_cb);
				pout->ref--;
				return 0;
			}
			pre_cb = tmp_cb;
			tmp_cb = tmp_cb->next;
		}
		if (!pout->cb_sec_list)
			pout->remain_len = 0;
	} else {
		if (pout->type == VIDEO_TYPE || pout->type == AUDIO_TYPE)
			mutex_lock(&es_output_mutex);

		tmp_cb = pout->cb_ts_list;
		while (tmp_cb) {
			if (tmp_cb->cb == cb && tmp_cb->udata[0] == udata) {
				if (tmp_cb == pout->cb_ts_list)
					pout->cb_ts_list = tmp_cb->next;
				else
					pre_cb->next = tmp_cb->next;

				vfree(tmp_cb);
				pout->ref--;
				if (pout->type == VIDEO_TYPE ||
				    pout->type == AUDIO_TYPE)
					mutex_unlock(&es_output_mutex);
				return 0;
			}
			pre_cb = tmp_cb;
			tmp_cb = tmp_cb->next;
		}
		if (pout->type == VIDEO_TYPE || pout->type == AUDIO_TYPE)
			mutex_unlock(&es_output_mutex);
	}
	return 0;
}

int ts_output_dump_info(char *buf)
{
	int i = 0;
	int count = 0;
	int r, total = 0;

	r = sprintf(buf, "********dvr********\n");
	buf += r;
	total += r;

	for (i = 0; i < MAX_OUT_ELEM_NUM; i++) {
		struct out_elem *pout = &out_elem_table[i];
		unsigned int total_size = 0;
		unsigned int buf_phy_start = 0;
		unsigned int free_size = 0;
		unsigned int wp_offset = 0;
		struct pid_entry *pid_list;

		if (pout->used && pout->format == DVR_FORMAT) {
			r = sprintf(buf, "%d sid:0x%0x ref:%d ",
				    count, pout->sid, pout->ref);
			buf += r;
			total += r;

			ts_output_get_mem_info(pout,
					       &total_size,
					       &buf_phy_start,
					       &free_size, &wp_offset, NULL);
			r = sprintf(buf,
				    "mem total:0x%0x, buf_base:0x%0x, ",
				    total_size, buf_phy_start);
			buf += r;
			total += r;

			r = sprintf(buf,
				    "free size:0x%0x, wp:0x%0x\n",
				    free_size, wp_offset);
			buf += r;
			total += r;

			pid_list = pout->pid_list;
			r = sprintf(buf, "    pid:");
			buf += r;
			total += r;

			while (pid_list) {
				r = sprintf(buf, "0x%0x ", pid_list->pid);
				buf += r;
				total += r;
				pid_list = pid_list->pnext;
			}
			r = sprintf(buf, "\n");
			buf += r;
			total += r;

			count++;
		}
	}

	r = sprintf(buf, "********PES********\n");
	buf += r;
	total += r;

	count = 0;
	for (i = 0; i < MAX_ES_NUM; i++) {
		struct es_entry *es_slot = &es_table[i];
		unsigned int total_size = 0;
		unsigned int buf_phy_start = 0;
		unsigned int free_size = 0;
		unsigned int wp_offset = 0;

		if (es_slot->used && es_slot->status == PES_FORMAT) {
			r = sprintf(buf, "%d dmx_id:%d sid:0x%0x type:%d,",
				count, es_slot->dmx_id,
				es_slot->pout->sid, es_slot->pout->type);
			buf += r;
			total += r;

			r = sprintf(buf, "pid:0x%0x ", es_slot->pid);
			buf += r;
			total += r;

			ts_output_get_mem_info(es_slot->pout,
					       &total_size,
					       &buf_phy_start,
					       &free_size, &wp_offset, NULL);
			r = sprintf(buf,
				    "mem total:0x%0x, buf_base:0x%0x, ",
				    total_size, buf_phy_start);
			buf += r;
			total += r;

			r = sprintf(buf,
				    "free size:0x%0x, wp:0x%0x\n",
				    free_size, wp_offset);
			buf += r;
			total += r;

			count++;
		}
	}

	r = sprintf(buf, "********ES********\n");
	buf += r;
	total += r;
	count = 0;

	for (i = 0; i < MAX_ES_NUM; i++) {
		struct es_entry *es_slot = &es_table[i];
		unsigned int total_size = 0;
		unsigned int buf_phy_start = 0;
		unsigned int free_size = 0;
		unsigned int wp_offset = 0;

		if (es_slot->used && es_slot->status == ES_FORMAT) {
			r = sprintf(buf, "%d dmx_id:%d sid:0x%0x type:%s",
				count, es_slot->dmx_id, es_slot->pout->sid,
				    (es_slot->pout->type == AUDIO_TYPE) ?
				    "aud" : "vid");
			buf += r;
			total += r;

			r = sprintf(buf, " pid:0x%0x ", es_slot->pid);
			buf += r;
			total += r;

			ts_output_get_mem_info(es_slot->pout,
					       &total_size,
					       &buf_phy_start,
					       &free_size, &wp_offset, NULL);

			r = sprintf(buf,
				    "mem total:0x%0x, buf_base:0x%0x, ",
				    total_size, buf_phy_start);
			buf += r;
			total += r;

			r = sprintf(buf,
				    "free size:0x%0x, wp:0x%0x, ",
				    free_size, wp_offset);
			buf += r;
			total += r;

			r = sprintf(buf,
				    "h rp:0x%0x, h wp:0x%0x, ",
				    es_slot->pout->pchan1->r_offset,
					SC2_bufferid_get_wp_offset(
						es_slot->pout->pchan1));
			buf += r;
			total += r;

			r = sprintf(buf,
				    "sec_level:0x%0x\n",
				    es_slot->pout->pchan->sec_level);
			buf += r;
			total += r;

			count++;
		}
	}
	r = sprintf(buf, "********section********\n");
	buf += r;
	total += r;
	count = 0;

	for (i = 0; i < MAX_ES_NUM; i++) {
		struct es_entry *es_slot = &es_table[i];
		unsigned int total_size = 0;
		unsigned int buf_phy_start = 0;
		unsigned int free_size = 0;
		unsigned int wp_offset = 0;

		if (es_slot->used && es_slot->status == SECTION_FORMAT) {
			r = sprintf(buf, "%d dmxid:%d sid:0x%0x ",
				    count, es_slot->dmx_id, es_slot->pout->sid);
			buf += r;
			total += r;

			r = sprintf(buf, "pid:0x%0x ref:%d ",
				    es_slot->pid, es_slot->pout->ref);
			buf += r;
			total += r;

			ts_output_get_mem_info(es_slot->pout,
					       &total_size,
					       &buf_phy_start,
					       &free_size, &wp_offset, NULL);

			r = sprintf(buf,
				    "mem total:0x%0x, buf_base:0x%0x, ",
				    total_size, buf_phy_start);
			buf += r;
			total += r;

			r = sprintf(buf,
				    "free size:0x%0x, wp:0x%0x\n",
				    free_size, wp_offset);
			buf += r;
			total += r;

			count++;
		}
	}

	return total;
}
