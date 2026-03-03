#include "glass_service_impl.h"
#include "base/app_command_defs.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <ratio>
#include <algorithm>
#include <stdio.h>

#include "base/app_common_defs.h"
#include "base/file_path.h"
#include "base/logging.h"
#include "base/uart_service.h"
#include "base/socket_server.h"
#include "base/file_enumerator.h"
#include "base/uart_helper.h"
#include "base/wifi_handler.h"

#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <fstream>

#define UIMAGE_URL "http://qn.oss.eqshares.com/glass/control/uImage.img"

namespace {

static SocketServer g_socket_server;
std::atomic<bool> is_recording_{false};
std::atomic<bool> is_time_synced_{false};
constexpr int kDmcMediaTypeAudio = (1 << 11);
constexpr int kAudioMediaChannel = 0;
constexpr char kP2PSupplicantConfPath[] = "/start/p2p_supplicant.conf";

extern "C" {
extern int take_big_photo(const char* path);
extern int take_small_photo(const char* path);
extern int take_small_photo_in_memory(uint8_t **data, uint32_t* length);
extern int librecord_init(const void* config);
extern int librecord_save(int media_chn, int media_type, int media_subtype, unsigned char *frame_data, int frame_len);
extern int librecord_uninit(void);
extern bool get_record_state(void);
extern void set_record_state(bool enable);
extern bool is_time_synced(void);
extern void set_time_synced(bool synced);
}
enum Caller {
    RECORD,
    PHOTO,
};

bool get_record_state(void) {
    return is_recording_.load(std::memory_order_acquire);
}
void set_record_state(bool enable) {
    is_recording_.store(enable, std::memory_order_release);
}
bool is_time_synced(void) {
    return is_time_synced_.load(std::memory_order_acquire);
}
void set_time_synced(bool synced) {
    is_time_synced_.store(synced, std::memory_order_release);
}

void get_file_name(char *buffer, uint8_t length, Caller filename)
{
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  uint64_t now_sec = ts.tv_sec;
  char time_str[64] = {0};
  time_t time_val = (time_t)now_sec;
  struct tm* timeinfo = localtime(&time_val);
  if (timeinfo != nullptr) {
      strftime(time_str, sizeof(time_str), "%Y-%m-%d-%H-%M-%S", timeinfo);
      LOGD("Generated file name time: %s", time_str);
  }

  // 当系统时间未同步时(Unix 纪元 1970-01-01)，使用单调时钟生成唯一文件名
  constexpr uint64_t kMinValidTimestamp = 86400;  // 1970-01-02 00:00:00
  if (now_sec < kMinValidTimestamp) {
      struct timespec mono_ts;
      clock_gettime(CLOCK_MONOTONIC, &mono_ts);
      snprintf(time_str, sizeof(time_str), "unsync_%llu",
               (unsigned long long)(mono_ts.tv_sec * 1000ULL + mono_ts.tv_nsec / 1000000));
      LOGW("System time not synced (epoch %llu), using fallback name: %s",
           (unsigned long long)now_sec, time_str);
  }

  if (filename == RECORD) {
    snprintf(buffer, length, "%s.mp4", time_str);
  } else if (filename == PHOTO) {
    snprintf(buffer, length, "%s.jpg", time_str);
  }
}

std::string TrimSpaces(const std::string& value)
{
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

}



GlassServiceImpl::GlassServiceImpl(AppConfig* app_config): app_config_(app_config) {
    // Constructor implementation
}

GlassServiceImpl::~GlassServiceImpl() {
    // Constructor implementation
}


void GlassServiceImpl::Start() {
    LOGD("Starting GlassServiceImpl with config: %s", app_config_->record_path);
    thumbnail_root_ = std::string(app_config_->record_path) + "/" + THUMB_IMG_DIR_NAME;
    uart_service_.StartUartService(this);
}

void GlassServiceImpl::Stop() {
    uart_service_.StopUartService();
    g_socket_server.Stop();
    is_audio_processing_ = false;
    if (audio_process_thread_ && audio_process_thread_->joinable()) {
        audio_process_thread_->join();
        delete audio_process_thread_;
        audio_process_thread_ = nullptr;
    }
}


void GlassServiceImpl::OnTakePhoto() {
    LOGD("Taking photo...");

    if (!is_time_synced()) {
        // 若未同步时间则请求同步，等待更长时间以便手机端响应
        auto UartFrame = WifiHandler::MakeUartFrame(DEV_ID_MASTER, DEV_ID_APP, APP_CMD_SYNC_TIME, 0);
        uart_service_.Uart2Send(UartFrame->header_start, UARTFrame_SIZE(0));
        free(UartFrame);
        UartHelper::SleepMilliSeconds(1500);  // 增加等待时间，提高同步成功率
    }

    set_record_state(true);
    UartHelper::SetGPIOValue(GPIO3_6, 1);

    //send to bluetooth on photoing
    UartHelper::SleepMilliSeconds(200);
    auto frame = WifiHandler::MakeUartFrame(DEV_ID_MASTER, DEV_ID_APP, APP_CMD_PHOTOING, 0);
    int ret = uart_service_.Uart2Send(frame->header_start, UARTFrame_SIZE(0));
    LOGI("OnTakePhoto uart_write: 0x%02x , ret=%d", APP_CMD_PHOTOING, ret);
    free(frame);

    char file_name[64] = { 0 };
    get_file_name(file_name, sizeof(file_name), PHOTO);
    char large_path[128] = { 0 };
    snprintf(large_path, sizeof(large_path), "%s/%s", app_config_->record_path, file_name);
    take_big_photo(large_path);

    std::string thumbnail_path = thumbnail_root_ + "/" + FilePath(file_name).BaseName();
    take_small_photo(thumbnail_path.data());

    UartHelper::SyncToDisc();
    UartHelper::SleepMilliSeconds(200);

    if (!get_record_state()) {
        UartHelper::SleepMilliSeconds(30);
        UartHelper::SetGPIOValue(GPIO3_6, 0);
    }

    uart_service_.DoAfterTask(WakeupReason::TakePhoto);
    set_record_state(false);
}

void GlassServiceImpl::OnStartRecording() {
    LOGD("Starting recording...");
    if (!is_time_synced()) {
        // 若未同步时间则请求同步，等待更长时间以便手机端响应
        auto UartFrame = WifiHandler::MakeUartFrame(DEV_ID_MASTER, DEV_ID_APP, APP_CMD_SYNC_TIME, 0);
        uart_service_.Uart2Send(UartFrame->header_start, UARTFrame_SIZE(0));
        free(UartFrame);
        UartHelper::SleepMilliSeconds(1500);  // 增加等待时间，提高同步成功率
    }
    if (get_record_state()) {
        LOG_W("Recording is already enabled.");
        return;
    }
    UartHelper::SetGPIOValue(GPIO3_6, 1);

        //send to bluetooth on recording
        UartHelper::SleepMilliSeconds(200);
        const uint16_t payload_size = 1;
        auto frame = WifiHandler::MakeUartFrame(DEV_ID_MASTER, DEV_ID_APP, APP_CMD_RECORDING, payload_size);
        frame->data[0] = 0x01;
        frame->length = htons(payload_size);
        int ret = uart_service_.Uart2Send(frame->header_start, UARTFrame_SIZE(payload_size));
        LOGI("OnStartRecording uart_write: 0x%02x , ret=%d", APP_CMD_RECORDING, ret);
        free(frame);

    get_file_name(app_config_->record_file_name, sizeof(app_config_->record_file_name), RECORD);
    librecord_init(app_config_);

    if (audio_stream_.StartAudio()) {
        is_audio_processing_ = true;
    } else {
        LOGW("OnStartRecording: start audio stream failed");
    }

    set_record_state(true);

    std::string thumbnail_path = thumbnail_root_ + "/" + app_config_->record_file_name;
    thumbnail_path = thumbnail_path.substr(0, thumbnail_path.size() - 4) + MEDIA_TYPE_JPEG;
    take_small_photo(thumbnail_path.data());
}

void GlassServiceImpl::OnStopRecording() {
    LOGD("Stopping recording...");
    UartHelper::SetGPIOValue(GPIO3_6, 0);
    if (!get_record_state()) {
        LOG_W("Recording is already stoped.");
        return;
    }

    audio_stream_.StopAudio();
    is_audio_processing_ = false;

    librecord_uninit();
        //send to bluetooth stop record
        const uint16_t payload_size = 1;
        auto frame = WifiHandler::MakeUartFrame(DEV_ID_MASTER, DEV_ID_APP, APP_CMD_RECORDING, payload_size);
        frame->data[0] = 0x00;
        frame->length = htons(payload_size);
        int ret = uart_service_.Uart2Send(frame->header_start, UARTFrame_SIZE(payload_size));
        LOGI("OnStopRecording uart_write: 0x%02x , ret=%d", APP_CMD_RECORDING, ret);
        free(frame);

    uart_service_.DoAfterTask(WakeupReason::StopRecord);
    set_record_state(false);
    UartHelper::SyncToDisc();
    UartHelper::SleepMilliSeconds(1000);
}

void GlassServiceImpl::OnStartWebService() {
    LOGD("Starting webserveice");
    g_socket_server.Start(app_config_->record_path , this);
}

bool GlassServiceImpl::OnOTAStartup(int device, const char* url) {
    LOGD("Starting OnOTAStartup:%s", url);
    return g_socket_server.PerformanceOTA(device, url) == 0;
}

void GlassServiceImpl::OnLinuxUpgraded(int result) {
    LOGD("Linux upgrade result: %d", result);
    const char* status = result == 0 ? "LinuxUpgradedSuccess" : "LinuxUpgradedfailed";
    uart_service_.Uart2Send((const uint8_t*)status, strlen(status));
}

void  GlassServiceImpl::OnARCHUpgraded(int result) {
    LOGD("ARCH upgrade result: %d", result);
    const char* status = result == 0 ? "ARCHUpgradedSuccess" : "ARCHUpgradedfailed";
    uart_service_.Uart2Send((const uint8_t*)status, strlen(status));
}

void GlassServiceImpl::OnOTAProgress(int stage, int percentage, const char* message) {
    if (percentage < 0) {
        LOG_E("OTA Progress Error [Stage:%d]: %s", stage, message);
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "OTA_ERROR:Stage%d:%s", stage, message);
        return;
    }

    LOG_I("OTA Progress [Stage:%d] %d%%: %s", stage, percentage, message);

    const uint16_t payload_size = 1;
    auto frame = WifiHandler::MakeUartFrame(DEV_ID_MASTER, DEV_ID_APP, APP_CMD_OTA_PROGRESS, payload_size);
    uint8_t current_percentage = static_cast<uint8_t>(std::max(1, std::min(percentage, 100)));
    frame->data[0] = current_percentage;
    frame->length = htons(payload_size);
    UartHelper::FillCRC((uint8_t*)frame, UARTFrame_SIZE(payload_size) - sizeof(uint16_t));
    int ret = uart_service_.Uart2Send(frame->header_start, UARTFrame_SIZE(payload_size));
    LOGI("OTA Progress frame send percent:%d ret:%d", current_percentage, ret);
    free(frame);
}

void GlassServiceImpl::OnRequestFileList(uint16_t* file_count, std::string& thumbnail_path) {
    LOGD("OnRequestFileList");
    const auto& path_list = FileEnumerator::FetchAll(app_config_->record_path, {MEDIA_TYPE_MP4, MEDIA_TYPE_JPEG});
    // const auto& thumb_list = FileEnumerator::FetchAll(thumbnail_root_, {MEDIA_TYPE_JPEG});
    *file_count = (uint16_t)path_list.size();
    if (!path_list.empty()) {
        thumbnail_path = *path_list.rbegin();
        LOGT("OnRequestFileList:%d, path:%s ", *file_count, thumbnail_path.data());
    }
}

void GlassServiceImpl::SetP2PName(const std::string& name) {
    if (name.empty()) {
        LOGW("SetP2PName: empty name, skip");
        return;
    }

    std::ifstream in(kP2PSupplicantConfPath);
    if (!in.is_open()) {
        LOGE("SetP2PName: open failed: %s, errno=%d", kP2PSupplicantConfPath, errno);
        return;
    }
    const std::string prefix = "device_name=";
    std::streampos line_start = 0;
    std::string line;
    while (true) {
        line_start = in.tellg();
        if (!std::getline(in, line)) {
            break;
        }
        if (line.rfind(prefix, 0) == 0) {
            std::string old_name = TrimSpaces(line.substr(prefix.size()));
            if (old_name == name) {
                LOGI("SetP2PName: device_name unchanged: %s", name.c_str());
                in.close();
                return;
            }
            std::string new_line = prefix + name;
            std::fstream io(kP2PSupplicantConfPath, std::ios::in | std::ios::out);
            if (!io.is_open()) {
                LOGE("SetP2PName: reopen failed: %s, errno=%d", kP2PSupplicantConfPath, errno);
                return;
            }
            io.seekp(line_start);
            io.write(new_line.data(), new_line.size());
            const size_t pad_len = line.size() - new_line.size();
            for (size_t i = 0; i < pad_len; ++i) {
                io.put(' ');
            }
            io.flush();
            io.close();
            LOGI("SetP2PName: updated device_name to %s", name.c_str());
            return;
        }
    }
    in.close();
}

void GlassServiceImpl::OnCommandStart(void) {
    uart_service_.EnableHeartbeat(false);
}
void GlassServiceImpl::OnCommandEnd(bool shutdown) {
    if (get_record_state()) {
        return;
    }

    if (!g_socket_server.IsTransfering() && shutdown) {
        LOGT("OnCommandEnd: enable heartbeat");
        uart_service_.EnableHeartbeat(true);
    }
}

void GlassServiceImpl::OnBeforePoweroff(void) {
    g_socket_server.Stop();
}

bool GlassServiceImpl::IsTransfering(void) {
    return g_socket_server.IsTransfering();
}

SequenceManager* GlassServiceImpl::GetSequenceManager(void) {
    return uart_service_.GetSequenceManager();
}

void GlassServiceImpl::OnAfterOTA(int fd, int type, int result) {
    LOGI("OnAfterOTA :%d result : %d", type, result);
    uart_service_.Restart(APP_RESTART_BY_OTA);
}

void GlassServiceImpl::PoweroffForce() {
    LOGI("PoweroffForce ");
    uart_service_.EnableHeartbeat(true);
    uart_service_.PowerOff();
}