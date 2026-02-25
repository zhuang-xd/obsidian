#include "fbv_common.h"

#ifdef ENBALE_SMALL_TO_BIG
static unsigned short g_awb_speed_save;
static unsigned short g_ae_speed_save;
#endif

#ifdef TIME_STAT_PTS

#ifndef ENBALE_SMALL_TO_BIG
#define FRAMES_TO_CHANGE_SIZE (0)
#endif

// Allen-D
#define SAMPLE_SENSOR_FLAG_NORMAL (0x00)
#define SAMPLE_SENSOR_FLAG_NIGHT (0x02)
#define SAMPLE_SENSOR_FLAG_WDR (0x01)
#define SAMPLE_SENSOR_FLAG_WDR_NIGHT (SAMPLE_SENSOR_FLAG_NIGHT | SAMPLE_SENSOR_FLAG_WDR)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
static rt_thread_t g_thread_isp[MAX_GRP_NUM] = {RT_NULL};

ISP_AE_INFO g_pstAeInfo;
FH_UINT32 g_newIntt;
FH_UINT32 g_newGain;

typedef enum {
    FORMAT_CHANGE_400WP30 = 4,
    FORMAT_CHANGE_400WP25 = 5,
    FORMAT_CHANGE_1080P30 = 6,
    FORMAT_CHANGE_1080P25 = 7,
    FORMAT_CHANGE_720P30 = 8,
    FORMAT_CHANGE_720P25 = 9,
    FORMAT_CHANGE_320P30 = 10,
    FORMAT_CHANGE_320P25 = 11,
} RTT_CMD_ID;

static int startof_pict_grp0() {
    static int idx_small = 0;
    static int idx_big = 0;
    static int count = 0;

    if ((count < FRAMES_TO_CHANGE_SIZE) && (idx_small == 0)) {
        fbv_time_stat_add("CLASS1", "(sensor 0)First small pic start");
        idx_small++;
    }

    if ((count >= FRAMES_TO_CHANGE_SIZE) && (idx_big == 0)) {
        fbv_time_stat_add("CLASS1", "(sensor 0)First big pic start");
        idx_big++;
    }

    count++;
    return 0;
}

static int endof_pict_grp0() {
    static int idx_small = 0;
    static int idx_big = 0;
    static int count = 0;

    if ((count < FRAMES_TO_CHANGE_SIZE) && (idx_small == 0)) {
        fbv_time_stat_add("CLASS1", "(sensor 0)First small pic end");
        idx_small++;
    }

    if ((count >= FRAMES_TO_CHANGE_SIZE) && (idx_big == 0)) {
        fbv_time_stat_add("CLASS1", "(sensor 0)First big pic end");
        idx_big++;
    }

    count++;
    return 0;
}

static int startof_pict_grp1() {
    static int idx_small = 0;
    static int idx_big = 0;
    static int count = 0;

    if ((count < FRAMES_TO_CHANGE_SIZE) && (idx_small == 0)) {
        fbv_time_stat_add("CLASS1", "(sensor 1)First small pic start");
        idx_small++;
    }

    if ((count >= FRAMES_TO_CHANGE_SIZE) && (idx_big == 0)) {
        fbv_time_stat_add("CLASS1", "(sensor 1)First big pic start");
        idx_big++;
    }

    count++;
    return 0;
}

static int endof_pict_grp1() {
    static int idx_small = 0;
    static int idx_big = 0;
    static int count = 0;

    if ((count < FRAMES_TO_CHANGE_SIZE) && (idx_small == 0)) {
        fbv_time_stat_add("CLASS1", "(sensor 1)First small pic end");
        idx_small++;
    }

    if ((count >= FRAMES_TO_CHANGE_SIZE) && (idx_big == 0)) {
        fbv_time_stat_add("CLASS1", "(sensor 1)First big pic end");
        idx_big++;
    }
    count++;
    return 0;
}

static int startof_pict_grp2() {
    static int idx_small = 0;
    static int idx_big = 0;
    static int count = 0;

    if ((count < FRAMES_TO_CHANGE_SIZE) && (idx_small == 0)) {
        fbv_time_stat_add("CLASS1", "(sensor 2)First small pic start");
        idx_small++;
    }

    if ((count >= FRAMES_TO_CHANGE_SIZE) && (idx_big == 0)) {
        fbv_time_stat_add("CLASS1", "(sensor 2)First big pic start");
        idx_big++;
    }

    count++;
    return 0;
}

static int endof_pict_grp2() {
    static int idx_small = 0;
    static int idx_big = 0;
    static int count = 0;

    if ((count < FRAMES_TO_CHANGE_SIZE) && (idx_small == 0)) {
        fbv_time_stat_add("CLASS1", "(sensor 2)First small pic end");
        idx_small++;
    }

    if ((count >= FRAMES_TO_CHANGE_SIZE) && (idx_big == 0)) {
        fbv_time_stat_add("CLASS1", "(sensor 2)First big pic end");
        idx_big++;
    }
    count++;
    return 0;
}
#endif /*TIME_STAT_PTS*/

ISP_INFO g_isp_info[MAX_GRP_NUM] = {

    {
        .enable = 0,
#ifdef VIDEO_GRP0
        .enable = 1,
        .grpid = 0,
#ifdef ENBALE_SMALL_TO_BIG
        .isp_init_width_pre = SMALL_ISP_WIGHT,
        .isp_init_height_pre = SMALL_ISP_HEIGHT,
#endif
        .isp_init_width = ISP_WIDTH_G0,
        .isp_init_height = ISP_HEIGHT_G0,
#ifdef ENBALE_SMALL_TO_BIG
        .isp_format_pre = ISP_FORMAT_G9,
#endif
        .isp_format = ISP_FORMAT_G0,
        .sensor_name = ISP_SENSOR_NAME_G0,
        .lut2dWorkMode = ISP_LUT2D_BYPASS,
#endif
    },
    {
        .enable = 0,
#ifdef VIDEO_GRP1
        .enable = 1,
        .grpid = 1,
#ifdef ENBALE_SMALL_TO_BIG
        .isp_init_width_pre = SMALL_ISP_WIGHT,
        .isp_init_height_pre = SMALL_ISP_HEIGHT,
#endif
        .isp_init_width = ISP_WIDTH_G1,
        .isp_init_height = ISP_HEIGHT_G1,
#ifdef ENBALE_SMALL_TO_BIG
        .isp_format_pre = ISP_FORMAT_G9,
#endif
        .isp_format = ISP_FORMAT_G1,
        .sensor_name = ISP_SENSOR_NAME_G1,
        .lut2dWorkMode = ISP_LUT2D_BYPASS,
#endif
    },
    {
        .enable = 0,
#ifdef VIDEO_GRP2
        .enable = 1,
        .grpid = 2,
#ifdef ENBALE_SMALL_TO_BIG
        .isp_init_width_pre = SMALL_ISP_WIGHT,
        .isp_init_height_pre = SMALL_ISP_HEIGHT,
#endif
        .isp_init_width = ISP_WIDTH_G2,
        .isp_init_height = ISP_HEIGHT_G2,
#ifdef ENBALE_SMALL_TO_BIG
        .isp_format_pre = ISP_FORMAT_G9,
#endif
        .isp_format = ISP_FORMAT_G2,
        .sensor_name = ISP_SENSOR_NAME_G2,
        .lut2dWorkMode = ISP_LUT2D_BYPASS,
#endif
    },
};

static FH_SINT32 get_sensorFormat_wdr_mode(FH_SINT32 format) {
    FH_SINT32 wdr_mode;

    wdr_mode = (format & 0x10000) >> 16;

    return wdr_mode;
}

FH_VOID Sensor_Destroy(struct isp_sensor_if* s_if) {
    return;
}

static FH_SINT32 choose_i2c(FH_SINT32 grp_id) {
    switch (grp_id) {
    case 0:
        return 0;
    case 1:
        return 0;
    default:
        break;
    }

    return 0;
}

#ifdef ENBALE_SMALL_TO_BIG
void isp_strategy(FH_SINT32 grp_id) {
    FH_SINT32 ret;
    FH_UINT32 frameCnt = 0;
    while (frameCnt < FRAMES_TO_CHANGE_SIZE) {
        // Allen-D add ae log
        // isp_write_proc("aelog_0xfff_0");
        ret = API_ISP_Run(grp_id);
        if (ret == 0) {
            frameCnt++;
        } else {
            rt_kprintf("[ISP STRATEGY]API_ISP_Run faild %x\n", ret);
        }
        /* save_chan_frame();   */
    }
    // isp_write_proc("aelog_0x0_0");
}
#endif

extern void sensor_reset(int grp_id, unsigned long rst_delay);
FH_SINT32 isp_change_resolution(FH_SINT32 grp_id) {
    int ret = -1;
    unsigned int rst_delay;
    ISP_SENSOR_COMMON_CMD_DATA0 fsParam;

#if 0
    if (g_sensor != RT_NULL)
    {
        rt_kprintf("common_if = %x\n", g_sensor->common_if);
    }
#endif
    // g_isp_info[grp_id].sensor->common_if((void *)g_isp_info[grp_id].sensor, CMD_GET_GPIO_PARAM, &fsParam, RT_NULL);
    // if (fsParam.gpio_param_cfg.gpio_enable)
    // {
    //     rst_delay = fsParam.gpio_param_cfg.gpio_time;
    //    sensor_reset(grp_id, rst_delay);
    // }

    AWB_DEFAULT_CFG awbCfg;
    AE_DEFAULT_CFG aeCfg;
    ISP_AE_INFO aeInfo;

    ISP_SENSOR_COMMON_CMD_DATA0 mapCfg;

    ret = API_ISP_GetAeInfo(grp_id, &aeInfo);
    FBV_FUNC_ERROR_PRT("API_ISP_GetAeInfo", ret);
    rt_kprintf("[AE-DEBUG]aeInfo intt = %d, aeInfo gain = %d\n", aeInfo.u32Intt, aeInfo.u32TotalGain);
    ret = API_ISP_SetSensorFmt(grp_id, g_isp_info[grp_id].isp_format);
    FBV_FUNC_ERROR_PRT("API_ISP_SetSensorFmt", ret);

    mapCfg.mapInttGainCfg.currFormat = g_isp_info[grp_id].isp_format_pre;
    // mapCfg.mapInttGainCfg.desFormat = g_isp_info[grp_id].isp_format;
    mapCfg.mapInttGainCfg.desFormat = FORMAT_400WP25;
    mapCfg.mapInttGainCfg.currIntt = aeInfo.u32Intt;
    mapCfg.mapInttGainCfg.currTotalGain = aeInfo.u32TotalGain;
    g_isp_info[grp_id].sensor->common_if((void*)g_isp_info[grp_id].sensor, CMD_MAP_INTT_GAIN, &mapCfg, RT_NULL);
    rt_kprintf("[AE-DEBUG]curfmt = 0x%x, desfmt = 0x%x, curIntt = %d, curGain = %d\n", mapCfg.mapInttGainCfg.currFormat,
               mapCfg.mapInttGainCfg.desFormat, mapCfg.mapInttGainCfg.currIntt, mapCfg.mapInttGainCfg.currTotalGain);

    ret = API_ISP_SetSensorIntt(grp_id, mapCfg.mapInttGainCfg.desIntt);
    FBV_FUNC_ERROR_PRT("API_ISP_SetSensorIntt", ret);
    rt_kprintf("[AE-DEBUG]desIntt = %d\n", mapCfg.mapInttGainCfg.desIntt);
    ret = API_ISP_SetSensorGain(grp_id, mapCfg.mapInttGainCfg.desTotalGain);
    FBV_FUNC_ERROR_PRT("API_ISP_SetSensorGain", ret);
    rt_kprintf("[AE-DEBUG]desTotalGain = %d\n", mapCfg.mapInttGainCfg.desTotalGain);
    g_newIntt = mapCfg.mapInttGainCfg.desIntt;
    g_newGain = mapCfg.mapInttGainCfg.desTotalGain;

    ret = API_ISP_SensorKick(grp_id);
    FBV_FUNC_ERROR_PRT("API_ISP_SensorKick", ret);
#if 0
    API_ISP_GetAeDefaultCfg(grp_id, &aeCfg);
    API_ISP_SetAeDefaultCfg(grp_id, &aeCfg);
#else
    API_ISP_GetAwbDefaultCfg(grp_id, &awbCfg);
    API_ISP_GetAeDefaultCfg(grp_id, &aeCfg);
    awbCfg.u16Speed = g_awb_speed_save;
    aeCfg.stAeSpeedCfg.u08Speed = g_ae_speed_save;
    API_ISP_SetAwbDefaultCfg(grp_id, &awbCfg);
    API_ISP_SetAeDefaultCfg(grp_id, &aeCfg);
#endif
    return 0;
}

FH_SINT32 isp_change_resolution_EXT(FH_SINT32 grp_id) {
    ISP_AE_STATUS stAeStatus;
    AWB_DEFAULT_CFG awbCfg;
    AE_DEFAULT_CFG aeCfg;
    uint32_t againMin = 256;
    uint32_t againMax = 4096;
    uint32_t inttMax = 1468;

    // 配置 sensor 输出制式（低帧率大幅面，例如 2560*1440@15fps）
    API_ISP_SetSensorFmt(grp_id, g_isp_info[grp_id].isp_format); // 注意不要 kick 出流

    // 小图的曝光参数转换到大图曝光参数
    uint32_t init;           // 小图曝光行
    uint32_t totalGain;      // 小图总增益
    uint32_t smallFps = 240; // 小图帧率
    uint32_t smallFH = 320;  // 小图 FrameHeight
    uint32_t exposureNew;    // 大图曝光量
    uint32_t initNew;        // 大图曝光行
    uint32_t totalGainNew;   // 大图总增益
    uint32_t largeFps = 25;  // 大图帧率
    uint32_t largeFH = 1440; // 大图 FrameHeight

    // 获取小图当前曝光参数
    API_ISP_GetAeStatus(grp_id, &stAeStatus);
    init = stAeStatus.u32CurrIntt;
    totalGain = stAeStatus.u32CurrTotalGain;

    // 曝光参数转换计算
    exposureNew = init * totalGain * largeFps * largeFH / (smallFps * smallFH);

    if (exposureNew < (16 * 64)) {
        initNew = MAX(1, exposureNew >> 6);
        totalGainNew = MAX(againMin, exposureNew / initNew);
    } else {
        initNew = MIN(inttMax, exposureNew >> 6);
        totalGainNew = MAX(againMin, exposureNew / initNew);
    }

    // 配置大图曝光参数
    ISP_AE_INFO pstAeInfo;
    pstAeInfo.u32Intt = initNew;
    pstAeInfo.u32TotalGain = totalGainNew;

    if (totalGainNew <= againMax) {
        pstAeInfo.u32SensorGain = totalGainNew;
        pstAeInfo.u32IspGain = 0x100; // ispGain U.8 精度
    } else {
        pstAeInfo.u32SensorGain = againMax;
        pstAeInfo.u32IspGain = (totalGainNew << 8) / againMax; // ispGain U.8 精度
    }
    API_ISP_SetAeInfo(grp_id, &pstAeInfo);

    g_pstAeInfo = pstAeInfo;

    // AWB 策略 speed 恢复原始设置
    API_ISP_GetAwbDefaultCfg(grp_id, &awbCfg);
    awbCfg.u16Speed = g_awb_speed_save;
    API_ISP_SetAwbDefaultCfg(grp_id, &awbCfg);

    // 其他大图初始化效果参数设置
    // API_ISP_SetXXXInitCfg(u32Id, &XXX);

    // delay 20ms 等待配置生效，Kick sensor 出流
    udelay(20000);
    rt_kprintf("[DEBUG] Add small to big parameters translate\n");
    API_ISP_SensorKick(grp_id);
    return 0;
}

static FH_SINT32 isp_init(FH_SINT32 grp_id) {
    FH_SINT32 ret = 0;
    FH_UINT16 sensorFps = 0;
    char* isp_param;
    ISP_MEM_INIT stMemInit = {0};

    stMemInit.enOfflineWorkMode = ISP_OFFLINE_MODE_DISABLE;
    // Allen-D Alloc isp memory as 400W(max) for size change function
    // stMemInit.stPicConf.u32Width = g_isp_info[grp_id].isp_init_width;
    // stMemInit.stPicConf.u32Height = g_isp_info[grp_id].isp_init_height;
    stMemInit.stPicConf.u32Width = 2560;
    stMemInit.stPicConf.u32Height = 1440;
    stMemInit.enIspOutMode = ISP_OUT_TO_VPU;
    ret = API_ISP_MemInit(grp_id, &stMemInit);
    rt_kprintf("[DEBUG]isp init max width = %d, max height = %d\n", stMemInit.stPicConf.u32Width,
               stMemInit.stPicConf.u32Height);
    FBV_FUNC_ERROR_PRT("API_ISP_MemInit", ret);

    g_isp_info[grp_id].sensor = Sensor_Create(g_isp_info[grp_id].sensor_name);

    ret = API_ISP_SensorRegCb(grp_id, 0, g_isp_info[grp_id].sensor);
    FBV_FUNC_ERROR_PRT("API_ISP_SensorRegCb", ret);

#ifdef CONFIG_SENSOR_SLAVE_MODE // sc3336 双目的时候不需要 sensor kick
    // pull up sensor
    sensor_exit_reset(grp_id);
#endif

    ret = change_sensor_slaveaddr(grp_id, g_isp_info[grp_id].sensor_name);
    FBV_FUNC_ERROR_PRT("change_sensor_slaveaddr", ret);

    Sensor_Init_t initConf = {0};
    initConf.u8CsiDeviceId = grp_id;
    initConf.u8CciDeviceId = choose_i2c(grp_id);
    initConf.bGrpSync = FH_FALSE;
    ret = API_ISP_SensorInit(grp_id, &initConf);
    FBV_FUNC_ERROR_PRT("API_ISP_SensorInit", ret);

#ifdef ENBALE_SMALL_TO_BIG
    ret = API_ISP_SetSensorFmt(grp_id, g_isp_info[grp_id].isp_format_pre);
    FBV_FUNC_ERROR_PRT("API_ISP_SetSensorFmt", ret);
#else
    ret = API_ISP_SetSensorFmt(grp_id, g_isp_info[grp_id].isp_format);
    FBV_FUNC_ERROR_PRT("API_ISP_SetSensorFmt", ret);
#endif

#ifdef TIME_STAT_PTS
    if (grp_id == 0) {
        API_ISP_RegisterPicStartCallback(grp_id, startof_pict_grp0);
        API_ISP_RegisterPicEndCallback(grp_id, endof_pict_grp0);
    } else if (grp_id == 1) {
        API_ISP_RegisterPicStartCallback(grp_id, startof_pict_grp1);
        API_ISP_RegisterPicEndCallback(grp_id, endof_pict_grp1);
    } else {
        API_ISP_RegisterPicStartCallback(grp_id, startof_pict_grp2);
        API_ISP_RegisterPicEndCallback(grp_id, endof_pict_grp2);
    }
#endif

#ifndef CONFIG_SENSOR_SLAVE_MODE // sc3336 双目的时候不需要 sensor kick
    ret = API_ISP_SensorKick(grp_id);
    FBV_FUNC_ERROR_PRT("API_ISP_SensorKick", ret);
#endif
    ISP_VI_ATTR_S sensor_vi_attr = {0};
    // vicap
    ret = API_ISP_GetViAttr(grp_id, &sensor_vi_attr);
    FBV_FUNC_ERROR_PRT("API_ISP_GetViAttr", ret);

#ifdef FH_USING_GC2083_MIPI
    if (grp_id == 0) {
        //修改sensor 幅面对应的寄存器只修改寄存器的低8位 1076 -> 0x434
        ret = API_ISP_SetSensorReg(grp_id, 0x0196, 0x34);
        FBV_FUNC_ERROR_PRT("API_ISP_SetSensorReg", ret);
        sensor_vi_attr.u16InputHeight = 1076; // 1084
        sensor_vi_attr.u16PicHeight = 1076;
    } else if (grp_id == 1) {
        ret = API_ISP_SetSensorReg(grp_id, 0x0196, 0x38);
        FBV_FUNC_ERROR_PRT("API_ISP_SetSensorReg", ret);
        sensor_vi_attr.u16InputHeight = 1080; // 1080
        sensor_vi_attr.u16PicHeight = 1080;
    } else if (grp_id == 2) {
        ret = API_ISP_SetSensorReg(grp_id, 0x0196, 0x36);
        FBV_FUNC_ERROR_PRT("API_ISP_SetSensorReg", ret);
        sensor_vi_attr.u16InputHeight = 1078; // 1076
        sensor_vi_attr.u16PicHeight = 1078;   // 1076
    }

    ret = API_ISP_SetViAttr(grp_id, &sensor_vi_attr);
    FBV_FUNC_ERROR_PRT("API_ISP_SetViAttr", ret);
#endif

    ret = API_ISP_DropCnt(grp_id, 30);
    FBV_FUNC_ERROR_PRT("API_ISP_DropCnt", ret);
    ret = API_ISP_Init(grp_id);
    FBV_FUNC_ERROR_PRT("API_ISP_Init", ret);

    ret = API_ISP_GetViAttr(grp_id, &sensor_vi_attr);
    FBV_FUNC_ERROR_PRT("API_ISP_GetViAttr", ret);

    sensorFps = sensor_vi_attr.u16FrameRate;
    rt_kprintf("VICAP's FPS = %d\n", sensorFps);

    FH_VICAP_DEV_ATTR_S stViDev = {0};
    FH_VICAP_VI_ATTR_S stViAttr = {0};

    if (grp_id < 2) {
        stViAttr.enWorkMode = VICAP_WORK_MODE_ONLINE;
        stViDev.enWorkMode = VICAP_WORK_MODE_ONLINE;
#ifdef CONIFG_SWITCH_MODE
        stViAttr.enWorkMode = VICAP_WORK_MODE_SWITCH;
        stViDev.enWorkMode = VICAP_WORK_MODE_SWITCH;
        sensorFps = 0;
#endif
        stViDev.stSize.u16Width = g_isp_info[grp_id].isp_init_width;
        stViDev.stSize.u16Height = g_isp_info[grp_id].isp_init_height;
        stViDev.bUsingVb = 0;
        ret = FH_VICAP_InitViDev(grp_id, &stViDev);
        FBV_FUNC_ERROR_PRT("FH_VICAP_InitViDev", ret);

        ret = set_sensor_sync(grp_id, sensorFps);
        FBV_FUNC_ERROR_PRT("set_sensor_sync", ret);

        stViAttr.stInSize.u16Width = sensor_vi_attr.u16InputWidth;
        stViAttr.stInSize.u16Height = sensor_vi_attr.u16InputHeight;
        stViAttr.stCropSize.bCutEnable = 0;
        stViAttr.stCropSize.stRect.u16Width = sensor_vi_attr.u16PicWidth;
        stViAttr.stCropSize.stRect.u16Height = sensor_vi_attr.u16PicHeight;
        stViAttr.stCropSize.stRect.u16X = sensor_vi_attr.u16OffsetX;
        stViAttr.stCropSize.stRect.u16Y = sensor_vi_attr.u16OffsetY;
        stViAttr.stOfflineCfg.u8Priority = 0;
        stViAttr.u8WdrMode = get_sensorFormat_wdr_mode(g_isp_info[grp_id].isp_format);
        stViAttr.enBayerType = sensor_vi_attr.enBayerType;
        stViAttr.bitwidth = 12;
        ret = FH_VICAP_SetViAttr(grp_id, &stViAttr);
        FBV_FUNC_ERROR_PRT("FH_VICAP_SetViAttr", ret);
    }

#ifdef CONIFG_SWITCH_MODE
    FH_VICAP_SNS_MOUNT_CFG_S stSnsMountCfg = {0};
    if (grp_id == 0) {
        stSnsMountCfg.enSnsMount = SENSOR_ON_MIPI0;
        stSnsMountCfg.u16Height = 1076;
        FH_VICAP_SetSnsMount(grp_id, &stSnsMountCfg);
    } else if (grp_id == 1) {
        stSnsMountCfg.enSnsMount = SENSOR_ON_MIPI1;
        stSnsMountCfg.u16Height = 1080;
        FH_VICAP_SetSnsMount(grp_id, &stSnsMountCfg);
    } else if (grp_id == 2) {
        stSnsMountCfg.enSnsMount = SENSOR_ON_MIPI0;
        stSnsMountCfg.u16Height = 1078;
        FH_VICAP_SetSnsMount(grp_id, &stSnsMountCfg);
    }
#endif
    ret = start_sensor_sync(grp_id);
    FBV_FUNC_ERROR_PRT("start_sensor_sync", ret);

    // load hex file
    if (get_sensor_param(g_isp_info[grp_id].sensor_name, &isp_param, 0)) {
        rt_kprintf("get_sensor_param failed %s\n", g_isp_info[grp_id].sensor_name);
        return -1;
    }

    API_ISP_LoadIspParam(grp_id, isp_param);
    FBV_FUNC_ERROR_PRT("API_ISP_LoadIspParam", ret);

#ifdef ENBALE_SMALL_TO_BIG
    AWB_DEFAULT_CFG awbCfg;
    AE_DEFAULT_CFG aeCfg;
    API_ISP_GetAwbDefaultCfg(grp_id, &awbCfg);
    API_ISP_GetAeDefaultCfg(grp_id, &aeCfg);
    g_awb_speed_save = awbCfg.u16Speed;
    g_ae_speed_save = aeCfg.stAeSpeedCfg.u08Speed;
    awbCfg.u16Speed = 0xfff;         // awb调整速率调到最大
    aeCfg.stAeSpeedCfg.u08Speed = 0; // ae调整速率调到最大
    // aeCfg.stAeInitCfg.u08InitSpeed = 0x80; //Allen-D
    // aeCfg.stAeInitCfg.bInitAeFlag = 1; //Allen-D
    // aeCfg.stAeInitCfg.u08InitTolerance = 0xa; //Allen-D
    API_ISP_SetAwbDefaultCfg(grp_id, &awbCfg);
    API_ISP_SetAeDefaultCfg(grp_id, &aeCfg);
    rt_kprintf("[DEBUG] AE default\n");
    rt_kprintf("[DEBUG] u08OverExpGDlyFrm = %d, u08GreatChangeZone %d\n", aeCfg.stAeSpeedCfg.u08OverExpGDlyFrm,
               aeCfg.stAeSpeedCfg.u08GreatChangeZone);
    rt_kprintf("[DEBUG] bInitAeFlag = %d\n", aeCfg.stAeInitCfg.bInitAeFlag);
#endif
    return ret;
}

FH_SINT32 fbv_isp_init(FH_VOID) {
    FH_SINT32 ret = 0;
    FH_SINT32 grp_id = 0;
    // Allen-D
    mipi_write_proc("lane_4");
    // Allen-D debug trace
    // media_trace_write("create_0x16000000_0x100000");
    // media_trace_write("zonelog_0xffffffdf");
    // media_trace_write("timelev_8_0");
    // media_trace_write("timelev_9_0");

    for (grp_id = 0; grp_id < MAX_GRP_NUM; grp_id++) {
        if (g_isp_info[grp_id].enable) {
            ret = isp_init(grp_id);
            FBV_FUNC_ERROR_PRT("isp_init", ret);

#ifdef ENBALE_SMALL_TO_BIG
            isp_strategy(grp_id);
            if (change_size(grp_id)) {
                rt_kprintf("\nFAST_VIDEO_RESIZE faild!!!!\n");
                return -1;
            }
#endif
        }
    }
    return ret;
}

static FH_VOID fbv_isp_proc(FH_VOID* args) {
    ISP_INFO* isp_info = (ISP_INFO*)args;
    // Allen-D
    FH_SINT32 grp_id = isp_info->grpid;
    isp_info->pause = 1;
    isp_info->resume = 1;
    while (1) {
        // API_ISP_Run(isp_info->grpid);
        if (g_isp_info[grp_id].pause && g_isp_info[grp_id].resume) {
            rt_kprintf("[DEBUG] Start ISP strategy, initialize\n");
            usleep(100 * 1000);
            API_ISP_Run(isp_info->grpid);
            g_isp_info[grp_id].pause = 0;
            g_isp_info[grp_id].resume = 0;
        } else if (!g_isp_info[grp_id].pause) {
            API_ISP_Run(isp_info->grpid);
        }
        // else
        //{
        //     rt_kprintf("[DEBUG] Stop ISP strategy, pause = %d, resume = %d\n",
        //     g_isp_info[grp_id].pause,
        //     g_isp_info[grp_id].resume);
        // }
    }
}

// Allen-D Add for deinit arc
void stop_isp_proc(void) {
    FH_SINT32 grp_id = 0;

    for (grp_id = 0; grp_id < MAX_GRP_NUM; grp_id++) {
        if (g_thread_isp[grp_id] != RT_NULL) {
            // 挂起线程
            rt_thread_suspend(g_thread_isp[grp_id]);

            // 删除线程
            rt_thread_delete(g_thread_isp[grp_id]);

            g_thread_isp[grp_id] = RT_NULL;

            rt_kprintf("ISP thread %d stopped\n", grp_id);
        }
    }

    rt_kprintf("All ISP threads stopped\n");
}

FH_VOID fbv_isp_start_proc(FH_VOID) {
    rt_thread_t thread_isp[MAX_GRP_NUM];
    FH_SINT32 grp_id = 0;
    /*
     * ISP策略线程
     */
    for (grp_id = 0; grp_id < MAX_GRP_NUM; grp_id++) {
        if (g_isp_info[grp_id].enable) {
            // thread_isp[grp_id] = rt_thread_create("fast_isp_thrd", fbv_isp_proc, &g_isp_info[grp_id],
            //                                       4 * 1024 - 64, FH_DEMO_THREAD_PRIORITY, 10);
            g_thread_isp[grp_id] = rt_thread_create("fast_isp_thrd", fbv_isp_proc, &g_isp_info[grp_id], 4 * 1024 - 64,
                                                    FH_DEMO_THREAD_PRIORITY, 10);
            rt_thread_startup(g_thread_isp[grp_id]);
        }
    }
}

FH_SINT32 fbv_isp_pause(FH_SINT32 grp_id) {
    FH_SINT32 ret = 0;
    ret = API_ISP_Pause(grp_id);
    FBV_FUNC_ERROR_PRT("API_ISP_Pause", ret);
    return ret;
}

FH_SINT32 fbv_isp_resume(FH_SINT32 grp_id) {
    FH_SINT32 ret = 0;
    ret = API_ISP_Resume(grp_id);
    FBV_FUNC_ERROR_PRT("API_ISP_Resume", ret);
    return ret;
}

FH_SINT32 fbv_isp_w(FH_SINT32 grp_id) {
    return g_isp_info[grp_id].isp_init_width;
}

FH_SINT32 fbv_isp_w_pre(FH_SINT32 grp_id) {
    return g_isp_info[grp_id].isp_init_width_pre;
}

FH_SINT32 fbv_isp_h(FH_SINT32 grp_id) {
    return g_isp_info[grp_id].isp_init_height;
}

FH_SINT32 fbv_isp_h_pre(FH_SINT32 grp_id) {
    return g_isp_info[grp_id].isp_init_height_pre;
}

// Allen-D
FH_SINT32 fbv_get_isp_config(FH_SINT32 grp_id) {
    return &g_isp_info[grp_id];
}

FH_SINT32 change_mode(FH_SINT32 grp_id, FH_SINT32 mode) {
    FH_SINT32 ret = 0;
    ISP_INFO* isp_info;
    static FH_SINT32 isp_format_bak;
    rt_kprintf("[DEBUG] Start change mode\n");
    isp_info = fbv_get_isp_config(grp_id);
    if (!isp_info->enable) {
        rt_kprintf("changeIspResolution Failed! ISP[%d] not enable!\n", grp_id);
        return -1;
    }

    isp_format_bak = isp_info->isp_format;

    switch (mode) {
    case FORMAT_CHANGE_400WP30:
        isp_info->isp_format = FORMAT_400WP30;
        break;
    case FORMAT_CHANGE_400WP25:
        isp_info->isp_format = FORMAT_400WP25;
        break;
    case FORMAT_CHANGE_1080P30:
        isp_info->isp_format = FORMAT_1080P30;
        break;
    case FORMAT_CHANGE_1080P25:
        isp_info->isp_format = FORMAT_1080P25;
        break;
    case FORMAT_CHANGE_720P30:
        isp_info->isp_format = FORMAT_720P30;
        break;
    case FORMAT_CHANGE_720P25:
        isp_info->isp_format = FORMAT_720P25;
        break;
    case FORMAT_CHANGE_320P30:
        isp_info->isp_format = FORMAT_320P30;
        break;
    case FORMAT_CHANGE_320P25:
        isp_info->isp_format = FORMAT_320P25;
        break;
    default:
        break;
    }

    ret = change_isp_format(grp_id, isp_format_bak);
    FBV_FUNC_ERROR_PRT("change_isp_format", ret);
    return ret;
}

FH_SINT32 change_isp_format(FH_SINT32 grp_id, FH_SINT32 isp_format_bak) {
    FH_SINT32 ret = 0;
    FH_SINT32 flag = SAMPLE_SENSOR_FLAG_NORMAL;
    FH_SINT32 hex_file_len;
    // FH_CHAR *param;
    FH_VICAP_VI_ATTR_S stViAttr;
    ISP_VI_ATTR_S ispViAttr;
    unsigned int rst_delay;
    ISP_SENSOR_COMMON_CMD_DATA0 fsParam;
    char* isp_param;
    ENC_INFO* venc_info;
    FH_VENC_CHN_CONFIG cfg_param;
    FH_VENC_CHN_CAP cfg_vencmem;

    rt_memset(&cfg_param, 0, sizeof(FH_VENC_CHN_CONFIG));
    rt_memset(&cfg_vencmem, 0, sizeof(FH_VENC_CHN_CAP));

    venc_info = get_venc_config(grp_id, 0);

    rt_kprintf("[DEBUG] Start change isp format from 0x%x to 0x%x\n", isp_format_bak, g_isp_info[grp_id].isp_format);

    if (!g_isp_info[grp_id].enable) {
        rt_kprintf("change_isp_format Failed, ISP[%d] not enable!\n", grp_id);
        return -1;
    }

    // isp_strategy_pause(grp_id);
    // g_isp_info[grp_id].pause = 1;

    ret = FH_VICAP_Pause(grp_id);
    FBV_FUNC_ERROR_PRT("FH_VICAP_Pause", ret);

    // 停止isp
    ret = API_ISP_Pause(grp_id);
    FBV_FUNC_ERROR_PRT("API_ISP_Pause", ret);

    // reset_sensor(grp_id);
    rst_delay = 100000;
    sensor_reset(grp_id, rst_delay);

    ret = change_sensor_slaveaddr(grp_id, g_isp_info[grp_id].sensor_name);
    FBV_FUNC_ERROR_PRT("change_sensor_slaveaddr", ret);

    ret = vpu_grp_disable(grp_id);
    FBV_FUNC_ERROR_PRT("vpu_grp_disable", ret);

    ret = API_ISP_SetSensorFmt(grp_id, g_isp_info[grp_id].isp_format);
    if (ret) {
        rt_kprintf("Setting ISP[%d] back to Format:0x%x! ret = %x\n", grp_id, isp_format_bak, ret);
        g_isp_info[grp_id].isp_format = isp_format_bak;
        ret = API_ISP_SetSensorFmt(grp_id, g_isp_info[grp_id].isp_format);
        FBV_FUNC_ERROR_PRT("API_ISP_SetSensorFmt", ret);
    }

    // ret = API_ISP_SensorKick(grp_id);
    // FBV_FUNC_ERROR_PRT("API_ISP_SensorKick", ret);

    ret = API_ISP_GetViAttr(grp_id, &ispViAttr);
    FBV_FUNC_ERROR_PRT("API_ISP_GetViAttr", ret);

    g_isp_info[grp_id].isp_init_width = ispViAttr.u16PicWidth;
    g_isp_info[grp_id].isp_init_height = ispViAttr.u16PicHeight;

    ret = FH_VICAP_GetViAttr(grp_id, &stViAttr);
    FBV_FUNC_ERROR_PRT("FH_VICAP_GetViAttr", ret);

    stViAttr.stInSize.u16Width = ispViAttr.u16InputWidth;
    stViAttr.stInSize.u16Height = ispViAttr.u16InputHeight;
    stViAttr.stCropSize.stRect.u16Width = ispViAttr.u16PicWidth;
    stViAttr.stCropSize.stRect.u16Height = ispViAttr.u16PicHeight;
    stViAttr.stCropSize.stRect.u16X = ispViAttr.u16OffsetX;
    stViAttr.stCropSize.stRect.u16Y = ispViAttr.u16OffsetY;
    stViAttr.u8WdrMode = get_sensorFormat_wdr_mode(g_isp_info[grp_id].isp_format);
    ret = FH_VICAP_SetViAttr(grp_id, &stViAttr);
    FBV_FUNC_ERROR_PRT("FH_VICAP_SetViAttr", ret);

    ret = vpu_grp_set_vi_attr(grp_id, ispViAttr.u16PicWidth, ispViAttr.u16PicHeight);
    FBV_FUNC_ERROR_PRT("vpu_grp_set_vi_attr", ret);

    venc_info->width = ispViAttr.u16PicWidth;
    venc_info->height = ispViAttr.u16PicHeight;

    // ret = enc_set_chn_buffer(0);
    cfg_param.chn_attr.enc_type = venc_info->enc_type;
    cfg_param.chn_attr.h264_attr.profile = H264_PROFILE_MAIN;
    cfg_param.chn_attr.h264_attr.i_frame_intterval = 50;
    cfg_param.chn_attr.h264_attr.size.u32Width = venc_info->width;
    cfg_param.chn_attr.h264_attr.size.u32Height = venc_info->height;

    cfg_param.rc_attr.rc_type = FH_RC_H264_VBR;
    cfg_param.rc_attr.h264_vbr.init_qp = 35;
    cfg_param.rc_attr.h264_vbr.bitrate = venc_info->bps;
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
    cfg_param.rc_attr.h264_vbr.FrameRate.frame_count = venc_info->frame_count;
    cfg_param.rc_attr.h264_vbr.FrameRate.frame_time = venc_info->frame_time;

    FH_BIND_INFO src, dst;

    src.obj_id = FH_OBJ_VPU_VO;
    src.dev_id = grp_id;
    src.chn_id = 0;

    dst.obj_id = FH_OBJ_ENC;
    dst.dev_id = 0;
    dst.chn_id = 0;

    ret = FH_SYS_UnBindbySrc(src);

    ret = FH_VENC_StopRecvPic(0);
    FBV_FUNC_ERROR_PRT("FH_VENC_StopRecvPic", ret);

    ret = FH_VENC_DestroyChn(0);
    FBV_FUNC_ERROR_PRT("FH_VENC_DestroyChn", ret);

    cfg_vencmem.support_type = venc_info->enc_type;
    cfg_vencmem.max_size.u32Width = 2560;
    cfg_vencmem.max_size.u32Height = 1440;
    cfg_vencmem.work_mode = FH_NORMAL_MODE;

    ret = FH_VENC_CreateChn(0, &cfg_vencmem);
    FBV_FUNC_ERROR_PRT("FH_VENC_CreateChn", ret);

    ret = FH_VENC_SetChnAttr(0, &cfg_param);
    FBV_FUNC_ERROR_PRT("FH_VENC_SetChnAttr", ret);

    ret = FH_VENC_StartRecvPic(0);
    FBV_FUNC_ERROR_PRT("FH_VENC_StartRecvPic", ret);

    ret = FH_SYS_Bind(src, dst);
    FBV_FUNC_ERROR_PRT("FH_SYS_Bind", ret);

    ret = vpu_grp_enable(grp_id);
    FBV_FUNC_ERROR_PRT("vpu_grp_enable", ret);

    if ((g_isp_info[grp_id].isp_format >> 16) & 1) {
        flag |= SAMPLE_SENSOR_FLAG_WDR;
    }

    // param = getSensorHex(grp_id, (FH_CHAR *)g_isp_info[grp_id].sensor->name, flag, &hex_file_len);
    // load hex file
    if (get_sensor_param(g_isp_info[grp_id].sensor_name, &isp_param, 0)) {
        rt_kprintf("get_sensor_param failed %s\n", g_isp_info[grp_id].sensor_name);
        return -1;
    }
    if (isp_param) {
        ret = API_ISP_LoadIspParam(grp_id, isp_param);
        FBV_FUNC_ERROR_PRT("API_ISP_LoadIspParam", ret);
        freeSensorHex(isp_param);
    } else {
        rt_kprintf("Error: Cann't load sensor hex file!\n");
    }

    // ret = isp_change_resolution(grp_id);

    /*ret = API_ISP_SetAeInfo(grp_id, &g_pstAeInfo);
    if (ret != 0)
    {
        rt_kprintf("[DEBUG] set ae info failed\n");
    }*/
    ret = API_ISP_SetSensorIntt(grp_id, g_newIntt);
    FBV_FUNC_ERROR_PRT("API_ISP_SetSensorIntt", ret);
    ret = API_ISP_SetSensorGain(grp_id, g_newGain);
    FBV_FUNC_ERROR_PRT("API_ISP_SetSensorGain", ret);

    rt_kprintf("[AE-DEBUG] set new init = %d, new gain = %d\n", g_newIntt, g_newGain);

    udelay(20 * 1000);
    rt_kprintf("[AE-DEBUG] delay 10 ms before sensor kick\n");
    ret = API_ISP_SensorKick(grp_id);
    FBV_FUNC_ERROR_PRT("API_ISP_SensorKick", ret);

    //恢复isp
    ret = API_ISP_Resume(grp_id);
    FBV_FUNC_ERROR_PRT("API_ISP_Resume", ret);

    // isp_strategy_resume(grp_id);
    g_isp_info[grp_id].resume = 1;
    rt_kprintf("[DEBUG] resume after change format = %d\n", g_isp_info[grp_id].resume);

    rt_kprintf("ISP[%d] Change Format:0x%x!\n", grp_id, g_isp_info[grp_id].isp_format);

    return ret;
}

FH_VOID freeSensorHex(FH_CHAR* param) {
#ifdef __LINUX_OS__
    if (param) {
        free(param);
    }
#elif defined __RTTHREAD_OS__
#endif
}

FH_SINT32 vpu_grp_set_vi_attr(FH_SINT32 grp_id, FH_SINT32 w, FH_SINT32 h) {
    FH_SINT32 ret = 0;
    FH_VPU_SIZE vi_pic;
    FH_VPU_CHN_CONFIG chn_attr;

    chn_attr.vpu_chn_size.u32Width = w;
    chn_attr.vpu_chn_size.u32Height = h;
    chn_attr.crop_area.crop_en = 0;
    chn_attr.crop_area.vpu_crop_area.u32X = 0;
    chn_attr.crop_area.vpu_crop_area.u32Y = 0;
    chn_attr.crop_area.vpu_crop_area.u32Width = 0;
    chn_attr.crop_area.vpu_crop_area.u32Height = 0;
    chn_attr.stride = 0;
    chn_attr.offset = 0;
    chn_attr.depth = 1;

    ret = FH_VPSS_GetViAttr(grp_id, &vi_pic);
    FBV_FUNC_ERROR_PRT("FH_VPSS_GetViAttr", ret);

    vi_pic.vi_size.u32Width = w;
    vi_pic.vi_size.u32Height = h;

    ret = FH_VPSS_SetViAttr(grp_id, &vi_pic);
    FBV_FUNC_ERROR_PRT("FH_VPSS_SetViAttr", ret);

    ret = FH_VPSS_SetChnAttr(grp_id, 0, &chn_attr);
    FBV_FUNC_ERROR_PRT("FH_VPSS_SetViAttr", ret);

    ret = FH_VPSS_SetVOMode(grp_id, 0, g_vpu_chn_info[grp_id][0].yuv_type);
    FBV_FUNC_ERROR_PRT("FH_VPSS_SetVOMode", ret);

    ret = FH_VPSS_OpenChn(grp_id, 0);
    FBV_FUNC_ERROR_PRT("FH_VPSS_OpenChn", ret);

    return ret;
}