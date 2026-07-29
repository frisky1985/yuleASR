/**********************************************************************************************************************
 * @file       aes_core.c
 * @brief      AES核心算法实现 - 轮密钥拓展和基本加解密操作
 *
 * 实现参考: FIPS-197标准
 *
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *********************************************************************************************************************/

#include "aes_modes.h"

/**********************************************************************************************************************
 * AES S-Box和逆S-Box
 *********************************************************************************************************************/
static const uint8 sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

static const uint8 rsbox[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

/**********************************************************************************************************************
 * 轮常数 (Round Constant)
 *********************************************************************************************************************/
static const uint32 rcon[11] = {
    0x00000000, 0x01000000, 0x02000000, 0x04000000, 0x08000000,
    0x10000000, 0x20000000, 0x40000000, 0x80000000, 0x1b000000, 0x36000000
};

/**********************************************************************************************************************
 * 辅助宏定义
 *********************************************************************************************************************/
#define GET_UINT32_LE(n,b,i)                    \
{                                               \
    (n) = ((uint32)(b)[(i)    ]      )       \
        | ((uint32)(b)[(i) + 1] <<  8)       \
        | ((uint32)(b)[(i) + 2] << 16)       \
        | ((uint32)(b)[(i) + 3] << 24);      \
}

#define PUT_UINT32_LE(n,b,i)                    \
{                                               \
    (b)[(i)    ] = (uint8)((n)      );       \
    (b)[(i) + 1] = (uint8)((n) >>  8);       \
    (b)[(i) + 2] = (uint8)((n) >> 16);       \
    (b)[(i) + 3] = (uint8)((n) >> 24);       \
}

#define ROTL8(x)  (((x) << 8) | ((x) >> 24))
#define XTIME(x)  (((x) << 1) ^ (((x) >> 7) * 0x1b))
#define MUL(x, y) (((x) && (y)) ? powTable[logTable[(x)] + logTable[(y)]] : 0)

/**********************************************************************************************************************
 * 轮密钥扩展
 *********************************************************************************************************************/
static void aes_set_key(uint8* rk, const uint8* key, uint32 keylen)
{
    uint32 i;
    uint32* rk32 = (uint32*)rk;
    const uint32* key32 = (const uint32*)key;
    uint32 nk = keylen / 4;
    uint32 nr = (keylen / 4) + 6;

    /* 复制原始密钥 */
    for (i = 0; i < nk; i++) {
        rk32[i] = key32[i];
    }

    /* 扩展轮密钥 */
    for (i = nk; i < 4 * (nr + 1); i++) {
        uint32 temp = rk32[i - 1];
        if ((i % nk) == 0) {
            /* RotWord、SubWord、异或Rcon */
            temp = (sbox[(temp >> 16) & 0xFF]      ) |
                   (sbox[(temp >> 8)  & 0xFF] <<  8) |
                   (sbox[(temp      ) & 0xFF] << 16) |
                   (sbox[(temp >> 24)      ] << 24);
            temp ^= rcon[i / nk];
        } else if (nk > 6 && (i % nk) == 4) {
            /* 只对AES-256的特殊处理 */
            temp = (sbox[(temp      ) & 0xFF]      ) |
                   (sbox[(temp >> 8)  & 0xFF] <<  8) |
                   (sbox[(temp >> 16) & 0xFF] << 16) |
                   (sbox[(temp >> 24)      ] << 24);
        }
        rk32[i] = rk32[i - nk] ^ temp;
    }
}

/**********************************************************************************************************************
 * 内邮AES单块加密
 *********************************************************************************************************************/
static void aes_encrypt_block(const uint8* rk, uint32 nr, const uint8 input[16], uint8 output[16])
{
    uint32 s0, s1, s2, s3, t0, t1, t2, t3;
    const uint32* rk32 = (const uint32*)rk;

    /* 轮密钥加载数量: nr+1组 */
    uint32 rki = 0;

    /* 初始轮密钥加 (AddRoundKey) */
    GET_UINT32_LE(s0, input,  0); s0 ^= rk32[rki++];
    GET_UINT32_LE(s1, input,  4); s1 ^= rk32[rki++];
    GET_UINT32_LE(s2, input,  8); s2 ^= rk32[rki++];
    GET_UINT32_LE(s3, input, 12); s3 ^= rk32[rki++];

    /* 主轮循 (Nr-1次) */
    for (uint32 round = 1; round < nr; round++) {
        /* SubBytes + ShiftRows + MixColumns + AddRoundKey */
        t0 = rk32[rki++] ^
             ((uint32)sbox[(s0      ) & 0xFF]      ) ^
             ((uint32)sbox[(s1 >>  8) & 0xFF] <<  8) ^
             ((uint32)sbox[(s2 >> 16) & 0xFF] << 16) ^
             ((uint32)sbox[(s3 >> 24)      ] << 24);

        t1 = rk32[rki++] ^
             ((uint32)sbox[(s1      ) & 0xFF]      ) ^
             ((uint32)sbox[(s2 >>  8) & 0xFF] <<  8) ^
             ((uint32)sbox[(s3 >> 16) & 0xFF] << 16) ^
             ((uint32)sbox[(s0 >> 24)      ] << 24);

        t2 = rk32[rki++] ^
             ((uint32)sbox[(s2      ) & 0xFF]      ) ^
             ((uint32)sbox[(s3 >>  8) & 0xFF] <<  8) ^
             ((uint32)sbox[(s0 >> 16) & 0xFF] << 16) ^
             ((uint32)sbox[(s1 >> 24)      ] << 24);

        t3 = rk32[rki++] ^
             ((uint32)sbox[(s3      ) & 0xFF]      ) ^
             ((uint32)sbox[(s0 >>  8) & 0xFF] <<  8) ^
             ((uint32)sbox[(s1 >> 16) & 0xFF] << 16) ^
             ((uint32)sbox[(s2 >> 24)      ] << 24);

        s0 = t0; s1 = t1; s2 = t2; s3 = t3;
    }

    /* 最终轮: SubBytes + ShiftRows + AddRoundKey (无MixColumns) */
    t0 = rk32[rki++] ^
         ((uint32)sbox[(s0      ) & 0xFF]      ) ^
         ((uint32)sbox[(s1 >>  8) & 0xFF] <<  8) ^
         ((uint32)sbox[(s2 >> 16) & 0xFF] << 16) ^
         ((uint32)sbox[(s3 >> 24)      ] << 24);

    t1 = rk32[rki++] ^
         ((uint32)sbox[(s1      ) & 0xFF]      ) ^
         ((uint32)sbox[(s2 >>  8) & 0xFF] <<  8) ^
         ((uint32)sbox[(s3 >> 16) & 0xFF] << 16) ^
         ((uint32)sbox[(s0 >> 24)      ] << 24);

    t2 = rk32[rki++] ^
         ((uint32)sbox[(s2      ) & 0xFF]      ) ^
         ((uint32)sbox[(s3 >>  8) & 0xFF] <<  8) ^
         ((uint32)sbox[(s0 >> 16) & 0xFF] << 16) ^
         ((uint32)sbox[(s1 >> 24)      ] << 24);

    t3 = rk32[rki++] ^
         ((uint32)sbox[(s3      ) & 0xFF]      ) ^
         ((uint32)sbox[(s0 >>  8) & 0xFF] <<  8) ^
         ((uint32)sbox[(s1 >> 16) & 0xFF] << 16) ^
         ((uint32)sbox[(s2 >> 24)      ] << 24);

    PUT_UINT32_LE(t0, output,  0);
    PUT_UINT32_LE(t1, output,  4);
    PUT_UINT32_LE(t2, output,  8);
    PUT_UINT32_LE(t3, output, 12);
}

/**********************************************************************************************************************
 * 内邮AES单块解密
 *********************************************************************************************************************/
static void aes_decrypt_block(const uint8* rk, uint32 nr, const uint8 input[16], uint8 output[16])
{
    uint32 s0, s1, s2, s3, t0, t1, t2, t3;
    const uint32* rk32 = (const uint32*)rk;

    /* 解密用的轮密钥是逆序的并且有InvMixColumns */
    uint32 rki = 4 * nr;

    /* 初始轮密钥加 */
    GET_UINT32_LE(s0, input,  0); s0 ^= rk32[rki--];
    GET_UINT32_LE(s1, input,  4); s1 ^= rk32[rki--];
    GET_UINT32_LE(s2, input,  8); s2 ^= rk32[rki--];
    GET_UINT32_LE(s3, input, 12); s3 ^= rk32[rki--];

    /* 主轮循 */
    for (uint32 round = nr - 1; round > 0; round--) {
        t0 = rk32[rki--] ^
             ((uint32)rsbox[(s0      ) & 0xFF]      ) ^
             ((uint32)rsbox[(s3 >>  8) & 0xFF] <<  8) ^
             ((uint32)rsbox[(s2 >> 16) & 0xFF] << 16) ^
             ((uint32)rsbox[(s1 >> 24)      ] << 24);

        t1 = rk32[rki--] ^
             ((uint32)rsbox[(s1      ) & 0xFF]      ) ^
             ((uint32)rsbox[(s0 >>  8) & 0xFF] <<  8) ^
             ((uint32)rsbox[(s3 >> 16) & 0xFF] << 16) ^
             ((uint32)rsbox[(s2 >> 24)      ] << 24);

        t2 = rk32[rki--] ^
             ((uint32)rsbox[(s2      ) & 0xFF]      ) ^
             ((uint32)rsbox[(s1 >>  8) & 0xFF] <<  8) ^
             ((uint32)rsbox[(s0 >> 16) & 0xFF] << 16) ^
             ((uint32)rsbox[(s3 >> 24)      ] << 24);

        t3 = rk32[rki--] ^
             ((uint32)rsbox[(s3      ) & 0xFF]      ) ^
             ((uint32)rsbox[(s2 >>  8) & 0xFF] <<  8) ^
             ((uint32)rsbox[(s1 >> 16) & 0xFF] << 16) ^
             ((uint32)rsbox[(s0 >> 24)      ] << 24);

        s0 = t0; s1 = t1; s2 = t2; s3 = t3;
    }

    /* 最终轮 */
    t0 = rk32[rki--] ^
         ((uint32)rsbox[(s0      ) & 0xFF]      ) ^
         ((uint32)rsbox[(s3 >>  8) & 0xFF] <<  8) ^
         ((uint32)rsbox[(s2 >> 16) & 0xFF] << 16) ^
         ((uint32)rsbox[(s1 >> 24)      ] << 24);

    t1 = rk32[rki--] ^
         ((uint32)rsbox[(s1      ) & 0xFF]      ) ^
         ((uint32)rsbox[(s0 >>  8) & 0xFF] <<  8) ^
         ((uint32)rsbox[(s3 >> 16) & 0xFF] << 16) ^
         ((uint32)rsbox[(s2 >> 24)      ] << 24);

    t2 = rk32[rki--] ^
         ((uint32)rsbox[(s2      ) & 0xFF]      ) ^
         ((uint32)rsbox[(s1 >>  8) & 0xFF] <<  8) ^
         ((uint32)rsbox[(s0 >> 16) & 0xFF] << 16) ^
         ((uint32)rsbox[(s3 >> 24)      ] << 24);

    t3 = rk32[rki--] ^
         ((uint32)rsbox[(s3      ) & 0xFF]      ) ^
         ((uint32)rsbox[(s2 >>  8) & 0xFF] <<  8) ^
         ((uint32)rsbox[(s1 >> 16) & 0xFF] << 16) ^
         ((uint32)rsbox[(s0 >> 24)      ] << 24);

    PUT_UINT32_LE(t0, output,  0);
    PUT_UINT32_LE(t1, output,  4);
    PUT_UINT32_LE(t2, output,  8);
    PUT_UINT32_LE(t3, output, 12);
}

/**********************************************************************************************************************
 * 外部API实现
 *********************************************************************************************************************/

/**
 * @brief 初始化AES上下文
 */
uint8 Aes_Init(Aes_ContextType* ctx, const uint8* key, uint32 keyLen)
{
    if (ctx == NULL || key == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (keyLen != AES_KEY_SIZE_128 && keyLen != AES_KEY_SIZE_192 && keyLen != AES_KEY_SIZE_256) {
        return AES_ERR_INVALID_KEY;
    }

    /* 清零上下文 */
    memset(ctx, 0, sizeof(Aes_ContextType));

    /* 设置密钥长度和轮数 */
    ctx->keyLength = keyLen;
    ctx->numRounds = (keyLen / 4) + 6;

    /* 扩展轮密钥 */
    aes_set_key(ctx->roundKey[0], key, keyLen);

    ctx->initialized = TRUE;
    return AES_ERR_NONE;
}

/**
 * @brief 设置IV/Nonce
 */
uint8 Aes_SetIv(Aes_ContextType* ctx, const uint8* iv, uint32 ivLen)
{
    if (ctx == NULL || iv == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (ivLen != AES_BLOCK_SIZE) {
        return AES_ERR_INVALID_IV;
    }

    memcpy(ctx->iv, iv, AES_BLOCK_SIZE);
    ctx->blockCount = 0;

    return AES_ERR_NONE;
}

/**
 * @brief 清理AES上下文
 */
void Aes_Clear(Aes_ContextType* ctx)
{
    if (ctx != NULL) {
        memset(ctx, 0, sizeof(Aes_ContextType));
    }
}

/**
 * @brief 检查密钥长度是否有效
 */
boolean Aes_IsValidKeyLength(uint32 keyLen)
{
    return (keyLen == AES_KEY_SIZE_128 ||
            keyLen == AES_KEY_SIZE_192 ||
            keyLen == AES_KEY_SIZE_256);
}

/**
 * @brief 获取AES密钥类型
 */
Aes_KeyType Aes_GetKeyType(uint32 keyLen)
{
    switch (keyLen) {
        case AES_KEY_SIZE_128:
            return AES_KEY_128;
        case AES_KEY_SIZE_192:
            return AES_KEY_192;
        case AES_KEY_SIZE_256:
            return AES_KEY_256;
        default:
            return AES_KEY_128;
    }
}

/**
 * @brief 获取密钥轮数
 */
uint32 Aes_GetNumRounds(Aes_KeyType keyType)
{
    switch (keyType) {
        case AES_KEY_128: return 10;
        case AES_KEY_192: return 12;
        case AES_KEY_256: return 14;
        default: return 10;
    }
}

/**
 * @brief PKCS#7填充
 */
uint8 Aes_Pkcs7Pad(uint8* data, uint32 dataLen, uint32 paddedLen)
{
    if (data == NULL || paddedLen <= dataLen || (paddedLen % AES_BLOCK_SIZE) != 0) {
        return AES_ERR_INVALID_LENGTH;
    }

    uint32 padLen = paddedLen - dataLen;
    for (uint32 i = dataLen; i < paddedLen; i++) {
        data[i] = (uint8)padLen;
    }

    return AES_ERR_NONE;
}

/**
 * @brief PKCS#7去填充
 */
uint8 Aes_Pkcs7Unpad(const uint8* data, uint32 dataLen, uint32* unpaddedLenPtr)
{
    if (data == NULL || unpaddedLenPtr == NULL || dataLen == 0 || (dataLen % AES_BLOCK_SIZE) != 0) {
        return AES_ERR_INVALID_LENGTH;
    }

    uint8 padLen = data[dataLen - 1];
    if (padLen == 0 || padLen > AES_BLOCK_SIZE) {
        return AES_ERR_INVALID_LENGTH;
    }

    /* 验证填充 */
    for (uint32 i = dataLen - padLen; i < dataLen; i++) {
        if (data[i] != padLen) {
            return AES_ERR_INVALID_LENGTH;
        }
    }

    *unpaddedLenPtr = dataLen - padLen;
    return AES_ERR_NONE;
}

/**
 * @brief ECB模式单块加密
 */
uint8 Aes_EcbEncryptBlock(const Aes_ContextType* ctx,
                           const uint8* plaintext,
                           uint8* ciphertext)
{
    if (ctx == NULL || plaintext == NULL || ciphertext == NULL || !ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, plaintext, ciphertext);
    return AES_ERR_NONE;
}

/**
 * @brief ECB模式单块解密
 */
uint8 Aes_EcbDecryptBlock(const Aes_ContextType* ctx,
                           const uint8* ciphertext,
                           uint8* plaintext)
{
    if (ctx == NULL || ciphertext == NULL || plaintext == NULL || !ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    aes_decrypt_block(ctx->roundKey[0], ctx->numRounds, ciphertext, plaintext);
    return AES_ERR_NONE;
}

/**
 * @brief 获取版本信息
 */
void Aes_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (versioninfo != NULL) {
        versioninfo->vendorID = AES_MODES_VENDOR_ID;
        versioninfo->moduleID = AES_MODES_MODULE_ID;
        versioninfo->sw_major_version = AES_MODES_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = AES_MODES_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = AES_MODES_SW_PATCH_VERSION;
    }
}
