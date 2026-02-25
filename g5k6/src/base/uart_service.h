#ifndef BASE_UART_SERVICE_H_
#define BASE_UART_SERVICE_H_

#include "base/wifi_handler.h"
#include "base/sequence_manager.h"

#include <cstdint>
#include <memory>
#include <thread>

class GlassServiceDelegate;

enum class WakeupReason:int8_t {
    Ble, // by App
    TakePhoto, // Take an photo by audio
    StartRecord, // Start Record by audio
    StopRecord, // Stop Record by audio
};

class UartService:
    public SequenceManager::Deletate {

public:
    UartService() = default;
    ~UartService() = default;

public:
    int32_t Send(int fd, const uint8_t* data, int32_t length);
    int32_t Uart1Send(const uint8_t* data, int32_t length);
    int32_t Uart2Send(const uint8_t* data, int32_t length);
    int32_t StartHeartbeat(void);

    int32_t StartUartService(GlassServiceDelegate* glass_delegate);
    int32_t StopUartService(void);
    void FastLaunchAndSendByBle(int32_t command, uint8_t* data=nullptr, uint16_t length=0);

    void UpdateHeartbeat(void);
    void SetRecordingState(bool is_recording);
    bool GetRecordingState(void);
    void SetWakeupReason(WakeupReason reason) {
        wakeup_reason_ = reason;
    }

    SequenceManager* GetSequenceManager(void) {
        return sequence_manager_.get();
    }

    void EnableHeartbeat(bool enable);

    void DoAfterTask(WakeupReason reason);
    uint32_t Restart(AppRestartReson reason);
    void PowerOff(void);

public:
     bool OnAllTaskFinished(void) override;

private:
    void HeartbeatThreadMain(void);
    void Uart1ThreadMain(void);
    void Uart2ThreadMain(void);
    void OnDataArrived(uint8_t* data, uint32_t length);

private:
    static int32_t SendHandshake(int fd);

private:
    // for receive audio
    int fd_uart1_{-1};

    // for BLE and wifi
    int fd_uart2_{-1};

    std::unique_ptr<std::thread> thread_heartbeat_;
    std::unique_ptr<std::thread> thread_uart1_;
    std::unique_ptr<std::thread> thread_uart2_;
    std::unique_ptr<std::thread> thread_command_;

    std::unique_ptr<WifiHandler> wifi_handler_;
    bool is_running_{false};
    volatile bool enable_heartbeat_{true};
    uint64_t record_start_time_{0};
    GlassServiceDelegate* glass_delegate_{nullptr};
    WakeupReason wakeup_reason_{WakeupReason::Ble};

    std::mutex mutex_;
    std::unique_ptr<SequenceManager> sequence_manager_;

};

#endif  // BASE_UART_SERVICE_H_
