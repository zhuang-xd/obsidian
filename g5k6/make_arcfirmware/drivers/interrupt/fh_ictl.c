/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12               the first version
 *
 */

#include <rtthread.h>
#include <rthw.h>
#include <fh_arch.h>
#include <fh_clock.h>
#include <asm/io.h>

#ifdef CONFIG_EXT_INTC

#define REG_OFF_INT_IRQ_MASK_STS (0x00)
#define REG_OFF_INT_ENABLE_SET   (0x08)
#define REG_OFF_INT_ENABLE_CLR   (0x0c)

#define REG_APB_CLK_CTRL          (0xb0)
#define REG_INT_CTRL_EN           (1 << 3)

#define INTC_CLK_NAME "intc_clk"

struct ext_int_ctrl
{
    unsigned int reg_base;
    unsigned int self_irq_num; //controller's level_1 interrupt number
    unsigned int virt_irq_num_start;
};

static struct ext_int_ctrl g_int_ctrls[] =
{
    {0x0a500000, 30, 32},
    {0x0a600000, 31, 64},
};

extern struct rt_irq_desc irq_desc[];

void ictl_mask_isr(int irq)
{
    int i;
    int bit_offset;
    struct ext_int_ctrl *pctrl;

    for (i=0; i<sizeof(g_int_ctrls)/sizeof(g_int_ctrls[0]); i++)
    {
        pctrl = &g_int_ctrls[i];
        bit_offset = irq - (int)pctrl->virt_irq_num_start;
        if (bit_offset >= 0 && bit_offset < 32)
        {
            writel(1 << bit_offset, pctrl->reg_base + REG_OFF_INT_ENABLE_CLR);
            break;
        }
    }
}

void ictl_unmask_isr(int irq)
{
    int i;
    int bit_offset;
    struct ext_int_ctrl *pctrl;

    for (i=0; i<sizeof(g_int_ctrls)/sizeof(g_int_ctrls[0]); i++)
    {
        pctrl = &g_int_ctrls[i];
        bit_offset = irq - (int)pctrl->virt_irq_num_start;
        if (bit_offset >= 0 && bit_offset < 32)
        {
            writel(1 << bit_offset, pctrl->reg_base + REG_OFF_INT_ENABLE_SET);
            break;
        }
    }
}

static void ictl_isr(int irqnr, void *param)
{
    unsigned int n;
    unsigned int sts;
    unsigned int irq;
    struct ext_int_ctrl *pctrl = (struct ext_int_ctrl *)param;

    sts = readl(pctrl->reg_base + REG_OFF_INT_IRQ_MASK_STS);

    if (sts)
    {
        do
        {
            n = __rt_ffs(sts) - 1;
            sts &= ~(1 << n);
            irq = n + pctrl->virt_irq_num_start;
            RUNLOG_ADD(0, 0, irq, "irqEnter");
            irq_desc[irq].handler(irq, irq_desc[irq].param);
            RUNLOG_ADD(0, 0, irq, "irqExit");
        } while (sts);
    }
    else
    {
        rt_kprintf("No ictl interrupt!\n");
    }
}

void ictl_init(void)
{
    int i;
    struct ext_int_ctrl *pctrl;
    int ret = 0;

    ret = readl(CPU_SYS_AHB_REG_BASE + REG_APB_CLK_CTRL);
    ret = ret | REG_INT_CTRL_EN;
    writel(ret, CPU_SYS_AHB_REG_BASE + REG_APB_CLK_CTRL);

    for (i=0; i<sizeof(g_int_ctrls)/sizeof(g_int_ctrls[0]); i++)
    {
        pctrl = &g_int_ctrls[i];
        writel(0xffffffff, pctrl->reg_base + REG_OFF_INT_ENABLE_CLR);
        rt_hw_interrupt_install(pctrl->self_irq_num, ictl_isr, pctrl, "ext_int_ctrl");
        rt_hw_interrupt_umask(pctrl->self_irq_num);
    }
}

#ifdef RT_USING_PM
void ictl_resume(void)
{
    int i;
    struct ext_int_ctrl *pctrl;
    int ret = 0;

    ret = readl(CPU_SYS_AHB_REG_BASE + REG_APB_CLK_CTRL);
    ret = ret | REG_INT_CTRL_EN;
    writel(ret, CPU_SYS_AHB_REG_BASE + REG_APB_CLK_CTRL);

    for (i=0; i<sizeof(g_int_ctrls)/sizeof(g_int_ctrls[0]); i++)
    {
        pctrl = &g_int_ctrls[i];
        writel(0xffffffff, pctrl->reg_base + REG_OFF_INT_ENABLE_CLR);
    }
}
#endif

#endif //CONFIG_EXT_INTC
