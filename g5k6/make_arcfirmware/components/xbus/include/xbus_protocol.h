#ifndef __xbus_protocol_h__
#define __xbus_protocol_h__

#define XBUS_ENABLE_CHECKSUM

#if defined(CONFIG_CHIP_FHCH) || \
    defined(CONFIG_CHIP_FHXGM) || \
    defined(CONFIG_CHIP_FHXGM2) || \
    defined(CONFIG_CHIP_FHCH2) || \
    defined(CONFIG_CHIP_MC632X) || \
    defined(CONFIG_CHIP_FHS16)
#define XBUS_CACHE_LINE_SIZE (64)
#define RSH_SIZE_MASTER_OUT  (512 - sizeof(struct xbus_sys_info))  /*relative to master ARM*/
#else
#define XBUS_CACHE_LINE_SIZE (32)
#define RSH_SIZE_MASTER_OUT  (256 - sizeof(struct xbus_sys_info))  /*relative to master ARM*/
#endif

#if XBUS_CACHE_LINE_SIZE >= 64
//#error "too big XBUS_CACHE_LINE_SIZE,struct rsh_ring->buf has no space, please check!"
#endif


/*
 * now use two cachelines,
 * if you change it larger, please ensure rsh_ring->buf is enough
 */
#define XBUS_INTR_CALLBACK_MEM_SIZE (XBUS_CACHE_LINE_SIZE * 2)

#define XBUS_ALIGNED_UP(x)   (((unsigned int)(x) + (XBUS_CACHE_LINE_SIZE - 1)) & (~(XBUS_CACHE_LINE_SIZE - 1)))

#define XBUS_CMD_TYPE_RPC      (0)
#define XBUS_CMD_TYPE_VBUS_COMPATIBLE (1)
#define XBUS_CMD_TYPE_MAX      (2) /*not include, don't use it*/
#define XBUS_CMD_TYPE_SHIFT    (28)
#define XBUS_CMD_TYPE(cmd)     ((unsigned int)(cmd) >> XBUS_CMD_TYPE_SHIFT)
#define XBUS_CMD_SUBCMD(cmd)   ((unsigned int)(cmd) & ((1<<XBUS_CMD_TYPE_SHIFT) - 1))
#define XBUS_CMD(type, subcmd) (((type)<<XBUS_CMD_TYPE_SHIFT) | subcmd)

#define XBUS_PKT_TYPE_CMD         (0x2A)
#define XBUS_PKT_TYPE_ACK         (0x2B)
#define XBUS_PKT_TYPE_NEED_ACK    (0x80)
#define XBUS_PKT_TYPE_M(pkt_type) ((pkt_type) & 0x3f)
#define XBUS_PKT_TYPE_N(pkt_type) ((pkt_type) & (0x3f|XBUS_PKT_TYPE_NEED_ACK))
#define XBUS_PKT_TYPE(type,ack)   ((type) | (ack))

#define XBUS_MAGIC_START    (0x5b5bb5b5)
#define XBUS_SYS_INFO_MAGIC (XBUS_MAGIC_START + 0)
#define XBUS_RSHOUT_MAGIC   (XBUS_MAGIC_START + 1) //relative to master ARM
#define XBUS_RSHIN_MAGIC    (XBUS_MAGIC_START + 2) //relative to master ARM
#define XBUS_XOUT_MAGIC     (XBUS_MAGIC_START + 3) //relative to master ARM
#define XBUS_XIN_MAGIC      (XBUS_MAGIC_START + 4) //relative to master ARM

#define DSP_FLAG1_LOCK      (0x23)
#define DSP_FLAG1_UNLOCK    (0x24)

struct xbus_sys_info
{
    volatile unsigned int   magic; /*must be XBUS_SYS_INFO_MAGIC*/
    volatile unsigned int   vmm_phy_addr;
    volatile unsigned int   vmm_size;
    volatile unsigned short rshout_sz;
    volatile unsigned short rshin_sz;
    volatile unsigned short xout_sz;
    volatile unsigned short xin_sz;
    volatile unsigned int   firmware_git_version;
#if defined(CONFIG_CHIP_FHCH) || defined(CONFIG_CHIP_FHCH2)
    volatile unsigned char  reserved[XBUS_CACHE_LINE_SIZE - 6 * sizeof(unsigned int)];
#else
    volatile unsigned int   dsp_flag1; /*DSP_FLAG1_XXX*/
    volatile unsigned char  reserved[XBUS_CACHE_LINE_SIZE - 7 * sizeof(unsigned int)];
#endif

    volatile unsigned int   cb_msg_info;
    volatile unsigned char  reserved2[XBUS_CACHE_LINE_SIZE - sizeof(unsigned int)];
    
    volatile unsigned int   intr_cb_mem[XBUS_INTR_CALLBACK_MEM_SIZE/sizeof(unsigned int)];

#if defined(CONFIG_CHIP_FHCH) || defined(CONFIG_CHIP_FHCH2)
    volatile unsigned int   dsp_flag1; /*DSP_FLAG1_XXX*/
    volatile unsigned char  reserved3[XBUS_CACHE_LINE_SIZE - sizeof(unsigned int)];
#endif
};

struct xbus_ring
{
    volatile unsigned int  magic; /*must be XBUS_XOUT_MAGIC or XBUS_XIN_MAGIC*/
    volatile unsigned int  rdpos;
    volatile unsigned char reserved1[XBUS_CACHE_LINE_SIZE - 2 * sizeof(unsigned int)];

    volatile unsigned int  wrpos;
    volatile unsigned char reserved2[XBUS_CACHE_LINE_SIZE - sizeof(unsigned int)];

    unsigned char buf[0];
};


/*Attention: must require sizeof(struct xbus_pkt) < XBUS_CACHE_LINE_SIZE*/
#define XBUS_MAGIC_PKT (0x6B706278) /*xbpk, xbus-packet magic*/
struct xbus_pkt
{
    /*magic,it must be XBUS_MAGIC_PKT*/
    volatile unsigned int   magic;

    /*packet length in bytes, not include the head itself.*/
    volatile unsigned short len;

    /*XBUS_CMD_FLAG_XXX*/
    volatile unsigned char  pkt_type;

    /*priority to execute this command*/
    volatile unsigned char  priority;

    /*checksum for the whole packet including the header itself*/
#ifdef XBUS_ENABLE_CHECKSUM
    volatile unsigned int   checksum;
#endif

    /*
     *packet command.
     *If it's a response packet, it must be same with original command packet
     */
    volatile unsigned int   command;

    /*packet sequence number,should not be zero.*/
    volatile unsigned int   seqno;

    /*packet content...*/
    unsigned char  buf[0];
};

#define RSH_CMD_EXEC_END (0xfe)
struct rsh_ring
{
    volatile unsigned int  magic; /*must be XBUS_RSHOUT_MAGIC or XBUS_RSHIN_MAGIC*/
    volatile unsigned int  rdpos;
    volatile unsigned char reserved1[XBUS_CACHE_LINE_SIZE - 2 * sizeof(unsigned int)];	

    volatile unsigned int  wrpos;
    volatile unsigned char reserved2[XBUS_CACHE_LINE_SIZE - sizeof(unsigned int)];

    unsigned char buf[0];
};

#endif /*__xbus_protocol_h__*/
