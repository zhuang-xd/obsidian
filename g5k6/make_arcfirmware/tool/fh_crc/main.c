#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include "fh_crc32.c"

int main(int argc, char *argv[])
{
    int ret;
    int fd;
    int flen;
    unsigned int crc = 0;
    struct stat stat;
    unsigned char* buff;

    if (argc != 3)
    {
        printf("Error: please supply file name!\n");
        return -1;
    }

    fd  = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        printf("Error: cann't open input file!\n");
        return -1;
    }

    ret = fstat(fd, &stat);  
    if (ret != 0 || stat.st_size < 4096 || stat.st_size > 32*1024*1024)
    {
        printf("Error: fstat failed!\n");
        return -1;
    }
    flen = stat.st_size;

    buff = malloc(flen + 16/*just more space*/);
    if (!buff)
    {
        printf("Error: malloc failed!\n");
        return -1;
    }

    ret = read(fd, buff, flen);
    if (ret != flen)
    {
        printf("Error: read failed!\n");
        return -1;
    }

    close(fd);

    /*append to 4Bytes aligned*/
    while (flen & 3)
    {
        buff[flen++] = 0;
    }

    crc = fh_crc32(crc, buff, flen);
    *((unsigned int *)(&buff[flen])) = crc;
    flen += 4;

    fd  = open(argv[2], O_RDWR | O_TRUNC | O_CREAT);
    if (fd < 0)
    {
        printf("Error: cann't open output file!\n");
        return -1;
    }


    ret = write(fd, buff, flen);
    if (ret != flen)
    {
        printf("Error: write failed!\n");
        return -1;
    }

    fchmod(fd, 0644);

    close(fd);
    free(buff);

    printf("Add CRC32 OK!\n");
    return 0;
}
