#ifndef MBEDTLS_SHA384_H
#define MBEDTLS_SHA384_H
#include "Std_Types.h"
#include "mbedtls/sha512.h"
#define MBEDTLS_SHA384_DIGEST_SIZE 48
extern int mbedtls_sha384(const unsigned char* input, size_t ilen, unsigned char* output);
#endif
