#ifndef __file_op___h__
#define __file_op___h__

extern int r_read(int fd, char* buf, int len);
extern int r_write(int fd, char* buf, int len);
extern int copy_file(int infd, int outfd);

#endif //__file_op___h__

