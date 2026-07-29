#ifndef MBEDTLS_ERROR_H
#define MBEDTLS_ERROR_H
#include "Std_Types.h"
extern int mbedtls_strerror(int errnum, char* buffer, size_t buflen);
extern void mbedtls_strerror_high(int errnum, char* buffer, size_t buflen);
#endif
