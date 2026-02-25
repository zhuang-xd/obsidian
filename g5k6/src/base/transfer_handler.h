#ifndef BASE_TRANSFER_HANDLER_H_
#define BASE_TRANSFER_HANDLER_H_

#include "base/app_common_defs.h"
#include <cstdint>
#include <string>
#include <memory>
#include <thread>
class SocketServer;
class GlassServiceDelegate;

class TransferHandler {
public:
    TransferHandler(SocketServer* server, GlassServiceDelegate* glass_delegate, const std::string& path);
    ~TransferHandler();

public:
    using CRCType = uint16_t;
    static constexpr uint32_t MaxConnections = SERVER_MAX_CONNECTIONS;
    static constexpr uint16_t FrameMagicNuber = TRANSFER_FRAME_MAGIC_NUBER;
    static constexpr uint32_t FrameHeaderSize = sizeof(TransferFrame);
    static constexpr uint32_t FrameCRCSize = sizeof(CRCType);
    static constexpr uint32_t DataBufferDefautSize = 512;

public:
    static void InitFrameHeader(TransferFramePtr frame, AppCommand command, uint32_t data_length, uint16_t extern_data_length=0);
    static TransferFrame CreateTransferFrame(AppCommand command, uint32_t data_length, uint16_t extern_data_length=0);
    static TransferFramePtr ParseDataToFrame(uint8_t* data, int32_t length);

public:
    int32_t OnDataArrived(int fd, TransferFramePtr frame, uint8_t* paylaod);
    int32_t ARCHOTA(const char* url);
    int32_t LinuxOTA(const char* url);
    int32_t CombinedOTA(const char* url);

private:
    int32_t ResponseFile(int fd, TransferFramePtr frame, uint8_t* paylaod);
    int32_t ResponseFileList(int fd, TransferFramePtr frame, uint8_t* paylaod);
    int32_t UpgradeDevice(const char* device, const char* url, const char* image_name);
    int32_t UpgradeCombined(const char* url, const char* image_name);


private:
    SocketServer* server_{nullptr};
    GlassServiceDelegate* glass_delegate_{nullptr};
    std::unique_ptr<std::thread> send_heart_thread_{nullptr};
    std::string record_root_path_;
    std::string thumb_root_path_;

};

#endif  // __BASE__TRANSFER_HANDLER_H__
