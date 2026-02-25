#ifndef __rpc_hdr_h__
#define __rpc_hdr_h__

struct xbus_ioc_hdr
{
    unsigned int   out_len_min; /*let compiler 4Bytes aligned*/
    unsigned short out_len_max;
    unsigned short in_len;
};

#define RPC_TABLE_ID_ACW      (0)
#define RPC_TABLE_ID_HUMAN    (1)
#define RPC_TABLE_ID_FASTNN   (1) /*just reuse, human is not used now*/
#define RPC_TABLE_ID_VMM      (2)
#define RPC_TABLE_ID_CONTROL  (3)
#define RPC_TABLE_ID_TINY_NN  (4)
#define RPC_TABLE_ID_ISP0     (5)
#define RPC_TABLE_ID_ISP1     (6)
#define RPC_TABLE_ID_ISP2     (7)
#define RPC_TABLE_ID_ISP3     (8)
#define RPC_TABLE_ID_ISP4     (9)
#define RPC_TABLE_ID_DSP0     (10)
#define RPC_TABLE_ID_DSP1     (11)
#define RPC_TABLE_ID_DSP2     (12)
#define RPC_TABLE_ID_DSP3     (13)
#define RPC_TABLE_ID_DSP4     (14)
#define RPC_TABLE_ID_DSP5     (15)
#define RPC_TABLE_ID_DSP6     (16)
#define RPC_TABLE_ID_DSP7     (17)
#define RPC_TABLE_ID_DSP8     (18)
#define RPC_TABLE_ID_DSP9     (19)
#define RPC_TABLE_ID_VB       (20)
#define RPC_TABLE_ID_MAX      (21) /*not included, don't use it...*/

#define RPC_GRP_ID_NONE        (0)
#define RPC_GRP_ID_ACW_IOCTL   (1)
#define RPC_GRP_ID_HUMAN_IOCTL (2)
#define RPC_GRP_ID_FASTNN_IOCTL (2) /*just reuse, human is not used now*/
#define RPC_GRP_ID_MAX         (3) /*not included, don't use it...*/

#define RPC_PRIO_DEFAULT      (0) /*now, same with RPC_PRIO_MID*/
#define RPC_PRIO_HIGHEST      (1)
#define RPC_PRIO_HIGH         (2)
#define RPC_PRIO_MID          (3)
#define RPC_PRIO_LOW          (4)
#define RPC_PRIO_LOWEST       (5)
#define RPC_PRIO_EXT_SERV     (6) /*special for audio service,because it need large stack*/
#define RPC_PRIO_LVL_MAX      (6) /*include...*/

/*
 * version:  8bit
 * grp_id:   6bit
 * priority: 4bit
 * table_id: 5bit
 * func_id:  9bit
 */

#define RPC_VERSION_BITS       (8)
#define RPC_FUNC_ID_BITS       (9)

#define RPC_XID(version, grp_id, priority, table_id, func_id) (((version)<<24) | ((grp_id)<<18) | ((priority)<<14) | ((table_id)<<9) | (func_id))
#define RPC_ID(version, grp_id, table_id, func_id) RPC_XID(version, grp_id, RPC_PRIO_DEFAULT, table_id, func_id)
#define RPC_VERSIONX(ID)       (((unsigned int)(ID) >> 24) & 0xff)
#define RPC_GRP_ID(ID)         (((unsigned int)(ID) >> 18) & 0x3f)
#define RPC_PRIO(ID)           (((unsigned int)(ID) >> 14) & 0x0f)
#define RPC_TABLE_ID(ID)       (((unsigned int)(ID) >> 9)  & 0x1f)
#define RPC_FUNC_ID(ID)        ((ID) & 0x1ff)
#define RPC_TABLE_FUNC_ID(ID)  ((ID) & 0x3fff)

struct rpc_hdr
{
    /*function ID to excecute on peer*/
    unsigned int ID;
    int          ret;

    unsigned char param[0];
};

#endif /*__rpc_hdr_h__*/
