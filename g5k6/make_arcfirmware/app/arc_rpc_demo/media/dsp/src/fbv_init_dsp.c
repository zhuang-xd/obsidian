#include "dsp/fh_venc_mpi.h"
#include "fbv_common.h"

// Allen-D
#define MAX_MJPEG_CHN 29 // MJPEG从29递减，30 31位JPEG通道
#define CH0_MJPEG_G0
static FH_SINT32 g_jpeg_chn = 0;
static FH_SINT32 g_mjpeg_chn = MAX_MJPEG_CHN;
#define JPEG_CHN 30

static VPU_INFO g_vpu_grp_infos[MAX_GRP_NUM] = {
    // grp 0
    {
        .enable = 0,
#ifdef VIDEO_GRP0
        .enable = 1,
        .grp_id = 0,
        .mode = VPU_MODE_ISP,
#endif
    },
    // grp 1
    {
        .enable = 0,
#ifdef VIDEO_GRP1
        .enable = 1,
        .grp_id = 1,
        .mode = VPU_MODE_ISP,
#endif
    },
    // grp 2
    {
        .enable = 0,
#ifdef VIDEO_GRP2
        .enable = 1,
        .grp_id = 2,
        .mode = VPU_MODE_ISP,
#endif
    },
};

VPU_CHN_INFO g_vpu_chn_info[MAX_GRP_NUM][MAX_VPU_CHN_NUM] = {
    // grp 0
    {
        {
            .enable = 0,
#ifdef CH0_ENABLE_G0
            .enable = 1,
            .channel = 0,
            .width = VPU_WIDTH_CH0_G0,
            .height = VPU_HEIGHT_CH0_G0,
            .yuv_type = VPU_VOMODE_SCAN,
            .bufnum = VPU_BUF_NUM_CH0_G0,
#endif
        },
        {
            .enable = 0,
#ifdef CH1_ENABLE_G0
            .enable = 1,
            .channel = 1,
            .width = VPU_WIDTH_CH1_G0,
            .height = VPU_HEIGHT_CH1_G0,
            .yuv_type = VPU_VOMODE_SCAN,
            .bufnum = VPU_BUF_NUM_CH1_G0,
#endif
        },
        {
            .enable = 0,
#ifdef CH2_ENABLE_G0
            .enable = 1,
            .channel = 2,
            .width = VPU_WIDTH_CH2_G0,
            .height = VPU_HEIGHT_CH2_G0,
            .yuv_type = VPU_VOMODE_RGB888,
            .bufnum = VPU_BUF_NUM_CH2_G0,
#endif
        },
    },
    // grp 1
    {
        {
            .enable = 0,
#ifdef CH0_ENABLE_G1
            .enable = 1,
            .channel = 0,
            .width = VPU_WIDTH_CH0_G1,
            .height = VPU_HEIGHT_CH0_G1,
            .yuv_type = VPU_VOMODE_SCAN,
            .bufnum = VPU_BUF_NUM_CH0_G1,
#endif
        },
        {
            .enable = 0,
#ifdef CH1_ENABLE_G1
            .enable = 1,
            .channel = 1,
            .width = VPU_WIDTH_CH1_G1,
            .height = VPU_HEIGHT_CH1_G1,
            .yuv_type = VPU_VOMODE_SCAN,
            .bufnum = VPU_BUF_NUM_CH1_G1,
#endif
        },
        {
            .enable = 0,
#ifdef CH2_ENABLE_G1
            .enable = 1,
            .channel = 2,
            .width = VPU_WIDTH_CH2_G1,
            .height = VPU_HEIGHT_CH2_G1,
            .yuv_type = VPU_VOMODE_RGB888,
            .bufnum = VPU_BUF_NUM_CH2_G1,
#endif
        },
    },
    // grp 2
    {
        {
            .enable = 0,
#ifdef CH0_ENABLE_G2
            .enable = 1,
            .channel = 0,
            .width = VPU_WIDTH_CH0_G2,
            .height = VPU_HEIGHT_CH0_G2,
            .yuv_type = VPU_VOMODE_SCAN,
            .bufnum = VPU_BUF_NUM_CH0_G2,
#endif
        },
        {
            .enable = 0,
#ifdef CH1_ENABLE_G2
            .enable = 1,
            .channel = 1,
            .width = VPU_WIDTH_CH1_G2,
            .height = VPU_HEIGHT_CH1_G2,
            .yuv_type = VPU_VOMODE_SCAN,
            .bufnum = VPU_BUF_NUM_CH1_G2,
#endif
        },
        {
            .enable = 0,
#ifdef CH2_ENABLE_G2
            .enable = 1,
            .channel = 2,
            .width = VPU_WIDTH_CH2_G2,
            .height = VPU_HEIGHT_CH2_G2,
            .yuv_type = VPU_VOMODE_RGB888,
            .bufnum = VPU_BUF_NUM_CH2_G2,
#endif
        },
    },
};

static ENC_INFO g_enc_chn_infos[MAX_GRP_NUM][MAX_VPU_CHN_NUM] = {
    // grp 0
    {
        {
            .enable = 0,
#ifdef VENC_ENABLE_G0_CH0
            .enable = 1,
            .channel = 0,
            .width = VPU_WIDTH_CH0_G0,
            .height = VPU_HEIGHT_CH0_G0,
            .frame_count = CH0_FRAME_COUNT_G0,
            .frame_time = CH0_FRAME_TIME_G0,
            .bps = CH0_BIT_RATE_G0 * 1024,
            .enc_type = CH0_ENC_TYPE_G0,
            .rc_type = CH0_RC_TYPE_G0,
#endif
        },
        {
            .enable = 0,
#ifdef VENC_ENABLE_G0_CH1
            .enable = 1,
            .channel = 1,
            .width = VPU_WIDTH_CH1_G0,
            .height = VPU_HEIGHT_CH1_G0,
            .frame_count = CH1_FRAME_COUNT_G0,
            .frame_time = CH1_FRAME_TIME_G0,
            .bps = CH1_BIT_RATE_G0 * 1024,
            .enc_type = CH1_ENC_TYPE_G0,
            .rc_type = CH1_RC_TYPE_G0,
#endif
        },

        {
            .enable = 0,
#ifdef VENC_ENABLE_G0_CH2
            .enable = 1,
            .channel = 2,
            .width = VPU_WIDTH_CH2_G0,
            .height = VPU_HEIGHT_CH2_G0,
            .frame_count = CH2_FRAME_COUNT_G0,
            .frame_time = CH2_FRAME_TIME_G0,
            .bps = CH2_BIT_RATE_G0 * 1024,
            .enc_type = CH2_ENC_TYPE_G0,
            .rc_type = CH2_RC_TYPE_G0,
#endif
        },
    },
    // grp 1
    {
        {
            .enable = 0,
#ifdef VENC_ENABLE_G1_CH0
            .enable = 1,
            .channel = 0,
            .width = VPU_WIDTH_CH0_G1,
            .height = VPU_HEIGHT_CH0_G1,
            .frame_count = CH0_FRAME_COUNT_G1,
            .frame_time = CH0_FRAME_TIME_G1,
            .bps = CH0_BIT_RATE_G1 * 1024,
            .enc_type = CH0_ENC_TYPE_G1,
            .rc_type = CH0_RC_TYPE_G1,
#endif
        },
        {
            .enable = 0,
#ifdef VENC_ENABLE_G1_CH1
            .enable = 1,
            .channel = 1,
            .width = VPU_WIDTH_CH1_G1,
            .height = VPU_HEIGHT_CH1_G1,
            .frame_count = CH1_FRAME_COUNT_G1,
            .frame_time = CH1_FRAME_TIME_G1,
            .bps = CH1_BIT_RATE_G1 * 1024,
            .enc_type = CH1_ENC_TYPE_G1,
            .rc_type = CH1_RC_TYPE_G1,
#endif
        },

        {
            .enable = 0,
#ifdef VENC_ENABLE_G1_CH2
            .enable = 1,
            .channel = 2,
            .width = VPU_WIDTH_CH2_G1,
            .height = VPU_HEIGHT_CH2_G1,
            .frame_count = CH2_FRAME_COUNT_G1,
            .frame_time = CH2_FRAME_TIME_G1,
            .bps = CH2_BIT_RATE_G1 * 1024,
            .enc_type = CH2_ENC_TYPE_G1,
            .rc_type = CH2_RC_TYPE_G1,
#endif
        },
    },
    // grp 2
    {
        {
            .enable = 0,
#ifdef VENC_ENABLE_G2_CH0
            .enable = 1,
            .channel = 0,
            .width = VPU_WIDTH_CH0_G2,
            .height = VPU_HEIGHT_CH0_G2,
            .frame_count = CH0_FRAME_COUNT_G2,
            .frame_time = CH0_FRAME_TIME_G2,
            .bps = CH0_BIT_RATE_G2 * 1024,
            .enc_type = CH0_ENC_TYPE_G2,
            .rc_type = CH0_RC_TYPE_G2,
#endif
        },
        {
            .enable = 0,
#ifdef VENC_ENABLE_G2_CH1
            .enable = 1,
            .channel = 1,
            .width = VPU_WIDTH_CH1_G2,
            .height = VPU_HEIGHT_CH1_G2,
            .frame_count = CH1_FRAME_COUNT_G2,
            .frame_time = CH1_FRAME_TIME_G2,
            .bps = CH1_BIT_RATE_G2 * 1024,
            .enc_type = CH1_ENC_TYPE_G2,
            .rc_type = CH1_RC_TYPE_G2,
#endif
        },

        {
            .enable = 0,
#ifdef VENC_ENABLE_G2_CH2
            .enable = 1,
            .channel = 2,
            .width = VPU_WIDTH_CH2_G2,
            .height = VPU_HEIGHT_CH2_G2,
            .frame_count = CH2_FRAME_COUNT_G2,
            .frame_time = CH2_FRAME_TIME_G2,
            .bps = CH2_BIT_RATE_G2 * 1024,
            .enc_type = CH2_ENC_TYPE_G2,
            .rc_type = CH2_RC_TYPE_G2,
#endif
        },
    },
};

// Allen-D
ENC_INFO g_jpeg_chn_infos = {
    .enable = 1,
    .channel = 0,
    .width = 736,
    .height = 576,
    .frame_count = 25,
    .frame_time = 1,
    .bps = 0 * 1024, // not support
    .enc_type = FH_MJPEG,
    .rc_type = FH_RC_MJPEG_FIXQP,
};

extern FH_SINT32 vpu_write_proc(FH_CHAR* s);
static FH_SINT32 fbv_dsp_system_init(FH_VOID) {
    FH_SINT32 ret = 0;
    FH_UINT32 blk_size = 0;
    FH_SINT32 grp_id = 0;
    FH_SINT32 chn_id = 0;
    FH_SINT32 poolid = 0;
    VB_CONF_S stVbConf;
    rt_memset(&stVbConf, 0, sizeof(VB_CONF_S));

    // proc init

    // Allen-D
    // vpu_write_proc("cirbuf_on");
    vpu_write_proc("procmode_1");
    ret = FH_SYS_Init();
    FBV_FUNC_ERROR_PRT("FH_SYS_Init", ret);

    ret = API_ISP_Open();
    FBV_FUNC_ERROR_PRT("API_ISP_Open", ret);

    // vb init
    stVbConf.u32MaxPoolCnt =
        MAX_GRP_NUM * MAX_VPU_CHN_NUM + MAX_GRP_NUM * 2; // MAX_GRP_NUM * 2为3个VICAP+3个ISP的最大值
    for (grp_id = 0; grp_id < MAX_GRP_NUM; grp_id++) {
        for (chn_id = 0; chn_id < MAX_VPU_CHN_NUM; chn_id++) {
            if (g_vpu_chn_info[grp_id][chn_id].enable && !g_vpu_chn_info[grp_id][chn_id].bufnum) {
                ret =
                    FH_VPSS_GetFrameBlkSize(g_vpu_chn_info[grp_id][chn_id].width, g_vpu_chn_info[grp_id][chn_id].height,
                                            1 << g_vpu_chn_info[grp_id][chn_id].yuv_type, &blk_size);
                FBV_FUNC_ERROR_PRT("FH_VPSS_GetFrameBlkSize", ret);
                if (blk_size) {
                    stVbConf.astCommPool[poolid].u32BlkSize = blk_size;
                    stVbConf.astCommPool[poolid].u32BlkCnt = 3;
                    poolid++;
                    printf("poolid = %d\n");
                }
            }
        }
    }

    ret = FH_VB_SetConf(&stVbConf);
    FBV_FUNC_ERROR_PRT("FH_VB_SetConf", ret);

    ret = FH_VB_Init();
    FBV_FUNC_ERROR_PRT("FH_VB_Init", ret);

    return ret;
}

static FH_SINT32 vpu_init(FH_SINT32 grp_id) {
    FH_SINT32 ret = 0;
    FH_VPU_SIZE vi_pic;
    rt_memset(&vi_pic, 0, sizeof(FH_VPU_SIZE));

    // g_vpu_grp_infos[grp_id].grp_info.vi_max_size.u32Width = fbv_isp_w(grp_id);
    // g_vpu_grp_infos[grp_id].grp_info.vi_max_size.u32Height = fbv_isp_h(grp_id);
    g_vpu_grp_infos[grp_id].grp_info.vi_max_size.u32Width = 2560;
    g_vpu_grp_infos[grp_id].grp_info.vi_max_size.u32Height = 1440;
    g_vpu_grp_infos[grp_id].grp_info.ycmean_en = 1;
    g_vpu_grp_infos[grp_id].grp_info.ycmean_ds = 16;

    rt_kprintf("[DEBUG] VPU create group, max width = %d, height = %d\n",
               g_vpu_grp_infos[grp_id].grp_info.vi_max_size.u32Width,
               g_vpu_grp_infos[grp_id].grp_info.vi_max_size.u32Height);

    ret = FH_VPSS_CreateGrp(grp_id, &g_vpu_grp_infos[grp_id].grp_info);
    FBV_FUNC_ERROR_PRT("FH_VPSS_GetFrameBlkSize", ret);

#ifdef ENBALE_SMALL_TO_BIG
    vi_pic.vi_size.u32Width = fbv_isp_w_pre(grp_id);
    vi_pic.vi_size.u32Height = fbv_isp_h_pre(grp_id);
    vi_pic.crop_area.crop_en = 0;
    vi_pic.crop_area.vpu_crop_area.u32X = 0;
    vi_pic.crop_area.vpu_crop_area.u32Y = 0;
    vi_pic.crop_area.vpu_crop_area.u32Width = 0;
    vi_pic.crop_area.vpu_crop_area.u32Height = 0;
#else
    vi_pic.vi_size.u32Width = fbv_isp_w(grp_id);
    vi_pic.vi_size.u32Height = fbv_isp_h(grp_id);
    vi_pic.crop_area.crop_en = 0;
    vi_pic.crop_area.vpu_crop_area.u32X = 0;
    vi_pic.crop_area.vpu_crop_area.u32Y = 0;
    vi_pic.crop_area.vpu_crop_area.u32Width = 0;
    vi_pic.crop_area.vpu_crop_area.u32Height = 0;
#endif

    ret = FH_VPSS_SetViAttr(grp_id, &vi_pic);
    FBV_FUNC_ERROR_PRT("FH_VPSS_SetViAttr", ret);

    ret = FH_VPSS_Enable(grp_id, g_vpu_grp_infos[grp_id].mode);
    FBV_FUNC_ERROR_PRT("FH_VPSS_Enable", ret);
    return 0;
}

static FH_SINT32 fbv_dsp_vpu_init(FH_VOID) {
    FH_SINT32 ret = 0;
    FH_SINT32 grp_id = 0;
    for (grp_id = 0; grp_id < MAX_GRP_NUM; grp_id++) {
        if (g_vpu_grp_infos[grp_id].enable) {
            ret = vpu_init(grp_id);
            FBV_FUNC_ERROR_PRT("vpu_init", ret);
        }
    }
    return 0;
}

FH_SINT32 change_size(FH_SINT32 grp_id) {
    FH_SINT32 ret = 0;
    FH_SINT32 chn_id = 0;
    FH_VPU_CHN_CONFIG chn_attr;
    FH_VPU_SIZE vi_size;

    fbv_isp_pause(grp_id);

    if (isp_change_resolution(grp_id)) {
        return -1;
    }

    for (chn_id = 0; chn_id < MAX_VPU_CHN_NUM; chn_id++) {
        if (g_vpu_chn_info[grp_id][chn_id].enable) {
            vi_size.vi_size.u32Width = fbv_isp_w(grp_id);
            vi_size.vi_size.u32Height = fbv_isp_h(grp_id);
            vi_size.crop_area.crop_en = 0;
            vi_size.crop_area.vpu_crop_area.u32X = 0;
            vi_size.crop_area.vpu_crop_area.u32Y = 0;
            vi_size.crop_area.vpu_crop_area.u32Width = 0;
            vi_size.crop_area.vpu_crop_area.u32Height = 0;

            ret = FH_VPSS_SetViAttr(grp_id, &vi_size);
            FBV_FUNC_ERROR_PRT("FH_VPSS_SetViAttr", ret);

            chn_attr.vpu_chn_size.u32Width = g_vpu_chn_info[grp_id][chn_id].width;
            chn_attr.vpu_chn_size.u32Height = g_vpu_chn_info[grp_id][chn_id].height;
            chn_attr.crop_area.crop_en = 0;
            chn_attr.crop_area.vpu_crop_area.u32X = 0;
            chn_attr.crop_area.vpu_crop_area.u32Y = 0;
            chn_attr.crop_area.vpu_crop_area.u32Width = 0;
            chn_attr.crop_area.vpu_crop_area.u32Height = 0;
            chn_attr.stride = 0;
            chn_attr.offset = 0;
            chn_attr.depth = 1;
            ret = FH_VPSS_SetChnAttr(grp_id, chn_id, &chn_attr);
            FBV_FUNC_ERROR_PRT("FH_VPSS_SetChnAttr", ret);

            ret = FH_VPSS_SetVOMode(grp_id, chn_id, g_vpu_chn_info[grp_id][chn_id].yuv_type);
            FBV_FUNC_ERROR_PRT("FH_VPSS_SetVOMode", ret);

            ret = FH_VPSS_OpenChn(grp_id, chn_id);
            FBV_FUNC_ERROR_PRT("FH_VPSS_OpenChn", ret);

            ret = FH_VPSS_Enable(grp_id, g_vpu_grp_infos[grp_id].mode);
            FBV_FUNC_ERROR_PRT("FH_VPSS_Enable", ret);
        }
    }

    fbv_isp_resume(grp_id);

    return 0;
}

static FH_SINT32 vpu_chn_init(FH_SINT32 grp_id, FH_SINT32 chn_id) {
    FH_SINT32 ret = 0;
    FH_VPU_CHN_INFO chn_info;
    FH_VPU_CHN_CONFIG chn_attr;

    chn_info.chn_max_size.u32Width = g_vpu_chn_info[grp_id][chn_id].width;
    chn_info.chn_max_size.u32Height = g_vpu_chn_info[grp_id][chn_id].height;
    if (chn_id == 0) {
        chn_info.chn_max_size.u32Width = 2560;
        chn_info.chn_max_size.u32Height = 1440;
    }

    chn_info.out_mode = g_vpu_chn_info[grp_id][chn_id].yuv_type;
    chn_info.support_mode = 1 << chn_info.out_mode;
    chn_info.bufnum = g_vpu_chn_info[grp_id][chn_id].bufnum;
    chn_info.max_stride = 0;

    rt_kprintf("[DEBUG] VPU create channel, max width = %d, height = %d\n", chn_info.chn_max_size.u32Width,
               chn_info.chn_max_size.u32Height);
    ret = FH_VPSS_CreateChn(grp_id, chn_id, &chn_info);
    FBV_FUNC_ERROR_PRT("FH_VPSS_CreateChn", ret);

    if (chn_id == 2) {
        chn_attr.vpu_chn_size.u32Width = g_vpu_chn_info[grp_id][chn_id].width;
        chn_attr.vpu_chn_size.u32Height = g_vpu_chn_info[grp_id][chn_id].height;
        chn_attr.crop_area.crop_en = 0; // TODO no chn crop
        chn_attr.crop_area.vpu_crop_area.u32X = 0;
        chn_attr.crop_area.vpu_crop_area.u32Y = 0;
        chn_attr.crop_area.vpu_crop_area.u32Width = 0;
        chn_attr.crop_area.vpu_crop_area.u32Height = 0;
        chn_attr.stride = 0;
        chn_attr.offset = 0;
        chn_attr.depth = 1;
    } else {
#ifdef ENBALE_SMALL_TO_BIG
        chn_attr.vpu_chn_size.u32Width = fbv_isp_w_pre(grp_id);
        chn_attr.vpu_chn_size.u32Height = fbv_isp_h_pre(grp_id);
        chn_attr.crop_area.crop_en = 0; // TODO no chn crop
        chn_attr.crop_area.vpu_crop_area.u32X = 0;
        chn_attr.crop_area.vpu_crop_area.u32Y = 0;
        chn_attr.crop_area.vpu_crop_area.u32Width = 0;
        chn_attr.crop_area.vpu_crop_area.u32Height = 0;
        chn_attr.stride = 0;
        chn_attr.offset = 0;
        chn_attr.depth = 0;
#else
        chn_attr.vpu_chn_size.u32Width = g_vpu_chn_info[grp_id][chn_id].width;
        chn_attr.vpu_chn_size.u32Height = g_vpu_chn_info[grp_id][chn_id].height;
        chn_attr.crop_area.crop_en = 0; // TODO no chn crop
        chn_attr.crop_area.vpu_crop_area.u32X = 0;
        chn_attr.crop_area.vpu_crop_area.u32Y = 0;
        chn_attr.crop_area.vpu_crop_area.u32Width = 0;
        chn_attr.crop_area.vpu_crop_area.u32Height = 0;
        chn_attr.stride = 0;
        chn_attr.offset = 0;
        chn_attr.depth = 0;
#endif
    }

    ret = FH_VPSS_SetChnAttr(grp_id, chn_id, &chn_attr);
    FBV_FUNC_ERROR_PRT("FH_VPSS_SetChnAttr", ret);

    ret = FH_VPSS_SetVOMode(grp_id, chn_id, g_vpu_chn_info[grp_id][chn_id].yuv_type);
    FBV_FUNC_ERROR_PRT("FH_VPSS_SetVOMode", ret);

    ret = FH_VPSS_OpenChn(grp_id, chn_id);
    FBV_FUNC_ERROR_PRT("FH_VPSS_OpenChn", ret);

    return ret;
}

static FH_SINT32 fbv_dsp_vpu_chn_init(FH_VOID) {
    FH_SINT32 ret = 0;
    FH_SINT32 grp_id = 0;
    FH_SINT32 chn_id = 0;

    for (grp_id = 0; grp_id < MAX_GRP_NUM; grp_id++) {
        if (g_vpu_grp_infos[grp_id].enable) {
            for (chn_id = 0; chn_id < MAX_VPU_CHN_NUM; chn_id++) {
                if (g_vpu_chn_info[grp_id][chn_id].enable) {
                    ret = vpu_chn_init(grp_id, chn_id);
                    FBV_FUNC_ERROR_PRT("vpu_chn_init", ret);
                }
            }
        }
    }

    return ret;
}

int enc_mod_set(void) {
    FH_SINT32 ret = 0;

    FH_VENC_MOD_PARAM jpeg_param;
    // FH_VENC_MOD_PARAM mjpeg_param;

    jpeg_param.stm_size = JPEG_COMMON_BUFFER_SIZE;
    // mjpeg_param.stm_size = MJPEG_COMMON_BUFFER_SIZE;

#ifdef FH_CH0_USING_SAMPLE_H264_G0
    FH_VENC_MOD_PARAM h264_param;

    h264_param.stm_size = H264_COMMON_BUFFER_SIZE;
    h264_param.type = FH_STREAM_H264;
    h264_param.max_stm_num = 128;
    h264_param.stm_cache = 0;

    ret = FH_VENC_SetModParam(&h264_param);
    FBV_FUNC_ERROR_PRT("FH_VENC_SetModParam", ret);
#endif
    // SDK_FUNC_ERROR(model_name, ret);

    jpeg_param.type = FH_STREAM_JPEG;
    jpeg_param.max_stm_num = 128;
    jpeg_param.stm_cache = 0;

    ret = FH_VENC_SetModParam(&jpeg_param);
    printf("enc set mod for jpeg: buffer size = %d\n", jpeg_param.stm_size);
    FBV_FUNC_ERROR_PRT("FH_VENC_SetModParam", ret);
    // SDK_FUNC_ERROR(model_name, ret);
#ifdef FH_CH0_USING_SAMPLE_H265_G0
    FH_VENC_MOD_PARAM h265_param;

    h265_param.stm_size = H265_COMMON_BUFFER_SIZE;

    h265_param.type = FH_STREAM_H265;
    h265_param.max_stm_num = 128;
    h265_param.stm_cache = 0;

    ret = FH_VENC_SetModParam(&h265_param);
    FBV_FUNC_ERROR_PRT("FH_VENC_SetModParam", ret);

#endif

    // mjpeg_param.type = FH_STREAM_MJPEG;
    // mjpeg_param.max_stm_num = 128;
    // mjpeg_param.stm_cache = 0;

    // ret = FH_VENC_SetModParam(&mjpeg_param);
    // printf("enc set mod for mjpeg: buffer size = %d\n", mjpeg_param.stm_size);
    // FBV_FUNC_ERROR_PRT("FH_VENC_SetModParam", ret);
    //  SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

static int enc_set_chn_buffer(int chn_id) {
    FH_SINT32 ret = 0;
    FH_VENC_CHN_PARAM pstchnparam;

    ret = FH_VENC_GetChnParam(chn_id, &pstchnparam);
    FBV_FUNC_ERROR_PRT("FH_VENC_GetChnParam", ret);

    pstchnparam.stm_mode = FH_STM_PUB_BUF;
    // pstchnparam.stm_mode = FH_STM_CHN_BUF;
#ifdef FH_CH0_USING_SAMPLE_H264_G0
    pstchnparam.stm_size = H264_COMMON_BUFFER_SIZE;
#endif

#ifdef FH_CH0_USING_SAMPLE_H265_G0
    pstchnparam.stm_size = H265_COMMON_BUFFER_SIZE;
#endif

    ret = FH_VENC_SetChnParam(chn_id, &pstchnparam);
    FBV_FUNC_ERROR_PRT("FH_VENC_GetChnParam", ret);

    return ret;
}

static int dsp_enc_init(FH_SINT32 grpid, FH_SINT32 chn_id) {
    FH_SINT32 ret;
    ret = enc_set_chn_buffer(grpid * MAX_VPU_CHN_NUM + chn_id);
    FBV_FUNC_ERROR_PRT("enc_set_chn_buffer", ret);
    FH_VENC_CHN_CAP cfg_vencmem;
    FH_VENC_CHN_CONFIG cfg_param;

    rt_memset(&cfg_vencmem, 0, sizeof(FH_VENC_CHN_CAP));
    rt_memset(&cfg_param, 0, sizeof(FH_VENC_CHN_CONFIG));

    cfg_vencmem.support_type = g_enc_chn_infos[grpid][chn_id].enc_type;
    // Allen-D
    // cfg_vencmem.max_size.u32Width = g_enc_chn_infos[grpid][chn_id].width;
    // cfg_vencmem.max_size.u32Height = g_enc_chn_infos[grpid][chn_id].height;
    cfg_vencmem.max_size.u32Width = 2560;
    cfg_vencmem.max_size.u32Height = 1440;

    //主通道用在线模式，子通道用离线模式
    // Allen-D change work mode for jpeg
    // if (chn_id == 0)
    //{

    //    cfg_vencmem.work_mode = FH_HARD_MODE;
    //}
    // else
    //{
    //    cfg_vencmem.work_mode = FH_NORMAL_MODE;
    //}
    cfg_vencmem.work_mode = FH_NORMAL_MODE;

    ret = FH_VENC_CreateChn(grpid * MAX_VPU_CHN_NUM + chn_id, &cfg_vencmem);
    FBV_FUNC_ERROR_PRT("FH_VENC_CreateChn", ret);

    cfg_param.chn_attr.enc_type = g_enc_chn_infos[grpid][chn_id].enc_type;

    if (g_enc_chn_infos[grpid][chn_id].enc_type == FH_NORMAL_H264) {
        cfg_param.chn_attr.h264_attr.profile = H264_PROFILE_MAIN;
        cfg_param.chn_attr.h264_attr.i_frame_intterval = 50;
        cfg_param.chn_attr.h264_attr.size.u32Width = g_enc_chn_infos[grpid][chn_id].width;
        cfg_param.chn_attr.h264_attr.size.u32Height = g_enc_chn_infos[grpid][chn_id].height;

        cfg_param.rc_attr.rc_type = FH_RC_H264_VBR;
        cfg_param.rc_attr.h264_vbr.init_qp = 35;
        cfg_param.rc_attr.h264_vbr.bitrate = g_enc_chn_infos[grpid][chn_id].bps;
        cfg_param.rc_attr.h264_vbr.ImaxQP = 42;
        cfg_param.rc_attr.h264_vbr.IminQP = 28;
        cfg_param.rc_attr.h264_vbr.PmaxQP = 42;
        cfg_param.rc_attr.h264_vbr.PminQP = 28;
        cfg_param.rc_attr.h264_vbr.maxrate_percent = 200;
        cfg_param.rc_attr.h264_vbr.IFrmMaxBits = 0;
        cfg_param.rc_attr.h264_vbr.IP_QPDelta = 3;
        cfg_param.rc_attr.h264_vbr.I_BitProp = 5;
        cfg_param.rc_attr.h264_vbr.P_BitProp = 1;
        cfg_param.rc_attr.h264_vbr.fluctuate_level = 0;
        cfg_param.rc_attr.h264_vbr.FrameRate.frame_count = g_enc_chn_infos[grpid][chn_id].frame_count;
        cfg_param.rc_attr.h264_vbr.FrameRate.frame_time = g_enc_chn_infos[grpid][chn_id].frame_time;
    } else if (g_enc_chn_infos[grpid][chn_id].enc_type == FH_NORMAL_H265) {
        cfg_param.chn_attr.h265_attr.profile = H265_PROFILE_MAIN;
        cfg_param.chn_attr.h265_attr.i_frame_intterval = 50;
        cfg_param.chn_attr.h265_attr.size.u32Width = g_enc_chn_infos[grpid][chn_id].width;
        cfg_param.chn_attr.h265_attr.size.u32Height = g_enc_chn_infos[grpid][chn_id].height;

        cfg_param.rc_attr.rc_type = FH_RC_H265_VBR;
        cfg_param.rc_attr.h265_vbr.init_qp = 35;
        cfg_param.rc_attr.h265_vbr.bitrate = g_enc_chn_infos[grpid][chn_id].bps;
        cfg_param.rc_attr.h265_vbr.ImaxQP = 42;
        cfg_param.rc_attr.h265_vbr.IminQP = 28;
        cfg_param.rc_attr.h265_vbr.PmaxQP = 42;
        cfg_param.rc_attr.h265_vbr.PminQP = 28;
        cfg_param.rc_attr.h265_vbr.maxrate_percent = 200;
        cfg_param.rc_attr.h265_vbr.IFrmMaxBits = 0;
        cfg_param.rc_attr.h265_vbr.IP_QPDelta = 3;
        cfg_param.rc_attr.h265_vbr.I_BitProp = 5;
        cfg_param.rc_attr.h265_vbr.P_BitProp = 1;
        cfg_param.rc_attr.h265_vbr.fluctuate_level = 0;
        cfg_param.rc_attr.h265_vbr.FrameRate.frame_count = g_enc_chn_infos[grpid][chn_id].frame_count;
        cfg_param.rc_attr.h265_vbr.FrameRate.frame_time = g_enc_chn_infos[grpid][chn_id].frame_time;
    }
    ret = FH_VENC_SetChnAttr(grpid * MAX_VPU_CHN_NUM + chn_id, &cfg_param);
    FBV_FUNC_ERROR_PRT("FH_VENC_SetChnAttr", ret);

    FH_BIND_INFO src, dst;
    src.obj_id = FH_OBJ_VPU_VO;
    src.dev_id = grpid;
    src.chn_id = chn_id;

    dst.obj_id = FH_OBJ_ENC;
    dst.dev_id = 0;
    dst.chn_id = grpid * MAX_VPU_CHN_NUM + chn_id;

    ret = FH_SYS_UnBindbyDst(dst);
    FBV_FUNC_ERROR_PRT("FH_SYS_UnBindbyDst", ret);

    ret = FH_SYS_Bind(src, dst);
    FBV_FUNC_ERROR_PRT("FH_SYS_Bind", ret);

    ret = FH_VENC_StartRecvPic(grpid * MAX_VPU_CHN_NUM + chn_id);
    FBV_FUNC_ERROR_PRT("FH_VENC_StartRecvPic", ret);

    return ret;
}

FH_SINT32 fbv_dsp_enc_init(FH_VOID) {
    FH_SINT32 ret = 0;
    FH_SINT32 grp_id = 0;
    FH_SINT32 chn_id = 0;
    enc_mod_set();
    for (grp_id = 0; grp_id < MAX_GRP_NUM; grp_id++) {
        if (g_vpu_grp_infos[grp_id].enable) {
            for (chn_id = 0; chn_id < MAX_VPU_CHN_NUM; chn_id++) {
                if (g_enc_chn_infos[grp_id][chn_id].enable) {
                    ret = dsp_enc_init(grp_id, chn_id);
                    FBV_FUNC_ERROR_PRT("dsp_enc_init", ret);
                }
            }
        }
    }
    return ret;
}

FH_SINT32 fbv_dsp_init(FH_VOID) {
    FH_SINT32 ret = 0;

    ret = fbv_dsp_system_init();
    FBV_FUNC_ERROR_PRT("fbv_dsp_system_init", ret);

    ret = fbv_dsp_vpu_init();
    FBV_FUNC_ERROR_PRT("fbv_dsp_vpu_init", ret);

    ret = fbv_dsp_vpu_chn_init();
    FBV_FUNC_ERROR_PRT("fbv_dsp_vpu_chn_init", ret);

    return ret;
}

// Allen-D
FH_SINT32 mjpeg_create_chn(FH_SINT32 grp_id, FH_SINT32 chn_id) {
    FH_SINT32 ret = 0;
    FH_VENC_CHN_CAP cfg_vencmem;
    FH_SINT32 mjpeg_chn = 0;

    cfg_vencmem.support_type = FH_MJPEG;
    cfg_vencmem.max_size.u32Width = g_jpeg_chn_infos.width;
    cfg_vencmem.max_size.u32Height = g_jpeg_chn_infos.height;
    mjpeg_chn = g_jpeg_chn_infos.channel;
    cfg_vencmem.work_mode = FH_NORMAL_MODE;

    if (mjpeg_chn == 0) // 未被初始化过
    {
        ret = jpeg_set_chn_buffer(g_mjpeg_chn);
        FBV_FUNC_ERROR_PRT("jpeg_set_chn_buffer", ret);

        ret = FH_VENC_CreateChn(g_mjpeg_chn, &cfg_vencmem);
        FBV_FUNC_ERROR_PRT("FH_VENC_CreateChn", ret);
        g_jpeg_chn_infos.channel = g_mjpeg_chn;
        g_mjpeg_chn--;
        printf("current mjpeg channel %d\n", g_jpeg_chn_infos.channel);
    } else {
        ret = FH_VENC_CreateChn(mjpeg_chn, &cfg_vencmem);
        FBV_FUNC_ERROR_PRT("FH_VENC_CreateChn", ret);
    }

    return ret;
}

FH_SINT32 jpeg_create_chn(FH_UINT32 chn_w, FH_UINT32 chn_h, FH_UINT32 chn_id) {
    FH_SINT32 ret = 0;
    FH_VENC_CHN_CAP cfg_vencmem;

    cfg_vencmem.support_type = FH_JPEG;
    cfg_vencmem.max_size.u32Width = chn_w;
    cfg_vencmem.max_size.u32Height = chn_h;
    cfg_vencmem.work_mode = FH_NORMAL_MODE;

    if (g_jpeg_chn == 0) // 未被初始化过
    {
        ret = jpeg_set_chn_buffer(JPEG_CHN);
        FBV_FUNC_ERROR_PRT("jpeg_set_chn_buffer", ret);

        ret = FH_VENC_CreateChn(JPEG_CHN, &cfg_vencmem);
        FBV_FUNC_ERROR_PRT("FH_VENC_CreateChn", ret);
        g_jpeg_chn = JPEG_CHN;

        ret = set_jpeg(g_jpeg_chn);
        FBV_FUNC_ERROR_PRT("set_jpeg", ret);
    } else if (g_jpeg_chn == JPEG_CHN) {
        ret = jpeg_set_chn_buffer(JPEG_CHN + 1);
        FBV_FUNC_ERROR_PRT("jpeg_set_chn_buffer for jpeg channel 1", ret);

        ret = FH_VENC_CreateChn(JPEG_CHN + 1, &cfg_vencmem);
        FBV_FUNC_ERROR_PRT("FH_VENC_CreateChn for jpeg channel 1", ret);
        g_jpeg_chn = 31;

        ret = set_jpeg(g_jpeg_chn);
        FBV_FUNC_ERROR_PRT("set_jpeg for jpeg channel 1", ret);
    }

    return ret;
}

FH_SINT32 set_jpeg(FH_UINT32 jpeg_chn) {
    FH_SINT32 ret = 0;
    FH_VENC_CHN_CONFIG cfg_param;

    cfg_param.chn_attr.enc_type = FH_JPEG;
    cfg_param.chn_attr.jpeg_attr.qp = 80; // Allen-D Increase the Qfactor for jpeg
    cfg_param.chn_attr.jpeg_attr.rotate = 0;
    cfg_param.chn_attr.jpeg_attr.encode_speed = 0; /* 大幅面下编码器性能不够时，设置为0 */

    ret = FH_VENC_SetChnAttr(jpeg_chn, &cfg_param);
    FBV_FUNC_ERROR_PRT("FH_VENC_SetChnAttr", ret);

    return ret;
}

FH_SINT32 set_mjpeg_rc(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_VENC_CHN_CONFIG* cfg_param) {
    FH_SINT32 ret = 0;
    cfg_param->rc_attr.rc_type = FH_RC_MJPEG_FIXQP;
    cfg_param->rc_attr.mjpeg_fixqp.qp = 50;
    cfg_param->rc_attr.mjpeg_fixqp.FrameRate.frame_count = 25;
    cfg_param->rc_attr.mjpeg_fixqp.FrameRate.frame_time = 1;

    ret = FH_VENC_SetChnAttr(1, cfg_param);
    FBV_FUNC_ERROR_PRT("FH_VENC_SetChnAttr", ret);

    return ret;
}

FH_SINT32 set_mjpeg(FH_SINT32 grp_id, FH_SINT32 chn_id) {
    FH_SINT32 ret = 0;
    FH_VENC_CHN_CONFIG cfg_param;

    cfg_param.chn_attr.enc_type = FH_MJPEG;
    cfg_param.chn_attr.mjpeg_attr.pic_size.u32Width = g_jpeg_chn_infos.width;
    cfg_param.chn_attr.mjpeg_attr.pic_size.u32Height = g_jpeg_chn_infos.height;
    cfg_param.chn_attr.mjpeg_attr.rotate = 0;
    cfg_param.chn_attr.mjpeg_attr.encode_speed = 0; /* 0-9 */

    ret = set_mjpeg_rc(grp_id, chn_id, &cfg_param);
    FBV_FUNC_ERROR_PRT("set_mjpeg_rc", ret);

    return ret;
}

FH_SINT32 jpeg_set_chn_buffer(FH_SINT32 chn_id) {
    FH_SINT32 ret = 0;
    FH_VENC_CHN_PARAM pstchnparam;

    memset(&pstchnparam, 0, sizeof(pstchnparam));
    ret = FH_VENC_GetChnParam(chn_id, &pstchnparam);
    FBV_FUNC_ERROR_PRT("FH_VENC_GetChnParam", ret);

    pstchnparam.stm_mode = FH_STM_PUB_BUF;

    ret = FH_VENC_SetChnParam(chn_id, &pstchnparam);
    FBV_FUNC_ERROR_PRT("FH_VENC_SetChnParam", ret);
    return ret;
}

ENC_INFO* get_mjpeg_config(FH_SINT32 grp_id, FH_SINT32 chn_id) {
    return &g_jpeg_chn_infos;
}

ENC_INFO* get_venc_config(FH_SINT32 grp_id, FH_SINT32 chn_id) {
    return &g_enc_chn_infos;
}

FH_SINT32 mjpeg_start(FH_SINT32 grp_id, FH_SINT32 chn_id) {
    FH_SINT32 ret = 0;

    ret = FH_VENC_StartRecvPic(g_jpeg_chn_infos.channel);
    FBV_FUNC_ERROR_PRT("FH_VENC_StartRecvPic", ret);

    return ret;
}

/*
FH_SINT32 jpeg_stop(FH_VOID)
{
    FH_SINT32 ret = 0;

    if (g_jpeg_chn)
    {
        ret = FH_VENC_StopRecvPic(g_jpeg_chn);
        FBV_FUNC_ERROR_PRT("FH_VENC_StopRecvPic", ret);
    }

    return ret;
}

FH_SINT32 jpeg_destroy(FH_VOID)
{
    FH_SINT32 ret = 0;

    ret = jpeg_stop();
    if (g_jpeg_chn)
    {
        ret = FH_VENC_DestroyChn(g_jpeg_chn);
        FBV_FUNC_ERROR_PRT("FH_VENC_DestroyChn", ret);
        g_jpeg_chn = 0;
    }

    return ret;
}*/

FH_SINT32 get_jpeg_chn(FH_VOID) {
    return g_jpeg_chn;
}

FH_SINT32 get_mjpeg_chn(FH_VOID) {
    return g_mjpeg_chn;
}

FH_SINT32 vpu_bind_jpeg(FH_UINT32 src_grp_id, FH_UINT32 src_chn_id, FH_UINT32 dst_chn_id) {
    FH_SINT32 ret = 0;
    FH_BIND_INFO src, dst;

    src.obj_id = FH_OBJ_VPU_VO;
    src.dev_id = src_grp_id;
    src.chn_id = src_chn_id;

    dst.obj_id = FH_OBJ_JPEG;
    dst.dev_id = 0;
    dst.chn_id = dst_chn_id;

    ret = FH_SYS_Bind(src, dst);
    FBV_FUNC_ERROR_PRT("FH_SYS_Bind", ret);

    return ret;
}

FH_SINT32 unbind_jpeg_chn(FH_UINT32 jpeg_chn) {
    FH_SINT32 ret = 0;
    FH_BIND_INFO dst;

    dst.obj_id = FH_OBJ_JPEG;
    dst.dev_id = 0;
    dst.chn_id = jpeg_chn;

    ret = FH_SYS_UnBindbyDst(dst);
    FBV_FUNC_ERROR_PRT("FH_SYS_UnBindbyDst", ret);

    return ret;
}

FH_SINT32 vpu_grp_disable(FH_SINT32 grp_id) {
    FH_SINT32 ret = 0;

    // 去使能vpu
    ret = FH_VPSS_Disable(grp_id);
    FBV_FUNC_ERROR_PRT("FH_VPSS_Disable", ret);

    return ret;
}

FH_SINT32 vpu_grp_enable(FH_SINT32 grp_id) {
    FH_SINT32 ret = 0;

    // 使能vpu
    ret = FH_VPSS_Enable(grp_id, g_vpu_grp_infos[grp_id].mode); /*ISP直通模式,可以节省内存开销*/
    FBV_FUNC_ERROR_PRT("FH_VPSS_Enable", ret);

    return ret;
}