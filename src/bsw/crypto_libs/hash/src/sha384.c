/**********************************************************************************************************************
 * @file       sha384.c
 * @brief      SHA-384 Hash Algorithm Implementation
 *
 * 功能: 实现SHA-384哈希算法 (FIPS 180-4)
 * SHA-384与SHA-512共享相同的核心逻辑，但使用不同的初始值和截断输出
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

/* SHA-384 Initial hash values (H0-H7) */
static const uint64 sha384_initial_h[8] = {
    0xCBBB9D5DC1059ED8ULL,
    0x629A292A367CD507ULL,
    0x9159015A3070DD17ULL,
    0x152FECD8F70E5939ULL,
    0x67332667FFC00B31ULL,
    0x8EB44A8768581511ULL,
    0xDB0C2E0D64F98FA7ULL,
    0x47B5481DBEFA4FA4ULL
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
 * @brief Process a 1024-bit block (shared between SHA-384 and SHA-512)
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
    uint64 msg_bit_len_hi, msg_bit_len_lo;
    uint8 padding[HASH_SHA512_BLOCK_SIZE * 2];

    /* Calculate message length (128-bit representation) */
    msg_bit_len_lo = (uint64)(ctx->length.lo);
    msg_bit_len_hi = (uint64)(ctx->length.hi);

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
 * SHA-384 SPECIFIC FUNCTIONS
 *********************************************************************************************************************/

/**
 * @brief Compute SHA-384 hash in one shot
 */
Hash_ReturnType sha384_compute(const uint8* data, uint32 len, uint8* digest)
{
    sha512_state_t ctx;
    Hash_ReturnType ret;

    if (digest == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    ret = sha384_init(&ctx);
    if (ret != HASH_ERR_NONE) {
        return ret;
    }

    if (data != NULL && len > 0) {
        ret = sha384_update(&ctx, data, len);
        if (ret != HASH_ERR_NONE) {
            return ret;
        }
    }

    return sha384_final(&ctx, digest);
}

/**
 * @brief Initialize SHA-384 context
 */
Hash_ReturnType sha384_init(sha512_state_t* ctx)
{
    if (ctx == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    /* Initialize hash values with SHA-384 constants */
    (void)memcpy(ctx->h, sha384_initial_h, sizeof(sha384_initial_h));
    
    /* Clear other fields */
    (void)memset(ctx->w, 0, sizeof(ctx->w));
    ctx->length.lo = 0;
    ctx->length.hi = 0;
    ctx->buflen = 0;
    ctx->finalized = 0;
    ctx->digest_size = HASH_SHA384_DIGEST_SIZE;

    return HASH_ERR_NONE;
}

/**
 * @brief Update SHA-384 hash with data
 */
Hash_ReturnType sha384_update(sha512_state_t* ctx, const uint8* data, uint32 len)
{
    uint32 i;
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
 * @brief Finalize SHA-384 hash computation
 */
Hash_ReturnType sha384_final(sha512_state_t* ctx, uint8* digest)
{
    uint32 i;

    if (ctx == NULL || digest == NULL) {
        return HASH_ERR_NULL_POINTER;
    }

    if (ctx->finalized) {
        /* Already finalized, output first 48 bytes */
        for (i = 0; i < 6; i++) {
            hash_store64_be(&digest[i * 8], ctx->h[i]);
        }
        return HASH_ERR_NONE;
    }

    /* Pad and process final block(s) */
    sha512_pad_message(ctx);

    /* Output the hash (first 48 bytes, truncate last 16 bytes) */
    for (i = 0; i < 6; i++) {
        hash_store64_be(&digest[i * 8], ctx->h[i]);
    }

    /* Mark as finalized */
    ctx->finalized = 1;

    /* Clear sensitive data */
    (void)memset(ctx->w, 0, sizeof(ctx->w));
    (void)memset(ctx->buffer, 0, sizeof(ctx->buffer));

    return HASH_ERR_NONE;
}
