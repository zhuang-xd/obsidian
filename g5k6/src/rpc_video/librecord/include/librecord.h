#ifndef __librecord_h__
#define __librecord_h__

#ifdef __cplusplus
extern "C" {
#endif

#define DMC_MEDIA_TYPE_H264 (1 << 8)   /*H264*/
#define DMC_MEDIA_TYPE_H265 (1 << 9)   /*H265*/
#define DMC_MEDIA_TYPE_JPEG (1 << 10)  /*JPEG*/
#define DMC_MEDIA_TYPE_AUDIO (1 << 11) /*Audio*/
#define DMC_MEDIA_TYPE_MJPEG (1 << 12) /*motion JPEG*/

#define MAX_VPU_CHN_NUM 4
#define MAX_GRP_NUM 2

#define DMC_MEDIA_SUBTYPE_IFRAME (1 << 0)
#define DMC_MEDIA_SUBTYPE_PFRAME (1 << 1)

#define STREAM_MAGIC (0x13769849)

int librecord_init(const void* config);

int librecord_save(int media_chn, int media_type, int media_subtype, unsigned char *frame_data, int frame_len);

int librecord_set_audio_params(int media_chn, int sample_rate, int channels);

int audio_save(int media_chn, unsigned char *audio_buf, int audio_len, int sample_rate, int channels);

int librecord_uninit(void);

#ifdef __cplusplus
}
#endif

#endif /*__librecord_h__*/