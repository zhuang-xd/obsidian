#include "fbv_common.h"
#include "fh_timer.h"
#include <acw_rpc_stub.h>
#include <asm/cacheflush.h>
#include <delay.h>
#include <fh_chip.h>
#include <fh_clock.h>
#include <i2c.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <stdio.h>

FH_SINT32 frame_id = 0;

FH_SINT32 capture_mjpeg(FH_SINT32 grp_id, FH_SINT32 chn_id);
FH_SINT32 capture_jpeg(FH_SINT32 grp_id, FH_SINT32 chn_id);

// Allen-D Add for i2c
static struct rt_i2c_bus_device* i2c_bus = RT_NULL;
#define AW37004_ADDR 0x28
#define REG_CHIP_ID 0x00
#define REG_CLMTRCR 0x01
#define REG_DISCR 0x02
#define REG_DVDD1_VOUT 0x03
#define REG_DVDD2_VOUT 0x04
#define REG_AVDD1_VOUT 0x05
#define REG_AVDD2_VOUT 0x06
#define REG_DVDD_SEQ 0x0A
#define REG_AVDD_SEQ 0x0B
#define REG_ENCR 0x0E
#define REG_SEQCR 0x0F

#define DVDD_VOUT_DEFAULT 0x64  // 1.2V
#define AVDD1_VOUT_DEFAULT 0x80 // 2.8V
#define AVDD2_VOUT_DEFAULT 0x2F // 1.8V
#define I2C_BUS_NAME "i2c1"

typedef struct {
    rt_device_t i2c_bus; // I2C总线设备
    rt_uint8_t dev_addr; // 设备地址
    rt_mutex_t lock;     // 互斥锁
    rt_bool_t initialized;
} aw37004_device_t;
static rt_err_t aw37004_i2c_write_reg(aw37004_device_t* dev, rt_uint8_t reg, rt_uint8_t data);

typedef struct {
    unsigned int version;
    unsigned int max_sections;
    unsigned int sections;
    unsigned int reserved;
    unsigned int DDRSize;
    unsigned int KernelMemSize;
    unsigned int VmmSize;
    unsigned int NBGSize;
    unsigned int UserSize;
    unsigned int NBGAddr;
    unsigned int params[6];
    unsigned int user_params_len;
    unsigned int* user_params_data; /* point to user params addr */
    unsigned int cmdline_len;
    char cmdline[0];
} load_info_t;

unsigned int DDRSize = 0;
unsigned int KernelMemSize = 0;
unsigned int VmmSize = 0;
unsigned int NBGSize = 0;
unsigned int UserSize = 0;
unsigned int NBGAddr = 0;
unsigned int user_params_len = 0;

void media_init(void* args) {
    int gpio_value = 0;
#ifdef TIME_STAT_PTS
    fbv_time_stat_add("CLASS1", "Enter fbv_dsp_init func");
#endif

    fbv_dsp_init();

#ifdef TIME_STAT_PTS
    fbv_time_stat_add("CLASS1", "Enter start_do_nn_detect func");

#endif

#ifdef ENABLE_NNA_DET
    start_do_nn_detect();
#endif

#ifdef TIME_STAT_PTS
    fbv_time_stat_add("CLASS1", "Enter fbv_isp_init func");
#endif
    // Allen-D
    // set_gpio_pm_exclude(31);
    // gpio_set_value(31, 1);
    fbv_isp_init();

#ifdef ENABLE_AUDIO
    audio_init();
#endif

    fbv_isp_start_proc();

#ifdef ENABLE_STARTUP_OSD
    fbv_osd_init(FH_APP_GRP_ID);
#endif

#ifdef TIME_STAT_PTS
    fbv_time_stat_add("CLASS1", "Enter fbv_dsp_enc_init func");
#endif

    fbv_dsp_enc_init();

#ifdef TIME_STAT_PTS
    fbv_time_stat_add("CLASS1", "Media init end");
#endif
    gpio_value = gpio_get_value(31);
    rt_kprintf("gpio_value_after = %d\n", gpio_value);
#ifdef TIME_STAT_PTS
    rt_thread_delay(100);
    fbv_time_stat_prinf("CLASS1");
#endif
#ifdef SAVE_JPEG
    // Allen-D
    capture_jpeg(0, 0);
    capture_jpeg(0, 1);
#endif
}

static void startup_video(void) {
#ifdef TIME_STAT_PTS
    fbv_time_stat_add("CLASS1", "Enter startup_video func");
#endif
    fh_media_process_module_init();
    fh_isp_module_init();
    fh_vise_module_init();
    venc_module_init();
    // mipi_parse_cmdline("lane_4");
#ifdef ENABLE_NNA_DET
    fh_nna_module_init();
#endif
    // fh_jpeg_module_init();

#ifdef FH_USING_START_VIDEO_ON_ARC
    rt_thread_t th_vlc;
    th_vlc = rt_thread_create("vlc_th", media_init, RT_NULL, 10 * 1024, FH_DEMO_THREAD_PRIORITY, 10);
    if (th_vlc) {
        rt_thread_startup(th_vlc);
    }
#endif
}

static void user_vmm_init(void) {
#ifdef FH_USING_VMM_ON_ARC
    unsigned int vmm_size, vmm_start, os_size, ddr_size;
    extern unsigned int _reset;

    vmm_size = VmmSize;
    os_size = FH_DDR_END - (unsigned int)&_reset;
    ddr_size = GET_REG(0x1000050c) << 17;
    if (ddr_size < 0x4000000) /* at least 64MB for FH chip */
        ddr_size = 0x4000000;
    vmm_start = 0x40000000 + ddr_size - os_size - vmm_size - NBGSize;
    if (bufferInit((unsigned char*)vmm_start, vmm_size) != 0) {
        rt_kprintf("Vmm: bufferInit failed!\n");
    }
#endif
}

int user_reset_all_module(void) {
#if 0
    rt_kprintf("user_reset_all_module!\n");

#ifdef FH_USING_START_VIDEO_ON_ARC
    stop_isp_proc();

    ret = FH_SYS_Exit();
    API_ISP_Exit(0);
#endif

#ifdef FH_USING_VMM_ON_ARC
    buffer_reset_vmm();
#endif

    rt_kprintf("user_reset_all_module finish!\n");
#endif
    return 0;
}

// Allen-D Release this function for deinit arc
int user_reset_all_module_EXT(void) {
    rt_kprintf("user_reset_all_module!\n");
    int ret = -1;

    stop_isp_proc();

    ret = FH_SYS_Exit();
    API_ISP_Exit(0);

#ifdef FH_USING_VMM_ON_ARC
    buffer_reset_vmm();
#endif

    rt_kprintf("user_reset_all_module finish!\n");
    return 0;
}

#define SNS_CLK_NAME "sensor%d_clk"
void fh_sensor_probe(unsigned int sensor_id) {
    char sensor_clk_name[20] = {0};
    struct clk* sns_clk_rate = RT_NULL;
    rt_snprintf(sensor_clk_name, sizeof(sensor_clk_name), SNS_CLK_NAME, sensor_id);
    sns_clk_rate = clk_get(RT_NULL, sensor_clk_name);
    clk_enable(sns_clk_rate);
}

void os_hook_startup(void) {
#ifdef FH_USING_START_VIDEO_ON_ARC
    extern void rt_hw_gpio_init(void);
    rt_hw_gpio_init();
    extern void sensor_enter_reset(int grp_id);
    sensor_enter_reset(0);
    sensor_enter_reset(1);
    sensor_enter_reset(2);

#endif
}

void user_sensor_init(void) {
    fh_sensor_probe(0);
    fh_sensor_probe(1);

    //修改sensor clk 驱动能力
    SET_REG_M(REG_PMU_PAD_SENSOR0_CLK0_CFG, 0x60, 0x30);

    SET_REG_M(REG_PMU_PAD_SENSOR1_CLK1_CFG, 0x70, 0x30);

    /*pinmux to pwm2*/
    SET_REG_M(CEN_PIN_REG_BASE + REG_PMU_PAD_PWM2_CFG, 0x2, 0x7);

    /*pinmux to pwm3*/
    SET_REG_M(CEN_PIN_REG_BASE + REG_PMU_PAD_PWM3_CFG, 0x2, 0x7);

    /*pinmux to SCL for I2C0*/
    SET_REG_M(CEN_PIN_REG_BASE + REG_PMU_PAD_I2C0_SCL_CFG, 0, 0x7);

    /*pinmux to SDA for I2C0*/
    SET_REG_M(CEN_PIN_REG_BASE + REG_PMU_PAD_I2C0_SDA_CFG, 0, 0x7);

    /*pinmux to SENSOR_CLK*/
    SET_REG_M(CEN_PIN_REG_BASE + REG_PMU_PAD_SENSOR_CLK_CFG, 0, 0x7);

    // SET_REG_M(CEN_PIN_REG_BASE + REG_PMU_PAD_SENSOR2_CLK_CFG, 0, 0x7);
#ifndef CONFIG_SENSOR_SLAVE_MODE
    /*let sensor start to run...*/
    sensor_exit_reset(0);
#endif
}

/*
 * Attention, Don't modify this function!!!!!!!!!!!!!!!!!!!!!
 */
static int* UserParamDataBuf;

unsigned int fh_get_VmmSize(void) {
    return VmmSize;
}
unsigned int fh_get_DDRSize(void) {
    return DDRSize;
}
unsigned int fh_get_KernelMemSize(void) {
    return KernelMemSize;
}

unsigned int fh_getNBGSize(void) {
    return NBGSize;
}

unsigned int fh_getUserSize(void) {
    return UserSize;
}

unsigned int fh_getNBGAddr(void) {
    return NBGAddr;
}

unsigned int fh_getUserParamDataBufLen(void) {
    return user_params_len;
}

int* fh_getUserParamDataBuf(void) {
    return UserParamDataBuf;
}

int user_main(void) {
    // int gpio_value = 0;
    fbv_time_stat_init("CLASS1");

#ifdef TIME_STAT_PTS
    fbv_time_stat_add("CLASS1", "Enter user_main func");
#endif

    load_info_t* loadInfo = (load_info_t*)fh_hw_readreg(0x10000520);
    fh_hw_invalidate_dcache((rt_uint32_t)loadInfo, sizeof(load_info_t));

#ifdef FH_USING_DDRBOOT
    DDRSize = loadInfo->DDRSize;
    KernelMemSize = loadInfo->KernelMemSize;
    VmmSize = loadInfo->VmmSize;
    NBGSize = loadInfo->NBGSize;
    UserSize = loadInfo->UserSize;
    NBGAddr = loadInfo->NBGAddr;
#else
    DDRSize = 0x4000000;
    KernelMemSize = 0x1c00000;
    VmmSize = 0x1c00000;
    NBGSize = 0x400000;
    UserSize = 0;
    NBGAddr = 0x43800000;
#endif

    rt_kprintf("DDRSize = %x\n", DDRSize);
    rt_kprintf("KernelMemSize = %x\n", DDRSize);
    rt_kprintf("VmmSize = %x\n", VmmSize);
    rt_kprintf("NBGSize = %x\n", NBGSize);
    rt_kprintf("UserSize = %x\n", UserSize);
    rt_kprintf("NBGAddr = %x\n", NBGAddr);

    // UserParamDataBuf = (int *)rt_malloc(user_params_len);
    // rt_memcpy((void *)UserParamDataBuf, (void *)loadInfo->user_params_data, loadInfo->user_params_len);

    RUNLOG_INIT();

    user_vmm_init();

    // Allen-D Set GPIO3_7 for sensor power onp
    SET_REG(CEN_PIN_REG_BASE + 0x08c, 0X01);
    // gpio_set_direction(31, 1);
    // set_gpio_pm_exclude(31);
    // gpio_set_value(31, 1);
    // gpio_value = gpio_get_value(31);
    // rt_kprintf("gpio_value = %d\n", gpio_value);
    // user_sensor_init();

    // rt_hw_i2c_init();

    rt_hw_i2c_init();
    set_sensor_power();
    rt_kprintf("Start set sensor power!\n");
    user_sensor_init();
    // rt_hw_wdt_init();

    rt_hw_pwm_init();

    rt_hw_sadc_init();

    fh_dma_init(0);

    rpc_init_slave();

    xbus_init();

    fh_control_register_stub();

    fh_venc_register_stub();

    // fast_nn_register_stub();

    rpc_hd_init_dsp();

    rpc_hd_init_isp();

    rpc_vmm_init_arc();

    rpc_hd_init_proc();

    xbus_notify_host();

    startup_video();

    shell_init();

    while (1) {
        /*rt_kprintf("I am alived...\n");*/
        rt_thread_delay(RT_TICK_PER_SECOND * 1);
    }
}

/* jpeg抓图 */
FH_SINT32 capture_mjpeg(FH_SINT32 grp_id, FH_SINT32 chn_id) {
    FH_SINT32 ret = RETURN_OK;
    VPU_CHN_INFO* vpu_chn_info = NULL;
    FH_UINT32 chn_w = 0;
    FH_UINT32 chn_h = 0;
    FH_UINT32 jpeg_chn;
    ENC_INFO* mjpeg_info;

    vpu_chn_info = g_vpu_chn_info;
    if (!vpu_chn_info->enable) {
        printf("capture_mjpeg Failed! VPU[%d] CHN[%d] enc not enable!\n", grp_id, chn_id);
        return -1;
    }

    chn_w = vpu_chn_info->width;
    chn_h = vpu_chn_info->height;
    printf("chn_w = %d, chn_h = %d\n", chn_w, chn_h);

    mjpeg_info = get_mjpeg_config(grp_id, chn_id);

    ret = mjpeg_create_chn(grp_id, chn_id);
    printf("mjpeg create channel %d\n", ret);

    ret = set_mjpeg(grp_id, chn_id);
    printf("set_mjpeg %d\n", ret);

    ret = mjpeg_start(grp_id, chn_id);
    printf("mjpeg_start %d\n", ret);

    ret = vpu_bind_jpeg(grp_id, chn_id, mjpeg_info->channel);
    printf("vpu binnd mjpeg %d\n", ret);

    return ret;
}

FH_SINT32 capture_jpeg(FH_SINT32 grp_id, FH_SINT32 chn_id) {
    FH_SINT32 ret = RETURN_OK;
    VPU_CHN_INFO* vpu_chn_info = NULL;
    FH_UINT32 chn_w = 0;
    FH_UINT32 chn_h = 0;
    FH_UINT32 jpeg_chn;

    vpu_chn_info = &g_vpu_chn_info[grp_id][chn_id];
    if (!vpu_chn_info->enable) {
        printf("capture_jpeg Failed! VPU[%d] CHN[%d] enc not enable!\n", grp_id, chn_id);
        return -1;
    }

    chn_w = vpu_chn_info->width;
    chn_h = vpu_chn_info->height;
    printf("VPU[%d][%d] size chn_w = %d, chn_h = %d\n", grp_id, chn_id, chn_w, chn_h);

    ret = jpeg_create_chn(chn_w, chn_h, chn_id);
    printf("jpeg create channel result = %d\n", ret);

    jpeg_chn = get_jpeg_chn();

    ret = vpu_bind_jpeg(grp_id, chn_id, jpeg_chn);
    printf("vpu binnd jpeg %d\n", ret);

    ret = unbind_jpeg_chn(jpeg_chn);
    printf("vpu unbinnd jpeg %d\n", ret);

    return ret;
}

void set_sensor_power(void) {
    aw37004_device_t* dev;
    int ret = 0;
    i2c_bus = rt_i2c_bus_device_find("i2c1");
    if (i2c_bus == RT_NULL) {
        rt_kprintf("I2C bus not found!\n");
    } else {
        rt_kprintf("Find I2C bus device!\n");
    }

    if (rt_device_open(i2c_bus, RT_DEVICE_FLAG_RDWR) != RT_EOK) {
        rt_kprintf("[AW37004] Error: Failed to open I2C bus %s\n", I2C_BUS_NAME);
    } else {
        rt_kprintf("Open I2C bus device!\n");
    }

    dev = (aw37004_device_t*)rt_malloc(sizeof(aw37004_device_t));
    if (!dev) {
        rt_device_close(i2c_bus);
        rt_kprintf("[AW37004] Error: Memory allocation failed\n");
        return;
    }

    dev->i2c_bus = i2c_bus;
    dev->dev_addr = AW37004_ADDR;
    dev->initialized = RT_FALSE;

    ret = aw37004_i2c_write_reg(dev, REG_AVDD1_VOUT, AVDD1_VOUT_DEFAULT);
    // rt_kprintf("[DEBUG] I2C write reg AVDD1 ret = %d\n", ret);
    // rt_thread_mdelay(10);
    ret = aw37004_i2c_write_reg(dev, REG_AVDD2_VOUT, AVDD2_VOUT_DEFAULT);
    // rt_kprintf("[DEBUG] I2C write reg AVDD2 ret = %d\n", ret);
    // rt_thread_mdelay(10);
    ret = aw37004_i2c_write_reg(dev, REG_DVDD1_VOUT, DVDD_VOUT_DEFAULT);
    // rt_kprintf("[DEBUG] I2C write reg DVDD1 ret = %d\n", ret);
    // rt_thread_mdelay(10);

    rt_uint8_t encr_val = 0x00;
    // AVDD1
    encr_val |= 0x04;
    ret |= aw37004_i2c_write_reg(dev, REG_ENCR, encr_val);
    if (ret == 1) {
        rt_kprintf("Enable AVDD1 success 0x%02X\n", dev->dev_addr);
    }
    // rt_thread_mdelay(10);

    // AVDD2
    encr_val |= 0x08;
    ret |= aw37004_i2c_write_reg(dev, REG_ENCR, encr_val);
    if (ret == 1) {
        rt_kprintf("Enable AVDD2 success 0x%02X\n", dev->dev_addr);
    }
    // rt_thread_mdelay(10);

    // DVDD1
    encr_val |= 0x01;
    ret |= aw37004_i2c_write_reg(dev, REG_ENCR, encr_val);
    if (ret == 1) {
        rt_kprintf("Enable DVDD1 success 0x%02X\n", dev->dev_addr);
    }
    // rt_thread_mdelay(10);
}

static rt_err_t aw37004_i2c_write_reg(aw37004_device_t* dev, rt_uint8_t reg, rt_uint8_t data) {
    struct rt_i2c_msg msgs[1];
    rt_uint8_t buf[2];

    if (!dev) {
        rt_kprintf("Device is null !\n");
        return -RT_ERROR;
    }

    buf[0] = reg;
    buf[1] = data;

    msgs[0].addr = dev->dev_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = buf;
    msgs[0].len = 2;

    rt_int32_t ret = rt_i2c_transfer(i2c_bus, msgs, 1);
    if (ret != 1) {
        rt_kprintf("Failed to write I2C device 0x%02X\n", dev->dev_addr);
        return ret;
    }

    rt_kprintf("[AW37004] Write success: addr=0x%02X, reg=0x%02X, data=0x%02X\n", dev->dev_addr, reg, data);
    return ret;
}