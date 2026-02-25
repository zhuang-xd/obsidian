/*
 * File      : cpuport.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2013, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date         Author      Notes
 * 2009-01-05   Bernard     first version
 * 2011-02-14   onelife     Modify for EFM32
 * 2011-06-17   onelife     Merge all of the C source code into cpuport.c
 * 2012-12-23   aozima      stack addr align to 8byte.
 * 2012-12-29   Bernard     Add exception hook.
 * 2013-07-09   aozima      enhancement hard fault exception handler.
 */

#include <rthw.h>
#include <rtthread.h>
#include <asm/arc6xx.h>

/*
 * stack frame over view:
 *    this stack is used only for task switch, just used in thread create.
 *  for normal task switch, we needn't save r0-r12, we only need to save
 *  r13-r25, blink(r31), fp(r27), and the stack pointer sp(r28), totally
 *  18 registers.
 *
 */
struct stack_frame
{
//	rt_uint32_t r28;			// stack pointer
    rt_uint32_t stack_type;		// stack type.
    rt_uint32_t lp_count;
    rt_uint32_t lp_start;
    rt_uint32_t lp_end;			// 3 loop register, totally 28+2+1+3=34 register
    /* r4 ~ r11 register */
    rt_uint32_t macmode;		// mac mode, dsp register
    rt_uint32_t xmaclw_h;		// 32x16 accumulator, high
    rt_uint32_t xmaclw_l;		// 32x16 accumulator, low
    rt_uint32_t acc1;			// ACC1, or r56
    rt_uint32_t acc2;			// ACC2, or r57
    rt_uint32_t r0;				// r0 must be reserved to save thread parameter.
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
    rt_uint32_t r13;	// r0-r12, needn't to be saved for normal task switch.
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
    // rt_uint32_t r26;		
    rt_uint32_t fp;				// r27
    rt_uint32_t blink;			// r31
    rt_uint32_t status32_l1;	
    rt_uint32_t ilink1;			// r29
};

enum {
	STACK_TYPE_TASK   = 0x4b534154,
	STACK_TYPE_ILINK1 = 0x314b4e4c,
	STACK_TYPE_ILINK2 = 0x324b4e4c		// really, ilink2 is not supported.
};

/* flag in interrupt handling */
rt_uint32_t rt_interrupt_from_thread;
rt_uint32_t rt_interrupt_to_thread;
rt_uint32_t rt_thread_switch_interrupt_flag;

extern void machine_shutdown(void);

/**
 * This function will initialize thread stack
 *
 * @param tentry the entry of thread
 * @param parameter the parameter of entry
 * @param stack_addr the beginning stack address
 * @param texit the function will be called when thread exit
 *
 * @return stack address
 */
rt_uint8_t *rt_hw_stack_init(void       *tentry,
                             void       *parameter,
                             rt_uint8_t *stack_addr,
                             void       *texit)
{
    struct stack_frame *stack_frame;
    rt_uint8_t         *stk;
    unsigned long       i;

    stk  = stack_addr + sizeof(rt_uint32_t);
    stk  = (rt_uint8_t *)RT_ALIGN_DOWN((rt_uint32_t)stk, 8);
    stk -= sizeof(struct stack_frame);

    stack_frame = (struct stack_frame *)stk;

    /* init all register */
    for (i = 0; i < sizeof(struct stack_frame) / sizeof(rt_uint32_t); i ++)
    {
        ((rt_uint32_t *)stack_frame)[i] = 0xdeadbeef;
    }

    stack_frame->stack_type = (rt_uint32_t)STACK_TYPE_ILINK1;
    stack_frame->r0			= (rt_uint32_t)parameter;
    stack_frame->blink	 	= (rt_uint32_t)texit;
    stack_frame->ilink1		= (rt_uint32_t)tentry;
    stack_frame->status32_l1= (rt_uint32_t)6;

    /* return task's current stack address */
    return stk;
}

/**
 * shutdown CPU
 */
void rt_hw_cpu_shutdown(void)
{
    rt_kprintf("shutdown...\n");

    machine_shutdown();

    RT_ASSERT(0);
}

#ifdef RT_USING_CPU_FFS
/**
 * This function finds the first bit set (beginning with the least significant bit)
 * in value and return the index of that bit.
 *
 * Bits are numbered starting at 1 (the least significant bit).  A return value of
 * zero from any of these functions means that the argument was zero.
 *
 * @return return the index of the first bit set. If value is 0, then this function
 * shall return 0.
 */
#if defined(__CC_ARM)
__asm int __rt_ffs(int value)
{
	// not supported.
    CMP     r0, #0x00
    BEQ     exit
    RBIT    r0, r0
    CLZ     r0, r0
    ADDS    r0, r0, #0x01

    exit
    BX      lr
}
#elif defined(__IAR_SYSTEMS_ICC__)
int __rt_ffs(int value)
{
	// not supported.
    if (value == 0) return value;

    __ASM("RBIT r0, r0");
    __ASM("CLZ  r0, r0");
    __ASM("ADDS r0, r0, #0x01");
}
#elif defined(__GNUC__)
int __rt_ffs(int value)
{
    return __builtin_ffs(value);
}
#endif

#endif
