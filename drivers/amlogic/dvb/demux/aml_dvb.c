/*
 * drivers/amlogic/dvb/demux/aml_dvb.c
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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>

#include <linux/wait.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/of.h>
#include <linux/dvb/dmx.h>
#include <linux/amlogic/tee.h>
#include <linux/amlogic/aml_key.h>

#include "aml_dvb.h"
#include "am_key.h"
#include "dmx_log.h"
#include "sc2_demux/ts_output.h"
#include "sc2_demux/frontend.h"
#include "sc2_demux/dvb_reg.h"

#define dprint_i(fmt, args...)  \
	dprintk(LOG_ERROR, debug_dvb, fmt, ## args)
#define dprint(fmt, args...)   \
	dprintk(LOG_ERROR, debug_dvb, fmt, ## args)
#define pr_dbg(fmt, args...)   \
	dprintk(LOG_DBG, debug_dvb, "dvb:" fmt, ## args)

MODULE_PARM_DESC(debug_dvb, "\n\t\t Enable demux debug information");
static int debug_dvb = 1;
module_param(debug_dvb, int, 0644);

#define CARD_NAME "amlogic-dvb"

DVB_DEFINE_MOD_OPT_ADAPTER_NR(adapter_nr);

static struct aml_dvb aml_dvb_device;
static int dmx_dev_num;
static int tsn_in;
static int tsn_out;

#define MAX_DMX_DEV_NUM      32
static int sid_info[MAX_DMX_DEV_NUM];
#define DEFAULT_DMX_DEV_NUM  3

ssize_t get_pcr_show(struct class *class,
		     struct class_attribute *attr, char *buf)
{
	struct aml_dvb *dvb = aml_get_dvb_device();
	int r, total = 0;
	int i;
	u64 stc;
	unsigned int base;

	for (i = 0; i < 4; i++) {
		stc = 0;
		base = 0;
		dmx_get_stc(&dvb->dmx[0].dmx, i, &stc, &base);

		r = sprintf(buf, "num%d stc:0x%llx base:%d\n", i, stc, base);
		buf += r;
		total += r;
	}

	return total;
}

ssize_t dmx_setting_show(struct class *class, struct class_attribute *attr,
			 char *buf)
{
	int r, total = 0;
	int i;
	struct aml_dvb *dvb = aml_get_dvb_device();

	for (i = 0; i < dmx_dev_num; i++) {
		r = sprintf(buf, "dmx%d source:%s ", i,
			    dvb->dmx[i].source == INPUT_DEMOD ? "input_demod" :
			    (dvb->dmx[i].source == INPUT_LOCAL ?
			     "input_local" : "input_local_sec"));
		buf += r;
		total += r;
		r = sprintf(buf, "demod_sid:0x%0x local_sid:0x%0x\n",
			    dvb->dmx[i].demod_sid, dvb->dmx[i].local_sid);
		buf += r;
		total += r;
	}

	return total;
}

ssize_t dsc_setting_show(struct class *class, struct class_attribute *attr,
			 char *buf)
{
	int total = 0;

	total = dsc_dump_info(buf);

	return total;
}

int demux_get_stc(int demux_device_index, int index,
		  u64 *stc, unsigned int *base)
{
	struct aml_dvb *dvb = aml_get_dvb_device();

	if (demux_device_index >= DMX_DEV_COUNT)
		return -1;

	dmx_get_stc(&dvb->dmx[demux_device_index].dmx, index, stc, base);

	return 0;
}

EXPORT_SYMBOL(demux_get_stc);

int demux_get_pcr(int demux_device_index, int index, u64 *pcr)
{
	struct aml_dvb *dvb = aml_get_dvb_device();

	if (demux_device_index >= DMX_DEV_COUNT)
		return -1;

	return dmx_get_pcr(&dvb->dmx[demux_device_index].dmx, index, pcr);
}
EXPORT_SYMBOL(demux_get_pcr);

ssize_t tsn_source_show(struct class *class,
			struct class_attribute *attr, char *buf)
{
	int r, total = 0;

	if (tsn_in == INPUT_DEMOD)
		r = sprintf(buf, "tsn_source:demod\n");
	else if (tsn_in == INPUT_LOCAL)
		r = sprintf(buf, "tsn_source:local\n");
	else if (tsn_in == INPUT_LOCAL_SEC)
		r = sprintf(buf, "tsn_source:local_sec\n");
	else
		return 0;

	buf += r;
	total += r;
	return total;
}

ssize_t tsn_source_store(struct class *class,
			 struct class_attribute *attr,
			 const char *buf, size_t count)
{
	struct aml_dvb *advb = aml_get_dvb_device();
	int tsn_in_reg = 0;
	int i = 0;

	if (!strncmp(buf, "demod", 5))
		tsn_in = INPUT_DEMOD;
	else if (!strncmp(buf, "local", 5))
		tsn_in = INPUT_LOCAL;
	else if (!strncmp(buf, "local_sec", 9))
		tsn_in = INPUT_LOCAL_SEC;
	else
		return count;

	if (mutex_lock_interruptible(&advb->mutex))
		return -ERESTARTSYS;

	if (tsn_in == INPUT_DEMOD)
		tsn_in_reg = 1;

//	pr_dbg("tsn_in:%d, tsn_out:%d\n", tsn_in_reg, tsn_out);
	advb->dsc_pipeline = tsn_in_reg;
	//set demod/local
	tee_demux_config_pipeline(tsn_in_reg, tsn_out);

	for (i = 0; i < dmx_dev_num; i++) {
		advb->dmx[i].source = tsn_in;
		advb->dsc[i].source = tsn_in;
	}

	mutex_unlock(&advb->mutex);
	return count;
}

static struct class_attribute aml_dvb_class_attrs[] = {
	__ATTR(ts_setting, 0664, ts_setting_show, ts_setting_store),
	__ATTR(get_pcr, 0664, get_pcr_show, NULL),
	__ATTR(dmx_setting, 0664, dmx_setting_show, NULL),
	__ATTR(dsc_setting, 0664, dsc_setting_show, NULL),
	__ATTR(tsn_source, 0664, tsn_source_show, tsn_source_store),
	__ATTR_NULL
};

static struct class aml_dvb_class = {
	.name = "stb",
	.class_attrs = aml_dvb_class_attrs,
};

int dmx_get_dev_num(struct platform_device *pdev)
{
	char buf[32];
	u32 dmxdev = 0;
	int ret = 0;

	memset(buf, 0, 32);
	snprintf(buf, sizeof(buf), "dmxdev_num");
	ret = of_property_read_u32(pdev->dev.of_node, buf, &dmxdev);
	if (!ret)
		dprint("%s: 0x%x\n", buf, dmxdev);
	else
		dmxdev = DEFAULT_DMX_DEV_NUM;

	return dmxdev;
}

int dmx_get_tsn_flag(struct platform_device *pdev, int *tsn_in, int *tsn_out)
{
	char buf[32];
	u32 source = 0;
	int ret = 0;
	const char *str;

	*tsn_out = 0;

	source = 0;
	memset(buf, 0, 32);
	snprintf(buf, sizeof(buf), "tsn_from");
	ret = of_property_read_string(pdev->dev.of_node, buf, &str);
	if (!ret) {
		dprint("%s:%s\n", buf, str);
		if (!strcmp(str, "demod"))
			source = INPUT_DEMOD;
		else if (!strcmp(str, "local"))
			source = INPUT_LOCAL;
		else
			source = INPUT_LOCAL_SEC;
	} else {
		source = INPUT_DEMOD;
	}

	*tsn_in = source;
	return 0;
}

struct aml_dvb *aml_get_dvb_device(void)
{
	return &aml_dvb_device;
}

EXPORT_SYMBOL(aml_get_dvb_device);

struct dvb_adapter *aml_get_dvb_adapter(void)
{
	return &aml_dvb_device.dvb_adapter;
}

EXPORT_SYMBOL(aml_get_dvb_adapter);

struct device *aml_get_device(void)
{
	return aml_dvb_device.dev;
}

static int aml_dvb_remove(struct platform_device *pdev)
{
	struct aml_dvb *advb;
	int i;

	advb = &aml_dvb_device;

	for (i = 0; i < dmx_dev_num; i++) {
		if (advb->tsp[i])
			swdmx_ts_parser_free(advb->tsp[i]);
		if (advb->swdmx[i])
			swdmx_demux_free(advb->swdmx[i]);
	}

	for (i = 0; i < dmx_dev_num; i++) {
		if (advb->dmx[i].init) {
			dmx_destroy(&advb->dmx[i]);
			advb->dmx[i].id = -1;
		}
	}

	for (i = 0; i < dmx_dev_num; i++) {
		dsc_release(&advb->dsc[i]);
		advb->dsc[i].id = -1;
	}
	dmx_key_exit();

	ts_output_destroy();

	mutex_destroy(&advb->mutex);
	dvb_unregister_adapter(&advb->dvb_adapter);
	class_unregister(&aml_dvb_class);
	dmx_unregist_dmx_class();

	return 0;
}

static int get_all_sid_info(int dmx_dev_num, struct aml_dvb *advb)
{
	int i = 0;
	int j = 0;
	int count = 0;

	for (i = 0; i < dmx_dev_num; i++)
		sid_info[i] = i;

	count = i;

	for (j = 0; j < FE_DEV_COUNT; j++) {
		if (advb->ts[j].ts_sid != -1) {
			sid_info[count] = advb->ts[j].ts_sid;
			count++;
		}
	}
	return count;
}

static int get_first_valid_ts(struct aml_dvb *advb)
{
	int i = 0;

	for (i = 0; i < FE_DEV_COUNT; i++) {
		if (advb->ts[i].ts_sid != -1)
			return i;
	}
	return 0;
}

static int aml_dvb_probe(struct platform_device *pdev)
{
	struct aml_dvb *advb;
	int i, ret = 0;
	int tsn_in_reg = 0;
	int sid_num = 0;
	int valid_ts = 0;

	dprint("probe amlogic dvb driver\n");

	advb = &aml_dvb_device;
	memset(advb, 0, sizeof(aml_dvb_device));

	advb->dev = &pdev->dev;
	advb->pdev = pdev;

	ret = dvb_register_adapter(&advb->dvb_adapter, CARD_NAME, THIS_MODULE,
				   advb->dev, adapter_nr);
	if (ret < 0)
		return ret;

	mutex_init(&advb->mutex);
	spin_lock_init(&advb->slock);

	ret = init_demux_addr(pdev);
	if (ret != 0)
		return ret;

	frontend_probe(pdev);
	dmx_dev_num = dmx_get_dev_num(pdev);

	dmx_get_tsn_flag(pdev, &tsn_in, &tsn_out);

	if (tsn_in == INPUT_DEMOD)
		tsn_in_reg = 1;

	pr_dbg("tsn_in:%d, tsn_out:%d\n", tsn_in_reg, tsn_out);
	advb->dsc_pipeline = tsn_in_reg;
	//set demod/local
	tee_demux_config_pipeline(tsn_in_reg, tsn_out);

	sid_num  = get_all_sid_info(dmx_dev_num, advb);
	dmx_init_hw(sid_num, (int *)&sid_info);

	valid_ts = get_first_valid_ts(advb);
	//create dmx dev
	for (i = 0; i < dmx_dev_num; i++) {
		advb->swdmx[i] = swdmx_demux_new();
		if (!advb->swdmx[i])
			goto INIT_ERR;

		advb->tsp[i] = swdmx_ts_parser_new();
		if (!advb->tsp[i])
			goto INIT_ERR;

		swdmx_ts_parser_add_ts_packet_cb(advb->tsp[i],
						 swdmx_demux_ts_packet_cb,
						 advb->swdmx[i]);

		advb->dmx[i].id = i;
		advb->dmx[i].pmutex = &advb->mutex;
		advb->dmx[i].pslock = &advb->slock;
		advb->dmx[i].swdmx = advb->swdmx[i];
		advb->dmx[i].tsp = advb->tsp[i];
		advb->dmx[i].source = tsn_in;
		advb->dmx[i].ts_index = valid_ts;
		advb->dmx[i].demod_sid = advb->ts[advb->dmx[i].ts_index].ts_sid;
		advb->dmx[i].local_sid = i;
		ret = dmx_init(&advb->dmx[i], &advb->dvb_adapter);
		if (ret)
			goto INIT_ERR;

		advb->dsc[i].mutex = advb->mutex;
//              advb->dsc[i].slock = advb->slock;
		advb->dsc[i].id = i;
		advb->dsc[i].source = tsn_in;
		advb->dsc[i].demod_sid = advb->dmx[i].demod_sid;
		advb->dsc[i].local_sid = i;

		ret = dsc_init(&advb->dsc[i], &advb->dvb_adapter);
		if (ret)
			goto INIT_ERR;
	}
	frontend_config_ts_sid();
	dmx_key_init();

	class_register(&aml_dvb_class);
	dmx_regist_dmx_class();

	dprint("probe dvb done\n");

	return 0;

INIT_ERR:
	aml_dvb_remove(pdev);

	return -1;
}

#ifdef CONFIG_OF
static const struct of_device_id aml_dvb_dt_match[] = {
	{
	 .compatible = "amlogic sc2, dvb-demux",
	 },
	{}
};
#endif /*CONFIG_OF */

struct platform_driver aml_dvb_driver = {
	.probe = aml_dvb_probe,
	.remove = aml_dvb_remove,
	.suspend = NULL,
	.resume = NULL,
	.driver = {
		   .name = "amlogic-dvb-demux",
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = aml_dvb_dt_match,
#endif
	}
};

static int __init aml_dvb_init(void)
{
	dprint("aml dvb init\n");
	return platform_driver_register(&aml_dvb_driver);
}

static void __exit aml_dvb_exit(void)
{
	dprint("aml dvb exit\n");
	platform_driver_unregister(&aml_dvb_driver);
}

module_init(aml_dvb_init);
module_exit(aml_dvb_exit);

MODULE_DESCRIPTION("driver for the AMLogic DVB card");
MODULE_AUTHOR("AMLOGIC");
MODULE_LICENSE("GPL");
