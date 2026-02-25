#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <g7xx_api.h>
#include <aacenc_api.h>
#include "fileop.h"


#define ONE_FRAME_SAMPLES (1024)
static char  data[ONE_FRAME_SAMPLES]; //please ensure the buffer is large enough to keep one encoded frame...
static short pcm[ONE_FRAME_SAMPLES];

/*
static int get_random_samples(void)
{
	static int first = 1;

	unsigned int s;

	if (first)
	{
		srandom(time(0));
		first = 0;
	}

	s = random();
	s %= ONE_FRAME_SAMPLES;
	
	return s+1;
}
*/


int file_trans_pcm_2_aac(int ifd, int ofd, int channels, int sample_rate, int bit_rate)
{
	int   n;
	AAC_ENC_HANDLE h;

	h = fh_aacenc_create(channels, sample_rate, bit_rate);
	if (!h)
	{
		printf("fh_aacenc_create: failed!!!\n");
		return 0;
	}
	
	while (1)
	{
		n = r_read(ifd, (char*)pcm, sizeof(pcm));
		if (n <= 0)
		{
			break;
		}

		n = fh_aacenc_encode(h, (unsigned char*)pcm, n, (unsigned char*)data, sizeof(data));
		if (n > 0)
		{
			if (r_write(ofd, (char*)data, n) != n)
			{
				printf("Write file failed @ aacenc, abnormal terminated!!!\n");
				break;
			}
		}
	}
	
	fh_aacenc_destroy(h);
	h = NULL;
	
	return 0;
}
