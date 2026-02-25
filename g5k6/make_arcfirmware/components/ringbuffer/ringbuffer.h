#ifndef __ringbuffer_h__
#define __ringbuffer_h__

/*don't care this struct, just put it here...*/
#define _END_FLAG_ (0xf9f9f9f9)
#define _PAD_SIZE_ (32)
typedef struct
{
    volatile unsigned int  rdpos;
    volatile unsigned int  size; /*not include the header itself*/
    volatile unsigned char reserved1[_PAD_SIZE_ - 2*sizeof(unsigned int)];

    volatile unsigned int  wrpos;
    volatile unsigned char reserved2[_PAD_SIZE_ - sizeof(unsigned int)];

    unsigned char buf[0];
} ringbuffer_t;

typedef void* HRINGBUFFER;

/*functions call on ARC*/
HRINGBUFFER ringbuffer_init(void* mem, int memsz);
void ringbuffer_print(HRINGBUFFER h, const char *fmt, ...);
void ringbuffer_set_endflag(HRINGBUFFER h);


/*functions call on ARM*/
/*
 * h: it's the mmaped virtual address, it must be non-cached...
 * Return: >= 0: the real read bytes.
 *           <0: error.
 */
int ringbuffer_read(HRINGBUFFER h, char* buf, int bufsz, int* end);

/*just put it here...*/
int proc_register(char *name,  int (*read)(void *handle), int (*write)(char *buf, int size));

#endif //__ringbuffer_h__
