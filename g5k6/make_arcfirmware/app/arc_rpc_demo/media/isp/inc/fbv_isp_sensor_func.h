#ifndef __FBV_ISP_SENSOR_FUNC_H__
#define __FBV_ISP_SENSOR_FUNC_H__

#ifdef FH_USING_PS5270_MIPI
extern char isp_param_buff_PS5270_MIPI[];
#define ISP_SENSOR_NAME_G0 "ps5270_mipi"
#define ISP_SENSOR_NAME_G1 "ps5270_mipi"
#define ISP_SENSOR_NAME_G2 "ps5270_mipi"
#endif

#ifdef FH_USING_OVOS02K_MIPI
extern char isp_param_buff_OVOS02K_MIPI[];
#define ISP_SENSOR_NAME_G0 "ovos02k_mipi"
#define ISP_SENSOR_NAME_G1 "ovos02k_mipi"
#define ISP_SENSOR_NAME_G2 "ovos02k_mipi"
#endif

#ifdef FH_USING_SC3336_MIPI
extern char isp_param_buff_SC3336_MIPI[];
#define ISP_SENSOR_NAME_G0 "sc3336_mipi"
#define ISP_SENSOR_NAME_G1 "sc3336_mipi"
#define ISP_SENSOR_NAME_G2 "sc3336_mipi"
#endif

#ifdef FH_USING_OVOS04C10_MIPI
extern char isp_param_buff_OVOS04C10_MIPI[];
#define ISP_SENSOR_NAME_G0 "ovos04c10_mipi"
#define ISP_SENSOR_NAME_G1 "ovos04c10_mipi"
#define ISP_SENSOR_NAME_G2 "ovos04c10_mipi"
#endif

#ifdef FH_USING_OV04C10_MIPI
extern char isp_param_buff_OV04C10_MIPI[];
#define ISP_SENSOR_NAME_G0 "ov04c10_mipi"
#define ISP_SENSOR_NAME_G1 "ov04c10_mipi"
#define ISP_SENSOR_NAME_G2 "ov04c10_mipi"
#endif

#ifdef FH_USING_OVOS08_MIPI
extern char isp_param_buff_OVOS08_MIPI[];
#define ISP_SENSOR_NAME_G0 "ovos08_mipi"
#define ISP_SENSOR_NAME_G1 "ovos08_mipi"
#define ISP_SENSOR_NAME_G2 "ovos08_mipi"
#endif

#ifdef FH_USING_GC2083_MIPI
extern char isp_param_buff_GC2083_MIPI[];
#define ISP_SENSOR_NAME_G0 "gc2083_mipi"
#define ISP_SENSOR_NAME_G1 "gc2083_mipi"
#define ISP_SENSOR_NAME_G2 "gc2083_mipi"
#endif


struct param_list
{
    char sensor_name[50];
    char *day_param;
    char *night_param;
    char *wdr_param;
    char *night_wdr_param;
};


#define PARAM_NORMAL (1 << 0)
#define PARAM_NIGHT (1 << 1)
#define PARAM_WDR (1 << 2)
#define PARAM_WDR_NIGHT (1 << 3)

struct isp_sensor_if *Sensor_Create(char *name);
FH_SINT32 get_sensor_param(char *sensor_name, char **param, unsigned int flag);

#define VICAP_CLK 360000000
#define VICAP_HSYNC_TOTAL 600

#define SENSOR_REST_GPIO0 36
#define SENSOR_REST_GPIO1 40
#define SENSOR_REST_GPIO2 45

void sensor_exit_reset(int grp_id);
int change_sensor_slaveaddr(int grp_id, char *sensor_name);
FH_SINT32 set_sensor_sync(FH_SINT32 grp_id, FH_UINT16 sensorFps);
FH_SINT32 start_sensor_sync(FH_SINT32 grp_id);


#endif /* __FBV_ISP_SENSOR_FUNC_H__ */