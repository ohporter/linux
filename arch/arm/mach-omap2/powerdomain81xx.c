/*
 * TI81xx powerdomain control
 *
 * Copyright (C) 2009-2011 Texas Instruments, Inc.
 * Copyright (C) 2007-2009 Nokia Corporation
 *
 * Rajendra Nayak <rnayak@ti.com>
 * Paul Walmsley
 * Hemant Pedanekar
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/errno.h>
#include <linux/delay.h>

#include <plat/prcm.h>

#include "powerdomain.h"
#include "prcm81xx.h"

static int ti81xx_pwrdm_set_next_pwrst(struct powerdomain *pwrdm, u8 pwrst)
{
	ti81xx_prcm_pwrdm_set_powerstate(pwrdm->prcm_offs, pwrst);
	return 0;
}

static int ti81xx_pwrdm_read_next_pwrst(struct powerdomain *pwrdm)
{
	return ti81xx_prcm_pwrdm_read_powerstate(pwrdm->prcm_offs);
}

static int ti81xx_pwrdm_read_pwrst(struct powerdomain *pwrdm)
{
	return ti81xx_prcm_pwrdm_read_powerstatest(pwrdm->prcm_offs);
}

static int ti81xx_pwrdm_read_logic_pwrst(struct powerdomain *pwrdm)
{
	return ti81xx_prcm_pwrdm_read_logicstatest(pwrdm->prcm_offs);
}

static int ti81xx_pwrdm_read_mem_pwrst(struct powerdomain *pwrdm, u8 bank)
{
	return ti81xx_prcm_pwrdm_read_mem_statest(pwrdm->prcm_offs);
}

static int ti81xx_pwrdm_set_lowpwrstchange(struct powerdomain *pwrdm)
{
	ti81xx_prcm_pwrdm_set_lowpowerstatechange(pwrdm->prcm_offs);
	return 0;
}

static int ti81xx_pwrdm_wait_transition(struct powerdomain *pwrdm)
{
	int r;

	r = ti81xx_prcm_pwrdm_wait_transition(pwrdm->prcm_offs);
	if (r == -ETIMEDOUT) {
		pr_err("powerdomain: waited too long for powerdomain %s to complete transition\n",
		       pwrdm->name);
		return -EAGAIN;
	}

	pr_debug("powerdomain: completed transition in %d loops\n", r);

	return 0;
}

struct pwrdm_ops ti81xx_pwrdm_operations = {
	.pwrdm_set_next_pwrst	= ti81xx_pwrdm_set_next_pwrst,
	.pwrdm_read_next_pwrst	= ti81xx_pwrdm_read_next_pwrst,
	.pwrdm_read_pwrst	= ti81xx_pwrdm_read_pwrst,
	.pwrdm_set_lowpwrstchange	= ti81xx_pwrdm_set_lowpwrstchange,
	.pwrdm_read_logic_pwrst	= ti81xx_pwrdm_read_logic_pwrst,
	.pwrdm_read_mem_pwrst	= ti81xx_pwrdm_read_mem_pwrst,
	.pwrdm_wait_transition	= ti81xx_pwrdm_wait_transition,
};
