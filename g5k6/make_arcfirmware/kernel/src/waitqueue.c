/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018/06/26     Bernard      Fix the wait queue issue when wakeup a soon 
 *                             to blocked thread.
 */

#include <stdint.h>

#include <rthw.h>
#include <rtdevice.h>
#include <rtservice.h>
#include <waitqueue.h>

void rt_wqueue_add(rt_wqueue_t *queue, struct rt_wqueue_node *node)
{
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    rt_list_insert_before(&(queue->waiting_list), &(node->list));
    rt_hw_interrupt_enable(level);
}

void rt_wqueue_remove(struct rt_wqueue_node *node)
{
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    rt_list_remove(&(node->list));
    rt_hw_interrupt_enable(level);
}
