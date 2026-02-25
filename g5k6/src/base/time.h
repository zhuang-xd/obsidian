#ifndef BASE_TIME_UTILS_H_
#define BASE_TIME_UTILS_H_

#include <cstdint>
namespace base {

class Time {
public:
    static constexpr int64_t kMillisecondsPerSecond = 1000;
    static constexpr int64_t kMicrosecondsPerMillisecond = 1000;
    static constexpr int64_t kMicrosecondsPerSecond =
        kMicrosecondsPerMillisecond * kMillisecondsPerSecond;
};

}  // namespace base
#endif  // BASE_TIME_UTILS_H_
