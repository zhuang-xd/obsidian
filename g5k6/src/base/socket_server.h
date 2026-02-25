#ifndef BASE_SOCKET_SERVER_H_
#define BASE_SOCKET_SERVER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <set>
#include <vector>
#include "base/app_common_defs.h"

class GlassServiceDelegate;
class TransferHandler;


struct TcpCommandTaskData {
    int fd{-1};
    TransferFrame frame{};
    std::vector<uint8_t> payload;
    TransferHandler* handler{nullptr};
    GlassServiceDelegate* delegate{nullptr};
};


class SocketServer {
public:
    SocketServer(uint16_t port=SERVER_PORT);
    ~SocketServer();

public:
    int32_t Start(const char* path, GlassServiceDelegate* glass_delegate);
    int32_t Stop(uint32_t timeout=2000);

    int32_t Send(int fd, uint8_t* data, int32_t length);
    int32_t SendFile(int client_fd, int file_fd);
    int32_t WriteFile(int client_fd, int file_fd);
    int32_t PerformanceOTA(int device, const char* url);
    void SendFileHeartBeat(void);
    void UpdateHeartbeat(void);
    void DownloadLogFile(void);

    bool IsRunning(void) {
        return running_;
    }

    void StopHeartbeat(void);

    bool IsTransfering(void) {
        return send_file_heartbeat_;
    }

    bool send_file_down_{false};

private:
    void ThreadMain(void);
    int CreateAndListen(void);
    void AcceptNewConnection(void);
    void HandleClientEvent(int client_fd);

private:
    uint16_t port_{SERVER_PORT};
    int server_fd_{-1};
    bool running_{false};
    bool send_file_heartbeat_{false};
    uint64_t last_heartbeat_time_{0};

    std::unique_ptr<std::thread> thread_;
    std::unique_ptr<std::thread> send_heart_thread_;
    std::set<int> client_fds_;
    std::mutex heartbeat_mutex_;
    std::string record_root_path_;
    GlassServiceDelegate* glass_delegate_{nullptr};
    std::unique_ptr<TransferHandler> transfer_handle_;
};

#endif  // BASE_SOCKET_SERVER_H_
