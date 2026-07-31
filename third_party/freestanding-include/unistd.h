/* Minimal freestanding unistd.h for arm-none-eabi builds. */
#ifndef _FREESTANDING_UNISTD_H
#define _FREESTANDING_UNISTD_H
#include <stddef.h>
#include <sys/types.h>
#ifdef __cplusplus
extern "C" {
#endif
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int close(int fd);
unsigned int sleep(unsigned int seconds);
#ifdef __cplusplus
}
#endif
#endif
