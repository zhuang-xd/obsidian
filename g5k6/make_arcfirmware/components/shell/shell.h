/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-06-02     Bernard      Add finsh_get_prompt function declaration
 */
#ifndef __SHELL_H__
#define __SHELL_H__

#include <rtthread.h>

#define FINSH_CMD_SIZE           40
#define FINSH_THREAD_NAME        "shell"

typedef int (*shell_func)(int argc, char *argv[]);

shell_func get_shell_cmd(char *name);

#endif
