#ifndef __xbus_core_h__
#define __xbus_core_h__

#include "xbus_list.h"
#include "xbus_protocol.h"

struct xbus_pkt_list
{
    struct list_head list;
    struct xbus_pkt  blk;
};

struct xbus_debug
{
    unsigned int intr_cnt;
    unsigned int over_thd_cnt;
    
    unsigned int err_tx_large_pkt;
    unsigned int err_tx_rdwr;
    unsigned int err_tx_no_sndbuf;

    unsigned int err_rx_rdwr;
    unsigned int err_rx_rd;
    unsigned int err_rx_nomem;
    unsigned int err_rx_cksum;
    unsigned int err_rx_unknown_pkt;
};

struct xbus_thr_dbg
{
    unsigned int  ID;
    unsigned char vbusCmd;
    unsigned char enter;
    unsigned char exit;
    volatile unsigned char quid; //FIXME, just use it...
};

typedef int (*xbus_cmd_cb_t)(struct xbus_pkt_list *pkt, struct xbus_thr_dbg *dbg);

extern void xbus_register_cmd_callback(unsigned int cmd_type, xbus_cmd_cb_t cb);

extern void xbus_free_queue_thread(struct xbus_thr_dbg *dbg);

extern int xbus_send(
        unsigned char  flags,
        unsigned char  priority,
        unsigned int   cmd_status,
        unsigned int   seqno,
        unsigned char* buf,
        unsigned int   size);


#endif /*__xbus_core_h__*/
