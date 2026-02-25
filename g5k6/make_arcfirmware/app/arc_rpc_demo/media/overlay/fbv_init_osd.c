

#include <rtthread.h>
#include "types/bufCtrl.h"
#include "logo_array.h"
#include "font_array.h"

#include "FHAdv_OSD_mpi.h"

#define OSD_FONT_DISP_SIZE (32)
#define SAMPLE_LOGO_GRAPH FHT_OSD_GRAPH_CTRL_LOGO_BEFORE_VP /* 设置LOGO在VPU分通道前的GOSD层 */
#define SAMPLE_TOSD_GRAPH FHT_OSD_GRAPH_CTRL_TOSD_AFTER_VP  /* 设置TOSD在VPU分通道前的GOSD层 */
#define SAMPLE_GBOX_GRAPH FHT_OSD_GRAPH_CTRL_GBOX_AFTER_VP  /* 设置GBOX在VPU分通道前的GOSD层 */
#define SAMPLE_MASK_GRAPH FHT_OSD_GRAPH_CTRL_MASK_AFTER_VP  /* 设置MASK在VPU分通道前的GOSD层 */

FH_SINT32 sample_set_gbox(FH_SINT32 grp_id, FH_UINT32 chan, FH_UINT32 enable, FH_UINT32 box_id, FH_UINT32 x, FH_UINT32 y, FH_UINT32 w, FH_UINT32 h, FHT_RgbColor_t color)
{
    FH_SINT32 ret;
    FHT_OSD_Gbox_t gbox_cfg;

    rt_memset(&gbox_cfg, 0, sizeof(gbox_cfg));

    /* 配置gbox 0 */
    gbox_cfg.enable = enable; /* 使能gbox显示 */
    gbox_cfg.gboxId = box_id; /* gbox ID 为0 */

    if (enable)
    {
        gbox_cfg.rotate = 0;         /* 不旋转 */
        gbox_cfg.gboxLineWidth = 2;  /* gbox边框像素宽度为2*/
        gbox_cfg.area.fTopLeftX = x; /* gbox左上角起始点像素宽度位置 */
        gbox_cfg.area.fTopLeftY = y; /* gbox左上角起始点像素高度位置 */
        gbox_cfg.area.fWidth = w;    /* gbox宽度 */
        gbox_cfg.area.fHeigh = h;    /* gbox高度 */
        gbox_cfg.osdColor.fAlpha = color.fAlpha;
        gbox_cfg.osdColor.fRed = color.fRed;
        gbox_cfg.osdColor.fGreen = color.fGreen;
        gbox_cfg.osdColor.fBlue = color.fBlue;
    }
    else /*just work-around FHAdv_Osd_Ex_SetGbox,it will check fWidth and fHeight*/
    {
        gbox_cfg.gboxLineWidth = 2;
        gbox_cfg.area.fTopLeftX = 0;
        gbox_cfg.area.fTopLeftY = 0;
        gbox_cfg.area.fWidth = 16;
        gbox_cfg.area.fHeigh = 16;
        gbox_cfg.osdColor.fAlpha = color.fAlpha;
        gbox_cfg.osdColor.fRed = color.fRed;
        gbox_cfg.osdColor.fGreen = color.fGreen;
        gbox_cfg.osdColor.fBlue = color.fBlue;
    }

    ret = FHAdv_Osd_Ex_SetGbox(grp_id, chan, &gbox_cfg);
    if (ret != FH_SUCCESS)
    {
        rt_kprintf("FHAdv_Osd_SetGbox failed with %x\n", ret);
        return ret;
    }
    return 0;
}

FH_SINT32 fbv_osd_init(FH_SINT32 grp_id)
{
    FH_SINT32 ret;
    FH_UINT32 graph_ctrl = 0;
    // FH_VPU_CHN_CONFIG box_chan_cfg;

    graph_ctrl |= SAMPLE_TOSD_GRAPH;
    graph_ctrl |= SAMPLE_LOGO_GRAPH;
    graph_ctrl |= SAMPLE_GBOX_GRAPH;
    FHAdv_Osd_Config_GboxPixel(grp_id, 4, 0);
    /* 从epoch到当前时间经历的秒数 + 8小时，必须在FHAdv_Osd_Init之前调用 */
    extern void arc_settimeofday(int second); /* 在librtthread-arc.a中实现 */
    arc_settimeofday(1645508326 + 8 * 60 * 60);

    /* 初始化OSD */
    ret = FHAdv_Osd_Init(grp_id, FHT_OSD_DEBUG_LEVEL_ERROR, graph_ctrl, 0, 0);
    if (ret != FH_SUCCESS)
    {
        rt_kprintf("FHAdv_Osd_Init failed with %x\n", ret);
        return ret;
    }

    FHT_OSD_FontLib_t font_lib;

    font_lib.pLibData = asc16;
    font_lib.libSize = sizeof(asc16);
    ret = FHAdv_Osd_LoadFontLib(FHEN_FONT_TYPE_ASC, &font_lib);
    if (ret != 0)
    {
        rt_kprintf("Error: FHAdv_Osd_LoadFontLib failed, ret=%d\n", ret);
        return ret;
    }

    FHT_OSD_CONFIG_t osd_cfg;
    FHT_OSD_Layer_Config_t pOsdLayerInfo[1];
    FHT_OSD_TextLine_t text_line_cfg[1];
    FH_CHAR text_data[128]; /*it should be enough*/

    rt_memset(&osd_cfg, 0, sizeof(osd_cfg));
    rt_memset(&pOsdLayerInfo[0], 0, sizeof(FHT_OSD_Layer_Config_t));
    rt_memset(&text_line_cfg[0], 0, sizeof(FHT_OSD_TextLine_t));
    rt_memset(&text_data, 0, sizeof(text_data));

    /* 不旋转 */
    osd_cfg.osdRotate = 0;
    osd_cfg.pOsdLayerInfo = &pOsdLayerInfo[0];
    /* 设置text行块个数 */
    osd_cfg.nOsdLayerNum = 1; /*我们的demo中只演示了一个行块*/

    pOsdLayerInfo[0].layerStartX = 0;
    pOsdLayerInfo[0].layerStartY = 0;
    /* pOsdLayerInfo[0].layerMaxWidth = 640; */ /*根据需求配置，如果设置则以设置的最大值配置内存，如果缺省则以实际幅面大小申请内存*/
    /* pOsdLayerInfo[0].layerMaxHeight = 480; */
    /* 设置字符大小,像素单位 */
    pOsdLayerInfo[0].osdSize = OSD_FONT_DISP_SIZE;

    /* 设置字符颜色为白色 */
    pOsdLayerInfo[0].normalColor.fAlpha = 255;
    pOsdLayerInfo[0].normalColor.fRed = 255;
    pOsdLayerInfo[0].normalColor.fGreen = 255;
    pOsdLayerInfo[0].normalColor.fBlue = 255;

    /* 设置字符反色颜色为黑色 */
    pOsdLayerInfo[0].invertColor.fAlpha = 255;
    pOsdLayerInfo[0].invertColor.fRed = 0;
    pOsdLayerInfo[0].invertColor.fGreen = 0;
    pOsdLayerInfo[0].invertColor.fBlue = 0;

    /* 设置字符钩边颜色为黑色 */
    pOsdLayerInfo[0].edgeColor.fAlpha = 255;
    pOsdLayerInfo[0].edgeColor.fRed = 0;
    pOsdLayerInfo[0].edgeColor.fGreen = 0;
    pOsdLayerInfo[0].edgeColor.fBlue = 0;

    /* 不显示背景 */
    pOsdLayerInfo[0].bkgColor.fAlpha = 0;

    /* 钩边像素为1 */
    pOsdLayerInfo[0].edgePixel = 1;

    /* 反色控制 */
    pOsdLayerInfo[0].osdInvertEnable = FH_OSD_INVERT_DISABLE; /*disable反色功能*/
    // pOsdLayerInfo[0].osdInvertEnable  = FH_OSD_INVERT_BY_CHAR; /*以字符为单位进行反色控制*/
    /*osd_cfg.osdInvertEnable  = FH_OSD_INVERT_BY_LINE;*/ /*以行块为单位进行反色控制*/
    pOsdLayerInfo[0].osdInvertThreshold.high_level = 180;
    pOsdLayerInfo[0].osdInvertThreshold.low_level = 160;
    pOsdLayerInfo[0].layerFlag = FH_OSD_LAYER_USE_TWO_BUF;
    pOsdLayerInfo[0].layerId = 0;

    ret = FHAdv_Osd_Ex_SetText(grp_id, 0, &osd_cfg);
    if (ret != FH_SUCCESS)
    {
        rt_kprintf("FHAdv_Osd_Ex_SetText failed with %d\n", ret);
        return ret;
    }

    FHT_OSD_TextLine_t *text_line;
    text_line = &text_line_cfg[0];

    text_line->timeOsdEnable = 1; /* 使能时间显示 */

    /* 12小时制式、显示星期、时间在自定义字符后面显示 */
    text_line->timeOsdFlags = FHT_OSD_TIME_BIT_HOUR12 | FHT_OSD_TIME_BIT_WEEK | FHT_OSD_TIME_BIT_AFTER;

    text_line->timeOsdFormat = FHTEN_OSD_TimeFmt0; /* 使用时间格式FHTEN_OSD_TimeFmt0  YYYY-MM-DD 00:00:00  */

    text_line->textLineWidth = (OSD_FONT_DISP_SIZE / 2) * 36; /* 每行最多显示36个像素宽度为16的字符, 超过后自动换行 */
    text_line->linePositionX = 0;                             /* 左上角起始点水平方向像素位置 */
    text_line->linePositionY = 0;                             /* 左上角起始点垂直方向像素位置 */
    text_line->lineId = 0;
    text_line->enable = 1;

    ret = FHAdv_Osd_SetTextLine(grp_id, 0, pOsdLayerInfo[0].layerId, &text_line_cfg[0]);
    if (ret != FH_SUCCESS)
    {

        rt_kprintf("FHAdv_Osd_Ex_SetTextLine failed with %d\n", ret);
        return ret;
    }

    FHT_OSD_Logo_t logo_cfg;

    rt_memset(&logo_cfg, 0, sizeof(logo_cfg));

    logo_cfg.enable = 1;                        /* 使能logo显示 */
    logo_cfg.rotate = 0;                        /* 不旋转 */
    logo_cfg.maxW = AUTO_GEN_PIC_WIDTH;         /* 最大logo像素宽度，用于第一次设置内存分配 */
    logo_cfg.maxH = AUTO_GEN_PIC_HEIGHT;        /* 最大logo像素高度，用于第一次设置内存分配 */
    logo_cfg.alpha = 255;                       /* 不透明 */
    logo_cfg.area.fTopLeftX = 0;                /* logo左上角起始点像素宽度位置 */
    logo_cfg.area.fTopLeftY = 64;               /* logo左上角起始点像素高度位置 */
    logo_cfg.area.fWidth = AUTO_GEN_PIC_WIDTH;  /* logo实际像素宽度 */
    logo_cfg.area.fHeigh = AUTO_GEN_PIC_HEIGHT; /* logo实际像素高度 */
    logo_cfg.pData = logo_data;                 /* logo数据 */

    ret = FHAdv_Osd_SetLogo(grp_id, &logo_cfg);
    if (ret != FH_SUCCESS)
    {
        rt_kprintf("FHAdv_Osd_SetLogo failed with %x\n", ret);
        return ret;
    }

    return 0;
}
