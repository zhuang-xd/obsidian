#include "base/mtd_upgrade.h"
#include "base/logging.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <mtd/mtd-user.h>


namespace {
// 初始化MTD设备
int mtd_init(const char *device) {
    int fd = open(device, O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("无法打开MTD设备");
        return -1;
    }
    return fd;
}

// 清理MTD设备
void mtd_cleanup(int fd) {
    if (fd >= 0) {
        close(fd);
    }
}

// 擦除MTD块
int mtd_erase_block(int fd, off_t offset, struct mtd_info_user *info) {
    struct erase_info_user erase;
    erase.start = offset;
    erase.length = info->size;

    LOG_D("mtd erase: 0x%lx - 0x%lx (size: %u bytes)",
           offset, offset + erase.length, erase.length);

    if (ioctl(fd, MEMERASE, &erase) < 0) {
        LOG_E("erase fail");
        return -1;
    }
    return 0;
}

// 写入数据到MTD
int mtd_write_data(int fd, off_t offset, const uint8_t *data, size_t len) {
    // 实际写入数据
    ssize_t written = pwrite(fd, data, len, offset);
    if (written < 0) {
        return -1;
    }

    if (written != len) {
        LOG_E("write length mismatch: expected %d bytes, actual %d bytes", len, written);
        return -1;
    }

    return 0;
}
}  // namespace

int32_t MTDUpgrade::UpgradeDevice(const char* device, const char* image_path,
                                   ProgressCallback progress_callback) {
    LOG_I("MTD UpgradeDevice: device=%s, image_path=%s", device, image_path);
    struct mtd_info_user mtd_info;
    int ret;
    FILE *pImage = fopen(image_path, "rb");

    if (!pImage) {
        perror("无法创建镜像文件");
        return EXIT_FAILURE;
    }

    // 获取文件大小
    struct stat file_stat;
    if (fstat(fileno(pImage), &file_stat) < 0) {
        LOG_E("获取文件大小失败");
        fclose(pImage);
        return EXIT_FAILURE;
    }
    size_t total_size = file_stat.st_size;
    LOG_D("镜像文件大小: %zu 字节 (%.2f KB)", total_size, total_size / 1024.0);

    if (progress_callback) {
        progress_callback(0, "开始擦除MTD设备");
    }

    // 1. 初始化MTD设备
    int fd = mtd_init(device);
    if (fd < 0) {
        LOG_E("mtd_init fail");
        remove(image_path);
        return EXIT_FAILURE;
    }

    // 2. 获取MTD设备信息
    if (ioctl(fd, MEMGETINFO, &mtd_info) < 0) {
        LOG_E("获取MTD信息失败");
        remove(image_path);
        mtd_cleanup(fd);
        return EXIT_FAILURE;
    }

    // mtd_info.erasesize = mtd_info.size;
    LOG_D("MTD设备信息:");
    LOG_D("  设备大小: %u 字节 (%.2f KB)", mtd_info.size, mtd_info.size / 1024.0);
    LOG_D("  擦除块大小: %u 字节 (%.2f KB)", mtd_info.erasesize, mtd_info.erasesize / 1024.0);

    ret = mtd_erase_block(fd, 0, &mtd_info);
    if (ret < 0) {
        LOG_E("mtd_erase_block fail");
        remove(image_path);
        mtd_cleanup(fd);
        return EXIT_FAILURE;
    }

    if (progress_callback) {
        progress_callback(5, "erase completed, start writing");
    }

    int32_t offset_bytes = 0;
    uint8_t buffer[8192];
    size_t buffer_size = sizeof(buffer);
    int last_percentage = 0;
    static constexpr int kProgressUpdateInterval = 10; // update once every 10%

    while (true) {
        memset(buffer, 0, buffer_size);
        uint32_t read_bytes = fread(buffer, 1, buffer_size, pImage);
        if (read_bytes <= 0) {
            break;
        }

        ret = mtd_write_data(fd, offset_bytes, buffer, read_bytes);
        if (ret < 0) {
            LOG_E("mtd_write_data fail");
            mtd_cleanup(fd);
            return EXIT_FAILURE;
        }
        offset_bytes += read_bytes;

        // update progress (5-100%)
        if (progress_callback && total_size > 0) {
            int percentage = 5 + (offset_bytes * 95) / total_size;
            if (percentage >= 100) {
                percentage = 100;
            }

            // update progress every kProgressUpdateInterval%
            if (percentage >= last_percentage + kProgressUpdateInterval || percentage == 100) {
                char message[64];
                snprintf(message, sizeof(message), "writing: %d/%zu KB",
                        offset_bytes / 1024, total_size / 1024);
                progress_callback(percentage, message);
                last_percentage = percentage;
            }
        }
    }
    fclose(pImage);

    mtd_cleanup(fd);

    if (progress_callback) {
        progress_callback(100, "writing completed");
    }

    remove(image_path);

    LOG_I("MTD UpgradeDevice success: device=%s, image_path=%s", device, image_path);
    return EXIT_SUCCESS;
}
