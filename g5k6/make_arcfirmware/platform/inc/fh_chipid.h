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

#ifndef __FH_CHIPID_H__
#define __FH_CHIPID_H__

#include <rtdef.h>

#define FH_CHIP_FH8830          0x883000A1
#define FH_CHIP_FH8630M         0x883000B1
#define FH_CHIP_FH8632          0x863200A1
#define FH_CHIP_FH8632v2        0x863200A2
#define FH_CHIP_FH8856          0x885600A1
#define FH_CHIP_FH8852          0x885600B1
#define FH_CHIP_FH8626V100      0x8626A100
#define FH_CHIP_FH8858V310      0x8858A310
#define FH_CHIP_FH8856V310      0x8856A310
#define FH_CHIP_FH8852V310      0x8852A310
#define FH_CHIP_MC6322          0x6322A100
#define FH_CHIP_MC6326          0x6326A100
#define FH_CHIP_MC6322L         0x6322B100
#define FH_CHIP_MC6326L         0x6326B100
#define FH_CHIP_MC6321          0x6321A100
#define FH_CHIP_MC6321L         0x6321B100
#define FH_CHIP_MC3303          0x3303A100

struct fh_chip_info
{
    int _plat_id; /* 芯片寄存器中的plat_id */
    int _chip_id; /* 芯片寄存器中的chip_id */
    int _chip_mask; /* 芯片寄存器中的chip_id */
    int chip_id; /* 芯片chip_id，详见上述定义 */
    int ddr_size; /* 芯片DDR大小，单位Mbit */
    char chip_name[32]; /* 芯片名称 */
};

unsigned int fh_is_8830(void);
unsigned int fh_is_8632(void);
unsigned int fh_is_8852(void);
unsigned int fh_is_8856(void);
unsigned int fh_is_8626v100(void);
unsigned int fh_is_6322(void);
unsigned int fh_is_6326(void);
unsigned int fh_is_6322l(void);
unsigned int fh_is_6326l(void);
unsigned int fh_is_6321(void);
unsigned int fh_is_6321l(void);
unsigned int fh_is_3303(void);

#endif /* __FH_CHIPID_H__ */
