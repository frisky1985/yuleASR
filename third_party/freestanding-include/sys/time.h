/* Minimal freestanding sys/time.h for arm-none-eabi builds. */
#ifndef _FREESTANDING_SYS_TIME_H
#define _FREESTANDING_SYS_TIME_H
#include <sys/types.h>
struct timeval {
    long tv_sec;
    long tv_usec;
};
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};
int gettimeofday(struct timeval *tv, struct timezone *tz);
#endif
