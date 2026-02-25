#include "fbv_common.h"

static NN_ADDR_INFO_t *nn_model_addr;
static char *pp_init_buffer;
static FH_VOID *g_postHandle;

static FH_UINT32 compare_rect(FH_RECT_T rect1, FH_RECT_T rect2)
{
    FH_UINT32 dt_x = 0;
    FH_UINT32 dt_y = 0;
    FH_UINT32 dt_w = 0;
    FH_UINT32 dt_h = 0;
    FH_UINT32 thresh = 10; // 0~100% percent

    if (rect1.x > rect2.x)
    {
        dt_x = (FH_UINT32)(rect1.x - rect2.x);
    }
    else
    {
        dt_x = (FH_UINT32)(rect2.x - rect1.x);
    }

    if ((FH_UINT32)dt_x * 100 > (FH_UINT32)rect1.w * thresh)
    {
        return 1;
    }

    if (rect1.y > rect2.y)
    {
        dt_y = (FH_UINT32)(rect1.y - rect2.y);
    }
    else
    {
        dt_y = (FH_UINT32)(rect2.y - rect1.y);
    }

    if ((FH_UINT32)dt_y * 100 > (FH_UINT32)rect1.h * thresh)
    {
        return 1;
    }

    if (rect1.w > rect2.w)
    {
        dt_w = (FH_UINT32)(rect1.w - rect2.w);
    }
    else
    {
        dt_w = (FH_UINT32)(rect2.w - rect1.w);
    }

    if ((FH_UINT32)dt_w * 100 > (FH_UINT32)rect1.w * thresh)
    {
        return 1;
    }

    if (rect1.h > rect2.h)
    {
        dt_h = (FH_UINT32)(rect1.h - rect2.h);
    }
    else
    {
        dt_h = (FH_UINT32)(rect2.h - rect1.h);
    }

    if ((FH_UINT32)dt_h * 100 > (FH_UINT32)rect1.h * thresh)
    {
        return 1;
    }

    return 0;
}

static FH_VOID draw_box(FH_DETECTION_T *out, FH_UINT32 box_chnw, FH_UINT32 box_chnh)
{
    static FH_DETECTION_T out_last = {0};
    FH_SINT32 i;
    FH_SINT32 dt_x;
    FH_SINT32 dt_y;
    FH_SINT32 dt_w;
    FH_SINT32 dt_h;

    FHT_RgbColor_t box_color = {255, 0, 0, 255};

    for (i = 0; i < out->boxNum; i++)
    {
        out->detBBox[i].bbox.x *= box_chnw;
        out->detBBox[i].bbox.y *= box_chnh;
        out->detBBox[i].bbox.w *= box_chnw;
        out->detBBox[i].bbox.h *= box_chnh;

        if (!compare_rect(out_last.detBBox[i].bbox, out->detBBox[i].bbox))
        {
            continue;
        }
        else
        {
            out_last.detBBox[i].bbox = out->detBBox[i].bbox;
        }

        dt_x = out->detBBox[i].bbox.x;
        dt_y = out->detBBox[i].bbox.y;
        dt_w = out->detBBox[i].bbox.w;
        dt_h = out->detBBox[i].bbox.h;

        sample_set_gbox(FH_APP_GRP_ID, 0, 1, i, dt_x, dt_y, dt_w, dt_h, box_color);
    }

    for (; i < out_last.boxNum; i++)
    {
        sample_set_gbox(FH_APP_GRP_ID, 0, 0, i, 0, 0, 0, 0, box_color);
    }

    out_last.boxNum = out->boxNum;
}

extern void do_nna_work(void);
extern unsigned int fh_getNBGAddr(void);

static void init_nbg_model(void)
{
    unsigned int NN_MODEL_BUFFER = fh_getNBGAddr();
    MODEL_CHECK_HDR_t *model_data = (MODEL_CHECK_HDR_t *)NN_MODEL_BUFFER;

    while (model_data->check_magic != 0xaabbccdd)
    {

        rt_thread_delay(1);

        fh_hw_invalidate_dcache(NN_MODEL_BUFFER, 32);
    }

    unsigned int NN_ADDR = fh_getNBGAddr() + 0x20;
    nn_model_addr = (NN_ADDR_INFO_t *)NN_ADDR;
    fh_hw_invalidate_dcache(NN_ADDR, 32);

    if (nn_model_addr->nbg_load_addr != NN_ADDR - 0x20)
    {
        rt_kprintf("\nnbg base addr mismatch!!!!!\n");
        return;
    }

    pp_init_buffer = (char *)rt_malloc(nn_model_addr->post_length);
    rt_memcpy((void *)pp_init_buffer, (void *)(NN_ADDR + 32), nn_model_addr->post_length);

    unsigned int magic_end = readl(NN_MODEL_BUFFER + model_data->headLen + model_data->dataLen);
    while (magic_end != 0xaabbccdd)
    {
        rt_thread_delay(1);
        magic_end = readl(NN_MODEL_BUFFER + model_data->headLen + model_data->dataLen);
    }
    /* do some init here */
    extern void do_nna_init(unsigned int addr);
    do_nna_init(nn_model_addr->pkg_addr);
}

//销毁后处理handle
static FH_SINT32 sample_nna_post_deinit(FH_VOID *postHandle)
{
    int ret = 0;
    if (postHandle == RT_NULL)
    {
        rt_kprintf("postHandle NULL\n");
        return -1;
    }

    ret = FH_NNPost_PrimaryDet_Destroy(postHandle);
    if (ret)
    {
        rt_kprintf("FH_NNPost_PrimaryDet_Destroy faild %x\n", ret);
    }
    return ret;
}

//创建后处理handle
static FH_VOID sample_nna_post_init(FH_VOID)
{
    int ret = 0;
    NnPostInitParam post_param;

    rt_memset(&post_param, 0, sizeof(NnPostInitParam));

    post_param.pp_conf_in.vbase = (void *)pp_init_buffer;
    post_param.pp_conf_in.base = (unsigned int)pp_init_buffer;
    post_param.pp_conf_in.size = nn_model_addr->post_length;
    post_param.config_th = 0.8;
    post_param.rotate = FN_ROT_0;

    ret = FH_NNPost_PrimaryDet_Create(&g_postHandle, &post_param);
    if (ret)
    {
        rt_kprintf("FH_NNPost_PrimaryDet_Create faild %x\n", ret);
    }
}

static void wait_for_a_pic(FH_UINT64 *frame_id, FH_UINT64 *time_stamp)
{
    FH_SINT32 ret;
#ifdef TIME_STAT_PTS
    static int idx = 0;
#endif
    FH_VPU_STREAM_ADV frmData;
    ret = FH_VPSS_GetChnFrameAdv_NoRpt(FH_APP_GRP_ID, NN_DETECT_CHAN, &frmData, 1000);
    if (ret)
    {
        rt_kprintf("[nn]:FH_VPSS_GetChnFrameAdv_NoRpt fail %x\n", ret);
        return;
    }

    *frame_id = frmData.frame_id;
    *time_stamp = frmData.time_stamp;
#ifdef TIME_STAT_PTS
    idx += 1;
    if (idx == 1)
    {
        fbv_time_stat_add("CLASS1", "First NN Frame Start");
    }
#endif
    rt_memcpy((void *)nn_model_addr->image_input_addr, frmData.frm_rgb888.data.vbase, frmData.frm_rgb888.data.size);
}

void do_nn_detect(void *args)
{
    int ret = 0;
    static int flag = 0;
    FH_NN_POST_INPUT_INFO post_input_info;
    FH_UINT32 box_chn_w;
    FH_UINT32 box_chn_h;
    FH_DETECTION_T det_out;
    FH_UINT64 frame_id = 0;
    FH_UINT64 time_stamp = 0;
    rt_memset(&post_input_info, 0, sizeof(FH_NN_POST_INPUT_INFO));


    /* 等待NBG模型加载/初始化完毕  */
    init_nbg_model();

    //初始化后处理handle
    sample_nna_post_init();

    box_chn_w = g_vpu_chn_info[FH_APP_GRP_ID][0].width;
    box_chn_h = g_vpu_chn_info[FH_APP_GRP_ID][0].height;

    while (1)
    {
        post_input_info.hw_output_mem.base = (unsigned int)nn_model_addr->post_process_addr;
        post_input_info.hw_output_mem.vbase = (void *)nn_model_addr->post_process_addr;
        post_input_info.current_rotate = FN_ROT_0;
        wait_for_a_pic(&frame_id, &time_stamp);
        do_nna_work();
        ret = FH_NNPost_PrimaryDet_Process(g_postHandle, &post_input_info, &det_out);
        if (ret)
        {
            rt_kprintf("[nn]:FH_NNPost_PrimaryDet_Process fail %x\n", ret);
        }

        draw_box(&det_out, box_chn_w, box_chn_h);

#ifdef TIME_STAT_PTS
        if (det_out.boxNum > 0 && !flag)
        {
            fbv_time_stat_add("CLASS1", "First NN Detected");
            rt_kprintf("\nnna dected  bxox num =  %d, timestamp = %d\n", det_out.boxNum, read_pts32());
            flag++;
        }
#endif
    }
    sample_nna_post_deinit(g_postHandle);
}

void start_do_nn_detect(void)
{
    rt_thread_t fbv_thread_nn;

    /* NN线程优先级应低于应用线程，以保证应用逻辑能够正常运行
     * 应用逻辑包括策略调整，从而保证快启模式下，前面几帧能够快速调整完毕
     */
    fbv_thread_nn = rt_thread_create("fbv_nn_thrd", do_nn_detect, RT_NULL,
                                     15 * 1024, FH_DEMO_THREAD_PRIORITY + 1, 10);
    rt_thread_startup(fbv_thread_nn);
}
