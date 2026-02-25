/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <rtthread.h>
#include <rthw.h>
#include <asm/arc6xx.h>
#include <asm/io.h>
#include <firmware_info.h>

#define OS_TICK_TIMER_FREQ (500000000)  /*500MHz, FIXME*/

#define TMR1_IRQn   3

extern void rt_hw_clock_init(void);


/*
 * this function is defined in OS,
 * see file libcpu/arc/arc600/context_gcc.S
 */
extern void tick_config(unsigned int tk);

extern rt_err_t rshell_device_register(void);

#ifdef FH_USING_RPC_SOLUTION
static unsigned int get_mcu_clockfreq(void)
{
    //return 500000000; /*500MHz*/
    return 0; /*using default value*/
}
#endif

static void isr_tick_handler(int vector, void *param)
{
    /* clear isr first. */
    fh_hw_writeaux(0x22, 3);
    
    rt_tick_increase();
}

static void SysTick_Config()
{
    unsigned int val;

    rt_hw_interrupt_install(TMR1_IRQn, isr_tick_handler, RT_NULL, "timer");

    val = readl(&_arc_fw_ready_flag);
#ifdef FH_USING_RPC_SOLUTION
    if (!val)
    {
        val = get_mcu_clockfreq();
    }
#endif

    if (!val)
    {
        val = OS_TICK_TIMER_FREQ;
    }

    tick_config(val / RT_TICK_PER_SECOND);

    /* timer is really started when scheduler started.     */
    rt_hw_interrupt_umask(TMR1_IRQn);

    rt_kprintf("clock %d\n", val);
}


void rt_hw_board_init(void)
{


#ifdef FH_USING_XBRSH
	rshell_device_register();
#ifdef FH_USING_UART_SHAREDMODE
extern void uart_init(void);
    uart_init();
#endif
#endif

    rt_hw_clock_init();
    SysTick_Config();

#ifdef RT_USING_PM
extern void rt_timer0_init();
    rt_timer0_init();
#endif
}
