/*
 * TI81XX clockdomain control
 *
 * Copyright (C) 2008-2011 Texas Instruments, Inc.
 * Copyright (C) 2008-2010 Nokia Corporation
 *
 * Paul Walmsley
 * Rajendra Nayak <rnayak@ti.com>
 * Hemant Pedanekar
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>

#include <plat/prcm.h>

#include "prcm81xx.h"
#include "prm.h"
#include "clockdomain.h"

static int ti81xx_clkdm_sleep(struct clockdomain *clkdm)
{
	ti81xx_prcm_clkdm_force_sleep(clkdm->cm_inst, clkdm->clkdm_offs);
	return 0;
}

static int ti81xx_clkdm_wakeup(struct clockdomain *clkdm)
{
	ti81xx_prcm_clkdm_force_wakeup(clkdm->cm_inst, clkdm->clkdm_offs);
	return 0;
}

static void ti81xx_clkdm_allow_idle(struct clockdomain *clkdm)
{
	ti81xx_prcm_clkdm_enable_hwsup(clkdm->cm_inst, clkdm->clkdm_offs);
}

static void ti81xx_clkdm_deny_idle(struct clockdomain *clkdm)
{
	ti81xx_prcm_clkdm_disable_hwsup(clkdm->cm_inst, clkdm->clkdm_offs);
}

static int ti81xx_clkdm_clk_enable(struct clockdomain *clkdm)
{
	bool hwsup;

	hwsup = ti81xx_prcm_is_clkdm_in_hwsup(clkdm->cm_inst, clkdm->clkdm_offs);

	if (!hwsup && clkdm->flags & CLKDM_CAN_FORCE_WAKEUP)
		ti81xx_clkdm_wakeup(clkdm);

	return 0;
}

static int ti81xx_clkdm_clk_disable(struct clockdomain *clkdm)
{
	bool hwsup;

	hwsup = ti81xx_prcm_is_clkdm_in_hwsup(clkdm->cm_inst, clkdm->clkdm_offs);

	if (!hwsup && clkdm->flags & CLKDM_CAN_FORCE_SLEEP)
		ti81xx_clkdm_sleep(clkdm);

	return 0;
}

struct clkdm_ops ti81xx_clkdm_operations = {
	.clkdm_sleep		= ti81xx_clkdm_sleep,
	.clkdm_wakeup		= ti81xx_clkdm_wakeup,
	.clkdm_allow_idle	= ti81xx_clkdm_allow_idle,
	.clkdm_deny_idle	= ti81xx_clkdm_deny_idle,
	.clkdm_clk_enable	= ti81xx_clkdm_clk_enable,
	.clkdm_clk_disable	= ti81xx_clkdm_clk_disable,
};
