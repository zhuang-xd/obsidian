/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-04-30     Bernard      the first version for FinSH
 * 2006-05-08     Bernard      change finsh thread stack to 2048
 * 2006-06-03     Bernard      add support for skyeye
 * 2006-09-24     Bernard      remove the code related with hardware
 * 2010-01-18     Bernard      fix down then up key bug.
 * 2010-03-19     Bernard      fix backspace issue and fix device read in shell.
 * 2010-04-01     Bernard      add prompt output when start and remove the empty history
 * 2011-02-23     Bernard      fix variable section end issue of finsh shell
 *                             initialization when use GNU GCC compiler.
 * 2016-11-26     armink       add password authentication
 * 2018-07-02     aozima       add custome prompt support.
 */

#include <rthw.h>
#include <fhconfig.h>
#include <xbus_protocol.h>

#ifdef FH_USING_SHELL
#include "shell.h"

enum input_stat
{
    WAIT_NORMAL = 0,
    WAIT_SPEC_KEY,
    WAIT_FUNC_KEY,
};

/* finsh thread */
static struct rt_thread finsh_thread;
ALIGN(RT_ALIGN_SIZE)
static char finsh_thread_stack[FH_SHELL_THREAD_STACK_SIZE];

#define FINSH_ARG_MAX    4

static int msh_exec(char *cmd)
{
    int argc = 0;
    char *argv[FINSH_ARG_MAX];
    shell_func func;

    rt_memset(argv, 0x00, sizeof(argv));

	/* split arguments */
    while (1)
    {
    	if (argc >= FINSH_ARG_MAX)
    		break;
    		
  	    /* strim the beginning of command */
  	    /* Tab is filter out before call me */
    	while (*cmd == ' ')
    		cmd++;

    	if (*cmd == 0)
    		break;

    	argv[argc++] = cmd;

    	while (*cmd && *cmd != ' ')
    		cmd++;

    	if(*cmd == 0)
    		break;

    	*(cmd++) = 0; /*NULL terminated*/
    }

    if (argc == 0)
    	return 0;
    
    func = get_shell_cmd(argv[0]);
    if (func == RT_NULL)
    {
    	rt_kprintf("   command not found.\n");
        return -RT_ERROR;
    }

    /* exec this command */
    return func(argc, argv);
}


static int finsh_getchar(void)
{
    static int delayticks = RT_TICK_PER_SECOND;
    char ch = 0;
    
    while (rt_device_read(rt_console_get_device(), 0, &ch, 1) != 1)
    {
        rt_thread_delay(delayticks);
    }

    delayticks = 5;
    return (int)ch;
}

void finsh_prompt(void)
{
    rt_kprintf("xsh >");
}

static void cmd_end_prompt(void)
{
    char flag[2];

    flag[0] = RSH_CMD_EXEC_END;
    flag[1] = 0; //NULL terminated

    rt_kprintf(flag);
}

void finsh_thread_entry(void *parameter)
{
    int  ch;
    int  stat = WAIT_NORMAL;
    int  line_position = 0;
    char line[FINSH_CMD_SIZE];
    
    finsh_prompt();

    while (1)
    {
        ch = finsh_getchar();
        if (ch < 0)
        {
            continue;
        }

        /*
         * handle control key
         * up key  : 0x1b 0x5b 0x41
         * down key: 0x1b 0x5b 0x42
         * right key:0x1b 0x5b 0x43
         * left key: 0x1b 0x5b 0x44
         */        
        if (ch == 0x1b)
        {
            stat = WAIT_SPEC_KEY;
            continue;
        }
        else if (stat == WAIT_SPEC_KEY)
        {
            if (ch == 0x5b)
            {
                stat = WAIT_FUNC_KEY;
                continue;
            }
            stat = WAIT_NORMAL;
        }
        else if (stat == WAIT_FUNC_KEY)
        {
            stat = WAIT_NORMAL;
            if (ch >= 0x41 && ch <= 0x44) /* up/down/left/right key */
            {
                continue;
            }
        }

        /* received null or error */
        if (ch == '\0' || /*NULL*/
        	ch == 0xFF || /*error*/
        	ch == '\t')   /*Tab*/
        	continue;

        /*handle Ctrl+Break*/
        if (ch == 0x03)
        {
        	line_position = 0;
        	rt_kprintf("\n");
        	finsh_prompt();
        	continue;
        }

        /*handle backspace*/
        if (ch == 0x7f || ch == 0x08)
        {
        	if (line_position > 0)
        	{
        		line_position--;
        		rt_kprintf("\b \b");
        	}
        	continue;
        }

        /* handle end of line, break */
        if (ch == '\r' || ch == '\n')
        {
            rt_kprintf("\n");
            /*ensure NULL terminated*/
            line[line_position] = 0;
            msh_exec(line);
            line_position = 0;
            
            finsh_prompt();
            cmd_end_prompt();
            continue;
        }

        /* it's a large line, discard it */
        if (line_position >= FINSH_CMD_SIZE - 1)
        	continue;

        /* normal character */
        line[line_position++] = ch;
        rt_kprintf("%c", ch);
    } /* end of device read */
}


/*
 * @ingroup finsh
 *
 * This function will initialize finsh shell
 */
int shell_init(void)
{
    rt_thread_init(&finsh_thread,
                            FINSH_THREAD_NAME,
                            finsh_thread_entry, RT_NULL,
                            &finsh_thread_stack[0], sizeof(finsh_thread_stack),
                            FH_SHELL_THREAD_PRIORITY, 10);
    rt_thread_startup(&finsh_thread);
    return 0;
}

#endif /* FH_USING_SHELL */
