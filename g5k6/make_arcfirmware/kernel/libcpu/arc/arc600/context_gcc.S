/*
 * File      : context_gcc.S
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
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
 * Change Logs:
 * Date           Author       Notes
 * 2016-10-18     wuzh     create file for ARC
 */

.file       "context_gcc.S"
.include    "macro.S"

/*
 * rt_base_t rt_hw_interrupt_disable();
 */
.global rt_hw_interrupt_disable
rt_hw_interrupt_disable:
    lr  r0, [0x0a]
    and r1, r0, ~6		; get the e1/e2 bit
    flag r1
    j_s [blink]
    nop_s


/*
 * void rt_hw_interrupt_enable(rt_base_t level);
 */
.global rt_hw_interrupt_enable
rt_hw_interrupt_enable:
    flag r0
    j_s [blink]
    nop_s


/*
 * void rt_hw_context_switch(rt_uint32 from, rt_uint32 to);
 * r0 --> &from_thread->sp
 * r1 --> &to_thread->sp
 */
.global rt_hw_context_switch
rt_hw_context_switch:
    _push_task_regs
    _push_task_type 0   /* task_type = 0 (r3 used) */

    st      sp, [r0]    /* store sp in preempted (from) tasks TCB */
    ld      sp, [r1]    /* get new (to) task stack pointer */

    _pop_task_type     /* r3 = task_type */

    mov     r2, TASK_TYPE_0
    cmp     r3, r2
    beq     _to_task_type0

_to_task_type1:
    _pop_isr_regs 1     /* to interrupt task */
    j.f     [ilink1]
    nop_s

_to_task_type0:
    _pop_task_regs      /* to normal task */
    j_s     [blink]
    nop_s


.global tick_config
tick_config:
    sr 0, [0x22]        ; disable timer0
    sr 0, [0x21]        ; set tick count
    sr r0, [0x23]       ; set limit count

    j  [blink]

/*
 * void rt_hw_context_switch_to(rt_uint32 to);
 * r0 --> to
 */
.global rt_hw_context_switch_to
rt_hw_context_switch_to:
    ld      sp, [r0]		/* get new (to) task stack pointer */

    _pop_task_type          /* r3 = task_type */

    sr 3, [0x22]            ; enable timer

    mov     r2, TASK_TYPE_0
    cmp     r3, r2
    beq     _to_task_type0

    j       _to_task_type1
    nop_s



/*
 * void rt_hw_context_switch_interrupt(rt_uint32 from, rt_uint32 to);
 */
.global rt_thread_switch_interrupt_flag
.global rt_interrupt_from_thread
.global rt_interrupt_to_thread
.global rt_hw_context_switch_interrupt
rt_hw_context_switch_interrupt:
    ld  r2, [rt_thread_switch_interrupt_flag]       /* if rt_thread_switch_interrupt_flag != 0      */
    brne.d  r2, 0, _reswitch                        /*      goto reswitch                           */
    mov_s   r3, 0x1                                 /* else                                         */
    st  r3, [rt_thread_switch_interrupt_flag]       /*      rt_thread_switch_interrupt_flag = 1     */
    st  r0, [rt_interrupt_from_thread]				/*      rt_interrupt_from_thread = from         */
_reswitch:
    st  r1, [rt_interrupt_to_thread]                /* rt_interrupt_to_thread = to                  */
    j_s     [blink]
    nop_s



