/* Minimal freestanding sys/types.h for arm-none-eabi builds. */
#ifndef _FREESTANDING_SYS_TYPES_H
#define _FREESTANDING_SYS_TYPES_H
#include <stddef.h>
#include <stdint.h>
typedef long ssize_t;
typedef unsigned long off_t;
typedef unsigned int mode_t;
typedef long time_t;
typedef long clock_t;
typedef int pid_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
#endif
