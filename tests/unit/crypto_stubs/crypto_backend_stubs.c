/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Crypto (MCAL) Unit Test - Backend Stubs
*
* SW Version           : 1.0.0
* Build Date           : 2026-10-26
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* @file crypto_backend_stubs.c
* @brief Crypto.c 后端符号桩 (HSM 硬件 / MbedTLS 软件后端) + Det_ReportError 计数桩
*
* 说明:
*  - Crypto.c 通过 extern 声明引用 Crypto_Hsm_* 与 Crypto_MbedTLS_* 后端符号,
*    本文件按 src/bsw/mcal/crypto/src/Crypto.c 中的 extern 原型逐一提供桩实现,
*    使单测只链接被测核心 (Crypto.c) + 真实 blake2 (third_party/crypto/blake2)。
*  - Hsm_Init 返回 E_NOT_OK -> Crypto_HsmAvailable 保持 FALSE, 覆盖 Crypto.c
*    的软件回退路径 (Crypto_MbedTLS_*) 与 HSM 包装函数的不可用分支。
*  - MbedTLS 桩返回 E_OK 并回写确定性伪数据 (SHA/Random/ECDSA), 使成功路径被覆盖。
*  - Det_ReportError 提供计数实现, 测试可通过 Crypto_Stubs_GetDetErrorCount()
*    断言 DEF 分支被触发。
==================================================================================================*/

#include "Std_Types.h"
#include "Crypto.h"
#include "Det.h"

/*==================================================================================================
*                                      DET ERROR COUNTER STUB
==================================================================================================*/
static uint32 crypto_stub_det_errors = 0U;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    crypto_stub_det_errors++;
    return E_OK;
}

uint32 Crypto_Stubs_GetDetErrorCount(void)
{
    return crypto_stub_det_errors;
}

void Crypto_Stubs_ResetDetErrorCount(void)
{
    crypto_stub_det_errors = 0U;
}

/*==================================================================================================
*                                      STUB CONTROL FLAGS
*  允许测试动态切换 HSM 可用性、MbedTLS_Init 成功/失败、HSM 调用返回值等。
*==================================================================================================*/
static boolean crypto_stub_hsm_enabled = TRUE;   /* 默认 HSM 可用 -> 触发 Crypto.c 的 HSM 分支 */
static boolean crypto_stub_mbedtls_init_fail = FALSE;
static Std_ReturnType crypto_stub_hsm_process_ret = E_OK;
static Std_ReturnType crypto_stub_hsm_loadkey_ret = E_OK;
static Std_ReturnType crypto_stub_hsm_selftest_ret = E_OK;

void Crypto_Stubs_SetHsmEnabled(boolean enabled)        { crypto_stub_hsm_enabled = enabled; }
void Crypto_Stubs_SetMbedTlsInitFail(boolean fail)      { crypto_stub_mbedtls_init_fail = fail; }
void Crypto_Stubs_SetHsmProcessRet(Std_ReturnType ret)  { crypto_stub_hsm_process_ret = ret; }
void Crypto_Stubs_SetHsmLoadKeyRet(Std_ReturnType ret)  { crypto_stub_hsm_loadkey_ret = ret; }
void Crypto_Stubs_SetHsmSelfTestRet(Std_ReturnType ret) { crypto_stub_hsm_selftest_ret = ret; }
void Crypto_Stubs_ResetAllHsmFlags(void)
{
    crypto_stub_hsm_enabled = TRUE;
    crypto_stub_mbedtls_init_fail = FALSE;
    crypto_stub_hsm_process_ret = E_OK;
    crypto_stub_hsm_loadkey_ret = E_OK;
    crypto_stub_hsm_selftest_ret = E_OK;
}

/*==================================================================================================
*                                      HSM BACKEND STUBS
*  策略: 默认 HSM 可用 (Init + IsAvailable 都返回正)
*        -> Crypto.c 进入 HSM 路径 + Crypto_HsmAvailable = TRUE
*  测试可通过 Crypto_Stubs_SetHsmEnabled(FALSE) 切换回软件回退模式
==================================================================================================*/
Std_ReturnType Crypto_Hsm_Init(const Crypto_HsmConfigType* config)
{
    (void)config;
    return crypto_stub_hsm_enabled ? E_OK : E_NOT_OK;
}

void Crypto_Hsm_DeInit(void)
{
    /* nothing to do */
}

boolean Crypto_Hsm_IsAvailable(void)
{
    return crypto_stub_hsm_enabled;
}

Crypto_HsmStateType Crypto_Hsm_GetState(void)
{
    return crypto_stub_hsm_enabled ? CRYPTO_HSM_IDLE : CRYPTO_HSM_UNINIT;
}

Std_ReturnType Crypto_Hsm_ProcessJob(Crypto_JobType* job)
{
    (void)job;
    return crypto_stub_hsm_process_ret;
}

Std_ReturnType Crypto_Hsm_LoadKey(Crypto_KeyIdType keyId)
{
    (void)keyId;
    return crypto_stub_hsm_loadkey_ret;
}

Std_ReturnType Crypto_Hsm_SelfTest(void)
{
    return crypto_stub_hsm_selftest_ret;
}

/*==================================================================================================
*                                      MbedTLS BACKEND STUBS
==================================================================================================*/
Std_ReturnType Crypto_MbedTLS_Init(void)
{
    return crypto_stub_mbedtls_init_fail ? E_NOT_OK : E_OK;
}

void Crypto_MbedTLS_DeInit(void)
{
    /* nothing to do */
}

Std_ReturnType Crypto_MbedTLS_ProcessJob(Crypto_JobType* job)
{
    (void)job;
    return E_OK;
}

/* 确定性伪随机: 0xA5, 0xA6, ... */
Std_ReturnType Crypto_MbedTLS_RandomGenerate(uint8* resultPtr, uint32 resultLength)
{
    uint32 i;
    if ((resultPtr == NULL_PTR) || (resultLength == 0U))
    {
        return E_NOT_OK;
    }
    for (i = 0U; i < resultLength; i++)
    {
        resultPtr[i] = (uint8)(0xA5U + i);
    }
    return E_OK;
}

Std_ReturnType Crypto_MbedTLS_KeyGenerate(Crypto_KeyIdType keyId)
{
    (void)keyId;
    return E_OK;
}

Std_ReturnType Crypto_MbedTLS_KeyDerive(Crypto_KeyIdType srcKeyId, Crypto_KeyIdType dstKeyId)
{
    (void)srcKeyId;
    (void)dstKeyId;
    return E_OK;
}

Std_ReturnType Crypto_MbedTLS_ECDH_CalcSecret(Crypto_KeyIdType privKeyId, const uint8* pubKeyPtr,
                                              uint32 pubKeyLen)
{
    (void)privKeyId;
    (void)pubKeyPtr;
    (void)pubKeyLen;
    return E_OK;
}

/* 确定性签名: 64 字节 0x5A 序列, 回写 *sigLen */
Std_ReturnType Crypto_MbedTLS_ECDSA_Sign(Crypto_KeyIdType keyId, const uint8* digest, uint32 digestLen,
                                         uint8* sig, uint32* sigLen)
{
    uint32 i;
    (void)keyId;
    (void)digest;
    (void)digestLen;
    if ((sig == NULL_PTR) || (sigLen == NULL_PTR))
    {
        return E_NOT_OK;
    }
    for (i = 0U; (i < 64U) && (i < *sigLen); i++)
    {
        sig[i] = 0x5AU;
    }
    *sigLen = 64U;
    return E_OK;
}

Std_ReturnType Crypto_MbedTLS_ECDSA_Verify(Crypto_KeyIdType keyId, const uint8* digest, uint32 digestLen,
                                           const uint8* sig, uint32 sigLen)
{
    (void)keyId;
    (void)digest;
    (void)digestLen;
    (void)sig;
    (void)sigLen;
    return E_OK;
}

Std_ReturnType Crypto_MbedTLS_AES_GCM_Encrypt(Crypto_KeyIdType keyId, const uint8* pt, uint32 ptLen,
                                              const uint8* aad, uint32 aadLen, const uint8* iv, uint8* ct,
                                              uint8* tag)
{
    uint32 i;
    (void)keyId;
    (void)aad;
    (void)aadLen;
    (void)iv;
    for (i = 0U; i < ptLen; i++)
    {
        ct[i] = (uint8)(pt[i] ^ 0x5AU);
    }
    if (tag != NULL_PTR)
    {
        for (i = 0U; i < CRYPTO_CFG_CCC_TAG_SIZE; i++)
        {
            tag[i] = 0xC3U;
        }
    }
    return E_OK;
}

Std_ReturnType Crypto_MbedTLS_AES_GCM_Decrypt(Crypto_KeyIdType keyId, const uint8* ct, uint32 ctLen,
                                              const uint8* aad, uint32 aadLen, const uint8* iv, const uint8* tag,
                                              uint8* pt)
{
    uint32 i;
    (void)keyId;
    (void)aad;
    (void)aadLen;
    (void)iv;
    (void)tag;
    for (i = 0U; i < ctLen; i++)
    {
        pt[i] = (uint8)(ct[i] ^ 0x5AU);
    }
    return E_OK;
}

/* 确定性摘要: 32 字节 (i*7+1) */
Std_ReturnType Crypto_MbedTLS_SHA256(const uint8* data, uint32 dataLen, uint8* digest)
{
    uint32 i;
    (void)data;
    (void)dataLen;
    if (digest == NULL_PTR)
    {
        return E_NOT_OK;
    }
    for (i = 0U; i < 32U; i++)
    {
        digest[i] = (uint8)((i * 7U) + 1U);
    }
    return E_OK;
}

Std_ReturnType Crypto_MbedTLS_HMAC(Crypto_KeyIdType keyId, const uint8* data, uint32 dataLen, uint8* mac)
{
    uint32 i;
    (void)keyId;
    (void)data;
    (void)dataLen;
    if (mac == NULL_PTR)
    {
        return E_NOT_OK;
    }
    for (i = 0U; i < 32U; i++)
    {
        mac[i] = (uint8)(0x30U + i);
    }
    return E_OK;
}

Std_ReturnType Crypto_MbedTLS_HKDF(Crypto_KeyIdType ikmKeyId, const uint8* salt, uint32 saltLen,
                                   const uint8* info, uint32 infoLen, uint8* okm, uint32 okmLen)
{
    uint32 i;
    (void)ikmKeyId;
    (void)salt;
    (void)saltLen;
    (void)info;
    (void)infoLen;
    if (okm == NULL_PTR)
    {
        return E_NOT_OK;
    }
    for (i = 0U; i < okmLen; i++)
    {
        okm[i] = (uint8)(0x11U + i);
    }
    return E_OK;
}
