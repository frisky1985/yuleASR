#ifndef MBEDTLS_PLATFORM_H
#define MBEDTLS_PLATFORM_H
#include "Std_Types.h"
#define MBEDTLS_PLATFORM_STD_PRINTF   printf
#define MBEDTLS_PLATFORM_STD_SNPRINTF snprintf
#define MBEDTLS_PLATFORM_STD_EXIT     exit
#define MBEDTLS_PLATFORM_STD_FPRINTF  fprintf
extern void mbedtls_platform_set_printf(void* func);
extern void mbedtls_platform_set_snprintf(void* func);
extern void mbedtls_platform_set_exit(void* func);
extern void mbedtls_platform_set_fprintf(void* func);
#endif
