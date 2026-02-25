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

#ifndef __FH_PMU_H__
#define __FH_PMU_H__

#include <rtdef.h>

#define SYS_REG_V2P(va) (va)
#define SYS_REG_P2V(pa) (pa)

int fh_pmu_read(rt_uint32_t offset, rt_uint32_t *value);
int fh_pmu_write(rt_uint32_t offset, const rt_uint32_t value);
int fh_pmu_write_mask(rt_uint32_t offset, const rt_uint32_t value,
                      const rt_uint32_t mask);

unsigned int fh_pmu_get_reg(unsigned int offset);
void fh_pmu_set_reg(unsigned int offset, unsigned int data);
void fh_pmu_set_reg_m(unsigned int offset, unsigned int data, unsigned int mask);
unsigned int fh_pmu_get_ddrsize(void);
#endif /* FH_PMU_H_ */
