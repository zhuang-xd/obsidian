#ifndef __vbus_protocol_h__
#define __vbus_protocol_h__

#define	VBUS_PROTO_CMD_CONNECT  (1)
#define	VBUS_PROTO_CMD_OPEN     (2)
#define	VBUS_PROTO_CMD_CLOSE    (3)
#define	VBUS_PROTO_CMD_IOCTL    (4)
#define	VBUS_PROTO_CMD_SHMREAD  (5)
#define	VBUS_PROTO_CMD_SHMWRITE (6)

#define VBUS_PROTO_CHANNEL_CONTROL (0)
#define VBUS_PROTO_CHANNEL_MAX     (4) /*not included...*/
#define VBUS_PROTO_SERV_NAME_MAX   (16)

struct vbus_proto_hdr
{
	unsigned int channel;    /*IN*/
    unsigned int cmd_status; /*IN: command; OUT:ret status*/
};

/*only for rt_vbus_post_cbmesg*/
#define VBUS_SERV_NAME_MAX (11)
struct vbus_cbmesg
{
	unsigned char service_name[VBUS_SERV_NAME_MAX];
	unsigned char cbmesg_len;
	unsigned char cbmesg[8];
};

/*the service channel is returned through cmd_status*/
typedef struct
{
    struct vbus_proto_hdr hdr;
    unsigned char serv_name[VBUS_PROTO_SERV_NAME_MAX]; /*IN*/
    unsigned int  prio; /*IN*/
} vbus_proto_connect_T;

/*the ioctl return-value is returned through cmd_status*/
typedef struct
{
    struct vbus_proto_hdr hdr;
    unsigned int  ioc_command; /*IN*/
    unsigned char arg[0]; /*IN-OUT*/
} vbus_proto_ioctl_T;

/*the open/close return-value is returned through cmd_status*/
typedef struct 
{
	struct vbus_proto_hdr hdr;
} vbus_proto_openclose_T;

/*the read/write return-value is returned through cmd_status*/
typedef struct 
{
    struct vbus_proto_hdr hdr;
    unsigned int  phy_addr; /*IN*/
    unsigned int  size;     /*IN*/
} vbus_proto_shmrw_T;

#endif /*__vbus_protocol_h__*/
