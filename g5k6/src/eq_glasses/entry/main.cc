#include "base/app_command_defs.h"
#include "base/logging.h"
#include "base/app_common_defs.h"
#include "base/socket_server.h"
#include "base/system_util.h"
#include "base/uart_helper.h"
#include "base/uart_service.h"
#include "eq_glasses/entry/main.h"
#include "third_party/cJSON/cJSON.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <memory>

#include "glass_service_impl.h"

namespace {
static std::unique_ptr<GlassServiceImpl> s_service_impl;

static int load_application_config(AppConfigPtr config) {
    set_record_state(false);
    config->audio_framerate = 30;
    config->video_width = 1920;
    config->video_height = 1080;
    config->audio_framerate = 8000;

    char buffer[1024];
    char path[64] = {0};
    snprintf(path, 64, "./%s", APPLICATION_CONFIG_FILE);
    int fd = open(path, O_RDONLY);
    if (fd > 0) {
        read(fd, buffer, sizeof(buffer));
        close(fd);
        LOG_D("config: %s", buffer);
    } else {
        LOG_W("The application config is not found: %s", path);
    }

    cJSON* root = cJSON_Parse(buffer);
    if (root) {
        config->video_width = cJSON_GetItemIntValue(root, "width");
        config->video_height = cJSON_GetItemIntValue(root, "height");
        config->video_framerate = cJSON_GetItemIntValue(root, "video_framerate");
        config->audio_framerate = cJSON_GetItemIntValue(root, "audio_framerate");
        char *record_path = cJSON_GetItemStringValue(root, "record_path");
        if (record_path) {
            memset(config->record_path, 0, sizeof(config->record_path));
            strcpy(config->record_path, record_path);
        }
        cJSON_free(root);
    } else {
        LOG_W("Parser failed path: %s", path);
    }

    return 0;
}

}

int start_application(AppConfigPtr config) {
    printf("------------------------- %s startup: %s build by %s %s -------------------------\n", APP_NAME, APP_VERSION, __DATE__, __TIME__);
    if (!UartHelper::IsMounted("/start") && !UartHelper::IsMounted("/mnt/sd")) {
        LOGE("/start or /mnt/sd is not mounted");
        reboot_hardware(APP_RESTART_BY_ERROR);
    }
    system("mkdir /start/log");
    LogInit(LOG_FILE_PATH);
    load_application_config(config);
    s_service_impl = std::make_unique<GlassServiceImpl>(config);
    s_service_impl->Start();
    return 0;
}

int stop_application(void) {
    LOG_C("application stop");
    s_service_impl->Stop();
    s_service_impl.reset();
    LogClose();
    return 0;
}

int reboot_hardware(AppRestartReson reason) {
    LOG_C("reboot hardware by:%d", reason);

    UartService uart_service;
    uart_service.FastLaunchAndSendByBle(APP_CMD_REBOOT);
    return 0;
}
