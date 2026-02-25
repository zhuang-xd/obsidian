
#include "fbv_common.h"
#include "FHAdv_MD_mpi.h"

#define MD_CD_CHECK_INTERVAL (1) /*25毫秒检测一次,FHAdv_MD_CD_Check函数同时检测MD/MD_EX/CD, \
                                     如果用户对于MD/MD_EX/CD配置了不同的检测间隔,        \
                                     advapi内部会取他们的最小检测间隔*/

static FH_UINT32 handle_md_ex_result(FHT_MDConfig_Ex_Result_t *result)
{
    FH_SINT32 i;
    FH_SINT32 j;
    FH_UINT8 motion_flag;
    FH_UINT32 motion_pix = 0;
    for (i = 0; i < result->vertical_count; i++)
    {
        for (j = 0; j < result->horizontal_count; j++)
        {
            motion_flag = *(result->start + i * result->horizontal_count + j);
            if(motion_flag > 0)
            {
                motion_pix++;
            }
        }
    }
    // if(motion_pix > 50)
    // {
        return motion_pix;
    // } else
    //     return 0;
}

static FH_UINT32 sample_md_ex_get_result(FH_SINT32 grpid)
{
    FH_SINT32 ret;
    FHT_MDConfig_Ex_Result_t result;

    ret = FHAdv_MD_Ex_GetResult(grpid, &result); /* 获取设置全局运动检测结果 */
    if (ret == FH_SUCCESS)
    {
        ret = handle_md_ex_result(&result);
        rt_kprintf("\n[group %d]=======detected move[%d]===========\n", grpid, ret);
        return ret;
    }
    else
    {
        rt_kprintf("\nError: FHAdv_MD_Ex_GetResult failed, ret=%d\n", ret);
    }
    return 0;
}

FH_UINT32 fbv_md_cd_get_result(FH_UINT32 grpid)
{
    int ret = 0;

    ret = FHAdv_MD_CD_Check(grpid); /* 做一次运动、遮挡检测 */
    if (ret)
    {
        rt_kprintf("[GROUP %d][ERROR]: FHAdv_MD_CD_Check failed, ret=%x\n", grpid, ret);
        return ret;
    }
    return sample_md_ex_get_result(grpid);
}

static FH_SINT32 sample_md_ex_init(FH_SINT32 grpid)
{
    FH_SINT32 ret;

    FHT_MDConfig_Ex_t md_config;

    ret = FHAdv_MD_Ex_Init(grpid);
    if (ret != 0)
    {
        rt_kprintf("[ERROR]: FHAdv_MD_Ex_Init failed, ret=%d\n", ret);
        return ret;
    }

    md_config.threshold = 128;                   /* 运动检测阈值，该值越大，检测灵敏度越高 */
    md_config.framedelay = MD_CD_CHECK_INTERVAL; /* 设置检测帧间隔 | 时间间隔，默认使用帧间隔，可以调用FHAdv_MD_Set_Check_Mode设置使用时间间隔 */
    md_config.enable = 1;

    /* 设置区域检测配置 */
    ret = FHAdv_MD_Ex_SetConfig(grpid, &md_config);
    if (ret != 0)
    {
        rt_kprintf("[ERROR]: FHAdv_MD_Ex_SetConfig failed, ret=%d\n", ret);
        return ret;
    }

    return 0;
}

FH_SINT32 fbv_md_cd_init(FH_SINT32 grpid)
{
    FH_SINT32 ret;

    ret = sample_md_ex_init(grpid);
    if (ret == 0)
    {
        FHAdv_MD_Set_Check_Mode(grpid, FH_MD_TIME_CHEAK); /*通过调用此函数配置为使用时间间隔,如果不调用,就是缺省的帧间隔*/
    }

    return ret;
}
