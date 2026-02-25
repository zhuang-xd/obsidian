#include "audio_stream.h"
#include "base/logging.h"
#include "aacenc_api.h"
#include "dsp/fh_audio_mpipara.h"
#include "dsp/fh_common.h"
#include "rpc_video/librecord/include/librecord.h"
#include <cstring>
#include <cstdlib>
#include <thread>
#include <atomic>
#include <chrono>
#include <future>
#include <unistd.h>

#define CHECK_RET(expr) \
    do { \
        FH_SINT32 ret = expr; \
        if (ret != 0) { \
            LOG_E("AudioStream: %s failed, ret=0x%08x", #expr, ret); \
            return false; \
        } \
    } while(0)
static constexpr uint16_t kFrameSamples = 1024;

AudioStream::AudioStream() {
    memset(&ac_config_, 0, sizeof(ac_config_));
    memset(&mix_config_, 0, sizeof(mix_config_));
    memset(&stream_, 0, sizeof(stream_));
    stream_.bcontinue = false;
    stream_.source = CC_RAW_SOURCE_H264;
}

AudioStream::~AudioStream() {
    StopAudio();
}

bool AudioStream::StartAudio() {
    if (audio_thread_running_.load()) {
        LOG_W("AudioStream: audio already started");
        return true;
    }
    FH_UINT32    version1,version2;
    FH_UINT32    support_chmask = 0;
    int ain_select = FH_AC_MIC_IN;
    int micin_vol = 2;
    int ain_vol  = 28;
    int enable_doa = 0;

    int enable_hpf = 0;
    int enable_nr = 0;
    int enable_agc = 0;
    int enable_aec = 0;

    CHECK_RET(FH_AC_Init());

    CHECK_RET(FH_AC_Get_Algo_Version(&version1, &version2));
    printf("[INFO]: Alg version(%d.%d.%d),%08x-%d\n", version2>>24,(version2>>16)&0xff,(version2>>8)&0xff, version2, version1);

    CHECK_RET(FH_AC_AI_Get_Feature_ChannelMask(&support_chmask));
    printf("[INFO]: supported AI channel_mask=%08x.\n", support_chmask);

    ac_config_.io_type = (FH_AC_IO_TYPE_E)ain_select;
    ac_config_.sample_rate = AC_SR_16K;
    ac_config_.bit_width = AC_BW_16;
    ac_config_.enc_type  = FH_PT_LPCM;
    ac_config_.period_size = kFrameSamples;
    ac_config_.channel_mask = 0x01;
    ac_config_.frame_num = 0; //default 0,mean use max buffer.
    ac_config_.reserved = 0;

    CHECK_RET(FH_AC_Set_Config(&ac_config_));
    sample_rate_ = static_cast<int>(ac_config_.sample_rate);
    channels_ = ChannelNum(ac_config_.channel_mask);
    if (channels_ == 0) {
        channels_ = 1;
    }
    mix_config_.mix_enable = 1;
    mix_config_.mix_channel_mask = ac_config_.channel_mask;
    if (ChannelNum(mix_config_.mix_channel_mask) > 1) {
        enable_doa = 1;
    }
    mix_config_.reserved = 0;
    CHECK_RET(FH_AC_AI_MIX_Set_Config(&mix_config_));
    CHECK_RET(FH_AC_AI_CH_SetAnologVol(ac_config_.channel_mask, micin_vol, ain_vol));
    CHECK_RET(ConfigAIAlgParameter(enable_hpf, enable_nr, enable_agc, enable_doa, enable_aec));

    // Enable AI capture and start the audio thread that will pull frames
    CHECK_RET(FH_AC_AI_Enable());

    LOGD("audio_thread_try to create thread");
    audio_ai_enabled_ = true;
    audio_thread_running_ = true;
    stream_.bcontinue = true;
    audio_thread_ = new std::thread(&AudioStream::AudioThreadMain, this, &stream_);
    LOGD("audio_thread_created thread");
    LOGD("AudioStream: started audio thread and enabled AI");
    return true;
}

void AudioStream::StopAudio() {
    if (!audio_thread_running_.load()) {
        return;
    }

    audio_thread_running_ = false;
    stream_.bcontinue = false;

    if (audio_ai_enabled_) {
        FH_AC_AI_Disable();
        audio_ai_enabled_ = false;
    }

    if (audio_thread_ && audio_thread_->joinable()) {
        audio_thread_->detach();
        delete audio_thread_;
        audio_thread_ = nullptr;
    }

    LOGD("AudioStream: audio stopped");
}

void AudioStream::AudioThreadMain(void *args) {
    FH_AC_AI_Frame_S ai_frame;
    constexpr uint16_t data_length = kFrameSamples * 2;
    LOGC("--- AudioThreadMain Start ---");
    stream_t *stream_local = (stream_t *)args;
    char playback_buffer[data_length];

    AAC_ENC_HANDLE aac_enc_handle = fh_aacenc_create(channels_, sample_rate_, 0);
    if (!aac_enc_handle) {
        LOGE("AudioStream: Failed to create AAC encoder");
        return;
    }

    librecord_set_audio_params(0, sample_rate_, channels_);

    unsigned char aac_data[2048] = {0};
    // FILE * aac_file = fopen("/mnt/sd/test.aac", "wb");
    while (stream_local->bcontinue) {
        int ret = FH_AC_AI_GetFrame(&ai_frame);
        if (ret == 0) {
            if (ai_frame.ch_data_len != data_length || !ai_frame.mix) {
                printf("Error: FH_AC_AI_GetFrame,len=%d,mix=%p,it should not happen!!!\n", ai_frame.ch_data_len, ai_frame.mix);
                continue;
            }
            // pcm data
            memcpy(playback_buffer, ai_frame.mix, ai_frame.ch_data_len);
            memset(aac_data, 0, sizeof(aac_data));

            // pcm to aac data
            int aac_length = fh_aacenc_encode(aac_enc_handle,
                                              reinterpret_cast<unsigned char*>(playback_buffer),
                                              data_length,
                                              aac_data,
                                              sizeof(aac_data));
            // if (aac_file) {
            //     fwrite(aac_data, 1, aac_length, aac_file);
            // }

            if (aac_length > 0) {
                int ret_save = librecord_save(0,
                                              DMC_MEDIA_TYPE_AUDIO,
                                              0,
                                              aac_data,
                                              aac_length);
                if (ret_save != 0) {
                    LOG_E("AudioStream: librecord_save audio failed, ret=%d", ret_save);
                }
            } else if (aac_length == 0) {
                continue;
            } else {
                LOGE("AudioStream: fh_aacenc_encode failed, ret=%d", aac_length);
            }
        } else {
            usleep(1000);  // 1ms delay
        }
    }
    if (aac_enc_handle) {
        fh_aacenc_destroy(aac_enc_handle);
    }
}

int AudioStream::ConfigAIAlgParameter(int enable_hpf, int enable_nr, int enable_agc, int enable_doa, int enable_aec) {
    FH_AC_SesParam alg;
    FH_SINT32 ret;

    ret = FH_AC_AI_MIX_Get_Algo_Param(&alg, sizeof(alg));
    if (ret != 0) {
        LOG_E("AudioStream: FH_AC_AI_MIX_Get_Algo_Param failed, ret=0x%08x", ret);
        return -1;
    }

    alg.hpf_flag = enable_hpf;
    alg.anc_flag = enable_nr;
    alg.agc_flag = enable_agc;
    alg.doa_flag = enable_doa;
    alg.aec_flag = enable_aec;

    ret = FH_AC_AI_MIX_Set_Algo_Param(&alg, sizeof(alg));
    if (ret != 0) {
        LOG_E("AudioStream: FH_AC_AI_MIX_Set_Algo_Param failed, ret=0x%08x", ret);
        return -1;
    }

    return 0;
}

int AudioStream::ChannelNum(int mask) {
    int i;
    int num = 0;

    for (i = 0; i < 32; i++) {
        if (mask & (1 << i)) {
            num++;
        }
    }

    return num;
}

void AudioStream::PushAudioData(const uint8_t* data, uint32_t length)
{
    std::lock_guard<std::mutex> lock(audio_data_mutex_);
    if (audio_data_queue_.size() > 10) {
        LOGD("PushAudioData, drop data: ---%d", length);
        return;
    }
    std::pair<uint32_t, uint8_t*> save_data;
    save_data.first = length;
    save_data.second = (uint8_t*)malloc(length);
    memcpy(save_data.second, data, length);
    audio_data_queue_.push(save_data);
}

void AudioStream::PeekAudioData(uint8_t** data, uint32_t *length)
{
    std::lock_guard<std::mutex> lock(audio_data_mutex_);
    if (!audio_data_queue_.empty()) {
        std::pair<uint32_t, uint8_t*> save_data = audio_data_queue_.front();
        *length = save_data.first;
        *data = save_data.second;
        audio_data_queue_.pop();
    } else {
        *length = 0;
        *data = nullptr;
    }
}

