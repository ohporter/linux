/*
 * BeagleBone 6502 Remote Processor driver
 *
 * Copyright (C) 2012 Matt Porter <matt@ohporter.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/genalloc.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/remoteproc.h>

#include "remoteproc_internal.h"

struct b6502_rproc {
	struct rproc *rproc;
	int reset;
	struct pwm_device *pwm;
	struct gen_pool *sram;
	void *fw_addr;
};

#define B6502_FW_MAGIC 0xdeadbeef
#define B6502_CLK_PER 1000

struct b6502_fw_header {
	__le32 magic;
	__le32 fw_start;
	__le32 fw_size;
	__le32 rsc_start;
	__le32 rsc_size;
};

static int b6502_load_segments(struct rproc *rproc, const struct firmware *fw)
{
	struct b6502_rproc *bproc = rproc->priv;
	struct b6502_fw_header *header = (struct b6502_fw_header *)fw->data;

	memcpy(bproc->fw_addr, fw->data + header->fw_start, header->fw_size);

	return 0;
};

static int b6502_sanity_check(struct rproc *rproc, const struct firmware *fw)
{
	struct b6502_fw_header *header = (struct b6502_fw_header *)fw->data;
	if (header->magic != B6502_FW_MAGIC) {
		dev_err(&rproc->dev, "BeagleBone-6502 firmware is corrupt\n");
		return -EINVAL;
	}

	return 0;
};

static struct resource_table *
b6502_find_rsc_table(struct rproc *rproc, const struct firmware *fw,
		     int *tablesz)
{
	struct resource_table *table;
	struct b6502_fw_header *header = (struct b6502_fw_header *)fw->data;

	table = (void *)(fw->data + header->rsc_start);
	*tablesz = header->rsc_size;

	return table;
};

const struct rproc_fw_ops b6502_fw_ops = {
	.load = b6502_load_segments,
	.find_rsc_table = b6502_find_rsc_table,
	.sanity_check = b6502_sanity_check,
};

static int b6502_rproc_start(struct rproc *rproc)
{
	struct b6502_rproc *bproc = rproc->priv;
	int ret;

	/* Start pwm and deassert reset */
	ret = pwm_config(bproc->pwm, B6502_CLK_PER/2, B6502_CLK_PER);
	if (ret < 0)
		return ret;
	pwm_set_polarity(bproc->pwm, PWM_POLARITY_NORMAL);
	ret = pwm_enable(bproc->pwm);
	if (ret < 0)
		return ret;

	gpio_set_value(bproc->reset, 1);

	return ret;
}

static int b6502_rproc_stop(struct rproc *rproc)
{
	struct b6502_rproc *bproc = rproc->priv;

	/* Assert reset and stop pwm */
	gpio_set_value(bproc->reset, 0);
	pwm_disable(bproc->pwm);

	return 0;
}

static struct rproc_ops b6502_rproc_ops = {
	.start		= b6502_rproc_start,
	.stop		= b6502_rproc_stop,
};

static int __devinit b6502_rproc_probe(struct platform_device *pdev)
{
	struct b6502_rproc *bproc;
	struct rproc *rproc;
	struct device_node *node = pdev->dev.of_node;
	struct pinctrl *pinctrl;
	int ret;

	pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
	if (IS_ERR(pinctrl))
		dev_warn(&pdev->dev,
			"pins are not configured from the driver\n");

	rproc = rproc_alloc(&pdev->dev, "bone-6502", &b6502_rproc_ops,
				"bone-6502-fw.bin", sizeof(*bproc));
	if (!rproc)
		return -ENOMEM;

	bproc = rproc->priv;
	bproc->rproc = rproc;
	platform_set_drvdata(pdev, rproc);
	rproc->fw_ops = &b6502_fw_ops;

	ret = rproc_add(rproc);
	if (ret)
		goto free_rproc;

	if (node) {
		bproc->reset = of_get_named_gpio(node, "reset", 0);
		if (bproc->reset < 0) {
			ret = -EIO;
			goto rm_rproc;
		}
		ret = gpio_request_one(bproc->reset, GPIOF_OUT_INIT_LOW,
				       "Bone-6502 Reset");
		if (ret < 0) {
			ret = -EIO;
			goto rm_rproc;
		}
		bproc->pwm = pwm_get(&pdev->dev, "clock"); 
		if (!bproc->pwm) {	
			ret = -EIO;
			goto free_gpio;
		}
		bproc->sram = of_get_named_gen_pool(node, "sram", 0);
		if (!bproc->sram) {
			ret = -ENOMEM;
			goto free_pwm;
		}
		bproc->fw_addr = (void *)gen_pool_alloc(bproc->sram, SZ_8K);
		if (!bproc->fw_addr) {
			ret = -ENOMEM;
			goto free_pwm;
		}
	}
		
	return 0;

free_gpio:
	gpio_free(bproc->reset);
free_pwm:
	pwm_put(bproc->pwm);
rm_rproc:
	rproc_del(rproc);
free_rproc:
	rproc_put(rproc);
	return ret;
}

static int __devexit b6502_rproc_remove(struct platform_device *pdev)
{
	struct rproc *rproc = platform_get_drvdata(pdev);

	rproc_del(rproc);
	rproc_put(rproc);

	return 0;
}

static struct of_device_id b6502_dt_ids[] = {
	{ .compatible = "bone6502" },
	{ /* sentinel */ }
};

static struct platform_driver b6502_rproc_driver = {
	.probe = b6502_rproc_probe,
	.remove = __devexit_p(b6502_rproc_remove),
	.driver = {
		.name = "b6502-rproc",
		.of_match_table = of_match_ptr(b6502_dt_ids),
		.owner = THIS_MODULE,
	},
};

module_platform_driver(b6502_rproc_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("BeagleBone 6502 Remote Processor driver");
