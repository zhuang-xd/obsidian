/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <rtdevice.h>
#include <rthw.h>
#include <fh_chip.h>
#include <fh_def.h>
#include <delay.h>
#include <asm/io.h>

#ifndef REG_PTSLO
#define REG_PTSLO (PMU_REG_BASE + REG_PMU_PTSLO)
#endif

#ifndef REG_PTSHI
#define REG_PTSHI (PMU_REG_BASE + REG_PMU_PTSHI)
#endif

#if defined(CONFIG_CHIP_MC632X)
#define REG_PTS_UPDATE (0x2c300000)
#define REG_PTS_CTRL1  (0x2c10007c)
#define REG_PTS_CTRL2  (0x2c100080)
uint64 read_pts(void)
{
    unsigned long long pts;

    writel(0, REG_PTS_UPDATE);
    pts = readl(REG_PTS_CTRL2);
    pts = (pts << 32) | readl(REG_PTS_CTRL1);
    return pts;
}

uint32 read_pts32(void)
{
    // writel(0x01, REG_PTSLO);
    return readl(0x0970000C);
}

static unsigned long long pts_offset = 0;
uint64 read_pts_new(void)
{
    return read_pts() + pts_offset;
}

void update_pts_offset(unsigned int offset)
{
    pts_offset += offset;
}
#else
uint64 read_pts(void)
{
    unsigned long long pts;

    writel(0x01, REG_PTSLO);
    pts = readl(REG_PTSHI);
    pts = (pts << 32) | readl(REG_PTSLO);
    return pts;
}

uint32 read_pts32(void)
{
	writel(0x01, REG_PTSLO);

	return readl(REG_PTSLO);
}

#endif

void udelay(int us)
{
    uint32 t;
    int diff;

    t = read_pts32();
    do 
    {
        diff = read_pts32() - t;
    } while (diff <= us);
}

int usleep(useconds_t usec)
{
    unsigned int msec;
    unsigned int tick;

    if (usec > 5000)
    {
        msec = usec / 1000;
        tick = (msec + (1000/RT_TICK_PER_SECOND - 1)) / (1000/RT_TICK_PER_SECOND);
        rt_thread_delay(tick);
    }
    else
    {
        udelay(usec);
    }

    return 0;
}
