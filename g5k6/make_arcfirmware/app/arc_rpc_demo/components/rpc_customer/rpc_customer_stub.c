#include <rtthread.h>
#include "rpc_customer_proto.h"

/*
 * 功能：
 *      此函数对ARM端同名函数进行处理，并将处理结果返回ARM端。
 *
 * 返回值:
 *      返回值会原封不动传回主CPU端；
 *      大于等于0表示成功；
 *      小于0表示失败，代表错误码，客户可以增加自己的错误码；
 */
int FH_RPC_Customer_Command(unsigned int cmd, void* arg, unsigned int len)
{
    int ret = 0;
    rt_kprintf("[DEBUG] Enter xbus\n");

    cmd &= 0xffff; //低16位有效，高16位保留
    switch (cmd)
    {
        case RPC_CUSTOMER_CMD_1:
            {
                struct rpc_customer_cmd_1 *c1 = (struct rpc_customer_cmd_1 *)arg;
                rt_kprintf("RPC cmd RPC_CUSTOMER_CMD_1:a=%08x,b=%08x,c=%08x.\n", c1->a, c1->b, c1->c);
                c1->a += 1;
                c1->b += 2;
                c1->c += 3;
                ret = 0x12345678; /*return value will bring back to linux side.*/
            }
            break;
        case RPC_CUSTOMER_CMD_2:
            ret = 0;
            break;
        //Allen-D
        case RPC_CUSTOMER_CMD_400WP30:
            ret = change_mode(0,RPC_CUSTOMER_CMD_400WP30);
            rt_kprintf("[DEBUG]change mode to jpeg 400WP30, ret = %d\n", ret);
            break;
        case RPC_CUSTOMER_CMD_400WP25:
            ret = change_mode(0,RPC_CUSTOMER_CMD_400WP25);
            rt_kprintf("[DEBUG]change mode to jpeg 400WP25, ret = %d\n", ret);
            break;
        case RPC_CUSTOMER_CMD_1080P30:
            ret = change_mode(0,RPC_CUSTOMER_CMD_1080P30);
            rt_kprintf("[DEBUG]change mode to jpeg 1080P30, ret = %d\n", ret);
            break;
        case RPC_CUSTOMER_CMD_1080P25:
            ret = change_mode(0,RPC_CUSTOMER_CMD_1080P25);
            rt_kprintf("[DEBUG]change mode to jpeg 1080P25, ret = %d\n", ret);
            break;
        case RPC_CUSTOMER_CMD_720P30:
            ret = change_mode(0,RPC_CUSTOMER_CMD_720P30);
            rt_kprintf("[DEBUG]change mode to jpeg 720P30, ret = %d\n", ret);
            break;
        case RPC_CUSTOMER_CMD_720P25:
            ret = change_mode(0,RPC_CUSTOMER_CMD_720P25);
            rt_kprintf("[DEBUG]change mode to jpeg 720P25, ret = %d\n", ret);
            break;
        case RPC_CUSTOMER_CMD_320P30:
            ret = change_mode(0,RPC_CUSTOMER_CMD_320P30);
            rt_kprintf("[DEBUG]change mode to jpeg 320P30, ret = %d\n", ret);
            break;
        case RPC_CUSTOMER_CMD_320P25:
            ret = change_mode(0,RPC_CUSTOMER_CMD_320P25);
            rt_kprintf("[DEBUG]change mode to jpeg 320P25, ret = %d\n", ret);
            break;
        case RPC_CUSTOMER_CMD_CLOSE:
            ret = user_reset_all_module_EXT();
            rt_kprintf("[DEBUG]close camera, ret = %d\n", ret);
            break;
        default:
            ret = RPC_CUSTOMER_ERR_NOT_SUPPORT; //not support command
            break;
    }

    return ret;
}
