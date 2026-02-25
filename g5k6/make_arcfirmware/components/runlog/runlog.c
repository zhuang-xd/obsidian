#include <rthw.h>
#include <rtthread.h>

#include <fh_chip.h>
#include <asm/cacheflush.h>

#define MAX_TRACE_NODE (16)

struct trace_node
{
    unsigned int seqno;
    unsigned int para0;
    unsigned int para1;
    unsigned int para2;
    char  name[DCACHE_LINE_SIZE-16];  //just one cache-line
};

static unsigned int      g_trace_pos;
static unsigned short    g_trace_seqno;
struct trace_node* g_trace_list;

static unsigned char __attribute__((aligned(DCACHE_LINE_SIZE))) g_runlog_buf[sizeof(struct trace_node) * MAX_TRACE_NODE];

void runlog_init(void)
{
    unsigned char *buf;

    buf = g_runlog_buf;
    g_trace_list = (struct trace_node *)buf;
    rt_kprintf("log buff:%08x.\n", buf);
}

void runlog_add_node(unsigned int para0, unsigned int para1, unsigned int para2, char* name)
{
    int i;
    struct trace_node* n;
    register rt_base_t level;

    if (!g_trace_list)
        return;

    level = rt_hw_interrupt_disable();

    n = &g_trace_list[g_trace_pos];
    if (++g_trace_pos >= MAX_TRACE_NODE)
        g_trace_pos = 0;
    n->seqno = ++g_trace_seqno;
    n->para0 = para0;
    n->para1 = para1;
    n->para2 = para2;

    i = 0;
    if (name)
    {
        while (i < sizeof(n->name))
        {
            if (!name[i])
                break;
            n->name[i] = name[i];
            i++;
        }
    }

    while (i < sizeof(n->name))
    {
        if (!n->name[i])
            break;
        n->name[i++] = 0;
    }

    mmu_clean_dcache_intron((rt_uint32_t)n, sizeof(*n));

    rt_hw_interrupt_enable(level);
}
