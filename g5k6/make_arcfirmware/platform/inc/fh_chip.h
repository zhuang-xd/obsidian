/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-09     songyh    the first version
 *
 */

#ifndef __FH_CHIP_H__
#define __FH_CHIP_H__

#if defined(CONFIG_CHIP_FH8852)
#include <fh8852/chip.h>
#endif

#if defined(CONFIG_CHIP_FH8856)
#include <fh8856/chip.h>
#endif

#if defined(CONFIG_CHIP_FH8626V100) //ZT
#include <fh8626v100/chip.h>
#endif


#if defined(CONFIG_CHIP_FH8656) //WS
#include <fh8656/chip.h>
#endif

#if defined(CONFIG_CHIP_FHCH)
#include <fhch/chip.h>
#endif

#if defined(CONFIG_CHIP_FHCH2)
#include <fhch2/chip.h>
#endif

#if defined(CONFIG_CHIP_FHS16)
#include <fhs16/chip.h>
#endif

#if defined(CONFIG_ARCH_FH885xV200) //HL
#include <fh8856v200/chip.h>
#endif

#if defined(CONFIG_CHIP_FHZTV2) //ZTV2
#include <fhztv2/chip.h>
#endif

#if defined(CONFIG_ARCH_FHXGM)
#include <fhxgm/chip.h>
#endif

#if defined(CONFIG_ARCH_FHXGM2)
#include <fhxgm2/chip.h>
#endif

#if defined(CONFIG_ARCH_MC632X)
#include <mc632x/chip.h>
#endif

#endif /* FH_ARCH_H_ */
