#include <rtthread.h>
#include <rthw.h>
#include "xbus_core.h"
#include "vbus_protocol.h"
#include "xbus_osl.h"
#include <fh_vbus_dev.h>

static const char* g_service_name[VBUS_PROTO_CHANNEL_MAX];
static rt_device_t g_vbus_dev[VBUS_PROTO_CHANNEL_MAX];

rt_err_t rt_vbus_register_service(const char *sname, unsigned int priority, int is_server)
{
    return 0;
}

static int register_vbus_dev(rt_device_t dev, const char *servname)
{
    int i;
    int ch = -1;
    XBUS_IRQ_FLAG_DECLARE(level);

    XBUS_IRQ_SAVE(level);
    for(i=1; i<VBUS_PROTO_CHANNEL_MAX; i++) /*channel 0 is for VBUS_PROTO_CHANNEL_CONTROL*/
    {
        if (!g_service_name[i])
        {
            ch = i;
            g_service_name[ch] = servname;
            g_vbus_dev[ch] = dev;
            break;
        }
    }
    XBUS_IRQ_RESTORE(level);

    return ch;
}

static int vbus_find_channel(const char* name)
{
    int i;
    int ch = -1;
    XBUS_IRQ_FLAG_DECLARE(level);

    XBUS_IRQ_SAVE(level);
    for(i=1; i<VBUS_PROTO_CHANNEL_MAX; i++)
    {
        if (g_service_name[i] && name && !XBusStrCmp(g_service_name[i], name))
        {
            ch = i;
            break;
        }
    }
    XBUS_IRQ_RESTORE(level);

    return ch;
}


int rt_vbus_device_register(fh_vbus_dev_t *vbusdev, const char *devname, const char *servname, rt_uint32_t flag)
{
    int ch;
    rt_err_t ret;
    rt_device_t dev;


    ret = rt_device_register(&vbusdev->rtdev, devname, flag);
    if (ret == RT_EOK )
    {
        dev = rt_device_find(devname);
        if(dev)
        {
            rt_device_init(dev);
            ch = register_vbus_dev(dev, servname);
            if (ch >= 0)
            {
                return RT_EOK;
            }
        }
    }

    return -1;
}

int rt_vbus_device_register_ex(fh_vbus_dev_t *vbusdev, const char *serv_name, int priority, rt_uint32_t flag)
{
    char devname[RT_NAME_MAX];
    int len = 0;

    if (serv_name)
        len = XBusStrLen(serv_name);

    if (len <= 0 || len >= (RT_NAME_MAX-2))
    {
        XBusPrint("VBUS: error serv_name!\n");
        return -RT_ERROR;
    }

    XBusMemCpy(devname, serv_name, len);
    devname[len] = '_';
    devname[len+1] = 'd';
    devname[len+2] = 0;	

    return rt_vbus_device_register(vbusdev, devname, serv_name, flag);
}

int rt_vbus_init_callback(const char *servname, int priority)
{
    return 0;
}

rt_err_t rt_vbus_post_cbmesg(const char *sname, char *mesg, int msglen)
{
    int len;

    struct vbus_cbmesg msg;

    if (!sname || *sname == 0 || (unsigned int)msglen > sizeof(msg.cbmesg))
    {
        goto _Error;
    }

    len = XBusStrLen(sname);
    if (len >= sizeof(msg.service_name))
    {
        goto _Error;
    }

    XBusMemCpy(msg.service_name, sname, len+1);
    msg.cbmesg_len = msglen;
    XBusMemCpy(msg.cbmesg, mesg, msglen);

    xbus_send(XBUS_PKT_TYPE(XBUS_PKT_TYPE_CMD, 0), /*dont' need ACK*/
            0,
            XBUS_CMD(XBUS_CMD_TYPE_VBUS_COMPATIBLE, 0), 
            0/*seqno*/,
            (unsigned char *)&msg, 
            sizeof(msg));
    return 0;

_Error:	
    return -1;
}

//only used on LB2(FH8632) platform
void triger_bgm_mesg(unsigned int lines)
{
}

static int vbus_compatible_callback(struct xbus_pkt_list* pkt, struct xbus_thr_dbg *dbg)
{
    int ret;
    int tk;
    unsigned int cmd;
    unsigned int channel;
    rt_device_t  pdev;
    vbus_proto_ioctl_T *ioc;
    unsigned int pkt_len = pkt->blk.len;

    dbg->enter++;

    if (pkt_len < sizeof(struct vbus_proto_hdr))
    {
        goto Error;
    }

    ioc = (vbus_proto_ioctl_T *)(pkt->blk.buf);
    channel = ioc->hdr.channel;
    cmd     = ioc->hdr.cmd_status;

    dbg->vbusCmd = cmd;

    if (cmd == VBUS_PROTO_CMD_CONNECT)
    {
        ret = vbus_find_channel((const char *)(((vbus_proto_connect_T *)ioc)->serv_name));
    }
    else
    {    
        if (channel >= VBUS_PROTO_CHANNEL_MAX ||
                (pdev = g_vbus_dev[channel]) == RT_NULL)
        {
            goto Error;
        }

        switch (cmd)
        {
            case VBUS_PROTO_CMD_IOCTL:
                dbg->ID = ioc->ioc_command;
                tk = rt_tick_get();
                ret = rt_device_control(pdev, ioc->ioc_command, ioc->arg);
                tk = rt_tick_get() - tk;
                if (tk >= RT_TICK_PER_SECOND * 5)
                {
                    rt_kprintf("CH%d CMD%08x take %d tick!\n", channel, ioc->ioc_command, tk);
                }
                break;

            case VBUS_PROTO_CMD_OPEN:
                ret = rt_device_open(pdev, RT_DEVICE_OFLAG_RDWR);
                break;

            case VBUS_PROTO_CMD_CLOSE:
                ret = rt_device_close(pdev);
                break;

            default:
                goto Error;
                break;
        }
    }

    dbg->exit++;

    ioc->hdr.cmd_status = (unsigned int)ret;

    xbus_free_queue_thread(dbg);

    xbus_send(XBUS_PKT_TYPE(XBUS_PKT_TYPE_ACK, 0), 0, 0, pkt->blk.seqno, (unsigned char *)ioc, pkt_len);

    XBusFree(pkt);
    return 0;

Error:
    dbg->exit++;
    XBusFree(pkt);
    XBusPrint("vbus: error@callback!\n");
    return -1;
}

int vbus_compatible_init_arc(void)
{
    xbus_register_cmd_callback(XBUS_CMD_TYPE_VBUS_COMPATIBLE, vbus_compatible_callback);

    return RT_EOK;
}

