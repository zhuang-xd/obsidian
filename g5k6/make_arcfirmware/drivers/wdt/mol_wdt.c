/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12               the first version
 *
 */

#include "mol_wdt.h"

void wdt_write_reg(struct fh_wdt_obj *wdt_obj, unsigned int offset,
                unsigned int val, unsigned int mask)
{
	unsigned int reg = GET_REG(wdt_obj->base + offset);
	reg &= ~mask;
	reg |= val;
	SET_REG(wdt_obj->base + WDOG_OP_LOCK_OFFSET, WDOG_UNLOCK_KEY);
	SET_REG(wdt_obj->base + offset, reg);
	SET_REG(wdt_obj->base + WDOG_OP_LOCK_OFFSET, 0);
}

void wdt_stop(struct fh_wdt_obj *wdt_obj)
{
    wdt_write_reg(wdt_obj, WDOG_CTRL_OFFSET, 0, WDOG_RUN);
}

static void wdt_wait_sync(struct fh_wdt_obj *wdt_obj)
{
	unsigned int reg;

	do
	{
		reg = GET_REG(wdt_obj->base + WDOG_RAW_INT_OFFSET);
	} while (reg & WDOG_WAIT_SYNC);
}

int wdt_start(struct fh_wdt_obj *wdt_obj)
{
	unsigned long load_count, data;

	data        = WDOG_RUN | WDOG_NEW;
	load_count  = wdt_obj->timeout * WDT_CLOCK;

	//default use reset mode, in case system cannot reset when
	//system cannot response interrupt(such as system was crashed).
	data |= WDOG_RST_EN;
	data &= ~WDOG_IRQ_EN;
	// if (wdt->rst) {
	//	data |= WDOG_RST_EN;
	//	data &= ~WDOG_IRQ_EN;
	// } else {
	//	data &= ~WDOG_RST_EN;
	//	data |= WDOG_IRQ_EN;
	// }

	wdt_write_reg(wdt_obj, WDOG_CTRL_OFFSET, data, 0xFFFFFFFF);
	wdt_write_reg(wdt_obj, WDOG_LOAD_VALUE_OFFSET, load_count, 0xFFFFFFFF);
	wdt_write_reg(wdt_obj, WDOG_INT_CLEAR_OFFSET, 0x1, 0x1);
	wdt_write_reg(wdt_obj, WDOG_IRQ_VALUE_OFFSET, 0, 0xFFFFFFFF);
	// wdt_wait_sync(wdt_obj);

	return 0;
}

int wdt_settimeout(struct fh_wdt_obj *wdt_obj, unsigned int new_time)
{
	wdt_obj->timeout = new_time;
	wdt_start(wdt_obj);

	return 0;
}

void wdt_reload(struct fh_wdt_obj *wdt_obj)
{
	wdt_write_reg(wdt_obj, WDOG_LOAD_VALUE_OFFSET, wdt_obj->timeout * WDT_CLOCK, 0xFFFFFFFF);
}

unsigned int wdt_gettimeout(struct fh_wdt_obj *wdt_obj)
{
	int time_left;

	time_left = GET_REG(wdt_obj->base + WDOG_CNT_READ_OFFSET);
	return time_left / WDT_CLOCK;
}
