/*==================================================================================================
 * platform.h - mbedTLS platform abstraction stub (yuleASR)
 *================================================================================================*/
#ifndef MBEDTLS_PLATFORM_H
#define MBEDTLS_PLATFORM_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MBEDTLS_PLATFORM_STD_CALLOC     calloc
#define MBEDTLS_PLATFORM_STD_FREE       free

void *mbedtls_calloc(size_t n, size_t size);
void  mbedtls_free(void *ptr);

#endif /* MBEDTLS_PLATFORM_H */
