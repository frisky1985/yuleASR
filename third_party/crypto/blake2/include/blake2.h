/**********************************************************************************************************************
 * @file       blake2.h
 * @brief      BLAKE2 Hash Algorithm Header File
 *
 * 功能: 提供BLAKE2b和BLAKE2s哈希算法的API接口
 * BLAKE2b: 64位平台优化，最大512位输出
 * BLAKE2s: 32位平台优化，最大256位输出
 *
 * 特性:
 * - 符合RFC 7693标准
 * - 支持密钥化哈希(BLAKE2b/s)
 * - 支持可配置输出长度
 * - 比SHA-3更快，比MD5更安全
 *
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *********************************************************************************************************************/

#ifndef BLAKE2_H
#define BLAKE2_H

/**********************************************************************************************************************
 * VERSION INFORMATION
 *********************************************************************************************************************/
#define BLAKE2_VENDOR_ID                    0x2025U  /* YuleTech */
#define BLAKE2_MODULE_ID                    0xF1U    /* BLAKE2 Module */
#define BLAKE2_SW_MAJOR_VERSION             1U
#define BLAKE2_SW_MINOR_VERSION             0U
#define BLAKE2_SW_PATCH_VERSION             0U

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include <stddef.h>
#include "Std_Types.h"

/**********************************************************************************************************************
 * CONSTANT MACROS
 *********************************************************************************************************************/
/* BLAKE2b Constants (64-bit variant) */
#define BLAKE2B_BLOCKBYTES                  128U    /* 1024 bits */
#define BLAKE2B_KEYBYTES                    64U     /* 512 bits */
#define BLAKE2B_SALTBYTES                   16U     /* 128 bits */
#define BLAKE2B_PERSONALBYTES               16U     /* 128 bits */
#define BLAKE2B_OUTBYTES                    64U     /* 512 bits */

/* BLAKE2s Constants (32-bit variant) */
#define BLAKE2S_BLOCKBYTES                  64U     /* 512 bits */
#define BLAKE2S_KEYBYTES                    32U     /* 256 bits */
#define BLAKE2S_SALTBYTES                   8U      /* 64 bits */
#define BLAKE2S_PERSONALBYTES               8U      /* 64 bits */
#define BLAKE2S_OUTBYTES                    32U     /* 256 bits */

/* Algorithm Selection */
#define BLAKE2B_ID                          0x01U
#define BLAKE2S_ID                          0x02U
#define BLAKE2BP_ID                         0x03U   /* BLAKE2b parallel */
#define BLAKE2SP_ID                         0x04U   /* BLAKE2s parallel */

/* Error Codes */
#define BLAKE2_ERR_NONE                     0x00U
#define BLAKE2_ERR_INVALID_PARAM            0x01U
#define BLAKE2_ERR_INVALID_KEYLEN           0x02U
#define BLAKE2_ERR_INVALID_OUTLEN           0x03U
#define BLAKE2_ERR_NULL_POINTER             0x04U
#define BLAKE2_ERR_BUFFER_TOO_SMALL         0x05U
#define BLAKE2_ERR_STATE_ERROR              0x06U

/**********************************************************************************************************************
 * TYPE DEFINITIONS
 *********************************************************************************************************************/
/* BLAKE2b State Structure */
typedef struct {
    uint64 h[8];                        /* Chaining values */
    uint64 t[2];                        /* Message counter */
    uint64 f[2];                        /* Finalization flags */
    uint8  buf[BLAKE2B_BLOCKBYTES];     /* Buffer */
    uint32 buflen;                      /* Buffer length */
    uint8  outlen;                      /* Output length */
    uint8  last_node;                   /* Last node flag */
} blake2b_state_t;

/* BLAKE2s State Structure */
typedef struct {
    uint32 h[8];                        /* Chaining values */
    uint32 t[2];                        /* Message counter */
    uint32 f[2];                        /* Finalization flags */
    uint8  buf[BLAKE2S_BLOCKBYTES];     /* Buffer */
    uint32 buflen;                      /* Buffer length */
    uint8  outlen;                      /* Output length */
    uint8  last_node;                   /* Last node flag */
} blake2s_state_t;

/* BLAKE2b Parameter Block */
typedef struct {
    uint8  digest_length;               /* 1 byte */
    uint8  key_length;                  /* 1 byte */
    uint8  fanout;                      /* 1 byte */
    uint8  depth;                       /* 1 byte */
    uint32 leaf_length;                 /* 4 bytes */
    uint32 node_offset;                 /* 4 bytes */
    uint32 xof_length;                  /* 4 bytes */
    uint8  node_depth;                  /* 1 byte */
    uint8  inner_length;                /* 1 byte */
    uint8  reserved[14];                /* 14 bytes */
    uint8  salt[BLAKE2B_SALTBYTES];     /* 16 bytes */
    uint8  personal[BLAKE2B_PERSONALBYTES]; /* 16 bytes */
} blake2b_param_t;

/* BLAKE2s Parameter Block */
typedef struct {
    uint8  digest_length;               /* 1 byte */
    uint8  key_length;                  /* 1 byte */
    uint8  fanout;                      /* 1 byte */
    uint8  depth;                       /* 1 byte */
    uint16 leaf_length;                 /* 2 bytes */
    uint16 node_offset;                 /* 2 bytes */
    uint16 xof_length;                  /* 2 bytes */
    uint8  node_depth;                  /* 1 byte */
    uint8  inner_length;                /* 1 byte */
    uint8  reserved[14];                /* 14 bytes */
    uint8  salt[BLAKE2S_SALTBYTES];     /* 8 bytes */
    uint8  personal[BLAKE2S_PERSONALBYTES]; /* 8 bytes */
} blake2s_param_t;

/* Generic BLAKE2 State */
typedef union {
    blake2b_state_t b;                  /* BLAKE2b state */
    blake2s_state_t s;                  /* BLAKE2s state */
} blake2_state_t;

/* BLAKE2 Configuration */
typedef struct {
    uint8  digest_length;               /* Output length (1-64 for BLAKE2b, 1-32 for BLAKE2s) */
    uint8  key_length;                  /* Key length (0-64 for BLAKE2b, 0-32 for BLAKE2s) */
    const uint8* key;                   /* Key pointer (can be NULL) */
    const uint8* salt;                  /* Salt pointer (can be NULL) */
    const uint8* personal;              /* Personalization pointer (can be NULL) */
} blake2_config_t;

/* AUTOSAR-specific types */
typedef uint8 Blake2_ReturnType;
typedef uint8 Blake2_AlgorithmType;

/**********************************************************************************************************************
 * FUNCTION PROTOTYPES - BLAKE2b
 *********************************************************************************************************************/
/* Simple hashing */
Blake2_ReturnType blake2b(
    uint8* out,
    const uint8* in,
    uint32 inlen,
    const uint8* key,
    uint8 keylen,
    uint8 outlen
);

/* Incremental hashing */
Blake2_ReturnType blake2b_init(blake2b_state_t* S, uint8 outlen);
Blake2_ReturnType blake2b_init_key(blake2b_state_t* S, uint8 outlen, const uint8* key, uint8 keylen);
Blake2_ReturnType blake2b_init_param(blake2b_state_t* S, const blake2b_param_t* P);
Blake2_ReturnType blake2b_update(blake2b_state_t* S, const uint8* in, uint32 inlen);
Blake2_ReturnType blake2b_final(blake2b_state_t* S, uint8* out, uint8 outlen);

/**********************************************************************************************************************
 * FUNCTION PROTOTYPES - BLAKE2s
 *********************************************************************************************************************/
/* Simple hashing */
Blake2_ReturnType blake2s(
    uint8* out,
    const uint8* in,
    uint32 inlen,
    const uint8* key,
    uint8 keylen,
    uint8 outlen
);

/* Incremental hashing */
Blake2_ReturnType blake2s_init(blake2s_state_t* S, uint8 outlen);
Blake2_ReturnType blake2s_init_key(blake2s_state_t* S, uint8 outlen, const uint8* key, uint8 keylen);
Blake2_ReturnType blake2s_init_param(blake2s_state_t* S, const blake2s_param_t* P);
Blake2_ReturnType blake2s_update(blake2s_state_t* S, const uint8* in, uint32 inlen);
Blake2_ReturnType blake2s_final(blake2s_state_t* S, uint8* out, uint8 outlen);

/**********************************************************************************************************************
 * FUNCTION PROTOTYPES - Generic Interface
 *********************************************************************************************************************/
Blake2_ReturnType blake2(
    uint8* out,
    const uint8* in,
    uint32 inlen,
    const uint8* key,
    uint8 keylen,
    uint8 outlen,
    Blake2_AlgorithmType algo
);

Blake2_ReturnType blake2_init(
    blake2_state_t* S,
    uint8 outlen,
    Blake2_AlgorithmType algo
);

Blake2_ReturnType blake2_init_key(
    blake2_state_t* S,
    uint8 outlen,
    const uint8* key,
    uint8 keylen,
    Blake2_AlgorithmType algo
);

Blake2_ReturnType blake2_update(
    blake2_state_t* S,
    const uint8* in,
    uint32 inlen,
    Blake2_AlgorithmType algo
);

Blake2_ReturnType blake2_final(
    blake2_state_t* S,
    uint8* out,
    uint8 outlen,
    Blake2_AlgorithmType algo
);

/**********************************************************************************************************************
 * INLINE HELPER FUNCTIONS
 *********************************************************************************************************************/
/* Load/Store functions for 64-bit */
static inline uint64 blake2b_load64(const uint8 src[8])
{
    uint64 w = ((uint64)src[0] << 0)  |
               ((uint64)src[1] << 8)  |
               ((uint64)src[2] << 16) |
               ((uint64)src[3] << 24) |
               ((uint64)src[4] << 32) |
               ((uint64)src[5] << 40) |
               ((uint64)src[6] << 48) |
               ((uint64)src[7] << 56);
    return w;
}

static inline void blake2b_store64(uint8 dst[8], uint64 w)
{
    dst[0] = (uint8)(w >> 0);
    dst[1] = (uint8)(w >> 8);
    dst[2] = (uint8)(w >> 16);
    dst[3] = (uint8)(w >> 24);
    dst[4] = (uint8)(w >> 32);
    dst[5] = (uint8)(w >> 40);
    dst[6] = (uint8)(w >> 48);
    dst[7] = (uint8)(w >> 56);
}

/* Load/Store functions for 32-bit */
static inline uint32 blake2s_load32(const uint8 src[4])
{
    uint32 w = ((uint32)src[0] << 0)  |
               ((uint32)src[1] << 8)  |
               ((uint32)src[2] << 16) |
               ((uint32)src[3] << 24);
    return w;
}

static inline void blake2s_store32(uint8 dst[4], uint32 w)
{
    dst[0] = (uint8)(w >> 0);
    dst[1] = (uint8)(w >> 8);
    dst[2] = (uint8)(w >> 16);
    dst[3] = (uint8)(w >> 24);
}

#endif /* BLAKE2_H */
