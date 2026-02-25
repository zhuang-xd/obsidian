#ifndef __rpc_slave_h__
#define __rpc_slave_h__
#include "rpc_hdr.h"

typedef int (*rpc_function_cb)(struct rpc_hdr *pkt, unsigned int pkt_len, void *priv);

extern int rpc_register_table(unsigned int version, unsigned int table_id, rpc_function_cb *cb_list, unsigned int cb_num);

extern int rpc_send_back(struct rpc_hdr *pkt, unsigned int pkt_len, void *priv);

/*
 * just put it here
 * Notice: this function can only be called in interrupt context.
 */
extern void xbus_trigger_host_intr(unsigned short type, void *args, unsigned short len);


struct xbus_cb_entry
{
    void (*func)(void *arg);
    void *arg;
    struct xbus_cb_entry *next;
};


extern void xbus_register_intr_callback(struct xbus_cb_entry *cb);
extern void xbus_unregister_intr_callback(struct xbus_cb_entry *cb);


#endif //__rpc_slave_h__
