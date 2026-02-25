#ifndef __fhconfig_h__
#define __fhconfig_h__

/*section for audio buffer, don't change it!*/
#define ACW_PLY_MEMORY_SIZE  (1024*8)
#define ACW_SWAP_MEMORY_SIZE (1024*2)
#define ACW_CAP_MEMORY_SIZE  (1024*2*4*5) //1024*2Byte*4Frames
#define ACW_RAW_MEMORY_SIZE  (0) //don't support raw

#define ACW_MEMORY_SIZE      (((ACW_CAP_MEMORY_SIZE + ACW_PLY_MEMORY_SIZE + ACW_RAW_MEMORY_SIZE + ACW_SWAP_MEMORY_SIZE) + (FH_PAGE_SIZE-1)) & \
                             (~(FH_PAGE_SIZE-1)))

/*section for memory layout, don't change!*/
#define FH_PAGE_SIZE      (4096)
#define XBUS_MEMORY_SIZE  (16 * FH_PAGE_SIZE)
#define XBUS_MEMORY_START (FH_DDR_END - XBUS_MEMORY_SIZE)
#define ACW_MEMORY_START  (XBUS_MEMORY_START - ACW_MEMORY_SIZE)
#define OS_SDRAM_END      (ACW_MEMORY_START)

//#define FH_USING_RUNLOG

/*select console for print*/
#define FH_USING_XBRSH /*using rshell based on xbus*/

#define FH_USING_VMM_ON_ARC
#define FH_USING_START_VIDEO_ON_ARC


/*enable shell command*/
#define FH_USING_SHELL

/*enable audio*/
#define FH_USING_ACW

/*enable softcore for DSP*/
#define FH_USING_EXTLIB_SOFTCORE

/*thread*/
#define FH_USING_SHELL_IN_MAIN  /*run shell in main thread...*/
#define FH_MAIN_THREAD_PRIORITY       (25)
#define FH_MAIN_THREAD_STACK_SIZE     (4096)
#define FH_SHELL_THREAD_PRIORITY      (9)
#define FH_SHELL_THREAD_STACK_SIZE    (4096)
#define FH_XBUS_THREAD_NUM            (16)

#define FH_XBUS_THREAD_STACK_SIZE     (16384 - 64)

#define FH_XbusDspLock_THREAD_STACK_SIZE  (4096) /*don't modify*/
#define FH_XbusRx_THREAD_STACK_SIZE       (4096) /*don't modify*/

#define FH_DEMO_THREAD_PRIORITY      (25)

#define CONFIG_EXT_INTC
#endif /*__fhconfig_h__*/
