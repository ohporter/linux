/*
 * include/linux/platform_data/sram.h
 *
 * Platform data for generic sram driver
 *
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
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

#ifndef _SRAM_H_
#define _SRAM_H_

struct sram_pdata {
	unsigned alloc_order;	/* Optional: driver defaults to PAGE_SHIFT */
};

#endif /* _SRAM_H_ */
