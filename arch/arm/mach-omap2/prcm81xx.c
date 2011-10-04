/*
 * TI81XX PRCM register access functions
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
#include <linux/errno.h>
#include <linux/io.h>

#include <plat/common.h>

#include "powerdomain.h"

#include "prcm81xx.h"
#include "prcm-regbits-81xx.h"
#include "prm.h"

#include "prm-regbits-44xx.h"

#include "cm-regbits-34xx.h"
#include "cm-regbits-44xx.h"

/* prm_base = cm_base on TI81xx, so either is fine */

static u32 ti81xx_prcm_inst_read(u16 inst, u16 offs)
{
	return __raw_readl(prm_base + inst + offs);
}

static void ti81xx_prcm_inst_write(u32 v, u16 inst, u16 offs)
{
	__raw_writel(v, prm_base + inst + offs);
}

void ti81xx_prcm_pwrdm_set_powerstate(u16 offs, u8 pwrst)
{
	u32 v;

	v = ti81xx_prcm_inst_read(offs, TI81XX_PM_PWRSTCTRL_OFFSET);
	v &= ~OMAP_POWERSTATE_MASK;
	v |= pwrst << OMAP_POWERSTATE_SHIFT;
	ti81xx_prcm_inst_write(v, offs, TI81XX_PM_PWRSTCTRL_OFFSET);
}

u8 ti81xx_prcm_pwrdm_read_powerstate(u16 offs)
{
	u32 v;

	v = ti81xx_prcm_inst_read(offs, TI81XX_PM_PWRSTCTRL_OFFSET);
	v &= OMAP_POWERSTATE_MASK;
	v >>= OMAP_POWERSTATE_SHIFT;

	return v;
}

u8 ti81xx_prcm_pwrdm_read_powerstatest(u16 offs)
{
	u32 v;

	v = ti81xx_prcm_inst_read(offs, TI81XX_PM_PWRSTST_OFFSET);
	v &= OMAP_POWERSTATEST_MASK;
	v >>= OMAP_POWERSTATEST_SHIFT;

	return v;
}

u8 ti81xx_prcm_pwrdm_read_logicstatest(u16 offs)
{
	u32 v;

	v = ti81xx_prcm_inst_read(offs, TI81XX_PM_PWRSTCTRL_OFFSET);
	v &= OMAP4430_LOGICSTATEST_MASK;
	v >>= OMAP4430_LOGICSTATEST_SHIFT;

	return v;
}

u8 ti81xx_prcm_pwrdm_read_mem_statest(u16 offs)
{
	u32 v;

	v = ti81xx_prcm_inst_read(offs, TI81XX_PM_PWRSTST_OFFSET);
	v &= TI81XX_MEM_STATEST_MASK;
	v >>= TI81XX_MEM_STATEST_SHIFT;

	return v;
}

void ti81xx_prcm_pwrdm_set_lowpowerstatechange(u16 offs)
{
	u32 v;

	v = ti81xx_prcm_inst_read(offs, TI81XX_PM_PWRSTCTRL_OFFSET);
	v &= ~OMAP4430_LOWPOWERSTATECHANGE_MASK;
	v |= 1 << OMAP4430_LOWPOWERSTATECHANGE_SHIFT;
	ti81xx_prcm_inst_write(v, offs, TI81XX_PM_PWRSTCTRL_OFFSET);
}

int ti81xx_prcm_pwrdm_wait_transition(u16 offs)
{
	u32 c = 0;

	/*
	 * REVISIT: pwrdm_wait_transition() may be better implemented
	 * via a callback and a periodic timer check -- how long do we expect
	 * powerdomain transitions to take?
	 */

	omap_test_timeout((ti81xx_prcm_inst_read(offs,
						 TI81XX_PM_PWRSTST_OFFSET) &
			   OMAP_INTRANSITION_MASK),
			  PWRDM_TRANSITION_BAILOUT, c);

	if (c > INT_MAX)
		c = INT_MAX;

	return (c <= PWRDM_TRANSITION_BAILOUT) ? c : -ETIMEDOUT;
}

void ti81xx_prcm_clkdm_enable_hwsup(s16 inst, u16 offs)
{
	u32 v;

	v = ti81xx_prcm_inst_read(inst, offs);
	v &= ~OMAP4430_CLKTRCTRL_MASK;
	v |= OMAP34XX_CLKSTCTRL_ENABLE_AUTO << OMAP4430_CLKTRCTRL_SHIFT;
	ti81xx_prcm_inst_write(v, inst, offs);

}

void ti81xx_prcm_clkdm_disable_hwsup(s16 inst, u16 offs)
{
	u32 v;

	v = ti81xx_prcm_inst_read(inst, offs);
	v &= ~OMAP4430_CLKTRCTRL_MASK;
	v |= OMAP34XX_CLKSTCTRL_DISABLE_AUTO << OMAP4430_CLKTRCTRL_SHIFT;
	ti81xx_prcm_inst_write(v, inst, offs);
}

void ti81xx_prcm_clkdm_force_sleep(s16 inst, u16 offs)
{
	u32 v;

	v = ti81xx_prcm_inst_read(inst, offs);
	v &= ~OMAP4430_CLKTRCTRL_MASK;
	v |= OMAP34XX_CLKSTCTRL_FORCE_SLEEP << OMAP4430_CLKTRCTRL_SHIFT;
	ti81xx_prcm_inst_write(v, inst, offs);
}

void ti81xx_prcm_clkdm_force_wakeup(s16 inst, u16 offs)
{
	u32 v;

	v = ti81xx_prcm_inst_read(inst, offs);
	v &= ~OMAP4430_CLKTRCTRL_MASK;
	v |= OMAP34XX_CLKSTCTRL_FORCE_WAKEUP << OMAP4430_CLKTRCTRL_SHIFT;
	ti81xx_prcm_inst_write(v, inst, offs);
}

bool ti81xx_prcm_is_clkdm_in_hwsup(s16 inst, u16 offs)
{
	u32 v;
	bool ret = 0;

	v = ti81xx_prcm_inst_read(inst, offs);
	v &= OMAP4430_CLKTRCTRL_MASK;
	v >>= OMAP4430_CLKTRCTRL_SHIFT;
	ret = (v == OMAP34XX_CLKSTCTRL_ENABLE_AUTO) ? 1 : 0;

	return ret;
}
