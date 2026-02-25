/*
* Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12     songyh         add license Apache-2.0
 */

#include <rtthread.h>
#include <rthw.h>
#include <rtdef.h>
#include "fh_def.h"
#include "mol_wdt.h"
#include "wdt.h"
#include <fh_arch.h>
// #include "board_info.h"
#include "watchdog.h"
#include <fh_clock.h>

#define WDT_DEFAULT_TIMEOUT		10

#if defined(FH_WDT_DEBUG) && defined(RT_DEBUG)
#define PRINT_WDT_DBG(fmt, args...)   \
    do                                \
    {                                 \
        rt_kprintf("FH_WDT_DEBUG: "); \
        rt_kprintf(fmt, ##args);      \
    } while (0)
#else
#define PRINT_WDT_DBG(fmt, args...) \
    do                              \
    {                               \
    } while (0)
#endif

rt_err_t fh_watchdog_init(rt_watchdog_t *wdt)
{
    return RT_EOK;
}

rt_err_t fh_watchdog_ctrl(rt_watchdog_t *wdt, int cmd, void *arg)
{
    int heartbeat;
    struct fh_wdt_obj *wdt_obj = wdt->parent.user_data;

    switch (cmd)
    {
    case RT_DEVICE_CTRL_WDT_START:
        wdt_start(wdt_obj);
        break;

    case RT_DEVICE_CTRL_WDT_STOP:
        wdt_stop(wdt_obj);
        break;

    case RT_DEVICE_CTRL_WDT_KEEPALIVE:
        wdt_reload(wdt_obj);
        break;

    case RT_DEVICE_CTRL_WDT_SET_TIMEOUT:
        heartbeat = *((int *)(arg));
        PRINT_WDT_DBG("[wdt] settime value %lu\n", heartbeat);
        wdt_settimeout(wdt_obj, (unsigned int)heartbeat);
		wdt_reload(wdt_obj);
        break;

    case RT_DEVICE_CTRL_WDT_GET_TIMEOUT:
    	*((int *)arg) = wdt_obj->timeout;
        break;

    case RT_DEVICE_CTRL_WDT_GET_TIMELEFT:
        *((int *)arg) = (int)wdt_gettimeout(wdt_obj);
        break;
    default:
        return -RT_EIO;
    }

    return RT_EOK;
}

static void fh_wdt_interrupt(int irq, void *param)
{
    struct fh_wdt_obj *wdt_obj = (struct fh_wdt_obj *)param;

    wdt_stop(wdt_obj);
	wdt_write_reg(wdt_obj, WDOG_INT_CLEAR_OFFSET, 0x1, 0x1);
	wdt_obj->rst = 1;
	wdt_settimeout(wdt_obj, 1);
}

struct rt_watchdog_ops fh_watchdog_ops = {
    .init = &fh_watchdog_init, .control = &fh_watchdog_ctrl,
};

static struct fh_wdt_obj wdt_obj = {
    .id = 0, .irq = WDT_IRQn, .base = WDT0_REG_BASE,
    .timeout = WDT_DEFAULT_TIMEOUT,
};

int fh_wdt_probe(void *priv_data)
{
    rt_watchdog_t *wdt_dev;
    struct fh_wdt_obj *wdt_obj = (struct fh_wdt_obj *)priv_data;
    char wdt_dev_name[8] = {0};
    char wdt_clk_name[16] = {0};

    rt_sprintf(wdt_clk_name, "wdt_clk");
    wdt_obj->clk = clk_get(RT_NULL, wdt_clk_name);
    if (wdt_obj->clk == RT_NULL)
    {
        rt_kprintf("ERROR: %s rt_watchdog_t get clk failed\n", __func__);
        return -RT_EIO;
    }

    clk_enable(wdt_obj->clk);

    rt_hw_interrupt_install(wdt_obj->irq, fh_wdt_interrupt, (void *)wdt_obj,
                            "wdt_irq");
    rt_hw_interrupt_umask(wdt_obj->irq);


    wdt_dev = (rt_watchdog_t *)rt_malloc(sizeof(rt_watchdog_t));
    if (wdt_dev == RT_NULL)
    {
        rt_kprintf("ERROR: %s rt_watchdog_t malloc failed\n", __func__);
        return -RT_ENOMEM;
    }

    wdt_dev->ops = &fh_watchdog_ops;
    rt_sprintf(wdt_dev_name, "%s%d", "fh_wdt", wdt_obj->id);
    rt_hw_watchdog_register(wdt_dev, wdt_dev_name, RT_DEVICE_OFLAG_RDWR, wdt_obj);
	PRINT_WDT_DBG("%s register success\n", wdt_dev_name);
    return 0;

}

void rt_hw_wdt_init(void)
{
    PRINT_WDT_DBG("%s start\n", __func__);
    fh_wdt_probe(&wdt_obj);
    PRINT_WDT_DBG("%s end\n", __func__);
}
