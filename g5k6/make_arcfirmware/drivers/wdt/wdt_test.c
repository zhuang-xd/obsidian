#include <rtthread.h>
#include <rtdevice.h>
#include <delay.h>
#include "wdt.h"

#define FH_WDT_NAME "fh_wdt0"

void wdt_test(void)
{
	int feed_times = 0;
	int timeout = 5;
	int ret = 0;
	rt_device_t wdt_dev;

	wdt_dev = rt_device_find(FH_WDT_NAME);
	if (wdt_dev == RT_NULL) {
		rt_kprintf("find dev failed\n");
		return;
	}

	rt_kprintf("set wdt timeout\n");
	ret = rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);
	if(ret != RT_EOK) {
		rt_kprintf("set wdt timeout failed\n");
		return;
	}

	rt_kprintf("enable wdt\n");
	ret = rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_START, RT_NULL);
	if(ret != RT_EOK) {
		rt_kprintf("enable wdt failed\n");
		return;
	}

	while (feed_times < 10) {
		rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, RT_NULL);
		rt_kprintf("feed wdt %d", feed_times);
		feed_times++;
		rt_thread_mdelay(1000);
	}
	feed_times = 0;
	while(1) {
		rt_kprintf("stop feed wdt %d", feed_times);
		feed_times++;
		rt_thread_mdelay(1000);
	}
}
// MSH_CMD_EXPORT(wdt_test, test fullhan watchdog);