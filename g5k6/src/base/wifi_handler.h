#ifndef BASE_WIFI_HANDLER_H_
#define BASE_WIFI_HANDLER_H_

#include "base/app_common_defs.h"
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
#include <sys/types.h>
#include <queue>
#include <mutex>
#include <condition_variable>


class UartService;
class GlassServiceDelegate;

typedef struct {
    int fd;
    std::vector<uint8_t> data;
} UartCommandData, *UartCommandDataPtr;

class WifiHandler {
public:
    static constexpr uint16_t kUartFrameHeaderSize = sizeof(UARTFrame);
    static constexpr uint16_t kUartFrameCrcSize = sizeof(uint16_t);
    static constexpr uint16_t kUartMaxFramelength = 512;
    static constexpr uint16_t kUartMaxPayloadlength = kUartMaxFramelength - kUartFrameHeaderSize - kUartFrameCrcSize;

public:
    WifiHandler(UartService* service, GlassServiceDelegate* glass_delegate);
    ~WifiHandler();

public:
    bool OnDataArrived(const uint8_t* data, uint16_t length);
    void StopRecord(void);
    bool IsWaitingForFileDownload();

public:
    static UARTFramePtr MakeUartFrame(uint8_t sender_id, uint8_t receiver_id, uint16_t command, uint16_t length);
    static UARTFramePtr MakeUartFrame(const UARTFramePtr req, uint16_t length);
    static const UARTFramePtr ParseUartFrame(const uint8_t* data, uint16_t length);
    static void FreeUartFrame(UARTFramePtr frame);


private:
    int32_t OnMediaCommandArrived(const uint8_t* data, uint16_t length);
    uint32_t ResponseFileList(const UARTFramePtr frame_req);
    std::vector<std::string> RequestFileList(const UARTFramePtr frame_req);
    void SendFileListInBatch(const UARTFramePtr frame_req,
                               const std::vector<std::string>& names);
    int32_t LaunchWifiAndSocketServer();

private:
    UartService* service_{nullptr};
    GlassServiceDelegate* glass_delegate_{nullptr};
    std::atomic_bool is_wifi_server_running_{false};
    std::atomic_bool is_p2p_started_{false};
};

#endif  // BASE_WIFI_HANDLER_H_
