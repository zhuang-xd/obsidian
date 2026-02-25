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

#ifndef __FH_WDT_H__
#define __FH_WDT_H__

#include "fh_def.h"

#define WDT_DEFAULT_TIMEOUT		10
#define WDT_MIN_TIMEOUT			1

#define WDOG_LOAD_VALUE_OFFSET			0x00
#define WDOG_CTRL_OFFSET				0x04
#define WDOG_INT_CLEAR_OFFSET			0x08
#define WDOG_RAW_INT_OFFSET				0x0C
#define WDOG_INT_STATUS_OFFSET			0x10
#define WDOG_CNT_VALUE_OFFSET			0x14
#define WDOG_OP_LOCK_OFFSET				0x18
#define WDOG_CNT_READ_OFFSET			0x1C
#define WDOG_IRQ_VALUE_OFFSET			0x20
#define WDOG_UNLOCK_KEY					(0x1ACCE551)

/*
 * WDT_CTRL Register
 */
#define WDOG_IRQ_EN						BIT(0)
#define WDOG_RUN						BIT(1)
#define WDOG_NEW						BIT(2)
#define WDOG_RST_EN						BIT(3)
#define WDOG_WAIT_SYNC					BIT(4)
#define WDT_CLOCK						32768

struct fh_wdt_obj
{
    int id;
    int irq;
    unsigned int base;
    struct clk *clk;
	int rst;
	int timeout;
};

void wdt_write_reg(struct fh_wdt_obj *wdt_obj, unsigned int offset,
                unsigned int val, unsigned int mask);
void wdt_stop(struct fh_wdt_obj *wdt_obj);
int wdt_start(struct fh_wdt_obj *wdt_obj);
int wdt_settimeout(struct fh_wdt_obj *wdt_obj, unsigned int new_time);
void wdt_reload(struct fh_wdt_obj *wdt_obj);
unsigned int wdt_gettimeout(struct fh_wdt_obj *wdt_obj);

#endif /* FH_WDT_H_ */
