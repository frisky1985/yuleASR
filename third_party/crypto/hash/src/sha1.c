/**********************************************************************************************************************
 * @file       sha1.c
 * @brief      SHA-1 Hash Algorithm Implementation
 *
 * 功能: 实现SHA-1哈希算法 (FIPS 180-4)
 *
 * 特性:
 * - 纯C语言实现
 * - 符合FIPS 180-4标准
 * - 支持逐块更新 (streaming)
 * - 安全考虑: SHA-1已被评为不安全，仅用于兼容旧系统
 *
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "hash_algos.h"
#include <string.h>

/**********************************************************************************************************************
 * CONSTANT MACROS
 *********************************************************************************************************************/

/* SHA-1 Constants */
#define SHA1_K0     0x5A827999U  /* 0 <= t <= 19 */
#define SHA1_K1     0x6ED9EBA1U  /* 20 <= t <= 39 */
#define SHA1_K2     0x8F1BBCDCU  /* 40 <= t <= 59 */
#define SHA1_K3     0xCA62C1D6U  /* 60 <= t <= 79 */

/* Initial hash values (H0-H4) */
static const uint32 sha1_initial_h[5] = {
    0x67452301U,
    0xEFCDAB89U,
    0x98BADCFEU,
    0x10325476U,
    0xC3D2E1F0U
};

/**********************************************************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 *********************************************************************************************************************/
static void sha1_process_block(sha1_state_t* ctx, const uint8* block);
static void sha1_pad_message(sha1_state_t* ctx);

/**********************************************************************************************************************
 * FUNCTION DEFINITIONS
 *********************************************************************************************************************/

/**
 * @brief Process a 512-bit block
 */
static void sha1_process_block(sha1_state_t* ctx, const uint8* block)
{
    uint32 a, b, c, d, e;
    uint32 t, temp;
    uint32 w[80];

    /* Prepare message schedule W[0..15] from the block */
    for (t = 0; t < 16; t++) {
        w[t] = hash_load32_be(&block[t * 4]);
    }

    /* Extend to W[16..79] */
    for (t = 16; t < 80; t++) {
        w[t] = sha1_rotl(w[t-3] ^ w[t-8] ^ w[t-14] ^ w[t-16], 1);
    }

    /* Initialize working variables */
    a = ctx->h[0];
    b = ctx->h[1];
    c = ctx->h[2];
    d = ctx->h[3];
    e = ctx->h[4];

    /* Main loop - 80 rounds */
    for (t = 0; t < 80; t++) {
        if (t < 20) {
            temp = sha1_f1(b, c, d) + SHA1_K0;
        } else if (t < 40) {
            temp = sha1_f2(b, c, d) + SHA1_K1;
        } else if (t < 60) {
            temp = sha1_f3(b, c, d) + SHA1_K2;
        } else {
            temp = sha1_f4(b, c, d) + SHA1_K3;
        }
        
        temp += sha1_rotl(a, 5) + e + w[t];
        e = d;
        d = c;
        c = sha1_rotl(b, 30);
        b = a;
        a = temp;
    }

    /* Add to current hash value */
    ctx->h[0] += a;
    ctx->h[1] += b;
    ctx->h[2] += c;
    ctx->h[3] += d;
    ctx->h[4] += e;
}

/**
 * @brief Pad message according to FIPS 180-4
 */
static void sha1_pad_message(sha1_state_t* ctx)
{
    uint8 padding[HASH_SHA1_BLOCK_SIZE * 2];
    uint32 buflen = ctx->buflen;
    uint64 msg_bit_len = ctx->length;

    /* Zero entire padding (incl. second block) to keep uninitialized bytes out of the digest */
    (void)memset(padding, 0, sizeof(padding));

    /* FIPS 180-4: message tail + 0x80 + zeros + 64-bit big-endian length */
    if (buflen > 0U) {
        (void)memcpy(padding, ctx->buffer, buflen);
    }
    padding[buflen] = 0x80;

    if (buflen < 56U) {
        /* Single block: length written at 56..63 */
        hash_store64_be(&padding[56], msg_bit_len);
        sha1_process_block(ctx, padding);
    } else {
        /* Double block: length written at second block 120..127 */
        hash_store64_be(&padding[120], msg_bit_len);
        sha1_process_block(ctx, padding);
        sha1_process_block(ctx, &padding[64]);
    }
}

/**
 * @brief Compute SHA-1 hash in one shot
 */
Hash_ReturnType sha1_compute(const uint8* data, uint32 len, uint8* digest)
{
    sha1_state_t ctx;
    Hash_ReturnType ret;

    if (digest == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    ret = sha1_init(&ctx);
    if (ret != HASH_ERR_NONE) {
        return ret;
    }

    if (data != NULL && len > 0) {
        ret = sha1_update(&ctx, data, len);
        if (ret != HASH_ERR_NONE) {
            return ret;
        }
    }

    return sha1_final(&ctx, digest);
}

/**
 * @brief Initialize SHA-1 context
 */
Hash_ReturnType sha1_init(sha1_state_t* ctx)
{
    if (ctx == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    /* Initialize hash values */
    (void)memcpy(ctx->h, sha1_initial_h, sizeof(sha1_initial_h));
    
    /* Clear other fields */
    (void)memset(ctx->w, 0, sizeof(ctx->w));
    ctx->length = 0;
    ctx->buflen = 0;
    ctx->finalized = 0;

    return HASH_ERR_NONE;
}

/**
 * @brief Update SHA-1 hash with data
 */
Hash_ReturnType sha1_update(sha1_state_t* ctx, const uint8* data, uint32 len)
{
    uint32 i;
    uint32 fill;

    if (ctx == NULL || data == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    if (len == 0) {
        return HASH_ERR_NONE;
    }

    if (ctx->finalized) {
        return HASH_ERR_STATE_ERROR;
    }

    /* Update message length (in bits) */
    ctx->length += ((uint64)len << 3);

    /* Check for overflow (message too long) */
    if (ctx->length < ((uint64)len << 3)) {
        return HASH_ERR_OVERFLOW;
    }

    /* Fill the buffer if there's leftover data */
    if (ctx->buflen > 0) {
        fill = HASH_SHA1_BLOCK_SIZE - ctx->buflen;
        
        if (len < fill) {
            (void)memcpy(&ctx->buffer[ctx->buflen], data, len);
            ctx->buflen += len;
            return HASH_ERR_NONE;
        }

        (void)memcpy(&ctx->buffer[ctx->buflen], data, fill);
        sha1_process_block(ctx, ctx->buffer);
        ctx->buflen = 0;
        
        data += fill;
        len -= fill;
    }

    /* Process full blocks */
    while (len >= HASH_SHA1_BLOCK_SIZE) {
        sha1_process_block(ctx, data);
        data += HASH_SHA1_BLOCK_SIZE;
        len -= HASH_SHA1_BLOCK_SIZE;
    }

    /* Save remaining data */
    if (len > 0) {
        (void)memcpy(ctx->buffer, data, len);
        ctx->buflen = len;
    }

    return HASH_ERR_NONE;
}

/**
 * @brief Finalize SHA-1 hash computation
 */
Hash_ReturnType sha1_final(sha1_state_t* ctx, uint8* digest)
{
    if (ctx == NULL || digest == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    if (ctx->finalized) {
        /* Already finalized, just output the digest */
        hash_store32_be(&digest[0], ctx->h[0]);
        hash_store32_be(&digest[4], ctx->h[1]);
        hash_store32_be(&digest[8], ctx->h[2]);
        hash_store32_be(&digest[12], ctx->h[3]);
        hash_store32_be(&digest[16], ctx->h[4]);
        return HASH_ERR_NONE;
    }

    /* Pad and process final block(s) */
    sha1_pad_message(ctx);

    /* Output the hash (big-endian) */
    hash_store32_be(&digest[0], ctx->h[0]);
    hash_store32_be(&digest[4], ctx->h[1]);
    hash_store32_be(&digest[8], ctx->h[2]);
    hash_store32_be(&digest[12], ctx->h[3]);
    hash_store32_be(&digest[16], ctx->h[4]);

    /* Mark as finalized */
    ctx->finalized = 1;

    /* Clear sensitive data */
    (void)memset(ctx->w, 0, sizeof(ctx->w));
    (void)memset(ctx->buffer, 0, sizeof(ctx->buffer));

    return HASH_ERR_NONE;
}
