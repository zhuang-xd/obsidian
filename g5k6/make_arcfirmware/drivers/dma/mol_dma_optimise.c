#if defined(CONFIG_CHIP_FHS16) || \
    defined(CONFIG_CHIP_MC632X)

#ifdef CONFIG_CHIP_MC632X
#define ARC_DMA_CH_OFFSET (24)
#else
#define ARC_DMA_CH_OFFSET (0)
#endif
#include <types/type_def.h>
#include <fh_def.h>
#include "fh_dma.h"
#include <fh_arch.h>
#include <rthw.h>
#include <asm/io.h>
#include <asm/cacheflush.h>
#include <fh_chip.h>

#define MC_DESC_ALLIGN             64  /*DO NOT TOUCH!!!*/
#define MC_DMA_MAX_NR_CHANNELS     32
#define MC_DMA_MAX_NR_CHANNELS_APP 4
#define MC_DMA_MAX_HAND_SHAKE_NO   64

//channel def
#define CHAN_PAUSE	1
#define CHAN_ACTIVE	0
#define CHAN_PAUSE_BIT_MASK	1
#define LLIST_EN					   (1<<4)

//desc def
#define BLK_LEN_MASK					0x1ffff
#define FRAG_LEN_MASK					0x1ffff
#define TRSC_LEN_MASK					0xfffffff

#define REQ_MODE_SHIFT					24
#define REQ_MODE_MASK					0x3


#define WORD_STEP						0x4
#define DST_TRSF_STEP_SHIFT				16
#define TRSF_STEP_MASK					0xffffffff
#define LLIST_END						1
#define NOT_LLIST_END					0
#define LLIST_END_SHIFT					19


#define SRC_DST_AHB_SIZE_MASK			0xf
#define DST_AHB_SIZE_SHIFT				28
#define SRC_AHB_SIZE_SHIFT				30
#define ADDR_FIX_EN_SHIFT				20
#define ADDR_FIX_SEL_SHIFT				21
#define ADDR_WRAP_EN_SHIFT				22


#define DMA_PAUSE			BIT(0)
#define DMA_PAUSE_STAT		BIT(2)
#define GLB_REG_CLK_EN		BIT(16)
#define CHNL_REG_CLK_EN		BIT(17)
#define REQ_CID_CLK_EN		BIT(18)
#define INT_REG_CLK_EN		BIT(19)
#define AXI_MST_CLK_EN		BIT(20)
#define AUDIO_CNT_CLK_EN	BIT(21)

#define SWT_MODE_SHIFT              26
#define SWT_MODE_MASK               3


#define CH_ENABLE					1

#define CH_FRAGMENT_INT_EN				(0x1<<0)
#define CH_BLK_INT_EN					(0x1<<1)
#define CH_TRSC_INT_EN					(0x1<<2)
#define CH_LLIST_INT_EN					(0x1<<3)
#define CH_CFG_ERR_INT_EN				(0x1<<4)
#define CH_AUTO_CLOSE_EN			    (0x1<<5)

#define CH_RAW_ISR_STATUS_MASK		    (0x1f << 8)


#define CH_ALLINTRC_EN_MASK				CH_FRAGMENT_INT_EN|\
                                        CH_BLK_INT_EN|\
                                        CH_TRSC_INT_EN|\
                                        CH_LLIST_INT_EN|\
                                        CH_CFG_ERR_INT_EN

enum mc_dma_req_mode{
    FRAGMENT,
    BLOCK,
    TRANSACTION,
    LINKLIST
};

/* Hardware register definitions. */
struct dw_mc_dma_chan_regs {
    FH_UINT32 pause;//0
    FH_UINT32 req;
    FH_UINT32 cfg;
    FH_UINT32 interrupt;
    FH_UINT32 src_addr;//10
    FH_UINT32 dst_addr;
    FH_UINT32 frag_len;
    FH_UINT32 blk_len;
    FH_UINT32 trsc_len;//20
    FH_UINT32 trsf_step;
    FH_UINT32 wrap_ptr;
    FH_UINT32 wrap_to;
    FH_UINT32 llist_ptr;//30
    FH_UINT32 frag_step;
    FH_UINT32 src_blk_step;
    FH_UINT32 dst_blk_step;
};

struct dw_mc_dma_regs {
    FH_UINT32 pause;//0
    FH_UINT32 frag_wait;
    FH_UINT32 pend_0_en;
    FH_UINT32 pend_1_en;
    FH_UINT32 int_raw_stat;//10
    FH_UINT32 int_mask_stat;
    FH_UINT32 req_stat;
    FH_UINT32 en_stat;
    FH_UINT32 debug_stat;//20
    FH_UINT32 arb_sel_stat;
    FH_UINT32 ch_cfg_grp_1;
    FH_UINT32 ch_cfg_grp_2;
    FH_UINT32 ch_arprot;//30
    FH_UINT32 ch_awprot;
    FH_UINT32 ch_prot_flag;
    FH_UINT32 global_prot;
    FH_UINT32 pend_0_port;//40
    FH_UINT32 pend_1_port;
    FH_UINT32 reqid_0_port;
    FH_UINT32 reqid_1_port;
    FH_UINT32 sync_sec_n_normal;//50
    FH_UINT32 cnt_ch_sel;
    FH_UINT32 total_trans_cnt_1;
    FH_UINT32 total_trans_cnt_2;
    FH_UINT32 rev_0[1000];
    struct dw_mc_dma_chan_regs ch[MC_DMA_MAX_NR_CHANNELS];
    FH_UINT32 rev_1[0x200];
    FH_UINT32 hand_cid[MC_DMA_MAX_HAND_SHAKE_NO];
};

struct mc_dma_lli {
    FH_UINT32 src_addr;
    FH_UINT32 dst_addr;
    FH_UINT32 frag_len;
    FH_UINT32 blk_len;
    FH_UINT32 trans_len;
    FH_UINT32 trsf_step;
    FH_UINT32 wrap_ptr;
    FH_UINT32 wrap_to;
    FH_UINT32 llist_ptr;
    FH_UINT32 frag_step;
    FH_UINT32 src_blk_step;
    FH_UINT32 dst_blk_step;
    FH_UINT32 reserved[4]; //ensure 64bytes aligned
};

struct dw_mc_dma {
    /* vadd */
    void *regs;
    FH_UINT32 channel_max_number;
};

#define CHANNEL_STATUS_IDLE   (0)
#define CHANNEL_STATUS_CYCLIC (1)
#define CHANNEL_STATUS_SINGLE (2)
struct dma_channel {
    FH_UINT32 channel_status;  /* open, busy ,closed */

    void* allloced_desc_mem;
    struct mc_dma_lli *base_lli;
    FH_UINT32          lli_count;
    void* callback_param;
    dma_complete_callback callback;
};

struct fh_mc_dma {
    struct dw_mc_dma dwc;
    struct dma_channel dma_channel[MC_DMA_MAX_NR_CHANNELS_APP];
};

static struct fh_mc_dma g_dma_controller[1];

#define __dma_raw_writel(v, a)  writel(v, a)
#define __dma_raw_readl(a)      readl(a)

#define dw_readl(dw, name) \
    __dma_raw_readl(&(((struct dw_mc_dma_regs *)dw->regs)->name))
#define dw_writel(dw, name, val) \
    __dma_raw_writel((val), &(((struct dw_mc_dma_regs *)dw->regs)->name))


#define MC_ALIGN(p, align) ((((unsigned int)(p)) + align - 1) & (~(align - 1)))


static void mc_dma_ch_isr_set(struct dw_mc_dma *mc_dma_obj, FH_UINT32 no, FH_UINT32 enable)
{
    FH_UINT32 ret;
    ret = dw_readl(mc_dma_obj, ch[no].interrupt);
    ret &= ~(CH_ALLINTRC_EN_MASK);
    ret |= enable;

    dw_writel(mc_dma_obj, ch[no].interrupt, ret);
}

static void mc_dma_ch_pause_set(struct dw_mc_dma *mc_dma_obj, FH_UINT32 no, FH_UINT32 val){
    int ret;

    ret = dw_readl(mc_dma_obj, ch[no].pause);
    ret &= ~(CHAN_PAUSE_BIT_MASK);
    ret |= val;
    dw_writel(mc_dma_obj, ch[no].pause, ret);
}

static void mc_dma_ch_disable(struct dw_mc_dma *mc_dma_obj, FH_UINT32 no){
    FH_UINT32 ret;

    ret = dw_readl(mc_dma_obj, ch[no].cfg);
    ret &= ~CH_ENABLE;
    dw_writel(mc_dma_obj, ch[no].cfg, ret);
}

static void mc_dma_ch_clear_isr_status(struct dw_mc_dma *mc_dma_obj, FH_UINT32 no){
    FH_UINT32 ret;

    ret = dw_readl(mc_dma_obj, ch[no].interrupt);
    ret |= (0x1f << 24);
    dw_writel(mc_dma_obj, ch[no].interrupt, ret);
}

static void mc_dma_enable(struct dw_mc_dma *mc_dma_obj)
{
    FH_UINT32 ret;

    ret = dw_readl(mc_dma_obj, pause);
    ret |= (GLB_REG_CLK_EN | CHNL_REG_CLK_EN | 
            REQ_CID_CLK_EN | INT_REG_CLK_EN | 
            AXI_MST_CLK_EN | AUDIO_CNT_CLK_EN);
    dw_writel(mc_dma_obj, pause, ret);
}

static void fh_mc_dma_isr(int irq, void* param)
{
    FH_UINT32 status;
    FH_UINT32 i;
    struct fh_mc_dma *dma = (struct fh_mc_dma *)param;
    struct dw_mc_dma *temp_dwc = &dma->dwc;
    dma_complete_callback callback;

    status = dw_readl(temp_dwc, int_mask_stat);
    for (i = 0; i < temp_dwc->channel_max_number; i++) 
    {
        if ((1 << (i + ARC_DMA_CH_OFFSET)) & status)
        {
            mc_dma_ch_clear_isr_status(temp_dwc, i + ARC_DMA_CH_OFFSET); /* clear isr status */
            callback = dma->dma_channel[i].callback;
            if (callback)
                callback(dma->dma_channel[i].callback_param);
        }
    }
}

static void handle_dma_open(struct fh_mc_dma *p_dma)
{
    int i;
    struct dw_mc_dma *temp_dwc;

    temp_dwc = &p_dma->dwc;

    for(i = ARC_DMA_CH_OFFSET; i < ARC_DMA_CH_OFFSET + MC_DMA_MAX_NR_CHANNELS_APP; i++){
        mc_dma_ch_isr_set(temp_dwc, i , 0);
        mc_dma_ch_pause_set(temp_dwc, i, CHAN_ACTIVE);
        mc_dma_ch_disable(temp_dwc, i);
        mc_dma_ch_clear_isr_status(temp_dwc, i);
    }
    mc_dma_enable(temp_dwc);
}

int fh_dma_init(unsigned int id)
{
    static int inited = 0;
    struct fh_mc_dma *dma;

    if (!inited)
    {   
        inited = 1;

        dma = &g_dma_controller[0];
        dma->dwc.regs = (void *)DMAC0_REG_BASE;
        dma->dwc.channel_max_number = MC_DMA_MAX_NR_CHANNELS_APP;

        handle_dma_open(dma);

        rt_hw_interrupt_install(DMAC0_IRQn, fh_mc_dma_isr, dma, "dma_isr");
        rt_hw_interrupt_umask(DMAC0_IRQn);
    }

    return 0;	
}

void mc_dma_desc_set_frag_len(struct mc_dma_lli *desc, FH_UINT32 len){
    desc->frag_len &= ~FRAG_LEN_MASK;
    desc->frag_len |= len;
}

void mc_dma_desc_set_ahb_size(struct mc_dma_lli *desc,FH_UINT32 src_size,FH_UINT32 dest_size){
    desc->frag_len &= ~(SRC_DST_AHB_SIZE_MASK<<DST_AHB_SIZE_SHIFT);
    desc->frag_len |= src_size<< SRC_AHB_SIZE_SHIFT|dest_size<<DST_AHB_SIZE_SHIFT;
}

void mc_dma_desc_set_trsf_step(struct mc_dma_lli *desc,FH_UINT32 src_step,FH_UINT32 dest_step){
    desc->trsf_step &= ~TRSF_STEP_MASK;
    desc->trsf_step |= dest_step<<DST_TRSF_STEP_SHIFT|src_step;
}

void mc_dma_desc_set_wrap_disable(struct mc_dma_lli *desc){
    desc->frag_len &= ~(1<<ADDR_WRAP_EN_SHIFT);
}

void mc_dma_desc_set_req_mode(struct mc_dma_lli *desc,FH_UINT32 val){

    desc->frag_len &= ~(REQ_MODE_MASK<<REQ_MODE_SHIFT);
    desc->frag_len |= val<<REQ_MODE_SHIFT;
}

void mc_dma_desc_set_swtich_mode(struct mc_dma_lli *desc,FH_UINT32 mode)
{
    desc->frag_len &= ~(SWT_MODE_MASK << SWT_MODE_SHIFT);
    mode &= SWT_MODE_MASK;
    desc->frag_len |= (mode << SWT_MODE_SHIFT);
}

void mc_dma_desc_bind_next_desc(struct mc_dma_lli *desc,FH_UINT32 val){
    desc->llist_ptr = val;
}

void mc_dma_desc_set_step(struct mc_dma_lli *desc,FH_UINT32 frag_step,FH_UINT32 src_blk_step,FH_UINT32 dest_blk_step){
    desc->frag_step = frag_step;
    desc->src_blk_step = src_blk_step;
    desc->dst_blk_step = dest_blk_step;
}

void mc_dma_ch_request(struct dw_mc_dma *mc_dma_obj, FH_UINT32 no, FH_UINT32 val)
{
    dw_writel(mc_dma_obj, ch[no].req, val);
}

void mc_dma_ch_llist_en(struct dw_mc_dma *mc_dma_obj, FH_UINT32 no){

    FH_UINT32 ret;
    ret = dw_readl(mc_dma_obj, ch[no].cfg);
    ret |= LLIST_EN;
    dw_writel(mc_dma_obj, ch[no].cfg, ret);
}

void mc_dma_ch_llist_addr_set(struct dw_mc_dma *mc_dma_obj, FH_UINT32 no, FH_UINT32 list_addr)
{
    dw_writel(mc_dma_obj, ch[no].llist_ptr, list_addr);
}

static void mc_dma_ch_enable(struct dw_mc_dma *mc_dma_obj, FH_UINT32 no)
{
    FH_UINT32 ret;

    ret = dw_readl(mc_dma_obj, ch[no].cfg);
    ret |= CH_ENABLE;
    dw_writel(mc_dma_obj, ch[no].cfg, ret);

}

static void mc_dma_ch_force_reset(struct fh_mc_dma *dma, FH_UINT32 no)
{
    int k;
    struct dw_mc_dma_chan_regs *p_ch;
    struct dw_mc_dma_regs *p_dma;

    p_dma = (struct dw_mc_dma_regs *)dma->dwc.regs;
    p_ch = &p_dma->ch[no];
    for (k = 0; k < sizeof(struct dw_mc_dma_chan_regs)/sizeof(FH_UINT32); k++)
    {
        __dma_raw_writel(0, ((FH_UINT32 *)p_ch + k));
    }
}

void mc_dma_desc_set_inc_mode(struct mc_dma_lli *desc, int fc_mode)
{	
    if(fc_mode == DMA_M2M)
    {
        //src inc & dst inc
        desc->frag_len &= ~(1 << ADDR_FIX_EN_SHIFT);
    }
    else if(fc_mode == DMA_M2P) //src inc & dst fix
    {		
        desc->frag_len |= (1 << ADDR_FIX_EN_SHIFT);
        desc->frag_len |= 1 << ADDR_FIX_SEL_SHIFT;
    }
    else //DMA_P2M
    {
        desc->frag_len |= (1 << ADDR_FIX_EN_SHIFT);
        desc->frag_len &= ~(1 << ADDR_FIX_SEL_SHIFT);
    }
}

/*
   static void dump_dma_common_reg(struct fh_mc_dma *p_dma)
   {

   struct dw_mc_dma *temp_dwc;

   temp_dwc = &p_dma->dwc;
   if (!temp_dwc->regs)
   return;

   rt_kprintf("pause: 0x%08x\n\
frag_wait: 0x%08x\n\
pend_0_en: 0x%08x\n\
pend_1_en: 0x%08x\n\
int_raw_stat: 0x%08x\n\
int_mask_stat :0x%08x\n\
req_stat: 0x%08x\n\
en_stat: 0x%08x\n\
debug_stat: 0x%08x\n\
arb_sel_stat: 0x%08x\n\
ch_cfg_grp_1: 0x%08x\n\
ch_cfg_grp_2: 0x%08x\n\
ch_arprot: 0x%08x\n\
ch_awprot: 0x%08x\n\
ch_prot_flag: 0x%08x\n\
global_prot: 0x%08x\n\
pend_0_port: 0x%08x\n\
pend_1_port: 0x%08x\n\
reqid_0_port: 0x%08x\n\
reqid_1_port: 0x%08x\n\
sync_sec_n_normal: 0x%08x\n\
cnt_ch_sel: 0x%08x\n\
total_trans_cnt_1: 0x%08x\n\
total_trans_cnt_2: 0x%08x\n",
dw_readl(temp_dwc, pause),
dw_readl(temp_dwc, frag_wait),
dw_readl(temp_dwc, pend_0_en),
dw_readl(temp_dwc, pend_1_en),
dw_readl(temp_dwc, int_raw_stat),
dw_readl(temp_dwc, int_mask_stat),
dw_readl(temp_dwc, req_stat),
dw_readl(temp_dwc, en_stat),
dw_readl(temp_dwc, debug_stat),
dw_readl(temp_dwc, arb_sel_stat),
dw_readl(temp_dwc, ch_cfg_grp_1),
dw_readl(temp_dwc, ch_cfg_grp_2),
dw_readl(temp_dwc, ch_arprot),
dw_readl(temp_dwc, ch_awprot),
dw_readl(temp_dwc, ch_prot_flag),
dw_readl(temp_dwc, global_prot),
dw_readl(temp_dwc, pend_0_port),
dw_readl(temp_dwc, pend_1_port),
dw_readl(temp_dwc, reqid_0_port),
dw_readl(temp_dwc, reqid_1_port),
dw_readl(temp_dwc, sync_sec_n_normal),
dw_readl(temp_dwc, cnt_ch_sel),
dw_readl(temp_dwc, total_trans_cnt_1),
dw_readl(temp_dwc, total_trans_cnt_2)
);

}

static void dump_channel_reg(struct fh_mc_dma *p_dma, FH_UINT32 chan_no)
{

struct dw_mc_dma *temp_dwc;	

temp_dwc = &p_dma->dwc;

rt_kprintf("[CHAN : %d]\n\
pause: 0x%08x\n\
req: 0x%08x\n\
    cfg: 0x%08x\n\
    interrupt: %08x\n\
    src_addr: %08x\n\
    dst_addr :0x%08x\n\
    frag_len: 0x%08x\n\
    blk_len: 0x%08x\n\
    trsc_len: 0x%08x\n\
    trsf_step: 0x%08x\n\
    wrap_ptr: 0x%08x\n\
    wrap_to: 0x%08x\n\
    llist_ptr: 0x%08x\n\
    frag_step: 0x%08x\n\
    src_blk_step: 0x%08x\n\
    dst_blk_step: 0x%08x\n",
    chan_no,
    dw_readl(temp_dwc, ch[chan_no].pause),
    dw_readl(temp_dwc, ch[chan_no].req),
    dw_readl(temp_dwc, ch[chan_no].cfg),
    dw_readl(temp_dwc, ch[chan_no].interrupt),
    dw_readl(temp_dwc, ch[chan_no].src_addr),
    dw_readl(temp_dwc, ch[chan_no].dst_addr),
    dw_readl(temp_dwc, ch[chan_no].frag_len),
    dw_readl(temp_dwc, ch[chan_no].blk_len),
    dw_readl(temp_dwc, ch[chan_no].trsc_len),
    dw_readl(temp_dwc, ch[chan_no].trsf_step),
    dw_readl(temp_dwc, ch[chan_no].wrap_ptr),
    dw_readl(temp_dwc, ch[chan_no].wrap_to),
    dw_readl(temp_dwc, ch[chan_no].llist_ptr),
    dw_readl(temp_dwc, ch[chan_no].frag_step),
    dw_readl(temp_dwc, ch[chan_no].src_blk_step),
    dw_readl(temp_dwc, ch[chan_no].dst_blk_step)
    );

    }


static void dump_lli(struct mc_dma_lli *p_lli)
{
    rt_kprintf("lli add is [%08x]\n",(int)p_lli);
    rt_kprintf("\nSAR: 0x%08x DAR: 0x%08x FRAG_LEN: 0x%08x BLK_LEN: 0x%08x\n\
            TRANS_LEN: 0x%08x TRANS_STEP: 0x%08x WRAP_PTR: 0x%08x WRAP_TO: 0x%08x\n\
            LIST_PTR: 0x%08x FRAG_STEP: 0x%08x  SRC_BLK_STEP: 0x%08x  DST_BLK_STEP: 0x%08x\n",
            (p_lli->src_addr),
            (p_lli->dst_addr),
            (p_lli->frag_len),
            (p_lli->blk_len),
            (p_lli->trans_len),
            (p_lli->trsf_step),
            (p_lli->wrap_ptr),
            (p_lli->wrap_to),
            (p_lli->llist_ptr),
            (p_lli->frag_step),
            (p_lli->src_blk_step),
            (p_lli->dst_blk_step));
}
*/

void dma_dump(void)
{
    /*
       FH_UINT32 i;
       struct mc_dma_lli *p_lli;

       struct fh_mc_dma *p_dma = &g_dma_controller[0];
       FH_UINT32  channel = 0;


       p_lli = p_dma->dma_channel[channel].base_lli;
       dump_dma_common_reg(p_dma);
       rt_thread_delay(2);

       dump_channel_reg(p_dma, channel);
       rt_thread_delay(2);
       for (i = 0; i < p_dma->dma_channel[channel].lli_count; i++) {
       dump_lli(&p_lli[i]);
       }
       */
}

int fh_dma_cyclic_start(unsigned int id,
        unsigned int channel,
        unsigned int fc_mode, /*DMA_M2P, DMA_P2M...*/
        unsigned int bitwidth,/*DW_DMA_SLAVE_WIDTH_16BIT, DW_DMA_SLAVE_WIDTH_32BIT...*/
        unsigned int msize,   /*DW_DMA_SLAVE_MSIZE_8,DW_DMA_SLAVE_MSIZE_16,DW_DMA_SLAVE_MSIZE_32*/
        unsigned int per,     /*ACODEC_RX, ACODEC_TX...*/
        unsigned int dst_add,
        unsigned int src_add,
        unsigned int period_len, /*in unit of bitwidth*/
        unsigned int periods,
        dma_complete_callback cb,
        void*        complete_para)
{
    int i;
    unsigned int lli_mem_sz;
    FH_UINT32 frag_len;
    struct mc_dma_lli *p_lli;
    struct fh_mc_dma *dma = &g_dma_controller[id];
    struct dw_mc_dma *temp_dwc = &dma->dwc;
    struct dma_channel *chan = &dma->dma_channel[channel];

    if (chan->channel_status != CHANNEL_STATUS_IDLE || periods > 127)
        return -1;

    channel += ARC_DMA_CH_OFFSET; /*from now on, channel is the physical DMA channel NO.*/

    /* alloc desc.... */
    lli_mem_sz = periods * sizeof(struct mc_dma_lli);
    chan->allloced_desc_mem = rt_malloc(MC_DESC_ALLIGN + lli_mem_sz);
    if (!chan->allloced_desc_mem)
        return -1;
    chan->base_lli = (struct mc_dma_lli *)MC_ALIGN(chan->allloced_desc_mem, MC_DESC_ALLIGN);
    rt_memset((void *)chan->base_lli, 0, lli_mem_sz);
    p_lli = chan->base_lli;

    chan->callback = cb;
    chan->callback_param = complete_para;

    mc_dma_ch_force_reset(dma, channel);
    mc_dma_ch_isr_set(temp_dwc, channel, CH_AUTO_CLOSE_EN);

    chan->lli_count = periods;

    for (i = 0; i < periods; i++, p_lli++)
    {
        p_lli->src_addr = src_add;
        p_lli->dst_addr = dst_add;
        frag_len = msize;
        if (msize != DW_DMA_SLAVE_MSIZE_1)
            frag_len++;
        frag_len = (1 << frag_len) * (1<<bitwidth);
        p_lli->trans_len = period_len << bitwidth;
        p_lli->blk_len = frag_len;
        mc_dma_desc_set_frag_len(p_lli, frag_len);
        if (fc_mode == DMA_M2M)
            mc_dma_desc_set_req_mode(p_lli, LINKLIST);
        else
            mc_dma_desc_set_req_mode(p_lli, FRAGMENT);

        mc_dma_desc_set_ahb_size(p_lli, bitwidth, bitwidth);
        mc_dma_desc_set_trsf_step(p_lli, 1 << bitwidth, 1 << bitwidth);
        mc_dma_desc_set_wrap_disable(p_lli);
        mc_dma_desc_set_inc_mode(p_lli, fc_mode);
        mc_dma_desc_set_swtich_mode(p_lli, 0/*SWT_ABCD_ABCD*/);
        mc_dma_desc_set_step(p_lli, 0, 0, 0);
        if ((i + 1) < periods)
        {
            mc_dma_desc_bind_next_desc(p_lli, (FH_UINT32)(p_lli+1));
        }
        else
        {
            /*cyclic make a ring..*/
            mc_dma_desc_bind_next_desc(p_lli, (FH_UINT32)chan->base_lli);
        }

        if (fc_mode == DMA_M2P)
        {
            src_add += period_len << bitwidth;
        }
        else //DMA_P2M
        {
            dst_add += period_len << bitwidth;
        }
    }

    mmu_clean_invalidated_dcache((FH_UINT32)chan->base_lli, lli_mem_sz);

    mc_dma_ch_isr_set(temp_dwc, channel, CH_TRSC_INT_EN); //for PERIODIC_TRANSFER

    /* only hw handshaking need this.. */
    switch (fc_mode) {
        case DMA_M2M:
            mc_dma_ch_request(temp_dwc, channel, 1);
            break;
        case DMA_M2P:
            mc_dma_ch_request(temp_dwc, channel, 0);
            dw_writel(temp_dwc, hand_cid[per], channel + 1);
            break;
        case DMA_P2M:
            mc_dma_ch_request(temp_dwc, channel, 0);
            dw_writel(temp_dwc, hand_cid[per], channel + 1);
            break;
        default:
            break;
    }

    mc_dma_ch_llist_addr_set(temp_dwc, channel, (FH_UINT32)chan->base_lli);
    mc_dma_ch_llist_en(temp_dwc, channel);

    chan->channel_status = CHANNEL_STATUS_CYCLIC; 
    mc_dma_ch_enable(temp_dwc, channel);

    return 0;
}

int fh_dma_cyclic_stop(unsigned int id, unsigned int channel)
{
    struct fh_mc_dma *dma = &g_dma_controller[id];
    struct dma_channel *chan = &dma->dma_channel[channel];

    if (chan->channel_status != CHANNEL_STATUS_CYCLIC)
        return -1;

    channel += ARC_DMA_CH_OFFSET; /*from now on, channel is the physical DMA channel NO.*/

    mc_dma_ch_disable(&dma->dwc, channel);

    if (chan->allloced_desc_mem)
    {
        rt_free(chan->allloced_desc_mem);
        chan->allloced_desc_mem = (void*)0;
    }

    chan->channel_status = CHANNEL_STATUS_IDLE;

    return 0;	
}

#endif //CONFIG_CHIP_xxx
