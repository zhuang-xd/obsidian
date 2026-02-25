#ifndef BASE_APP_COMMON_DEFS_H_
#define BASE_APP_COMMON_DEFS_H_

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
/**
    some common defines
*/

#define TRANSFER_FRAME_MAGIC_NUBER (0xAF8C)

#define MAIN_CONTROL_READY    (0xAF8D)
#define TAKE_PHOTO_FINISHED   (0xAF8E)
#define TAKE_RECORD_FINISHED  (0xAF8F)

#define APP_NAME ("AiGlasses")

#define APP_VERSION ("01.21.01.17")

#define SERVER_PORT (8556)

#define SERVER_MAX_CONNECTIONS (16)

#define TRANSFER_FRAME_VERSION (0x0001)

#define SD_CARD_DATA_ROOT ("/mnt/sd")
#define THUMB_IMG_DIR_NAME ("thumb")
#define WIFI_CONFIG_EXE ("wifi_config")
#define WIFI_INTERFACE_NAME ("wlan0")
#define P2P_INTERFACE_NAME ("p2p-wlan0-0")

#define MEDIA_TYPE_MP4 (".mp4")
#define MEDIA_TYPE_JPEG (".jpg")

#define APPLICATION_CONFIG_FILE ("g5k6_conf.json")

#define ARCH_IMAGE_NAME ("g5k6_rtthread_arc.bin.img")
#define KERNEL_IMAGE_NAME ("g5k6_uImage.img")
#define LOG_FILE_PATH ("/start/log/")
#define LOG_FILE_NAME ("eq_glasses.log")

#define ARCH_DEVICE ("/dev/mtd1")
#define KERNEL_DEVICE ("/dev/mtd3")
#define COMBINED_DEVICE ("/dev/mtd1")

#define COMBINED_OTA_PACKAGE_NAME ("g5k6_package.tar.gz")
#define COMBINED_OTA_IMAGE_COUNT (2)
#pragma pack(push, 1)

/** data transfer struct between linux and APP.
*/
typedef struct _TransferFrame {
    // 0xAF8C
    uint16_t magic;

    uint16_t version;

    // command type
    uint16_t command;

    uint16_t source_id;

    uint16_t target_id;

    uint16_t reserved;

    // external data length in front of data
    uint16_t external_length;

    // header+paylaod+crc
    uint32_t total_length;

    // payload data
    uint8_t data[0];

    // data crc, this field is need, will put the end of data, but not count in frame header,
    // uint16_t crc;
} TransferFrame, *TransferFramePtr;


typedef struct _UARTFrame {
    uint8_t header_start[0];

    // 0XAA
    uint8_t SOF;
    uint8_t sender_id;

    uint8_t receiver_id;

    uint8_t command;

    // payload length
    uint16_t length;

    // payload data
    uint8_t data[0];

    // data crc, this field is need, will put the end of data, but not count in frame header,
    // uint16_t crc;
} UARTFrame, *UARTFramePtr;

#pragma pack(pop)

#define UARTFRAME_SOF (0xAA)

// UARTFrame size + payload size + crc size
#define UARTFrame_SIZE(length) (sizeof(UARTFrame) + length + 2)


typedef enum {
    APP_COMMAND_SEND_COMMAND = 0,
    APP_COMMAND_SEND_FILE,
    APP_COMMAND_SEND_STREAM,
    APP_COMMAND_SEND_FILE_LIST,
    APP_COMMAND_DOWNLOAD_RTTHEAD_URL,
    APP_COMMAND_DOWNLOAD_LINUX_URL,
    APP_COMMAND_HEARTBEAT,
    APP_COMMAND_SEND_FILE_DOWN,
    APP_COMMAND_DOWNLOAD_LOG_FILE,
    APP_COMMAND_MAX
} AppCommand;

typedef enum {
    APP_RESTART_BY_NORMAL = 0,
    APP_RESTART_BY_OTA,
    APP_RESTART_BY_ERROR,
    APP_RESTART_MAX,
} AppRestartReson;

#define HANDLE_EINTR(x)                                     \
  ({                                                        \
    int eintr_wrapper_result;                               \
    do {                                                    \
      eintr_wrapper_result = (x);                           \
    } while ((eintr_wrapper_result == -1) && (errno == EINTR || errno == EAGAIN)); \
    eintr_wrapper_result;                                   \
  })

typedef struct {
  uint16_t video_width;
  uint16_t video_height;
  uint16_t video_framerate;
  uint16_t audio_framerate;
  char record_path[128];
  char record_file_name[32];
} AppConfig, *AppConfigPtr;

#ifdef __cplusplus
extern "C" {
#endif
bool get_record_state(void);
void set_record_state(bool enable);
bool is_time_synced(void);
void set_time_synced(bool synced);
#ifdef __cplusplus
}
#endif // __cplusplus

#endif  // BASE_APP_COMMON_DEFS_H_
