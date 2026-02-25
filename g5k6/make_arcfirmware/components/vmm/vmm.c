
#include <rtthread.h>
#include <rthw.h>
#include <fh_chip.h>
#include "vmm.h"
#include <asm/cacheflush.h>
#include "../ringbuffer/ringbuffer.h"

#define DEFAULT_MAOLLOC_NAME  "NONAME"

#define VMM_DECLARE_LOCK() rt_ubase_t level
#define VMM_LOCK() level = rt_hw_interrupt_disable()
#define VMM_UNLOCK() rt_hw_interrupt_enable(level)

#define mmz_align2(x,g) (((x)+(g)-1)&(~((g)-1)))

#define SZ_1K   1024
#define MMZ_DBG_LEVEL (0)
#define mmz_trace(level, s, params...) do{ if(level & MMZ_DBG_LEVEL)\
        rt_kprintf("[%s, %d]: " s "\n", __FUNCTION__, __LINE__, params);\
        }while(0)

struct vmm_node
{
    /*here must be the first...*/
    rt_list_t list;

    char name[VMM_MAX_NAME_LEN];
    rt_uint32_t base_addr;
    rt_uint32_t size;
    rt_uint32_t align;
};

#define vmm_list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
        pos = n, n = pos->next)

#define vmm_list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

static int align_check(rt_uint32_t align)
{
    rt_uint32_t bits = 0;

    if ((align >> 28) != 0) /*align cann't be too large*/
    {
        return 0;
    }

    while (align != 0)
    {
        if ((align & 1) != 0)
            bits++;
        align >>= 1;
    }

    if (bits == 1)
        return 1;

    return 0;
}

int vmm_init(
        struct vmm_control *p_vmm,
        char *name,
        rt_uint32_t beginAddr,
        rt_uint32_t bufSize,
        rt_uint32_t bufAllign)
{
    rt_uint32_t maxsz;

    VMM_DECLARE_LOCK();

    /* already inited */
    if (p_vmm->begin_addr)
        return -2;

    if (bufAllign < DCACHE_LINE_SIZE)
        bufAllign = DCACHE_LINE_SIZE;

    /*maxsz = ~0 - beginAddr + 1;*/
    maxsz = -beginAddr;
    if (bufSize == 0 || 
            bufSize >= maxsz || /*don't let end_addr reach 0xffffffff*/
            !align_check(bufAllign))
    {
        return -1;
    }

    VMM_LOCK();

    rt_list_init(&p_vmm->vmm_node_list);
    rt_strncpy(p_vmm->name, name, VMM_MAX_NAME_LEN);
    p_vmm->align      = bufAllign;
    p_vmm->begin_addr = beginAddr;
    p_vmm->end_addr   = beginAddr + (bufSize - 1);
    p_vmm->nbytes     = bufSize;
    VMM_UNLOCK();

    return 0;
}

void *vmm_malloc_with_name(
        struct vmm_control *p_vmm,
        rt_uint32_t size,
        rt_uint32_t align,
        char *name)
{    
    rt_uint32_t fixed_len=~1;
    rt_uint32_t fixed_start = 0;
    rt_list_t  *fixed_pos = RT_NULL;
    const rt_list_t * const head = &p_vmm->vmm_node_list;

    struct vmm_node *node;	
    rt_uint32_t blank_len;
    rt_uint32_t start;
    rt_uint32_t len;
    rt_uint32_t low;
    rt_uint32_t hi;
    rt_list_t  *pos;    

    VMM_DECLARE_LOCK();

    if (align < p_vmm->align)
        align = p_vmm->align;

    len = mmz_align2(size, align);

    if (!align_check(align) || 
            len < size/*overflow check*/ ||
            len <= 0 ||
            len > p_vmm->nbytes)
    {
        return RT_NULL;
    }

    node = rt_malloc(sizeof(struct vmm_node));
    if (!node)
    {
        return RT_NULL;
    }

    if (!name)
        name = DEFAULT_MAOLLOC_NAME;
    rt_strncpy(node->name, name, VMM_MAX_NAME_LEN);
    node->base_addr = 0;
    node->size = len;
    node->align = align;

    VMM_LOCK();

    pos = head->next;
    low = p_vmm->begin_addr;
    while (1)
    {
        if (pos == head) /*it's the head*/
        {
            hi = p_vmm->begin_addr + p_vmm->nbytes;
        }
        else
        {
            hi = ((struct vmm_node *)pos)->base_addr;
        }

        start = mmz_align2(low, align);
        blank_len = hi - start;
        if (start >= low/*overflow check*/ && start < hi && blank_len >= len && blank_len < fixed_len)
        {
            fixed_start = start;
            fixed_len = blank_len;
            fixed_pos = pos;
        }

        if (pos == head) /*it's the head*/
        {
            break;
        }

        low = ((struct vmm_node *)pos)->base_addr + ((struct vmm_node *)pos)->size;
        pos = pos->next;
    }

    if (fixed_pos)
    {
		node->base_addr = fixed_start;
        rt_list_insert_before(fixed_pos, &node->list);
		
        VMM_UNLOCK();
        return (void *)node->base_addr;
    }

    VMM_UNLOCK();
    rt_free(node);
    return RT_NULL;
}

void *vmm_malloc(struct vmm_control *p_vmm, rt_size_t size, rt_size_t align)
{
    return vmm_malloc_with_name(p_vmm, size, align, RT_NULL);
}

int vmm_get_block_info(struct vmm_control *p_vmm, unsigned int *addr_in, unsigned int *size)
{
    rt_list_t *pos;
    rt_list_t *head = &p_vmm->vmm_node_list;
    struct vmm_node *node;

    VMM_DECLARE_LOCK();

    VMM_LOCK();

    if (p_vmm->begin_addr)
    {
        vmm_list_for_each(pos, head)
        {
            node = (struct vmm_node *)pos;
            if (*addr_in >= node->base_addr && *addr_in < node->base_addr + node->size)
            {
                int offset = *addr_in - node->base_addr;
                *addr_in = node->base_addr;
                *size = node->size;
                VMM_UNLOCK();
                return offset;
            }
        }
    }

    VMM_UNLOCK();

    return -1;
}

void *vmm_get_info(struct vmm_control *p_vmm,unsigned int *size)
{
    unsigned int addr = p_vmm->begin_addr;

    *size = 0;
    if (addr)
    {
        *size = p_vmm->end_addr - p_vmm->begin_addr + 1;
    }
    return (void *)addr;
}

int vmm_free(struct vmm_control *p_vmm, rt_uint32_t addr)
{
    rt_list_t *pos;
    rt_list_t *n;
    rt_list_t *head = &p_vmm->vmm_node_list;
    struct vmm_node *node;

    VMM_DECLARE_LOCK();

    VMM_LOCK();

    if (p_vmm->begin_addr)
    {
        vmm_list_for_each_safe(pos, n, head)
        {
            node = (struct vmm_node *)pos;
            if (addr >= node->base_addr && addr < node->base_addr + node->size)
            {
                mmz_trace(2, "mmz:%s,name=%s,addr=0x%x,size=0x%x\n",
                    p_vmm->name,node->name,node->base_addr, node->size);
                rt_list_remove(pos);
                rt_free(pos);
                VMM_UNLOCK();
                return 0;
            }
        }
    }

    VMM_UNLOCK();

    return -1;
}

int vmm_reset(struct vmm_control *p_vmm)
{
    rt_list_t *pos;
    rt_list_t *n;
    rt_list_t *head = &p_vmm->vmm_node_list;

    VMM_DECLARE_LOCK();

    VMM_LOCK();

    if (p_vmm->begin_addr)
    {
        vmm_list_for_each_safe(pos, n, head)
        {
            rt_list_remove(pos);
            rt_free(pos);
        }
    }

    VMM_UNLOCK();

    return 0;
}

#define MMZ_FMT_S "phys(0x%08lX~0x%08lX), size: %luKB,\tname: \"%s\""
#define mmz_fmt_arg(p) (p)->begin_addr,(p)->end_addr,(p)->nbytes/SZ_1K,(p)->name
#define MMB_FMT_S "phys(0x%08lX~0x%08lX), size: %luKB,\tname: \"%s\""
#define mmb_fmt_arg(p) (p)->base_addr,((p)->base_addr+(p)->size)-1,(p)->size/SZ_1K,(p)->name

void vmm_dump_all_info(struct vmm_control *p_vmm)
{
    unsigned int block_number = 0;
    unsigned int blank_number = 0;
    unsigned int used_size = 0;
    unsigned int free_size = 0;
    unsigned int mmz_total_size = 0;

    rt_list_t *pos;
    struct vmm_node *mmb;
    unsigned long last_phy_addr = p_vmm->begin_addr;

    if (!last_phy_addr) {
        rt_kprintf("VMM not init!\n");
        return;
    }
    rt_kprintf("VMM ZONE:  " MMZ_FMT_S "\n", mmz_fmt_arg(p_vmm));
    rt_kprintf("-------------------\n");
    mmz_total_size += p_vmm->nbytes / 1024;
    VMM_DECLARE_LOCK();
    VMM_LOCK();
    vmm_list_for_each(pos, &p_vmm->vmm_node_list) {
        mmb = (struct vmm_node *)pos;
        rt_kprintf("  BLOCK%02d:  " MMB_FMT_S, block_number,
                mmb_fmt_arg(mmb));
        rt_kprintf("\n");
        used_size += mmb->size / 1024;
        ++block_number;
    }

    rt_kprintf("\n");
    vmm_list_for_each(pos, &p_vmm->vmm_node_list) {
        mmb = (struct vmm_node *)pos;
        unsigned long blank = (mmb->base_addr - last_phy_addr);
        if(blank > 0) {
            rt_kprintf("  BLANK%02d:  phys(0x%08lX~0x%08lX), size: %luKB\n",
                blank_number++, last_phy_addr, mmb->base_addr-1, blank / 1024);
        }
        last_phy_addr = (mmb)->base_addr+(mmb)->size;
    }
    free_size = mmz_total_size - used_size;
    rt_kprintf("\n-------------------");
    rt_kprintf("\nVMM Count:\n total=%d.%03dMB,"
           "used=%d.%03dMB,free=%d.%03dMB,nr_block=%d\n",
           mmz_total_size / 1024, (mmz_total_size % 1024) * 1000 / 1024,
           used_size / 1024, (used_size % 1024) * 1000 / 1024,
           free_size / 1024, (free_size % 1024) * 1000 / 1024,
           block_number);
    VMM_UNLOCK();
}

void rpc_vmm_dump_all_info(void *handle, struct vmm_control *p_vmm)
{
    unsigned int block_number = 0;
    unsigned int blank_number = 0;
    unsigned int used_size = 0;
    unsigned int free_size = 0;
    unsigned int mmz_total_size = 0;

    rt_list_t *pos;
    struct vmm_node *mmb;
    unsigned long last_phy_addr = p_vmm->begin_addr;

    if (!last_phy_addr) {
        ringbuffer_print(handle, "VMM not init!\n");
        return;
    }
    ringbuffer_print(handle, "VMM ZONE:  " MMZ_FMT_S "\n", mmz_fmt_arg(p_vmm));
    ringbuffer_print(handle, "-------------------\n");
    mmz_total_size += p_vmm->nbytes / 1024;
    VMM_DECLARE_LOCK();
    VMM_LOCK();
    vmm_list_for_each(pos, &p_vmm->vmm_node_list) {
        mmb = (struct vmm_node *)pos;
        ringbuffer_print(handle, "  BLOCK%02d:  " MMB_FMT_S, block_number,
                mmb_fmt_arg(mmb));
        ringbuffer_print(handle, "\n");
        used_size += mmb->size / 1024;
        ++block_number;
    }

    ringbuffer_print(handle, "\n");
    vmm_list_for_each(pos, &p_vmm->vmm_node_list) {
        mmb = (struct vmm_node *)pos;
        unsigned long blank = (mmb->base_addr - last_phy_addr);
        if(blank > 0) {
            ringbuffer_print(handle, "  BLANK%02d:  phys(0x%08lX~0x%08lX), size: %luKB\n",
                blank_number++, last_phy_addr, mmb->base_addr-1, blank / 1024);
        }
        last_phy_addr = (mmb)->base_addr+(mmb)->size;
    }
    free_size = mmz_total_size - used_size;
    ringbuffer_print(handle, "\n-------------------");
    ringbuffer_print(handle, "\nVMM Count:\n total=%d.%03dMB,"
           "used=%d.%03dMB,free=%d.%03dMB,nr_block=%d\n",
           mmz_total_size / 1024, (mmz_total_size % 1024) * 1000 / 1024,
           used_size / 1024, (used_size % 1024) * 1000 / 1024,
           free_size / 1024, (free_size % 1024) * 1000 / 1024,
           block_number);
    VMM_UNLOCK();
}
