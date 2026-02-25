#ifndef __rpc_proto_control_h__
#define __rpc_proto_control_h__
#include <rpc_hdr.h>

#define RPC_VERSION_CONTROL (1)

#define RPC_FUNC_ID_FH_CTRL_DeinitAllModule (0)
#define RPC_FUNC_ID_FH_CTRL_FlushCache (1)
#define RPC_FUNC_ID_FH_CTRL_Customer  (2)
#define RPC_FUNC_ID_FH_Proc_Info      (3)
#define RPC_FUNC_ID_FH_Proc_Read      (4)
#define RPC_FUNC_ID_FH_Proc_Write     (5)

#define PROC_NAME_LEN_MAX (32)
#define PROC_INFO_SIZE_MAX (1024)

struct rpc_customer_cmd
{
    unsigned int  cmd;
    unsigned int  arg_bytes; /*bytes of arg, not include rpc_customer_cmd itself*/
    unsigned char arg[0];
};


struct rpc_proc_info
{
    struct rpc_hdr hdr; /*[IN,OUT]*/
    unsigned int ringbuffer; /*[OUT], ringbuffer address*/
    unsigned int ringbuffer_size; /*[OUT], ringbuffer size*/
    char names[0]; /*[OUT], names of all proc...*/
};

struct rpc_proc_read
{
    struct rpc_hdr hdr; /*[IN,OUT]*/
    char proc_name[PROC_NAME_LEN_MAX]; /*[IN]*/
};

struct rpc_proc_write
{
    struct rpc_hdr hdr; /*[IN,OUT]*/
    char proc_name[PROC_NAME_LEN_MAX]; /*[IN]*/
    unsigned int  count; /*[IN]*/
    char          buffer[0]; /*[IN]*/
};

#endif /*__rpc_proto_control_h__*/
