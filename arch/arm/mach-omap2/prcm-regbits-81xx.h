/*
 * OMAP81xx PRCM register bits
 *
 * Copyright (C) 2011 Texas Instruments, Inc.
 *
 * Paul Walmsley (paul@pwsan.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ARCH_ARM_MACH_OMAP2_PRCM_REGBITS_81XX_H
#define __ARCH_ARM_MACH_OMAP2_PRCM_REGBITS_81XX_H

/*
 * Used by PM_ACTIVE_PWRSTST, PM_HDVICP_PWRSTST, PM_ISP_PWRSTST,
 * PM_DSS_PWRSTST, PM_SGX_PWRSTST
 */
#define TI81XX_MEM_STATEST_SHIFT				4
#define TI81XX_MEM_STATEST_MASK					(0x3 << 4)

#endif
