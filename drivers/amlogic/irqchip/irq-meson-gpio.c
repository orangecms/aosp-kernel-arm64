/*
 * Copyright (c) 2015 Endless Mobile, Inc.
 * Author: Carlo Caione <carlo@endlessm.com>
 * Copyright (c) 2016 BayLibre, SAS.
 * Author: Jerome Brunet <jbrunet@baylibre.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * The full GNU General Public License is included in this distribution
 * in the file called COPYING.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/io.h>
#include <linux/module.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/irqchip.h>
#include <linux/of.h>
#include <linux/of_address.h>

#define NUM_CHANNEL 8
#define MAX_INPUT_MUX 256

#define REG_EDGE_POL	0x00
#define REG_PIN_03_SEL	0x04
#define REG_PIN_47_SEL	0x08
#define REG_FILTER_SEL	0x0c

#define REG_EDGE_POL_MASK(x)	(BIT(x) | BIT(16 + (x)))
#define REG_EDGE_POL_EDGE(x)	BIT(x)
#define REG_EDGE_POL_LOW(x)	BIT(16 + (x))
#define REG_EDGE_BOTH_EDGE(x)	BIT(8 + (x))
#define REG_PIN_SEL_SHIFT(x)	(((x) % 4) * 8)
#define REG_FILTER_SEL_SHIFT(x)	((x) * 4)

/* for sc2 */
#define REG_PIN_SC2_SEL			0x04
#define REG_EDGE_POL_EXTR		0x1c
#define REG_EDGE_POL_MASK_SC2(x)			\
({typeof(x) _x = (x); BIT(_x) | BIT(12 + (_x)); })

struct meson_gpio_irq_controller;

static unsigned int
meson_sc2_gpio_irq_sel_pin(struct meson_gpio_irq_controller *ctl,
			   unsigned int channel, unsigned long hwirq);
static unsigned int
meson_sc2_gpio_irq_sel_type(struct meson_gpio_irq_controller *ctl,
			    unsigned int idx, u32 val);
static void meson_sc2_gpio_irq_init(struct meson_gpio_irq_controller *ctl);

struct irq_ctl_ops {
	unsigned int (*gpio_irq_sel_pin)(struct meson_gpio_irq_controller *ctl,
					 unsigned int channel,
					 unsigned long hwirq);
	unsigned int (*gpio_irq_sel_type)(struct meson_gpio_irq_controller *ctl,
					  unsigned int idx, u32 val);
	void (*gpio_irq_init)(struct meson_gpio_irq_controller *ctl);
};

struct meson_gpio_irq_params {
	unsigned int nr_hwirq;
	u8 channel_num;
	u8 support_double_edge;
	struct irq_ctl_ops ops;
};

static const struct meson_gpio_irq_params meson8_params = {
	.nr_hwirq = 134,
	.channel_num = 8,
};

static const struct meson_gpio_irq_params meson8b_params = {
	.nr_hwirq = 119,
	.channel_num = 8,
};

static const struct meson_gpio_irq_params gxbb_params = {
	.nr_hwirq = 133,
	.channel_num = 8,
};

static const struct meson_gpio_irq_params gxl_params = {
	.nr_hwirq = 110,
	.channel_num = 8,
};

static const struct meson_gpio_irq_params axg_params = {
	.nr_hwirq = 100,
	.channel_num = 8,
};

static const struct meson_gpio_irq_params txlx_params = {
	.nr_hwirq = 119,
	.channel_num = 8,
};

static const struct meson_gpio_irq_params g12a_params = {
	.nr_hwirq = 100,
	.channel_num = 8,
};

static const struct meson_gpio_irq_params txl_params = {
	.nr_hwirq = 93,
	.channel_num = 8,
};

static const struct meson_gpio_irq_params tl1_params = {
	.nr_hwirq = 102,
	.channel_num = 8,
};

static const struct meson_gpio_irq_params sm1_params = {
	.nr_hwirq = 100,
	.support_double_edge = 1,
	.channel_num = 8,
};

static const struct meson_gpio_irq_params tm2_params = {
	.nr_hwirq = 104,
	.support_double_edge = 1,
	.channel_num = 8,
};

static const struct meson_gpio_irq_params sc2_params = {
	.nr_hwirq = 87,
	.support_double_edge = 1,
	.channel_num = 12,
	.ops = {
		.gpio_irq_sel_pin = meson_sc2_gpio_irq_sel_pin,
		.gpio_irq_init = meson_sc2_gpio_irq_init,
		.gpio_irq_sel_type = meson_sc2_gpio_irq_sel_type,
	},
};

static const struct of_device_id meson_irq_gpio_matches[] = {
	{ .compatible = "amlogic,meson8-gpio-intc", .data = &meson8_params },
	{ .compatible = "amlogic,meson8b-gpio-intc", .data = &meson8b_params },
	{ .compatible = "amlogic,meson-gxbb-gpio-intc", .data = &gxbb_params },
	{ .compatible = "amlogic,meson-gxl-gpio-intc", .data = &gxl_params },
	{ .compatible = "amlogic,meson-axg-gpio-intc", .data = &axg_params },
	{ .compatible = "amlogic,meson-txlx-gpio-intc", .data = &txlx_params },
	{ .compatible = "amlogic,meson-g12a-gpio-intc", .data = &g12a_params },
	{ .compatible = "amlogic,meson-txl-gpio-intc", .data = &txl_params },
	{ .compatible = "amlogic,meson-tl1-gpio-intc", .data = &tl1_params },
	{ .compatible = "amlogic,meson-sm1-gpio-intc", .data = &sm1_params },
	{ .compatible = "amlogic,meson-tm2-gpio-intc", .data = &tm2_params },
	{ .compatible = "amlogic,meson-sc2-gpio-intc", .data = &sc2_params },
	{ }
};

struct meson_gpio_irq_controller {
	unsigned int nr_hwirq;
	u8 support_double_edge;
	void __iomem *base;
	u32 *channel_irqs;
	u8 channel_num;
	unsigned long *channel_map;
	const struct irq_ctl_ops *ops;
	spinlock_t lock;
};

static void meson_gpio_irq_update_bits(struct meson_gpio_irq_controller *ctl,
				       unsigned int reg, u32 mask, u32 val)
{
	u32 tmp;

	tmp = readl_relaxed(ctl->base + reg);
	tmp &= ~mask;
	tmp |= val;
	writel_relaxed(tmp, ctl->base + reg);
}

static unsigned int
meson_sc2_gpio_irq_sel_pin(struct meson_gpio_irq_controller *ctl,
			   unsigned int channel, unsigned long hwirq)
{
	unsigned int reg_offset;
	unsigned int bit_offset;

	bit_offset = ((channel % 2) == 0) ? 0 : 16;
	reg_offset = REG_PIN_SC2_SEL + ((channel / 2) << 2);

	meson_gpio_irq_update_bits(ctl, reg_offset,
				   0x7f << bit_offset,
				   hwirq << bit_offset);
	return 0;
}

static unsigned int
meson_sc2_gpio_irq_sel_type(struct meson_gpio_irq_controller *ctl,
			    unsigned int idx, unsigned int type)

{
	unsigned int val = 0;
	unsigned long flags;

	/* clear double edge */
	meson_gpio_irq_update_bits(ctl, REG_EDGE_POL_EXTR, BIT(0 + (idx)), 0);

	if (type == IRQ_TYPE_EDGE_BOTH) {
		val |= BIT(0 + (idx));
		spin_lock_irqsave(&ctl->lock, flags);
		meson_gpio_irq_update_bits(ctl, REG_EDGE_POL_EXTR,
					   BIT(0 +  (idx)),
					   val);
		spin_unlock_irqrestore(&ctl->lock, flags);
		return 0;
	}

	spin_lock_irqsave(&ctl->lock, flags);

	if (type & (IRQ_TYPE_LEVEL_LOW | IRQ_TYPE_EDGE_FALLING))
		val |= BIT(0 + (idx));

	if (type & (IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING))
		val |= BIT(12 + (idx));

	meson_gpio_irq_update_bits(ctl, REG_EDGE_POL,
				   REG_EDGE_POL_MASK_SC2(idx), val);

	spin_unlock_irqrestore(&ctl->lock, flags);

	return 0;
}

static void meson_sc2_gpio_irq_init(struct meson_gpio_irq_controller *ctl)
{
	meson_gpio_irq_update_bits(ctl, REG_EDGE_POL, BIT(31), BIT(31));
}
static unsigned int meson_gpio_irq_channel_to_reg(unsigned int channel)
{
	return (channel < 4) ? REG_PIN_03_SEL : REG_PIN_47_SEL;
}

static int
meson_gpio_irq_request_channel(struct meson_gpio_irq_controller *ctl,
			       unsigned long  hwirq,
			       u32 **channel_hwirq)
{
	unsigned int reg, idx;
	unsigned long flags;

	spin_lock_irqsave(&ctl->lock, flags);

	/* Find a free channel */
	idx = find_first_zero_bit(ctl->channel_map, ctl->channel_num);
	if (idx >= ctl->channel_num) {
		spin_unlock_irqrestore(&ctl->lock, flags);
		pr_debug("No channel available\n");
		return -ENOSPC;
	}

	/* Mark the channel as used */
	set_bit(idx, ctl->channel_map);

	/*
	 * Setup the mux of the channel to route the signal of the pad
	 * to the appropriate input of the GIC
	 */
	if (ctl->ops->gpio_irq_sel_pin) {
		reg = ctl->ops->gpio_irq_sel_pin(ctl, idx, hwirq);
	} else {
		reg = meson_gpio_irq_channel_to_reg(idx);
		meson_gpio_irq_update_bits(ctl, reg,
					   0xff << REG_PIN_SEL_SHIFT(idx),
					   hwirq << REG_PIN_SEL_SHIFT(idx));
	}

	/*
	 * Get the hwirq number assigned to this channel through
	 * a pointer the channel_irq table. The added benifit of this
	 * method is that we can also retrieve the channel index with
	 * it, using the table base.
	 */
	*channel_hwirq = &(ctl->channel_irqs[idx]);

	spin_unlock_irqrestore(&ctl->lock, flags);

	pr_debug("hwirq %lu assigned to channel %d - irq %u\n",
		 hwirq, idx, **channel_hwirq);

	return 0;
}

static unsigned int
meson_gpio_irq_get_channel_idx(struct meson_gpio_irq_controller *ctl,
			       u32 *channel_hwirq)
{
	return channel_hwirq - ctl->channel_irqs;
}

static void
meson_gpio_irq_release_channel(struct meson_gpio_irq_controller *ctl,
			       u32 *channel_hwirq)
{
	unsigned int idx;

	idx = meson_gpio_irq_get_channel_idx(ctl, channel_hwirq);
	clear_bit(idx, ctl->channel_map);
}

static int meson_gpio_irq_type_setup(struct meson_gpio_irq_controller *ctl,
				     unsigned int type,
				     u32 *channel_hwirq)
{
	u32 val = 0;
	unsigned int idx;
	unsigned long flags;

	idx = meson_gpio_irq_get_channel_idx(ctl, channel_hwirq);

	/*
	 * The controller has a filter block to operate in either LEVEL or
	 * EDGE mode, then signal is sent to the GIC. To enable LEVEL_LOW and
	 * EDGE_FALLING support (which the GIC does not support), the filter
	 * block is also able to invert the input signal it gets before
	 * providing it to the GIC.
	 */
	type &= IRQ_TYPE_SENSE_MASK;

	if (ctl->ops->gpio_irq_sel_type)
		return ctl->ops->gpio_irq_sel_type(ctl, idx, type);

	if (type == IRQ_TYPE_EDGE_BOTH) {
		if (!ctl->support_double_edge)
			return -EINVAL;
		val |= REG_EDGE_BOTH_EDGE(idx);
		spin_lock_irqsave(&ctl->lock, flags);
		meson_gpio_irq_update_bits(ctl, REG_EDGE_POL,
				   REG_EDGE_BOTH_EDGE(idx), val);
		spin_unlock_irqrestore(&ctl->lock, flags);
		return 0;
	}

	if (type & (IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING))
		val |= REG_EDGE_POL_EDGE(idx);

	if (type & (IRQ_TYPE_LEVEL_LOW | IRQ_TYPE_EDGE_FALLING))
		val |= REG_EDGE_POL_LOW(idx);

	spin_lock_irqsave(&ctl->lock, flags);

	/* Double-edge has priority over all others. If a double-edge gpio
	 * changes to another method's, we need to reset the corresponding bit
	 * of double-edge register.
	 */
	if (ctl->support_double_edge)
		meson_gpio_irq_update_bits(ctl, REG_EDGE_POL,
					   REG_EDGE_BOTH_EDGE(idx), 0);

	meson_gpio_irq_update_bits(ctl, REG_EDGE_POL,
				   REG_EDGE_POL_MASK(idx), val);

	spin_unlock_irqrestore(&ctl->lock, flags);

	return 0;
}

static unsigned int meson_gpio_irq_type_output(unsigned int type)
{
	unsigned int sense = type & IRQ_TYPE_SENSE_MASK;

	type &= ~IRQ_TYPE_SENSE_MASK;

	/*
	 * The polarity of the signal provided to the GIC should always
	 * be high.
	 */
	if (sense & (IRQ_TYPE_LEVEL_HIGH | IRQ_TYPE_LEVEL_LOW))
		type |= IRQ_TYPE_LEVEL_HIGH;
	else if (sense & (IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING))
		type |= IRQ_TYPE_EDGE_RISING;

	return type;
}

static int meson_gpio_irq_set_type(struct irq_data *data, unsigned int type)
{
	struct meson_gpio_irq_controller *ctl = data->domain->host_data;
	u32 *channel_hwirq = irq_data_get_irq_chip_data(data);
	int ret;

	ret = meson_gpio_irq_type_setup(ctl, type, channel_hwirq);
	if (ret)
		return ret;

	return irq_chip_set_type_parent(data,
					meson_gpio_irq_type_output(type));
}

static struct irq_chip meson_gpio_irq_chip = {
	.name			= "meson-gpio-irqchip",
	.irq_mask		= irq_chip_mask_parent,
	.irq_unmask		= irq_chip_unmask_parent,
	.irq_eoi		= irq_chip_eoi_parent,
	.irq_set_type		= meson_gpio_irq_set_type,
	.irq_retrigger		= irq_chip_retrigger_hierarchy,
#ifdef CONFIG_SMP
	.irq_set_affinity	= irq_chip_set_affinity_parent,
#endif
	.flags = IRQCHIP_SET_TYPE_MASKED | IRQCHIP_SKIP_SET_WAKE,
};

static int meson_gpio_irq_domain_translate(struct irq_domain *domain,
					   struct irq_fwspec *fwspec,
					   unsigned long *hwirq,
					   unsigned int *type)
{
	if (is_of_node(fwspec->fwnode) && fwspec->param_count == 2) {
		*hwirq	= fwspec->param[0];
		*type	= fwspec->param[1];
		return 0;
	}

	return -EINVAL;
}

static int meson_gpio_irq_allocate_gic_irq(struct irq_domain *domain,
					   unsigned int virq,
					   u32 hwirq,
					   unsigned int type)
{
	struct irq_fwspec fwspec;

	fwspec.fwnode = domain->parent->fwnode;
	fwspec.param_count = 3;
	fwspec.param[0] = 0;	/* SPI */
	fwspec.param[1] = hwirq;
	fwspec.param[2] = meson_gpio_irq_type_output(type);

	return irq_domain_alloc_irqs_parent(domain, virq, 1, &fwspec);
}

static int meson_gpio_irq_domain_alloc(struct irq_domain *domain,
				       unsigned int virq,
				       unsigned int nr_irqs,
				       void *data)
{
	struct irq_fwspec *fwspec = data;
	struct meson_gpio_irq_controller *ctl = domain->host_data;
	unsigned long hwirq;
	u32 *channel_hwirq;
	unsigned int type;
	int ret;

	if (WARN_ON(nr_irqs != 1))
		return -EINVAL;

	ret = meson_gpio_irq_domain_translate(domain, fwspec, &hwirq, &type);
	if (ret)
		return ret;

	ret = meson_gpio_irq_request_channel(ctl, hwirq, &channel_hwirq);
	if (ret)
		return ret;

	ret = meson_gpio_irq_allocate_gic_irq(domain, virq,
					      *channel_hwirq, type);
	if (ret < 0) {
		pr_err("failed to allocate gic irq %u\n", *channel_hwirq);
		meson_gpio_irq_release_channel(ctl, channel_hwirq);
		return ret;
	}

	irq_domain_set_hwirq_and_chip(domain, virq, hwirq,
				      &meson_gpio_irq_chip, channel_hwirq);

	return 0;
}

static void meson_gpio_irq_domain_free(struct irq_domain *domain,
				       unsigned int virq,
				       unsigned int nr_irqs)
{
	struct meson_gpio_irq_controller *ctl = domain->host_data;
	struct irq_data *irq_data;
	u32 *channel_hwirq;

	if (WARN_ON(nr_irqs != 1))
		return;

	irq_domain_free_irqs_parent(domain, virq, 1);

	irq_data = irq_domain_get_irq_data(domain, virq);
	channel_hwirq = irq_data_get_irq_chip_data(irq_data);

	meson_gpio_irq_release_channel(ctl, channel_hwirq);
}

static const struct irq_domain_ops meson_gpio_irq_domain_ops = {
	.alloc		= meson_gpio_irq_domain_alloc,
	.free		= meson_gpio_irq_domain_free,
	.translate	= meson_gpio_irq_domain_translate,
};

static unsigned long *bitmap_alloc(unsigned int nbits, gfp_t flags)
{
	return kmalloc_array(BITS_TO_LONGS(nbits), sizeof(unsigned long),
			     flags);
}

static int __init meson_gpio_irq_parse_dt(struct device_node *node,
					  struct meson_gpio_irq_controller *ctl)
{
	const struct of_device_id *match;
	const struct meson_gpio_irq_params *params;
	int ret;

	match = of_match_node(meson_irq_gpio_matches, node);
	if (!match)
		return -ENODEV;

	params = match->data;
	ctl->nr_hwirq = params->nr_hwirq;
	ctl->support_double_edge = params->support_double_edge;
	ctl->ops = &params->ops;
	ctl->channel_num = params->channel_num;
	ctl->channel_irqs = kcalloc(ctl->channel_num,
				    sizeof(*ctl->channel_irqs), GFP_KERNEL);
	ctl->channel_map = bitmap_alloc(ctl->channel_num, GFP_KERNEL
					| __GFP_ZERO);

	if (!ctl->channel_irqs || !ctl->channel_map) {
		pr_err("request mem for irqchip failed\n");
		return -ENOMEM;
	}

	ret = of_property_read_variable_u32_array(node,
						  "amlogic,channel-interrupts",
						  ctl->channel_irqs,
						  ctl->channel_num,
						  ctl->channel_num);
	if (ret < 0) {
		pr_err("can't get %d channel interrupts\n", NUM_CHANNEL);
		return ret;
	}

	return 0;
}

static int __init meson_gpio_irq_of_init(struct device_node *node,
					 struct device_node *parent)
{
	struct irq_domain *domain, *parent_domain;
	struct meson_gpio_irq_controller *ctl;
	int ret;

	if (!parent) {
		pr_err("missing parent interrupt node\n");
		return -ENODEV;
	}

	parent_domain = irq_find_host(parent);
	if (!parent_domain) {
		pr_err("unable to obtain parent domain\n");
		return -ENXIO;
	}

	ctl = kzalloc(sizeof(*ctl), GFP_KERNEL);
	if (!ctl)
		return -ENOMEM;

	spin_lock_init(&ctl->lock);

	ctl->base = of_iomap(node, 0);
	if (!ctl->base) {
		ret = -ENOMEM;
		goto free_ctl;
	}

	ret = meson_gpio_irq_parse_dt(node, ctl);
	if (ret)
		goto free_channel_irqs;

	domain = irq_domain_create_hierarchy(parent_domain, 0, ctl->nr_hwirq,
					     of_node_to_fwnode(node),
					     &meson_gpio_irq_domain_ops,
					     ctl);
	if (!domain) {
		pr_err("failed to add domain\n");
		ret = -ENODEV;
		goto free_channel_irqs;
	}

	if (ctl->ops->gpio_irq_init)
		ctl->ops->gpio_irq_init(ctl);

	pr_info("%d to %d gpio interrupt mux initialized\n",
		ctl->nr_hwirq, ctl->channel_num);

	return 0;

free_channel_irqs:
	iounmap(ctl->base);
free_ctl:
	kfree(ctl);

	return ret;
}

IRQCHIP_DECLARE(meson_gpio_intc, "amlogic,meson-gpio-intc",
		meson_gpio_irq_of_init);
