#ifndef MBEDTLS_HKDF_H
#define MBEDTLS_HKDF_H
#include "Std_Types.h"
extern int mbedtls_hkdf(const mbedtls_md_info_t* md, const unsigned char* salt, size_t salt_len, const unsigned char* ikm, size_t ikm_len, const unsigned char* info, size_t info_len, unsigned char* okm, size_t okm_len);
extern int mbedtls_hkdf_expand(const mbedtls_md_info_t* md, const unsigned char* prk, size_t prk_len, const unsigned char* info, size_t info_len, unsigned char* okm, size_t okm_len);
extern int mbedtls_hkdf_extract(const mbedtls_md_info_t* md, const unsigned char* salt, size_t salt_len, const unsigned char* ikm, size_t ikm_len, unsigned char* prk, size_t* prk_len);
#endif
