#include "base/wifi_handler.h"
#include "app_common_defs.h"
#include "base/app_common_defs.h"
#include "base/app_command_defs.h"
#include "base/system_constants.h"
#include "base/file_path.h"
#include "base/file_enumerator.h"
#include "base/logging.h"
#include "base/system_util.h"
#include "base/uart_helper.h"
#include "base/uart_service.h"
#include "base/glass_service_delegate.h"
#include "base/socket_server.h"
#include "logging.h"
#include "uart_service.h"
#include "wifi_handler.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>

#include <mutex>
#include <netinet/in.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <fh_rpc_customer_mpi.h>
#include <vector>

namespace {
static constexpr char kWifiConfigDelimiter = '/';
static constexpr uint16_t kFinishedFlagSize = 1;
static constexpr char * kWPASupplicantConfigPath = "/mnt/sd/wpa_supplicant.conf";

char s_ssid[64] = {0};
static uint16_t s_LaunchCount = 0;

static constexpr uint8_t kMediaCommandTakePhoto[] = {0xA5,0x01,0x00,0xA6,0xEF};
static constexpr uint8_t kMediaCommandStartRecord[] = {0xA5,0x02,0x00,0xA7,0xEF};
static constexpr uint8_t kMediaCommandStopRecord[] = {0xA5,0x03,0x00,0xA8,0xEF};


int save_wifi_config(const char *ssid, const char *psk) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "wpa_passphrase %s %s", ssid, psk);

    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        perror("popen failed");
        return 1;
    }

    FILE *fwpa = fopen(kWPASupplicantConfigPath, "w+");
    char buf[512];
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        fprintf(fwpa, "%s", buf);
    }
    fclose(fwpa);

    pclose(fp);
    UartHelper::SyncToDisc();
    return 0;
}


typedef struct {
    int cmd;
    int grpid;
} XBUS_CMD_S;

static int wifi_config(const char* buffer, UartService* service) {
  // The data format received from the host is "ssid/password"
  LOGI("uart_read ssid and password: %s", buffer);
  const char *pos = strchr(buffer, kWifiConfigDelimiter);
    if (pos == NULL) {
        LOGE("Input format error, missing '%c'", kWifiConfigDelimiter);
        return -1;
    }

    size_t ssid_len = pos - buffer;
    size_t pwd_len = strlen(pos + 1);

    if (ssid_len >= 128 || pwd_len >= 128) {
        printf("SSID or password too long\n");
        return -1;
    }

    char ssid[128] = {0};
    char password[128] = {0};
    strncpy(ssid, buffer, ssid_len);
    strncpy(s_ssid, buffer, ssid_len);
    strncpy(password, pos + 1, pwd_len);

    save_wifi_config(ssid, password);

    LOG_S("WiFi configuration saved: SSID=%s, PASSWORD=%s\n", ssid, password);

    ++s_LaunchCount;
    if(s_LaunchCount > 3){
        service->Restart(APP_RESTART_BY_NORMAL);
    }

    std::string command_line = "bash /sbin/launch_wifi.sh " + std::to_string(s_LaunchCount);
    base::LaunchProcess(command_line);
    return 0;
}

static int read_ssid_from_config(char* ssid, size_t max_len) {
    FILE *fp = fopen(kWPASupplicantConfigPath, "r");
    if (fp == NULL) {
        return -1;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp) != NULL) {
        const char* ssid_line = strstr(line, "ssid=");
        if (ssid_line != NULL) {
            ssid_line += 5;
            while (*ssid_line == ' ' || *ssid_line == '\t' || *ssid_line == '"') {
                ssid_line++;
            }
            size_t i = 0;
            while (i < max_len - 1 && ssid_line[i] != '\0' &&
                   ssid_line[i] != '"' && ssid_line[i] != '\n' &&
                   ssid_line[i] != '\r') {
                ssid[i] = ssid_line[i];
                i++;
            }
            ssid[i] = '\0';
            fclose(fp);
            return 0;
        }
    }
    fclose(fp);
    return -1;
}

static int get_local_ip(const char *interface, char* local_ip)
{
    int fd;
    int ret = 0;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, interface, IFNAMSIZ-1);

    ret = ioctl(fd, SIOCGIFADDR, &ifr);
    if (ret == 0) {
        strcpy(local_ip, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    } else {
        LOG_W("get_local_ip failed by errno:%d", errno);
    }

    close(fd);
    return ret;
}

static int get_p2p_interface_ip(char* local_ip)
{
    const char* p2p_interfaces[] = {"p2p-wlan0-0", "p2p-wlan0-1", "p2p-wlan0-2"};
    for (int i = 0; i < 3; i++) {
        int ret = get_local_ip(p2p_interfaces[i], local_ip);
        if (ret == 0) {
            LOG_D("Found P2P interface: %s, IP: %s", p2p_interfaces[i], local_ip);
            return 0;
        }
    }
    return -1;
}

int set_system_time(uint32_t timestamp) {
    struct timespec ts;
    ts.tv_sec = timestamp;
    ts.tv_nsec = 0;
    if (clock_settime(CLOCK_REALTIME, &ts) == -1) {
        LOGE("clock_settime failed");
        return 1;
    }
    set_time_synced(true);
    return 0;
}

// void read_file_data(int fd, uint8_t* data, uint32_t length) {
//     uint32_t offset_bytes = 0;
//     while (offset_bytes < length) {
//         int32_t read_bytes = read(fd, data + offset_bytes, 4096);
//         if (read_bytes <= 0) {
//             break;
//         }
//         offset_bytes += read_bytes;
//     }
// }
}

WifiHandler::WifiHandler(UartService* service, GlassServiceDelegate* glass_delegate)
    : service_(service),
      glass_delegate_(glass_delegate) {
}

WifiHandler::~WifiHandler() {
}

bool WifiHandler::OnDataArrived(const uint8_t* data, uint16_t length) {
    UARTFramePtr frame_req = WifiHandler::ParseUartFrame(data, length);
    if (!frame_req) {
        return OnMediaCommandArrived(data, length);
    }

    UARTFramePtr frame_resp = NULL;
    bool enable_heartbeat = true;
    uint32_t payload_size = 0;
    LOGD("OnDataArrived:0x%02x, data length:%u", frame_req->command, frame_req->length);
    switch (frame_req->command) {
    case APP_CMD_SYNC_TIME:{
        enable_heartbeat = false;
        char buffer[32] = {0};
        int32_t timestamp = 0;
        memcpy(buffer, frame_req->data, frame_req->length);
        buffer[frame_req->length] = '\0';
        timestamp = std::atoi(buffer);
        LOGI("set_system_time: received timestamp string='%s', parsed=%d", buffer, timestamp);
        set_system_time(timestamp);
        break;
    }

    case APP_CMD_CONFIG_WIFI: {
        char local_ip[32] = {0};
        usleep(2000 * 1000);
        int ret = get_local_ip(WIFI_INTERFACE_NAME, local_ip);
        if (ret != 0) {
            char buffer[64];
            memcpy(buffer, frame_req->data, frame_req->length);
            buffer[frame_req->length] = '\0';
            wifi_config(buffer, service_);
            ret = get_local_ip(WIFI_INTERFACE_NAME, local_ip);
        }

        LOG_D("local_ip: %s", local_ip);
        bool connect_result = (ret == 0);
        LOG_D("connect_result: %d", connect_result);
        payload_size = 1;
        frame_resp = WifiHandler::MakeUartFrame(frame_req, payload_size);
        frame_resp->data[0] = connect_result;
        break;
    }

    case APP_CMD_QUERY_FILE_LIST: {
        ResponseFileList(frame_req);
        break;
    }

    case APP_CMD_FORMAT_SD_CARD: {
        bool result = UartHelper::FormatSDCard();
        payload_size = 1;
        frame_resp = WifiHandler::MakeUartFrame(frame_req, payload_size);
        frame_resp->data[0] = result;
        LOGD("APP_CMD_FORMAT_SD_CARD result:%d", result);
        break;
    }

    case APP_CMD_OAT_STARTUP: {
        std::string url((char*)frame_req->data + 1, frame_req->length - 1);
        uint8_t device = frame_req->data[0];
        LaunchWifiAndSocketServer();
        int32_t result = glass_delegate_->OnOTAStartup(device, url.data());
        LOGI("APP_CMD_OAT_STARTUP url:%s result:%d", url.data(), result);
        // TODO: response
        break;
    }

    case APP_CMD_RUN_AS_AP: {
        {
            char local_ip[32] = {0};
            int ret = 0;
            usleep(1000 * 1000);
            ret = get_local_ip(WIFI_INTERFACE_NAME, local_ip);
            if ((s_LaunchCount == 0) || (ret != 0)) {
                ++s_LaunchCount;
                std::string command_line = "bash /sbin/launch_wifi.sh " + std::to_string(s_LaunchCount);
                base::LaunchProcess(command_line);
            }

            ret = get_local_ip(WIFI_INTERFACE_NAME, local_ip);

            char ssid[128] = {0};
            memset(ssid, 0, sizeof(ssid));
            snprintf(ssid, sizeof(ssid), "%s/%s", s_ssid, local_ip);
            payload_size = strlen(ssid);
            frame_resp = WifiHandler::MakeUartFrame(frame_req, payload_size);
            memcpy(frame_resp->data, ssid, payload_size);
            LOG_D("ssid :%s", ssid);
            is_wifi_server_running_.store((ret == 0), std::memory_order_release);

            // usleep(4000 * 1000); // wait for ap mode ready
            int CLOSE_CAMERA = 12;
            XBUS_CMD_S arguments;
            int ret2 = FH_RPC_Customer_Init();
            ret2 = FH_RPC_Customer_Command(CLOSE_CAMERA, &arguments, sizeof(arguments));
            LOGD("FH_RPC_Customer_Command ret:0x%x.", ret2);
            FH_RPC_Customer_DeInit();
            glass_delegate_->OnStartWebService();
        }
        break;
    }

    case APP_CMD_TAKE_PHOTO: {
        glass_delegate_->OnTakePhoto();
        break;
    }
    case APP_CMD_VERSION: {
        payload_size = strlen(APP_VERSION);
        frame_resp = WifiHandler::MakeUartFrame(frame_req, payload_size);
        memcpy(frame_resp->data, APP_VERSION, payload_size);
        LOGD("APP_CMD_VERSION: %s", APP_VERSION);
        break;
    }
    case APP_CMD_CONFIRM_WIFICONFIG: {
        LOGD("APP_CMD_CONFIRM_WIFICONFIG");
        char config_ssid[128] = {0};
        bool has_ssid = false;
        int ret = read_ssid_from_config(config_ssid, sizeof(config_ssid));
        if (ret == 0 && strlen(config_ssid) > 0) {
            strncpy(s_ssid, config_ssid, sizeof(s_ssid) - 1);
            s_ssid[sizeof(s_ssid) - 1] = '\0';
            has_ssid = true;
        }

        if (has_ssid) {
            payload_size = strlen(s_ssid);
            frame_resp = WifiHandler::MakeUartFrame(frame_req, payload_size);
            memcpy(frame_resp->data, s_ssid, payload_size);
        } else {
            payload_size = 0;
            frame_resp = WifiHandler::MakeUartFrame(frame_req, payload_size);
            uint32_t total_length = UARTFrame_SIZE(payload_size);
            UartHelper::FillCRC((uint8_t*)frame_resp, total_length - kUartFrameCrcSize);
            service_->Uart2Send((const uint8_t*)frame_resp, total_length);
        }

        break;
    }
    case PRESS_BUTTON_START_RECORD: {
        glass_delegate_->OnStartRecording();
        enable_heartbeat = false;
        break;
    }
    case PRESS_BUTTON_STOP_RECORD: {
        glass_delegate_->OnStopRecording();
        break;
    }
    case APP_CMD_LAUNCH_P2P: {
        {
            if (is_p2p_started_.load(std::memory_order_acquire)) {
                LOGW("P2P already started");
                break;
            }
            char local_ip[32] = {0};
            int ret = 0;
            is_p2p_started_.store(true, std::memory_order_release);
            if (s_LaunchCount == 0) {
                ++s_LaunchCount;
                std::string command_line = "bash /sbin/launch_p2p.sh " + std::to_string(s_LaunchCount);
                base::LaunchProcess(command_line);
                ret = get_p2p_interface_ip(local_ip);
            }

            is_wifi_server_running_.store((ret == 0), std::memory_order_release);

            int CLOSE_CAMERA = 12;
            XBUS_CMD_S arguments;
            int ret2 = FH_RPC_Customer_Init();
            ret2 = FH_RPC_Customer_Command(CLOSE_CAMERA, &arguments, sizeof(arguments));
            LOGD("FH_RPC_Customer_Command ret:%d.", ret2);
            FH_RPC_Customer_DeInit();
            // TCP server start
            glass_delegate_->OnStartWebService();

            // response server start
            if (ret == 0) {
                payload_size = 1;
                frame_resp = WifiHandler::MakeUartFrame(frame_req,payload_size);
                frame_resp->data[0] = 0x01;
                LOGE("response server start");
            }
        }
        break;
    }
    case APP_CMD_WIFI_IS_CONNECTED: {
        char local_ip[32] = {0};
        int ret = get_local_ip(WIFI_INTERFACE_NAME, local_ip);
        if (ret == 0) {
            payload_size = 1;
            frame_resp = WifiHandler::MakeUartFrame(frame_req, payload_size);
            frame_resp->data[0] = 0x01;
        } else {
            payload_size = 1;
            frame_resp = WifiHandler::MakeUartFrame(frame_req, payload_size);
            frame_resp->data[0] = 0x00;
        }
        break;
    }
    case APP_CMD_SET_RECORD_MAXTIME: {
        //todo update record maxtime
        LOGW("Need to Set Record MAX Time");
        break;
    }
    case APP_CMD_REQUEST_FILE_LIST: {
        auto names = RequestFileList(frame_req);
        SendFileListInBatch(frame_req, names);
        break;
    }
    case APP_CMD_SET_P2P_NAME: {
        std::string p2p_name((char*)frame_req->data, frame_req->length);
        LOGI("set_p2p_name: %s", p2p_name.data());
        glass_delegate_->SetP2PName(p2p_name);
        break;
    }
    default:
        LOGW("No handle command: %d", frame_req->command);
        break;
    }

    if (frame_resp == NULL) {
        return true;
    }
    if (payload_size == 0) {
        LOGW("Empty payload, command=%d", frame_req->command);
        FreeUartFrame(frame_resp);
        return true;
    }

    uint32_t total_length = UARTFrame_SIZE(payload_size);
    UartHelper::FillCRC((uint8_t*)frame_resp, total_length - kUartFrameCrcSize);
    service_->Uart2Send((const uint8_t*)frame_resp, total_length);
    HEX_PRINT(((uint8_t*)frame_resp), total_length);

    FreeUartFrame(frame_resp);
    frame_resp = NULL;

    return enable_heartbeat;
}


uint32_t WifiHandler::ResponseFileList(const UARTFramePtr frame_req) {
    uint32_t result = 0;
    uint16_t file_count = 0;
    static constexpr uint16_t kHeaderSize = 3;
    std::string path;
    glass_delegate_->OnRequestFileList(&file_count, path);
    {
        // send file count
        auto frame_resp = WifiHandler::MakeUartFrame(frame_req,  kHeaderSize);
        LOGD("local file count:%d", file_count);

        uint16_t sending_count = file_count;
        file_count = htons(file_count);
        memcpy(frame_resp->data, &file_count, 2);
        frame_resp->data[2] = (sending_count == 0);
        uint32_t total_length = UARTFrame_SIZE(kHeaderSize);
        UartHelper::FillCRC((uint8_t*)frame_resp, total_length - kUartFrameCrcSize);
        service_->Uart2Send((const uint8_t*)frame_resp, total_length);
        FreeUartFrame(frame_resp);
    }
    return result;
}

std::vector<std::string> WifiHandler::RequestFileList(const UARTFramePtr frame_req) {
    const auto& path_list = FileEnumerator::FetchAll(
        SD_CARD_DATA_ROOT,
        {MEDIA_TYPE_MP4, MEDIA_TYPE_JPEG});
    std::vector<std::string> names;
    names.reserve(path_list.size());
    for (const auto& path : path_list) {
        FilePath file_path(path);
        std::string name = file_path.BaseName();
        if (name.empty()) {
            continue;
        }
        names.push_back(name);
    }
    return names;
}

void WifiHandler::SendFileListInBatch(const UARTFramePtr frame_req,
                                        const std::vector<std::string>& names) {
    static constexpr size_t kFilesPerPacket = 20;
    if (names.empty()) {
        uint32_t payload_size = 0;
        auto frame_resp = WifiHandler::MakeUartFrame(DEV_ID_MASTER, DEV_ID_APP,
                                                     frame_req->command, payload_size);
        uint32_t total_length = UARTFrame_SIZE(payload_size);
        UartHelper::FillCRC((uint8_t*)frame_resp, total_length - kUartFrameCrcSize);
        service_->Uart2Send((const uint8_t*)frame_resp, total_length);
        FreeUartFrame(frame_resp);
        return;
    }

    for (size_t i = 0; i < names.size(); i += kFilesPerPacket) {
        std::string chunk;
        size_t end = std::min(i + kFilesPerPacket, names.size());
        for (size_t j = i; j < end; ++j) {
            if (!chunk.empty()) {
                chunk.push_back(',');
            }
            chunk.append(names[j]);
        }
        uint32_t payload_size = chunk.size();
        auto frame_resp = WifiHandler::MakeUartFrame(DEV_ID_MASTER, DEV_ID_APP,
                                                     frame_req->command, payload_size);
        if (payload_size > 0) {
            memcpy(frame_resp->data, chunk.data(), payload_size);
        }
        sleep(1);
        uint32_t total_length = UARTFrame_SIZE(payload_size);
        UartHelper::FillCRC((uint8_t*)frame_resp, total_length - kUartFrameCrcSize);
        service_->Uart2Send((const uint8_t*)frame_resp, total_length);
        FreeUartFrame(frame_resp);
    }
}

UARTFramePtr WifiHandler::MakeUartFrame(uint8_t sender_id, uint8_t receiver_id, uint16_t command, uint16_t length) {
    UARTFramePtr frame = static_cast<UARTFramePtr>(malloc(kUartFrameHeaderSize + length + kUartFrameCrcSize));
    frame->SOF = UARTFRAME_SOF;
    frame->command = command;
    frame->sender_id = sender_id;
    frame->receiver_id = receiver_id;
    frame->length = htons(length);

    return frame;
}


UARTFramePtr WifiHandler::MakeUartFrame(const UARTFramePtr req, uint16_t length) {
    return MakeUartFrame(req->receiver_id, req->sender_id, req->command, length);
}

const UARTFramePtr WifiHandler::ParseUartFrame(const uint8_t* data, uint16_t length) {
    const UARTFramePtr frame = (const UARTFramePtr)data;
    if (frame->SOF != UARTFRAME_SOF) {
        return nullptr;
    }

    frame->length = ntohs(frame->length);

    // TODO: maybe is needed
    // check crc16
    // uint16_t crc16_data = UartHelper::CRC16(data, (UARTFrame_SIZE(frame->length) - 2));
    // uint16_t crc16_frame = 0;
    // memcpy(&crc16_frame, frame->data + frame->length, WifiHandler::kUartFrameCrcSize);
    // if (crc16_data != crc16_frame) {
    //     LOGW("CRC check error!!src:%02x dest_cle!%02x", crc16_frame, crc16_data);
    // }

    return frame;
}

void WifiHandler::FreeUartFrame(UARTFramePtr frame) {
    if (frame) {
        free(frame);
    }
}

int32_t WifiHandler::OnMediaCommandArrived(const uint8_t* data, uint16_t length) {
    if (memcmp(data, kMediaCommandTakePhoto, length) == 0) {
        service_->SetWakeupReason(WakeupReason::TakePhoto);
        glass_delegate_->OnTakePhoto();
    } else if (memcmp(data, kMediaCommandStartRecord, length) == 0) {
        service_->SetWakeupReason(WakeupReason::StartRecord);
        glass_delegate_->OnStartRecording();
    } else if (memcmp(data, kMediaCommandStopRecord, length) == 0) {
        service_->SetWakeupReason(WakeupReason::StopRecord);
        glass_delegate_->OnStopRecording();
    }
    return 0;
}

void WifiHandler::StopRecord(void) {
    glass_delegate_->OnStopRecording();
    glass_delegate_->OnCommandEnd();
}

bool WifiHandler::IsWaitingForFileDownload() {
    if (is_wifi_server_running_.load(std::memory_order_acquire)) {
         uint16_t file_count = 0;
        std::string path;
        glass_delegate_->OnRequestFileList(&file_count, path);
        return (file_count > 0);
    }
    return false;
}

int32_t WifiHandler::LaunchWifiAndSocketServer() {
    char local_ip[32] = {0};
    int ret = 0;
    ret = get_local_ip(WIFI_INTERFACE_NAME, local_ip);
    if ((s_LaunchCount == 0) || (ret != 0)) {
        ++s_LaunchCount;
        std::string command_line = "bash /sbin/launch_wifi.sh " + std::to_string(s_LaunchCount);
        base::LaunchProcess(command_line);
        ret = get_local_ip(WIFI_INTERFACE_NAME, local_ip);
    }

    if (ret == 0) {
        glass_delegate_->OnStartWebService();
        LOG_D("local_ip: %s", local_ip);
    } else {
        LOGW("Launch WIFI failed.");
    }

    return ret;

}

