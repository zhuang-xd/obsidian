#ifndef __FH_DMA_H__
#define __FH_DMA_H__

#define DMA_M2M (0)       /* MEM <=> MEM */
#define DMA_M2P (1)       /* MEM => peripheral A */
#define DMA_P2M (2)       /* MEM <= peripheral A */
#define DMA_P2P (3)       /* peripheral A <=> peripheral B */

#define DW_DMA_SLAVE_WIDTH_8BIT (0)
#define DW_DMA_SLAVE_WIDTH_16BIT (1)
#define DW_DMA_SLAVE_WIDTH_32BIT (2)

#define DW_DMA_SLAVE_MSIZE_1 (0)
#define DW_DMA_SLAVE_MSIZE_4 (1)
#define DW_DMA_SLAVE_MSIZE_8 (2)
#define DW_DMA_SLAVE_MSIZE_16 (3)
#define DW_DMA_SLAVE_MSIZE_32 (4)
#define DW_DMA_SLAVE_MSIZE_64 (5)
#define DW_DMA_SLAVE_MSIZE_128 (6)
#define DW_DMA_SLAVE_MSIZE_256 (7)

#define DW_DMA_SLAVE_INC (0)
#define DW_DMA_SLAVE_DEC (1)
#define DW_DMA_SLAVE_FIX (2)


#define FH_DMA_CHAN_ACW_CAP   (0)
#define FH_DMA_CHAN_ACW_PLAY  (1)
#define FH_DMA_CHAN_DMIC      (2)

typedef int (*dma_complete_callback)(void *complete_para);

int fh_dma_init(unsigned int id);

int fh_dma_cyclic_start(unsigned int id, /*dma ID, 0:DMA0; 1:DMA1*/
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
        void*        complete_para);

int fh_dma_cyclic_stop(unsigned int id, unsigned int channel);

#endif //__FH_DMA_H__
