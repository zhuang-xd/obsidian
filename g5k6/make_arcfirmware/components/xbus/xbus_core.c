#include <rtthread.h>
#include <rthw.h>
#include <asm/arc6xx.h>
#include <fh_chip.h>
#include "xbus_osl.h"
#include "xbus_core.h"
#include "rpc_hdr.h"
#include "c2c_comm.h"
#include "rpc_slave.h"
#ifdef RT_USING_PM
#include <pm.h>
#endif

#define XBUS_CMD_QUEUE_NUM (RPC_PRIO_LVL_MAX)

struct cmd_queue
{
    /*list head for "struct xbus_pkt_list"*/
    struct list_head cmd_list;
    int              cmd_num;	
    int              idle_thread_num;
    XBUS_SEMA_T      sem;
};

static XBUS_SEMA_T         xbus_tx_lock;
static XBUS_SEMA_T         xbus_rx_sem;
static struct rt_thread    xbus_rx_thread;
static unsigned int        xbus_rx_stack[FH_XbusRx_THREAD_STACK_SIZE/4];

static struct xbus_debug   xbus_dbg;
static struct xbus_thr_dbg xbus_thread_debug[FH_XBUS_THREAD_NUM];
static struct cmd_queue    xbus_cmd_queue[XBUS_CMD_QUEUE_NUM];
static xbus_cmd_cb_t       xbus_cmd_cb_list[XBUS_CMD_TYPE_MAX];

extern struct xbus_sys_info *xbus_sysinfo;
extern struct xbus_ring*   XOUT_RING;
extern unsigned int        XOUT_RING_SZ;
extern struct xbus_ring*   XIN_RING;
extern unsigned int        XIN_RING_SZ;

#if !defined(CONFIG_CHIP_FH8626V100) && !defined(CONFIG_CHIP_FH885xV200)
static XBUS_SEMA_T         xbus_dspLock_sem;
static XBUS_SEMA_T         xbus_lpWait_sem;
static volatile int        xbus_dsp_flag;
volatile unsigned int dsp_loop_cancel;
#endif

/*
 * buf: must be 4Bytes aligned.
 */
#ifdef XBUS_ENABLE_CHECKSUM
static unsigned int calc_checksum(unsigned char *buf, unsigned int size)
{
    unsigned int i;
    unsigned int sum = 0;

    i = size & 3;
    size -= i;

    while (i > 0)
    {
        i--;
        sum = (sum << 8) | buf[size + i];
    }

    for (; i < size; i += 4)
    {
        sum += *((unsigned int *)(buf + i));
    }

    return sum;
}
#endif /* XBUS_ENABLE_CHECKSUM */

#if !defined(CONFIG_CHIP_FH8626V100) && !defined(CONFIG_CHIP_FH885xV200)
void xbus_dsp_down_sem(void)
{
    XBusSemDown(xbus_lpWait_sem);
}
#endif


static struct xbus_cb_entry *g_xbus_intr_cb_list;
void xbus_register_intr_callback(struct xbus_cb_entry *cb)
{
    XBUS_IRQ_FLAG_DECLARE(level);
    struct xbus_cb_entry *entry;

    XBUS_IRQ_SAVE(level);

    entry = g_xbus_intr_cb_list;
    while (entry)
    {
        if (entry == cb)
        {
            XBUS_IRQ_RESTORE(level);
            return;
        }
        entry = entry->next;
    }

    cb->next = g_xbus_intr_cb_list;
    g_xbus_intr_cb_list = cb;

    XBUS_IRQ_RESTORE(level);
}

void xbus_unregister_intr_callback(struct xbus_cb_entry *cb)
{
    XBUS_IRQ_FLAG_DECLARE(level);
    struct xbus_cb_entry *entry;
    struct xbus_cb_entry **pprev;

    XBUS_IRQ_SAVE(level);

    entry = g_xbus_intr_cb_list;
    pprev = &g_xbus_intr_cb_list;
    while (entry)
    {
        if (entry == cb)
        {
            *pprev = entry->next;
            break;
        }
        pprev = &entry->next;
        entry = entry->next;
    }

    XBUS_IRQ_RESTORE(level);
}


static void xbus_isr_arc(int irqnr, void *param)
{
    struct xbus_cb_entry *entry;

    xbus_dbg.intr_cnt++;

    /*clear interrupt*/
    fh_mcu_clear_interrupt();

#if !defined(CONFIG_CHIP_FH8626V100) && !defined(CONFIG_CHIP_FH885xV200)
    if (1)
    {
        unsigned int dsp_flag1;

        dsp_flag1 = readl(&xbus_sysinfo->dsp_flag1);
        if (dsp_flag1 == DSP_FLAG1_LOCK)
        {
            dsp_loop_cancel = 1;
            writel(0, (unsigned int)&xbus_sysinfo->dsp_flag1);
            xbus_dsp_flag = 1;
            XBusSemUp(xbus_dspLock_sem);
        }
        else if (dsp_flag1 == DSP_FLAG1_UNLOCK)
        {
            writel(0, (unsigned int)&xbus_sysinfo->dsp_flag1);
            xbus_dsp_flag = 0;
        }
    }

    if (xbus_lpWait_sem.value == 0)
    {
        XBusSemUp(xbus_lpWait_sem);
    }
#endif

    entry = g_xbus_intr_cb_list;
    while (entry)
    {
        entry->func(entry->arg);
        entry = entry->next;
    }

    /*wake up rx thread*/
    XBusSemUp(xbus_rx_sem);
}


static struct xbus_pkt_list* xbus_get_command(struct cmd_queue *queue)
{
    XBUS_IRQ_FLAG_DECLARE(level);
    struct xbus_pkt_list* pcmd = RT_NULL;

    /*I will sleep to wait if no rx-command, let other thread to run...*/
    XBusSemDown(queue->sem);

    /*disable interrupt*/
    XBUS_IRQ_SAVE(level);

    if (queue->cmd_num > 0)
    {
        pcmd = list_entry(queue->cmd_list.next, struct xbus_pkt_list, list);
        list_del(&pcmd->list);
        queue->cmd_num--;
        queue->idle_thread_num--;
    }

    /*enable interrupt*/
    XBUS_IRQ_RESTORE(level);

    return pcmd;
}

void xbus_free_queue_thread(struct xbus_thr_dbg *dbg)
{
    XBUS_IRQ_FLAG_DECLARE(level);

    if (dbg->quid > 0)
    {
        XBUS_IRQ_SAVE(level);
        xbus_cmd_queue[dbg->quid - 1].idle_thread_num++;
        dbg->quid = 0;
        XBUS_IRQ_RESTORE(level);
    }
}

static void xbus_handle_command_proc(void* arg)
{
    XBUS_IRQ_FLAG_DECLARE(level);
    unsigned char quid = (unsigned int)arg & 0xff;
    unsigned int cmd_type;
    struct xbus_pkt_list* pcmd;
    struct cmd_queue *queue =  &xbus_cmd_queue[quid];
    struct xbus_thr_dbg *dbg = &xbus_thread_debug[(unsigned int)arg >> 16];

    quid++;

    while (1)
    {
        pcmd = xbus_get_command(queue);
        if (!pcmd)
        {
            continue;
        }

        cmd_type = XBUS_CMD_TYPE(pcmd->blk.command);
        dbg->quid = quid; //now quid is bigger than zero...
        if (cmd_type < XBUS_CMD_TYPE_MAX && xbus_cmd_cb_list[cmd_type])
        {
            xbus_cmd_cb_list[cmd_type](pcmd, dbg); /*free pcmd in function xbus_cmd_cb_list*/
        }
        else
        {
            XBusFree(pcmd);
        }

        if (dbg->quid)
        {
            dbg->quid = 0;
            XBUS_IRQ_SAVE(level);
            queue->idle_thread_num++;
            XBUS_IRQ_RESTORE(level);
        }
    }
}

static unsigned int map_to_prio(struct xbus_pkt_list* pcmd)
{
    unsigned int prio = pcmd->blk.priority;

    if (prio == RPC_PRIO_DEFAULT)
        prio = RPC_PRIO_MID;
    if (prio > RPC_PRIO_LVL_MAX)
        prio = RPC_PRIO_LVL_MAX;
    return prio - 1;
}


#if !defined(CONFIG_CHIP_FH8626V100) && !defined(CONFIG_CHIP_FH885xV200)
static void xbus_dsp_lock_proc(void* arg)
{
    while (1)
    {
        XBusSemDown(xbus_dspLock_sem);
        while (xbus_dsp_flag);
    }
}
#endif

static int xbus_preprocess_cmd(struct xbus_pkt_list* pcmd)
{
    static int g_work_cnt = 0;

    unsigned int prio;
    unsigned int real_prio;
    unsigned int stacksz;
    XBUS_IRQ_FLAG_DECLARE(level);
    rt_thread_t tid;
    char name[16];
    struct cmd_queue *queue;

    /*get corresponding queue according to priority*/
    prio = map_to_prio(pcmd);
    queue = &xbus_cmd_queue[prio];

    XBUS_IRQ_SAVE(level);

    /*create worker thread if needed*/    
    if (queue->cmd_num + 1 > queue->idle_thread_num)
    {
        if (g_work_cnt < FH_XBUS_THREAD_NUM)
        {
            XBUS_IRQ_RESTORE(level);            
            rt_sprintf(name, "xbus-rx%02d", g_work_cnt);
            stacksz = FH_XBUS_THREAD_STACK_SIZE;
            real_prio = 8 + (prio << 2);
            if (prio == RPC_PRIO_EXT_SERV - 1) //in map_to_prio, it will sub one...
            {
                /*same with acw's acw-ll-txrx thread...*/
                stacksz = 16*1024;
                real_prio = 11;
            }
            tid = rt_thread_create(name, xbus_handle_command_proc, (void*)(prio|(g_work_cnt<<16)), stacksz, real_prio/*priority*/, 20);
            if (tid != RT_NULL)
            {
                g_work_cnt++;
                rt_thread_startup(tid);
            }
            XBUS_IRQ_SAVE(level);

            if (tid != RT_NULL)
            {
                queue->idle_thread_num++;
            }
        }
        else
        {
            xbus_dbg.over_thd_cnt++;
        }
    }

    /* put it into queue list */
    queue->cmd_num++;
    list_add_tail(&pcmd->list, &queue->cmd_list);

    XBUS_IRQ_RESTORE(level);

    /*let rx thread to get the command*/
    XBusSemUp(queue->sem);

    return 0;
}

/*
 * send xbus packet to peer
 */
int xbus_send(
        unsigned char  pkt_type,
        unsigned char  priority,
        unsigned int   cmd_status,
        unsigned int   seqno,
        unsigned char* buf,
        unsigned int   size)
{
    static unsigned int pktseq;

    unsigned int rd;
    unsigned int wr;
    unsigned int t;
    unsigned int times = 0;
    unsigned int pktsz;
    struct xbus_pkt* blk;
    struct xbus_ring* ring = XOUT_RING;
    unsigned int      ringsz = XOUT_RING_SZ;

    //validation checks...
    pktsz = XBUS_ALIGNED_UP(size + sizeof(struct xbus_pkt));
    if ((int)size < 0 || pktsz >= ringsz)
    {
        xbus_dbg.err_tx_large_pkt++;
        return -1;  //too large request size, don't support!!!
    }

    //get lock...    
    XBusSemDown(xbus_tx_lock);

    //wait buffer available...
    wr = readl((unsigned int)&ring->wrpos);
    while (1) 
    {
        rd = readl((unsigned int)&ring->rdpos);

        /*validation checks*/
        if ( ((rd | wr) & (XBUS_CACHE_LINE_SIZE - 1)) != 0 || rd >= ringsz || wr >= ringsz)
        {        	
            //XBusPrint("Xbus: error,OutRing=%p,rd=%08x,wr=%08x!\n", ring, rd, wr);
            xbus_dbg.err_tx_rdwr++;
            XBusSemUp(xbus_tx_lock);
            return -1;
        }

        if (wr >= rd)
            t = ringsz - (wr - rd) - 1;
        else
            t = rd - wr - 1;

        if (t >= pktsz)
        {
            break;
        }

        if (++times == 200)
        {
            xbus_dbg.err_tx_no_sndbuf++;
            XBusSemUp(xbus_tx_lock);
            return -1;
        }

        /*delay one tick*/
        XBusDelayTicks(1);
    }

    //write packet's header.
    blk = (struct xbus_pkt *)(ring->buf + wr);
    blk->magic = XBUS_MAGIC_PKT;
    blk->len  = size;
    blk->pkt_type = pkt_type;
    blk->priority = priority;    
#ifdef XBUS_ENABLE_CHECKSUM
    blk->checksum = 0;
#endif
    blk->command = cmd_status;
    if (!seqno)
    {
        if (++pktseq == 0)
            pktseq = 1;
        seqno = pktseq;
    }
    blk->seqno = seqno;
#ifdef XBUS_ENABLE_CHECKSUM
    blk->checksum = calc_checksum((unsigned char *)blk, sizeof(*blk)) + 
        calc_checksum(buf, size);
#endif

    /*write packet's segment 1*/
    t = ringsz - (wr + sizeof(struct xbus_pkt));
    if (t > size)
    {
        t = size;
    }

    if (t > 0)
    {
        XBusMemCpy(blk->buf, buf, t);
        buf  += t;
        size -= t;
    }
    MMU_FLUSH_DCACHE(blk, sizeof(struct xbus_pkt) + t);

    //write packet's segment 2.
    if (size > 0)
    {
        XBusMemCpy(ring->buf, buf, size);                
        MMU_FLUSH_DCACHE(ring->buf, size);
    }

    //update write pointer
    wr += pktsz;
    if (wr >= ringsz)
        wr -= ringsz;

    writel(wr, (unsigned int)&ring->wrpos);

    //trigger interrupt to peer
    fh_mcu_trigger_interrupt_to_host();

    XBusSemUp(xbus_tx_lock);

    return 0;
}

static int xbus_rx_once(unsigned char* ringbuf,
        unsigned int ringsz,
        unsigned int rd,
        unsigned int wr)
{
    unsigned int totlen;
    unsigned int pktsz;
    unsigned int blen;
    unsigned int seg1sz;
    unsigned int seg2sz;
    unsigned int origrd = rd;
    struct xbus_pkt* pblk;
    struct xbus_pkt_list* pcmd;
#ifdef XBUS_ENABLE_CHECKSUM
    unsigned int sum;
#endif

    totlen = wr > rd ? wr - rd : ringsz - rd + wr;

    while(rd != wr)
    {
        pblk   = (struct xbus_pkt *)(ringbuf + rd);
        blen   = pblk->len;
        seg1sz = sizeof(struct xbus_pkt) + blen;

        /*validation checks*/
        pktsz = XBUS_ALIGNED_UP(seg1sz);
        if (pktsz > totlen || 
                rd + sizeof(struct xbus_pkt) > ringsz || 
                pblk->magic != XBUS_MAGIC_PKT)
        {
            xbus_dbg.err_rx_rd++;
            break;
        }

        pcmd = (struct xbus_pkt_list *)XBusMalloc(sizeof(*pcmd) + blen);
        if (!pcmd)
        {
            xbus_dbg.err_rx_nomem++;
            break;
        }

        seg2sz = seg1sz;
        if (seg1sz > ringsz - rd)
            seg1sz = ringsz - rd;
        seg2sz -= seg1sz;

        XBusMemCpy(&pcmd->blk, ringbuf + rd, seg1sz);
        if (seg2sz > 0)
            XBusMemCpy((unsigned char*)&pcmd->blk + seg1sz, ringbuf, seg2sz);

#ifdef XBUS_ENABLE_CHECKSUM
        sum = calc_checksum((unsigned char*)(&pcmd->blk), sizeof(struct xbus_pkt) + pcmd->blk.len);
        if (sum - pcmd->blk.checksum != pcmd->blk.checksum)
        {
            xbus_dbg.err_rx_cksum++;
            XBusFree(pcmd);
            break;
        }
#endif

        if ((XBUS_PKT_TYPE_M(pcmd->blk.pkt_type) == XBUS_PKT_TYPE_CMD))
        {
            xbus_preprocess_cmd(pcmd);
        }
        else /*it should not happen*/
        {
            xbus_dbg.err_rx_unknown_pkt++;
            XBusFree(pcmd);
        }

        //update read pointer...
        totlen -= pktsz;
        rd += pktsz;
        if (rd >= ringsz)
            rd -= ringsz;
    }

    if (origrd <= wr)
    {
        MMU_INVALIDATE_DCACHE(ringbuf + origrd, wr - origrd);
    }
    else
    {
        MMU_INVALIDATE_DCACHE(ringbuf + origrd, ringsz - origrd);
        MMU_INVALIDATE_DCACHE(ringbuf, wr);
    }

    return 0;
}

/*
 * the real RX handle function
 */
static void xbus_rx_proc(void *param)
{
    unsigned int rd;
    unsigned int wr;
    struct xbus_ring *ring;
    unsigned int ringsz;

    ring = XIN_RING;
    ringsz = XIN_RING_SZ;

    rd = readl((unsigned int)&ring->rdpos);
    while (1)
    {
        wr = readl((unsigned int)&ring->wrpos);
        if (rd == wr)
        {
            /*wait for at most 4 ticks*/
            XBusSemDownTimeout(xbus_rx_sem, 4);
            continue;
        }

        //validation checks
        if ( ((rd | wr) & (XBUS_CACHE_LINE_SIZE - 1)) != 0 || 
                rd >= ringsz || 
                wr >= ringsz)
        {
            //XBusPrint("Xbus: error,InRing=%p,rd=%08x,wr=%08x!\n", ring, rd, wr);
            xbus_dbg.err_rx_rdwr++;
            break;
        }

        xbus_rx_once(ring->buf, ringsz, rd, wr);

        /*update read pointer*/
        rd = wr;
        writel(rd, (unsigned int)&ring->rdpos);
    }

    while (1)
        XBusDelayTicks(10);
}

void xbus_register_cmd_callback(unsigned int cmd_type, xbus_cmd_cb_t cb)
{
    if (cmd_type < XBUS_CMD_TYPE_MAX)
    {
        xbus_cmd_cb_list[cmd_type] = cb;
    }
}

#ifdef RT_USING_PM
int xbus_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    return RT_EOK;
}

void xbus_resume(const struct rt_device *device, rt_uint8_t mode)
{
    fh_c2c_init();
}

static struct rt_device_pm_ops xbus_pm_ops =
{
    .suspend = xbus_suspend,
    .resume = xbus_resume
};
#endif

int xbus_init(void)
{
    int i;
    struct cmd_queue *queue;

    if (!(XOUT_RING && XIN_RING))
        return -1;

#ifdef CONFIG_ARCH_FHZTV2
    int limit = 0;
    unsigned int reg;
    reg = readl(0xf000033c);
    if ( (reg & 0xffff) == 0x03 && ((reg >> 16) & 0x0f) == 0x01 ) /* it must be 8636 */
        limit = 1;
    if (reg == 0x00020003) /*8626v200*/
        limit = 1;
#ifdef FH_USING_RPC_SOLUTION
    if (limit)
    {
        return 0;
    }
#else
    if (limit)
    {
        reg = readl(0xf0000338);
        if (reg & (1<<2)) /*media process is running, so Moria is in fastboot mode...*/
        {
            return 0;
        }
    }
#endif
#endif /*CONFIG_ARCH_FHZTV2*/

    //init semaphore...
    XBusSemInit(xbus_tx_lock, "xbus_tx_lock", 1);
    XBusSemInit(xbus_rx_sem,  "xbus_rx_sem",  0);
    for (i = XBUS_CMD_QUEUE_NUM, queue = xbus_cmd_queue; i > 0; i--)
    {
        XBusSemInit(queue->sem, "xbus_rxlist", 0);
        INIT_LIST_HEAD(&queue->cmd_list);
        queue->cmd_num = 0;
        queue->idle_thread_num = 0;
        queue++;
    }

#if !defined(CONFIG_CHIP_FH8626V100) && !defined(CONFIG_CHIP_FH885xV200)
    if (1)
    {
        rt_thread_t tid;
        XBusSemInit(xbus_lpWait_sem,   "lpWait",  0);
        XBusSemInit(xbus_dspLock_sem,  "dspLock",  0);
        tid = rt_thread_create("dsplock_proc", xbus_dsp_lock_proc, (void *)0, FH_XbusDspLock_THREAD_STACK_SIZE, 7/*priority*/, 20);
        if (tid != RT_NULL)
        {
            rt_thread_startup(tid);
        }
    }
#endif

    rt_thread_init(&xbus_rx_thread, "xbus", xbus_rx_proc, RT_NULL, xbus_rx_stack, sizeof(xbus_rx_stack), 7/*priority*/, 20);
    rt_thread_startup(&xbus_rx_thread);
    rt_hw_interrupt_install(PMU_ARC_IRQn, xbus_isr_arc, RT_NULL, "xbus");
    rt_hw_interrupt_umask(PMU_ARC_IRQn);

    fh_c2c_init();

#ifdef RT_USING_PM
    rt_pm_device_register(RT_NULL, &xbus_pm_ops);
#endif

    return 0;
}

extern unsigned int g_trigger_intr_lost;

void xbus_dump(void)
{
    int i;
    struct xbus_thr_dbg* dbg = xbus_thread_debug;

    for (i=0; i<FH_XBUS_THREAD_NUM; i++)
    {
        XBusPrint("Th%d %02x/%02x,ID%02x-%08x\n", i, dbg->exit, dbg->enter, dbg->vbusCmd, dbg->ID);
        dbg++;
    }

    XBusPrint("intr %d\n", xbus_dbg.intr_cnt);
    XBusPrint("OverThd %d\n", xbus_dbg.over_thd_cnt);
    XBusPrint("txerr %d:%d:%d\n", xbus_dbg.err_tx_large_pkt, xbus_dbg.err_tx_rdwr, xbus_dbg.err_tx_no_sndbuf);
    XBusPrint("rxerr %d:%d:%d:%d:%d:%d\n", xbus_dbg.err_rx_rdwr, xbus_dbg.err_rx_rd, xbus_dbg.err_rx_nomem, 
    	          xbus_dbg.err_rx_cksum, xbus_dbg.err_rx_unknown_pkt, g_trigger_intr_lost);
}
