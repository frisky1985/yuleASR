/***********************************************************************************************************************
 * File:        sha3_384.c
 * Description: SHA-3-384 implementation (FIPS 202, 384-bit digest)
 *              Keccak-f[1600] with 768-bit capacity and 832-bit rate
 **********************************************************************************************************************/

#include "hash_algos.h"
#include <string.h>

/*==================================================================================================
 *                                    SHA3-384 Constants
==================================================================================================*/

#define SHA3_384_BLOCK_SIZE         104U    /* 832 bits = 13 * 64-bit lanes */
#define SHA3_384_DIGEST_SIZE        48U     /* 384 bits */
#define SHA3_384_CAPACITY           96U     /* 768 bits */

static const uint64_t SHA3_ROUNDC[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
    0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
    0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
    0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
    0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
};

/*==================================================================================================
 *                                    Internal Functions
==================================================================================================*/

static uint64_t sha3_rol64(uint64_t val, uint8_t offset)
{
    return (val << offset) | (val >> (64 - offset));
}

static void sha3_keccak_f1600(uint64_t state[25])
{
    uint64_t a, b[5];
    uint8_t round;

    for (round = 0; round < 24; round++) {
        b[0] = state[0] ^ state[5] ^ state[10] ^ state[15] ^ state[20];
        b[1] = state[1] ^ state[6] ^ state[11] ^ state[16] ^ state[21];
        b[2] = state[2] ^ state[7] ^ state[12] ^ state[17] ^ state[22];
        b[3] = state[3] ^ state[8] ^ state[13] ^ state[18] ^ state[23];
        b[4] = state[4] ^ state[9] ^ state[14] ^ state[19] ^ state[24];

        for (int i = 0; i < 25; i++) {
            state[i] ^= b[(i + 4) % 5] ^ sha3_rol64(b[(i + 1) % 5], 1);
        }

        uint64_t tmp = state[1];
        state[1]  = sha3_rol64(state[6], 44);
        state[6]  = sha3_rol64(state[9], 20);
        state[9]  = sha3_rol64(state[22], 61);
        state[22] = sha3_rol64(state[14], 39);
        state[14] = sha3_rol64(state[20], 18);
        state[20] = sha3_rol64(state[2], 62);
        state[2]  = sha3_rol64(state[12], 43);
        state[12] = sha3_rol64(state[13], 25);
        state[13] = sha3_rol64(state[19], 8);
        state[19] = sha3_rol64(state[23], 56);
        state[23] = sha3_rol64(state[15], 41);
        state[15] = sha3_rol64(state[4], 27);
        state[4]  = sha3_rol64(state[24], 14);
        state[24] = sha3_rol64(state[21], 2);
        state[21] = sha3_rol64(state[8], 55);
        state[8]  = sha3_rol64(state[16], 45);
        state[16] = sha3_rol64(state[5], 36);
        state[5]  = sha3_rol64(state[3], 28);
        state[3]  = sha3_rol64(state[18], 21);
        state[18] = sha3_rol64(state[17], 15);
        state[17] = sha3_rol64(state[11], 10);
        state[11] = sha3_rol64(state[7], 6);
        state[7]  = sha3_rol64(state[10], 3);
        state[10] = sha3_rol64(tmp, 1);

        for (int j = 0; j < 25; j += 5) {
            uint64_t t[5];
            t[0] = state[j + 0]; t[1] = state[j + 1]; t[2] = state[j + 2];
            t[3] = state[j + 3]; t[4] = state[j + 4];
            state[j + 0] = t[0] ^ ((~t[1]) & t[2]);
            state[j + 1] = t[1] ^ ((~t[2]) & t[3]);
            state[j + 2] = t[2] ^ ((~t[3]) & t[4]);
            state[j + 3] = t[3] ^ ((~t[4]) & t[0]);
            state[j + 4] = t[4] ^ ((~t[0]) & t[1]);
        }

        state[0] ^= SHA3_ROUNDC[round];
    }
}

static void sha3_absorb(uint64_t state[25], const uint8_t* data, uint32_t numBlocks, uint32_t blockSize)
{
    for (uint32_t i = 0; i < numBlocks; i++) {
        for (uint32_t j = 0; j < blockSize / 8; j++) {
            state[j] ^= ((uint64_t)data[0] << 0)  |
                        ((uint64_t)data[1] << 8)  |
                        ((uint64_t)data[2] << 16) |
                        ((uint64_t)data[3] << 24) |
                        ((uint64_t)data[4] << 32) |
                        ((uint64_t)data[5] << 40) |
                        ((uint64_t)data[6] << 48) |
                        ((uint64_t)data[7] << 56);
            data += 8;
        }
        sha3_keccak_f1600(state);
    }
}

static void sha3_squeeze(uint64_t state[25], uint8_t* out, uint32_t numBytes, uint32_t rate)
{
    uint32_t i = 0;
    uint32_t rateWords = rate / 8;
    while (i < numBytes) {
        uint32_t j = 0;
        while (j < rateWords && i < numBytes) {
            uint64_t lane = state[j++];
            for (uint32_t k = 0; k < 8 && i < numBytes; k++) {
                out[i++] = (uint8_t)(lane >> (k * 8));
            }
        }
        if (i < numBytes) sha3_keccak_f1600(state);
    }
}

/*==================================================================================================
 *                                    SHA3-384 API
==================================================================================================*/

void SHA3_384_Init(SHA3_384_Context* ctx)
{
    memset(ctx, 0, sizeof(*ctx));
}

void SHA3_384_Update(SHA3_384_Context* ctx, const uint8_t* data, uint32_t len)
{
    const uint8_t* d = data;
    uint32_t blocks;

    if (ctx->bufLen > 0) {
        uint32_t need = SHA3_384_BLOCK_SIZE - ctx->bufLen;
        if (len < need) {
            memcpy(ctx->buffer + ctx->bufLen, d, len);
            ctx->bufLen += len;
            return;
        }
        memcpy(ctx->buffer + ctx->bufLen, d, need);
        sha3_absorb(ctx->state, ctx->buffer, 1, SHA3_384_BLOCK_SIZE);
        d += need;
        len -= need;
        ctx->bufLen = 0;
    }

    blocks = len / SHA3_384_BLOCK_SIZE;
    if (blocks > 0) {
        sha3_absorb(ctx->state, d, blocks, SHA3_384_BLOCK_SIZE);
        d += blocks * SHA3_384_BLOCK_SIZE;
        len -= blocks * SHA3_384_BLOCK_SIZE;
    }

    if (len > 0) {
        memcpy(ctx->buffer, d, len);
        ctx->bufLen = len;
    }
}

void SHA3_384_Final(SHA3_384_Context* ctx, uint8_t digest[48])
{
    uint32_t padLen = SHA3_384_BLOCK_SIZE - ctx->bufLen;
    ctx->buffer[ctx->bufLen] = 0x06;
    memset(ctx->buffer + ctx->bufLen + 1, 0, padLen - 2);
    ctx->buffer[SHA3_384_BLOCK_SIZE - 1] = 0x80;

    sha3_absorb(ctx->state, ctx->buffer, 1, SHA3_384_BLOCK_SIZE);
    sha3_squeeze(ctx->state, digest, SHA3_384_DIGEST_SIZE, SHA3_384_BLOCK_SIZE);

    memset(ctx, 0, sizeof(*ctx));
}

void SHA3_384(const uint8_t* data, uint32_t len, uint8_t digest[48])
{
    SHA3_384_Context ctx;
    SHA3_384_Init(&ctx);
    SHA3_384_Update(&ctx, data, len);
    SHA3_384_Final(&ctx, digest);
}
