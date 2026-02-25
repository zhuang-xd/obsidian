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
static char  data[ONE_FRAME_SAMPLES];
static short pcm[ONE_FRAME_SAMPLES];

int file_trans_pcm_2_g711U(int ifd, int ofd)
{
	int n;

	while (1)
	{
		n = r_read(ifd, (char*)pcm, sizeof(pcm));
		if (n <= 0)
		{
			break;
		}

		n = fh_pcm_2_g711U((unsigned char*)pcm, n, (unsigned char*)data);
		
		if (r_write(ofd, data, n) != n)
		{
			printf("Write file failed, abnormal terminated!!!\n");
			break;
		}
	}
	
	return 0;
}
