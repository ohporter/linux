/*
 * TI81XX PRCM register access functions
 *
 * Copyright (C) 2010-2011 Texas Instruments, Inc. - http://www.ti.com/
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

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/io.h>

#include "prcm81xx.h"

static u32 ti81xx_prcm_inst_read(u16 inst, u16 offs)
{
	return __raw_readl(prm_base + inst + offs);
}

static void ti81xx_prcm_inst_write(u32 v, u16 inst, u16 offs)
{
	__raw_writel(v, prm_base + inst + offs);
}


