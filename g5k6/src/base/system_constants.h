#ifndef BASE_SYSTEM_CONSTS_H_
#define BASE_SYSTEM_CONSTS_H_

#include <cstdint>
#include <stddef.h>

namespace base {
    const uint8_t kMediaCommandStartRecord[] = {0xA5, 0x02, 0x00, 0xA7, 0xEF};
    const uint8_t kMediaCommandTakePhoto[] = {0xA5, 0x01, 0x00, 0xA6, 0xEF};
    const size_t kMediaCommandStartRecordSize = sizeof(kMediaCommandStartRecord);
    const size_t kMediaCommandTakePhotoSize = sizeof(kMediaCommandTakePhoto);
    const uint64_t kUartHeartbeatInterval = 1500 * 1000;
    const uint64_t kRecordTimeMax = 60 * 1000 * 1000;
}

#endif  // BASE_SYSTEM_CONSTS_H_
