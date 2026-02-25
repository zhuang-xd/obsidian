
#include <rtdef.h>
#include <rthw.h>
#include <pm.h>
#include "fh_pm.h"
#include <fh_def.h>
#include <fh_chip.h>
#include <asm/cacheflush.h>
#include "c2c_comm.h"
#include "delay.h"
#include <fh_timer.h>

#ifdef RT_USING_PM

extern unsigned int iram_code_end;
extern unsigned int iram_code;
#define ARM_IN_SLP  0xdeadbeef
static int linux_wakeup = 1;
#ifdef FH_USING_UART_SHAREDMODE
extern void change_console_to_rsh(void);
extern void change_console_to_uart0(void);
#endif

#if 0
static int calc_aov_time(int time_start, int time_end)
{
    int cost = 0;
    rt_kprintf("total time %d.%d\n", (time_end - time_start)/1000, (time_end - time_start) %  1000);

    cost = (time_end - time_start) / 1000;
    if (cost > 200)
        cost  = 0;
    return cost;   //ms
}

static void dump_buf(unsigned char *buf, int len)
{
    int i = 0;

    rt_kprintf("[%s-%d]: buf %p, len %d\n", __func__, __LINE__, buf, len);

    for (i = 0; i < len; i++)
    {
        if ((i % 16) == 0)
        {
            rt_kprintf("\n");
        }

        rt_kprintf("0x%02x ", buf[i]);
    }

    rt_kprintf("\n");
}
#endif

static int pm_ramcode_load(void *dest, const void *src, int n)
{
    UINT32 addr = (UINT32)dest;

    rt_memcpy(dest, src, n);
    mmu_clean_invalidated_dcache(addr, n);

    return 0;
}

volatile int arm_in_slp(void){
    volatile int ret;
    ret = GET_REG(0x1010032C);
    return ret;
}


void clear_arm_slp_state(void){
    writel(0, 0x1010032C);
}

#ifdef FH_CONFIG_AOV_MCU_POWER_DOWN
void enter_iram(void)
{
    int ret;
    extern unsigned int _estack;

    int (*jump_to_iram)(unsigned int) = RT_NULL;

    pm_ramcode_load((void *)IRAM_REG_BASE, (unsigned char *)&iram_code, (unsigned int)&iram_code_end - (unsigned int)&iram_code);

    jump_to_iram = (int (*)(unsigned int))IRAM_REG_BASE;

    rt_kprintf("enter iram....\n");
    ret = jump_to_iram((unsigned int)&_estack); /*现场保存在系统堆栈里面*/

    rt_kprintf("exit iram,ret=%d\n", ret);

}
#else
extern unsigned int get_timer0_value(int id);
extern void kick_timer0(unsigned int time, int id);
extern void clear_timer0(int id);
extern void update_pts_offset(unsigned int offset);
void enter_iram(void)
{
    void (*jump_to_iram)(void) = RT_NULL;
    unsigned int sleep_count = 0;
    volatile int arm_state;
    unsigned int retry = 10000;
    unsigned int timer0_value0 = 0, timer0_value1 = 0;
    unsigned long long pts_off0 = 0, pts_off1 = 0;
    unsigned int pts_off = 0;

    while (retry > 0)
    {
        arm_state = arm_in_slp();
        if(arm_state == ARM_IN_SLP)
        {
            sleep_count++;
        }
        else
        {
            udelay(1000);
            retry--;
        }
        if (sleep_count > 2)
            break;
    }
    if (sleep_count > 2)
    {
        if (linux_wakeup)
        {
            linux_wakeup = 0;
#ifdef FH_USING_UART_SHAREDMODE
            change_console_to_uart0();
#endif
        }

        kick_timer0(0x7fffffff, 1);

        pm_ramcode_load((void *)IRAM_REG_BASE, (unsigned char *)&iram_code, (unsigned int)&iram_code_end - (unsigned int)&iram_code);

        timer0_value0 = get_timer0_value(1);
        pts_off0 = read_pts_new();

        jump_to_iram = (void (*)(void))IRAM_REG_BASE;
        jump_to_iram();

        clear_timer0(1);
        timer0_value1 = get_timer0_value(1);
        pts_off1 = read_pts_new();

        pts_off = (unsigned int)((int)(timer0_value0 - timer0_value1) * 1000ull *1000/(32*1024));
        pts_off -= (pts_off1 - pts_off0);

        update_pts_offset(pts_off);
    }
}
#endif

void fh_wakeup_arm()
{
    unsigned int sleep_count = 0;
    volatile int arm_state;
    unsigned int retry = 100;

    while (retry > 0)
    {
        arm_state = arm_in_slp();
        if(arm_state == ARM_IN_SLP)
        {
            sleep_count++;
        }
        else
        {
            rt_thread_delay(1);
            retry--;
        }
        if (sleep_count > 2)
            break;
    }
    if (sleep_count > 2)
    {
        udelay(200);
        //open arm c2c isr en
#ifdef FH_AOV_DEBUG
        rt_kprintf("[%s-%d]: wakeup linux!!!!\n", __func__, __LINE__);
#endif

#ifdef FH_USING_UART_SHAREDMODE
        change_console_to_rsh();
#endif
        linux_wakeup = 1;
        fh_mcu_c2c_wakeup_host();

        clear_arm_slp_state();
    }
}

static void arc_deep_sleep(int sleep_time_ms)
{
#ifdef FH_AOV_DEBUG
    rt_kprintf("go arc deep sleep!!\n");
    udelay(40000);
#endif
    enter_iram();
}

static void sleep(struct rt_pm *pm, uint8_t mode)
{
    switch (mode)
    {
    case PM_SLEEP_MODE_NONE:
        break;

    case PM_SLEEP_MODE_IDLE:
        break;

    case PM_SLEEP_MODE_LIGHT:
        break;

    case PM_SLEEP_MODE_DEEP:
        arc_deep_sleep(1000);
        break;

    case PM_SLEEP_MODE_STANDBY:
        break;

    case PM_SLEEP_MODE_SHUTDOWN:
        break;

    default:
        RT_ASSERT(0);
        break;
    }

}

static void run(struct rt_pm *pm, uint8_t mode)
{
    switch (mode)
    {
    case PM_RUN_MODE_HIGH_SPEED:
    case PM_RUN_MODE_NORMAL_SPEED:
        break;
    case PM_RUN_MODE_MEDIUM_SPEED:
        break;
    case PM_RUN_MODE_LOW_SPEED:
        break;
    default:
        break;

    }
}

static void pm_timer_start(struct rt_pm *pm, rt_uint32_t timeout)
{
    #if 0
    RT_ASSERT(pm != RT_NULL);
    RT_ASSERT(timeout > 0);
	unsigned  long sys_time_0,sys_time_1;

	sys_time_0 = syst_get_time();
    if (rt_deep_sleep)
	    rt_deep_sleep(timeout);
	sys_time_1 = syst_get_time();
    if (sys_time_1 > sys_time_0)
	    rt_syst_diff= sys_time_1 - sys_time_0;
    if (sys_time_1 < sys_time_0)
        rt_syst_diff= 0xffffffff - sys_time_0 + sys_time_1;
	rt_kprintf("t0:%d,t1:%d, diff:%d,timeout:%d\n",sys_time_0,sys_time_1,rt_syst_diff,timeout);
    #endif
}

/**
 * This function stop the timer of pm
 *
 * @param pm Pointer to power manage structure
 */
static void pm_timer_stop(struct rt_pm *pm)
{
    RT_ASSERT(pm != RT_NULL);
}

/**
 * This function calculate how many OS Ticks that MCU have suspended
 *
 * @param pm Pointer to power manage structure
 *
 * @return OS Ticks
 */
static rt_tick_t pm_timer_get_tick(struct rt_pm *pm)
{
    rt_uint32_t timer_tick = 0;

#if 0
	timer_tick = rt_tick_from_millisecond(rt_syst_diff);
	rt_syst_diff = 0;
#endif
	return timer_tick;
}

/**
 * This function initialize the power manager
 */
int fh_hw_pm_init(void)
{
    static const struct rt_pm_ops _ops =
    {
        sleep,
        run,
        pm_timer_start,
        pm_timer_stop,
        pm_timer_get_tick
    };

    rt_uint8_t timer_mask = 0;

    /* initialize timer mask */
    timer_mask = 1UL << PM_SLEEP_MODE_DEEP;

    /* initialize system pm module */
    rt_system_pm_init(&_ops, timer_mask, RT_NULL);

	rt_pm_release_all(PM_SLEEP_MODE_NONE);

    return 0;
}

#endif
