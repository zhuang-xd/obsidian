#include "fbv_common.h"
#include <asm/io.h>
#include <fh_chip.h>
#include <delay.h>
#include <gpio.h>

extern struct isp_sensor_if *Sensor_Create_ps5270_mipi(void);
extern struct isp_sensor_if *Sensor_Create_ovos02k_mipi(void);
extern struct isp_sensor_if *Sensor_Create_sc3336_mipi(void);
extern struct isp_sensor_if *Sensor_Create_ovos04c10_mipi(void);
extern struct isp_sensor_if *Sensor_Create_ov04c10_mipi(void);
extern struct isp_sensor_if *Sensor_Create_ovos08_mipi(void);
extern struct isp_sensor_if *Sensor_Create_gc2083_mipi(void);

struct param_list sensor_param_list[] = {
#ifdef FH_USING_OVOS02K_MIPI
    {
        .sensor_name = "ovos02k_mipi",
        .day_param = isp_param_buff_OVOS02K_MIPI,
        .night_param = RT_NULL,
        .wdr_param = RT_NULL,
        .night_wdr_param = RT_NULL,
    },
#endif
#ifdef FH_USING_PS5270_MIPI
    {
        .sensor_name = "ps5270_mipi",
        .day_param = isp_param_buff_PS5270_MIPI,
        .night_param = RT_NULL,
        .wdr_param = RT_NULL,
        .night_wdr_param = RT_NULL,
    },
#endif
#ifdef FH_USING_OVOS04C10_MIPI
    {
        .sensor_name = "ovos04c10_mipi",
        .day_param = isp_param_buff_OVOS04C10_MIPI,
        .night_param = RT_NULL,
        .wdr_param = RT_NULL,
        .night_wdr_param = RT_NULL,
    },
#endif
#ifdef FH_USING_OV04C10_MIPI
    {
        .sensor_name = "ov04c10_mipi",
        .day_param = isp_param_buff_OV04C10_MIPI,
        .night_param = RT_NULL,
        .wdr_param = RT_NULL,
        .night_wdr_param = RT_NULL,
    },
#endif
#ifdef FH_USING_OVOS08_MIPI
    {
        .sensor_name = "ovos08_mipi",
        .day_param = isp_param_buff_OVOS08_MIPI,
        .night_param = RT_NULL,
        .wdr_param = RT_NULL,
        .night_wdr_param = RT_NULL,
    },
#endif
#ifdef FH_USING_SC3336_MIPI
    {
        .sensor_name = "sc3336_mipi",
        .day_param = isp_param_buff_SC3336_MIPI,
        .night_param = RT_NULL,
        .wdr_param = RT_NULL,
        .night_wdr_param = RT_NULL,
    },
#endif
#ifdef FH_USING_GC2083_MIPI
    {
        .sensor_name = "gc2083_mipi",
        .day_param = isp_param_buff_GC2083_MIPI,
        .night_param = RT_NULL,
        .wdr_param = RT_NULL,
        .night_wdr_param = RT_NULL,
    },
#endif
};

struct isp_sensor_if* Sensor_GetPtr(FH_UINT32 u32DevId)
{
    return g_isp_info[u32DevId].sensor;
}

struct isp_sensor_if *Sensor_Create(char *name)
{
#ifdef FH_USING_PS5270_MIPI
    if (rt_strcmp(name, "ps5270_mipi") == 0)
    {
        return Sensor_Create_ps5270_mipi();
    }
#endif
#ifdef FH_USING_OVOS02K_MIPI
    if (rt_strcmp(name, "ovos02k_mipi") == 0)
    {
        return Sensor_Create_ovos02k_mipi();
    }
#endif
#ifdef FH_USING_SC3336_MIPI
    if (rt_strcmp(name, "sc3336_mipi") == 0)
    {
        return Sensor_Create_sc3336_mipi();
    }
#endif
#ifdef FH_USING_OVOS04C10_MIPI
    if (rt_strcmp(name, "ovos04c10_mipi") == 0)
    {
        return Sensor_Create_ovos04c10_mipi();
    }
#endif
#ifdef FH_USING_OV04C10_MIPI
    if (rt_strcmp(name, "ov04c10_mipi") == 0)
    {
        return Sensor_Create_ov04c10_mipi();
    }
#endif
#ifdef FH_USING_OVOS08_MIPI
    if (rt_strcmp(name, "ovos08_mipi") == 0)
    {
        return Sensor_Create_ovos08_mipi();
    }
#endif
#ifdef FH_USING_GC2083_MIPI
    if (rt_strcmp(name, "gc2083_mipi") == 0)
    {
        return Sensor_Create_gc2083_mipi();
    }
#endif
    return RT_NULL;
}

FH_SINT32 get_sensor_param(char *sensor_name, char **param, unsigned int flag)
{
    FH_SINT32 sensor_num;
    FH_SINT32 i;

    sensor_num = sizeof(sensor_param_list) / sizeof(sensor_param_list[0]);

    for (i = 0; i < sensor_num; i++)
    {
        if (rt_strcmp(sensor_name, sensor_param_list->sensor_name))
        {
            continue;
        }

        if (flag == PARAM_NIGHT)
        {
            *param = sensor_param_list[i].night_param;
            return 0;
        }
        else if (flag == PARAM_WDR)
        {
            *param = sensor_param_list[i].wdr_param;
            return 0;
        }
        else if (flag == PARAM_WDR_NIGHT)
        {
            *param = sensor_param_list[i].night_wdr_param;
            return 0;
        }
        else
        {
            *param = sensor_param_list[i].day_param;
            return 0;
        }
    }

    *param = RT_NULL;
    return -1;
}

int get_sensor_reset_pin(int grp_id)
{
    if (grp_id == 0)
        return SENSOR_REST_GPIO0;
    else if (grp_id == 1)
        return SENSOR_REST_GPIO1;
    else
        return SENSOR_REST_GPIO2;
}

void sensor_reset(int grp_id, unsigned long rst_delay)
{
    int gpio = get_sensor_reset_pin(grp_id);

    if ((rst_delay & 0xffff) > 0)
    {
        gpio_set_value(gpio, 0);
        rt_kprintf("[DEBUG] Set gpio %d to 0\n", gpio);
        udelay(rst_delay & 0xffff);
        rt_kprintf("[DEBUG] Set gpio %d to 1\n", gpio);
        gpio_set_value(gpio, 1);
    }

    if ((rst_delay >> 16) > 0)
    {
        udelay(rst_delay >> 16);
    }
}

void sensor_enter_reset(int grp_id)
{
    static int init = 0;

    //切sensor reset引脚的pinmux
    if (!init)
    {
        SET_REG_M(CEN_PIN_REG_BASE + REG_PMU_PAD_SENSOR_RSTN_CFG, 1, 0x7);

        SET_REG_M(CEN_PIN_REG_BASE + REG_PMU_PAD_SENSOR2_RSTN_CFG, 1, 0x7);

        SET_REG_M(CEN_PIN_REG_BASE + REG_PMU_PAD_SENSOR3_RSTN_CFG, 1, 0x7);

        init = 1;
    }
    int gpio = get_sensor_reset_pin(grp_id);

    /*let sensor start reset...*/
    gpio_direction_output(gpio, 1);
    gpio_set_value(gpio, 0);
}

void sensor_exit_reset(int grp_id)
{
    int gpio = get_sensor_reset_pin(grp_id);

    /*now sensor will start to run...*/
    gpio_set_value(gpio, 1);
}

int change_sensor_slaveaddr(int grp_id, char *sensor_name)
{
    int ret = 0;

#ifdef CONFIG_SENSOR_SLAVE_MODE
    if (!rt_strcmp(sensor_name, "sc3336_mipi")) // 单双路的从机地址设置,目前sc3336支持
    {
        if (grp_id == 0)
        {
            ret = API_ISP_ChangeSnsSlaveAddr(grp_id, 0x64);
            FBV_FUNC_ERROR_PRT("API_ISP_ChangeSnsSlaveAddr", ret);
        }
        else if (grp_id == 1)
        {
            ret = API_ISP_ChangeSnsSlaveAddr(grp_id, 0x68);
            FBV_FUNC_ERROR_PRT("API_ISP_ChangeSnsSlaveAddr", ret);
        }
        rt_kprintf("Set Sensor[%d] slave addr!\n", grp_id);
    }
    else if (!rt_strcmp(sensor_name, "gc2083_mipi"))
    {
        // mipi 0接两个sensor 需要对修改地址为 0x60
        if (grp_id == 0)
        {
            ret = API_ISP_ChangeSnsSlaveAddr(grp_id, 0x60);
            FBV_FUNC_ERROR_PRT("API_ISP_ChangeSnsSlaveAddr", ret);
        }
        // else if (grp_id == 1)
        // {
        //     ret = API_ISP_ChangeSnsSlaveAddr(grp_id, 0x7e);
        //     FBV_FUNC_ERROR_PRT("API_ISP_ChangeSnsSlaveAddr", ret);
        // }
        // else if (grp_id == 2)
        // {
        //     ret = API_ISP_ChangeSnsSlaveAddr(grp_id, 0x6e);
        //     FBV_FUNC_ERROR_PRT("API_ISP_ChangeSnsSlaveAddr", ret);
        // }
        rt_kprintf("Set Sensor[%d] slave addr!\n", grp_id);
    }
#endif

    return ret;
}

FH_SINT32 set_sensor_sync(FH_SINT32 grp_id, FH_UINT16 sensorFps) // 同步信号设置
{

    FH_SINT32 ret = 0;
#ifdef CONIFG_SWITCH_MODE
    struct fh_pwm_chip_data pwm_data;

    rt_memset(&pwm_data, 0, sizeof(struct fh_pwm_chip_data));

    pwm_data.id = grp_id + 2; //enable pwm 2 & 3
    pwm_data.config.period_ns = 82151490;
    pwm_data.config.phase_ns = 0;
    pwm_data.config.pulses = 0;
    pwm_data.config.delay_ns = 0;

    if (pwm_data.id == 2)
    {
        pwm_data.config.duty_ns = pwm_data.config.period_ns / 3;
        pwm_data.config.delay_ns = pwm_data.config.period_ns / 3;
    }
    else if (pwm_data.id == 3)
    {
        pwm_data.config.duty_ns = pwm_data.config.period_ns / 3 * 2;
    }

    fh_pwm_set_config(&pwm_data);

    return ret;
#endif

#ifdef CONFIG_SENSOR_SLAVE_MODE
    FH_VICAP_FRAME_SEQ_CFG_S VicapSync;

    VicapSync.enSyncType = FH_VICAP_HSYNC;
    VicapSync.u16Period = VICAP_HSYNC_TOTAL;
    VicapSync.u16Duty = (VicapSync.u16Period) / 2;
    VicapSync.bPolatiry = 0;

    ret = FH_VICAP_SetFrameSeqGenCfg(grp_id, &VicapSync);
    FBV_FUNC_ERROR_PRT("FH_VICAP_SetFrameSeqGenCfg", ret);

    VicapSync.enSyncType = FH_VICAP_VSYNC;
    VicapSync.u16Period = (VICAP_CLK) / (sensorFps * VICAP_HSYNC_TOTAL * 32) + 1;
    VicapSync.u16Duty = (VicapSync.u16Period) / 2;

    ret = FH_VICAP_SetFrameSeqGenCfg(grp_id, &VicapSync);
    FBV_FUNC_ERROR_PRT("FH_VICAP_SetFrameSeqGenCfg", ret);
    return ret;
#endif
    return ret;
}

FH_SINT32 start_sensor_sync(FH_SINT32 grp_id)
{
    FH_SINT32 ret = 0;

#ifdef CONIFG_SWITCH_MODE
    //同时开启pwm2和pwm3
if(grp_id < MAX_GRP_NUM -1)
    return ret;

    API_ISP_SetSensorReg(0, 0x023e, 0x99);
    API_ISP_SetSensorReg(1, 0x023e, 0x99);
    API_ISP_SetSensorReg(2, 0x023e, 0x99);

    unsigned int pwm_mask = (1 << 2) | (1 << 3);

    fh_pwm_output_multi_enable(pwm_mask);
    return ret;
#endif

#ifdef CONFIG_SENSOR_SLAVE_MODE
    if (grp_id == 1)
    {
        ret = FH_VICAP_SetFrameSeqGenEn(0x3);
        FBV_FUNC_ERROR_PRT("FH_VICAP_SetFrameSeqGenEn", ret);
    }
    return ret;
#else
    // ret = FH_VICAP_SetFrameSeqGenEn(0x1);
    // FBV_FUNC_ERROR_PRT("FH_VICAP_SetFrameSeqGenEn", ret);
    return ret;
#endif
}