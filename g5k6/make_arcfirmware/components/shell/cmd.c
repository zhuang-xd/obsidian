
/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-04-30     Bernard      first implementation
 * 2006-05-04     Bernard      add list_thread,
 *                                 list_sem,
 *                                 list_timer
 * 2006-05-20     Bernard      add list_mutex,
 *                                 list_mailbox,
 *                                 list_msgqueue,
 *                                 list_event,
 *                                 list_fevent,
 *                                 list_mempool
 * 2006-06-03     Bernard      display stack information in list_thread
 * 2006-08-10     Bernard      change version to invoke rt_show_version
 * 2008-09-10     Bernard      update the list function for finsh syscall
 *                                 list and sysvar list
 * 2009-05-30     Bernard      add list_device
 * 2010-04-21     yi.qiu       add list_module
 * 2012-04-29     goprife      improve the command line auto-complete feature.
 * 2012-06-02     lgnq         add list_memheap
 * 2012-10-22     Bernard      add MS VC++ patch.
 * 2016-06-02     armink       beautify the list_thread command
 * 2018-11-22     Jesven       list_thread add smp support
 * 2018-12-27     Jesven       Fix the problem that disable interrupt too long in list_thread
 *                             Provide protection for the "first layer of objects" when list_*
 */

#include <rthw.h>
#include <rtthread.h>
#include <fhconfig.h>
#include <fh_def.h>
#include <firmware_info.h>
#include <asm/io.h>

#ifdef FH_USING_SHELL
#include "shell.h"

#define LIST_FIND_OBJ_NR 8

rt_inline void object_split(int len)
{
    while (len--) rt_kprintf("-");
}

typedef struct
{
    rt_list_t *list;
    rt_list_t **array;
    rt_uint8_t type;
    int nr;             /* input: max nr, can't be 0 */
    int nr_out;         /* out: got nr */
} list_get_next_t;

static void list_find_init(list_get_next_t *p, rt_uint8_t type, rt_list_t **array, int nr)
{
    struct rt_object_information *info;
    rt_list_t *list;

    info = rt_object_get_information((enum rt_object_class_type)type);
    list = &info->object_list;

    p->list = list;
    p->type = type;
    p->array = array;
    p->nr = nr;
    p->nr_out = 0;
}

static rt_list_t *list_get_next(rt_list_t *current, list_get_next_t *arg)
{
    int first_flag = 0;
    rt_ubase_t level;
    rt_list_t *node, *list;
    rt_list_t **array;
    int nr;

    arg->nr_out = 0;

    if (!arg->nr || !arg->type)
    {
        return (rt_list_t *)RT_NULL;
    }

    list = arg->list;

    if (!current) /* find first */
    {
        node = list;
        first_flag = 1;
    }
    else
    {
        node = current;
    }

    level = rt_hw_interrupt_disable();

    if (!first_flag)
    {
        struct rt_object *obj;
        /* The node in the list? */
        obj = rt_list_entry(node, struct rt_object, list);
        if ((obj->type & ~RT_Object_Class_Static) != arg->type)
        {
            rt_hw_interrupt_enable(level);
            return (rt_list_t *)RT_NULL;
        }
    }

    nr = 0;
    array = arg->array;
    while (1)
    {
        node = node->next;

        if (node == list)
        {
            node = (rt_list_t *)RT_NULL;
            break;
        }
        nr++;
        *array++ = node;
        if (nr == arg->nr)
        {
            break;
        }
    }

    rt_hw_interrupt_enable(level);
    arg->nr_out = nr;
    return node;
}

extern unsigned int _system_stack_size;
extern unsigned int _estack;
extern unsigned int __sstack;

void get_irq_stack(void)
{
    unsigned int stack_start = 0;
    unsigned int stack_size = 0;
    unsigned int stack_end = 0;
    unsigned int *stack_sp = 0;
    int i = 0;

    stack_size = (unsigned int)&_system_stack_size;
    stack_end = (unsigned int)&_estack;
    stack_start = (unsigned int)&__sstack;
    stack_sp = (unsigned int *)stack_start;

    rt_kprintf("%-*.*s %3d ", RT_NAME_MAX, RT_NAME_MAX, "irq", 1);

    for (i = 0; i < stack_size / 4; i++)
    {
        if (*stack_sp != 0xdeadbeef)
            break;
        stack_sp++;
    }

    if ((unsigned int)stack_sp <= stack_start)
        rt_kprintf(" overflow  ");
    else
        rt_kprintf(" ready  ");

    rt_kprintf(" 0x%08x 0x%08x    %02d$   0x%08x %03d\n",
                stack_sp,
                stack_size,
                (stack_end - (unsigned int)stack_sp) * 100
                / stack_size,
                0,
                0);
}

int list_thread(void)
{
    rt_ubase_t level;
    list_get_next_t find_arg;
    rt_list_t *obj_list[LIST_FIND_OBJ_NR];
    rt_list_t *next = (rt_list_t*)RT_NULL;
    const char *item_title = "thread";
    int maxlen;

    list_find_init(&find_arg, RT_Object_Class_Thread, obj_list, sizeof(obj_list)/sizeof(obj_list[0]));

    maxlen = RT_NAME_MAX;

    rt_kprintf("%-*.s pri  status      sp     stack size max used left tick  error\n", maxlen, item_title); object_split(maxlen);
    rt_kprintf(     " ---  ------- ---------- ----------  ------  ---------- ---\n");

    do
    {
        next = list_get_next(next, &find_arg);
        {
            int i;
            for (i = 0; i < find_arg.nr_out; i++)
            {
                struct rt_object *obj;
                struct rt_thread thread_info, *thread;

                obj = rt_list_entry(obj_list[i], struct rt_object, list);
                level = rt_hw_interrupt_disable();

                if ((obj->type & ~RT_Object_Class_Static) != find_arg.type)
                {
                    rt_hw_interrupt_enable(level);
                    continue;
                }
                /* copy info */
                rt_memcpy(&thread_info, obj, sizeof thread_info);
                rt_hw_interrupt_enable(level);

                thread = (struct rt_thread*)obj;
                {
                    rt_uint8_t stat;
                    rt_uint8_t *ptr;

                    rt_kprintf("%-*.*s %3d ", maxlen, RT_NAME_MAX, thread->name, thread->current_priority);
                    stat = (thread->stat & RT_THREAD_STAT_MASK);
                    if (stat == RT_THREAD_READY)        rt_kprintf(" ready  ");
                    else if (stat == RT_THREAD_SUSPEND) rt_kprintf(" suspend");
                    else if (stat == RT_THREAD_INIT)    rt_kprintf(" init   ");
                    else if (stat == RT_THREAD_CLOSE)   rt_kprintf(" close  ");
                    else if (stat == RT_THREAD_RUNNING) rt_kprintf(" running");

#if defined(ARCH_CPU_STACK_GROWS_UPWARD)
                    ptr = (rt_uint8_t *)thread->stack_addr + thread->stack_size - 1;
                    while (*ptr == '#')ptr --;

                    rt_kprintf(" 0x%08x 0x%08x    %02d%%   0x%08x %03d\n",
                            ((rt_ubase_t)thread->sp - (rt_ubase_t)thread->stack_addr),
                            thread->stack_size,
                            ((rt_ubase_t)ptr - (rt_ubase_t)thread->stack_addr) * 100 / thread->stack_size,
                            thread->remaining_tick,
                            thread->error);
#else
                    ptr = (rt_uint8_t *)thread->stack_addr;
                    while (*ptr == '#')ptr ++;

                    rt_kprintf(" 0x%08x 0x%08x    %02d$   0x%08x %03d\n",
                            thread->stack_size + ((rt_ubase_t)thread->stack_addr - (rt_ubase_t)thread->sp),
                            thread->stack_size,
                            (thread->stack_size - ((rt_ubase_t) ptr - (rt_ubase_t) thread->stack_addr)) * 100
                            / thread->stack_size,
                            thread->remaining_tick,
                            thread->error);
#endif
                }
            }
        }
    }
    while (next != (rt_list_t*)RT_NULL);

    get_irq_stack();

    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(list_thread, ps, list thread);

static void show_wait_queue(struct rt_list_node *list)
{
    struct rt_thread *thread;
    struct rt_list_node *node;

    for (node = list->next; node != list; node = node->next)
    {
        thread = rt_list_entry(node, struct rt_thread, tlist);
        rt_kprintf("%s", thread->name);

        if (node->next != list)
            rt_kprintf("/");
    }
}

#ifdef RT_USING_SEMAPHORE
long list_sem(void)
{
    rt_ubase_t level;
    list_get_next_t find_arg;
    rt_list_t *obj_list[LIST_FIND_OBJ_NR];
    rt_list_t *next = (rt_list_t*)RT_NULL;

    int maxlen;
    const char *item_title = "semaphore";

    list_find_init(&find_arg, RT_Object_Class_Semaphore, obj_list, sizeof(obj_list)/sizeof(obj_list[0]));

    maxlen = RT_NAME_MAX;

    rt_kprintf("%-*.s v   suspend thread\n", maxlen, item_title); object_split(maxlen);
    rt_kprintf(     " --- --------------\n");

    do
    {
        next = list_get_next(next, &find_arg);
        {
            int i;
            for (i = 0; i < find_arg.nr_out; i++)
            {
                struct rt_object *obj;
                struct rt_semaphore *sem;

                obj = rt_list_entry(obj_list[i], struct rt_object, list);
                level = rt_hw_interrupt_disable();
                if ((obj->type & ~RT_Object_Class_Static) != find_arg.type)
                {
                    rt_hw_interrupt_enable(level);
                    continue;
                }
                rt_hw_interrupt_enable(level);

                sem = (struct rt_semaphore*)obj;
                if (!rt_list_isempty(&sem->parent.suspend_thread))
                {
                    rt_kprintf("%-*.*s %03d %d:",
                            maxlen, RT_NAME_MAX,
                            sem->parent.parent.name,
                            sem->value,
                            rt_list_len(&sem->parent.suspend_thread));
                    show_wait_queue(&(sem->parent.suspend_thread));
                    rt_kprintf("\n");
                }
                else
                {
                    rt_kprintf("%-*.*s %03d %d\n",
                            maxlen, RT_NAME_MAX,
                            sem->parent.parent.name,
                            sem->value,
                            rt_list_len(&sem->parent.suspend_thread));
                }
            }
        }
    }
    while (next != (rt_list_t*)RT_NULL);

    return 0;
}
FINSH_FUNCTION_EXPORT(list_sem, list semaphore in system);
#endif

#ifdef RT_USING_EVENT
long list_event(void)
{
    rt_ubase_t level;
    list_get_next_t find_arg;
    rt_list_t *obj_list[LIST_FIND_OBJ_NR];
    rt_list_t *next = (rt_list_t*)RT_NULL;

    int maxlen;
    const char *item_title = "event";

    list_find_init(&find_arg, RT_Object_Class_Event, obj_list, sizeof(obj_list)/sizeof(obj_list[0]));

    maxlen = RT_NAME_MAX;

    rt_kprintf("%-*.s      set    suspend thread\n", maxlen, item_title); object_split(maxlen);
    rt_kprintf(     "  ---------- --------------\n");

    do
    {
        next = list_get_next(next, &find_arg);
        {
            int i;
            for (i = 0; i < find_arg.nr_out; i++)
            {
                struct rt_object *obj;
                struct rt_event *e;

                obj = rt_list_entry(obj_list[i], struct rt_object, list);
                level = rt_hw_interrupt_disable();
                if ((obj->type & ~RT_Object_Class_Static) != find_arg.type)
                {
                    rt_hw_interrupt_enable(level);
                    continue;
                }

                rt_hw_interrupt_enable(level);

                e = (struct rt_event *)obj;
                if (!rt_list_isempty(&e->parent.suspend_thread))
                {
                    rt_kprintf("%-*.*s  0x%08x %03d:",
                            maxlen, RT_NAME_MAX,
                            e->parent.parent.name,
                            e->set,
                            rt_list_len(&e->parent.suspend_thread));
                    show_wait_queue(&(e->parent.suspend_thread));
                    rt_kprintf("\n");
                }
                else
                {
                    rt_kprintf("%-*.*s  0x%08x 0\n",
                            maxlen, RT_NAME_MAX, e->parent.parent.name, e->set);
                }
            }
        }
    }
    while (next != (rt_list_t*)RT_NULL);

    return 0;
}
FINSH_FUNCTION_EXPORT(list_event, list event in system);
#endif

#ifdef RT_USING_MUTEX
long list_mutex(void)
{
    rt_ubase_t level;
    list_get_next_t find_arg;
    rt_list_t *obj_list[LIST_FIND_OBJ_NR];
    rt_list_t *next = (rt_list_t*)RT_NULL;

    int maxlen;
    const char *item_title = "mutex";

    list_find_init(&find_arg, RT_Object_Class_Mutex, obj_list, sizeof(obj_list)/sizeof(obj_list[0]));

    maxlen = RT_NAME_MAX;

    rt_kprintf("%-*.s   owner  hold suspend thread\n", maxlen, item_title); object_split(maxlen);
    rt_kprintf(     " -------- ---- --------------\n");

    do
    {
        next = list_get_next(next, &find_arg);
        {
            int i;
            for (i = 0; i < find_arg.nr_out; i++)
            {
                struct rt_object *obj;
                struct rt_mutex *m;

                obj = rt_list_entry(obj_list[i], struct rt_object, list);
                level = rt_hw_interrupt_disable();
                if ((obj->type & ~RT_Object_Class_Static) != find_arg.type)
                {
                    rt_hw_interrupt_enable(level);
                    continue;
                }

                rt_hw_interrupt_enable(level);

                m = (struct rt_mutex *)obj;
                rt_kprintf("%-*.*s %-8.*s %04d %d\n",
                        maxlen, RT_NAME_MAX,
                        m->parent.parent.name,
                        RT_NAME_MAX,
                        m->owner->name,
                        m->hold,
                        rt_list_len(&m->parent.suspend_thread));

            }
        }
    }
    while (next != (rt_list_t*)RT_NULL);

    return 0;
}
FINSH_FUNCTION_EXPORT(list_mutex, list mutex in system);
#endif

#ifdef RT_USING_MAILBOX
long list_mailbox(void)
{
    rt_ubase_t level;
    list_get_next_t find_arg;
    rt_list_t *obj_list[LIST_FIND_OBJ_NR];
    rt_list_t *next = (rt_list_t*)RT_NULL;

    int maxlen;
    const char *item_title = "mailbox";

    list_find_init(&find_arg, RT_Object_Class_MailBox, obj_list, sizeof(obj_list)/sizeof(obj_list[0]));

    maxlen = RT_NAME_MAX;

    rt_kprintf("%-*.s entry size suspend thread\n", maxlen, item_title); object_split(maxlen);
    rt_kprintf(     " ----  ---- --------------\n");

    do
    {
        next = list_get_next(next, &find_arg);
        {
            int i;
            for (i = 0; i < find_arg.nr_out; i++)
            {
                struct rt_object *obj;
                struct rt_mailbox *m;

                obj = rt_list_entry(obj_list[i], struct rt_object, list);
                level = rt_hw_interrupt_disable();
                if ((obj->type & ~RT_Object_Class_Static) != find_arg.type)
                {
                    rt_hw_interrupt_enable(level);
                    continue;
                }

                rt_hw_interrupt_enable(level);

                m = (struct rt_mailbox *)obj;
                if (!rt_list_isempty(&m->parent.suspend_thread))
                {
                    rt_kprintf("%-*.*s %04d  %04d %d:",
                            maxlen, RT_NAME_MAX,
                            m->parent.parent.name,
                            m->entry,
                            m->size,
                            rt_list_len(&m->parent.suspend_thread));
                    show_wait_queue(&(m->parent.suspend_thread));
                    rt_kprintf("\n");
                }
                else
                {
                    rt_kprintf("%-*.*s %04d  %04d %d\n",
                            maxlen, RT_NAME_MAX,
                            m->parent.parent.name,
                            m->entry,
                            m->size,
                            rt_list_len(&m->parent.suspend_thread));
                }

            }
        }
    }
    while (next != (rt_list_t*)RT_NULL);

    return 0;
}
FINSH_FUNCTION_EXPORT(list_mailbox, list mail box in system);
#endif

int list_thread(void);
static int help(void);
static int print_version(void);
extern int softcore_version(unsigned int *ver);
extern void prof_view(void);
extern void media_mem_proc(void);
extern void rt_show_version(void);
extern void fh_print_chip_info(void);
extern void list_mem(void);
void fh_clk_debug(void);
static int help(void);
extern int vpu_read_proc(void);
extern int bgm_read_proc();
extern int isp_read_proc();
extern int nna_read_proc();
extern int hevc_read_proc();
// extern int enc_read_proc();
extern int venc_read_proc();
extern int jpeg_hld_read_proc(void);
extern int media_read_proc();
extern int mipi_read_proc();
extern int Vicap_ReadProc();
// #ifdef CONFIG_CHIP_FH8656
// extern int nna_read_proc();
// #endif
// extern int nna_read_proc();
extern void acw_info(void);
extern void media_mem_proc(void);
extern void fh_acw_loopback_test(void);
extern void acw_self_test(void);
extern void fh_acw_fifo_test(void);
extern void fh_acw_dma_test(void);
extern void xbus_dump(void);
extern void dma_dump(void);
extern void FH_NR_OverflowTest(void);

unsigned int htoui(char *buff)
{
	unsigned int begin;
	unsigned int value = 0;

	if (buff[0] == '0' && buff[1] == 'x')
	{
		begin = 2;
	}
	else
		begin = 0;

	for (; (buff[begin] >= '0' && buff[begin] <= '9') || (buff[begin] >= 'a' && buff[begin] <= 'z'); ++begin)
	{
		if (buff[begin] > '9')
			value = 16 * value + (10 + buff[begin] - 'a');
		else
			value = 16 * value + (buff[begin] - '0');

	}

	return value;
}

void set_reg(int argc, char *argv[])
{
    unsigned int addr,val;
    if(argc != 3)
    {
        rt_kprintf("invalid parameter\n");
        return;
    }
    addr = htoui(argv[1]);
    val  = htoui(argv[2]);
    SET_REG(addr,val);
    rt_kprintf("set address %#x with value %#x\n",addr,val);
}

void mem_disp(int argc, char *argv[])
{
	int i;
    unsigned int addr,sz;

    if (argc != 3)
    {
        rt_kprintf("invalid parameter\n");
        return;
    }

    addr = htoui(argv[1]);
    sz  = htoui(argv[2]);

	sz /= 4;
    for (i=0; i<sz; i++)
    {
    	if (i % 4 == 0)
    	{
    		rt_kprintf("\n%08x: ", addr + i*4);
    	}
    	rt_kprintf("%08x ", *((unsigned int*)addr + i));
    }
}


void get_reg(int argc, char *argv[])
{
    int i;
    unsigned int addr,val,reg_num;
    if(argc!=2 && argc!=3)
    {
        rt_kprintf("invalid parameter\n");
        return;
    }

    reg_num = (argc==3) ? htoui(argv[2]):1;
    addr = htoui(argv[1]);
    for(i=0;i<reg_num;i++)
    {
        val = GET_REG(addr);
        rt_kprintf("%#x: %#x\n",addr,val);
        addr+=4;
    }
}

void user_ext_proc(int argc, char* argv[])
{
    rt_kprintf("user ext command!\n");
}

struct shell_cmd
{
    const char*  name; /* the name of shell command */
    shell_func   func; /* the function address for this shell command */
};

struct ext_shell_cmd
{
    char name[16]; /* the name of shell command */
    shell_func func; /* the function address for this shell command */
    struct ext_shell_cmd* next;
};

static struct ext_shell_cmd *g_ext_cmd_list;

int register_shell_command(const char*  name, int (*func)(int argc, char *argv[]))
{
    int len = 0;
    struct ext_shell_cmd *ecmd;
    struct ext_shell_cmd **last;
    struct ext_shell_cmd *curr;
    rt_ubase_t level;

    if (name)
    {
        for (; len<sizeof(ecmd->name); len++)
        {
            if (!name[len])
                break;
        }
    }

    if (!len || len >= sizeof(ecmd->name))
        return -1;

    ecmd = g_ext_cmd_list;
    while (ecmd)
    {
        if (rt_strcmp(ecmd->name, name) == 0)
        {
            ecmd->func = func;
            return 0;
        }
        ecmd = ecmd->next;
    }


    ecmd = (struct ext_shell_cmd *)rt_malloc(sizeof(struct ext_shell_cmd));
    if (!ecmd)
        return -2;

    rt_memcpy(ecmd->name, name, len + 1);
    ecmd->func = func;
    ecmd->next = RT_NULL;

    level = rt_hw_interrupt_disable();
    last = &g_ext_cmd_list;
    curr = g_ext_cmd_list;
    while (curr)
    {
        last = &curr->next;
        curr = curr->next;
    }
    *last = ecmd;
    rt_hw_interrupt_enable(level);

    return 0;
}


void dma_testxx(void);
// extern shell_func isp_i2c_read(int argc, char *argv[]);
static struct shell_cmd shell_cmd_tbl[] =
{
    {"help", (shell_func)help}, //this must be the first one,because I don't print item 0
    {"ver",  (shell_func)print_version},
    {"ps",   (shell_func)list_thread},
    {"free", (shell_func)list_mem},
    {"top",  (shell_func)prof_view},
    {"set_reg",(shell_func)set_reg},
    {"get_reg",(shell_func)get_reg},
    {"md", (shell_func)mem_disp},
    {"xbus_dump",(shell_func)xbus_dump},
    // {"isp_i2c_read",(shell_func)isp_i2c_read},
#ifdef FH_USING_RPC_SOLUTION
        {"fh_clk_debug", (shell_func)fh_clk_debug},
        {"fh_chip_info", (shell_func)fh_print_chip_info},
        {"media_mem_proc", (shell_func)media_mem_proc},
        // #ifdef CONFIG_CHIP_FH8656
        //     {"nna_read_proc", (shell_func)nna_read_proc},
        // #endif
        {"vpu_read_proc", (shell_func)vpu_read_proc},
        {"venc_read_proc", (shell_func)venc_read_proc},
        {"jpeged_read_proc", (shell_func)jpeg_hld_read_proc},
        //{"bgm_read_proc", (shell_func)bgm_read_proc},
        {"nna_read_proc", (shell_func)nna_read_proc},
        // {"enc_read_proc", (shell_func)enc_read_proc},
        // {"jpeg_read_proc", (shell_func)jpeg_read_proc},
        {"media_read_proc", (shell_func)media_read_proc},
        {"isp_read_proc", (shell_func)isp_read_proc},
        {"vicap_read_proc", (shell_func)Vicap_ReadProc},
        //    {"nna_read_proc", (shell_func)nna_read_proc},
        {"mipi_read_proc", (shell_func)mipi_read_proc},
        // {"ext", (shell_func)user_ext_proc},
#endif

#ifdef FH_USING_ACW
    {"acw_info",(shell_func)acw_info},
#endif
    //{"dma_dump",     (shell_func)dma_dump},
    {"nrovtest",     (shell_func)FH_NR_OverflowTest},
};

static int help(void)
{
	int i;
    struct ext_shell_cmd *ecmd = g_ext_cmd_list;

	for (i=1; i<sizeof(shell_cmd_tbl)/sizeof(shell_cmd_tbl[0]); i++)
    {
        rt_kprintf("%s \n", shell_cmd_tbl[i].name);
    }

    while (ecmd)
    {
        rt_kprintf("%s \n", ecmd->name);
        ecmd = ecmd->next;
    }

	return 0;
}

static int print_version(void)
{
    unsigned int ver;

    rt_show_version();

    ver = readl(&_arc_fw_sdk_git_version);
    rt_kprintf("git %08x\n", ver);

#ifdef FH_USING_EXTLIB_SOFTCORE
#ifndef CONFIG_CHIP_FH8626V100
    // softcore_version(&ver);
    // rt_kprintf("softcore %08x\n", ver);
#endif
#endif

    return 0;
}


shell_func get_shell_cmd(char *name)
{
    int i;
    struct shell_cmd *cmd = shell_cmd_tbl;
    struct ext_shell_cmd *ecmd = g_ext_cmd_list;

    for (i=0; i<sizeof(shell_cmd_tbl)/sizeof(shell_cmd_tbl[0]); i++)
    {
        if (rt_strcmp(cmd->name, name) == 0)
        {
            return cmd->func;
        }
        cmd++;
    }

    while (ecmd)
    {
        if (rt_strcmp(ecmd->name, name) == 0)
        {
            return ecmd->func;
        }
        ecmd = ecmd->next;
    }

    return RT_NULL;
}

#endif /* FH_USING_SHELL */

#if 0
extern int API_ISP_GetSensorReg(unsigned int u32IspDevId,  unsigned short addr, unsigned short *data);

shell_func isp_i2c_read(int argc, char *argv[])
{
    int i;
    unsigned int addr,val,reg_num;
    unsigned short data;
    addr = htoui(argv[1]);
    reg_num  = htoui(argv[2]);
    for (i = 0; i < reg_num; i++ )
    {
        API_ISP_GetSensorReg(0, addr + i , &data);
        rt_kprintf("%x %x\n", addr + i, data);
    }
}
#endif
