#include <rtthread.h>
#include <rthw.h>
#include "asm/io.h"
#include "asm/cacheflush.h"
#include "fh_arch.h"

static rt_sem_t g_nn_isr_sem;
static void nn_isr(int irq, void *param)
{
    fh_hw_writereg(NNA_REG_BASE + 0x04 , 1 << 9);

    /* send sync info */
    rt_sem_release(g_nn_isr_sem);
}

static void wait_for_nn_isr(void)
{
    /* wait for the semaphore */
    rt_sem_take(g_nn_isr_sem, RT_WAITING_FOREVER);
}

void fh_install_nn_isr()
{
    fh_hw_writereg(NNA_REG_BASE + 0x08, 1 << 9);        /* enable stone */
    fh_hw_writereg(NNA_REG_BASE + 0x0c, ~(1 << 9));     /* unmask stone */

    rt_hw_interrupt_install(NN_IRQn, nn_isr, RT_NULL, "nn_isr");
    rt_hw_interrupt_umask(NN_IRQn);
}

void do_nna_work(void)
{
    fh_hw_flush_dcache_all();
    fh_hw_writereg(NNA_REG_BASE + 0x1c, 3);     /* kick NN */
    wait_for_nn_isr();
}

void do_nna_init(unsigned int addr)
{
    rt_kprintf("nna reg config mc632x\n");
    fh_hw_writereg(NNA_REG_BASE + 0x08, 0x00700620);
    fh_hw_writereg(NNA_REG_BASE + 0x0c, 0);
    fh_hw_writereg(NNA_REG_BASE + 0x18, 0xf4);
    fh_hw_writereg(NNA_REG_BASE + 0x10, 1);
    fh_hw_writereg(NNA_REG_BASE + 0x44, addr);
    g_nn_isr_sem = rt_sem_create("nna_isr", 0, RT_IPC_FLAG_FIFO);
    fh_install_nn_isr();
}
