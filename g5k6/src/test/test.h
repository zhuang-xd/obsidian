#ifndef _TEST_H_
#define _TEST_H_

#include <stdint.h>

#if __cplusplus
extern "C"
{
#endif

    int32_t StartFileService(void);
    int32_t StopFileService(void);

    #if __cplusplus
}
#endif

#endif  // _TEST_H_