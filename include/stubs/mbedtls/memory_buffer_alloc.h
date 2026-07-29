#ifndef MBEDTLS_MEMORY_BUFFER_ALLOC_H
#define MBEDTLS_MEMORY_BUFFER_ALLOC_H
#include "Std_Types.h"
extern void mbedtls_memory_buffer_alloc_init(unsigned char* buf, size_t len);
extern void mbedtls_memory_buffer_alloc_free(void);
extern int mbedtls_memory_buffer_alloc_verify(void);
#endif
