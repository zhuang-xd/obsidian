#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int  r_read(int fd, char* buf, int len)
{
	int nread = 0;
	int ret;

	while(len > 0)
	{
		ret = read(fd, buf, len);
		if (ret <= 0)
		{
			break;
		}

		nread += ret;
		buf += ret;
		len -= ret;
	}

	return nread;
}


int  r_write(int fd, char* buf, int len)
{
	int ret;
	int nwrite = 0;

	while(len > 0)
	{
		ret = write(fd, buf, len);
		if (ret > 0) 
		{
			buf += ret;
			len -= ret;
			nwrite += ret;
		}
	}

	return nwrite;
}
		
int copy_file(int infd, int outfd)
{
	int len;
	char buf[512];

	while (1)
	{
		len = r_read(infd, buf, sizeof(buf));
		if (len <= 0)
			break;

		if (r_write(outfd, buf, len) != len)
		{
			printf("copy_file: Write file failed, abnormal terminated!!!\n");
			break;
		}

	}

	return 0;
}
