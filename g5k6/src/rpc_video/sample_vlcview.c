#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

#include "base/app_common_defs.h"
#include "base/logging.h"
#include "dsp/fh_system_mpi.h"
#include "dsp/fh_venc_mpi.h"
#include "dsp/fh_vpu_mpi.h"
#include "eq_glasses/entry/main.h"
#include "isp/isp_api.h"
#include "isp/isp_enum.h"
#include "librecord/include/librecord.h"
#include "sample_opts.h"
#include "types/type_def.h"
#include <fh_rpc_customer_mpi.h>

#define MAX_GRP_NUM 2

#define GRP_ENABLED 2
#define MAX_PES_CHANNEL_COUNT 36
static int g_demo_mode = 1;

struct channel_info {
    FH_UINT32 width;
    FH_UINT32 height;
    FH_UINT8 frame_count;
    FH_UINT8 frame_time;
    FH_UINT32 bps;
};

typedef struct {
    int cmd;
    int grpid;
} XBUS_CMD_S;

typedef enum {
    CAPTURE_JPEG = 0,
    PREVIEW = 1,
    RECORD = 2,
    CAPTURE_JPEG_SMALL_ONLY = 3,
    FORMAT_CHANGE_400WP30 = 4,
    FORMAT_CHANGE_400WP25 = 5,
    FORMAT_CHANGE_1080P30 = 6,
    FORMAT_CHANGE_1080P25 = 7,
    FORMAT_CHANGE_720P30 = 8,
    FORMAT_CHANGE_720P25 = 9,
    FORMAT_CHANGE_320P30 = 10,
    FORMAT_CHANGE_320P25 = 11,
} RTT_CMD_ID;

static FH_BOOL g_stop_running = FH_FALSE;
static pthread_t g_thread_stream = 0;
static volatile AppConfig g_app_config;

static volatile bool g_record_running = false;

void sample_vlcview_exit(void) {
    if (g_thread_stream != 0)
        pthread_join(g_thread_stream, NULL);
}

void sample_vlcview_handle_sig(FH_SINT32 signo) {
    printf("Caught %d, program exit abnormally", signo);
    if (g_stop_running) {
        exit(EXIT_FAILURE);
    }

    g_stop_running = FH_TRUE;
    librecord_uninit();
    sample_vlcview_exit();
    stop_application();

    exit(EXIT_FAILURE);
}

void* sample_vlcview_get_stream_proc(void* arg) {
    FH_SINT32 ret, i;
    FH_VENC_STREAM stream;
    FH_SINT32 subtype;
    // int index = 0;
    int chan = 0;
    // FH_VENC_RequestIDR(chan);
    // while (!g_stop_running) {
    //     index++;
    //     if (index > 5) {
    //         break;
    //         LOGI("break");
    //     }
    //     ret = FH_VENC_GetChnStream_Timeout(0, &stream, 0);
    // }
    FH_VENC_RequestIDR(chan);
    while (!g_stop_running) {
        g_record_running = get_record_state();
        if (!g_record_running) {
            usleep(50 * 1000);
            continue;
        }
        ret = FH_VENC_GetChnStream_Timeout(0, &stream, 10 * 1000);
        if (ret == RETURN_OK) {
            if (stream.stmtype == FH_STREAM_H265) {
                for (i = 0; i < stream.h265_stream.nalu_cnt; i++) {
                    if (g_record_running) {
                        subtype = stream.h265_stream.frame_type == FH_FRAME_I ? DMC_MEDIA_SUBTYPE_IFRAME
                                                                              : DMC_MEDIA_SUBTYPE_PFRAME;
                        librecord_save(stream.chan, DMC_MEDIA_TYPE_H265, subtype, stream.h265_stream.nalu[i].start,
                                       stream.h265_stream.nalu[i].length);
                    } else {
                        LOG_E("FH_VENC_GetStream_Timeout g_record_running:%d.", g_record_running);
                    }
                }
            }

            FH_VENC_ReleaseStream(&stream);
        } else {
            LOG_W("FH_VENC_GetStream_Timeout ret:0x%x.", ret);
        }
    }
    LOG_I("sample_vlcview_get_stream_proc out.")
    return NULL;
}

int main(int argc, char const* argv[]) {
    FH_SINT32 ret;
    XBUS_CMD_S arguments;

    if ((argc > 1) && (strcmp((const char*)argv[1], "--version") == 0)) {
        LOG_C("%s", APP_VERSION);
        return 0;
    }

    if ((argc > 1) && (strcmp((const char*)argv[1], "--reboot") == 0)) {
        return reboot_hardware(APP_RESTART_BY_NORMAL);
    }

    // pid_t pid = 0;
    // setpriority(PRIO_PROCESS, pid, -10);
    // struct sched_param param = {.sched_priority = 80};
    // if (sched_setscheduler(pid, SCHED_FIFO, &param) == -1) {
    //     LOGE("sched_setscheduler failed");
    // }

    signal(SIGINT, sample_vlcview_handle_sig);
    signal(SIGQUIT, sample_vlcview_handle_sig);
    signal(SIGKILL, sample_vlcview_handle_sig);
    signal(SIGTERM, sample_vlcview_handle_sig);

    /******************************************
     step  1: init media platform
    ******************************************/
    ret = FH_SYS_Init();
    if (ret != RETURN_OK) {
        printf("Error: FH_SYS_Init failed with %x", ret);
        goto err_exit;
    }

    ret = FH_RPC_Customer_Init();
    memset(&arguments, 0, sizeof(arguments));
    ret = FH_RPC_Customer_Command(FORMAT_CHANGE_400WP25, &arguments, sizeof(arguments));

    /******************************************
     step  11: initialize vlcview lib
    ******************************************/

    start_application(&g_app_config);
    /******************************************
     step  12: get stream, pack as PES stream and then send to vlc
    ******************************************/
    pthread_create(&g_thread_stream, NULL, sample_vlcview_get_stream_proc, NULL);

    while (1) {
        usleep(20000);
    }

    return 0;

    /******************************************
     step  14: exit process
    ******************************************/
err_exit:
    g_stop_running = 1;
    stop_application();
    sample_vlcview_exit();
    if (get_record_state()) {
        librecord_uninit();
    }

    return 0;
}

int take_big_photo(const char* path) {
    int ret = 0;
    ret = vpu_bind_jpeg(0, 0, 30);
    ret = getJpegStream(path, NULL, NULL);
    ret = unbind_jpeg_chn(30);
    return ret;
}

int take_small_photo(const char* path) {
    int ret = 0;
    ret = vpu_bind_jpeg(0, 1, 31);
    ret = getJpegStream(path, NULL, NULL);
    ret = unbind_jpeg_chn(31);

    return ret;
}

int take_small_photo_in_memory(uint8_t** data, uint32_t* length) {
    int ret = 0;
    ret = vpu_bind_jpeg(0, 1, 31);

    ret = getJpegStream(NULL, data, length);

    ret = unbind_jpeg_chn(31);

    return ret;
}

int save_jpeg_from_stream(const char* file_path, const unsigned char* jpeg_data, size_t data_size) {
    if (!jpeg_data || data_size == 0) {
        LOGE("Invalid jpeg stream!");
        return 0;
    }

    FILE* fp = fopen(file_path, "wb");
    if (!fp) {
        LOGE("Can not create jpeg file %s!", file_path);
        return 0;
    }

    size_t written = fwrite(jpeg_data, 1, data_size, fp);
    fflush(fp);
    fclose(fp);

    if (written != data_size) {
        LOGE("Write failed! (Jpeg file %zu bytes，acturelly write %zu bytes)", data_size, written);
        remove(file_path);
        return 0;
    }

    return 1;
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
    LOGD("FH_SYS_Bind result = %d", ret);

    return ret;
}

FH_SINT32 unbind_jpeg_chn(FH_UINT32 jpeg_chn) {
    FH_SINT32 ret = 0;
    FH_BIND_INFO dst;

    dst.obj_id = FH_OBJ_JPEG;
    dst.dev_id = 0;
    dst.chn_id = jpeg_chn;

    ret = FH_SYS_UnBindbyDst(dst);
    return ret;
}

static uint64_t now_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t absolute_micro = (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
    return absolute_micro;
}

int getJpegStream(const char* file_path, uint8_t** data, uint32_t* length) {
    FH_SINT32 ret = 0;
    FH_VENC_STREAM stream;
    FH_VENC_STREAM* cur_stream = malloc(sizeof(FH_VENC_STREAM));

    ret = FH_VENC_GetStream_Timeout(FH_STREAM_JPEG, &stream, 10 * 1000);

    LOGI("FH_VENC_GetStream_Timeout 0x%x", ret);
    memcpy(cur_stream, &stream, sizeof(FH_VENC_STREAM));
    if (file_path) {
        // Save to file
        LOGD("file path: %s", file_path);
        ret = save_jpeg_from_stream(file_path, cur_stream->mjpeg_stream.start, cur_stream->mjpeg_stream.length);
    } else {
        // Save to memory
        LOGD("Save to memory");
        uint8_t* payload = (uint8_t*)malloc(cur_stream->mjpeg_stream.length);
        memcpy(payload, cur_stream->mjpeg_stream.start, cur_stream->mjpeg_stream.length);
        *data = payload;
        *length = cur_stream->mjpeg_stream.length;
    }

    ret = FH_VENC_ReleaseStream(&stream);
    free(cur_stream);

    return ret;
}

FH_SINT32 enc_release_stream(FH_VENC_STREAM* stream) {
    return FH_VENC_ReleaseStream(stream);
}
