
#include <fhconfig.h>

#ifdef FH_USING_XBRSH

#include <rtthread.h>
#include "xbus_core.h"
#include <asm/io.h>

#define RSH_DEV_NAME "rshell"

#define RSH_REAL_SIZE_MASTER_OUT (RSH_SIZE_MASTER_OUT - sizeof(struct rsh_ring))

extern struct rsh_ring *RSH_OUT;
extern struct rsh_ring *RSH_IN;
extern unsigned int     RSH_OUT_SZ;

#define RSH_READ8(addr)       readb((unsigned int)(addr))
#define RSH_READ32(addr)      readl((unsigned int)(addr))
#define RSH_WRITE32(addr,val) writel(val,(unsigned int)(addr))

static rt_err_t rsh_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t rsh_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_size_t rsh_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size )
{
    unsigned int rd;
    unsigned int wr;
    struct rsh_ring *rsh = RSH_IN;

    rd = RSH_READ32(&rsh->rdpos);
    wr = RSH_READ32(&rsh->wrpos);
    if (rd == wr)
        return 0;

    *((unsigned char *)buffer) = RSH_READ8(&rsh->buf[rd]);

    if (++rd >= RSH_REAL_SIZE_MASTER_OUT)
    {
        rd -= RSH_REAL_SIZE_MASTER_OUT;
    }

    /*update read pointer*/    
    RSH_WRITE32(&rsh->rdpos, rd);

    return 1;
}

static rt_size_t rsh_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    int i;
    int len = size;
    unsigned int data;
    unsigned int wr;
    struct rsh_ring *rsh = RSH_OUT;

    /*rd = RSH_READ32(&rsh->rdpos);*/
    wr = RSH_READ32(&rsh->wrpos);
    wr &= ~3; /*ensure 4Bytes aligned*/

    while (len > 0)
    {
        i = len > 4 ? 4 : len;
        len -= i;

        data = 0;
        while (i-- > 0)
        {
            *((unsigned char *)(&data) + i) = *((unsigned char *)buffer + i);
        }

        buffer += 4;

        RSH_WRITE32(&rsh->buf[wr], data);
        wr += 4;
        if (wr >= RSH_OUT_SZ)
        {
            wr -= RSH_OUT_SZ;
        }
    }

    /*update write pointer*/
    RSH_WRITE32(&rsh->wrpos, wr);

    return size;
}

static struct rt_device rsh_dev = 
{
    .open  = rsh_open,
    .close = rsh_close,
    .read  = rsh_read,
    .write = rsh_write,
};

rt_err_t rshell_device_register(void)
{
    if (RSH_OUT && RSH_IN)
    {
        rt_device_register(&rsh_dev, RSH_DEV_NAME, RT_DEVICE_FLAG_RDWR);
        rt_console_set_device(RSH_DEV_NAME);
    }
    return RT_EOK;
}

void change_console_to_rsh(void)
{
  rt_console_set_device(RSH_DEV_NAME);
}

#endif /* FH_USING_XBRSH */
