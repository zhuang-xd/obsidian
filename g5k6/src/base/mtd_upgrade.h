#ifndef BASE_MTD_UPGRADE_H_
#define BASE_MTD_UPGRADE_H_

#include <cstdint>
#include <functional>

class MTDUpgrade {
public:
    using ProgressCallback = std::function<void(int percentage, const char* message)>;

    MTDUpgrade() = default;
    ~MTDUpgrade() = default;

public:
    static int32_t UpgradeDevice(const char* device, const char* image_path,
                                  ProgressCallback progress_callback = nullptr);
};

#endif  // BASE_MTD_UPGRADE_H_
