/**********************************************************************************************************************
 * @file       sha224.c
 * @brief      SHA-224 Hash Algorithm Implementation
 *
 * 功能: 实现SHA-224哈希算法 (FIPS 180-4)
 * SHA-224与SHA-256共享相同的核心逻辑，但使用不同的初始值和截断输出
 *
 * 特性:
 * - 纯C语言实现
 * - 符合FIPS 180-4标准
 * - 支持逐块更新 (streaming)
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

/* SHA-224 Initial hash values (H0-H7) */
static const uint32 sha224_initial_h[8] = {
    0xC1059ED8U,
    0x367CD507U,
    0x3070DD17U,
    0xF70E5939U,
    0xFFC00B31U,
    0x68581511U,
    0x64F98FA7U,
    0xBEFA4FA4U
};

/* SHA-256 Round constants (K0-K63) */
static const uint32 sha256_k[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
};

/**********************************************************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 *********************************************************************************************************************/
static void sha256_process_block(sha256_state_t* ctx, const uint8* block);
static void sha256_pad_message(sha256_state_t* ctx);

/**********************************************************************************************************************
 * FUNCTION DEFINITIONS
 *********************************************************************************************************************/

/**
 * @brief Process a 512-bit block (shared between SHA-224 and SHA-256)
 */
static void sha256_process_block(sha256_state_t* ctx, const uint8* block)
{
    uint32 a, b, c, d, e, f, g, h;
    uint32 t, T1, T2;

    /* Prepare message schedule W[0..15] */
    for (t = 0; t < 16; t++) {
        ctx->w[t] = hash_load32_be(&block[t * 4]);
    }

    /* Extend to W[16..63] */
    for (t = 16; t < 64; t++) {
        ctx->w[t] = hash_gamma1_32(ctx->w[t-2]) + ctx->w[t-7] +
                    hash_gamma0_32(ctx->w[t-15]) + ctx->w[t-16];
    }

    /* Initialize working variables */
    a = ctx->h[0];
    b = ctx->h[1];
    c = ctx->h[2];
    d = ctx->h[3];
    e = ctx->h[4];
    f = ctx->h[5];
    g = ctx->h[6];
    h = ctx->h[7];

    /* Main loop - 64 rounds */
    for (t = 0; t < 64; t++) {
        T1 = h + hash_sigma1_32(e) + hash_ch32(e, f, g) + sha256_k[t] + ctx->w[t];
        T2 = hash_sigma0_32(a) + hash_maj32(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    /* Add to current hash value */
    ctx->h[0] += a;
    ctx->h[1] += b;
    ctx->h[2] += c;
    ctx->h[3] += d;
    ctx->h[4] += e;
    ctx->h[5] += f;
    ctx->h[6] += g;
    ctx->h[7] += h;
}

/**
 * @brief Pad message according to FIPS 180-4
 */
static void sha256_pad_message(sha256_state_t* ctx)
{
    uint32 i;
    uint32 pad_len;
    uint64 msg_bit_len;
    uint8 padding[HASH_SHA256_BLOCK_SIZE * 2];

    /* Calculate message length in bits */
    msg_bit_len = ctx->length;

    /* Calculate padding length */
    i = (uint32)(ctx->length >> 3) % HASH_SHA256_BLOCK_SIZE;
    
    /* Need: 1 byte (0x80) + 0-55 bytes (zeros) + 8 bytes (length) = 64 bytes total */
    pad_len = (i < 56) ? (56 - i) : (120 - i);

    /* Start with 0x80 */
    padding[0] = 0x80;
    
    /* Fill rest with zeros */
    for (i = 1; i < pad_len; i++) {
        padding[i] = 0x00;
    }

    /* Append original message length as 64-bit big-endian */
    hash_store64_be(&padding[pad_len], msg_bit_len);
    pad_len += 8;

    /* Process the padding block(s) */
    sha256_process_block(ctx, padding);
    
    /* If we needed 2 blocks (i >= 56), process the second block */
    if (pad_len > 64) {
        sha256_process_block(ctx, &padding[64]);
    }
}

/**********************************************************************************************************************
 * SHA-224 SPECIFIC FUNCTIONS
 *********************************************************************************************************************/

/**
 * @brief Compute SHA-224 hash in one shot
 */
Hash_ReturnType sha224_compute(const uint8* data, uint32 len, uint8* digest)
{
    sha256_state_t ctx;
    Hash_ReturnType ret;

    if (digest == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    ret = sha224_init(&ctx);
    if (ret != HASH_ERR_NONE) {
        return ret;
    }

    if (data != NULL && len > 0) {
        ret = sha224_update(&ctx, data, len);
        if (ret != HASH_ERR_NONE) {
            return ret;
        }
    }

    return sha224_final(&ctx, digest);
}

/**
 * @brief Initialize SHA-224 context
 */
Hash_ReturnType sha224_init(sha256_state_t* ctx)
{
    if (ctx == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    /* Initialize hash values with SHA-224 constants */
    (void)memcpy(ctx->h, sha224_initial_h, sizeof(sha224_initial_h));
    
    /* Clear other fields */
    (void)memset(ctx->w, 0, sizeof(ctx->w));
    ctx->length = 0;
    ctx->buflen = 0;
    ctx->finalized = 0;

    return HASH_ERR_NONE;
}

/**
 * @brief Update SHA-224 hash with data
 */
Hash_ReturnType sha224_update(sha256_state_t* ctx, const uint8* data, uint32 len)
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

    /* Check for overflow */
    if (ctx->length < ((uint64)len << 3)) {
        return HASH_ERR_OVERFLOW;
    }

    /* Fill the buffer if there's leftover data */
    if (ctx->buflen > 0) {
        fill = HASH_SHA256_BLOCK_SIZE - ctx->buflen;
        
        if (len < fill) {
            (void)memcpy(&ctx->buffer[ctx->buflen], data, len);
            ctx->buflen += len;
            return HASH_ERR_NONE;
        }

        (void)memcpy(&ctx->buffer[ctx->buflen], data, fill);
        sha256_process_block(ctx, ctx->buffer);
        ctx->buflen = 0;
        
        data += fill;
        len -= fill;
    }

    /* Process full blocks */
    while (len >= HASH_SHA256_BLOCK_SIZE) {
        sha256_process_block(ctx, data);
        data += HASH_SHA256_BLOCK_SIZE;
        len -= HASH_SHA256_BLOCK_SIZE;
    }

    /* Save remaining data */
    if (len > 0) {
        (void)memcpy(ctx->buffer, data, len);
        ctx->buflen = len;
    }

    return HASH_ERR_NONE;
}

/**
 * @brief Finalize SHA-224 hash computation
 */
Hash_ReturnType sha224_final(sha256_state_t* ctx, uint8* digest)
{
    if (ctx == NULL || digest == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    if (ctx->finalized) {
        /* Already finalized, just output the digest (first 28 bytes) */
        hash_store32_be(&digest[0], ctx->h[0]);
        hash_store32_be(&digest[4], ctx->h[1]);
        hash_store32_be(&digest[8], ctx->h[2]);
        hash_store32_be(&digest[12], ctx->h[3]);
        hash_store32_be(&digest[16], ctx->h[4]);
        hash_store32_be(&digest[20], ctx->h[5]);
        hash_store32_be(&digest[24], ctx->h[6]);
        return HASH_ERR_NONE;
    }

    /* Pad and process final block(s) */
    sha256_pad_message(ctx);

    /* Output the hash (first 28 bytes, truncate last 4 bytes) */
    hash_store32_be(&digest[0], ctx->h[0]);
    hash_store32_be(&digest[4], ctx->h[1]);
    hash_store32_be(&digest[8], ctx->h[2]);
    hash_store32_be(&digest[12], ctx->h[3]);
    hash_store32_be(&digest[16], ctx->h[4]);
    hash_store32_be(&digest[20], ctx->h[5]);
    hash_store32_be(&digest[24], ctx->h[6]);

    /* Mark as finalized */
    ctx->finalized = 1;

    /* Clear sensitive data */
    (void)memset(ctx->w, 0, sizeof(ctx->w));
    (void)memset(ctx->buffer, 0, sizeof(ctx->buffer));

    return HASH_ERR_NONE;
}

/**********************************************************************************************************************
 * SHA-256 SPECIFIC FUNCTIONS
 *********************************************************************************************************************/

/* SHA-256 Initial hash values (H0-H7) */
static const uint32 sha256_initial_h[8] = {
    0x6a09e667U,
    0xbb67ae85U,
    0x3c6ef372U,
    0xa54ff53aU,
    0x510e527fU,
    0x9b05688cU,
    0x1f83d9abU,
    0x5be0cd19U
};

/**
 * @brief Compute SHA-256 hash in one shot
 */
Hash_ReturnType sha256_compute(const uint8* data, uint32 len, uint8* digest)
{
    sha256_state_t ctx;
    Hash_ReturnType ret;

    if (digest == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    ret = sha256_init(&ctx);
    if (ret != HASH_ERR_NONE) {
        return ret;
    }

    if (data != NULL && len > 0) {
        ret = sha256_update(&ctx, data, len);
        if (ret != HASH_ERR_NONE) {
            return ret;
        }
    }

    return sha256_final(&ctx, digest);
}

/**
 * @brief Initialize SHA-256 context
 */
Hash_ReturnType sha256_init(sha256_state_t* ctx)
{
    if (ctx == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    /* Initialize hash values with SHA-256 constants */
    (void)memcpy(ctx->h, sha256_initial_h, sizeof(sha256_initial_h));
    
    /* Clear other fields */
    (void)memset(ctx->w, 0, sizeof(ctx->w));
    ctx->length = 0;
    ctx->buflen = 0;
    ctx->finalized = 0;

    return HASH_ERR_NONE;
}

/**
 * @brief Update SHA-256 hash with data
 */
Hash_ReturnType sha256_update(sha256_state_t* ctx, const uint8* data, uint32 len)
{
    /* SHA-256 uses the same update logic as SHA-224 */
    return sha224_update(ctx, data, len);
}

/**
 * @brief Finalize SHA-256 hash computation
 */
Hash_ReturnType sha256_final(sha256_state_t* ctx, uint8* digest)
{
    uint32 i;

    if (ctx == NULL || digest == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    if (ctx->finalized) {
        /* Already finalized, output all 32 bytes */
        for (i = 0; i < 8; i++) {
            hash_store32_be(&digest[i * 4], ctx->h[i]);
        }
        return HASH_ERR_NONE;
    }

    /* Pad and process final block(s) */
    sha256_pad_message(ctx);

    /* Output the hash (all 32 bytes) */
    for (i = 0; i < 8; i++) {
        hash_store32_be(&digest[i * 4], ctx->h[i]);
    }

    /* Mark as finalized */
    ctx->finalized = 1;

    /* Clear sensitive data */
    (void)memset(ctx->w, 0, sizeof(ctx->w));
    (void)memset(ctx->buffer, 0, sizeof(ctx->buffer));

    return HASH_ERR_NONE;
}
