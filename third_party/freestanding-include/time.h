/* Minimal freestanding time.h for arm-none-eabi builds. */
#ifndef _FREESTANDING_TIME_H
#define _FREESTANDING_TIME_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef long time_t;
typedef long clock_t;
struct tm {
    int tm_sec; int tm_min; int tm_hour; int tm_mday;
    int tm_mon; int tm_year; int tm_wday; int tm_yday;
    int tm_isdst;
};
#define CLOCKS_PER_SEC 1000000
time_t time(time_t *tloc);
clock_t clock(void);
struct tm *gmtime(const time_t *timer);
struct tm *localtime(const time_t *timer);
time_t mktime(struct tm *timeptr);
size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);
#ifdef __cplusplus
}
#endif
#endif
