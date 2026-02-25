#ifndef __rpc_customer_proto_h__
#define __rpc_customer_proto_h__

#define _STD_MOD_CUSTOMER            (0x4e) //don't modify
#define RPC_CUSTOMER_ERR_CODE(code)  (0x80000000 | ((_STD_MOD_CUSTOMER)<<16) | ((0) << 12) | (code)) //no modify
#define RPC_CUSTOMER_ERR_OK          (0) //don't modify
#define RPC_CUSTOMER_ERR_PARAM       RPC_CUSTOMER_ERR_CODE(1) //don't modify
#define RPC_CUSTOMER_ERR_OPEN_DEVICE RPC_CUSTOMER_ERR_CODE(2) //don't modify
#define RPC_CUSTOMER_ERR_NOT_OPENED  RPC_CUSTOMER_ERR_CODE(3) //don't modify
#define RPC_CUSTOMER_ERR_NOMEM       RPC_CUSTOMER_ERR_CODE(4) //don't modify
#define RPC_CUSTOMER_ERR_NOT_SUPPORT RPC_CUSTOMER_ERR_CODE(5) //don't modify

#define RPC_CUSTOMER_ERR_1           RPC_CUSTOMER_ERR_CODE(6) //define your own error code like this...
#define RPC_CUSTOMER_ERR_2           RPC_CUSTOMER_ERR_CODE(7) //define your own error code like this...
#define RPC_CUSTOMER_ERR_3           RPC_CUSTOMER_ERR_CODE(8) //define your own error code like this...


#define RPC_CUSTOMER_CMD_1  (1) //you can modify...
#define RPC_CUSTOMER_CMD_2  (2) //you can modify...
//Allen-D
#define RPC_CUSTOMER_CMD_400WP30 (4)
#define RPC_CUSTOMER_CMD_400WP25 (5)
#define RPC_CUSTOMER_CMD_1080P30 (6)
#define RPC_CUSTOMER_CMD_1080P25 (7)
#define RPC_CUSTOMER_CMD_720P30  (8)
#define RPC_CUSTOMER_CMD_720P25  (9)
#define RPC_CUSTOMER_CMD_320P30  (10)
#define RPC_CUSTOMER_CMD_320P25  (11)
#define RPC_CUSTOMER_CMD_CLOSE   (12)

struct rpc_customer_cmd_1  //you can modify
{
    unsigned int a;
    unsigned int b;
    unsigned int c;
};

struct rpc_customer_cmd_2 //you can modify
{
    unsigned int  d;
    unsigned char t[128];
};

#endif /*__rpc_customer_proto_h__*/
