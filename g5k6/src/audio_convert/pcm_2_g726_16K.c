#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <g7xx_api.h>
#include "fileop.h"


#define ONE_FRAME_SAMPLES (1024)
static char  data[ONE_FRAME_SAMPLES/4];
static short pcm[ONE_FRAME_SAMPLES];

int file_trans_pcm_2_g726_16K(int ifd, int ofd)
{
	int   n;
	G726_HANDLE h;

	h = fh_g726_create();
	if (!h)
	{
		printf("fh_g726_create: failed!!!\n");
		return 0;
	}
	
	while (1)
	{
		n = r_read(ifd, (char*)pcm, sizeof(pcm));
		if (n <= 0)
		{
			break;
		}

		n = fh_pcm_2_g726_16K(h, (unsigned char*)pcm, n, (unsigned char*)data);
		
		if (r_write(ofd, (char*)data, n) != n)
		{
			printf("Write file failed, abnormal terminated!!!\n");
			break;
		}
	}
	
	fh_g726_destroy(h);
	h = NULL;
	
	return 0;
}
