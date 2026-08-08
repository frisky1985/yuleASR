/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : portable (any C99)
* Dependencies         : none (stdint.h only)
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file    Lib_Aes.c
 * @brief   Independent AES block cipher library — implementation (FIPS-197)
 * @version 1.0.0
 * @date    2026-08-09
 *
 * @details
 *   Byte-oriented AES (FIPS-197) with programmatically generated S-box
 *   (GF(2^8) multiplicative inverse + affine transform).  Correctness is
 *   verified by the unit tests against the NIST FIPS-197 appendix C
 *   vectors (AES-128/192/256) and SP 800-38A CBC vectors.
 *
 *   The S-box tables are generated once at the first Lib_AesInit call;
 *   Lib_AesInit must be called from a single thread (typical for BSW
 *   init phase before scheduler start).
 */

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "Lib_Aes.h"
#include <string.h>

/*==================================================================================================
 *                                         LOCAL MACROS
 *==================================================================================================*/

/** @brief xtime: multiplication by x in GF(2^8) with the AES polynomial */
#define AES_XTIME(a) \
    ((uint8_t)(((a) & 0x80u) ? (uint8_t)(((a) << 1u) ^ 0x1Bu) : (uint8_t)((a) << 1u)))

/** @brief Rotate a byte left by n bits */
#define AES_ROTL8(v, n) \
    ((uint8_t)(((v) << (n)) | ((v) >> (8u - (n)))))

/** @brief Number of 32-bit words in the key (AES-128/192/256) */
#define AES_NK_128                         (4u)
#define AES_NK_192                         (6u)
#define AES_NK_256                         (8u)

/** @brief Number of rounds (AES-128/192/256) */
#define AES_NR_128                         (10u)
#define AES_NR_192                         (12u)
#define AES_NR_256                         (14u)

/** @brief Number of columns in the state (fixed by FIPS-197) */
#define AES_NB                             (4u)

/*==================================================================================================
 *                                         MODULE STATE (S-BOX TABLES)
 *==================================================================================================*/

static uint8_t Sbox[256];
static uint8_t InvSbox[256];
static uint8_t TablesReady = 0u;

/*==================================================================================================
 *                                         LOCAL FUNCTION PROTOTYPES
 *==================================================================================================*/
static uint8_t GfMul(uint8_t a, uint8_t n);
static uint8_t GfInv(uint8_t a);
static void GenerateTables(void);
static uint32_t SubWord(uint32_t word);
static uint32_t RotWord(uint32_t word);
static int ExpandKey(Lib_AesContextType* ctx, const uint8_t* key);
static void AddRoundKey(uint8_t* state, const uint32_t* w, uint8_t round);
static void SubBytes(uint8_t* state);
static void InvSubBytes(uint8_t* state);
static void ShiftRows(uint8_t* state);
static void InvShiftRows(uint8_t* state);
static void MixColumns(uint8_t* state);
static void InvMixColumns(uint8_t* state);

/*==================================================================================================
 *                                         LOCAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Multiply two elements of GF(2^8) (Russian peasant, AES polynomial)
 */
static uint8_t GfMul(uint8_t a, uint8_t n)
{
    uint8_t result = 0u;

    while (n != 0u)
    {
        if ((n & 1u) != 0u)
        {
            result = (uint8_t)(result ^ a);
        }
        n = (uint8_t)(n >> 1u);
        if (n != 0u)
        {
            a = AES_XTIME(a);
        }
    }
    return result;
}

/**
 * @brief Multiplicative inverse in GF(2^8); GfInv(0) = 0 (per FIPS-197)
 */
static uint8_t GfInv(uint8_t a)
{
    uint8_t x;

    if (a == 0u)
    {
        return 0u;
    }
    for (x = 1u; x != 0u; x++)
    {
        if (GfMul(a, x) == 1u)
        {
            return x;
        }
    }
    return 0u;  /* unreachable: every non-zero element has an inverse */
}

/**
 * @brief Generate the AES S-box (and inverse) from the FIPS-197 definition
 */
static void GenerateTables(void)
{
    uint16_t i;

    if (TablesReady != 0u)
    {
        return;
    }

    for (i = 0u; i < 256u; i++)
    {
        uint8_t b = (uint8_t)i;
        uint8_t inv = GfInv(b);
        uint8_t x = inv;

        /* Affine transform (FIPS-197 5.1.1) */
        x = (uint8_t)(x ^ AES_ROTL8(x, 1u) ^ AES_ROTL8(x, 2u) ^
                      AES_ROTL8(x, 3u) ^ AES_ROTL8(x, 4u) ^ 0x63u);
        Sbox[i] = x;
    }

    for (i = 0u; i < 256u; i++)
    {
        InvSbox[Sbox[i]] = (uint8_t)i;
    }

    TablesReady = 1u;
}

/**
 * @brief Substitute each byte of a 32-bit word with the S-box value
 */
static uint32_t SubWord(uint32_t word)
{
    uint32_t result = 0u;
    uint8_t i;

    for (i = 0u; i < 4u; i++)
    {
        uint8_t byte = (uint8_t)((word >> (8u * (3u - i))) & 0xFFu);
        result |= ((uint32_t)Sbox[byte]) << (8u * (3u - i));
    }
    return result;
}

/**
 * @brief Rotate a 32-bit word left by one byte
 */
static uint32_t RotWord(uint32_t word)
{
    return ((word << 8u) | (word >> 24u));
}

/**
 * @brief AES key expansion (FIPS-197 5.2)
 */
static int ExpandKey(Lib_AesContextType* ctx, const uint8_t* key)
{
    uint8_t nk;
    uint8_t nr;
    uint32_t* w;
    uint8_t i;
    uint32_t temp;

    switch (ctx->keyLen)
    {
        case LIB_AES_KEY_128:
            nk = AES_NK_128;
            nr = AES_NR_128;
            break;
        case LIB_AES_KEY_192:
            nk = AES_NK_192;
            nr = AES_NR_192;
            break;
        case LIB_AES_KEY_256:
            nk = AES_NK_256;
            nr = AES_NR_256;
            break;
        default:
            return LIB_AES_ERR_KEY_LEN;
    }

    ctx->rounds = nr;
    w = ctx->roundKeys;

    /* Initial Nk words from the raw key (big-endian) */
    for (i = 0u; i < nk; i++)
    {
        w[i] = ((uint32_t)key[4u * i] << 24u)
             | ((uint32_t)key[4u * i + 1u] << 16u)
             | ((uint32_t)key[4u * i + 2u] << 8u)
             | ((uint32_t)key[4u * i + 3u]);
    }

    /* Remaining words */
    for (i = nk; i < (uint8_t)(AES_NB * (uint32_t)(nr + 1u)); i++)
    {
        temp = w[i - 1u];

        if ((i % nk) == 0u)
        {
            /* temp = SubWord(RotWord(temp)) ^ Rcon[i/nk] (FIPS-197 5.2) */
            uint8_t rconIndex = (uint8_t)((uint32_t)i / (uint32_t)nk);
            uint32_t rcon = 0x01000000u;   /* Rcon[1] = 0x01, byte 3 */
            uint8_t r;

            for (r = 1u; r < rconIndex; r++)
            {
                rcon = ((uint32_t)AES_XTIME((uint8_t)(rcon >> 24u))) << 24u;
            }
            temp = SubWord(RotWord(temp)) ^ rcon;
        }
        else if ((nk > AES_NK_192) && ((i % nk) == 4u))
        {
            temp = SubWord(temp);
        }

        w[i] = w[i - nk] ^ temp;
    }

    return LIB_AES_OK;
}

/**
 * @brief XOR the round key into the state
 */
static void AddRoundKey(uint8_t* state, const uint32_t* w, uint8_t round)
{
    uint8_t i;

    for (i = 0u; i < 16u; i++)
    {
        uint8_t wordIndex = (uint8_t)((uint32_t)round * AES_NB + ((uint32_t)i / 4u));
        uint8_t shift = (uint8_t)(8u * (3u - ((uint32_t)i % 4u)));
        state[i] = (uint8_t)(state[i] ^ ((uint8_t)((w[wordIndex] >> shift) & 0xFFu)));
    }
}

/**
 * @brief Substitute all state bytes
 */
static void SubBytes(uint8_t* state)
{
    uint8_t i;

    for (i = 0u; i < 16u; i++)
    {
        state[i] = Sbox[state[i]];
    }
}

/**
 * @brief Inverse substitute all state bytes
 */
static void InvSubBytes(uint8_t* state)
{
    uint8_t i;

    for (i = 0u; i < 16u; i++)
    {
        state[i] = InvSbox[state[i]];
    }
}

/**
 * @brief Shift rows (FIPS-197 5.1.2)
 */
static void ShiftRows(uint8_t* state)
{
    uint8_t t;

    /* Row 1: shift left by 1 */
    t = state[1];
    state[1] = state[5];
    state[5] = state[9];
    state[9] = state[13];
    state[13] = t;

    /* Row 2: shift left by 2 (swap pairs) */
    t = state[2];
    state[2] = state[10];
    state[10] = t;
    t = state[6];
    state[6] = state[14];
    state[14] = t;

    /* Row 3: shift left by 3 */
    t = state[15];
    state[15] = state[11];
    state[11] = state[7];
    state[7] = state[3];
    state[3] = t;
}

/**
 * @brief Inverse shift rows (FIPS-197 5.3.1)
 */
static void InvShiftRows(uint8_t* state)
{
    uint8_t t;

    /* Row 1: shift right by 1 */
    t = state[13];
    state[13] = state[9];
    state[9] = state[5];
    state[5] = state[1];
    state[1] = t;

    /* Row 2: shift right by 2 (swap pairs, same as ShiftRows) */
    t = state[2];
    state[2] = state[10];
    state[10] = t;
    t = state[6];
    state[6] = state[14];
    state[14] = t;

    /* Row 3: shift right by 1 (== shift left by 3) */
    t = state[3];
    state[3] = state[7];
    state[7] = state[11];
    state[11] = state[15];
    state[15] = t;
}

/**
 * @brief Mix columns (FIPS-197 5.1.3, XOR-optimized form)
 */
static void MixColumns(uint8_t* state)
{
    uint8_t c;

    for (c = 0u; c < AES_NB; c++)
    {
        uint8_t i = (uint8_t)(4u * c);
        uint8_t a0 = state[i];
        uint8_t a1 = state[i + 1u];
        uint8_t a2 = state[i + 2u];
        uint8_t a3 = state[i + 3u];

        state[i]       = (uint8_t)(AES_XTIME((uint8_t)(a0 ^ a1)) ^ a1 ^ a2 ^ a3);
        state[i + 1u]  = (uint8_t)(a0 ^ AES_XTIME((uint8_t)(a1 ^ a2)) ^ a2 ^ a3);
        state[i + 2u]  = (uint8_t)(a0 ^ a1 ^ AES_XTIME((uint8_t)(a2 ^ a3)) ^ a3);
        state[i + 3u]  = (uint8_t)(AES_XTIME((uint8_t)(a0 ^ a3)) ^ a0 ^ a1 ^ a2);
    }
}

/**
 * @brief Inverse mix columns (FIPS-197 5.3.3)
 */
static void InvMixColumns(uint8_t* state)
{
    uint8_t c;

    for (c = 0u; c < AES_NB; c++)
    {
        uint8_t i = (uint8_t)(4u * c);
        uint8_t a0 = state[i];
        uint8_t a1 = state[i + 1u];
        uint8_t a2 = state[i + 2u];
        uint8_t a3 = state[i + 3u];

        state[i]       = (uint8_t)(GfMul(a0, 14u) ^ GfMul(a1, 11u) ^ GfMul(a2, 13u) ^ GfMul(a3, 9u));
        state[i + 1u]  = (uint8_t)(GfMul(a0, 9u) ^ GfMul(a1, 14u) ^ GfMul(a2, 11u) ^ GfMul(a3, 13u));
        state[i + 2u]  = (uint8_t)(GfMul(a0, 13u) ^ GfMul(a1, 9u) ^ GfMul(a2, 14u) ^ GfMul(a3, 11u));
        state[i + 3u]  = (uint8_t)(GfMul(a0, 11u) ^ GfMul(a1, 13u) ^ GfMul(a2, 9u) ^ GfMul(a3, 14u));
    }
}

/*==================================================================================================
 *                                         GLOBAL FUNCTIONS
 *==================================================================================================*/

int Lib_AesInit(Lib_AesContextType* ctx, const uint8_t* key, Lib_AesKeyLenType keyLen)
{
    int result;

    if ((ctx == NULL) || (key == NULL))
    {
        return LIB_AES_ERR_PARAM;
    }

    GenerateTables();

    ctx->keyLen = keyLen;
    result = ExpandKey(ctx, key);
    if (result != LIB_AES_OK)
    {
        ctx->rounds = 0u;
    }
    return result;
}

int Lib_AesEncryptBlock(const Lib_AesContextType* ctx, const uint8_t in[LIB_AES_BLOCK_SIZE],
                        uint8_t out[LIB_AES_BLOCK_SIZE])
{
    uint8_t state[LIB_AES_BLOCK_SIZE];
    uint8_t round;

    if ((ctx == NULL) || (in == NULL) || (out == NULL))
    {
        return LIB_AES_ERR_PARAM;
    }
    if (ctx->rounds == 0u)
    {
        return LIB_AES_ERR_STATE;
    }

    (void)memcpy(state, in, LIB_AES_BLOCK_SIZE);

    AddRoundKey(state, ctx->roundKeys, 0u);

    for (round = 1u; round < ctx->rounds; round++)
    {
        SubBytes(state);
        ShiftRows(state);
        MixColumns(state);
        AddRoundKey(state, ctx->roundKeys, round);
    }

    SubBytes(state);
    ShiftRows(state);
    AddRoundKey(state, ctx->roundKeys, ctx->rounds);

    (void)memcpy(out, state, LIB_AES_BLOCK_SIZE);
    return LIB_AES_OK;
}

int Lib_AesDecryptBlock(const Lib_AesContextType* ctx, const uint8_t in[LIB_AES_BLOCK_SIZE],
                        uint8_t out[LIB_AES_BLOCK_SIZE])
{
    uint8_t state[LIB_AES_BLOCK_SIZE];
    uint8_t round;

    if ((ctx == NULL) || (in == NULL) || (out == NULL))
    {
        return LIB_AES_ERR_PARAM;
    }
    if (ctx->rounds == 0u)
    {
        return LIB_AES_ERR_STATE;
    }

    (void)memcpy(state, in, LIB_AES_BLOCK_SIZE);

    AddRoundKey(state, ctx->roundKeys, ctx->rounds);

    for (round = (uint8_t)(ctx->rounds - 1u); round > 0u; round--)
    {
        InvShiftRows(state);
        InvSubBytes(state);
        AddRoundKey(state, ctx->roundKeys, round);
        InvMixColumns(state);
    }

    InvShiftRows(state);
    InvSubBytes(state);
    AddRoundKey(state, ctx->roundKeys, 0u);

    (void)memcpy(out, state, LIB_AES_BLOCK_SIZE);
    return LIB_AES_OK;
}

int Lib_AesEncryptEcb(const Lib_AesContextType* ctx, const uint8_t* in,
                      uint8_t* out, size_t len)
{
    size_t offset;

    if ((ctx == NULL) || (in == NULL) || (out == NULL))
    {
        return LIB_AES_ERR_PARAM;
    }
    if (ctx->rounds == 0u)
    {
        return LIB_AES_ERR_STATE;
    }
    if ((len % LIB_AES_BLOCK_SIZE) != 0u)
    {
        return LIB_AES_ERR_LENGTH;
    }

    for (offset = 0u; offset < len; offset += LIB_AES_BLOCK_SIZE)
    {
        int rc = Lib_AesEncryptBlock(ctx, &in[offset], &out[offset]);
        if (rc != LIB_AES_OK)
        {
            return rc;
        }
    }
    return LIB_AES_OK;
}

int Lib_AesDecryptEcb(const Lib_AesContextType* ctx, const uint8_t* in,
                      uint8_t* out, size_t len)
{
    size_t offset;

    if ((ctx == NULL) || (in == NULL) || (out == NULL))
    {
        return LIB_AES_ERR_PARAM;
    }
    if (ctx->rounds == 0u)
    {
        return LIB_AES_ERR_STATE;
    }
    if ((len % LIB_AES_BLOCK_SIZE) != 0u)
    {
        return LIB_AES_ERR_LENGTH;
    }

    for (offset = 0u; offset < len; offset += LIB_AES_BLOCK_SIZE)
    {
        int rc = Lib_AesDecryptBlock(ctx, &in[offset], &out[offset]);
        if (rc != LIB_AES_OK)
        {
            return rc;
        }
    }
    return LIB_AES_OK;
}

int Lib_AesEncryptCbc(const Lib_AesContextType* ctx, const uint8_t iv[LIB_AES_BLOCK_SIZE],
                      const uint8_t* in, uint8_t* out, size_t len)
{
    uint8_t prev[LIB_AES_BLOCK_SIZE];
    uint8_t block[LIB_AES_BLOCK_SIZE];
    size_t offset;

    if ((ctx == NULL) || (iv == NULL) || (in == NULL) || (out == NULL))
    {
        return LIB_AES_ERR_PARAM;
    }
    if (ctx->rounds == 0u)
    {
        return LIB_AES_ERR_STATE;
    }
    if ((len % LIB_AES_BLOCK_SIZE) != 0u)
    {
        return LIB_AES_ERR_LENGTH;
    }

    (void)memcpy(prev, iv, LIB_AES_BLOCK_SIZE);

    for (offset = 0u; offset < len; offset += LIB_AES_BLOCK_SIZE)
    {
        uint8_t j;

        for (j = 0u; j < LIB_AES_BLOCK_SIZE; j++)
        {
            block[j] = (uint8_t)(in[offset + j] ^ prev[j]);
        }
        (void)Lib_AesEncryptBlock(ctx, block, &out[offset]);
        (void)memcpy(prev, &out[offset], LIB_AES_BLOCK_SIZE);
    }
    return LIB_AES_OK;
}

int Lib_AesDecryptCbc(const Lib_AesContextType* ctx, const uint8_t iv[LIB_AES_BLOCK_SIZE],
                      const uint8_t* in, uint8_t* out, size_t len)
{
    uint8_t prev[LIB_AES_BLOCK_SIZE];
    uint8_t block[LIB_AES_BLOCK_SIZE];
    size_t offset;

    if ((ctx == NULL) || (iv == NULL) || (in == NULL) || (out == NULL))
    {
        return LIB_AES_ERR_PARAM;
    }
    if (ctx->rounds == 0u)
    {
        return LIB_AES_ERR_STATE;
    }
    if ((len % LIB_AES_BLOCK_SIZE) != 0u)
    {
        return LIB_AES_ERR_LENGTH;
    }

    (void)memcpy(prev, iv, LIB_AES_BLOCK_SIZE);

    for (offset = 0u; offset < len; offset += LIB_AES_BLOCK_SIZE)
    {
        uint8_t j;

        (void)memcpy(block, &in[offset], LIB_AES_BLOCK_SIZE);
        (void)Lib_AesDecryptBlock(ctx, &in[offset], &out[offset]);
        for (j = 0u; j < LIB_AES_BLOCK_SIZE; j++)
        {
            out[offset + j] = (uint8_t)(out[offset + j] ^ prev[j]);
        }
        (void)memcpy(prev, block, LIB_AES_BLOCK_SIZE);
    }
    return LIB_AES_OK;
}
