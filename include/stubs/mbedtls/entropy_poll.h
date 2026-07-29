#ifndef MBEDTLS_ENTROPY_POLL_H
#define MBEDTLS_ENTROPY_POLL_H
#include "Std_Types.h"
extern int mbedtls_platform_entropy_poll(void* data, unsigned char* output, size_t len, size_t* olen);
extern int mbedtls_hardware_poll(void* data, unsigned char* output, size_t len, size_t* olen);
#endif
