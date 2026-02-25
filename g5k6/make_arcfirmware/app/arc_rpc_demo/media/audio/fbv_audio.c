#include <rtthread.h>
#include "dsp/fh_audio_mpi.h"
#include "dsp/fh_audio_mpipara.h"

static unsigned char __attribute__((aligned(FH_PAGE_SIZE))) g_audio_buf[ACW_MEMORY_SIZE];

extern int fh_audio_module_int(void *mem, int cap_mem_size, int play_mem_size, int swap_mem_size);

void audio_init(void)
{
    int ret;
   
    //audio需要使用DMA,因此，先初始化DMA驱动

    /*加载audio模块驱动*/
    ret = fh_audio_module_int(g_audio_buf, /*audio内部使用的buffer，4096对齐*/
            ACW_CAP_MEMORY_SIZE, /*AI内存大小*/
            ACW_PLY_MEMORY_SIZE, /*AO内存大小*/
            ACW_SWAP_MEMORY_SIZE); /*swap内存大小,用于Linux端向ARC端发送AO音频数据*/
    if (ret != 0)
    {
        rt_kprintf("fh_audio_module_int: failed!!!\n");
    }
//Allen-D
/*
    FH_AC_CONFIG cfg;
    FH_AC_MIX_CONFIG mix_config;
 
    ret = FH_AC_Init();
    if (ret != 0)
    {
        rt_kprintf("FH_AC_Init: failed,ret=%d!\n", ret);
        return;
    }

    cfg.io_type     = FH_AC_MIC_IN;
    cfg.sample_rate = 16000;
    cfg.bit_width   = AC_BW_16;
    cfg.enc_type    = FH_PT_LPCM;
    cfg.period_size = 1024;
    cfg.channel_mask = 1;
    cfg.frame_num    = 0; //default 0,mean use max buffer.
    cfg.reserved = 0;
    ret = FH_AC_Set_Config(&cfg);
    if (ret != 0)
    {
        rt_kprintf("Audio: FH_AC_Set_Config failed ret=%x.\n", ret);
        return;
    }


    mix_config.mix_enable = 1;
    mix_config.mix_channel_mask = 1;
    mix_config.reserved = 0;
    ret = FH_AC_AI_MIX_Set_Config(&mix_config);
    if (ret != 0)
    {
        rt_kprintf("Audio: FH_AC_AI_MIX_Set_Config failed ret=%x.\n", ret);
        return;
    }

    FH_AC_AI_CH_SetAnologVol(7, 2, 28);
    ret = FH_AC_AI_Enable();
    if (ret != 0)
    {
        rt_kprintf("Audio: FH_AC_AI_Enable failed ret=%x.\n", ret);
        return;
    }
*/
    // if (1)
    // {
    //     int count = 5;
    //     FH_AC_AI_Frame_S frame_info;

    //     while (count-- > 0)
    //     {
    //         ret = FH_AC_AI_GetFrame(&frame_info);
    //         if (ret != 0)
    //         {
    //             rt_kprintf("Audio: FH_AC_AI_GetFrame failed, ret=%d!\n", ret);
    //             continue;
    //         }
    //         rt_kprintf("Audio: FH_AC_AI_GetFrame,len=%d\n", frame_info.ch_data_len);
    //     }
    // }
}
