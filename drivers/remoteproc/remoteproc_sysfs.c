/*
 * Remote Processor sysfs support
 *
 * Copyright (C) 2012 Texas Instruments, Inc.
 *
 * Matt Porter <mporter@ti.com>
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

#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/remoteproc.h>
#include <linux/seq_file.h>

static int on_off_state;

static ssize_t rproc_name_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct rproc *rproc = container_of(dev, struct rproc, dev);

	return sprintf(buf, "%s\n", rproc->name ? : "");
}

static ssize_t rproc_on_off_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", on_off_state);
}

static ssize_t rproc_on_off_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct rproc *rproc = container_of(dev, struct rproc, dev);
	ssize_t	status = 0;

	if (sysfs_streq(buf, "1")) {
		status = rproc_boot(rproc);
		if (!status)
			on_off_state = 1;
	} else if (sysfs_streq(buf, "0")) {
		rproc_shutdown(rproc);
		on_off_state = 0;
	} else
		status = -EINVAL;

	return status ? : size;
}

static struct device_attribute rproc_attrs[] = {
	__ATTR(name, S_IRUGO, rproc_name_show, NULL),
	__ATTR(on_off, S_IRUGO | S_IWUSR, rproc_on_off_show, rproc_on_off_store),
	{ },
};

/**
 * rproc_init_sysfs - export rproc ops through sysfs
 * @rproc: gpio to make available, already requested
 *
 * Returns zero on success, else an error.
 */
int rproc_init_sysfs(struct rproc *rproc)
{
	int status = -EINVAL;
	struct class *rproc_class = rproc->dev.class;
	
	rproc_class->dev_attrs = rproc_attrs;

	return status;
}
EXPORT_SYMBOL_GPL(rproc_init_sysfs);
