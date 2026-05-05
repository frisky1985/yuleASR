/***********************************************************************************************************************
 * File:        sha3_224.c
 * Description: SHA-3-224 implementation (FIPS 202, 224-bit digest)
 *              Keccak-f[1600] with 448-bit capacity and 1152-bit rate
 **********************************************************************************************************************/

#include "hash_algos.h"
#include <string.h>

/*==================================================================================================
 *                                    SHA3-224 Constants
==================================================================================================*/

/* SHA3-224 parameters */
#define SHA3_224_BLOCK_SIZE         144U    /* 1152 bits = 18 * 64-bit lanes */
#define SHA3_224_DIGEST_SIZE        28U     /* 224 bits */
#define SHA3_224_CAPACITY           56U     /* 448 bits = c = 2 * hashlen */

/* Keccak Round Constants */
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

/* Keccak Rotation Offsets */
static const uint8_t ROTC[24] = {
    1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 2, 14,
    27, 41, 56, 8, 25, 43, 62, 18, 39, 61, 20, 44
};

/*==================================================================================================
 *                                    Internal Functions
==================================================================================================*/

/* ROL64 - 64-bit rotate left */
static uint64_t sha3_rol64(uint64_t val, uint8_t offset)
{
    return (val << offset) | (val >> (64 - offset));
}

/* Keccak-f[1600] permutation (24 rounds) */
static void sha3_keccak_f1600(uint64_t state[25])
{
    uint64_t a, b[5];
    uint8_t round;

    for (round = 0; round < 24; round++) {
        /* Theta step */
        b[0] = state[0] ^ state[5] ^ state[10] ^ state[15] ^ state[20];
        b[1] = state[1] ^ state[6] ^ state[11] ^ state[16] ^ state[21];
        b[2] = state[2] ^ state[7] ^ state[12] ^ state[17] ^ state[22];
        b[3] = state[3] ^ state[8] ^ state[13] ^ state[18] ^ state[23];
        b[4] = state[4] ^ state[9] ^ state[14] ^ state[19] ^ state[24];

        state[0]  ^= b[4] ^ sha3_rol64(b[1], 1);
        state[1]  ^= b[0] ^ sha3_rol64(b[2], 1);
        state[2]  ^= b[1] ^ sha3_rol64(b[3], 1);
        state[3]  ^= b[2] ^ sha3_rol64(b[4], 1);
        state[4]  ^= b[3] ^ sha3_rol64(b[0], 1);
        state[5]  ^= b[4] ^ sha3_rol64(b[1], 1);
        state[6]  ^= b[0] ^ sha3_rol64(b[2], 1);
        state[7]  ^= b[1] ^ sha3_rol64(b[3], 1);
        state[8]  ^= b[2] ^ sha3_rol64(b[4], 1);
        state[9]  ^= b[3] ^ sha3_rol64(b[0], 1);
        state[10] ^= b[4] ^ sha3_rol64(b[1], 1);
        state[11] ^= b[0] ^ sha3_rol64(b[2], 1);
        state[12] ^= b[1] ^ sha3_rol64(b[3], 1);
        state[13] ^= b[2] ^ sha3_rol64(b[4], 1);
        state[14] ^= b[3] ^ sha3_rol64(b[0], 1);
        state[15] ^= b[4] ^ sha3_rol64(b[1], 1);
        state[16] ^= b[0] ^ sha3_rol64(b[2], 1);
        state[17] ^= b[1] ^ sha3_rol64(b[3], 1);
        state[18] ^= b[2] ^ sha3_rol64(b[4], 1);
        state[19] ^= b[3] ^ sha3_rol64(b[0], 1);
        state[20] ^= b[4] ^ sha3_rol64(b[1], 1);
        state[21] ^= b[0] ^ sha3_rol64(b[2], 1);
        state[22] ^= b[1] ^ sha3_rol64(b[3], 1);
        state[23] ^= b[2] ^ sha3_rol64(b[4], 1);
        state[24] ^= b[3] ^ sha3_rol64(b[0], 1);

        /* Rho and Pi steps */
        a = state[1];
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
        state[10] = sha3_rol64(a, 1);

        /* Chi step */
        #define CHI(i, j, k) \
            a = state[i]; \
            b[0] = state[j]; \
            b[1] = state[k]; \
            state[i] = a ^ ((~b[0]) & b[1]);

        for (int j = 0; j < 25; j += 5) {
            CHI(j+0, j+1, j+2);
            CHI(j+1, j+2, j+3);
            CHI(j+2, j+3, j+4);
            CHI(j+3, j+4, j+0);
            CHI(j+4, j+0, j+1);
        }

        /* Iota step - XOR with round constant */
        state[0] ^= SHA3_ROUNDC[round];
    }
}

/* Absorb complete blocks into state */
static void sha3_absorb(uint64_t state[25], const uint8_t* data, uint32_t numBlocks, uint32_t blockSize)
{
    uint32_t i, j;
    for (i = 0; i < numBlocks; i++) {
        for (j = 0; j < blockSize / 8; j++) {
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

/* Squeeze output from state */
static void sha3_squeeze(uint64_t state[25], uint8_t* out, uint32_t numBytes, uint32_t rate)
{
    uint32_t i = 0;
    uint32_t rateWords = rate / 8;

    while (i < numBytes) {
        uint32_t j = 0;
        while (j < rateWords && i < numBytes) {
            uint64_t lane = state[j++];
            uint32_t k = 0;
            while (k < 8 && i < numBytes) {
                out[i++] = (uint8_t)(lane >> (k * 8));
                k++;
            }
        }
        if (i < numBytes) {
            sha3_keccak_f1600(state);
        }
    }
}

/*==================================================================================================
 *                                    SHA3-224 API
==================================================================================================*/

/* Initialize SHA3-224 context */
void SHA3_224_Init(SHA3_224_Context* ctx)
{
    memset(ctx, 0, sizeof(*ctx));
}

/* Update SHA3-224 with data */
void SHA3_224_Update(SHA3_224_Context* ctx, const uint8_t* data, uint32_t len)
{
    const uint8_t* d = data;
    uint32_t blocks;

    /* Process buffered data first */
    if (ctx->bufLen > 0) {
        uint32_t need = SHA3_224_BLOCK_SIZE - ctx->bufLen;
        if (len < need) {
            memcpy(ctx->buffer + ctx->bufLen, d, len);
            ctx->bufLen += len;
            return;
        }
        memcpy(ctx->buffer + ctx->bufLen, d, need);
        sha3_absorb(ctx->state, ctx->buffer, 1, SHA3_224_BLOCK_SIZE);
        d += need;
        len -= need;
        ctx->bufLen = 0;
    }

    /* Process complete blocks */
    blocks = len / SHA3_224_BLOCK_SIZE;
    if (blocks > 0) {
        sha3_absorb(ctx->state, d, blocks, SHA3_224_BLOCK_SIZE);
        d += blocks * SHA3_224_BLOCK_SIZE;
        len -= blocks * SHA3_224_BLOCK_SIZE;
    }

    /* Save remainder for next update */
    if (len > 0) {
        memcpy(ctx->buffer, d, len);
        ctx->bufLen = len;
    }
}

/* Finalize SHA3-224 and output digest */
void SHA3_224_Final(SHA3_224_Context* ctx, uint8_t digest[28])
{
    uint32_t padLen = SHA3_224_BLOCK_SIZE - ctx->bufLen;

    /* SHA3 uses multi-rate padding: 0x06 || 0x00* || 0x80 */
    ctx->buffer[ctx->bufLen] = 0x06;
    memset(ctx->buffer + ctx->bufLen + 1, 0, padLen - 2);
    ctx->buffer[SHA3_224_BLOCK_SIZE - 1] = 0x80;

    sha3_absorb(ctx->state, ctx->buffer, 1, SHA3_224_BLOCK_SIZE);
    sha3_squeeze(ctx->state, digest, SHA3_224_DIGEST_SIZE, SHA3_224_BLOCK_SIZE);

    /* Clear sensitive data */
    memset(ctx, 0, sizeof(*ctx));
}

/* One-shot SHA3-224 */
void SHA3_224(const uint8_t* data, uint32_t len, uint8_t digest[28])
{
    SHA3_224_Context ctx;
    SHA3_224_Init(&ctx);
    SHA3_224_Update(&ctx, data, len);
    SHA3_224_Final(&ctx, digest);
}
