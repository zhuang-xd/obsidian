/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-09              the first version
 *
 */

#ifndef __FH_DEF_H__
#define __FH_DEF_H__

#include <asm/io.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

typedef char SINT8;
typedef short SINT16;
typedef int SINT32;
typedef long long SINT64;
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;

#ifndef TYPE_DEFINED
typedef unsigned char uchar;
typedef signed char int8;
typedef unsigned char uint8;
typedef signed short int16;
typedef unsigned short uint16;
typedef signed int int32;
typedef unsigned int uint32;
typedef signed long long int64;
typedef unsigned long long uint64;
typedef float ieee_single;
typedef double ieee_double;

typedef unsigned char boolean;

#define TYPE_DEFINED

#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define SET_REG_B(addr, element, highbit, lowbit) \
    SET_REG_M((addr), ((element) << (lowbit)),    \
              (((1 << ((highbit) - (lowbit) + 1)) - 1) << (lowbit)))

#define GET_REG8(addr) readb(addr)
#define SET_REG8(addr, value) writeb(value, addr)

#define read_reg(addr) readl(addr)
#define write_reg(addr, reg) writel(reg, addr)
#define inw(addr) readl(addr)
#define outw(addr, reg) writel(reg, addr)
#ifndef BIT
#define BIT(nr) (1UL << (nr))
#endif

typedef volatile const unsigned int
    RoReg; /**< Read only 32-bit register (volatile const unsigned int) */
typedef volatile unsigned int
    WoReg; /**< Write only 32-bit register (volatile unsigned int) */
typedef volatile unsigned int
    RwReg; /**< Read-Write 32-bit register (volatile unsigned int) */

#endif /* FH_DEF_H_ */
