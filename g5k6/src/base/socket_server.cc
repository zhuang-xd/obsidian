#include "base/socket_server.h"

#include "base/app_common_defs.h"
#include "base/logging.h"
#include "base/glass_service_delegate.h"
#include "base/transfer_file.h"
#include "base/transfer_handler.h"
#include "base/wifi_handler.h"
#include "base/uart_service.h"
#include "base/file_enumerator.h"
#include "base/file_path.h"
#include "base/sequence_manager.h"

#include <cstdint>
#include <cstdio>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory>
#include <cstring>
#include <vector>

namespace  {
inline void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

inline void set_nodelay(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return;
    fcntl(fd, F_SETFL, flags | O_NDELAY);
}

}


SocketServer::SocketServer(uint16_t port)
    : port_(port) {
}

SocketServer::~SocketServer() {
    Stop();
}

int32_t SocketServer::Start(const char* path, GlassServiceDelegate* glass_delegate) {
    if (running_) {
        LOG_W("Server is already running");
        return -1;
    }

    LOGC("Socket server is starting");

    record_root_path_ = path;
    glass_delegate_ = glass_delegate;
    transfer_handle_ = std::make_unique<TransferHandler>(this, glass_delegate_, record_root_path_);
    thread_ = std::make_unique<std::thread>(&SocketServer::ThreadMain, this);
    return 0;
}

    // 处理新连接
void SocketServer::AcceptNewConnection(void) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_E("Accept error");
        }
        return;
    }

    set_nonblocking(client_fd);
    set_nodelay(client_fd);
    client_fds_.emplace(client_fd);
    LOG_S("New client connected: %d", client_fd);
}

void SocketServer::HandleClientEvent(int client_fd) {
    uint8_t buffer[TransferHandler::FrameHeaderSize] = {0};
    int32_t bytes_read = HANDLE_EINTR(read(client_fd, buffer, TransferHandler::FrameHeaderSize));
    if (bytes_read == 0) {
        close(client_fd);
        client_fds_.erase(client_fd);
        return;
    }

    TransferFrame* frame = (TransferFrame*)TransferHandler::ParseDataToFrame(buffer, bytes_read);
    LOGD("Client arrived command: %d", frame->command);
    uint8_t paylaod[TransferHandler::DataBufferDefautSize] = {0};
    bytes_read = HANDLE_EINTR(read(client_fd, paylaod, TransferHandler::DataBufferDefautSize));
    if (bytes_read == 0) {
        close(client_fd);
        client_fds_.erase(client_fd);
        return;
    }
    if (bytes_read < 0) {
        LOG_E("Read payload error from client %d, errno:%d", client_fd, errno);
        return;
    }

    LOGD("Received from client %d, frame->total_length:%d, command=0x%x", client_fd, frame->total_length, frame->command);
    SequenceManager* sequence_manager = glass_delegate_ ? glass_delegate_->GetSequenceManager() : nullptr;
    auto task_data = std::make_unique<TcpCommandTaskData>();
    task_data->fd = client_fd;
    task_data->frame = *frame;
    task_data->payload.assign(paylaod, paylaod + bytes_read);
    task_data->handler = transfer_handle_.get();
    task_data->delegate = glass_delegate_;

    SequenceManager::Task task;
    task.func = [this](void* arg)->bool {
        std::unique_ptr<TcpCommandTaskData> data(static_cast<TcpCommandTaskData*>(arg));
        if (!data || !data->handler || !data->delegate) {
            return true;
        }
        data->delegate->OnCommandStart();
        data->handler->OnDataArrived(data->fd, &data->frame, data->payload.data());
        if (send_file_heartbeat_ && send_file_down_) {
            send_file_down_ = false;
        }
        data->delegate->OnCommandEnd();
        return true;
    };
    task.custom_data = task_data.release();
    sequence_manager->PostTask(task);
}

uint64_t now_time_us(void) {
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    uint64_t s = tv.tv_sec;
    return s * 1000 * 1000 + tv.tv_nsec / 1000;
}

void SocketServer::UpdateHeartbeat(void) {
    bool thread_running = (send_heart_thread_.get() != nullptr && send_file_heartbeat_);

    if (!thread_running) {
        if (send_heart_thread_.get() != nullptr && send_heart_thread_->joinable()) {
            send_heart_thread_->join();
            send_heart_thread_.reset();
        }
        send_heart_thread_ = std::make_unique<std::thread>(&SocketServer::SendFileHeartBeat, this);
    }
    last_heartbeat_time_ = now_time_us();
}

void SocketServer::SendFileHeartBeat(void) {
    LOGI("SendFileHeartBeat  Strat --------");
    running_ = true;
    send_file_heartbeat_ = true;
    last_heartbeat_time_ = now_time_us();
    static constexpr uint64_t kHeartbeatCheckInterval = 3 * 1000 * 1000;
    static constexpr uint64_t kHeartbeatTimeout = 15 * 1000 * 1000;
    glass_delegate_->OnCommandEnd();
    while(send_file_heartbeat_) {
        usleep(kHeartbeatCheckInterval / 10);

        uint64_t now = now_time_us();

        if (now - last_heartbeat_time_ > kHeartbeatTimeout) {
            send_file_heartbeat_ = false;
            LOGI("Heartbeat timeout, stop heartbeat");
            break;
        }
    }

    send_file_heartbeat_ = false;
    LOGI("SendFileHeartBeat thread exit");
    if (glass_delegate_) {
        glass_delegate_->PoweroffForce();
    }
}

void SocketServer::DownloadLogFile(void) {
    LOGI("DownloadLogFile start");

    std::string log_file_path = std::string(LOG_FILE_PATH) + "eq_glasses.log";
    std::string file_name = "eq_glasses.log";

    for (int client_fd : client_fds_) {
        if (client_fd <= 0) {
            continue;
        }

        LOG_I("Transferring log file: %s to client %d", file_name.c_str(), client_fd);

        int file_fd = open(log_file_path.c_str(), O_RDONLY);
        if (file_fd <= 0) {
            LOG_W("Open log file failed, file_path:%s errno:%d", log_file_path.c_str(), errno);
            continue;
        }

        struct stat file_stat = {0};
        memset(&file_stat, 0, sizeof(file_stat));
        if (fstat(file_fd, &file_stat) != 0) {
            LOG_E("Failed to get file stat: %s", log_file_path.c_str());
            close(file_fd);
            continue;
        }

        uint32_t total_data_length = file_stat.st_size;

        TransferFrame targetFrame = TransferHandler::CreateTransferFrame(
            APP_COMMAND_DOWNLOAD_LOG_FILE, total_data_length, 0);

        //send frame header
        int32_t sent_bytes = Send(client_fd, (uint8_t*)&targetFrame, TransferHandler::FrameHeaderSize);
        if (sent_bytes <= 0) {
            LOG_E("Send frame header error for file: %s", file_name.c_str());
            close(file_fd);
            continue;
        }

        //send log file
        int32_t once_sent_bytes = SendFile(client_fd, file_fd);
        close(file_fd);

        if (once_sent_bytes > 0) {
            sent_bytes += once_sent_bytes;
        } else {
            LOG_W("Send file content failed for file: %s", file_name.c_str());
            continue;
        }

        uint16_t crc = htons(TransferHandler::FrameMagicNuber);
        once_sent_bytes = Send(client_fd, (uint8_t*)&crc, TransferHandler::FrameCRCSize);
        if (once_sent_bytes <= 0) {
            LOG_E("Sending CRC failed for file: %s", file_name.c_str());
            continue;
        }

        sent_bytes += once_sent_bytes;
        LOG_I("Successfully transferred log file: %s, total bytes: %d", file_name.c_str(), sent_bytes);
    }

    LOGI("DownloadLogFile completed");
}

void SocketServer::StopHeartbeat(void) {
    LOGI("StopHeartbeat");
    send_file_heartbeat_ = false;
    send_file_down_ = true;
    glass_delegate_->OnCommandEnd();

    if (send_heart_thread_.get() && send_heart_thread_->joinable()) {
        send_heart_thread_->join();
        send_heart_thread_.reset();
    }
}

void SocketServer::ThreadMain(void) {
    prctl(PR_SET_NAME, "file_server");
    running_ = true;
    server_fd_ = CreateAndListen();
    set_nonblocking(server_fd_);

    while (running_) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd_, &read_fds);
        int max_fd = server_fd_;
        for (int fd : client_fds_) {
            FD_SET(fd, &read_fds);
            if (fd > max_fd) {
                max_fd = fd;
            }
        }

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 2000 * 1000;
        int ready = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            LOG_E("Select error, errno:%d", errno);
            break;
        }

        if (ready == 0) {
            continue;
        }

        if (FD_ISSET(server_fd_, &read_fds)) {
            AcceptNewConnection();
        }

        std::vector<int> ready_clients;
        ready_clients.reserve(client_fds_.size());
        for (int fd : client_fds_) {
            if (FD_ISSET(fd, &read_fds)) {
                ready_clients.push_back(fd);
            }
        }
        for (int fd : ready_clients) {
            HandleClientEvent(fd);
        }
    }

    if (!client_fds_.empty()) {
        for (auto item: client_fds_) {
            shutdown(item, SHUT_RDWR);
            close(item);
        }
    }

    close(server_fd_);
    server_fd_ = -1;

    running_ = false;
}

int SocketServer::CreateAndListen(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    LOG_I("Server create and listen.");

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        LOG_E("bind error, errno:%d", errno);
        close(fd);
        return -1;
    }

    if (listen(fd, SOMAXCONN) == -1) {
        LOG_E("Server listen failed.");
        close(fd);
        return -1;
    }

    LOG_I("Server have been start.");
    return fd;
}


int32_t SocketServer::Stop(uint32_t timeout) {
    if (!running_) {
        return 0;
    }

    running_ = false;
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }

    LOG_I("Server have been stop.");
    thread_.reset();
    return 0;
}

int32_t SocketServer::Send(int fd, uint8_t* data, int32_t length) {
    if (!running_) {
        LOG_E("Server have been shutdown.");
        return -1;
    }

    return send(fd, data, length, 0);
}


int32_t SocketServer::SendFile(int client_fd, int file_fd) {
    if (!running_) {
        LOG_E("Server have been shutdown.");
        return -1;
    }

    off_t offset = 0;
    size_t count = 1024;
    uint32_t sent_bytes = 0;
    while (count > 0) {
        count = HANDLE_EINTR(sendfile(client_fd, file_fd, &offset, 4096));
        if (count == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(5 * 1000);
                continue;
            } else {
                LOGE("write error: %s  %d", strerror(errno), errno);
                break;
            }
        }
        sent_bytes += count;
    }

    return sent_bytes;
}
int32_t SocketServer::WriteFile(int client_fd, int file_fd) {
    if (!running_) {
        LOG_E("Server have been shutdown.");
        return -1;
    }
    LOGI("WriteFile start\n");

    char buffer[8192];
    ssize_t bytes_read = 1;
    uint32_t sent_bytes = 0;
    while (bytes_read > 0) {
        bytes_read = HANDLE_EINTR(read(file_fd, buffer, sizeof(buffer)));
        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(5 * 1000);
                continue;
            } else {
                LOGE("read");
                break;
            }
        } else if (bytes_read > 0) {
            ssize_t total_written = 0;
            while (total_written < bytes_read) {
                ssize_t bytes_written = HANDLE_EINTR(write(client_fd, buffer + total_written, bytes_read - total_written));
                total_written += bytes_written;
            }
            if(total_written == bytes_read) {
                sent_bytes += total_written;
            }
        }
    }
    return sent_bytes;
}


int32_t SocketServer::PerformanceOTA(int device, const char* url) {
    TransferHandler handler(this, glass_delegate_, record_root_path_);
    if (device == 0) {
        return handler.ARCHOTA(url);
    } else {
        return handler.CombinedOTA(url);
    }
}
