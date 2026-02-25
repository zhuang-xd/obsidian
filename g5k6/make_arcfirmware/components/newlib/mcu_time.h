/*************************************************************************
    > File Name: mmu_time.h
    > Author: dongky341
    > Mail: dongky341@fullhan.com
    > Created Time: 2022年02月22日 星期二 18时41分21秒
 ************************************************************************/

#include <sys/time.h>

struct tm *arc_localtime_r(const time_t *t, struct tm *r);

void arc_settimeofday(int second);

void arc_gettimeofday(struct timeval *tv);

