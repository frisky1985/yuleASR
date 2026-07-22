#ifndef BLAKE2_H
#define BLAKE2_H
#include <stddef.h>
#include <stdint.h>
int blake2b(uint8_t* out, size_t outlen, const uint8_t* in, size_t inlen, const uint8_t* key, size_t keylen);
int blake2s(uint8_t* out, size_t outlen, const uint8_t* in, size_t inlen, const uint8_t* key, size_t keylen);
#endif
