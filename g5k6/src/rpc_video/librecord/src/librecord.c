#include "librecord.h"
#include "base/app_common_defs.h"
#include "base/logging.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MINIMP4_IMPLEMENTATION
#include "third_party/mimimp4/mimimp4.h"

struct stream_head {
    int magic;
    int type;
    int channel;
    int data_length;   /*not include head itself*/
    int buffer_length; /*not include head itself*/
};

struct cbuf {
    int rd;
    int wr;
    int size;

    char buf[0];
};

static struct cbuf* g_mcbuf = NULL;
static int g_mfp[MAX_GRP_NUM * MAX_VPU_CHN_NUM];
static int g_mfpsz[MAX_GRP_NUM * MAX_VPU_CHN_NUM];
static int g_record_printed[MAX_GRP_NUM * MAX_VPU_CHN_NUM];
static MP4E_mux_t* g_mux[MAX_GRP_NUM * MAX_VPU_CHN_NUM];
static mp4_h26x_writer_t g_mp4wr[MAX_GRP_NUM * MAX_VPU_CHN_NUM];

static int g_audio_track_id[MAX_GRP_NUM * MAX_VPU_CHN_NUM];
static uint64_t g_audio_last_time[MAX_GRP_NUM * MAX_VPU_CHN_NUM];
static uint32_t g_audio_sample_rate[MAX_GRP_NUM * MAX_VPU_CHN_NUM];
static uint32_t g_audio_channels[MAX_GRP_NUM * MAX_VPU_CHN_NUM];

static pthread_mutex_t g_cbuf_mutex = PTHREAD_MUTEX_INITIALIZER;

static volatile AppConfig* g_app_config_ptr;
volatile int g_record_stream_stop = 0;
static pthread_t g_record_thread = 0;

struct cbuf* cbuf_init(int size) {
    if (size <= 0 || size > 20 * 1024 * 1024)
        return (void*)0;

    size = (size + 31) & (~31);

    struct cbuf* cbuf = (struct cbuf*)malloc(size + sizeof(struct cbuf));

    if (cbuf) {
        memset(cbuf, 0, sizeof(struct cbuf));
        cbuf->size = size;
    }

    return cbuf;
}

int cbuf_prefetch(struct cbuf* cbuf, int* type, int* channel, void** seg1, void** seg2, int* seg1sz, int* seg2sz,
                  int* advance_length)

{
    pthread_mutex_lock(&g_cbuf_mutex);
    int rd;
    int wr;
    int size;
    int clen;
    struct stream_head* hdr;

    if (!cbuf) {
        pthread_mutex_unlock(&g_cbuf_mutex);
        return -1;
    }

    rd = cbuf->rd;
    wr = cbuf->wr;
    size = cbuf->size;
    if (rd <= wr) {
        clen = wr - rd;
    } else {
        clen = size - rd + wr;
    }

    if (rd == wr) {
        pthread_mutex_unlock(&g_cbuf_mutex);
        return -1;
    }

    hdr = (struct stream_head*)(cbuf->buf + rd);
    if (rd + sizeof(struct stream_head) > size || hdr->magic != STREAM_MAGIC || hdr->data_length <= 0 ||
        hdr->data_length >= size || hdr->data_length > hdr->buffer_length ||
        sizeof(struct stream_head) + hdr->buffer_length > clen) {
        printf("%s: internal error, line %d!\n", __func__, __LINE__);
        pthread_mutex_unlock(&g_cbuf_mutex);
        return -1;
    }

    *type = hdr->type;
    *channel = hdr->channel;

    rd += sizeof(struct stream_head);
    if (rd >= size)
        rd = 0;

    *seg1 = cbuf->buf + rd;
    if (size - rd >= hdr->data_length) {
        *seg1sz = hdr->data_length;
        *seg2 = (void*)0;
        *seg2sz = 0;
    } else {
        *seg1sz = size - rd;
        *seg2 = cbuf->buf;
        *seg2sz = hdr->data_length - (size - rd);
    }

    *advance_length = sizeof(struct stream_head) + hdr->buffer_length;

    hdr->magic = 0; /*clear magic*/
    pthread_mutex_unlock(&g_cbuf_mutex);
    return 0;
}

int cbuf_advance(struct cbuf* cbuf, int advance_length) {
    int rd;
    int size;

    if (!cbuf || advance_length <= 0)
        return -1;

    rd = cbuf->rd;
    size = cbuf->size;

    rd = (rd + advance_length) % size;
    cbuf->rd = rd;

    return 0;
}

static int write_callback(int64_t offset, const void* buffer, size_t size, void* token) {
    int fd = (int)token;
    lseek(fd, offset, SEEK_SET);
    return write(fd, buffer, size) != size;
}

static ssize_t get_nal_size(uint8_t* buf, ssize_t size) {
    ssize_t pos = 3;
    while ((size - pos) > 3) {
        if (buf[pos] == 0 && buf[pos + 1] == 0 && buf[pos + 2] == 1)
            return pos;
        if (buf[pos] == 0 && buf[pos + 1] == 0 && buf[pos + 2] == 0 && buf[pos + 3] == 1)
            return pos;
        pos++;
    }
    return size;
}

static uint64_t now_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t absolute_micro = (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
    return absolute_micro;
}

static int extract_aac_dsi(const unsigned char* aac_data, int aac_len, int sample_rate, int channels,
                           unsigned char* dsi, int* dsi_len) {
    if (!aac_data || aac_len < 2 || !dsi || !dsi_len) {
        return -1;
    }

    if (aac_len >= 7 && (aac_data[0] == 0xFF) && ((aac_data[1] & 0xF0) == 0xF0)) {
        int profile = (aac_data[2] >> 6) & 0x03;
        int sfi = (aac_data[2] >> 2) & 0x0F;
        int channel_config = 1; // Force mono

        dsi[0] = (profile << 3) | ((sfi >> 1) & 0x07);
        dsi[1] = ((sfi & 0x01) << 7) | (channel_config << 3);
        *dsi_len = 2;
        return 0;
    }

    int sfi = 0;
    if (sample_rate >= 96000)
        sfi = 0;
    else if (sample_rate >= 88200)
        sfi = 1;
    else if (sample_rate >= 64000)
        sfi = 2;
    else if (sample_rate >= 48000)
        sfi = 3;
    else if (sample_rate >= 44100)
        sfi = 4;
    else if (sample_rate >= 32000)
        sfi = 5;
    else if (sample_rate >= 24000)
        sfi = 6;
    else if (sample_rate >= 22050)
        sfi = 7;
    else if (sample_rate >= 16000)
        sfi = 8;
    else if (sample_rate >= 12000)
        sfi = 9;
    else if (sample_rate >= 11025)
        sfi = 10;
    else
        sfi = 11; /* 8000 */

    int profile = 1; /* AAC-LC */
    int channel_config = channels > 0 ? channels : 1;

    dsi[0] = (profile << 3) | ((sfi >> 1) & 0x07);
    dsi[1] = ((sfi & 0x01) << 7) | (channel_config << 3);
    *dsi_len = 2;
    return 0;
}

int audio_save(int ch, unsigned char* audio_data, int audio_len, int sample_rate, int channels) {
    if (channels != 1) {
        LOG_W("audio_save: input channels=%d, forcing to 1 (mono)", channels);
        channels = 1;
    }
    if (!g_mux[ch] || g_mfp[ch] <= 0)
        return -1;

    if (g_audio_track_id[ch] < 0) {
        MP4E_track_t audio_track = {0};
        audio_track.object_type_indication = MP4_OBJECT_TYPE_AUDIO_ISO_IEC_14496_3; // AAC
        audio_track.track_media_kind = e_audio;
        audio_track.time_scale = sample_rate;
        audio_track.u.a.channelcount = channels;
        memcpy(audio_track.language, "und", 4);

        g_audio_track_id[ch] = MP4E_add_track(g_mux[ch], &audio_track);
        if (g_audio_track_id[ch] < 0) {
            LOG_E("audio_save: Failed to add audio track, error=%d", g_audio_track_id[ch]);
            return -1;
        }

        uint8_t dsi[16];
        int dsi_len = 0;
        if (extract_aac_dsi(audio_data, audio_len, sample_rate, channels, dsi, &dsi_len) != 0) {
            LOG_E("audio_save: Failed to extract AAC DSI");
            return -1;
        }

        if (MP4E_set_dsi(g_mux[ch], g_audio_track_id[ch], dsi, dsi_len) != MP4E_STATUS_OK) {
            LOG_E("audio_save: Failed to set AAC DSI");
            return -1;
        }

        LOG_S("audio_save: Audio track added, track_id=%d, sample_rate=%d, channels=%d", g_audio_track_id[ch],
              sample_rate, channels);
    }

    const unsigned char* payload = audio_data;
    int payload_len = audio_len;
    if (payload_len >= 7 && payload[0] == 0xFF && (payload[1] & 0xF0) == 0xF0) {
        payload += 7;
        payload_len -= 7;
    }
    if (payload_len <= 0)
        return -1;

    uint64_t now = now_time();
    int duration = 0;
    if (g_audio_last_time[ch] == 0) {
        duration = 1024;
    } else {
        duration = (int)(((now - g_audio_last_time[ch]) * sample_rate) / 1000000);
    }
    g_audio_last_time[ch] = now;

    if (duration <= 0) {
        duration = 1024;
    }

    int ret = MP4E_put_sample(g_mux[ch], g_audio_track_id[ch], payload, payload_len, duration, MP4E_SAMPLE_DEFAULT);
    if (ret != MP4E_STATUS_OK) {
        LOG_E("audio_save: Failed to write audio sample, error=%d", ret);
        return -1;
    }

    return 0;
}
void* cbuf_write_thread_proc(void* arg) {
    int k;
    int ret;
    int media_type;
    int media_subtype;
    int media_chn;
    void* seg1;
    void* seg2;
    int seg1sz;
    int seg2sz;
    int advance_length;
    char path[256];

    int sequential_mode = 0;
    int fragmentation_mode = 0;
    int is_hevc = 1;
    u_int32_t width = g_app_config_ptr->video_width;
    u_int32_t height = g_app_config_ptr->video_height;
    u_int32_t duration = 90000 / g_app_config_ptr->video_framerate;
    char* record_path = g_app_config_ptr->record_path;

    prctl(PR_SET_NAME, "demo_record");
    LOG_C("Recording started");
    uint64_t last_time = 0;

    while (g_record_stream_stop == 0) {
        ret = cbuf_prefetch(g_mcbuf, &media_type, &media_chn, &seg1, &seg2, &seg1sz, &seg2sz, &advance_length);
        if (ret != 0) {
            if (g_record_stream_stop != 0) {
                break;
            }
            // Reduce sleep time to check stop flag more frequently
            usleep(5 * 1000);
            continue;
        }

        if (g_record_stream_stop != 0) {
            cbuf_advance(g_mcbuf, advance_length);
            break;
        }

        media_subtype = media_type & 0xffff;
        media_type >>= 16;

        if ((media_type == DMC_MEDIA_TYPE_H265) && g_mfpsz[media_chn] == 0) {
            if (media_subtype != DMC_MEDIA_SUBTYPE_IFRAME) {
                cbuf_advance(g_mcbuf, advance_length);
                continue;
            }
        }

        if (media_type == DMC_MEDIA_TYPE_AUDIO) {
            if (g_record_stream_stop != 0) {
                cbuf_advance(g_mcbuf, advance_length);
                break;
            }
            if (!g_mux[media_chn]) {
                cbuf_advance(g_mcbuf, advance_length);
                continue;
            }
            int total_len = seg1sz + seg2sz;
            unsigned char stack_buf[1024];
            unsigned char* audio_buf = total_len <= sizeof(stack_buf) ? stack_buf : malloc(total_len);

            if (!audio_buf) {
                LOG_E("error: malloc %d failed", total_len);
                cbuf_advance(g_mcbuf, advance_length);
                continue;
            }

            if (seg1)
                memcpy(audio_buf, seg1, seg1sz);
            if (seg2)
                memcpy(audio_buf + seg1sz, seg2, seg2sz);

            int ret = audio_save(media_chn, audio_buf, total_len, g_audio_sample_rate[media_chn],
                                 g_audio_channels[media_chn]);
            if (ret != 0) {
                LOG_E("audio_save failed, ret=%d", ret);
            }

            if (audio_buf != stack_buf)
                free(audio_buf);
            cbuf_advance(g_mcbuf, advance_length);

            if (g_record_stream_stop != 0) {
                break;
            }
            continue;
        }

        if (g_mfp[media_chn] <= 0) {
            if (media_type == DMC_MEDIA_TYPE_H265) {
                snprintf(path, sizeof(path), "%s/%s", record_path, g_app_config_ptr->record_file_name);
            }
            g_mfp[media_chn] = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
            if (g_mfp[media_chn] < 0) {
                LOG_E("Error: open file %s failed\n", path);
                cbuf_advance(g_mcbuf, advance_length);
                // Check stop flag during sleep
                for (int i = 0; i < 100; i++) {
                    if (g_record_stream_stop != 0) {
                        break;
                    }
                    usleep(10 * 1000);
                }
                continue;
            }

            LOG_S("Start record to file %s", path);
            if (!g_record_printed[media_chn]) {
                g_record_printed[media_chn] = 1;
            }
            g_mfpsz[media_chn] = 0;
            g_mux[media_chn] = MP4E_open(sequential_mode, fragmentation_mode, (void*)g_mfp[media_chn], write_callback);

            if (MP4E_STATUS_OK != mp4_h26x_write_init(&g_mp4wr[media_chn], g_mux[media_chn], width, height, is_hevc)) {
                LOG_E("error: mp4_h26x_write_init failed");
            }
        }

        if (seg1) {
            if (g_record_stream_stop != 0) {
                cbuf_advance(g_mcbuf, advance_length);
                break;
            }
            int h265_size = seg1sz;
            void* buf_h265 = seg1;
            while (h265_size > 0) {
                if (g_record_stream_stop != 0) {
                    break;
                }
                ssize_t nal_size = get_nal_size(buf_h265, h265_size);
                uint64_t now = now_time();
                if (last_time == 0) {
                    duration = 400 * 9 / 100;
                } else {
                    duration = (now - last_time) * 9 / 100;
                }
                last_time = now;
                if (MP4E_STATUS_OK != mp4_h26x_write_nal(&g_mp4wr[media_chn], buf_h265, nal_size, duration)) {
                    LOG_E("error: mp4_h26x_write_nal failed");
                }
                buf_h265 += nal_size;
                h265_size -= nal_size;
            }

            g_mfpsz[media_chn] += seg1sz;
        }

        if (seg2) {
            if (g_record_stream_stop != 0) {
                cbuf_advance(g_mcbuf, advance_length);
                break;
            }
            int h265_size = seg2sz;
            void* buf_h265 = seg2;
            while (h265_size > 0) {
                if (g_record_stream_stop != 0) {
                    break;
                }
                ssize_t nal_size = get_nal_size(buf_h265, h265_size);
                if (MP4E_STATUS_OK != mp4_h26x_write_nal(&g_mp4wr[media_chn], buf_h265, nal_size, duration)) {
                    LOG_E("error: mp4_h26x_write_nal failed");
                    break;
                }
                buf_h265 += nal_size;
                h265_size -= nal_size;
            }
            g_mfpsz[media_chn] += seg2sz;
        }

        cbuf_advance(g_mcbuf, advance_length);

        if (g_mfpsz[media_chn] > 0x70000000) {
            ret = lseek(g_mfp[media_chn], 0, SEEK_SET);
            if (ret < 0) {
                LOG_E("%s: lseek failed!!!", __func__);
            }
            g_mfpsz[media_chn] = 0;
        }
    }

    for (k = 0; k < MAX_GRP_NUM * MAX_VPU_CHN_NUM; k++) {
        if (g_mux[k] > 0) {
            MP4E_close(g_mux[k]);
            g_mux[k] = 0;
            mp4_h26x_write_close(&g_mp4wr[k]);
        }

        if (g_mfp[k] > 0) {
            fsync(g_mfp[k]);
            close(g_mfp[k]);
        }
        g_mfp[k] = -1;
        g_record_printed[k] = 0;
        g_audio_track_id[k] = -1;
        g_audio_last_time[k] = 0;
    }
    g_record_stream_stop = 0;
    LOG_S("Stop record thread exited normally");
    return (void*)0;
}

int start_cbuf_write_thread(void) {
    pthread_attr_t attr;

    g_record_stream_stop = 0;
    g_record_thread = 0;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&g_record_thread, &attr, cbuf_write_thread_proc, (void*)&g_record_stream_stop);
    pthread_attr_destroy(&attr);

    return 0;
}

int cbuf_write(struct cbuf* cbuf, int type, int channel, void* data, int length) {
    int rd;
    int wr;
    int size;
    int room;
    int segsz;
    int aligned_length = (length + 3) & (~3);
    struct stream_head* hdr;

    if (!cbuf || !data)
        return -1;

    pthread_mutex_lock(&g_cbuf_mutex);
    rd = cbuf->rd;
    wr = cbuf->wr;
    size = cbuf->size;

    if (length <= 0 || length >= size - 1024) {
        printf("%s: ignore big frame(%d)!\n", __func__, length);
        return -1;
    }

    if (rd > wr)
        room = rd - wr;
    else
        room = size - wr + rd;

    if (sizeof(struct stream_head) * 2 + aligned_length + 1 > room) {
        printf("record: drop frame(%d)!\n", length);
        return -1;
    }

    if (wr + sizeof(struct stream_head) > size) {
        printf("%s: internal error, line %d!\n", __func__, __LINE__);
        return -1;
    }

    hdr = (struct stream_head*)(cbuf->buf + wr);
    hdr->magic = STREAM_MAGIC;
    hdr->type = type;
    hdr->channel = channel;
    hdr->data_length = length;
    hdr->buffer_length = aligned_length;

    wr += sizeof(struct stream_head);
    if (wr >= size)
        wr = 0;

    segsz = size - wr;
    if (length > segsz) {
        memcpy(cbuf->buf + wr, data, segsz);
        memcpy(cbuf->buf, data + segsz, length - segsz);
    } else {
        memcpy(cbuf->buf + wr, data, length);
    }

    wr = (wr + aligned_length) % size;

    if (size - wr < sizeof(struct stream_head)) {
        hdr->buffer_length += (size - wr);
        wr = 0;
    }

    cbuf->wr = wr;

    pthread_mutex_unlock(&g_cbuf_mutex);

    return 0;
}

int librecord_init(const void* config) {
    int k;

    for (k = 0; k < sizeof(g_mfp) / sizeof(int); k++) {
        g_mfp[k] = -1;
        g_mfpsz[k] = 0;
        g_record_printed[k] = 0;

        g_audio_track_id[k] = -1; // audio track id
        g_audio_last_time[k] = 0;
        g_audio_sample_rate[k] = 16000; // audio sample rate
        g_audio_channels[k] = 1;        // channel count
    }

    g_app_config_ptr = (AppConfigPtr)config;

    // init audio sample rate from config
    if (g_app_config_ptr && g_app_config_ptr->audio_framerate > 0) {
        for (k = 0; k < sizeof(g_mfp) / sizeof(int); k++) {
            g_audio_sample_rate[k] = g_app_config_ptr->audio_framerate;
        }
    }

    if (!g_mcbuf) {
        g_mcbuf = cbuf_init(8 * 1024 * 1024);
    }

    return start_cbuf_write_thread();
}

int librecord_save(int media_chn, int media_type, int media_subtype, unsigned char* frame_data, int frame_len) {
    return cbuf_write(g_mcbuf, (media_type << 16) | media_subtype, media_chn, frame_data, frame_len);
}

int librecord_set_audio_params(int media_chn, int sample_rate, int channels) {
    if (media_chn < 0 || media_chn >= MAX_GRP_NUM * MAX_VPU_CHN_NUM) {
        return -1;
    }
    if (sample_rate <= 0 || channels <= 0) {
        return -1;
    }
    g_audio_sample_rate[media_chn] = sample_rate;
    g_audio_channels[media_chn] = channels;
    return 0;
}

int stop_cbuf_write_thread(void) {
    if (g_record_thread == 0) {
        return 0;
    }

    // Set stop flag to allow thread to exit naturally
    g_record_stream_stop = 1;

    if (g_record_thread != 0) {
        pthread_t thread_to_join = g_record_thread;
        g_record_thread = 0;

        // Increase timeout to allow thread to finish writing data and closing files
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 5; // Increase to 5 seconds for data writing and file closing

        int ret = pthread_timedjoin_np(thread_to_join, NULL, &timeout);
        if (ret != 0) {
            printf("stop_cbuf_write_thread: join timeout or failed (ret=%d), waiting for file operations\n", ret);
            // Wait additional time for file operations to complete
            usleep(2000 * 1000); // Wait 2 more seconds
            // Try to join again
            clock_gettime(CLOCK_REALTIME, &timeout);
            timeout.tv_sec += 3;
            ret = pthread_timedjoin_np(thread_to_join, NULL, &timeout);
            if (ret != 0) {
                printf("stop_cbuf_write_thread: final join failed (ret=%d), forcing file close to save data\n", ret);
                // Force close files if thread is stuck (last resort to save data)
                for (int k = 0; k < MAX_GRP_NUM * MAX_VPU_CHN_NUM; k++) {
                    if (g_mux[k] > 0) {
                        MP4E_close(g_mux[k]);
                        g_mux[k] = 0;
                        mp4_h26x_write_close(&g_mp4wr[k]);
                    }
                    if (g_mfp[k] > 0) {
                        fsync(g_mfp[k]);
                        close(g_mfp[k]);
                        g_mfp[k] = -1;
                    }
                }
                pthread_detach(thread_to_join);
            } else {
                printf("stop_cbuf_write_thread: thread exited after additional wait\n");
            }
        } else {
            printf("stop_cbuf_write_thread: thread exited successfully, data saved\n");
        }
    }

    return 0;
}

int librecord_uninit(void) {
    return stop_cbuf_write_thread();
}