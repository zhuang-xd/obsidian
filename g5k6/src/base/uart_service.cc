#include "base/uart_service.h"

#include "base/app_command_defs.h"
#include "base/app_common_defs.h"
#include "base/sequence_manager.h"
#include "base/system_constants.h"
#include "base/system_util.h"
#include "base/transfer_handler.h"
#include "base/wifi_handler.h"
#include "base/glass_service_delegate.h"
#include "base/logging.h"
#include "base/ticks.h"
#include "base/time.h"
#include "base/logging.h"
#include "base/uart_helper.h"
#include "base/uart_service.h"


#include <fcntl.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <errno.h>
#include <functional>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/reboot.h>
#include <termios.h>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <vector>
#include <unordered_set>
#include <chrono>

namespace {
static constexpr char UART1_DEV_PATH[] = "/dev/ttyS1";
static constexpr char UART2_DEV_PATH[] = "/dev/ttyS2";

static constexpr speed_t UartBaudRate115200 = B115200;
static constexpr speed_t UartBaudRate9600 = B9600;
static volatile uint64_t s_last_heartbeat_time = 0;
static volatile bool s_power_off = false;

uint64_t now_time_us(void) {
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    uint64_t s = tv.tv_sec;
    return s * 1000 * 1000 + tv.tv_nsec / 1000;
}

}

int32_t UartService::StartHeartbeat(void) {
  if (thread_heartbeat_) {
    return 0;
  }

  thread_heartbeat_ = std::make_unique<std::thread>(&UartService::HeartbeatThreadMain, this);
  return 0;
}

int32_t UartService::StartUartService(GlassServiceDelegate* glass_delegate) {
    is_running_ = true;

    glass_delegate_ = glass_delegate;
    sequence_manager_ = std::make_unique<SequenceManager>(this);
    sequence_manager_->Start();

    StartHeartbeat();
    wifi_handler_ = std::make_unique<WifiHandler>(this, glass_delegate);
    thread_uart1_ = std::make_unique<std::thread>(&UartService::Uart1ThreadMain, this);
    thread_uart2_ = std::make_unique<std::thread>(&UartService::Uart2ThreadMain, this);
    return 0;
}


static int uart_write_align2(int fd, const uint8_t *data, int data_length) {
  const int buffer_size = 2;
  char buffer[buffer_size];

  int bytes_write = 0;
  while (bytes_write < data_length) {
    int sending_size = (data_length - bytes_write);
    memset(buffer, 0, buffer_size);
    memcpy(buffer, data + bytes_write, sending_size);
    int n = HANDLE_EINTR(write(fd, buffer, buffer_size));
    if (n <= 0) {
      return n;
    }

    if (sending_size < buffer_size) {
      n = sending_size;
    }

    bytes_write += n;
  }

  // LOG_I("uart_write %d", bytes_write);
  return bytes_write;
}

void UartService::HeartbeatThreadMain(void) {
    uint64_t now = 0;
    s_last_heartbeat_time = 0;
    uint32_t try_waiting_for_Web = 0;
    while (is_running_) {
      usleep(100 * 1000);

      now = now_time_us();
      if (get_record_state() && record_start_time_ > 0 && (now - record_start_time_ >= base::kRecordTimeMax)) {
        wifi_handler_->StopRecord();
        LOGC("Stop record, now:%llu, record_start_time:%llu, duration:%llu us",
             now, record_start_time_, now - record_start_time_);
      }

      if (!enable_heartbeat_) {
        UpdateHeartbeat();
        continue;
      }
      if (s_power_off) {
        continue;
      }

      if (now - s_last_heartbeat_time < base::kUartHeartbeatInterval) {
        continue;
      }

      if (get_record_state()) {
        s_last_heartbeat_time = now;
        try_waiting_for_Web = 0;
        continue;
      }

      if (s_last_heartbeat_time == 0) {
        s_last_heartbeat_time = now_time_us();
        continue;
      }

      if ((try_waiting_for_Web < 30) && wifi_handler_->IsWaitingForFileDownload()) {
        s_last_heartbeat_time = now_time_us();
        ++try_waiting_for_Web;
        continue;
      }

      if (glass_delegate_ && glass_delegate_->IsTransfering()) {
        s_last_heartbeat_time = now_time_us();
        LOGD("Heartbeat check: IsTransfering, update heartbeat");
        continue;
      }

      UartHelper::WriteLogToFile();
      PowerOff();
      s_last_heartbeat_time = now_time_us();
    }
}


void UartService::Uart1ThreadMain(void) {
    fd_uart1_ = open(UART1_DEV_PATH, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_uart1_ < 0) {
        LOG_E("Uart1 exit: open %s is failed, errno:%d", UART1_DEV_PATH, errno);
        return;
    }
    UartHelper::UartConfigure(fd_uart1_, UartBaudRate9600, 0);
    LOG_I("UartService started with device: %s", UART1_DEV_PATH);

    uint16_t hello_flag =  htons(MAIN_CONTROL_READY);
    while (is_running_) {
      if (s_power_off) {
        uint16_t command =  htons(TAKE_RECORD_FINISHED);
        int ret = uart_write_align2(fd_uart1_, (const uint8_t *)&command, sizeof(command));
        if (ret <= 0) {
          LOG_E("uart1_write fd:%d, data size:%d", fd_uart1_, ret);
        }
        usleep(500 * 1000);
        continue;
      }

      int ret = uart_write_align2(fd_uart1_, (const uint8_t *)&hello_flag, sizeof(hello_flag));
      if (ret <= 0) {
        LOG_E("uart1_write fd:%d, data size:%d", fd_uart1_, ret);
      }

      ret = UartHelper::WaitDeviceForRead(fd_uart1_, base::TickDelta(10000));
      if (ret <= 0) {
        continue;
      }

      uint8_t buffer[64] = {0};
      int32_t bytes_read = UartHelper::Read(fd_uart1_, buffer, 64);
      if (bytes_read <= 0) {
          continue;
      }

      OnDataArrived(buffer, bytes_read);
    }
}

void UartService::Uart2ThreadMain() {
    fd_uart2_ = open(UART2_DEV_PATH, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_uart2_ < 0) {
        LOG_E("Uart2 exit: open %s is failed, errno:%d", UART2_DEV_PATH, errno);
        return;
    }

    UartHelper::UartConfigure(fd_uart2_, UartBaudRate115200, 0);
    int32_t bytes_write = SendHandshake(fd_uart2_);
    if (bytes_write <= 0) {
        LOG_E("Uart2 thread: SendHandshake failed, bytes_write: %d", bytes_write);
        return;
    }

    static constexpr uint16_t kBufferSize = 128;
    static constexpr uint64_t kElapsedTimeMin = 80 * base::Time::kMicrosecondsPerMillisecond;
    uint8_t buffer[kBufferSize] = {0};
    uint64_t lasted_handle_time = 0;
    uint8_t last_commad = 0;
    int32_t bytes_read = 0;
    uint32_t expected_frame_size = 0;
    while (is_running_) {
        int ret = UartHelper::WaitDeviceForRead(fd_uart2_, base::TickDelta(5000));
        if (ret <= 0) {
          continue;
        }
        int rv = UartHelper::Read(fd_uart2_, buffer + bytes_read, kBufferSize - bytes_read);
        if (rv <= 0) {
          if (errno != EAGAIN && errno != EWOULDBLOCK) {
            continue;
          }
          continue;
        }
        bytes_read += rv;
        if (expected_frame_size == 0) {
          expected_frame_size = UartHelper::ParseDataLength(buffer) + 6;
        }
        if (bytes_read < expected_frame_size) {
          continue;
        }

        // duplicate command check
        uint64_t current_handle_time = now_time_us();
        uint64_t current_command = buffer[3];
        uint64_t elapsed_time = lasted_handle_time > 0 ? (current_handle_time - lasted_handle_time) : 1000 * base::Time::kMicrosecondsPerMillisecond;

        if (last_commad == current_command && elapsed_time < kElapsedTimeMin) {
          LOGW("ignore duplicate command: 0x%x, data length: %d, elapsed: %llu us",
               current_command, bytes_read, elapsed_time);
          bytes_read = 0;
          memset(buffer, 0, sizeof(buffer));
          continue;
        }

        LOGI("new command: 0x%x, data length: %d", current_command, bytes_read);
        lasted_handle_time = current_handle_time;
        last_commad = current_command;

        OnDataArrived(buffer, (uint32_t)bytes_read);
        bytes_read = 0;
        expected_frame_size = 0;
        memset(buffer, 0, sizeof(buffer));
    }
    }

void UartService::OnDataArrived(uint8_t* data, uint32_t length) {
    glass_delegate_->OnCommandStart();

    SequenceManager::Task task;
    auto command = std::vector<uint8_t>(data, data + length);
    task.func = [command] (void* arg)->bool {
      WifiHandler* wifi_handler = (WifiHandler*)arg;
      return wifi_handler->OnDataArrived(command.data(), (uint16_t)command.size());
    };
    task.custom_data = wifi_handler_.get();
    sequence_manager_->PostTask(task);
}

int32_t UartService::SendHandshake(int fd) {
  auto frame = WifiHandler::MakeUartFrame(DEV_ID_MASTER, DEV_ID_APP, APP_CMD_HELLO, 0);
  int ret = UartHelper::Write(fd, frame->header_start, UARTFrame_SIZE(0));
  LOG_C("send_handshake send hello uart_write: 0x%02x , ret=%d", APP_CMD_HELLO, ret);
  free(frame);
  return ret;
}

void UartService::PowerOff(void) {
  if (s_power_off) {
    return;
  }

  if (!enable_heartbeat_) {
    LOGW("PowerOff bypass by enable_heartbeat_=%df, s_power_off=%d", enable_heartbeat_, s_power_off);
    return;
  }

  s_power_off = true;
  LOGI("PowerOff");

  LogClose();

  UartHelper::UmountStorages();

  auto frame = WifiHandler::MakeUartFrame(DEV_ID_MASTER, DEV_ID_APP, APP_CMD_POWEROFF, 0);
  UartHelper::Write(fd_uart2_, frame->header_start, UARTFrame_SIZE(0));
  LOGI("PowerOff command have been sent to BLE.");
  free(frame);

  uint16_t command = 0;
  if (wakeup_reason_ == WakeupReason::TakePhoto) {
    command = TAKE_PHOTO_FINISHED;
  } else if (wakeup_reason_ == WakeupReason::StartRecord) {
    command = TAKE_RECORD_FINISHED;
  }
  if (command > 0) {
    for (int index = 0; index < 5; index++) {
      command = htons(command);
      Uart1Send((const uint8_t *)&command, sizeof(command));
      UartHelper::SleepMilliSeconds(20);
    }
  }

}

int32_t UartService::StopUartService(void) {
    is_running_ = false;
    // here can close fd for stop wait
    if (fd_uart1_ >= 0) {
        close(fd_uart1_);
        fd_uart1_ = -1;
    }

    if (thread_uart1_ && thread_uart1_->joinable()) {
        thread_uart1_->join();
    }

    if (fd_uart2_ >= 0) {
        close(fd_uart2_);
        fd_uart2_ = -1;
    }

    if (thread_uart2_ && thread_uart2_->joinable()) {
        thread_uart2_->join();
    }

    return 0;
}

void UartService::UpdateHeartbeat(void) {
    s_last_heartbeat_time = now_time_us();
}

void UartService::EnableHeartbeat(bool enable) {
  enable_heartbeat_ = enable;
  UpdateHeartbeat();

  if (enable && s_power_off) {
      LOGD("EnableHeartbeat: Reset s_power_off flag, allow power off again");
      s_power_off = false;
  } else if (!enable && s_power_off) {
      LOGD("EnableHeartbeat: Reset s_power_off flag when disabling heartbeat (command processing)");
      s_power_off = false;
  }
}

void UartService::SetRecordingState(bool is_recording) {
  set_record_state(is_recording);
  if (is_recording) {
   record_start_time_ = now_time_us();
  } else {
   if(wifi_handler_ && wifi_handler_->IsWaitingForFileDownload()) {
     UpdateHeartbeat();
     LOGD("SetRecordingState: update heartbeat when waiting for file download");
   }
  }
}

int32_t UartService::Send(int fd, const uint8_t* data, int32_t length) {
    return UartHelper::Write(fd, data, length);
}


int32_t UartService::Uart1Send(const uint8_t *data, int32_t length) {
    int ret = uart_write_align2(fd_uart1_, (const uint8_t *)data, length);
    if (ret <= 0) {
      LOG_E("uart1_write fd:%d, data size:%d", fd_uart1_, ret);
    }

    return ret;
}

int32_t UartService::Uart2Send(const uint8_t *data, int32_t length) {
    return UartHelper::Write(fd_uart2_, data, length);
}

void UartService::DoAfterTask(WakeupReason reason) {
}

uint32_t UartService::Restart(AppRestartReson reason) {
  UartHelper::UmountStorages();

  auto frame = WifiHandler::MakeUartFrame(DEV_ID_MASTER, DEV_ID_APP, APP_CMD_REBOOT, 0);
  int ret = Uart2Send(frame->header_start, UARTFrame_SIZE(0));
  LOG_C("restart by reason: %d send hello uart_write: 0x%02x , ret=%d", reason, APP_CMD_REBOOT, ret);
  free(frame);
  return ret;
}

void UartService::FastLaunchAndSendByBle(int32_t command, uint8_t* data, uint16_t length) {
  LOGD("FastLaunchAndSendByBle, command: %d, length:%d", command, length);
  fd_uart2_ = open(UART2_DEV_PATH, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd_uart2_ < 0) {
      LOG_E("Uart2 exit: open %s is failed, errno:%d", UART2_DEV_PATH, errno);
      return;
  }

  UartHelper::UartConfigure(fd_uart2_, UartBaudRate115200, 0);

  auto frame = WifiHandler::MakeUartFrame(DEV_ID_MASTER, DEV_ID_APP, command, length);
  if (data && length > 0) {
    memcpy(frame->data, data, length);
  }

  UartHelper::UmountStorages();

  int ret = Uart2Send(frame->header_start, UARTFrame_SIZE(0));
  LOG_C("FastLaunchAndSendByBle: 0x%02x , ret=%d", command, ret);
  free(frame);

  UartHelper::SleepMilliSeconds(10);
}

bool UartService::OnAllTaskFinished(void)
{
  glass_delegate_->OnCommandEnd();

  // continue run
  return true;
}