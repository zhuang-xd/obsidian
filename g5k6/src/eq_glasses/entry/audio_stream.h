#ifndef AUDIO_STREAM_H_
#define AUDIO_STREAM_H_
#include <memory>
#include <mutex>
#include <queue>
#include <utility>
#include <cstdint>
#include <stdint.h>
#include <thread>
#include <atomic>
#include <cstring>

extern "C" {
#include "dsp/fh_audio_mpi.h"
}
typedef enum
{
    CC_RAW_SOURCE_H264
} ccrawSource;
typedef struct stream_s {
    pthread_t pid_video;
    pthread_t pid_audio;
    bool bcontinue;
    ccrawSource source;
} stream_t;

class AudioStream {
public:
    AudioStream();
    ~AudioStream();

    void PushAudioData(const uint8_t* data, uint32_t length);
    void PeekAudioData(uint8_t** data, uint32_t* length);

    bool StartAudio();
    void StopAudio();

private:    
    void AudioThreadMain(void *args);
    
    int ConfigAIAlgParameter(int enable_hpf, int enable_nr, int enable_agc, int enable_doa, int enable_aec);
    
    int ChannelNum(int mask);

private:
    std::mutex audio_data_mutex_;
    std::queue<std::pair<uint32_t, uint8_t*>> audio_data_queue_;

    stream_t stream_;
    
    int sample_rate_{16000};
    int channels_{1};
    std::thread* audio_thread_{nullptr};
    std::atomic<bool> audio_thread_running_{false};
    
    FH_AC_CONFIG ac_config_;
    FH_AC_MIX_CONFIG mix_config_;
    bool audio_ai_enabled_{false};

};

#endif // AUDIO_STREAM_H_

