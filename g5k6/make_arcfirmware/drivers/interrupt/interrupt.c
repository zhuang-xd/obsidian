/*
 * File      : interrupt.c
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

/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include <rtthread.h>
#include <rthw.h>
#include <fh_def.h>
#include <fh_chip.h>
#include <asm/io.h>
#ifdef RT_USING_PM
#include <pm.h>
#endif

extern rt_uint32_t rt_interrupt_nest;
extern rt_uint32_t rt_interrupt_from_thread;
extern rt_uint32_t rt_interrupt_to_thread;
extern rt_uint32_t rt_thread_switch_interrupt_flag;

#ifdef REG_SUB_CPU_INTC_MASK
#define ABS_REG_ARC_INTC_MASK (REG_SUB_CPU_INTC_MASK)
#else
#define ABS_REG_ARC_INTC_MASK (PMU_REG_BASE + REG_PMU_ARC_INTC_MASK)
#endif

#if defined(CONFIG_CHIP_MC632X)
#define ZERO_DISABLE_INTR (1)
#define INTR_N_OFFSET     (4)
#else
#define ZERO_DISABLE_INTR (0)
#define INTR_N_OFFSET     (0)
#endif

#define INTC_FLAG_WAKEUP        BIT(0)
#define INTC_FLAG_MASK          BIT(1)
#define INTC_FLAG_UMASK         BIT(2)

struct rt_irq_desc irq_desc[MAX_HANDLERS];

/**
 * This function will initialize hardware interrupt
 */

struct rt_irq_desc *irq_to_desc(unsigned int irq)
{
    if (irq >= MAX_HANDLERS)
        return RT_NULL;
    return &irq_desc[irq];
}

static void rt_hw_interrupt_handle(int vector, void *param)
{
    rt_kprintf("Unhandled interrupt %d!\n", vector);
}

int rt_hw_enable_irq_wake(unsigned int irq)
{
    register rt_base_t level;

    if (irq >= MAX_HANDLERS)
        return -1;

    level = rt_hw_interrupt_disable();
    irq_desc[irq].intc_flag |= INTC_FLAG_WAKEUP;
    rt_hw_interrupt_enable(level);

    return 0;
}

int rt_hw_disable_irq_wake(unsigned int irq)
{
    register rt_base_t level;

    if (irq >= MAX_HANDLERS)
        return -1;

    level = rt_hw_interrupt_disable();
    irq_desc[irq].intc_flag &= (~INTC_FLAG_WAKEUP);
    rt_hw_interrupt_enable(level);

    return 0;
}

/**
 * This function will mask a interrupt.
 * @param vector the interrupt number
 */
void rt_hw_interrupt_mask(int irq)
{
    unsigned int val;
    register rt_base_t level;
    struct rt_irq_desc *desc;
    struct irq_chip *chip;

    if (irq == TMR1_IRQn)
    {
        fh_hw_writeaux(0x22, 0);
        irq_desc[irq].intc_flag |= INTC_FLAG_MASK;
        return;
    }

    level = rt_hw_interrupt_disable();

    if (irq >= GENERAL_INT_NUM + EXT_INT_NUM)/*only for gpio*/
    {
        desc = irq_to_desc(irq);
        if (desc)
        {
            chip = desc->data;
            chip->irq_mask(irq);
            irq_desc[irq].intc_flag |= INTC_FLAG_MASK;
        }
        rt_hw_interrupt_enable(level);
        return;
    }

    if (irq >= GENERAL_INT_NUM)
    {
#ifdef CONFIG_EXT_INTC
        extern void ictl_mask_isr(int irq);
        ictl_mask_isr(irq);
        irq_desc[irq].intc_flag |= INTC_FLAG_MASK;
#endif
        rt_hw_interrupt_enable(level);
        return;
    }

    if (ZERO_DISABLE_INTR)
    {
        if (irq < INTR_N_OFFSET)
        {
            irq_desc[irq].intc_flag |= INTC_FLAG_MASK;
            rt_hw_interrupt_enable(level);
            return;
        }
    }

    val = readl(ABS_REG_ARC_INTC_MASK);
    if (ZERO_DISABLE_INTR)
    {
        val &= ~(1 << (irq - INTR_N_OFFSET));
    }
    else
    {
        val |= (1 << irq);
    }
    writel(val, ABS_REG_ARC_INTC_MASK);
    irq_desc[irq].intc_flag |= INTC_FLAG_MASK;
    rt_hw_interrupt_enable(level);
}

void rt_hw_interrupt_umask(int irq)
{
    unsigned int val;
    register rt_base_t level;
    struct rt_irq_desc *desc;
    struct irq_chip *chip;

    if (irq == TMR1_IRQn)
    {
        fh_hw_writeaux(0x22, 3);
        irq_desc[irq].intc_flag &= (~INTC_FLAG_MASK);
        return;
    }

    level = rt_hw_interrupt_disable();

    if (irq >= 96)/*only for gpio*/
    {
        desc = irq_to_desc(irq);
        if (desc)
        {
            chip = desc->data;
            chip->irq_unmask(irq);
            irq_desc[irq].intc_flag &= (~INTC_FLAG_MASK);
        }
        rt_hw_interrupt_enable(level);
        return;
    }

    if (irq >= 32)
    {
#ifdef CONFIG_EXT_INTC
        extern void ictl_unmask_isr(int irq);
        ictl_unmask_isr(irq);
        irq_desc[irq].intc_flag &= (~INTC_FLAG_MASK);
#endif
        rt_hw_interrupt_enable(level);
        return;
    }

    if (ZERO_DISABLE_INTR)
    {
        if (irq < INTR_N_OFFSET)
        {
            irq_desc[irq].intc_flag &= (~INTC_FLAG_MASK);
            rt_hw_interrupt_enable(level);
            return;
        }
    }

    val = readl(ABS_REG_ARC_INTC_MASK);
    if (ZERO_DISABLE_INTR)
    {
        val |= (1 << (irq - INTR_N_OFFSET));
    }
    else
    {
        val &= ~(1 << irq);
    }
    writel(val, ABS_REG_ARC_INTC_MASK);
    irq_desc[irq].intc_flag &= (~INTC_FLAG_MASK);
    rt_hw_interrupt_enable(level);
}

/**
 * irq_set_chip - set the irq chip for an irq
 * @irq:    irq number
 * @chip:   pointer to irq chip description structure
 */
int irq_set_chip(unsigned int irq, struct irq_chip *chip)
{
    struct rt_irq_desc *desc = irq_to_desc(irq);

    if (desc)
        desc->data = chip;

    return 0;
}

/**
 * This function will install a interrupt service routine to a interrupt.
 * @param vector the interrupt number
 * @param handler the interrupt service routine to be installed
 * @param param the interrupt service function parameter
 * @param name the interrupt name
 * @return old handler
 */
rt_isr_handler_t rt_hw_interrupt_install(int vector, rt_isr_handler_t handler,
        void *param, const char *name)
{
    rt_isr_handler_t old_handler = RT_NULL;

    if (vector < MAX_HANDLERS)
    {
        old_handler = irq_desc[vector].handler;
        if (handler != RT_NULL)
        {
            irq_desc[vector].handler = (rt_isr_handler_t)handler;
            irq_desc[vector].param   = param;
        }
    }

    return old_handler;
}

#ifdef RT_USING_PM
int rt_hw_irq_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    int idx = 0;

    for (idx = 0; idx < MAX_HANDLERS; idx++)
    {
        if (!(irq_desc[idx].intc_flag & (INTC_FLAG_MASK | INTC_FLAG_WAKEUP)))
        {
            irq_desc[idx].intc_flag |= INTC_FLAG_UMASK;
            rt_hw_interrupt_mask(idx);
        }
    }

    return RT_EOK;
}

void rt_hw_irq_resume(const struct rt_device *device, rt_uint8_t mode)
{
    int idx = 0;

    /*mask all interrupt*/
    if (ZERO_DISABLE_INTR)
    {
        writel(0, ABS_REG_ARC_INTC_MASK);
    }
    else
    {
        writel(~0UL, ABS_REG_ARC_INTC_MASK);
    }

extern void ictl_resume(void);
    ictl_resume();

    for (idx = 0; idx < MAX_HANDLERS; idx++)
    {
        if (irq_desc[idx].intc_flag & (INTC_FLAG_UMASK | INTC_FLAG_WAKEUP))
        {
            rt_hw_interrupt_umask(idx);
            irq_desc[idx].intc_flag &= (~INTC_FLAG_UMASK);
        }
    }
}

struct rt_device_pm_ops hw_irq_pm_ops =
{
    .suspend = rt_hw_irq_suspend,
    .resume = rt_hw_irq_resume
};
#endif

/**
 * This function will initialize hardware interrupt
 */
struct rt_device * interrupt_device;
void rt_hw_interrupt_init(void)
{
    register rt_uint32_t idx;

    /*mask all interrupt*/
    if (ZERO_DISABLE_INTR)
    {
        writel(0, ABS_REG_ARC_INTC_MASK);
    }
    else
    {
        writel(~0UL, ABS_REG_ARC_INTC_MASK);
    }

    /* init exceptions table */
    for (idx = 0; idx < MAX_HANDLERS; idx++)
    {
        irq_desc[idx].handler      = rt_hw_interrupt_handle;
        irq_desc[idx].param        = RT_NULL;
        irq_desc[idx].intc_flag    = INTC_FLAG_MASK;
    }

    /* init interrupt nest, and context in thread sp */
    rt_interrupt_nest               = 0;
    rt_interrupt_from_thread        = 0;
    rt_interrupt_to_thread          = 0;
    rt_thread_switch_interrupt_flag = 0;

#ifdef CONFIG_EXT_INTC
    extern void ictl_init(void);
    ictl_init();
#endif
    interrupt_device = (struct rt_device *)rt_malloc(sizeof (struct rt_device));
    if (interrupt_device == RT_NULL)
    {
        rt_kprintf("malloc interrupt_device failed.\n");
        return;
    }

#ifdef RT_USING_PM
    rt_pm_device_register(interrupt_device, &hw_irq_pm_ops);
#endif
}
int generic_handle_irq(unsigned int irq)
{
    struct rt_irq_desc *desc = irq_to_desc(irq);

    if (desc)
        desc->handler(irq, desc->param);
    return 0;
}
