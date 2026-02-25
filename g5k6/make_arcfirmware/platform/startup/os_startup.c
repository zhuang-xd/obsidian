/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 */

#include <rthw.h>
#include <rtthread.h>
#include <firmware_info.h>
#include <fh_def.h>
#include <asm/io.h>
#include <fh_timer.h>
#include <types/bufCtrl.h>
#include <fh_chip.h>
#include <asm/cacheflush.h>
#ifdef RT_USING_PM
#include <fh_pm.h>
#endif
#include <gpio.h>

extern void rt_hw_board_init(void);
extern void rt_system_timer_init(void);
extern void rt_system_scheduler_init(void);
extern void rt_thread_idle_init(void);
extern void rt_hw_cpu_icache_enable(void);
extern void rt_system_heap_init(void *begin_addr, void *end_addr);
extern void rt_hw_finsh_init(void);
extern void rt_application_init(void);
extern int xbus_init_memory(unsigned int addr, unsigned int size);
extern int user_main(void);

/* if there is not enable heap, we should use static thread and stack. */
ALIGN(8)
static rt_uint8_t main_stack[FH_MAIN_THREAD_STACK_SIZE];
struct rt_thread main_thread;

void fh_dma_memcpy(void *dst, void *src, int len)
{
    rt_memcpy(dst,src,len);
}

/* the system main thread */
static void main_thread_entry(void *parameter)
{
    user_main(); /*don't return from user_main*/
}

void rt_application_init(void)
{
    rt_thread_t tid;

    tid = &main_thread;

    rt_thread_init(tid, "main", main_thread_entry, RT_NULL,
                            main_stack, sizeof(main_stack), FH_MAIN_THREAD_PRIORITY, 20);
    rt_thread_startup(tid);
}


/**
 * This function will startup RT-Thread RTOS.
 * It is called from assembly code in arc600/startup_gcc.S
 */
extern int __sys_heap_start;

static unsigned long g_os_start_time;
unsigned long get_os_start_time(void)
{
    return g_os_start_time;
}

unsigned int phy_address_trans(unsigned int addr)
{
    unsigned int compile_addr   = readl(&_arc_fw_compile_entry);
    unsigned int real_load_addr = readl(&_arc_fw_load_addr);
    unsigned int ret;

    ret = addr - (compile_addr - real_load_addr);

    rt_kprintf("load %08x,compile %08x,addr %08x,ret %08x.\n",real_load_addr, compile_addr, addr, ret);

    return ret;
}

void rt_show_version(void)
{
    rt_kprintf("build %s,%s\n", __DATE__, __TIME__);
}


#define AUX_DC_BILD   0x72
#define AUX_IC_BILD   0x77
static inline void show_cache_info(void)
{
    unsigned int ibuild;
    unsigned int dbuild;

    ibuild = fh_hw_readaux(AUX_IC_BILD);
    dbuild = fh_hw_readaux(AUX_DC_BILD);
    rt_kprintf("ICache:%dKB,%d;DCache:%dKB,%d\n",(1<<((ibuild>>12)&15))>>1, 8<<((ibuild>>16)&15), (1<<((dbuild>>12)&15))>>1, 16<<((dbuild>>16)&15));
}

extern void os_hook_startup(void);
int rtthread_startup(void)
{
    g_os_start_time = read_pts32();

    /* disable interrupt */
    rt_hw_interrupt_disable();

    /*heap initialization, so rt_malloc/rt_free can be used.*/
    rt_system_heap_init(&__sys_heap_start, (void*)(OS_SDRAM_END));

#ifdef RT_USING_PM
    fh_hw_pm_init();
#endif
#ifndef FH_USING_XBRSH
extern void uart_init(void);
    uart_init();
#endif
    rt_hw_interrupt_init();
    
    os_hook_startup();

    /*init xbus/rshell/sysinfo memory*/
    xbus_init_memory(phy_address_trans(XBUS_MEMORY_START), XBUS_MEMORY_SIZE);

    /* board level initialization
     * NOTE: please initialize heap inside board initialization.
     */
    rt_hw_board_init();

    /* show RT-Thread version */
    //rt_show_version();
    show_cache_info();

    /* timer system initialization */
    rt_system_timer_init();

    /* scheduler system initialization */
    rt_system_scheduler_init();

    /* create init_thread */
    rt_application_init();

    /* timer thread initialization */
    rt_system_timer_thread_init();

    /* idle thread initialization */
    rt_thread_idle_init();

    /* start scheduler */
    rt_system_scheduler_start();

    /* never reach here */
    return 0;
}

