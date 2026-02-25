#ifndef __xbus_osl_h__
#define __xbus_osl_h__

#define XBUS_OS_ARC_RTT

#if defined(XBUS_OS_LINUX)
#elif defined(XBUS_OS_ARM_RTT)
#elif defined(XBUS_OS_ARC_RTT)
#include "asm/cacheflush.h"
#include "asm/io.h"

#define MMU_INVALIDATE_DCACHE(x,y) mmu_invalidate_dcache((unsigned int)(x), (unsigned int)(y))
#define MMU_FLUSH_DCACHE(x,y)      mmu_clean_dcache((unsigned int)(x), (unsigned int)(y))

#define XBusMalloc(size)                  rt_malloc(size)
#define XBusFree                          rt_free

#define XBusMemCpy(dst,src,len)           rt_memcpy(dst,src,len)
#define XBusMemSet(src,c,n)               rt_memset(src,c,n)
#define XBusStrLen(str)                   rt_strlen(str)
#define XBusStrCmp(s1,s2)                 rt_strcmp((char*)(s1),(char*)(s2))

#define XBusDelayTicks(tick)              rt_thread_delay(tick)

#define XBUS_IRQ_FLAG_DECLARE(flags)      rt_ubase_t flags
#define XBUS_IRQ_SAVE(flags)              flags = rt_hw_interrupt_disable()
#define XBUS_IRQ_RESTORE(flags)           rt_hw_interrupt_enable(flags)

typedef struct rt_semaphore XBUS_SEMA_T;
#define XBusSemInit(sem, name, count)     rt_sem_init(&(sem), name, count, RT_IPC_FLAG_FIFO)
#define XBusSemDown(sem)                  rt_sem_take(&(sem), RT_WAITING_FOREVER)
#define XBusSemUp(sem)                    rt_sem_release(&(sem))
#define XBusSemDownTimeout(sem, t)        rt_sem_take(&(sem), t)

#define XBusPrint                         rt_kprintf
#define XBusPrintErr                      rt_kprintf

#else
#error "Please specify platform!!!"
#endif

#endif /*__xbus_osl_h__*/
