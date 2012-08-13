/*
 * TI EDMA DMA engine driver
 *
 * Copyright 2012 Texas Instruments
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>

#include <mach/edma.h>

#include "dmaengine.h"

#define EDMA_MAX_CHANNELS	64
#define EDMA_DESCRIPTORS	16
/* Max of 16 segments per channel to conserve PaRAM slots */
#define MAX_NR_SG		16
#define EDMA_MAX_SLOTS		MAX_NR_SG

struct edma_desc {
	struct dma_async_tx_descriptor	txd;
	struct list_head		node;

	struct edmacc_param		pset[EDMA_MAX_SLOTS];
	int				pset_nr;
	int				absync;
};

struct edma_cc;

struct edma_chan {
	struct dma_chan			chan;
	struct list_head		free;
	struct list_head		prepared;
	struct list_head		queued;
	struct list_head		active;
	struct list_head		completed;
	struct tasklet_struct		work;

	struct edma_cc			*ecc;
	int				ch_num;
	bool				alloced;
	int				slot[EDMA_MAX_SLOTS];

	dma_addr_t			addr;
	int				addr_width;
	int				maxburst;

	/* Lock for this structure */
	spinlock_t			lock;
};

/* EDMA channel controller */
struct edma_cc {
	struct dma_device		dma_slave;
	struct edma_chan		slave_chans[EDMA_MAX_CHANNELS];
	int				num_slave_chans;
	int				dummy_slot[2];
};

/* Convert struct dma_chan to struct edma_chan */
static inline
struct edma_chan *to_edma_chan(struct dma_chan *c)
{
	return container_of(c, struct edma_chan, chan);
}

/* Dispatch a queued descriptor to the controller (caller holds lock) */
static void edma_execute(struct edma_chan *echan)
{
	struct edma_desc *edesc = NULL;
	int i;

	/* Move the first queued descriptor to active list */
	edesc = list_first_entry(&echan->queued, struct edma_desc,
		node);
	list_move_tail(&edesc->node, &echan->active);

	/* Write descriptor PaRAM set(s) */
	for (i = 0; i < edesc->pset_nr; i++) {
		edma_write_slot(echan->slot[i], &edesc->pset[i]);
		dev_dbg(echan->chan.device->dev,
			"\n pset[%d]:\n"
			"  chnum\t%d\n"
			"  slot\t%d\n"
			"  opt\t%08x\n"
			"  src\t%08x\n"
			"  dst\t%08x\n"
			"  abcnt\t%08x\n"
			"  ccnt\t%08x\n"
			"  bidx\t%08x\n"
			"  cidx\t%08x\n"
			"  lkrld\t%08x\n",
			i, echan->ch_num, echan->slot[i],
			edesc->pset[i].opt,
			edesc->pset[i].src,
			edesc->pset[i].dst,
			edesc->pset[i].a_b_cnt,
			edesc->pset[i].ccnt,
			edesc->pset[i].src_dst_bidx,
			edesc->pset[i].src_dst_cidx,
			edesc->pset[i].link_bcntrld);
		/* Link to the previous slot if not the last set */
		if (i != (edesc->pset_nr - 1))
			edma_link(echan->slot[i], echan->slot[i+1]);
		/* Final pset links to the dummy pset */
		else
			edma_link(echan->slot[i],
				echan->ecc->dummy_slot[
				EDMA_CTLR(echan->ch_num)]);
	}

	edma_start(echan->ch_num);
}

/* Process completed descriptors */
static void edma_process_completed(struct edma_chan *echan)
{
	struct edma_desc *edesc;
	struct dma_async_tx_descriptor *txd;
	unsigned long flags;
	LIST_HEAD(list);

	/* Get all completed descriptors */
	if (!list_empty(&echan->completed)) {
		spin_lock_irqsave(&echan->lock, flags);
		list_splice_tail_init(&echan->completed, &list);
		spin_unlock_irqrestore(&echan->lock, flags);

		/* Execute callbacks and run dependencies */
		list_for_each_entry(edesc, &list, node) {
			txd = &edesc->txd;

			dma_cookie_complete(txd);

			if (txd->callback)
				txd->callback(txd->callback_param);

			dma_run_dependencies(txd);
		}

		/* Free descriptors */
		spin_lock_irqsave(&echan->lock, flags);
		list_splice_tail_init(&list, &echan->free);
		spin_unlock_irqrestore(&echan->lock, flags);
	}
}

/* EDMA tasklet */
static void edma_work(unsigned long data)
{
	struct edma_chan *echan = (void *)data;

	edma_process_completed(echan);
}

/* Queue descriptor for processing */
static dma_cookie_t edma_tx_submit(struct dma_async_tx_descriptor *txd)
{
	struct edma_chan *echan = to_edma_chan(txd->chan);
	struct edma_desc *edesc;
	dma_cookie_t cookie;
	unsigned long flags;

	edesc = container_of(txd, struct edma_desc, txd);

	spin_lock_irqsave(&echan->lock, flags);

	/* Assign a cookie and queue it */
	cookie = dma_cookie_assign(&edesc->txd);
	list_move_tail(&edesc->node, &echan->queued);

	spin_unlock_irqrestore(&echan->lock, flags);

	return cookie;
}

static int edma_slave_config(struct edma_chan *echan,
	struct dma_slave_config *config)
{
	if ((config->src_addr_width > DMA_SLAVE_BUSWIDTH_4_BYTES) ||
		(config->dst_addr_width > DMA_SLAVE_BUSWIDTH_4_BYTES))
		return -EINVAL;

	if (config->direction == DMA_MEM_TO_DEV) {
		if (config->dst_addr)
			echan->addr = config->dst_addr;
		if (config->dst_addr_width)
			echan->addr_width = config->dst_addr_width;
		if (config->dst_maxburst)
			echan->maxburst = config->dst_maxburst;
	} else if (config->direction == DMA_DEV_TO_MEM) {
		if (config->src_addr)
			echan->addr = config->src_addr;
		if (config->src_addr_width)
			echan->addr_width = config->src_addr_width;
		if (config->src_maxburst)
			echan->maxburst = config->src_maxburst;
	}

	return 0;
}

static int edma_control(struct dma_chan *chan, enum dma_ctrl_cmd cmd,
			unsigned long arg)
{
	int ret = 0;
	struct dma_slave_config *config;
	struct edma_chan *echan = to_edma_chan(chan);

	switch (cmd) {
	case DMA_TERMINATE_ALL:
		edma_stop(echan->ch_num);
		break;
	case DMA_SLAVE_CONFIG:
		config = (struct dma_slave_config *)arg;
		ret = edma_slave_config(echan, config);
		break;
	default:
		ret = -ENOSYS;
	}

	return ret;
}

static struct dma_async_tx_descriptor *edma_prep_slave_sg(
	struct dma_chan *chan, struct scatterlist *sgl,
	unsigned int sg_len, enum dma_transfer_direction direction,
	unsigned long dma_flags, void *context)
{
	struct edma_chan *echan = to_edma_chan(chan);
	struct device *dev = echan->chan.device->dev;
	struct edma_desc *edesc = NULL;
	struct scatterlist *sg;
	unsigned long flags;
	int i;
	int acnt, bcnt, ccnt, src, dst, cidx;
	int src_bidx, dst_bidx, src_cidx, dst_cidx;

	if (unlikely(!echan || !sgl || !sg_len))
		return NULL;

	if (echan->addr_width == DMA_SLAVE_BUSWIDTH_UNDEFINED) {
		dev_err(dev, "Undefined slave buswidth\n");
		return NULL;
	}

	if (sg_len > MAX_NR_SG) {
		dev_err(dev, "Exceeded max SG segments %d > %d\n",
			sg_len, MAX_NR_SG);
		return NULL;
	}

	spin_lock_irqsave(&echan->lock, flags);
	/* Get free descriptor */
	if (!list_empty(&echan->free)) {
		edesc = list_first_entry(&echan->free,
				struct edma_desc, node);
		list_del_init(&edesc->node);
	}
	spin_unlock_irqrestore(&echan->lock, flags);

	if (!edesc) {
		dev_dbg(dev, "Out of descriptors for channel %d",
			echan->ch_num);
		/* Try to free completed descriptors */
		edma_process_completed(echan);
		return NULL;
	}

	edesc->pset_nr = sg_len;

	for_each_sg(sgl, sg, sg_len, i) {
		/* Allocate a PaRAM slot, if needed */
		if (echan->slot[i] < 0) {
			echan->slot[i] =
				edma_alloc_slot(EDMA_CTLR(echan->ch_num),
						EDMA_SLOT_ANY);
			if (echan->slot[i] < 0) {
				dev_err(dev, "Failed to allocate slot\n");
				return NULL;
			}
		}

		acnt = echan->addr_width;

		/*
		 * If the maxburst is equal to the fifo width, use
		 * A-synced transfers. This allows for large contiguous
		 * buffer transfers using only one PaRAM set.
		 */
		if (echan->maxburst == 1) {
			edesc->absync = false;
			ccnt = sg_dma_len(sg) / acnt / (SZ_64K - 1);
			bcnt = sg_dma_len(sg) / acnt - ccnt * (SZ_64K - 1);
			if (bcnt)
				ccnt++;
			else
				bcnt = SZ_64K - 1;
			cidx = acnt;
		/*
		 * If maxburst is greater than the fifo address_width,
		 * use AB-synced transfers where A count is the fifo
		 * address_width and B count is the maxburst. In this
		 * case, we are limited to transfers of C count frames
		 * of (address_width * maxburst) where C count is limited
		 * to SZ_64K-1. This places an upper bound on the length
		 * of an SG segment that can be handled.
		 */
		} else {
			edesc->absync = true;
			bcnt = echan->maxburst;
			ccnt = sg_dma_len(sg) / (acnt * bcnt);
			if (ccnt > (SZ_64K - 1)) {
				dev_err(dev, "Exceeded max SG segment size\n");
				return NULL;
			}
			cidx = acnt * bcnt;
		}

		if (direction == DMA_MEM_TO_DEV) {
			src = sg_dma_address(sg);
			dst = echan->addr;
			src_bidx = acnt;
			src_cidx = cidx;
			dst_bidx = 0;
			dst_cidx = 0;
		} else {
			src = echan->addr;
			dst = sg_dma_address(sg);
			src_bidx = 0;
			src_cidx = 0;
			dst_bidx = acnt;
			dst_cidx = cidx;
		}

		edesc->pset[i].opt = EDMA_TCC(EDMA_CHAN_SLOT(echan->ch_num));
		/* Configure A or AB synchronized transfers */
		if (edesc->absync)
			edesc->pset[i].opt |= SYNCDIM;
		/* If this is the last set, enable completion interrupt flag */
		if (i == sg_len - 1)
			edesc->pset[i].opt |= TCINTEN;

		edesc->pset[i].src = src;
		edesc->pset[i].dst = dst;

		edesc->pset[i].src_dst_bidx = (dst_bidx << 16) | src_bidx;
		edesc->pset[i].src_dst_cidx = (dst_cidx << 16) | src_cidx;

		edesc->pset[i].a_b_cnt = bcnt << 16 | acnt;
		edesc->pset[i].ccnt = ccnt;
		edesc->pset[i].link_bcntrld = 0xffffffff;

	}
	edesc->txd.flags = dma_flags;

	/* Place descriptor in prepared list */
	spin_lock_irqsave(&echan->lock, flags);
	list_add_tail(&edesc->node, &echan->prepared);
	spin_unlock_irqrestore(&echan->lock, flags);

	return &edesc->txd;
}

static void edma_callback(unsigned ch_num, u16 ch_status, void *data)
{
	struct edma_chan *echan = to_edma_chan((struct dma_chan *)data);
	struct device *dev = echan->chan.device->dev;
	struct edma_desc *edesc = NULL;

	/* Stop the channel */
	edma_stop(echan->ch_num);

	switch (ch_status) {
	case DMA_COMPLETE:
		dev_dbg(dev, "transfer complete on channel %d\n", ch_num);

		spin_lock(&echan->lock);

		/* Grab the first active descriptor */
		edesc = list_first_entry(&echan->active, struct edma_desc,
			node);

		/* Mark the descriptor completed */
		list_move_tail(&edesc->node, &echan->completed);

		/* Execute queued descriptors */
		if (!list_empty(&echan->queued)) {
			dev_dbg(dev, "descriptor queued on channel %d\n",
				ch_num);
			edma_execute(echan);
		}

		spin_unlock(&echan->lock);

		/* Schedule work */
		tasklet_schedule(&echan->work);

		break;
	case DMA_CC_ERROR:
		dev_dbg(dev, "transfer error on channel %d\n", ch_num);
		break;
	default:
		break;
	}
}

/* Alloc channel resources */
static int edma_alloc_chan_resources(struct dma_chan *chan)
{
	struct edma_chan *echan = to_edma_chan(chan);
	struct device *dev = echan->chan.device->dev;
	struct edma_desc *edesc;
	int ret;
	int a_ch_num;
	LIST_HEAD(descs);
	int i;
	unsigned long flags;

	a_ch_num = edma_alloc_channel(echan->ch_num, edma_callback,
					chan, EVENTQ_DEFAULT);

	if (a_ch_num < 0) {
		ret = -ENODEV;
		goto err_no_chan;
	}

	if (a_ch_num != echan->ch_num) {
		dev_err(dev, "failed to allocate requested channel %d\n",
			echan->ch_num);
		ret = -ENODEV;
		goto err_wrong_chan;
	}

	dma_cookie_init(&echan->chan);
	echan->alloced = true;
	echan->slot[0] = echan->ch_num;

	/* Alloc descriptors for this channel */
	for (i = 0; i < EDMA_DESCRIPTORS; i++) {
		edesc = kzalloc(sizeof(*edesc), GFP_KERNEL);
		if (!edesc) {
			dev_notice(dev,
				"allocated only %u descriptors\n", i);
			break;
		}

		dma_async_tx_descriptor_init(&edesc->txd, chan);
		edesc->txd.flags = DMA_CTRL_ACK;
		edesc->txd.tx_submit = edma_tx_submit;

		list_add_tail(&edesc->node, &descs);
	}

	/* Return error only if no descriptors were allocated */
	if (i == 0) {
		ret = -ENOMEM;
		goto err_alloc_desc;
	}

	spin_lock_irqsave(&echan->lock, flags);

	list_splice_tail_init(&descs, &echan->free);

	spin_unlock_irqrestore(&echan->lock, flags);

	return 0;

err_alloc_desc:
err_wrong_chan:
	edma_free_channel(a_ch_num);
err_no_chan:
	return ret;
}

/* Free channel resources */
static void edma_free_chan_resources(struct dma_chan *chan)
{
	struct edma_chan *echan = to_edma_chan(chan);
	struct edma_desc *edesc, *tmp;
	unsigned long flags;
	LIST_HEAD(descs);
	int i;

	/* Terminate transfers */
	edma_stop(echan->ch_num);

	spin_lock_irqsave(&echan->lock, flags);

	/* Channel must be idle */
	BUG_ON(!list_empty(&echan->prepared));
	BUG_ON(!list_empty(&echan->queued));
	BUG_ON(!list_empty(&echan->active));
	BUG_ON(!list_empty(&echan->completed));

	list_splice_tail_init(&echan->free, &descs);

	spin_unlock_irqrestore(&echan->lock, flags);

	/* Free descriptors */
	list_for_each_entry_safe(edesc, tmp, &descs, node)
		kfree(edesc);

	/* Free EDMA PaRAM slots */
	for (i = 1; i < EDMA_MAX_SLOTS; i++) {
		if (echan->slot[i] >= 0) {
			edma_free_slot(echan->slot[i]);
			echan->slot[i] = -1;
		}
	}

	/* Free EDMA channel */
	if (echan->alloced) {
		edma_free_channel(echan->ch_num);
		echan->alloced = false;
	}
}

/* Send pending descriptor to hardware */
static void edma_issue_pending(struct dma_chan *chan)
{
	struct edma_chan *echan = to_edma_chan(chan);
	unsigned long flags;

	spin_lock_irqsave(&echan->lock, flags);

	if (list_empty(&echan->active) && !list_empty(&echan->queued))
		edma_execute(echan);

	spin_unlock_irqrestore(&echan->lock, flags);
}

/* Check request completion status */
static enum dma_status edma_tx_status(struct dma_chan *chan,
				      dma_cookie_t cookie,
				      struct dma_tx_state *txstate)
{
	struct edma_chan *echan = to_edma_chan(chan);
	unsigned long flags;
	enum dma_status ret;

	spin_lock_irqsave(&echan->lock, flags);
	ret = dma_cookie_status(chan, cookie, txstate);
	spin_unlock_irqrestore(&echan->lock, flags);

	return ret;
}

static void __init edma_chan_init(struct edma_cc *ecc,
				  struct dma_device *dma,
				  struct edma_chan *echans)
{
	int i, j;
	int chcnt = 0;

	INIT_LIST_HEAD(&dma->channels);

	for (i = 0; i < EDMA_MAX_CHANNELS; i++) {
		struct edma_chan *echan = &echans[chcnt];
		echan->ch_num = i;
		echan->ecc = ecc;
		echan->chan.device = dma;
		for (j = 0; j < EDMA_MAX_SLOTS; j++)
			echan->slot[j] = -1;

		spin_lock_init(&echan->lock);

		INIT_LIST_HEAD(&echan->free);
		INIT_LIST_HEAD(&echan->prepared);
		INIT_LIST_HEAD(&echan->queued);
		INIT_LIST_HEAD(&echan->active);
		INIT_LIST_HEAD(&echan->completed);

		tasklet_init(&echan->work, edma_work, (unsigned long)echan);

		list_add_tail(&echan->chan.device_node, &dma->channels);

		chcnt++;
	}
}

static void edma_ops_init(struct edma_cc *ecc, struct dma_device *dma,
			  struct device *dev)
{
	if (dma_has_cap(DMA_SLAVE, dma->cap_mask))
		dma->device_prep_slave_sg = edma_prep_slave_sg;

	dma->device_alloc_chan_resources = edma_alloc_chan_resources;
	dma->device_free_chan_resources = edma_free_chan_resources;
	dma->device_issue_pending = edma_issue_pending;
	dma->device_tx_status = edma_tx_status;
	dma->device_control = edma_control;
	dma->dev = dev;
}

static int __devinit edma_probe(struct platform_device *pdev)
{
	struct edma_cc *ecc;
	int ret;

	ecc = devm_kzalloc(&pdev->dev, sizeof(*ecc), GFP_KERNEL);
	if (!ecc) {
		dev_err(&pdev->dev, "Can't allocate controller\n");
		ret = -ENOMEM;
		goto err_alloc_ecc;
	}

	ecc->dummy_slot[0] = edma_alloc_slot(0, EDMA_SLOT_ANY);
	if (ecc->dummy_slot[0] < 0) {
		dev_err(&pdev->dev, "Can't allocate PaRAM dummy slot\n");
		ret = -EIO;
		goto err_alloc_slot0;
	}

	ecc->dummy_slot[1] = edma_alloc_slot(1, EDMA_SLOT_ANY);
	if (ecc->dummy_slot[1] < 0) {
		dev_err(&pdev->dev, "Can't allocate PaRAM dummy slot\n");
		ret = -EIO;
		goto err_alloc_slot1;
	}

	edma_chan_init(ecc, &ecc->dma_slave, ecc->slave_chans);

	dma_cap_zero(ecc->dma_slave.cap_mask);
	dma_cap_set(DMA_SLAVE, ecc->dma_slave.cap_mask);

	edma_ops_init(ecc, &ecc->dma_slave, &pdev->dev);

	ret = dma_async_device_register(&ecc->dma_slave);
	if (ret)
		goto err_reg1;

	platform_set_drvdata(pdev, ecc);

	dev_info(&pdev->dev, "TI EDMA DMA engine driver\n");

	return 0;

err_reg1:
	edma_free_slot(ecc->dummy_slot[1]);
err_alloc_slot1:
	edma_free_slot(ecc->dummy_slot[0]);
err_alloc_slot0:
	devm_kfree(&pdev->dev, ecc);
err_alloc_ecc:
	return ret;
}

static int __devexit edma_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct edma_cc *ecc = dev_get_drvdata(dev);

	dma_async_device_unregister(&ecc->dma_slave);
	edma_free_slot(ecc->dummy_slot[0]);
	edma_free_slot(ecc->dummy_slot[1]);
	devm_kfree(dev, ecc);

	return 0;
}

static struct platform_driver edma_driver = {
	.probe		= edma_probe,
	.remove		= __devexit_p(edma_remove),
	.driver = {
		.name = "edma-dma-engine",
		.owner = THIS_MODULE,
	},
};

bool edma_filter_fn(struct dma_chan *chan, void *param)
{
	if (chan->device->dev->driver == &edma_driver.driver) {
		struct edma_chan *echan = to_edma_chan(chan);
		unsigned ch_req = *(unsigned *)param;
		return ch_req == echan->ch_num;
	}
	return false;
}
EXPORT_SYMBOL(edma_filter_fn);

static struct platform_device *pdev;

static const struct platform_device_info edma_dev_info = {
	.name = "edma-dma-engine",
	.id = -1,
	.dma_mask = DMA_BIT_MASK(32),
};

static int edma_init(void)
{
	int ret = platform_driver_register(&edma_driver);

	if (ret == 0) {
		pdev = platform_device_register_full(&edma_dev_info);
		if (IS_ERR(pdev)) {
			platform_driver_unregister(&edma_driver);
			ret = PTR_ERR(pdev);
		}
	}
	return ret;
}
subsys_initcall(edma_init);

static void __exit edma_exit(void)
{
	platform_device_unregister(pdev);
	platform_driver_unregister(&edma_driver);
}
module_exit(edma_exit);

MODULE_AUTHOR("Matt Porter <mporter@ti.com>");
MODULE_DESCRIPTION("TI EDMA DMA engine driver");
MODULE_LICENSE("GPL v2");
