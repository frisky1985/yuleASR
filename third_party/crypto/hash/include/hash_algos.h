/**********************************************************************************************************************
 * @file       hash_algos.h
 * @brief      SHA Hash Algorithms Header File
 *
 * 功能: 提供SHA-1和SHA-2家族哈希算法的统一API接口
 * SHA-1:   160位哈希 (FIPS 180-4, 兼容旧系统)
 * SHA-224: 224位哈希 (FIPS 180-4)
 * SHA-256: 256位哈希 (FIPS 180-4)
 * SHA-384: 384位哈希 (FIPS 180-4)
 * SHA-512: 512位哈希 (FIPS 180-4)
 * SHA-512/224: SHA-512的224位截断版
 * SHA-512/256: SHA-512的256位截断版
 *
 * 特性:
 * - 纯C语言实现，符合FIPS 180-4标准
 * - 支持逐块更新 (streaming)
 * - 线程安全的状态管理
 * - 支持大端/小端平台
 * - AUTOSAR集成支持
 *
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *********************************************************************************************************************/

#ifndef HASH_ALGOS_H
#define HASH_ALGOS_H

/**********************************************************************************************************************
 * VERSION INFORMATION
 *********************************************************************************************************************/
#define HASH_ALGOS_VENDOR_ID                    0x2025U  /* YuleTech */
#define HASH_ALGOS_MODULE_ID                    0xF2U    /* Hash Algorithms Module */
#define HASH_ALGOS_SW_MAJOR_VERSION             1U
#define HASH_ALGOS_SW_MINOR_VERSION             0U
#define HASH_ALGOS_SW_PATCH_VERSION             0U

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include <stddef.h>
#include <stdint.h>
#include "Std_Types.h"

/* 128-bit integer support: always held as two 64-bit halves.
 * sha384.c/sha512.c access ctx->length.lo/.hi directly; keeping the struct
 * form on all targets avoids divergence between 32-bit and 64-bit hosts. */
typedef struct {
    unsigned long long lo;
    unsigned long long hi;
} uint128;

/**********************************************************************************************************************
 * CONSTANT MACROS
 *********************************************************************************************************************/

/* Algorithm IDs */
#define HASH_ALGO_SHA1                          0x01U
#define HASH_ALGO_SHA224                        0x02U
#define HASH_ALGO_SHA256                        0x03U
#define HASH_ALGO_SHA384                        0x04U
#define HASH_ALGO_SHA512                        0x05U
#define HASH_ALGO_SHA512_224                    0x06U
#define HASH_ALGO_SHA512_256                    0x07U

/* Digest Sizes (in bytes) */
#define HASH_SHA1_DIGEST_SIZE                   20U     /* 160 bits */
#define HASH_SHA224_DIGEST_SIZE                 28U     /* 224 bits */
#define HASH_SHA256_DIGEST_SIZE                 32U     /* 256 bits */
#define HASH_SHA384_DIGEST_SIZE                 48U     /* 384 bits */
#define HASH_SHA512_DIGEST_SIZE                 64U     /* 512 bits */
#define HASH_SHA512_224_DIGEST_SIZE             28U     /* 224 bits */
#define HASH_SHA512_256_DIGEST_SIZE             32U     /* 256 bits */

/* Block Sizes (in bytes) */
#define HASH_SHA1_BLOCK_SIZE                    64U     /* 512 bits */
#define HASH_SHA224_BLOCK_SIZE                  64U     /* 512 bits */
#define HASH_SHA256_BLOCK_SIZE                  64U     /* 512 bits */
#define HASH_SHA384_BLOCK_SIZE                  128U    /* 1024 bits */
#define HASH_SHA512_BLOCK_SIZE                  128U    /* 1024 bits */

/* Maximum digest size */
#define HASH_MAX_DIGEST_SIZE                    HASH_SHA512_DIGEST_SIZE
#define HASH_MAX_BLOCK_SIZE                     HASH_SHA512_BLOCK_SIZE

/* Error Codes */
#define HASH_ERR_NONE                           0x00U
#define HASH_ERR_INVALID_PARAM                  0x01U
#define HASH_ERR_NULL_POINTER                   0x02U
#define HASH_ERR_INVALID_ALGO                   0x03U
#define HASH_ERR_BUFFER_TOO_SMALL               0x04U
#define HASH_ERR_STATE_ERROR                    0x05U
#define HASH_ERR_NOT_INITIALIZED                0x06U
#define HASH_ERR_OVERFLOW                       0x07U

/**********************************************************************************************************************
 * TYPE DEFINITIONS
 *********************************************************************************************************************/

/* Return type */
typedef uint8 Hash_ReturnType;

/* Algorithm type */
typedef uint8 Hash_AlgorithmType;

/* SHA-1 State Structure (160-bit) */
typedef struct {
    uint32 h[5];                        /* Hash state */
    uint32 w[80];                       /* Message schedule */
    uint64 length;                      /* Total length in bits */
    uint8  buffer[HASH_SHA1_BLOCK_SIZE];/* Input buffer */
    uint32 buflen;                      /* Buffer fill level */
    uint8  finalized;                   /* Finalization flag */
} sha1_state_t;

/* SHA-256 State Structure (224/256-bit) */
typedef struct {
    uint32 h[8];                        /* Hash state */
    uint32 w[64];                       /* Message schedule */
    uint64 length;                      /* Total length in bits */
    uint8  buffer[HASH_SHA256_BLOCK_SIZE];  /* Input buffer */
    uint32 buflen;                      /* Buffer fill level */
    uint8  finalized;                   /* Finalization flag */
} sha256_state_t;

/* SHA-512 State Structure (384/512-bit) */
typedef struct {
    uint64 h[8];                        /* Hash state */
    uint64 w[80];                       /* Message schedule */
    uint128 length;                     /* Total length in bits (using struct) */
    uint8  buffer[HASH_SHA512_BLOCK_SIZE];  /* Input buffer */
    uint32 buflen;                      /* Buffer fill level */
    uint8  finalized;                   /* Finalization flag */
    uint32 digest_size;                 /* Output digest size (for SHA-512/t) */
} sha512_state_t;

/* Generic Hash Context */
typedef struct {
    uint8               algo;           /* Algorithm ID */
    union {
        sha1_state_t    sha1;           /* SHA-1 state */
        sha256_state_t  sha256;         /* SHA-224/256 state */
        sha512_state_t  sha512;         /* SHA-384/512 state */
    } state;
} hash_context_t;

/* Hash Result Structure */
typedef struct {
    uint8  digest[HASH_MAX_DIGEST_SIZE];/* Hash output */
    uint32 digest_size;                 /* Actual digest size */
    Hash_ReturnType status;             /* Operation status */
} hash_result_t;

/**********************************************************************************************************************
 * FUNCTION PROTOTYPES - SHA-1
 *********************************************************************************************************************/

/**
 * @brief Compute SHA-1 hash in one shot
 * @param data Input data
 * @param len Input data length
 * @param digest Output buffer (must be at least 20 bytes)
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha1_compute(const uint8* data, uint32 len, uint8* digest);

/**
 * @brief Initialize SHA-1 context
 * @param ctx Context to initialize
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha1_init(sha1_state_t* ctx);

/**
 * @brief Update SHA-1 hash with data
 * @param ctx SHA-1 context
 * @param data Input data
 * @param len Input data length
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha1_update(sha1_state_t* ctx, const uint8* data, uint32 len);

/**
 * @brief Finalize SHA-1 hash computation
 * @param ctx SHA-1 context
 * @param digest Output buffer (at least 20 bytes)
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha1_final(sha1_state_t* ctx, uint8* digest);

/**********************************************************************************************************************
 * FUNCTION PROTOTYPES - SHA-224
 *********************************************************************************************************************/

/**
 * @brief Compute SHA-224 hash in one shot
 * @param data Input data
 * @param len Input data length
 * @param digest Output buffer (must be at least 28 bytes)
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha224_compute(const uint8* data, uint32 len, uint8* digest);

/**
 * @brief Initialize SHA-224 context
 * @param ctx Context to initialize
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha224_init(sha256_state_t* ctx);

/**
 * @brief Update SHA-224 hash with data
 * @param ctx SHA-224 context
 * @param data Input data
 * @param len Input data length
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha224_update(sha256_state_t* ctx, const uint8* data, uint32 len);

/**
 * @brief Finalize SHA-224 hash computation
 * @param ctx SHA-224 context
 * @param digest Output buffer (at least 28 bytes)
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha224_final(sha256_state_t* ctx, uint8* digest);

/**********************************************************************************************************************
 * FUNCTION PROTOTYPES - SHA-256
 *********************************************************************************************************************/

/**
 * @brief Compute SHA-256 hash in one shot
 * @param data Input data
 * @param len Input data length
 * @param digest Output buffer (must be at least 32 bytes)
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha256_compute(const uint8* data, uint32 len, uint8* digest);

/**
 * @brief Initialize SHA-256 context
 * @param ctx Context to initialize
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha256_init(sha256_state_t* ctx);

/**
 * @brief Update SHA-256 hash with data
 * @param ctx SHA-256 context
 * @param data Input data
 * @param len Input data length
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha256_update(sha256_state_t* ctx, const uint8* data, uint32 len);

/**
 * @brief Finalize SHA-256 hash computation
 * @param ctx SHA-256 context
 * @param digest Output buffer (at least 32 bytes)
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha256_final(sha256_state_t* ctx, uint8* digest);

/**********************************************************************************************************************
 * FUNCTION PROTOTYPES - SHA-384
 *********************************************************************************************************************/

/**
 * @brief Compute SHA-384 hash in one shot
 * @param data Input data
 * @param len Input data length
 * @param digest Output buffer (must be at least 48 bytes)
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha384_compute(const uint8* data, uint32 len, uint8* digest);

/**
 * @brief Initialize SHA-384 context
 * @param ctx Context to initialize
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha384_init(sha512_state_t* ctx);

/**
 * @brief Update SHA-384 hash with data
 * @param ctx SHA-384 context
 * @param data Input data
 * @param len Input data length
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha384_update(sha512_state_t* ctx, const uint8* data, uint32 len);

/**
 * @brief Finalize SHA-384 hash computation
 * @param ctx SHA-384 context
 * @param digest Output buffer (at least 48 bytes)
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha384_final(sha512_state_t* ctx, uint8* digest);

/**********************************************************************************************************************
 * FUNCTION PROTOTYPES - SHA-512
 *********************************************************************************************************************/

/**
 * @brief Compute SHA-512 hash in one shot
 * @param data Input data
 * @param len Input data length
 * @param digest Output buffer (must be at least 64 bytes)
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha512_compute(const uint8* data, uint32 len, uint8* digest);

/**
 * @brief Initialize SHA-512 context
 * @param ctx Context to initialize
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha512_init(sha512_state_t* ctx);

/**
 * @brief Update SHA-512 hash with data
 * @param ctx SHA-512 context
 * @param data Input data
 * @param len Input data length
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha512_update(sha512_state_t* ctx, const uint8* data, uint32 len);

/**
 * @brief Finalize SHA-512 hash computation
 * @param ctx SHA-512 context
 * @param digest Output buffer (at least 64 bytes)
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha512_final(sha512_state_t* ctx, uint8* digest);

/**********************************************************************************************************************
 * FUNCTION PROTOTYPES - SHA-512/224
 *********************************************************************************************************************/

/**
 * @brief Compute SHA-512/224 hash in one shot
 * @param data Input data
 * @param len Input data length
 * @param digest Output buffer (must be at least 28 bytes)
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha512_224_compute(const uint8* data, uint32 len, uint8* digest);

/**
 * @brief Initialize SHA-512/224 context
 * @param ctx Context to initialize
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha512_224_init(sha512_state_t* ctx);

/**
 * @brief Update SHA-512/224 hash with data
 * @param ctx SHA-512/224 context
 * @param data Input data
 * @param len Input data length
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha512_224_update(sha512_state_t* ctx, const uint8* data, uint32 len);

/**
 * @brief Finalize SHA-512/224 hash computation
 * @param ctx SHA-512/224 context
 * @param digest Output buffer (at least 28 bytes)
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha512_224_final(sha512_state_t* ctx, uint8* digest);

/**********************************************************************************************************************
 * FUNCTION PROTOTYPES - SHA-512/256
 *********************************************************************************************************************/

/**
 * @brief Compute SHA-512/256 hash in one shot
 * @param data Input data
 * @param len Input data length
 * @param digest Output buffer (must be at least 32 bytes)
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha512_256_compute(const uint8* data, uint32 len, uint8* digest);

/**
 * @brief Initialize SHA-512/256 context
 * @param ctx Context to initialize
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha512_256_init(sha512_state_t* ctx);

/**
 * @brief Update SHA-512/256 hash with data
 * @param ctx SHA-512/256 context
 * @param data Input data
 * @param len Input data length
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha512_256_update(sha512_state_t* ctx, const uint8* data, uint32 len);

/**
 * @brief Finalize SHA-512/256 hash computation
 * @param ctx SHA-512/256 context
 * @param digest Output buffer (at least 32 bytes)
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType sha512_256_final(sha512_state_t* ctx, uint8* digest);

/**********************************************************************************************************************
 * FUNCTION PROTOTYPES - Generic Interface
 *********************************************************************************************************************/

/**
 * @brief Compute hash using specified algorithm (one-shot)
 * @param algo Algorithm ID (HASH_ALGO_*)
 * @param data Input data
 * @param len Input data length
 * @param digest Output buffer
 * @param digest_len Pointer to digest length (input: buffer size, output: actual length)
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType hash_compute(Hash_AlgorithmType algo,
                             const uint8* data,
                             uint32 len,
                             uint8* digest,
                             uint32* digest_len);

/**
 * @brief Initialize hash context for specified algorithm
 * @param ctx Hash context
 * @param algo Algorithm ID
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType hash_init(hash_context_t* ctx, Hash_AlgorithmType algo);

/**
 * @brief Update hash context with data
 * @param ctx Hash context
 * @param data Input data
 * @param len Input data length
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType hash_update(hash_context_t* ctx, const uint8* data, uint32 len);

/**
 * @brief Finalize hash computation
 * @param ctx Hash context
 * @param digest Output buffer
 * @param digest_len Pointer to digest length
 * @return HASH_ERR_NONE on success, error code otherwise
 */
Hash_ReturnType hash_final(hash_context_t* ctx, uint8* digest, uint32* digest_len);

/**
 * @brief Get digest size for specified algorithm
 * @param algo Algorithm ID
 * @return Digest size in bytes, 0 if invalid
 */
uint32 hash_get_digest_size(Hash_AlgorithmType algo);

/**
 * @brief Get block size for specified algorithm
 * @param algo Algorithm ID
 * @return Block size in bytes, 0 if invalid
 */
uint32 hash_get_block_size(Hash_AlgorithmType algo);

/**
 * @brief Get algorithm name string
 * @param algo Algorithm ID
 * @return Algorithm name string, or "UNKNOWN"
 */
const char* hash_get_algorithm_name(Hash_AlgorithmType algo);

/**********************************************************************************************************************
 * INLINE HELPER FUNCTIONS
 *********************************************************************************************************************/

/* Load 32-bit big-endian */
static inline uint32 hash_load32_be(const uint8 src[4])
{
    return ((uint32)src[0] << 24) |
           ((uint32)src[1] << 16) |
           ((uint32)src[2] << 8)  |
           ((uint32)src[3]);
}

/* Store 32-bit big-endian */
static inline void hash_store32_be(uint8 dst[4], uint32 val)
{
    dst[0] = (uint8)(val >> 24);
    dst[1] = (uint8)(val >> 16);
    dst[2] = (uint8)(val >> 8);
    dst[3] = (uint8)(val);
}

/* Load 64-bit big-endian */
static inline uint64 hash_load64_be(const uint8 src[8])
{
    return ((uint64)src[0] << 56) |
           ((uint64)src[1] << 48) |
           ((uint64)src[2] << 40) |
           ((uint64)src[3] << 32) |
           ((uint64)src[4] << 24) |
           ((uint64)src[5] << 16) |
           ((uint64)src[6] << 8)  |
           ((uint64)src[7]);
}

/* Store 64-bit big-endian */
static inline void hash_store64_be(uint8 dst[8], uint64 val)
{
    dst[0] = (uint8)(val >> 56);
    dst[1] = (uint8)(val >> 48);
    dst[2] = (uint8)(val >> 40);
    dst[3] = (uint8)(val >> 32);
    dst[4] = (uint8)(val >> 24);
    dst[5] = (uint8)(val >> 16);
    dst[6] = (uint8)(val >> 8);
    dst[7] = (uint8)(val);
}

/* Rotate right for 32-bit */
static inline uint32 hash_rotr32(uint32 x, uint32 n)
{
    return (x >> n) | (x << (32 - n));
}

/* Rotate right for 64-bit */
static inline uint64 hash_rotr64(uint64 x, uint64 n)
{
    return (x >> n) | (x << (64 - n));
}

/* SHA-256 Ch function */
static inline uint32 hash_ch32(uint32 x, uint32 y, uint32 z)
{
    return (x & y) ^ (~x & z);
}

/* SHA-256 Maj function */
static inline uint32 hash_maj32(uint32 x, uint32 y, uint32 z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}

/* SHA-256 Sigma0 function */
static inline uint32 hash_sigma0_32(uint32 x)
{
    return hash_rotr32(x, 2) ^ hash_rotr32(x, 13) ^ hash_rotr32(x, 22);
}

/* SHA-256 Sigma1 function */
static inline uint32 hash_sigma1_32(uint32 x)
{
    return hash_rotr32(x, 6) ^ hash_rotr32(x, 11) ^ hash_rotr32(x, 25);
}

/* SHA-256 gamma0 function */
static inline uint32 hash_gamma0_32(uint32 x)
{
    return hash_rotr32(x, 7) ^ hash_rotr32(x, 18) ^ (x >> 3);
}

/* SHA-256 gamma1 function */
static inline uint32 hash_gamma1_32(uint32 x)
{
    return hash_rotr32(x, 17) ^ hash_rotr32(x, 19) ^ (x >> 10);
}

/* SHA-512 Ch function */
static inline uint64 hash_ch64(uint64 x, uint64 y, uint64 z)
{
    return (x & y) ^ (~x & z);
}

/* SHA-512 Maj function */
static inline uint64 hash_maj64(uint64 x, uint64 y, uint64 z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}

/* SHA-512 Sigma0 function */
static inline uint64 hash_sigma0_64(uint64 x)
{
    return hash_rotr64(x, 28) ^ hash_rotr64(x, 34) ^ hash_rotr64(x, 39);
}

/* SHA-512 Sigma1 function */
static inline uint64 hash_sigma1_64(uint64 x)
{
    return hash_rotr64(x, 14) ^ hash_rotr64(x, 18) ^ hash_rotr64(x, 41);
}

/* SHA-512 gamma0 function */
static inline uint64 hash_gamma0_64(uint64 x)
{
    return hash_rotr64(x, 1) ^ hash_rotr64(x, 8) ^ (x >> 7);
}

/* SHA-512 gamma1 function */
static inline uint64 hash_gamma1_64(uint64 x)
{
    return hash_rotr64(x, 19) ^ hash_rotr64(x, 61) ^ (x >> 6);
}

/* SHA-1 F1 function (0 <= t <= 19) */
static inline uint32 sha1_f1(uint32 b, uint32 c, uint32 d)
{
    return (b & c) | (~b & d);
}

/* SHA-1 F2 function (20 <= t <= 39) */
static inline uint32 sha1_f2(uint32 b, uint32 c, uint32 d)
{
    return b ^ c ^ d;
}

/* SHA-1 F3 function (40 <= t <= 59) */
static inline uint32 sha1_f3(uint32 b, uint32 c, uint32 d)
{
    return (b & c) | (b & d) | (c & d);
}

/* SHA-1 F4 function (60 <= t <= 79) */
static inline uint32 sha1_f4(uint32 b, uint32 c, uint32 d)
{
    return b ^ c ^ d;
}

/* SHA-1 circular left shift */
static inline uint32 sha1_rotl(uint32 x, uint32 n)
{
    return (x << n) | (x >> (32 - n));
}

#endif /* HASH_ALGOS_H */


/*==================================================================================================
 *                                     SHA3 TYPES
==================================================================================================*/

typedef struct {
    uint64_t state[25];        /* 1600-bit Keccak state (5x5 lanes of 64 bits) */
    uint8_t buffer[144];       /* Maximum rate buffer (SHA3-224: 1152 bits) */
    uint32_t bufLen;           /* Current buffer length */
} SHA3_224_Context;

typedef struct {
    uint64_t state[25];
    uint8_t buffer[136];       /* SHA3-256: 1088-bit rate */
    uint32_t bufLen;
} SHA3_256_Context;

typedef struct {
    uint64_t state[25];
    uint8_t buffer[104];       /* SHA3-384: 832-bit rate */
    uint32_t bufLen;
} SHA3_384_Context;

typedef struct {
    uint64_t state[25];
    uint8_t buffer[72];        /* SHA3-512: 576-bit rate */
    uint32_t bufLen;
} SHA3_512_Context;

/*==================================================================================================
 *                                     SHA3 API DECLARATIONS
==================================================================================================*/

/* SHA3-224 */
extern void SHA3_224_Init(SHA3_224_Context* ctx);
extern void SHA3_224_Update(SHA3_224_Context* ctx, const uint8_t* data, uint32_t len);
extern void SHA3_224_Final(SHA3_224_Context* ctx, uint8_t digest[28]);
extern void SHA3_224(const uint8_t* data, uint32_t len, uint8_t digest[28]);

/* SHA3-256 */
extern void SHA3_256_Init(SHA3_256_Context* ctx);
extern void SHA3_256_Update(SHA3_256_Context* ctx, const uint8_t* data, uint32_t len);
extern void SHA3_256_Final(SHA3_256_Context* ctx, uint8_t digest[32]);
extern void SHA3_256(const uint8_t* data, uint32_t len, uint8_t digest[32]);

/* SHA3-384 */
extern void SHA3_384_Init(SHA3_384_Context* ctx);
extern void SHA3_384_Update(SHA3_384_Context* ctx, const uint8_t* data, uint32_t len);
extern void SHA3_384_Final(SHA3_384_Context* ctx, uint8_t digest[48]);
extern void SHA3_384(const uint8_t* data, uint32_t len, uint8_t digest[48]);

/* SHA3-512 */
extern void SHA3_512_Init(SHA3_512_Context* ctx);
extern void SHA3_512_Update(SHA3_512_Context* ctx, const uint8_t* data, uint32_t len);
extern void SHA3_512_Final(SHA3_512_Context* ctx, uint8_t digest[64]);
extern void SHA3_512(const uint8_t* data, uint32_t len, uint8_t digest[64]);
