/* RT-Thread config file */

#ifndef __RTTHREAD_CFG_H__
#define __RTTHREAD_CFG_H__

// Basic Configuration
// Maximal level of thread priority <8-256>, default is 32
#define RT_THREAD_PRIORITY_MAX  32
#define RT_TICK_PER_SECOND  100
#define RT_ALIGN_SIZE   4
#define RT_NAME_MAX    16

#define RT_USING_DEVICE

// Debug Configuration
//#define RT_DEBUG
//#define RT_DEBUG_INIT 0
#define RT_USING_OVERFLOW_CHECK

// <h>Hook Configuration
#define RT_USING_HOOK
//#define RT_USING_IDLE_HOOK

// Software timers Configuration
//#define RT_USING_TIMER_SOFT
//#define RT_TIMER_THREAD_PRIO        4
//#define RT_TIMER_THREAD_STACK_SIZE  512

// IPC(Inter-process communication) Configuration
#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
#define RT_USING_EVENT
#define RT_USING_MAILBOX
//#define RT_USING_MESSAGEQUEUE

// Memory Management Configuration
#define RT_USING_HEAP
#define RT_USING_SLAB
//#define RT_USING_MM_TRACE //not supported on ARC
//#define RT_USING_MEMALIGN

// <h>Console Configuration
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE          128

#endif
