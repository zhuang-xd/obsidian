#include "base/uart_helper.h"
#include "app_command_defs.h"
#include "base/app_common_defs.h"
#include "base/system_util.h"
#include "base/ticks.h"
#include "base/time.h"
#include "base/eintr_wrapper.h"
#include "logging.h"

#include <cstdint>
#include <string.h>
#include <cstdio>
#include <stdlib.h>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <sys/stat.h>

namespace {
inline static struct timeval TickDeltaToTimeVal(const base::TickDelta& delta) {
  struct timeval result;
  result.tv_sec = delta.InSeconds();
  result.tv_usec = delta.InMicroseconds() % base::Time::kMicrosecondsPerSecond;
  return result;
}

}

uint16_t UartHelper::CRC16(const uint8_t* data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    const uint8_t* addr = data;

    for(uint16_t i = 0; i < length; i++) {
        crc = crc ^ (*addr);
        for(uint16_t j = 0; j < 8; j++) {
            uint16_t temp = crc & 0x0001;
            crc = crc >> 1;
            if (temp) {
                crc = crc ^ 0xA001;
            }
        }
        addr++;
    }

    return crc;
}

void UartHelper::FillCRC(uint8_t* data, uint16_t length) {
    uint16_t crc = CRC16(data, length);
    memcpy(data + length, &crc, 2);
}

bool UartHelper::CheckCRC(uint16_t crc_in, const uint8_t* data, uint16_t length) {
    uint16_t crc_cur = CRC16(data, length);
    return crc_cur == crc_in;
}

bool UartHelper::FormatSDCard(void) {
    base::LaunchProcess("umount /mnt/sd");

    // Waiting for complete
    SleepMilliSeconds(100);

    base::LaunchProcess("mkfs.vfat -F 32 /dev/mmcblk0p1");
    SleepMilliSeconds(50);
    base::LaunchProcess("mount /dev/mmcblk0p1 /mnt/sd");
    SleepMilliSeconds(100);
    return true;
}

int32_t UartHelper::UmountSDCard(void) {
    SyncToDisc();
    base::LaunchProcess("umount /mnt/sd");
    // Waiting for complete
    SleepMilliSeconds(100);

    return 0;
}

bool UartHelper::IsMounted(const char *path)
{
    struct stat st1, st2;
    if (stat(path, &st1) != 0) {
        return false;
    }

    char parent[256] = { 0 };
    snprintf(parent, sizeof(parent), "%s/..", path);
    if (stat(parent, &st2) != 0) {
        return false;
    }
    if (st1.st_dev == 1) {
        return false;
    }

    printf("path:%s device st_dev:%lu : %lu\n", path, st1.st_dev, st2.st_dev);
    return (st1.st_dev != st2.st_dev);
}

void UartHelper::SyncToDisc(void) {
    LOGD("Sync to disc");
    errno = 0;
    base::LaunchProcess("sync");
    SleepMilliSeconds(200);
}

int32_t UartHelper::UmountStart(void) {
    SyncToDisc();
    base::LaunchProcess("umount /start");
    // Waiting for complete
    SleepMilliSeconds(50);

    return 0;
}

void UartHelper::UmountStorages(void) {
    LOGI("Umount storages");
    base::LaunchProcess("sync");
    base::LaunchProcess("umount /mnt/sd");
    base::LaunchProcess("umount /start");
    SleepMilliSeconds(50);
}

int32_t UartHelper::MountSDCard(void) {
    base::LaunchProcess("mount /dev/mmcblk0p1 /mnt/sd");
    // Waiting for complete
    SleepMilliSeconds(100);
    return 0;
}

void UartHelper::SleepMilliSeconds(uint32_t value) {
    usleep(1000 * value);
}

void UartHelper::SetGPIOValue(uint32_t gpio_num, uint8_t state) {
    char command[256];
    snprintf(command, sizeof(command),
             "echo %d > /sys/class/gpio/export; "
             "echo out > /sys/class/gpio/GPIO%d/direction; "
             "echo %d > /sys/class/gpio/GPIO%d/value; "
             "echo %d > /sys/class/gpio/unexport ",
             gpio_num,
             gpio_num,
             state,gpio_num,
             gpio_num);

    system(command);
}


int UartHelper::UartConfigure(int fd, speed_t speed, uint8_t timeout)
{
    struct termios newtio, oldtio;
    if (tcgetattr(fd, &oldtio) != 0) {
        perror("Uart configure error");
        return -1;
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag |= CLOCAL | CREAD;  // 设置本地连接和接收使能标志位
    newtio.c_cflag &= ~CSIZE;  // 清除数据位设置标志位
    newtio.c_cflag |= CS8;  // 设置数据位为8位
    newtio.c_cflag &= ~PARENB;  // 清除校验使能标志位

    cfsetispeed(&newtio, speed);
    cfsetospeed(&newtio, speed);

    newtio.c_cflag &= ~CSTOPB;
    newtio.c_cc[VTIME] = timeout;
    newtio.c_cc[VMIN] = 0;
    tcflush(fd, TCIFLUSH);

    if ((tcsetattr(fd, TCSANOW, &newtio)) != 0) {
        perror("com set error");
        return -1;
    }

    fcntl(fd, F_SETFL, 0);

    return 0;
}

// Wait a file for read for a certain timeout.
// Returns -1 if error occurred, 0 if timeout reached, > 0 if the file is
// ready for read.
int UartHelper::WaitDeviceForRead(int fd, const base::TickDelta& timeout) {
  fd_set read_fds;
  struct timeval tv = TickDeltaToTimeVal(timeout);

  FD_ZERO(&read_fds);
  FD_SET(fd, &read_fds);

  return HANDLE_EINTR(select(fd + 1, &read_fds, nullptr, nullptr, &tv));
}

int32_t UartHelper::Write(int fd, const uint8_t* data, int data_length) {
  // align to max bytes
  constexpr uint16_t buffer_size = 64;

  int bytes_write = 0;
  while (bytes_write < data_length) {
    int sending_size = (data_length - bytes_write);
    const uint8_t* palyload = data + bytes_write;
    if (sending_size > buffer_size) {
      palyload = data + bytes_write;
      sending_size = buffer_size;
    }


    int n = HANDLE_EINTR(write(fd, palyload, sending_size));
    if (n <= 0) {
      return n;
    }

    bytes_write += n;
  }

  return bytes_write;
}

int32_t UartHelper::Read(int fd, uint8_t* buffer, int32_t length) {
  int bytes_read = HANDLE_EINTR(read(fd, buffer, length));
  if (bytes_read > 0) {
      LOGD("uart_read %s", buffer);
  }
  return bytes_read;
}


uint16_t UartHelper::ParseDataLength(uint8_t* buffer) {
    uint8_t high_byte = buffer[4];
    uint8_t low_byte = buffer[5];

    uint16_t data_len = (uint16_t)high_byte << 8 | low_byte;

    return data_len;
}

void UartHelper::WriteLogToFile(void) {
    std::string log_file_path = std::string(LOG_FILE_PATH) + std::string(LOG_FILE_NAME);
    std::string command = "ls -l " + std::string(SD_CARD_DATA_ROOT) + " >> " + log_file_path;
    system(command.c_str());
}