/**********************************************************************************************************************
 * @file       sha512.c
 * @brief      SHA-512 Hash Algorithm Implementation
 *
 * 功能: 实现SHA-512哈希算法 (FIPS 180-4)
 * 包括: SHA-512, SHA-512/224, SHA-512/256
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

/* SHA-512 Initial hash values (H0-H7) */
static const uint64 sha512_initial_h[8] = {
    0x6a09e667f3bcc908ULL,
    0xbb67ae8584caa73bULL,
    0x3c6ef372fe94f82bULL,
    0xa54ff53a5f1d36f1ULL,
    0x510e527fade682d1ULL,
    0x9b05688c2b3e6c1fULL,
    0x1f83d9abfb41bd6bULL,
    0x5be0cd19137e2179ULL
};

/* SHA-512/224 Initial hash values (FIPS 180-4 Section 5.3.6) */
/* These are computed as: SHA512_IV XOR 0xA5A5A5A5A5A5A5A5 */
static const uint64 sha512_224_initial_h[8] = {
    0x8C3D37C819544DA2ULL,
    0x73E1996689DCD4D6ULL,
    0x1DFAB7AE32FF9C82ULL,
    0x679DD514582F9FCFULL,
    0x0F6D2B697BD44DA8ULL,
    0x77E36F7304C48942ULL,
    0x3F9D85A86A1D36C8ULL,
    0x1112E6AD91D692A1ULL
};

/* SHA-512/256 Initial hash values (FIPS 180-4 Section 5.3.6) */
/* These are computed as: SHA512_IV XOR 0x5A5A5A5A5A5A5A5A */
static const uint64 sha512_256_initial_h[8] = {
    0x22312194FC2BF72CULL,
    0x9F555FA3C84C64C2ULL,
    0x2393B86B6F53B151ULL,
    0x963877195940EABDULL,
    0x96283EE2A88EFFE3ULL,
    0xBE5E1E2553863992ULL,
    0x2B0199FC2C85B8AAULL,
    0x0EB72DDC81C52CA2ULL
};

/* SHA-512 Round constants (K0-K79) */
static const uint64 sha512_k[80] = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL,
    0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL,
    0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL, 0x12835b0145706fbeULL,
    0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL,
    0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL,
    0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL,
    0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL,
    0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
    0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL,
    0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL,
    0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL,
    0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL, 0xd69906245565a910ULL,
    0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL,
    0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL,
    0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL,
    0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL,
    0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
    0xca273eceea26619cULL, 0xd186b8c721c0c207ULL,
    0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL,
    0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
    0x28db77f523047d84ULL, 0x32caab7b40c72493ULL,
    0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL,
    0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};

/**********************************************************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 *********************************************************************************************************************/
static void sha512_process_block(sha512_state_t* ctx, const uint8* block);
static void sha512_pad_message(sha512_state_t* ctx);

/**********************************************************************************************************************
 * FUNCTION DEFINITIONS
 *********************************************************************************************************************/

/**
 * @brief Process a 1024-bit block
 */
static void sha512_process_block(sha512_state_t* ctx, const uint8* block)
{
    uint64 a, b, c, d, e, f, g, h;
    uint64 T1, T2;
    uint64 w[80];
    uint32 t;

    /* Prepare message schedule W[0..15] */
    for (t = 0; t < 16; t++) {
        w[t] = hash_load64_be(&block[t * 8]);
    }

    /* Extend to W[16..79] */
    for (t = 16; t < 80; t++) {
        w[t] = hash_gamma1_64(w[t-2]) + w[t-7] +
               hash_gamma0_64(w[t-15]) + w[t-16];
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

    /* Main loop - 80 rounds */
    for (t = 0; t < 80; t++) {
        T1 = h + hash_sigma1_64(e) + hash_ch64(e, f, g) + sha512_k[t] + w[t];
        T2 = hash_sigma0_64(a) + hash_maj64(a, b, c);
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
static void sha512_pad_message(sha512_state_t* ctx)
{
    uint32 i;
    uint32 pad_len;
    uint64 msg_bit_len_lo;
    uint64 msg_bit_len_hi;
    uint8 padding[HASH_SHA512_BLOCK_SIZE * 2];

    /* Calculate message length (128-bit representation) */
    msg_bit_len_lo = ctx->length.lo;
    msg_bit_len_hi = ctx->length.hi;

    /* Calculate padding length */
    i = (uint32)(ctx->length.lo >> 3) % HASH_SHA512_BLOCK_SIZE;
    
    /* Need: 1 byte (0x80) + 0-111 bytes (zeros) + 16 bytes (length) = 128 bytes total */
    pad_len = (i < 112) ? (112 - i) : (240 - i);

    /* Start with 0x80 */
    padding[0] = 0x80;
    
    /* Fill rest with zeros */
    for (i = 1; i < pad_len; i++) {
        padding[i] = 0x00;
    }

    /* Append original message length as 128-bit big-endian */
    hash_store64_be(&padding[pad_len], msg_bit_len_hi);
    hash_store64_be(&padding[pad_len + 8], msg_bit_len_lo);
    pad_len += 16;

    /* Process the padding block(s) */
    sha512_process_block(ctx, padding);
    
    /* If we needed 2 blocks (i >= 112), process the second block */
    if (pad_len > 128) {
        sha512_process_block(ctx, &padding[128]);
    }
}

/**********************************************************************************************************************
 * SHA-512 SPECIFIC FUNCTIONS
 *********************************************************************************************************************/

/**
 * @brief Compute SHA-512 hash in one shot
 */
Hash_ReturnType sha512_compute(const uint8* data, uint32 len, uint8* digest)
{
    sha512_state_t ctx;
    Hash_ReturnType ret;

    if (digest == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    ret = sha512_init(&ctx);
    if (ret != HASH_ERR_NONE) {
        return ret;
    }

    if (data != NULL && len > 0) {
        ret = sha512_update(&ctx, data, len);
        if (ret != HASH_ERR_NONE) {
            return ret;
        }
    }

    return sha512_final(&ctx, digest);
}

/**
 * @brief Initialize SHA-512 context
 */
Hash_ReturnType sha512_init(sha512_state_t* ctx)
{
    if (ctx == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    /* Initialize hash values with SHA-512 constants */
    (void)memcpy(ctx->h, sha512_initial_h, sizeof(sha512_initial_h));
    
    /* Clear other fields */
    (void)memset(ctx->w, 0, sizeof(ctx->w));
    ctx->length.lo = 0;
    ctx->length.hi = 0;
    ctx->buflen = 0;
    ctx->finalized = 0;
    ctx->digest_size = HASH_SHA512_DIGEST_SIZE;

    return HASH_ERR_NONE;
}

/**
 * @brief Update SHA-512 hash with data
 */
Hash_ReturnType sha512_update(sha512_state_t* ctx, const uint8* data, uint32 len)
{
    uint32 fill;
    uint64 old_len_lo;

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
    old_len_lo = ctx->length.lo;
    ctx->length.lo += ((uint64)len << 3);
    
    /* Handle carry to high word */
    if (ctx->length.lo < old_len_lo) {
        ctx->length.hi++;
    }
    
    /* Check for overflow of high word (message too long) */
    if (ctx->length.hi < ((old_len_lo + ((uint64)len << 3)) >> 63)) {
        return HASH_ERR_OVERFLOW;
    }

    /* Fill the buffer if there's leftover data */
    if (ctx->buflen > 0) {
        fill = HASH_SHA512_BLOCK_SIZE - ctx->buflen;
        
        if (len < fill) {
            (void)memcpy(&ctx->buffer[ctx->buflen], data, len);
            ctx->buflen += len;
            return HASH_ERR_NONE;
        }

        (void)memcpy(&ctx->buffer[ctx->buflen], data, fill);
        sha512_process_block(ctx, ctx->buffer);
        ctx->buflen = 0;
        
        data += fill;
        len -= fill;
    }

    /* Process full blocks */
    while (len >= HASH_SHA512_BLOCK_SIZE) {
        sha512_process_block(ctx, data);
        data += HASH_SHA512_BLOCK_SIZE;
        len -= HASH_SHA512_BLOCK_SIZE;
    }

    /* Save remaining data */
    if (len > 0) {
        (void)memcpy(ctx->buffer, data, len);
        ctx->buflen = len;
    }

    return HASH_ERR_NONE;
}

/**
 * @brief Finalize SHA-512 hash computation
 */
Hash_ReturnType sha512_final(sha512_state_t* ctx, uint8* digest)
{
    uint32 i;

    if (ctx == NULL || digest == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    if (ctx->finalized) {
        /* Already finalized, output all 64 bytes */
        for (i = 0; i < 8; i++) {
            hash_store64_be(&digest[i * 8], ctx->h[i]);
        }
        return HASH_ERR_NONE;
    }

    /* Pad and process final block(s) */
    sha512_pad_message(ctx);

    /* Output the hash (all 64 bytes) */
    for (i = 0; i < 8; i++) {
        hash_store64_be(&digest[i * 8], ctx->h[i]);
    }

    /* Mark as finalized */
    ctx->finalized = 1;

    /* Clear sensitive data */
    (void)memset(ctx->w, 0, sizeof(ctx->w));
    (void)memset(ctx->buffer, 0, sizeof(ctx->buffer));

    return HASH_ERR_NONE;
}

/**********************************************************************************************************************
 * SHA-512/224 SPECIFIC FUNCTIONS
 *********************************************************************************************************************/

/**
 * @brief Compute SHA-512/224 hash in one shot
 */
Hash_ReturnType sha512_224_compute(const uint8* data, uint32 len, uint8* digest)
{
    sha512_state_t ctx;
    Hash_ReturnType ret;

    if (digest == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    ret = sha512_224_init(&ctx);
    if (ret != HASH_ERR_NONE) {
        return ret;
    }

    if (data != NULL && len > 0) {
        ret = sha512_224_update(&ctx, data, len);
        if (ret != HASH_ERR_NONE) {
            return ret;
        }
    }

    return sha512_224_final(&ctx, digest);
}

/**
 * @brief Initialize SHA-512/224 context
 */
Hash_ReturnType sha512_224_init(sha512_state_t* ctx)
{
    if (ctx == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    /* Initialize hash values with SHA-512/224 constants */
    (void)memcpy(ctx->h, sha512_224_initial_h, sizeof(sha512_224_initial_h));
    
    /* Clear other fields */
    (void)memset(ctx->w, 0, sizeof(ctx->w));
    ctx->length.lo = 0;
    ctx->length.hi = 0;
    ctx->buflen = 0;
    ctx->finalized = 0;
    ctx->digest_size = HASH_SHA512_224_DIGEST_SIZE;

    return HASH_ERR_NONE;
}

/**
 * @brief Update SHA-512/224 hash with data
 */
Hash_ReturnType sha512_224_update(sha512_state_t* ctx, const uint8* data, uint32 len)
{
    /* SHA-512/224 uses the same update logic as SHA-512 */
    return sha512_update(ctx, data, len);
}

/**
 * @brief Finalize SHA-512/224 hash computation
 */
Hash_ReturnType sha512_224_final(sha512_state_t* ctx, uint8* digest)
{
    uint32 i;

    if (ctx == NULL || digest == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    if (ctx->finalized) {
        /* Already finalized, output first 28 bytes */
        for (i = 0; i < 3; i++) {
            hash_store64_be(&digest[i * 8], ctx->h[i]);
        }
        /* Last 4 bytes from h[3] (truncated) */
        hash_store32_be(&digest[24], (uint32)(ctx->h[3] >> 32));
        return HASH_ERR_NONE;
    }

    /* Pad and process final block(s) */
    sha512_pad_message(ctx);

    /* Output the hash (first 28 bytes) */
    for (i = 0; i < 3; i++) {
        hash_store64_be(&digest[i * 8], ctx->h[i]);
    }
    /* Last 4 bytes from h[3] (truncated) */
    hash_store32_be(&digest[24], (uint32)(ctx->h[3] >> 32));

    /* Mark as finalized */
    ctx->finalized = 1;

    /* Clear sensitive data */
    (void)memset(ctx->w, 0, sizeof(ctx->w));
    (void)memset(ctx->buffer, 0, sizeof(ctx->buffer));

    return HASH_ERR_NONE;
}

/**********************************************************************************************************************
 * SHA-512/256 SPECIFIC FUNCTIONS
 *********************************************************************************************************************/

/**
 * @brief Compute SHA-512/256 hash in one shot
 */
Hash_ReturnType sha512_256_compute(const uint8* data, uint32 len, uint8* digest)
{
    sha512_state_t ctx;
    Hash_ReturnType ret;

    if (digest == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    ret = sha512_256_init(&ctx);
    if (ret != HASH_ERR_NONE) {
        return ret;
    }

    if (data != NULL && len > 0) {
        ret = sha512_256_update(&ctx, data, len);
        if (ret != HASH_ERR_NONE) {
            return ret;
        }
    }

    return sha512_256_final(&ctx, digest);
}

/**
 * @brief Initialize SHA-512/256 context
 */
Hash_ReturnType sha512_256_init(sha512_state_t* ctx)
{
    if (ctx == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    /* Initialize hash values with SHA-512/256 constants */
    (void)memcpy(ctx->h, sha512_256_initial_h, sizeof(sha512_256_initial_h));
    
    /* Clear other fields */
    (void)memset(ctx->w, 0, sizeof(ctx->w));
    ctx->length.lo = 0;
    ctx->length.hi = 0;
    ctx->buflen = 0;
    ctx->finalized = 0;
    ctx->digest_size = HASH_SHA512_256_DIGEST_SIZE;

    return HASH_ERR_NONE;
}

/**
 * @brief Update SHA-512/256 hash with data
 */
Hash_ReturnType sha512_256_update(sha512_state_t* ctx, const uint8* data, uint32 len)
{
    /* SHA-512/256 uses the same update logic as SHA-512 */
    return sha512_update(ctx, data, len);
}

/**
 * @brief Finalize SHA-512/256 hash computation
 */
Hash_ReturnType sha512_256_final(sha512_state_t* ctx, uint8* digest)
{
    uint32 i;

    if (ctx == NULL || digest == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    if (ctx->finalized) {
        /* Already finalized, output first 32 bytes */
        for (i = 0; i < 4; i++) {
            hash_store64_be(&digest[i * 8], ctx->h[i]);
        }
        return HASH_ERR_NONE;
    }

    /* Pad and process final block(s) */
    sha512_pad_message(ctx);

    /* Output the hash (first 32 bytes) */
    for (i = 0; i < 4; i++) {
        hash_store64_be(&digest[i * 8], ctx->h[i]);
    }

    /* Mark as finalized */
    ctx->finalized = 1;

    /* Clear sensitive data */
    (void)memset(ctx->w, 0, sizeof(ctx->w));
    (void)memset(ctx->buffer, 0, sizeof(ctx->buffer));

    return HASH_ERR_NONE;
}
