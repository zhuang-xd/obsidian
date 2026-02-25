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

#ifndef __FH_ARCH_H__
#define __FH_ARCH_H__

#if defined(CONFIG_CHIP_FH8852)
#include <fh8852/arch.h>
#endif

#if defined(CONFIG_CHIP_FH8856)
#include <fh8856/arch.h>
#endif

#if defined(CONFIG_CHIP_FH8626V100)
#include <fh8626v100/arch.h>
#endif

#if defined(CONFIG_ARCH_FH885xV200)
#include <fh8856v200/arch.h>
#endif

#if defined(CONFIG_CHIP_FH8656)
#include <fh8656/arch.h>
#endif

#if defined(CONFIG_CHIP_FHCH)
#include <fhch/arch.h>
#endif

#if defined(CONFIG_CHIP_FHCH2)
#include <fhch2/arch.h>
#endif

#if defined(CONFIG_CHIP_FHS16)
#include <fhs16/arch.h>
#endif

#if defined(CONFIG_CHIP_FHZTV2)
#include <fhztv2/arch.h>
#endif

#if defined(CONFIG_ARCH_FHXGM)
#include <fhxgm/arch.h>
#endif

#if defined(CONFIG_ARCH_FHXGM2)
#include <fhxgm2/arch.h>
#endif

#if defined(CONFIG_ARCH_MC632X)
#include <mc632x/arch.h>
#endif

#endif /* __FH_ARCH_H__ */
