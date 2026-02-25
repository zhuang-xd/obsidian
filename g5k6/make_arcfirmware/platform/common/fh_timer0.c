#include "rtthread.h"
#include "rtdebug.h"
#include "fh_chip.h"
#include "fh_pmu.h"
#include "fh_def.h"
#include "rthw.h"

#define TMR0_LOAD_LO            (TIMER0_REG_BASE + 0x00)
#define TMR0_VALUE_LO           (TIMER0_REG_BASE + 0x08)
#define TMR0_CTL                (TIMER0_REG_BASE + 0x10)
#define TMR0_INT                (TIMER0_REG_BASE + 0x14)
#define TMR0_VALUE_SHDW_LO      (TIMER0_REG_BASE + 0x18)

#define TMR1_LOAD_LO    (TIMER0_REG_BASE + 0x20)
#define TMR1_CTL        (TIMER0_REG_BASE + 0x30)
#define TMR1_INT        (TIMER0_REG_BASE + 0x34)

#define TMR2_LOAD_LO    (TIMER0_REG_BASE + 0x40)
#define TMR2_CTL        (TIMER0_REG_BASE + 0x50)
#define TMR2_INT        (TIMER0_REG_BASE + 0x54)

#define TMR_IRQ_RAW     (1 << 1)

static void (*fh_timer0_callback)(int id);

static void timer0_handler(int vector, void *param)
{
    // rt_kprintf("[%s-%d]\n", __func__, __LINE__);
    int i = 0;
    int reg_val = 0;

    for (i = 0; i < 3; i++)
    {
        reg_val = readl(TMR0_INT + (i * 0x20));
        if (reg_val & TMR_IRQ_RAW)
        {
            writel(0x8, TMR0_INT + (i * 0x20));
            writel(0x1, TMR0_INT + (i * 0x20));

            if (fh_timer0_callback)
                fh_timer0_callback(i);
        }
    }
}

void fh_timer0_callback_register(void (*callback)(int id))
{
    fh_timer0_callback = callback;
}

void kick_timer0(unsigned int time, int id)
{
    writel(0x10000, TMR0_CTL + (id * 0x20));
    writel(time, TMR0_LOAD_LO + (id * 0x20));
    writel(0x1, TMR0_INT + (id * 0x20));
    writel(0x10002, TMR0_CTL + (id * 0x20));
}

void clear_timer0(int id)
{
    writel(0x8, TMR0_INT + (id * 0x20));
    writel(0x0, TMR0_INT + (id * 0x20));
}


unsigned int get_timer0_value(int id)
{
    return readl(TMR0_VALUE_SHDW_LO + (id * 0x20));
}

void rt_timer0_init()
{
    rt_kprintf("[%s-%d]\n", __func__, __LINE__);

    rt_hw_interrupt_install(EXT_TMR0_IRQn, timer0_handler, RT_NULL, "timer0");
    /* timer is really started when scheduler started.     */
    rt_hw_interrupt_umask(EXT_TMR0_IRQn);

    rt_hw_enable_irq_wake(EXT_TMR0_IRQn);
}
