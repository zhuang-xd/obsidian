#include <rtthread.h>
#include <asm/io.h>
#include <fh_chip.h>
#include "ringbuffer.h"

#define LOGBUF_SIZE (128)

#define RSH_READ32(addr)      readl((unsigned int)(addr))
#define RSH_WRITE32(addr,val) writel(val,(unsigned int)(addr))

HRINGBUFFER ringbuffer_init(void* mem, int memsz)
{
    ringbuffer_t *r = (ringbuffer_t*)(((unsigned int)mem + (DCACHE_LINE_SIZE-1)) & (~(DCACHE_LINE_SIZE-1)));

    if (!mem || memsz < 512)
    {
        return RT_NULL;
    }

    memsz -= ((unsigned int)r - (unsigned int)mem);
    memsz &= (~(DCACHE_LINE_SIZE-1));

    RSH_WRITE32(&r->rdpos, 0);
    RSH_WRITE32(&r->wrpos, 0);
    RSH_WRITE32(&r->size,  memsz - sizeof(ringbuffer_t));

    return (HRINGBUFFER)r;
}

static int ringbuffer_write(ringbuffer_t *ring, unsigned int* pdata, int num)
{
    unsigned int retry_cnt = 0;
    unsigned int rd;
    unsigned int wr;
    unsigned int nextwr;
    unsigned int size;
    unsigned int timeout = RT_TICK_PER_SECOND;

    wr   = RSH_READ32(&ring->wrpos);
    size = RSH_READ32(&ring->size);
    if (num == 1 && *pdata == _END_FLAG_)
        timeout <<= 6;

retry:
    rd   = RSH_READ32(&ring->rdpos);
    while (num > 0)
    {
        nextwr = wr + 4;
        if (nextwr >= size)
        {
            nextwr -= size;
        }

        if (nextwr == rd)
        {
            if (++retry_cnt > timeout)
                break;            
            rt_thread_delay(1);
            goto retry;
        }

        RSH_WRITE32(&ring->buf[wr], *(pdata++));
        num--;
        wr = nextwr;
        retry_cnt = 0;
    }

    /*update write pointer*/
    RSH_WRITE32(&ring->wrpos, wr);

    return 0;
}

void ringbuffer_print(HRINGBUFFER h, const char *fmt, ...)
{
    va_list args;
    int length;
    unsigned int rt_log_buf[LOGBUF_SIZE/4];
    char *log = (char *)rt_log_buf;

    if (!h)
        return;

    va_start(args, fmt);
    /* the return value of vsnprintf is the number of bytes that would be
     * written to buffer had if the size of the buffer been sufficiently
     * large excluding the terminating null byte. If the output string
     * would be larger than the rt_log_buf, we have to adjust the output
     * length. */
    length = rt_vsnprintf(log, sizeof(rt_log_buf), fmt, args);
    if (length > sizeof(rt_log_buf))
        length = sizeof(rt_log_buf);

    /*for speed optimise, ensure length is multiple of 4*/
    while (length & 3)
    {
        log[length++] = 0;
    }

    ringbuffer_write((ringbuffer_t*)h, rt_log_buf, length/4);

    va_end(args);
}

void ringbuffer_set_endflag(HRINGBUFFER h)
{
    unsigned int flag = _END_FLAG_;

    if (!h)
        return;

    ringbuffer_write((ringbuffer_t*)h, &flag, 1);
}
