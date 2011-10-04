/*
 * TI816X Clock Domain data.
 *
 * Copyright (C) 2010-2011 Texas Instruments, Inc. - http://www.ti.com/
 * Hemant Pedanekar
 * Paul Walmsley
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
#include <linux/io.h>

#include "clockdomain.h"
#include "cm.h"
#include "prcm81xx.h"
#include "prcm814x.h"
#include "prcm816x.h"
#include "prcm-regbits-81xx.h"

static struct clockdomain alwon_mpu_81xx_clkdm = {
	.name		= "alwon_mpu_clkdm",
	.pwrdm		= { .name = "alwon_pwrdm" },
	.cm_inst	= TI81XX_CM_ALWON_INST,
	.clkdm_offs	= TI816X_CM_ALWON_MPU_CLKSTCTRL_OFFSET,
	.flags		= CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain alwon_l3_slow_814x_clkdm = {
	.name		= "alwon_l3_slow_clkdm",
	.pwrdm		= { .name = "alwon_pwrdm" },
	.cm_inst	= TI81XX_CM_ALWON_INST,
	.clkdm_offs	= TI814X_CM_ALWON_L3_SLOW_CLKSTCTRL_OFFSET,
	.flags		= CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain alwon_l3_med_814x_clkdm = {
	.name		= "alwon_l3_med_clkdm",
	.pwrdm		= { .name = "alwon_pwrdm" },
	.cm_inst	= TI81XX_CM_ALWON_INST,
	.clkdm_offs	= TI814X_CM_ALWON_L3_MED_CLKSTCTRL_OFFSET,
	.flags		= CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain alwon_l3_fast_81xx_clkdm = {
	.name		= "alwon_l3_fast_clkdm",
	.pwrdm		= { .name = "alwon_pwrdm" },
	.cm_inst	= TI81XX_CM_ALWON_INST,
	.clkdm_offs	= TI81XX_CM_ALWON_L3_FAST_CLKSTCTRL_OFFSET,
	.flags		= CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain alwon_ethernet_81xx_clkdm = {
	.name		= "alwon_ethernet_clkdm",
	.pwrdm		= { .name = "alwon_pwrdm" },
	.cm_inst	= TI81XX_CM_ALWON_INST,
	.clkdm_offs	= TI81XX_CM_ETHERNET_CLKSTCTRL_OFFSET,
	.flags		= CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain mmu_81xx_clkdm = {
	.name		= "mmu_clkdm",
	.pwrdm		= { .name = "alwon_pwrdm" },
	.cm_inst	= TI81XX_CM_ALWON_INST,
	.clkdm_offs	= TI81XX_CM_MMU_CLKSTCTRL_OFFSET,
	.flags		= CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain mmu_cfg_81xx_clkdm = {
	.name		= "mmu_cfg_clkdm",
	.pwrdm		= { .name = "alwon_pwrdm" },
	.cm_inst	= TI81XX_CM_ALWON_INST,
	.clkdm_offs	= TI81XX_CM_MMUCFG_CLKSTCTRL_OFFSET,
	.flags		= CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain active_gem_81xx_clkdm = {
	.name		= "active_gem_clkdm",
	.pwrdm		= { .name = "active_pwrdm" },
	.cm_inst	= TI81XX_CM_ACTIVE_INST,
	.clkdm_offs	= TI81XX_CM_ACTIVE_GEM_CLKSTCTRL_OFFSET,
	.flags		= CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain hdvicp_814x_clkdm = {
	.name		= "hdvicp_clkdm",
	.pwrdm		= { .name = "hdvicp_pwrdm" },
	.cm_inst	= TI814X_CM_HDVICP_INST,
	.clkdm_offs	= TI814X_CM_HDVICP_CLKSTCTRL_OFFSET,
	.flags		= CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain isp_814x_clkdm = {
	.name		= "isp_clkdm",
	.pwrdm		= { .name = "isp_pwrdm" },
	.cm_inst	= TI814X_CM_ISP_INST,
	.clkdm_offs	= TI814X_CM_ISP_CLKSTCTRL_OFFSET,
	.flags		= CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain dss_814x_clkdm = {
	.name		= "dss_clkdm",
	.pwrdm		= { .name = "dss_pwrdm" },
	.cm_inst	= TI814X_CM_DSS_INST,
	.clkdm_offs	= TI814X_CM_DSS_CLKSTCTRL_OFFSET,
	.flags		= CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain sgx_81xx_clkdm = {
	.name		= "sgx_clkdm",
	.pwrdm		= { .name = "sgx_pwrdm" },
	.cm_inst	= TI81XX_CM_SGX_INST,
	.clkdm_offs	= TI81XX_CM_SGX_SGX_CLKSTCTRL_OFFSET,
	.flags		= CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain default_l3_slow_816x_clkdm = {
	.name		= "default_l3_slow_clkdm",
	.pwrdm		= { .name = "default_pwrdm" },
	.cm_inst	= TI81XX_CM_DEFAULT_INST,
	.clkdm_offs	= TI816X_CM_DEFAULT_L3_SLOW_CLKSTCTRL_OFFSET,
	.flags		= CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain default_tppss_814x_clkdm = {
	.name		= "default_tppss_clkdm",
	.pwrdm		= { .name = "default_pwrdm" },
	.cm_inst	= TI81XX_CM_DEFAULT_INST,
	.clkdm_offs	= TI814X_CM_DEFAULT_TPPSS_CLKSTCTRL_OFFSET,
	.flags		= CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain default_l3_med_816x_clkdm = {
	.name		= "default_l3_med_clkdm",
	.pwrdm		= { .name = "default_pwrdm" },
	.cm_inst	= TI81XX_CM_DEFAULT_INST,
	.clkdm_offs	= TI816X_CM_DEFAULT_L3_MED_CLKSTCTRL_OFFSET,
	.flags		= CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain default_ducati_814x_clkdm = {
	.name		= "default_ducati_clkdm",
	.pwrdm		= { .name = "default_pwrdm" },
	.cm_inst	= TI81XX_CM_DEFAULT_INST,
	.clkdm_offs	= TI814X_CM_DEFAULT_DUCATI_CLKSTCTRL_OFFSET,
	.flags		= CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain default_ducati_816x_clkdm = {
	.name		= "default_ducati_clkdm",
	.pwrdm		= { .name = "default_pwrdm" },
	.cm_inst	= TI81XX_CM_DEFAULT_INST,
	.clkdm_offs	= TI816X_CM_DEFAULT_DUCATI_CLKSTCTRL_OFFSET,
	.flags		= CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain default_pcie_814x_clkdm = {
	.name		= "default_pcie_clkdm",
	.pwrdm		= { .name = "default_pwrdm" },
	.cm_inst	= TI81XX_CM_DEFAULT_INST,
	.clkdm_offs	= TI814X_CM_DEFAULT_PCI_CLKSTCTRL_OFFSET,
	.flags		= CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain default_pcie_816x_clkdm = {
	.name		= "default_pcie_clkdm",
	.pwrdm		= { .name = "default_pwrdm" },
	.cm_inst	= TI81XX_CM_DEFAULT_INST,
	.clkdm_offs	= TI816X_CM_DEFAULT_PCI_CLKSTCTRL_OFFSET,
	.flags		= CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain *clockdomains_ti81xx[] __initdata = {
	&alwon_mpu_81xx_clkdm,
	&alwon_l3_fast_81xx_clkdm,
	&alwon_ethernet_81xx_clkdm,
	&mmu_81xx_clkdm,
	&mmu_cfg_81xx_clkdm,
	&active_gem_81xx_clkdm,
	&sgx_81xx_clkdm,
	NULL,
};

static struct clockdomain *clockdomains_ti814x[] __initdata = {
	&hdvicp_814x_clkdm,
	&isp_814x_clkdm,
	&dss_814x_clkdm,
	&alwon_l3_med_814x_clkdm,
	&alwon_l3_slow_814x_clkdm,
	&default_tppss_814x_clkdm,
	&default_ducati_814x_clkdm,
	&default_pcie_814x_clkdm,
	NULL,
};

static struct clockdomain *clockdomains_ti816x[] __initdata = {
	&default_l3_med_816x_clkdm,
	&default_l3_slow_816x_clkdm,
	&default_ducati_816x_clkdm,
	&default_pcie_816x_clkdm,
	NULL,
};

void __init ti81xx_clockdomains_init(void)
{
	if (!cpu_is_ti81xx())
		return;

	clkdm_register_platform_funcs(&ti81xx_clkdm_operations);
	clkdm_register_clkdms(clockdomains_ti81xx);
	if (cpu_is_ti814x())
		clkdm_register_clkdms(clockdomains_ti814x);
	if (cpu_is_ti816x())
		clkdm_register_clkdms(clockdomains_ti816x);
	clkdm_complete_init();
}
