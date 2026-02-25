#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <g7xx_api.h>
#include "fileop.h"
#include <aacdec_api.h>


#define ONE_FRAME_SAMPLES (1024)
static char  data[256];
static short pcm[ONE_FRAME_SAMPLES]; //please ensure the buffer is large enough to keep one decoded frame...

int file_trans_aac_2_pcm(int ifd, int ofd)
{
	int   frames = 0;
	int   n;
	int   ret;
	int   valid_bytes;
	bitstream_info_t info;
	AAC_DEC_HANDLE h;
	
	h = fh_aacdec_create(AAC_TT_MP4_ADTS);
	if (!h)
	{
		printf("fh_aacdec_create: failed!!!\n");
		return 0;
	}
	
	while (1)
	{
		n = r_read(ifd, (char*)data, sizeof(data));
		if (n <= 0)
		{
			break;
		}

		valid_bytes = n;
		do
		{
			fh_aacdec_fill_bitstream(h, (unsigned char*)data, n, &valid_bytes);

			do {
				ret = fh_aacdec_decode_frame(h, (unsigned char*)pcm, sizeof(pcm), &info);
				if (ret != AACDEC_OK)
				{
					if (ret != AACDEC_NOT_ENOUGH_BITS)
					{
						printf("fh_aacdec_decode_frame: decoder internal error!!!\n");
					}
					break;
				}

				printf("DEC%4d: channels=%d,    sample_rate=%d,    frame_size=%d,     bit_rate=%d.\n", 
						++frames,
						info.channels,
						info.sample_rate, 
						info.frame_size,
						info.bit_rate);


				if (r_write(ofd, (char*)pcm, info.frame_size*2) != info.frame_size*2)
				{
					printf("Write file failed @ aac_dec, abnormal terminated!!!\n");
					break;
				}
			} while (1);
		}while (valid_bytes > 0);

	}
	
	fh_aacdec_destroy(h);
	h = NULL;
	
	return 0;
}
