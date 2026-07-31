/* Minimal freestanding errno.h for arm-none-eabi builds. */
#ifndef _FREESTANDING_ERRNO_H
#define _FREESTANDING_ERRNO_H
extern int errno;
#define EDOM   33
#define ERANGE 34
#define EILSEQ 84
#define EINVAL 22
#define ENOMEM 12
#define EIO    5
#define ENOENT 2
#define EAGAIN 11
#endif
