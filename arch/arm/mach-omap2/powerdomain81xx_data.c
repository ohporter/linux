/*
 * TI81XX Power Domain data.
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

#include "powerdomain.h"
#include "prcm81xx.h"
#include "prcm814x.h"
#include "prcm816x.h"

static struct powerdomain alwon_814x_pwrdm = {
	.name		= "alwon_pwrdm",
	.prcm_offs	= TI814X_PRM_ALWON_INST,
};

static struct powerdomain active_81xx_pwrdm = {
	.name		= "active_pwrdm",
	.prcm_offs	= TI81XX_PRM_ACTIVE_INST,
	.pwrsts		= PWRSTS_OFF_ON,
};

static struct powerdomain default_81xx_pwrdm = {
	.name		= "default_pwrdm",
	.prcm_offs	= TI81XX_PRM_DEFAULT_INST,
	.pwrsts		= PWRSTS_OFF_ON,
};

static struct powerdomain hdvicp_814x_pwrdm = {
	.name		= "hdvicp_pwrdm",
	.prcm_offs	= TI814X_PRM_HDVICP_INST,
	.pwrsts		= PWRSTS_OFF_ON,
};

static struct powerdomain isp_814x_pwrdm = {
	.name		= "isp_pwrdm",
	.prcm_offs	= TI814X_PRM_ISP_INST,
	.pwrsts		= PWRSTS_OFF_ON,
};

static struct powerdomain dss_814x_pwrdm = {
	.name		= "dss_pwrdm",
	.prcm_offs	= TI814X_PRM_DSS_INST,
	.pwrsts		= PWRSTS_OFF_ON,
};

static struct powerdomain sgx_81xx_pwrdm = {
	.name		= "sgx_pwrdm",
	.prcm_offs	= TI81XX_PRM_SGX_INST,
	.pwrsts		= PWRSTS_OFF_ON,
};

static struct powerdomain *powerdomains_ti81xx[] __initdata = {
	&active_81xx_pwrdm,
	&default_81xx_pwrdm,
	&sgx_81xx_pwrdm,
	NULL
};

static struct powerdomain *powerdomains_ti814x[] __initdata = {
	&alwon_814x_pwrdm,
	&hdvicp_814x_pwrdm,
	&isp_814x_pwrdm,
	&dss_814x_pwrdm,
	NULL
};

void __init ti81xx_powerdomains_init(void)
{
	if (!cpu_is_ti81xx())
		return;

	pwrdm_register_platform_funcs(&ti81xx_pwrdm_operations);
	pwrdm_register_pwrdms(powerdomains_ti81xx);
	if (cpu_is_ti814x())
		pwrdm_register_pwrdms(powerdomains_ti814x);
	pwrdm_complete_init();
}
