#ifndef BASE_UART_HELPER_H_
#define BASE_UART_HELPER_H_

#include <cstdint>
#include <termio.h>
#include "base/ticks.h"

class UartHelper {
public:
    static uint16_t CRC16(const uint8_t* data, uint16_t length);
    static void FillCRC(uint8_t* data, uint16_t length);
    static bool CheckCRC(uint16_t crc_in, const uint8_t* data, uint16_t length);
    static bool FormatSDCard(void);
    static int32_t UmountSDCard(void);
    static int32_t UmountStart(void);
    static bool IsMounted(const char *path);
    static void UmountStorages(void);
    static int32_t MountSDCard(void);
    static void SyncToDisc(void);
    static void SleepMilliSeconds(uint32_t value);
    static void SetGPIOValue(uint32_t gpio_num, uint8_t state);
    static int UartConfigure(int fd, speed_t speed, uint8_t timeout);
    static int32_t Read(int fd, uint8_t* data, int32_t length);
    static int32_t Write(int fd, const uint8_t* data, int32_t length);
    static int WaitDeviceForRead(int fd, const base::TickDelta& timeout);
    static uint16_t ParseDataLength(uint8_t* buffer);
    static void WriteLogToFile(void);
};

#endif  // BASE_UART_HELPER_H_
