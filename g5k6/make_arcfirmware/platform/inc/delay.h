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

#ifndef __delayx_h__
#define __delayx_h__

typedef unsigned long useconds_t;

extern void udelay(int us);
extern int usleep(useconds_t usec);

#endif /* __delayx_h__ */
