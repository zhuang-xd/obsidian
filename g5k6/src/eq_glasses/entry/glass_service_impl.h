#ifndef DEMO_GLASS_SERVICE_IMPL_H_
#define DEMO_GLASS_SERVICE_IMPL_H_

#include "base/glass_service_delegate.h"
#include "base/app_common_defs.h"
#include "base/uart_service.h"
#include "audio_stream.h"
#include <cstdint>
#include <stdint.h>
#include <string>
#include <mutex>
#include <queue>
#include <utility>

class SequenceManager;

class GlassServiceImpl:
    public GlassServiceDelegate {

public:
    GlassServiceImpl(AppConfig* app_config);
    ~GlassServiceImpl();

    void Start();
    void Stop();

public:
    void OnTakePhoto() override;
    void OnStartRecording() override;
    void OnStopRecording() override;
    void OnStartWebService() override;
    bool OnOTAStartup(int device, const char* url) override;
    void OnLinuxUpgraded(int result) override;
    void OnARCHUpgraded(int result) override;
    void OnOTAProgress(int stage, int percentage, const char* message) override;
    void OnRequestFileList(uint16_t* file_count, std::string& thumbnail_path) override;
    void OnCommandStart(void) override;
    void OnCommandEnd(bool shutdown) override;
    void OnBeforePoweroff(void) override;
    void OnAfterOTA(int fd, int type, int result) override;
    void PoweroffForce() override;
    bool IsTransfering(void) override;
    void SetP2PName(const std::string& name) override;
    SequenceManager* GetSequenceManager(void);

private:
    AppConfig* app_config_{nullptr};
    UartService uart_service_;
    std::string thumbnail_root_;

    std::thread* audio_process_thread_{nullptr};
    std::atomic<bool> is_audio_processing_{false};
    AudioStream audio_stream_;

    std::mutex recording_mutex_;
private:
    void AudioProcessThreadMain(void);
    void audio_aac_to_mp4(void);
};

#endif  // DEMO_GLASS_SERVICE_IMPL_H_
