/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12     wangyl       add license Apache-2.0
 */

#include "rtthread.h"
#include "rtdebug.h"
#include "fh_chip.h"
#include "fh_pmu.h"
#include "fh_def.h"

#define FH_PMU_WRITEL(offset, value) SET_REG((offset), value)
#define FH_PMU_WRITEL_MASK(offset, value, mask) \
    SET_REG_M((offset), value, mask)
#define FH_PMU_READL(offset) GET_REG((offset))


int fh_pmu_read(rt_uint32_t offset, rt_uint32_t *value)
{
    *value = FH_PMU_READL(offset);
    return 0;
}

int fh_pmu_write(rt_uint32_t offset, const rt_uint32_t value)
{
    FH_PMU_WRITEL(offset, value);
    return 0;
}

int fh_pmu_write_mask(rt_uint32_t offset, const rt_uint32_t value,
                      const rt_uint32_t mask)
{
    FH_PMU_WRITEL_MASK(offset, value, mask);
    return 0;
}

unsigned int fh_pmu_get_reg(unsigned int offset)
{
    return FH_PMU_READL(offset);
}

void fh_pmu_set_reg(unsigned int offset, unsigned int data)
{
    //fh_set_scu_en();
    FH_PMU_WRITEL(offset, data);
    //fh_set_scu_di();
}

void fh_pmu_set_reg_m(unsigned int offset, unsigned int data, unsigned int mask)
{
    fh_pmu_set_reg(offset, (fh_pmu_get_reg(offset) & (~(mask))) |
                      ((data) & (mask)));
}

unsigned int fh_pmu_get_ddrsize(void)
{
#ifdef REG_PMU_DDR_SIZE
    return fh_pmu_get_reg(REG_PMU_DDR_SIZE);
#else
    return 0;
#endif
}
