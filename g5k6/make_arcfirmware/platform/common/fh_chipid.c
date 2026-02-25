/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-08-20     wangyl       add license Apache-2.0
 */

#include <rtthread.h>
#include <fh_chip.h>
#include <fh_pmu.h>
#include <fh_chipid.h>
#include "fh_def.h"

#define CHIP_INFO(__plat_id, __chip_id, __chip_mask, chip, size) \
    { \
        ._plat_id = __plat_id, \
        ._chip_id = __chip_id, \
        ._chip_mask = __chip_mask, \
        .chip_id = FH_CHIP_##chip, \
        .ddr_size = size, \
        .chip_name = #chip, \
    },


#define RD_REG      0xffffffff

static struct fh_chip_info chip_infos[] = {
    CHIP_INFO(0x17092901, 0xC, 0xF, FH8852, 512)
    CHIP_INFO(0x17092901, 0xD, 0xF, FH8856, 1024)
    CHIP_INFO(0x18112301, 0x3, 0x3, FH8626V100, 512)
    CHIP_INFO(0x22071801, 0x00410007, 0x00FFFFFF, FH8858V310, RD_REG)
    CHIP_INFO(0x22071801, 0x00100007, 0x00FFFFFF, FH8856V310, RD_REG)
    CHIP_INFO(0x22071801, 0x00000007, 0x00FFFFFF, FH8852V310, RD_REG)
    CHIP_INFO(0x20122801, 0x0000000a, 0x00FFFFFF, MC6322, RD_REG)
    CHIP_INFO(0x20122801, 0x0001000a, 0x00FFFFFF, MC6322L, RD_REG)
    CHIP_INFO(0x20122801, 0x0002000a, 0x00FFFFFF, MC6326L, RD_REG)
    CHIP_INFO(0x20122801, 0x0003000a, 0x00FFFFFF, MC6326, RD_REG)
    CHIP_INFO(0x20122801, 0x0000020a, 0x00FFFFFF, MC6322, RD_REG)
    CHIP_INFO(0x20122801, 0x0000170a, 0x00FFFFFF, MC6321, RD_REG)
    CHIP_INFO(0x20122801, 0x0000140a, 0x00FFFFFF, MC6321L, RD_REG)
    CHIP_INFO(0x20122801, 0x0000060a, 0x00FFFFFF, MC3303, RD_REG)
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

void fh_get_chipid(rt_uint32_t *plat_id, rt_uint32_t *chip_id)
{
    if (plat_id != RT_NULL)
        *plat_id  = read_reg(REG_PMU_CHIP_ID);

    if (chip_id != RT_NULL)
        *chip_id = read_reg(REG_PMU_IP_VER);
}

#define FH_GET_CHIP_ID(plat_id, chip_id) \
    rt_uint32_t plat_id = 0;\
    rt_uint32_t chip_id = 0;\
    fh_get_chipid(&plat_id, &chip_id)


struct fh_chip_info *fh_get_chip_info(void)
{
    static struct fh_chip_info *chip_info = RT_NULL;
    struct fh_chip_info *info = RT_NULL;
    rt_uint32_t plat_id = 0;
    rt_uint32_t chip_id = 0;
    int i = 0;

    if (chip_info != RT_NULL)
        return chip_info;

    fh_get_chipid(&plat_id, &chip_id);
#ifdef REG_PMU_CHIP_INFO
    chip_id = read_reg(REG_PMU_CHIP_INFO);
#endif

    for (i = 0; i < ARRAY_SIZE(chip_infos); i++)
    {
        info = &chip_infos[i];
        if (plat_id == info->_plat_id && (chip_id & info->_chip_mask) == info->_chip_id)
        {
            chip_info = info;
            if (chip_info->ddr_size == RD_REG)
                chip_info->ddr_size = fh_pmu_get_ddrsize();
            return info;
        }
    }

    return RT_NULL;
}

unsigned int fh_get_ddrsize_mbit(void)
{
    struct fh_chip_info *info = fh_get_chip_info();

    if (info)
        return info->ddr_size;
    return 0;
}

char *fh_get_chipname(void)
{
    struct fh_chip_info *info = fh_get_chip_info();

    if (info)
        return info->chip_name;
    return "UNKNOWN";
}

#define DEFINE_FUNC_FH_IS(name, chip) \
unsigned int fh_is_##name(void) \
{ \
    struct fh_chip_info *info = fh_get_chip_info(); \
 \
    if (info) \
        return info->chip_id == FH_CHIP_##chip; \
    return 0; \
}

DEFINE_FUNC_FH_IS(8830, FH8830);
DEFINE_FUNC_FH_IS(8852, FH8852);
DEFINE_FUNC_FH_IS(8856, FH8856);
DEFINE_FUNC_FH_IS(8626v100, FH8626V100);
DEFINE_FUNC_FH_IS(6322, MC6322);
DEFINE_FUNC_FH_IS(6326, MC6326);
DEFINE_FUNC_FH_IS(6322L, MC6322L);
DEFINE_FUNC_FH_IS(6326L, MC6326L);
DEFINE_FUNC_FH_IS(6321, MC6321);
DEFINE_FUNC_FH_IS(6321L, MC6321L);
DEFINE_FUNC_FH_IS(3303, MC3303);

void fh_print_chip_info(void)
{
    FH_GET_CHIP_ID(plat_id, chip_id);
    rt_kprintf("chip_name\t: %s\n", fh_get_chipname());
    rt_kprintf("ddr_size\t: %dMbit\n", fh_get_ddrsize_mbit());
    rt_kprintf("plat_id\t\t: 0x%x\npkg_id\t\t: 0x%x\n",
            plat_id, chip_id);
}
