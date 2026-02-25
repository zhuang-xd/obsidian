#include <rtthread.h>
#include <rthw.h>
#include <asm/cacheflush.h>
#include <fh_chip.h>
#include "xbus_osl.h"
#include "rpc_slave.h"
#include <control_rpc_proto.h>
#include "../ringbuffer/ringbuffer.h"

struct proc_entry
{
    char *name;
    int (*read)(void *handle);
    int (*write)(char *buf, int size);
    struct proc_entry *next;
};


extern int user_reset_all_module(void);

extern void fh_hw_flush_dcache_all(void);

static int stub_FH_CTRL_DeinitAllModule(struct rpc_hdr *pkt, unsigned int pkt_len, void *priv)
{
   user_reset_all_module();
    pkt->ret = 0;
    return rpc_send_back(pkt, sizeof(struct rpc_hdr), priv);
}

static int stub_FH_CTRL_FlushCache(struct rpc_hdr *pkt, unsigned int pkt_len, void *priv)
{
    fh_hw_flush_dcache_all();

    pkt->ret = 0;
    return rpc_send_back(pkt, sizeof(struct rpc_hdr), priv);
}

#ifdef FH_USING_RPC_SOLUTION
extern int FH_RPC_Customer_Command(unsigned int cmd, void* arg, unsigned int len);
static int stub_FH_CTRL_Customer(struct rpc_hdr *pkt, unsigned int pkt_len, void *priv)
{
    int ret;
    struct rpc_customer_cmd *pcmd = (struct rpc_customer_cmd *)(pkt + 1);
    unsigned int len = pkt_len - sizeof(struct rpc_hdr);

    if (len < sizeof(struct rpc_customer_cmd) ||
            len < sizeof(struct rpc_customer_cmd) + pcmd->arg_bytes)
    {
        ret = -1;
    }
    else
    {
        ret = FH_RPC_Customer_Command(pcmd->cmd, pcmd->arg, pcmd->arg_bytes);
    }

    pkt->ret = ret;
    return rpc_send_back(pkt, pkt_len, priv);
}

static struct proc_entry *g_proc_list;
static char __attribute__((aligned(DCACHE_LINE_SIZE))) __ringbuffer_buf[1024];
static HRINGBUFFER g_proc_ringbuffer;

static struct proc_entry* find_proc_entry(char *name)
{
    register rt_ubase_t level;
    struct proc_entry *entry;

    level = rt_hw_interrupt_disable();
    entry = g_proc_list;
    while (entry)
    {
        if (rt_strcmp(entry->name, name) == 0)
        {
            break;
        }
        entry = entry->next;
    }

    rt_hw_interrupt_enable(level);
    return entry;
}

int proc_register(char *name,  int (*read)(void *handle), int (*write)(char *buf, int size))
{
    register rt_ubase_t level;
    struct proc_entry *entry;
    int len;

    if (!name)
        goto _error;

    len = rt_strlen(name);
    if (len <= 0 || len >= PROC_NAME_LEN_MAX)
        goto _error;

    level = rt_hw_interrupt_disable();

    entry = g_proc_list;
    while (entry)
    {
        if (rt_strcmp(entry->name, name) == 0)
        {
            entry->read = read;
            entry->write = write;
            break;
        }
        entry = entry->next;
    }

    if (!entry)
    {
        entry = (struct proc_entry *)rt_malloc(sizeof(struct proc_entry) + len + 1);
        if (!entry)
        {
            rt_hw_interrupt_enable(level);
            goto _error;
        }

        entry->name = (char *)(entry + 1);
        rt_memcpy(entry->name, name, len + 1);
        entry->read = read;
        entry->write = write;
        entry->next = g_proc_list;
        g_proc_list = entry;
    }

    rt_hw_interrupt_enable(level);
    return 0;

_error:
    return -1;
}

static int stub_FH_Proc_Info(struct rpc_hdr *pkt, unsigned int pkt_len, void *priv)
{
    struct rpc_proc_info *info;
    struct proc_entry *entry;
    char *name;
    int  room = PROC_INFO_SIZE_MAX;
    int  len;
    int  ret;

    if (!g_proc_ringbuffer)
    {
        g_proc_ringbuffer = ringbuffer_init(__ringbuffer_buf, sizeof(__ringbuffer_buf));
    }

    info = rt_malloc(room);
    if (!info)
    {
        pkt->ret = 0;
        return rpc_send_back(pkt, sizeof(struct rpc_hdr), priv);
    }

    info->hdr.ret = 0;
    info->hdr.ID = pkt->ID;
    info->ringbuffer = (unsigned int)g_proc_ringbuffer;
    info->ringbuffer_size = sizeof(__ringbuffer_buf);
    name = info->names;

    room -= sizeof(struct rpc_proc_info);
    entry = g_proc_list;
    while (entry)
    {
        len = rt_strlen(entry->name);
        if (len + 2 > room)
        {
            break;
        }

        if (len > 0)
        {
            rt_memcpy(name, entry->name, len + 1);
            name += (len + 1);
            room -= (len + 1);
        }

        entry = entry->next;
    }

    *(name++) = 0;
    ret = rpc_send_back((struct rpc_hdr *)info, name - (char *)info, priv);
    rt_free(info);
    return ret;
}

static int stub_FH_Proc_Read(struct rpc_hdr *pkt, unsigned int pkt_len, void *priv)
{
    struct proc_entry *entry;
    int (*read)(void *handle);

    entry = find_proc_entry(((struct rpc_proc_read *)pkt)->proc_name);
    pkt->ret = 0;
    rpc_send_back(pkt, sizeof(struct rpc_hdr), priv);

    if (entry)
    {
        read = entry->read;
        if (read)
        {
            read(g_proc_ringbuffer);
        }
    }

    ringbuffer_set_endflag(g_proc_ringbuffer);

    return 0;
}

static int stub_FH_Proc_Write(struct rpc_hdr *pkt, unsigned int pkt_len, void *priv)
{
    struct proc_entry *entry;
    struct rpc_proc_write *wrx = (struct rpc_proc_write *)pkt;
    int (*write)(char *buf, int size);

    entry = find_proc_entry(wrx->proc_name);
    if (entry)
    {
        write = entry->write;
        if (write)
        {
            write(wrx->buffer, wrx->count);
        }
    }

    pkt->ret = 0;
    rpc_send_back(pkt, sizeof(struct rpc_hdr), priv);

    return 0;
}
#else
static int stub_FH_Proc_Info(struct rpc_hdr *pkt, unsigned int pkt_len, void *priv)
{
    pkt->ret = 0;
    return rpc_send_back(pkt, sizeof(struct rpc_hdr), priv);
}

#endif //FH_USING_RPC_SOLUTION

static rpc_function_cb g_callback[] =
{
    stub_FH_CTRL_DeinitAllModule,  //(0)
    stub_FH_CTRL_FlushCache,  //(1)
#ifdef FH_USING_RPC_SOLUTION
    stub_FH_CTRL_Customer,   //(2)
    stub_FH_Proc_Info,       //(3)
    stub_FH_Proc_Read,       //(4)
    stub_FH_Proc_Write,      //(5)
#else
    (void*)0, //(2)
    stub_FH_Proc_Info, //(3)
    (void*)0, //(4)
    (void*)0, //(5)
#endif
};

int fh_control_register_stub(void)
{
    return rpc_register_table(RPC_VERSION_CONTROL, RPC_TABLE_ID_CONTROL, g_callback, sizeof(g_callback)/sizeof(rpc_function_cb));
}
