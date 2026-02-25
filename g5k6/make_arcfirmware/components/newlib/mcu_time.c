/*************************************************************************
    > File Name: mmu_time.c
    > Author: dongky341
    > Mail: dongky341@fullhan.com
    > Created Time: 2022年02月22日 星期二 18时41分17秒
 ************************************************************************/

#include <stdio.h>
#include <rtdef.h>
#include <rtthread.h>
#include <fh_timer.h>
#include "mcu_time.h"

#define SPD 24 * 60 * 60

static int g_sys_second = 0;

const short __spm[13] = {
    0,
    (31),
    (31 + 28),
    (31 + 28 + 31),
    (31 + 28 + 31 + 30),
    (31 + 28 + 31 + 30 + 31),
    (31 + 28 + 31 + 30 + 31 + 30),
    (31 + 28 + 31 + 30 + 31 + 30 + 31),
    (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31),
    (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30),
    (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31),
    (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30),
    (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31),
};

static int __isleap(int year)
{
    /* every fourth year is a leap year except for century years that are
     * not divisible by 400. */
    /*  return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)); */
    return (!(year % 4) && ((year % 100) || !(year % 400)));
}

static struct tm *arc_gmtime_r(const time_t *timep, struct tm *r)
{
    time_t i;
    time_t work = *timep % (SPD);
    r->tm_sec = work % 60;
    work /= 60;
    r->tm_min  = work % 60;
    r->tm_hour = work / 60;
    work       = *timep / (SPD);
    r->tm_wday = (4 + work) % 7;
    for (i = 1970;; ++i)
    {
        time_t k = __isleap(i) ? 366 : 365;
        if (work >= k)
            work -= k;
        else
            break;
    }
    r->tm_year = i - 1900;
    r->tm_yday = work;

    r->tm_mday = 1;
    if (__isleap(i) && (work > 58))
    {
        if (work == 59) r->tm_mday = 2; /* 29.2. */
        work -= 1;
    }

    for (i = 11; i && (__spm[i] > work); --i)
        ;
    r->tm_mon = i;
    r->tm_mday += work - __spm[i];
    return r;
}

struct tm *arc_localtime_r(const time_t *t, struct tm *r)
{
    return arc_gmtime_r(t, r);
}

static unsigned long long g_last_pts = 0;
void arc_settimeofday(int second)
{
    g_sys_second = second;
    g_last_pts = read_pts_new();
}

void arc_gettimeofday(struct timeval *tv)
{
    unsigned long long pts = 0;

    pts = read_pts_new() - g_last_pts;

    tv->tv_sec = g_sys_second + pts / 1000000;
    tv->tv_usec = pts % 1000000;
}
