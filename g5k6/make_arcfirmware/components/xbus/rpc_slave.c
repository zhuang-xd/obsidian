#include <rtthread.h>
#include <rthw.h>
#include "xbus_osl.h"
#include "xbus_core.h"
#include "rpc_slave.h"

struct table_item
{
    rpc_function_cb* cb_list;
    unsigned short   cb_num;
    unsigned short   version;
};

static struct table_item table_list[RPC_TABLE_ID_MAX];

static int rpc_rx_callback(struct xbus_pkt_list* pkt, struct xbus_thr_dbg *dbg)
{
    int ret;
    int tk;
    rpc_function_cb *cb_list;
    unsigned int cb_num;
    unsigned int version;
    unsigned int table_id;
    unsigned int func_id;
    struct rpc_hdr *rpc = (struct rpc_hdr *)pkt->blk.buf;
    unsigned int pkt_len = pkt->blk.len;

    version  = RPC_VERSIONX(rpc->ID);
    table_id = RPC_TABLE_ID(rpc->ID);
    func_id  = RPC_FUNC_ID(rpc->ID);

    if (pkt_len >= sizeof(struct rpc_hdr) && table_id < RPC_TABLE_ID_MAX)
    {
        cb_list = table_list[table_id].cb_list;
        cb_num  = table_list[table_id].cb_num;
        if (cb_num == 1)
            func_id = 0; //force to zero....
        if (cb_list && 
                func_id < cb_num && 
                cb_list[func_id] && 
                version == table_list[table_id].version)
        {
            dbg->vbusCmd = 0;
            dbg->ID = rpc->ID;
            dbg->enter++;
            pkt->blk.magic = (unsigned int)dbg; //FIXME, just use it...
            tk = rt_tick_get();
            ret = cb_list[func_id](rpc, pkt_len, pkt);
            tk = rt_tick_get() - tk;
            if (tk >= RT_TICK_PER_SECOND * 5)
            {
                rt_kprintf("RPCID%08x take %d tick!\n", rpc->ID, tk);
            }
            dbg->exit++;
            XBusFree(pkt);
            return ret;
        }
    }

    XBusPrint("%s: unhandled rpc, ID=%08x,pktlen=%d.\n", __func__, rpc->ID, pkt_len);
    XBusFree(pkt);
    return -1;
}

int rpc_register_table(unsigned int version, unsigned int table_id, rpc_function_cb *cb_list, unsigned int cb_num)
{
    XBUS_IRQ_FLAG_DECLARE(level);

    if (version >= (1<<RPC_VERSION_BITS))
    {
        XBusPrint("%s: version error!\n", __func__);
        return -1;
    }

    XBUS_IRQ_SAVE(level);
    if (table_id < RPC_TABLE_ID_MAX && cb_num < (1<<RPC_FUNC_ID_BITS))
    {
        table_list[table_id].cb_list = cb_list;
        table_list[table_id].cb_num  = cb_num;
        table_list[table_id].version = version;
    }
    XBUS_IRQ_RESTORE(level);

    return 0;
}

int rpc_send_back(struct rpc_hdr *pkt, unsigned int pkt_len, void *priv)
{
    //we must increase before xbus_send...
    xbus_free_queue_thread((struct xbus_thr_dbg*)(((struct xbus_pkt_list*)priv)->blk.magic));

    return xbus_send(XBUS_PKT_TYPE(XBUS_PKT_TYPE_ACK, 0), 0, 0,
            ((struct xbus_pkt_list*)priv)->blk.seqno,
            (unsigned char *)pkt,
            pkt_len);
}

int rpc_init_slave(void)
{
    xbus_register_cmd_callback(XBUS_CMD_TYPE_RPC, rpc_rx_callback);

    return RT_EOK;
}
