/*
 * TI814X-specific PRCM register access macros
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

#ifndef __ARCH_ARM_MACH_OMAP2_PRCM814X_H
#define __ARCH_ARM_MACH_OMAP2_PRCM814X_H

#include "prcm81xx.h"

/* TI814X-specific PRCM instances */
#define TI814X_CM_HDVICP_INST			0x0600	/* 256B */
#define TI814X_CM_ISP_INST			0x0700	/* 256B */
#define TI814X_CM_DSS_INST			0x0800	/* 256B */
#define TI814X_PRM_HDVICP_INST			0x0c00	/* 256B */
#define TI814X_PRM_ISP_INST			0x0d00	/* 256B */
#define TI814X_PRM_DSS_INST			0x0e00	/* 256B */
#define TI814X_PRM_ALWON_INST			0x1800	/* 1KiB */


/*
 * TI814x-specific PRCM registers offsets. The offsets below are
 * relative to the PRCM instance base.
 */

/* CM_ALWON */
#define TI814X_CM_ALWON_L3_SLOW_CLKSTCTRL_OFFSET	0x0000
#define TI814X_CM_ALWON_L3_SLOW_CLKSTCTRL		TI81XX_CM_REGADDR(TI81XX_CM_ALWON_INST, 0x0000)
#define TI814X_CM_ALWON_L3_MED_CLKSTCTRL_OFFSET		0x0008
#define TI814X_CM_ALWON_L3_MED_CLKSTCTRL		TI81XX_CM_REGADDR(TI81XX_CM_ALWON_INST, 0x0008)

/* CM_HDVICP */
#define TI814X_CM_HDVICP_CLKSTCTRL_OFFSET		0x0000
#define TI814X_CM_HDVICP_CLKSTCTRL			TI81XX_CM_REGADDR(TI81XX_CM_HDVICP_INST, 0x0000)

/* CM_ISP */
#define TI814X_CM_ISP_CLKSTCTRL_OFFSET			0x0000
#define TI814X_CM_ISP_CLKSTCTRL				TI81XX_CM_REGADDR(TI81XX_CM_ISP_INST, 0x0000)

/* CM_DSS */
#define TI814X_CM_DSS_CLKSTCTRL_OFFSET			0x0000
#define TI814X_CM_DSS_CLKSTCTRL				TI81XX_CM_REGADDR(TI81XX_CM_ISP_INST, 0x0000)

/* CM_DEFAULT */
#define TI814X_CM_DEFAULT_TPPSS_CLKSTCTRL_OFFSET	0x0000
#define TI814X_CM_DEFAULT_TPPSS_CLKSTCTRL		TI81XX_CM_REGADDR(TI81XX_CM_DEFAULT_INST, 0x0000)
#define TI814X_CM_DEFAULT_PCI_CLKSTCTRL_OFFSET		0x0004
#define TI814X_CM_DEFAULT_PCI_CLKSTCTRL			TI81XX_CM_REGADDR(TI81XX_CM_DEFAULT_INST, 0x0004)
#define TI814X_CM_DEFAULT_DUCATI_CLKSTCTRL_OFFSET	0x000c
#define TI814X_CM_DEFAULT_DUCATI_CLKSTCTRL		TI81XX_CM_REGADDR(TI81XX_CM_DEFAULT_INST, 0x000c)

#endif
