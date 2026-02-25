/*
 * File      : trap.c
 * This file is part of FH8620 BSP for RT-Thread distribution.
 *
 * Copyright (c) 2016 Shanghai Fullhan Microelectronics Co., Ltd.
 * All rights reserved
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Visit http://www.fullhan.com to get contact with Fullhan.
 *
 * Change Logs:
 * Date           Author       Notes
 */

#include <rthw.h>

/* SAME ORDER WITH _push_register @ macro.S     */
struct rt_hw_register
{
    rt_uint32_t r0;
    rt_uint32_t r1;
    rt_uint32_t r2;
    rt_uint32_t r3;
    rt_uint32_t r4;
    rt_uint32_t r5;
    rt_uint32_t r6;
    rt_uint32_t r7;
    rt_uint32_t r8;
    rt_uint32_t r9;
    rt_uint32_t r10;
    rt_uint32_t r11;
    rt_uint32_t r12;
    rt_uint32_t r13;
    rt_uint32_t r14;
    rt_uint32_t r15;
    rt_uint32_t r16;
    rt_uint32_t r17;
    rt_uint32_t r18;
    rt_uint32_t r19;
    rt_uint32_t r20;
    rt_uint32_t r21;
    rt_uint32_t r22;
    rt_uint32_t r23;
    rt_uint32_t r24;
    rt_uint32_t r25;
    rt_uint32_t r26;
    rt_uint32_t r27;
    //rt_uint32_t sp;     /*r28*/
    rt_uint32_t ilink1; /*r29*/
    rt_uint32_t ilink2; /*r30*/

    rt_uint32_t status32_l1;
    rt_uint32_t status32_l2;
    rt_uint32_t blink;    /*r31*/
};

extern struct rt_thread *rt_current_thread;
/**
 * this function will show registers of CPU
 *
 * @param regs the registers point
 */

void rt_hw_show_register(struct rt_hw_register *regs)
{
    int i;
    rt_uint32_t *reg = &regs->r0;

    rt_kprintf("Thread %s\n", rt_current_thread->name);

    for (i=0; i<28; i+=4)
    {
        rt_kprintf("r%02d: %08x %08x %08x %08x\n", i, reg[0], reg[1], reg[2], reg[3]);
        reg += 4;
    }
    
    rt_kprintf("ILINK2=%08x,BLINK=%08x,STATUS32=%08x\n", regs->ilink2, regs->blink, regs->status32_l2);
}

void rt_hw_trap_memerr(struct rt_hw_register *regs)
{
    rt_kprintf("memory error:\n");
    rt_hw_show_register(regs);

    RUNLOG_ADD(0, 0, 0, "memerr");
    rt_hw_cpu_shutdown();
}

void rt_hw_trap_inserr(struct rt_hw_register *regs)
{
    rt_kprintf("instruction error:\n");
    rt_hw_show_register(regs);

    RUNLOG_ADD(0, 0, 0, "inserr");
    rt_hw_cpu_shutdown();
}

extern struct rt_irq_desc irq_desc[];

void rt_hw_trap_irq(int vector)
{
    vector &= 31; /*for safe*/

    RUNLOG_ADD(0, 0, vector, "irqEnter");

    irq_desc[vector].handler(vector, irq_desc[vector].param);

    RUNLOG_ADD(0, 0, vector, "irqExit");
}

/*@}*/
