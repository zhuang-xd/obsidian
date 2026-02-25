/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */
#ifndef __RTCOMPLETION_H__
#define __RTCOMPLETION_H__

#include <rtthread.h>

/**
 * Completion
 */

struct rt_completion
{
    rt_uint32_t flag;

    /* suspended list */
    rt_list_t suspended_list;
};

void rt_completion_init(struct rt_completion *completion);
rt_err_t rt_completion_wait(struct rt_completion *completion,
                            rt_int32_t            timeout);
void rt_completion_done(struct rt_completion *completion);
void rt_completion_all(struct rt_completion *completion);
void rt_reinit_completion(struct rt_completion *completion);

#endif /*__RTCOMPLETION_H__*/
