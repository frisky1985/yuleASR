/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : mbedTLS (软件密码后端)
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/
/* @req SWS_Csm_00001 @req SWS_Csm_00002 @req SWS_Csm_00010 */


/**
 * @file Csm_Cfg.c
 * @brief CSM 配置层回调实现 (P0-B 修复: Csm_Cfg_HwService 无定义)
 *
 * 背景 (yuleASR 全量复核 2026-08-08 P0-B-①):
 *   Csm_Cfg.h 仅 extern 声明 Csm_Cfg_HwService/KeyWrite/KeyRead/RandomGenerate/GetTimestamp,
 *   全仓无定义 -> libservice_csm.a 含 `U _Csm_Cfg_HwService` 等未定义符号,
 *   任何固件链接 libservice_csm.a 即失败。
 *
 * 修复方案 (真实实现, 禁止 return OK 假实现):
 *   本文件定义全部 5 个配置层回调, 密码操作接入 mbedTLS 软件后端:
 *     - CSM_SERVICE_HASH        -> mbedtls_sha256 (SHA-256, 32B)   [真实]
 *     - CSM_SERVICE_MAC_GENERATE-> HMAC-SHA256 (RFC2104, mbedtls_sha256 原语) [真实]
 *     - CSM_SERVICE_ENCRYPT/DECRYPT -> AES-128-CBC + PKCS7 (mbedtls_aes) [真实]
 *     - 其余服务 (MAC_VERIFY/SIGNATURE/KEY 派生等) -> 显式返回 E_NOT_OK
 *       (软件后端无对应能力, fail-closed; MAC_VERIFY 由 Csm_MacVerify 生成后比较)
 *   - 密钥材料: RAM 密钥存储 (Csm_Cfg_KeyWrite/KeyRead), 掉电不保留;
 *     量产持久化需接入 NvM (超出本配置层范围, 已文档声明)
 *   - Csm_Cfg_RandomGenerate: 无 TRNG 熵源接入 -> 显式 E_NOT_OK (fail-closed)
 *   - Csm_Cfg_GetTimestamp: 主机 clock_gettime; 裸机返回 0 (与 Dlt 同模式, 需平台 tick 集成)
 *
 * 注意: Csm_Cfg_HwService 签名不含 keyId/算法参数, 软件后端固定使用
 *       CSM_KEY_ID_MASTER / CSM_KEY_ELEMENT_ID_SECRET 作为密钥 (文档声明)。
 *
 * @author yuleASR Team
 * @version 1.1.0
 */

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "Csm_Cfg.h"
#include "Csm_Types.h"
#include "Std_Types.h"

#include <string.h>
#include <mbedtls/sha256.h>
#include <mbedtls/aes.h>

#if defined(__unix__) || defined(__APPLE__)
#include <time.h>
#endif

/*==================================================================================================
*                                       本地常量
==================================================================================================*/
#define CSM_CFG_KEY_STORE_MAX_KEYS      8U      /* keyId 0..7 (CSM_KEY_ID_MASTER=1 ... DEBUG=7) */
#define CSM_CFG_KEY_STORE_MAX_ELEMENTS  6U      /* elementId 1..6 */
#define CSM_CFG_SHA256_DIGEST_LEN       32U
#define CSM_CFG_AES_BLOCK_LEN           16U

/*==================================================================================================
*                                       RAM 密钥存储
==================================================================================================*/
/* 软件后端密钥存储: 会话内有效, 掉电不保留 (持久化需 NvM 集成, 文档声明) */
static uint8  Csm_Cfg_KeyStore[CSM_CFG_KEY_STORE_MAX_KEYS][CSM_CFG_KEY_STORE_MAX_ELEMENTS][CSM_MAX_KEY_LENGTH];
static uint32 Csm_Cfg_KeyStoreLen[CSM_CFG_KEY_STORE_MAX_KEYS][CSM_CFG_KEY_STORE_MAX_ELEMENTS];
static boolean Csm_Cfg_KeyStoreValid[CSM_CFG_KEY_STORE_MAX_KEYS][CSM_CFG_KEY_STORE_MAX_ELEMENTS];

/*==================================================================================================
*                                       内部函数
==================================================================================================*/
/**
 * @brief HMAC-SHA256 (RFC 2104) — 基于 mbedtls_sha256 原语, 无动态分配
 */
static void Csm_Cfg_HmacSha256(
    const uint8* key,
    uint32 keyLen,
    const uint8* msg,
    uint32 msgLen,
    uint8 out[CSM_CFG_SHA256_DIGEST_LEN])
{
    uint8 k[64];
    uint8 ipad[64];
    uint8 opad[64];
    uint8 inner[CSM_CFG_SHA256_DIGEST_LEN];
    uint32 i;

    /* 密钥规整: >64B 先哈希, <=64B 补零 */
    if (keyLen > 64U)
    {
        mbedtls_sha256(key, keyLen, k, 0);
        keyLen = CSM_CFG_SHA256_DIGEST_LEN;
    }
    else
    {
        (void)memcpy(k, key, keyLen);
    }
    for (i = keyLen; i < 64U; i++)
    {
        k[i] = 0U;
    }

    for (i = 0U; i < 64U; i++)
    {
        ipad[i] = (uint8)(k[i] ^ 0x36U);
        opad[i] = (uint8)(k[i] ^ 0x5CU);
    }

    /* inner = SHA256(ipad || msg) */
    {
        mbedtls_sha256_context ctx;
        mbedtls_sha256_init(&ctx);
        mbedtls_sha256_starts(&ctx, 0);
        mbedtls_sha256_update(&ctx, ipad, 64U);
        mbedtls_sha256_update(&ctx, msg, msgLen);
        mbedtls_sha256_finish(&ctx, inner);
        mbedtls_sha256_free(&ctx);
    }

    /* out = SHA256(opad || inner) */
    {
        mbedtls_sha256_context ctx;
        mbedtls_sha256_init(&ctx);
        mbedtls_sha256_starts(&ctx, 0);
        mbedtls_sha256_update(&ctx, opad, 64U);
        mbedtls_sha256_update(&ctx, inner, CSM_CFG_SHA256_DIGEST_LEN);
        mbedtls_sha256_finish(&ctx, out);
        mbedtls_sha256_free(&ctx);
    }
}

/**
 * @brief AES-128-CBC 加解密 (PKCS7 填充) — 基于 mbedtls_aes, 无动态分配
 *
 * @param encrypt   TRUE=加密, FALSE=解密
 * @param key       16 字节密钥
 * @param iv        16 字节 IV (软件后端缺省为零 IV, 文档声明)
 * @param input     输入
 * @param inputLen  输入长度 (解密时须为 16 的倍数)
 * @param output    输出缓冲区 (容量至少 CSM_MAX_DATA_LENGTH)
 * @param outputLen 输入: 输出容量; 输出: 实际长度
 * @return E_OK / E_NOT_OK
 */
static Std_ReturnType Csm_Cfg_Aes128Cbc(
    boolean encrypt,
    const uint8* key,
    const uint8* iv,
    const uint8* input,
    uint32 inputLen,
    uint8* output,
    uint32* outputLen)
{
    uint8 padded[CSM_MAX_DATA_LENGTH + CSM_CFG_AES_BLOCK_LEN];
    uint32 totalLen;
    uint32 padLen;
    uint32 i;
    int ret;
    mbedtls_aes_context aes;

    if ((output == NULL_PTR) || (outputLen == NULL_PTR) || (iv == NULL_PTR))
    {
        return E_NOT_OK;
    }

    if (encrypt)
    {
        /* PKCS7 填充: 总是补 1..16 字节 */
        padLen = CSM_CFG_AES_BLOCK_LEN - (inputLen % CSM_CFG_AES_BLOCK_LEN);
        totalLen = inputLen + padLen;

        if ((inputLen > CSM_MAX_DATA_LENGTH) || (totalLen > CSM_MAX_DATA_LENGTH) ||
            (*outputLen < totalLen))
        {
            return E_NOT_OK;
        }

        if ((inputLen > 0U) && (input != NULL_PTR))
        {
            (void)memcpy(padded, input, inputLen);
        }
        for (i = 0U; i < padLen; i++)
        {
            padded[inputLen + i] = (uint8)padLen;
        }

        mbedtls_aes_init(&aes);
        ret = mbedtls_aes_setkey_enc(&aes, key, 128U);
        if (ret == 0)
        {
            ret = mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, totalLen,
                                        (unsigned char*)iv, padded, output);
        }
        mbedtls_aes_free(&aes);

        if (ret != 0)
        {
            return E_NOT_OK;
        }
        *outputLen = totalLen;
        return E_OK;
    }
    else
    {
        /* 解密: 输入须为块对齐且 >= 1 块 */
        if ((inputLen == 0U) || ((inputLen % CSM_CFG_AES_BLOCK_LEN) != 0U) ||
            (inputLen > CSM_MAX_DATA_LENGTH) || (*outputLen < inputLen) ||
            (input == NULL_PTR))
        {
            return E_NOT_OK;
        }

        mbedtls_aes_init(&aes);
        ret = mbedtls_aes_setkey_dec(&aes, key, 128U);
        if (ret == 0)
        {
            ret = mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, inputLen,
                                        (unsigned char*)iv, input, output);
        }
        mbedtls_aes_free(&aes);

        if (ret != 0)
        {
            return E_NOT_OK;
        }

        /* PKCS7 去填充 */
        padLen = output[inputLen - 1U];
        if ((padLen == 0U) || (padLen > CSM_CFG_AES_BLOCK_LEN) || (padLen > inputLen))
        {
            return E_NOT_OK;   /* 填充非法 */
        }
        *outputLen = inputLen - padLen;
        return E_OK;
    }
}

/*==================================================================================================
*                                       配置回调实现
==================================================================================================*/

/**
 * @brief 密钥写入 (RAM 密钥存储, 会话内有效)
 */
Std_ReturnType Csm_Cfg_KeyWrite(
    uint32 keyId,
    uint32 elementId,
    const uint8* data,
    uint32 length)
{
    if ((data == NULL_PTR) || (length == 0U) || (length > CSM_MAX_KEY_LENGTH))
    {
        return E_NOT_OK;
    }
    if ((keyId >= CSM_CFG_KEY_STORE_MAX_KEYS) ||
        (elementId == 0U) || (elementId > CSM_CFG_KEY_STORE_MAX_ELEMENTS))
    {
        return E_NOT_OK;
    }

    (void)memcpy(Csm_Cfg_KeyStore[keyId][elementId - 1U], data, length);
    Csm_Cfg_KeyStoreLen[keyId][elementId - 1U] = length;
    Csm_Cfg_KeyStoreValid[keyId][elementId - 1U] = TRUE;
    return E_OK;
}

/**
 * @brief 密钥读取 (RAM 密钥存储)
 */
Std_ReturnType Csm_Cfg_KeyRead(
    uint32 keyId,
    uint32 elementId,
    uint8* data,
    uint32* length)
{
    uint32 storedLen;

    if ((data == NULL_PTR) || (length == NULL_PTR))
    {
        return E_NOT_OK;
    }
    if ((keyId >= CSM_CFG_KEY_STORE_MAX_KEYS) ||
        (elementId == 0U) || (elementId > CSM_CFG_KEY_STORE_MAX_ELEMENTS))
    {
        return E_NOT_OK;
    }
    if (!Csm_Cfg_KeyStoreValid[keyId][elementId - 1U])
    {
        return E_NOT_OK;
    }

    storedLen = Csm_Cfg_KeyStoreLen[keyId][elementId - 1U];
    if (*length < storedLen)
    {
        return E_NOT_OK;   /* 缓冲区不足 */
    }

    (void)memcpy(data, Csm_Cfg_KeyStore[keyId][elementId - 1U], storedLen);
    *length = storedLen;
    return E_OK;
}

/**
 * @brief 硬件加密服务 — mbedTLS 软件后端 (P0-B 修复: 原无定义)
 *
 * 支持: HASH(SHA-256) / MAC_GENERATE(HMAC-SHA256) / ENCRYPT·DECRYPT(AES-128-CBC)
 * 其余服务显式返回 E_NOT_OK (软件后端无能力, fail-closed, 禁止假 OK)。
 * 密钥固定使用 CSM_KEY_ID_MASTER / CSM_KEY_ELEMENT_ID_SECRET (签名无 keyId 参数)。
 */
Std_ReturnType Csm_Cfg_HwService(
    uint32 jobId,
    Csm_ServiceType serviceType,
    const uint8* input,
    uint32 inputLength,
    uint8* output,
    uint32* outputLength)
{
    uint8 keyBuf[CSM_MAX_KEY_LENGTH];
    uint32 keyLen;
    uint8 ivBuf[CSM_CFG_AES_BLOCK_LEN];
    uint32 ivLen;

    (void)jobId;

    if ((output == NULL_PTR) || (outputLength == NULL_PTR))
    {
        return E_NOT_OK;
    }
    if ((input == NULL_PTR) && (inputLength > 0U))
    {
        return E_NOT_OK;
    }

    switch (serviceType)
    {
        case CSM_SERVICE_HASH:
            /* SHA-256 (CSM_CFG_DEFAULT_HASH_ALGORITHM = SHA2_256) */
            if (inputLength > CSM_MAX_DATA_LENGTH)
            {
                return E_NOT_OK;
            }
            if (*outputLength < CSM_CFG_SHA256_DIGEST_LEN)
            {
                return E_NOT_OK;
            }
            if (mbedtls_sha256(input, inputLength, output, 0) != 0)
            {
                return E_NOT_OK;
            }
            *outputLength = CSM_CFG_SHA256_DIGEST_LEN;
            return E_OK;

        case CSM_SERVICE_MAC_GENERATE:
            /* HMAC-SHA256, 密钥 = MASTER/SECRET */
            keyLen = sizeof(keyBuf);
            if (Csm_Cfg_KeyRead(CSM_KEY_ID_MASTER, CSM_KEY_ELEMENT_ID_SECRET,
                                keyBuf, &keyLen) != E_OK)
            {
                return E_NOT_OK;   /* 未配置密钥: 显式失败 */
            }
            if (*outputLength < CSM_CFG_SHA256_DIGEST_LEN)
            {
                return E_NOT_OK;
            }
            Csm_Cfg_HmacSha256(keyBuf, keyLen, input, inputLength, output);
            *outputLength = CSM_CFG_SHA256_DIGEST_LEN;
            return E_OK;

        case CSM_SERVICE_ENCRYPT:
        case CSM_SERVICE_DECRYPT:
            /* AES-128-CBC, 密钥 = MASTER/SECRET; IV 优先取 MASTER/IV, 缺省零 IV */
            keyLen = sizeof(keyBuf);
            if (Csm_Cfg_KeyRead(CSM_KEY_ID_MASTER, CSM_KEY_ELEMENT_ID_SECRET,
                                keyBuf, &keyLen) != E_OK)
            {
                return E_NOT_OK;   /* 未配置密钥: 显式失败 */
            }
            (void)memset(ivBuf, 0, sizeof(ivBuf));   /* 缺省零 IV (文档声明) */
            ivLen = sizeof(ivBuf);
            if (Csm_Cfg_KeyRead(CSM_KEY_ID_MASTER, CSM_KEY_ELEMENT_ID_IV,
                                ivBuf, &ivLen) == E_OK)
            {
                if (ivLen >= CSM_CFG_AES_BLOCK_LEN)
                {
                    ivLen = CSM_CFG_AES_BLOCK_LEN;
                }
                else
                {
                    ivLen = 0U;   /* IV 不足 16B: 保持零 IV */
                }
            }
            return Csm_Cfg_Aes128Cbc((serviceType == CSM_SERVICE_ENCRYPT),
                                     keyBuf, ivBuf,
                                     input, inputLength, output, outputLength);

        case CSM_SERVICE_MAC_VERIFY:
            /* MAC_VERIFY 由 Csm_MacVerify 生成后比较; 软件后端不提供期望值通道 */
            return E_NOT_OK;

        default:
            /* SIGNATURE_GENERATE/VERIFY, KEY_GENERATE/DERIVE/EXCHANGE, RANDOM:
             * 软件后端无对应实现 -> 显式失败 (fail-closed) */
            return E_NOT_OK;
    }
}

/**
 * @brief 随机数生成 — 无 TRNG 熵源接入, 显式返回 E_NOT_OK (fail-closed)
 *
 * 量产配置需接入 Crypto_HwTrng / HSM 熵源 (超出本软件配置层范围, 文档声明)。
 */
Std_ReturnType Csm_Cfg_RandomGenerate(
    uint8* data,
    uint32 length)
{
    (void)data;
    (void)length;
    return E_NOT_OK;
}

/**
 * @brief 获取当前时间戳 (ms)
 *
 * 主机: clock_gettime(CLOCK_MONOTONIC); 裸机: 返回 0, 需平台 tick 集成
 * (与 Dlt_GetTimestampUs 模式一致; 仅用于作业排队/密钥时间戳, 非安全决策)
 */
uint32 Csm_Cfg_GetTimestamp(void)
{
#if defined(__unix__) || defined(__APPLE__)
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
    {
        return (uint32)((ts.tv_sec * 1000U) + (uint32)(ts.tv_nsec / 1000000U));
    }
    return 0U;
#else
    /* 裸机平台: 集成层需提供 tick 源 */
    return 0U;
#endif
}
