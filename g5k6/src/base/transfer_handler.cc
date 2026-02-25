#include "base/transfer_handler.h"
#include "base/uart_helper.h"
#include "base/app_common_defs.h"
#include "base/file_path.h"
#include "base/glass_service_delegate.h"
#include "base/logging.h"
#include "base/sequence_manager.h"
#include "base/socket_server.h"
#include "base/file_enumerator.h"
#include "base/mtd_upgrade.h"


#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <string>
#include <unistd.h>
#include <vector>


#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <functional>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

namespace {
static constexpr uint16_t MaxFilePathSize = 256;
static constexpr uint16_t DefaultCRC = TRANSFER_FRAME_MAGIC_NUBER;
static std::vector<std::string> kMediaExtensions = {MEDIA_TYPE_MP4, MEDIA_TYPE_JPEG};



class HttpDownloader {
public:
    using ProgressCallback = std::function<void(int percentage, const char* message)>;

    HttpDownloader() : timeout(30) {}

    void SetProgressCallback(ProgressCallback callback) {
        progress_callback_ = callback;
    }

    bool download(const std::string& url, const std::string& filepath) {
        std::string host;
        std::string path;
        std::string port;
        parseUrl(url, host,port, path);

        int sockfd = createConnection(host, port);
        if (sockfd < 0) return false;

        if (!sendRequest(sockfd, host, path)) {
            close(sockfd);
            return false;
        }

        if (!handleResponse(sockfd, filepath)) {
            close(sockfd);
            return false;
        }

        close(sockfd);
        return true;
    }

private:
    struct Header {
        std::string key;
        std::string value;
    };

    int timeout;
    ProgressCallback progress_callback_ = nullptr;

    void parseUrl(const std::string& url, std::string& host, std::string& port, std::string& path) {
        size_t protocol_end = url.find("://");
        if (protocol_end == std::string::npos) return;

        size_t host_start = protocol_end + 3;
        size_t path_start = url.find('/', host_start);
        port = "80";
        if (path_start == std::string::npos) {
            host = url.substr(host_start);
            path = "/";
        } else {
            host = url.substr(host_start, path_start - host_start);
            size_t port_start = host.rfind(':');
            if (port_start != std::string::npos) {
                port = host.substr(port_start + 1);
                host = host.substr(0, port_start);
            }
            path = url.substr(path_start);
        }
    }

    int createConnection(const std::string& host, const std::string& port) {
        struct addrinfo hints, *res;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(host.c_str(), port.c_str(), &hints, &res) != 0) {
            LOG_E("getaddrinfo fail:%s, %s, %d", host.c_str(), port.c_str(), errno);
            return -1;
        }

        int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0) {
            LOG_E("socket fail");
            freeaddrinfo(res);
            return -1;
        }

        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

        if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
            LOG_E("connect fail");
            close(sockfd);
            freeaddrinfo(res);
            return -1;
        }

        freeaddrinfo(res);
        return sockfd;
    }

    bool sendRequest(int sockfd, const std::string& host, const std::string& path) {
        std::string request = "GET " + path + " HTTP/1.1\r\n" +
                             "Host: " + host + "\r\n" +
                             "Connection: close\r\n\r\n";
        ssize_t sent = send(sockfd, request.c_str(), request.size(), 0);
        return sent == (ssize_t)request.size();
    }

    bool handleResponse(int sockfd, const std::string& filepath) {
        char buffer[4096];
        std::vector<Header> headers;
        bool header_end = false;
        std::string content_length_str;
        size_t total_received = 0;
        size_t total_size = 0;
        int last_percentage = 0;
        static constexpr int kProgressUpdateInterval = 10; // percentage 10 update once

        int file_fd = open(filepath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (file_fd < 0) return false;

        if (progress_callback_) {
            progress_callback_(0, "start download");
        }

        while (true) {
            ssize_t bytes_read = recv(sockfd, buffer, sizeof(buffer), 0);
            if (bytes_read <= 0) break;

            if (!header_end) {
                std::string chunk(buffer, bytes_read);
                size_t header_end_pos = chunk.find("\r\n\r\n");
                if (header_end_pos != std::string::npos) {
                    std::string header_part = chunk.substr(0, header_end_pos);
                    parseHeaders(header_part, headers);
                    header_end = true;

                    // 处理重定向
                    auto it = std::find_if(headers.begin(), headers.end(),
                                         [](const Header& h) { return h.key == "Location"; });
                    if (it != headers.end()) {
                        close(file_fd);
                        return download(it->value, filepath);
                    }

                    // 获取Content-Length
                    auto content_length_it = std::find_if(headers.begin(), headers.end(),
                                         [](const Header& h) {
                                             std::string key = h.key;
                                             std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                                             return key == "content-length";
                                         });
                    if (content_length_it != headers.end()) {
                        total_size = std::stoull(content_length_it->value);
                        LOG_D("total_size: %zu bytes (%.2f KB)", total_size, total_size / 1024.0);
                    } else {
                        LOG_E("parse Content-Length failed: %s", content_length_it->value.c_str());
                    }

                    size_t data_size = chunk.size() - header_end_pos - 4;
                    write(file_fd, chunk.substr(header_end_pos + 4).c_str(), data_size);
                    total_received += data_size;
                } else {
                    write(file_fd, buffer, bytes_read);
                }
            } else {
                // 写入文件数据
                write(file_fd, buffer, bytes_read);
                total_received += bytes_read;
            }

            // update progress
            if (progress_callback_ && total_size > 0) {
                int percentage = (total_received * 50) / total_size;
                if (percentage > 50) {
                    percentage = 50;
                }

                if (percentage >= last_percentage + kProgressUpdateInterval ||
                    total_received >= total_size) {
                    char message[64];
                    snprintf(message, sizeof(message), "downloading: %zu/%zu KB",
                            total_received / 1024, total_size / 1024);
                    progress_callback_(percentage, message);
                    last_percentage = percentage;
                }
            } else if (progress_callback_ && total_size == 0 && total_received > 0) {
                char message[64];
                snprintf(message, sizeof(message), "downloading: %zu KB", total_received / 1024);
                progress_callback_(25, message);
            }
        }

        close(file_fd);

        if (progress_callback_ && total_received > 0) {
            progress_callback_(50, "download completed");
        }

        return total_received > 0;
    }

    void parseHeaders(const std::string& header_str, std::vector<Header>& headers) {
        std::istringstream stream(header_str);
        std::string line;
        while (std::getline(stream, line) && line != "\r") {
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                Header h;
                h.key = line.substr(0, colon_pos);
                h.value = line.substr(colon_pos + 2, line.size() - colon_pos - 3); // 去除 \r
                headers.push_back(h);
            }
        }
    }
};


}  // namespace


TransferHandler::TransferHandler(SocketServer* server, GlassServiceDelegate* glass_delegate, const std::string& path)
    : server_(server),
      glass_delegate_(glass_delegate),
      record_root_path_(path) {
    thumb_root_path_ = record_root_path_ + "/" + THUMB_IMG_DIR_NAME;
}

TransferHandler::~TransferHandler() {
    // TODO: check and close client
}

void TransferHandler::InitFrameHeader(TransferFramePtr frame, AppCommand command, uint32_t data_length, uint16_t extern_data_length) {
    memset(frame, 0, FrameHeaderSize);
    frame->magic =  htons(FrameMagicNuber);
    frame->version = TRANSFER_FRAME_VERSION;
    frame->command = htons(command);
    frame->external_length = htons(extern_data_length);
    frame->total_length = htonl(FrameHeaderSize + data_length + FrameCRCSize);
}

int32_t TransferHandler::OnDataArrived(int fd, TransferFramePtr frame, uint8_t* payload) {
    uint16_t payload_size = frame->total_length - FrameHeaderSize - FrameCRCSize;
    if (payload_size > frame->total_length) {
        LOG_E("Invalid command:%d payload size:%d", frame->command, payload_size);
        return -1;
    }

    switch (frame->command) {
    case APP_COMMAND_SEND_FILE:{
        ResponseFile(fd, frame, payload);
        break;
    }

    case APP_COMMAND_SEND_FILE_LIST:{
        ResponseFileList(fd, frame, payload);
        break;
    }

    case APP_COMMAND_DOWNLOAD_RTTHEAD_URL:{
        std::string status = "failed";
        int ret = -1;
        if (payload_size > 0) {
            char *url = (char*)payload;
            url[payload_size] = '\0';
            ret = ARCHOTA(url);
            if (ret == 0) {
                status = "success";
                ret = 0;
            }
        } else {
            LOG_E("Invalid command:%d payload size:%d", frame->command, payload_size);
        }
        server_->Send(fd, (uint8_t*)status.data(), status.size());
        glass_delegate_->OnARCHUpgraded(ret);
        break;
    }

    case APP_COMMAND_DOWNLOAD_LINUX_URL:{
        std::string status = "failed";
        int ret = -1;
        if (payload_size > 0) {
            char *url = (char*)payload;
            url[payload_size] = '\0';
            ret = LinuxOTA(url);
            if (ret == 0) {
                status = "success";
            }
        } else {
            LOG_E("Invalid command:%d payload size:%d", frame->command, payload_size);
        }
        server_->Send(fd, (uint8_t*)status.data(), status.size());
        glass_delegate_->OnLinuxUpgraded(ret);
        break;
    }

    case APP_COMMAND_HEARTBEAT:{
        server_->UpdateHeartbeat();
        LOGD("start send file heartbeat");
        break;
    }

    case APP_COMMAND_SEND_FILE_DOWN:{
        LOGI("app_command_send_file_down");
        server_->StopHeartbeat();
        break;
    }

    case APP_COMMAND_DOWNLOAD_LOG_FILE:{
        server_->DownloadLogFile();
        break;
    }
    default:
        LOG_E("OnDataArrived command:%d", frame->command);
        break;
    }

    return  0;
}

int32_t TransferHandler::ResponseFile(int fd, TransferFramePtr sourceFrame, uint8_t* payload) {
    if (sourceFrame->total_length - FrameHeaderSize <= 0) {
        LOG_E("total_length, total_length:%d", sourceFrame->total_length);
        return -1;
    }

    uint16_t payload_length = sourceFrame->total_length - TransferHandler::FrameHeaderSize;
    uint16_t crc = 0;
    uint16_t file_name_length = payload_length - FrameCRCSize;
    memcpy(&crc, payload + file_name_length, FrameCRCSize);
    payload[file_name_length] = '\0';
    const char* file_name = (const char*)payload;

    LOG_D("request file:%s", file_name);

    char file_path[MaxFilePathSize] = { 0 };
    std::sprintf(file_path, "%s/%s", record_root_path_.data(), file_name);

    int file_fd = open(file_path, O_RDONLY);
    if (file_fd <= 0) {
        LOG_W("Open file failed, file_path:%s errno:%d", file_path, errno);
    }

    struct stat file_stat = { 0 };
    memset(&file_stat, 0, sizeof(file_stat));
    if (file_fd > 0) {
        fstat(file_fd, &file_stat);
    }

    TransferFrame targetFrame = CreateTransferFrame(APP_COMMAND_SEND_FILE, file_stat.st_size + file_name_length, file_name_length);
    targetFrame.target_id = sourceFrame->source_id;
    targetFrame.source_id = sourceFrame->target_id;

    int32_t sent_bytes = server_->Send(fd, (uint8_t*)&targetFrame, FrameHeaderSize);
    if (sent_bytes <= 0) {
        LOG_E("Send data error");
        close(file_fd);
        return -1;
    }

    int32_t once_sent_bytes = server_->Send(fd, (uint8_t*)file_name, file_name_length);
    if (file_fd > 0) {
        once_sent_bytes = server_->SendFile(fd, file_fd);
        close(file_fd);
        if (once_sent_bytes > 0) {
            sent_bytes+= once_sent_bytes;
        }

        std::string extension = FilePath(file_name).Extension();
        if (extension == MEDIA_TYPE_JPEG || extension == MEDIA_TYPE_MP4) {
            int ret = remove(file_path);
            std::string thumbnail_path = thumb_root_path_ + "/" + file_name;
            thumbnail_path = thumbnail_path.substr(0, thumbnail_path.size() - 4) + MEDIA_TYPE_JPEG;
            remove(thumbnail_path.data());
            UartHelper::SyncToDisc();
            LOG_D("Remove file:%s, ret=%d, error:%d", file_path, ret, errno);
        }
    }

    // TODO: general data crc
    crc = htons(DefaultCRC);
    once_sent_bytes = server_->Send(fd, (uint8_t*)&crc, FrameCRCSize);
    if (once_sent_bytes <= 0) {
        LOG_E("Sending CRC failed.");
        return -1;
    }

    sent_bytes += once_sent_bytes;
    LOG_I("Sending file:%s data:%d", file_name, sent_bytes);
    return sent_bytes;
}

int32_t TransferHandler::ResponseFileList(int fd, TransferFramePtr sourceFrame, uint8_t* payload) {
    std::string file_name_data;
    std::string root = record_root_path_;
    const auto& path_list = FileEnumerator::FetchAll(root, kMediaExtensions);
    for (const auto& path : path_list) {
        if (!file_name_data.empty()) {
            file_name_data.append(",");
        }
        file_name_data.append(FilePath(path).BaseName());
    }

    uint16_t data_length = file_name_data.length();
    TransferFrame targetFrame = CreateTransferFrame(APP_COMMAND_SEND_FILE_LIST, data_length, 0);
    targetFrame.target_id = sourceFrame->source_id;
    targetFrame.source_id = sourceFrame->target_id;

    int32_t sent_bytes = 0;
    int32_t once_sent_bytes = server_->Send(fd, (uint8_t*)&targetFrame, FrameHeaderSize);
    if (once_sent_bytes <= 0) {
        LOG_E("Send data error");
        return -1;
    }
    sent_bytes += once_sent_bytes;
    if (data_length > 0) {
        once_sent_bytes  = server_->Send(fd, (uint8_t*)file_name_data.data(), data_length);
        if (once_sent_bytes <= 0) {
            return -1;
        }
    }

    sent_bytes += once_sent_bytes;

    uint16_t crc = DefaultCRC;
    once_sent_bytes = server_->Send(fd, (uint8_t*)&crc, FrameCRCSize);
    if (once_sent_bytes <= 0) {
        LOG_E("Sending CRC failed.");
        return -1;
    }

    sent_bytes += once_sent_bytes;
    return sent_bytes;
}

int32_t TransferHandler::ARCHOTA(const char* url) {
    return UpgradeDevice(ARCH_DEVICE, url, ARCH_IMAGE_NAME);
}

int32_t TransferHandler::LinuxOTA(const char* url) {
    return UpgradeDevice(KERNEL_DEVICE, url, KERNEL_IMAGE_NAME);
}

int32_t TransferHandler::CombinedOTA(const char* url) {
    return UpgradeCombined(url, COMBINED_OTA_PACKAGE_NAME);
}

int32_t TransferHandler::UpgradeCombined(const char* url, const char* image_name) {
    static const char* kImageNames[2] = {
        ARCH_IMAGE_NAME,
        KERNEL_IMAGE_NAME
    };
    static const char* kDevices[2] = {
        ARCH_DEVICE,
        KERNEL_DEVICE
    };

    std::string package_path = record_root_path_ + "/" + COMBINED_OTA_PACKAGE_NAME;
    std::string extract_path = record_root_path_;
    LOG_S("UpgradeCombined url %s startup", url);

    int last_sent_progress = -1;
    constexpr int kProgressUpdateInterval = 5;
    auto sendProgressIfNeeded = [this, &last_sent_progress](
            int stage, int percentage, const char* message) {
        if (!glass_delegate_) return;

        if (percentage < 0 || percentage == 0 || percentage == 100) {
            glass_delegate_->OnOTAProgress(stage, percentage, message);
            if (percentage >= 0) {
                last_sent_progress = percentage;
            }
            return;
        }

        if (last_sent_progress < 0 ||
            (percentage - last_sent_progress) >= kProgressUpdateInterval) {
            glass_delegate_->OnOTAProgress(stage, percentage, message);
            last_sent_progress = percentage;
        }
    };

    // ========== first download combined image.tar.gz ==========
    HttpDownloader downloader;
    downloader.SetProgressCallback([&sendProgressIfNeeded](int percentage, const char* message) {
        //  0-30%
        int overall = (percentage * 30) / 100;
        sendProgressIfNeeded(0, overall, message);
    });

    int ret = 0;
    for (int attempt = 1; attempt <= 3; ++attempt) {
        LOG_I("Combined image package download %s startup, attempt:%d",
              package_path.c_str(), attempt);
        if (!downloader.download(url, package_path)) {
            LOG_E("Download combined image package failed: %s, attempt:%d", url, attempt);
            return -1;
        }

        // ========== second extract combined image.tar.gz ==========
        sendProgressIfNeeded(1, 30, "extracting package");
        std::string extract_cmd = "tar -xf " + package_path + " -C " + extract_path;
        ret = system(extract_cmd.c_str());
        if (ret == 0) {
            sendProgressIfNeeded(1, 35, "extract completed");
            break;
        }

        LOG_E("Extract combined image package failed, ret=%d, attempt:%d", ret, attempt);
        remove(package_path.c_str());
        if (attempt == 3) {
            return -1;
        }
    }

    // ========== third upgrade arch and kernel images ==========
    int32_t result = 0;
    const int kProgressBase = 35;
    const int kProgressRemaining = 100 - kProgressBase;
    const int kProgressPerImage = kProgressRemaining / COMBINED_OTA_IMAGE_COUNT;

    for (int i = 0; i < COMBINED_OTA_IMAGE_COUNT; i++) {
        std::string image_path = extract_path + "/" + kImageNames[i];
        int progress_start = kProgressBase + (i * kProgressPerImage);
        int progress_end = (i == COMBINED_OTA_IMAGE_COUNT - 1) ?
                           100 : (kProgressBase + ((i + 1) * kProgressPerImage));

        const char* device_type = (i == 0 ? "arch" : "kernel");
        const char* current_device = kDevices[i];
        const int stage = i + 2;

        MTDUpgrade::ProgressCallback write_callback =
        [progress_start, progress_end, stage, device_type, &sendProgressIfNeeded](
                int percentage, const char* message) -> void {
            int overall = progress_start + ((percentage * (progress_end - progress_start)) / 100);
            if (overall > 100) overall = 100;
            char progress_msg[128];
            snprintf(progress_msg, sizeof(progress_msg), "upgrading %s: %s",
                    device_type, message);
            sendProgressIfNeeded(stage, overall, progress_msg);
        };

        LOG_I("Starting upgrade: device=%s, image=%s, progress_range=%d-%d%%",
              current_device, image_path.c_str(), progress_start, progress_end);

        result = MTDUpgrade::UpgradeDevice(current_device, image_path.c_str(), write_callback);
        if (result != 0) {
            LOG_E("Upgrade device %s failed with image %s", kDevices[i], image_path.c_str());
            char error_msg[128];
            snprintf(error_msg, sizeof(error_msg), "upgrade %s failed", device_type);
            sendProgressIfNeeded(stage, -1, error_msg);
            break;
        }
        LOG_S("Upgrade device %s successfully", kDevices[i]);
    }

    if (result == 0) {
        LOG_S("UpgradeCombined successfully - all devices upgraded");
        sendProgressIfNeeded(COMBINED_OTA_IMAGE_COUNT + 1, 100, "OTA completed");
    } else {
        LOG_E("UpgradeCombined failed - some devices upgrade failed");
        sendProgressIfNeeded(COMBINED_OTA_IMAGE_COUNT + 1, -1, "OTA failed");
    }

    // ========== clean up temporary files ==========
    for (int i = 0; i < COMBINED_OTA_IMAGE_COUNT; i++) {
        std::string image_path = extract_path + "/" + kImageNames[i];
        remove(image_path.c_str());
    }
    remove(package_path.c_str());

    return result;
}

int32_t TransferHandler::UpgradeDevice(const char* device, const char* url, const char* image_name) {
    std::string filepath = record_root_path_ + "/" + image_name;
    LOG_S("UpgradeDevice device %s url %s startup", device, url);

    HttpDownloader downloader;

    downloader.SetProgressCallback([this](int percentage, const char* message) {
        if (glass_delegate_) {
            glass_delegate_->OnOTAProgress(0, percentage, message);
        }
    });

    LOG_I("Image download %s startup", filepath.c_str());
    if (downloader.download(url, filepath)) {
        LOG_I("Image download successfully:%s!", filepath.c_str());
    } else {
        LOG_E("Image download failed:%s!", url);
        if (glass_delegate_) {
            glass_delegate_->OnOTAProgress(0, -1, "download failed");
        }
        return -1;
    }

    MTDUpgrade::ProgressCallback write_callback = [this](int percentage, const char* message) {
        if (glass_delegate_) {
            int overall_percentage = 50 + (percentage * 50) / 100;
            glass_delegate_->OnOTAProgress(1, overall_percentage, message);
        }
    };

    int32_t result = MTDUpgrade::UpgradeDevice(device, filepath.c_str(), write_callback);
    if (result == 0) {
        LOG_S("UpgradeDevice device %s url %s successfully", device, url);
    } else {
        LOG_E("UpgradeDevice device %s url %s failed", device, url);
        if (glass_delegate_) {
            glass_delegate_->OnOTAProgress(1, -1, "write failed");
        }
    }
    return result;
}

TransferFrame TransferHandler::CreateTransferFrame(AppCommand command, uint32_t data_length, uint16_t extern_data_length) {
    TransferFrame frame;
    InitFrameHeader(&frame, command, data_length, extern_data_length);
    return frame;
}

TransferFramePtr TransferHandler::ParseDataToFrame(uint8_t* data, int32_t length) {
    if (data == 0) {
        return nullptr;
    }

    TransferFramePtr frame = (TransferFramePtr)data;
    frame->magic = ntohs(frame->magic);
    frame->version = ntohs(frame->version);
    frame->command = ntohs(frame->command);
    frame->source_id = ntohs(frame->source_id);
    frame->target_id = ntohs(frame->target_id);
    frame->reserved = ntohs(frame->reserved);
    frame->external_length = ntohs(frame->external_length);
    frame->total_length = ntohl(frame->total_length);

    return frame;
}
