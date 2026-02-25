/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-03-18     Bernard      the first version
 * 2006-04-25     Bernard      add rt_hw_context_switch_interrupt declaration
 * 2006-09-24     Bernard      add rt_hw_context_switch_to declaration
 * 2012-12-29     Bernard      add rt_hw_exception_install declaration
 * 2017-10-17     Hichard      add some micros
 */

#ifndef __RT_HW_H__
#define __RT_HW_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_EXT_INTC
#define MAX_HANDLERS (256)
#else
#define MAX_HANDLERS (32) /*after 32 is virtual*/
#endif

#define GENERAL_INT_NUM 32
#define EXT_INT_NUM 64


void rt_hw_cpu_reset(void);
void rt_hw_cpu_shutdown(void);

rt_uint8_t *rt_hw_stack_init(void       *entry,
                             void       *parameter,
                             rt_uint8_t *stack_addr,
                             void       *exit);

/*
 * Interrupt handler definition
 */
typedef void (*rt_isr_handler_t)(int vector, void *param);

struct irq_chip
{
    const char      *name;
    void            (*irq_ack)(unsigned int irq);
    void            (*irq_mask)(unsigned int irq);
    void            (*irq_unmask)(unsigned int irq);
    void            (*irq_eoi)(unsigned int irq);
};

struct rt_irq_desc
{
    rt_isr_handler_t handler;
    void            *param;
    unsigned int intc_flag;
    struct irq_chip *data;
};

/*
 * Interrupt interfaces
 */
void rt_hw_interrupt_init(void);
void rt_hw_interrupt_mask(int vector);
void rt_hw_interrupt_umask(int vector);
rt_isr_handler_t rt_hw_interrupt_install(int              vector,
                                         rt_isr_handler_t handler,
                                         void            *param,
                                         const char      *name);

rt_base_t rt_hw_interrupt_disable(void);
void rt_hw_interrupt_enable(rt_base_t level);
int rt_hw_enable_irq_wake(unsigned int irq);
int rt_hw_disable_irq_wake(unsigned int irq);

int irq_set_chip(unsigned int irq, struct irq_chip *chip);
int generic_handle_irq(unsigned int irq);

/*
 * Context interfaces
 */
void rt_hw_context_switch(rt_uint32_t from, rt_uint32_t to);
void rt_hw_context_switch_to(rt_uint32_t to);
void rt_hw_context_switch_interrupt(rt_uint32_t from, rt_uint32_t to);

void rt_hw_console_output(const char *str);

void rt_hw_backtrace(rt_uint32_t *fp, rt_uint32_t thread_entry);
void rt_hw_show_memory(rt_uint32_t addr, rt_uint32_t size);

/*
 * Exception interfaces
 */
void rt_hw_exception_install(rt_err_t (*exception_handle)(void *context));

/*
 * delay interfaces
 */
void rt_hw_us_delay(rt_uint32_t us);

#define RT_DEFINE_SPINLOCK(x)  
#define RT_DECLARE_SPINLOCK(x)    rt_ubase_t x

#define rt_hw_spin_lock(lock)     *(lock) = rt_hw_interrupt_disable()
#define rt_hw_spin_unlock(lock)   rt_hw_interrupt_enable(*(lock))

#ifdef __cplusplus
}
#endif

#endif
