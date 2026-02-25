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

int file_trans_g711U_2_pcm(int ifd, int ofd)
{
	int   n;
	
	while (1)
	{
		n = r_read(ifd, data, sizeof(data));
		if (n <= 0)
		{
			break;
		}

		n = fh_g711U_2_pcm((unsigned char*)data, n, (unsigned char*)pcm);
		
		if (r_write(ofd, (char*)pcm, n) != n)
		{
			printf("Write file failed, abnormal terminated!!!\n");
			break;
		}
	}
	
	return 0;
}
