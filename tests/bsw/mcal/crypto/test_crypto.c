/**
 * @file test_crypto.c
 * @brief Crypto Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/mcal/crypto/src/Crypto.c  @tests src/bsw/mcal/crypto/include/Crypto.h

#include "unity.h"
#include "mock_registers.h"
#include "mock_det.h"
#include "Crypto.h"

#include <string.h>

/*==================================================================================================
 * Test support
 *==================================================================================================*/

/*
 * Crypto.c delegates all primitive crypto operations to the Crypto_MbedTLS_*
 * backend and (when compiled in, CRYPTO_CFG_HSM_ENABLED == STD_ON) the
 * Crypto_Hsm_* backend. Both backends are replaced here by capturing stubs so
 * the core driver logic (DET validation, job routing, queue, key element
 * storage, notifications) is verified in isolation.
 *
 * BLAKE2 hashing is NOT stubbed: the real third_party implementation is
 * linked, so Crypto_Blake2b/Crypto_Blake2s results are checked against the
 * RFC 7693 appendix B reference vectors.
 *
 * Crypto_DeInit() fully resets the driver state, so tearDown can restore the
 * genuine UNINIT state before every test (unlike Fls, which has no DeInit).
 */

#define TEST_CRYPTO_KEY0_ELEM0_ID   (10U)  /* key material, 32 bytes, no partial access */
#define TEST_CRYPTO_KEY0_ELEM1_ID   (7U)   /* salt, 16 bytes, partial access allowed    */
#define TEST_CRYPTO_KEY1_ELEM0_ID   (2U)   /* hmac key, 32 bytes, no partial access     */

static uint8 test_Crypto_Key0Elem0Data[32U];
static uint8 test_Crypto_Key0Elem1Data[16U];
static uint8 test_Crypto_Key1Elem0Data[32U];

static Crypto_KeyElementType test_Crypto_Key0Elements[2] = {
    { TEST_CRYPTO_KEY0_ELEM0_ID, 32U, FALSE, TRUE, test_Crypto_Key0Elem0Data },
    { TEST_CRYPTO_KEY0_ELEM1_ID, 16U, TRUE,  TRUE, test_Crypto_Key0Elem1Data }
};

static Crypto_KeyElementType test_Crypto_Key1Elements[1] = {
    { TEST_CRYPTO_KEY1_ELEM0_ID, 32U, FALSE, TRUE, test_Crypto_Key1Elem0Data }
};

static Crypto_KeyType test_Crypto_Keys[2] = {
    { 0U, 2U, test_Crypto_Key0Elements, CRYPTO_KEY_TYPE_CUSTOM, CRYPTO_KEY_INVALID },
    { 1U, 1U, test_Crypto_Key1Elements, CRYPTO_KEY_TYPE_CUSTOM, CRYPTO_KEY_INVALID }
};

static Crypto_ConfigType test_Crypto_Config = {
    2U,                                   /* numKeys                */
    test_Crypto_Keys,                     /* keys                   */
    4U,                                   /* numChannels            */
    2U,                                   /* queueSize              */
    { FALSE, 0U, 0U, 1000U, 5000U },      /* hsmConfig              */
    TRUE,                                 /* cryptoDevErrorDetect   */
    TRUE,                                 /* cryptoVersionInfoApi   */
    NULL_PTR,                             /* driverObjects          */
    0U,                                   /* numDriverObjects       */
    NULL_PTR,                             /* channels               */
    FALSE,                                /* hwAccelerationEnabled  */
    0U                                    /* clockFrequency         */
};

/* Notification callback: overrides the __attribute__((weak)) default in
 * Crypto.c so tests can observe job completion / cancellation. */
static uint32 test_Crypto_NotifCount;
static Crypto_JobType* test_Crypto_NotifLastJob;
static Crypto_ResultType test_Crypto_NotifLastResult;

void Crypto_JobNotification(Crypto_JobType* job, Crypto_ResultType result)
{
    test_Crypto_NotifCount++;
    test_Crypto_NotifLastJob = job;
    test_Crypto_NotifLastResult = result;
}

/*==================================================================================================
 * MbedTLS backend stubs (capture arguments, return configurable results)
 *==================================================================================================*/

static Std_ReturnType test_Crypto_MbedtlsInitResult = E_OK;
static uint32 test_Crypto_MbedtlsInitCount;
static uint32 test_Crypto_MbedtlsDeInitCount;

static Std_ReturnType test_Crypto_MbedtlsProcessJobResult = E_OK;
static uint32 test_Crypto_MbedtlsProcessJobCount;
static Crypto_JobType* test_Crypto_MbedtlsProcessJobLastJob;

static uint32 test_Crypto_MbedtlsRandomCount;
static const uint8* test_Crypto_MbedtlsRandomLastPtr;
static uint32 test_Crypto_MbedtlsRandomLastLen;

static uint32 test_Crypto_MbedtlsKeyGenerateCount;
static Crypto_KeyIdType test_Crypto_MbedtlsKeyGenerateLastId;

static uint32 test_Crypto_MbedtlsKeyDeriveCount;
static Crypto_KeyIdType test_Crypto_MbedtlsKeyDeriveLastSrc;
static Crypto_KeyIdType test_Crypto_MbedtlsKeyDeriveLastDst;

static uint32 test_Crypto_MbedtlsEcdhCount;
static Crypto_KeyIdType test_Crypto_MbedtlsEcdhLastKeyId;
static const uint8* test_Crypto_MbedtlsEcdhLastPtr;
static uint32 test_Crypto_MbedtlsEcdhLastLen;

static uint32 test_Crypto_MbedtlsSignCount;
static Crypto_KeyIdType test_Crypto_MbedtlsSignLastKeyId;
static uint8 test_Crypto_MbedtlsSignLastDigest0;
static uint32 test_Crypto_MbedtlsSignLastDigestLen;

static uint32 test_Crypto_MbedtlsSha256Count;
static const uint8* test_Crypto_MbedtlsSha256LastPtr;
static uint32 test_Crypto_MbedtlsSha256LastLen;

static uint32 test_Crypto_MbedtlsGcmEncCount;
static Crypto_KeyIdType test_Crypto_MbedtlsGcmEncLastKeyId;
static const uint8* test_Crypto_MbedtlsGcmEncLastPt;
static uint32 test_Crypto_MbedtlsGcmEncLastPtLen;
static const uint8* test_Crypto_MbedtlsGcmEncLastAad;
static uint32 test_Crypto_MbedtlsGcmEncLastAadLen;
static const uint8* test_Crypto_MbedtlsGcmEncLastIv;

static uint32 test_Crypto_MbedtlsGcmDecCount;
static const uint8* test_Crypto_MbedtlsGcmDecLastCt;
static uint32 test_Crypto_MbedtlsGcmDecLastCtLen;

Std_ReturnType Crypto_MbedTLS_Init(void)
{
    test_Crypto_MbedtlsInitCount++;
    return test_Crypto_MbedtlsInitResult;
}

void Crypto_MbedTLS_DeInit(void)
{
    test_Crypto_MbedtlsDeInitCount++;
}

Std_ReturnType Crypto_MbedTLS_ProcessJob(Crypto_JobType* job)
{
    test_Crypto_MbedtlsProcessJobCount++;
    test_Crypto_MbedtlsProcessJobLastJob = job;
    return test_Crypto_MbedtlsProcessJobResult;
}

Std_ReturnType Crypto_MbedTLS_RandomGenerate(uint8* resultPtr, uint32 resultLength)
{
    uint32 i;
    test_Crypto_MbedtlsRandomCount++;
    test_Crypto_MbedtlsRandomLastPtr = resultPtr;
    test_Crypto_MbedtlsRandomLastLen = resultLength;
    for (i = 0U; i < resultLength; i++)
    {
        resultPtr[i] = (uint8)(0x5CU ^ (uint8)i); /* deterministic pattern */
    }
    return E_OK;
}

Std_ReturnType Crypto_MbedTLS_KeyGenerate(Crypto_KeyIdType keyId)
{
    test_Crypto_MbedtlsKeyGenerateCount++;
    test_Crypto_MbedtlsKeyGenerateLastId = keyId;
    return E_OK;
}

Std_ReturnType Crypto_MbedTLS_KeyDerive(Crypto_KeyIdType srcKeyId, Crypto_KeyIdType dstKeyId)
{
    test_Crypto_MbedtlsKeyDeriveCount++;
    test_Crypto_MbedtlsKeyDeriveLastSrc = srcKeyId;
    test_Crypto_MbedtlsKeyDeriveLastDst = dstKeyId;
    return E_OK;
}

Std_ReturnType Crypto_MbedTLS_ECDH_CalcSecret(Crypto_KeyIdType privKeyId, const uint8* pubKeyPtr,
                                               uint32 pubKeyLen)
{
    test_Crypto_MbedtlsEcdhCount++;
    test_Crypto_MbedtlsEcdhLastKeyId = privKeyId;
    test_Crypto_MbedtlsEcdhLastPtr = pubKeyPtr;
    test_Crypto_MbedtlsEcdhLastLen = pubKeyLen;
    return E_OK;
}

Std_ReturnType Crypto_MbedTLS_ECDSA_Sign(Crypto_KeyIdType keyId, const uint8* digest, uint32 digestLen,
                                         uint8* sig, uint32* sigLen)
{
    uint32 i;
    test_Crypto_MbedtlsSignCount++;
    test_Crypto_MbedtlsSignLastKeyId = keyId;
    test_Crypto_MbedtlsSignLastDigest0 = digest[0];
    test_Crypto_MbedtlsSignLastDigestLen = digestLen;
    for (i = 0U; i < 64U; i++)
    {
        sig[i] = (uint8)(0xE9U ^ (uint8)i); /* deterministic signature */
    }
    *sigLen = 64U;
    return E_OK;
}

Std_ReturnType Crypto_MbedTLS_ECDSA_Verify(Crypto_KeyIdType keyId, const uint8* digest, uint32 digestLen,
                                            const uint8* sig, uint32 sigLen)
{
    (void)keyId; (void)digest; (void)digestLen; (void)sig; (void)sigLen;
    return E_NOT_OK; /* not reached by the tested code paths */
}

Std_ReturnType Crypto_MbedTLS_AES_GCM_Encrypt(Crypto_KeyIdType keyId, const uint8* pt, uint32 ptLen,
                                              const uint8* aad, uint32 aadLen, const uint8* iv, uint8* ct,
                                              uint8* tag)
{
    uint32 i;
    test_Crypto_MbedtlsGcmEncCount++;
    test_Crypto_MbedtlsGcmEncLastKeyId = keyId;
    test_Crypto_MbedtlsGcmEncLastPt = pt;
    test_Crypto_MbedtlsGcmEncLastPtLen = ptLen;
    test_Crypto_MbedtlsGcmEncLastAad = aad;
    test_Crypto_MbedtlsGcmEncLastAadLen = aadLen;
    test_Crypto_MbedtlsGcmEncLastIv = iv;
    for (i = 0U; i < ptLen; i++)
    {
        ct[i] = (uint8)(0x3CU ^ (uint8)i); /* deterministic ciphertext */
    }
    for (i = 0U; i < 16U; i++)
    {
        tag[i] = 0x7DU;
    }
    return E_OK;
}

Std_ReturnType Crypto_MbedTLS_AES_GCM_Decrypt(Crypto_KeyIdType keyId, const uint8* ct, uint32 ctLen,
                                              const uint8* aad, uint32 aadLen, const uint8* iv, const uint8* tag,
                                              uint8* pt)
{
    uint32 i;
    (void)aad; (void)aadLen; (void)iv; (void)tag;
    test_Crypto_MbedtlsGcmDecCount++;
    test_Crypto_MbedtlsGcmDecLastCt = ct;
    test_Crypto_MbedtlsGcmDecLastCtLen = ctLen;
    for (i = 0U; i < ctLen; i++)
    {
        pt[i] = (uint8)(0xB4U ^ (uint8)i); /* deterministic plaintext */
    }
    return E_OK;
}

Std_ReturnType Crypto_MbedTLS_SHA256(const uint8* data, uint32 dataLen, uint8* digest)
{
    uint32 i;
    test_Crypto_MbedtlsSha256Count++;
    test_Crypto_MbedtlsSha256LastPtr = data;
    test_Crypto_MbedtlsSha256LastLen = dataLen;
    for (i = 0U; i < 32U; i++)
    {
        digest[i] = 0xD7U; /* deterministic digest */
    }
    return E_OK;
}

Std_ReturnType Crypto_MbedTLS_HMAC(Crypto_KeyIdType keyId, const uint8* data, uint32 dataLen, uint8* mac)
{
    (void)keyId; (void)data; (void)dataLen; (void)mac;
    return E_NOT_OK; /* not reached by the tested code paths */
}

Std_ReturnType Crypto_MbedTLS_HKDF(Crypto_KeyIdType ikmKeyId, const uint8* salt, uint32 saltLen,
                                    const uint8* info, uint32 infoLen, uint8* okm, uint32 okmLen)
{
    (void)ikmKeyId; (void)salt; (void)saltLen; (void)info; (void)infoLen; (void)okm; (void)okmLen;
    return E_NOT_OK; /* not reached by the tested code paths */
}

/*==================================================================================================
 * HSM backend stubs (CRYPTO_CFG_HSM_ENABLED == STD_ON in Crypto_Cfg.h)
 *==================================================================================================*/

static Std_ReturnType test_Crypto_HsmInitResult = E_NOT_OK; /* default: HSM off */
static boolean test_Crypto_HsmAvailable = FALSE;
static uint32 test_Crypto_HsmInitCount;
static const Crypto_HsmConfigType* test_Crypto_HsmInitLastConfig;
static uint32 test_Crypto_HsmDeInitCount;
static uint32 test_Crypto_HsmGetStateCount;
static uint32 test_Crypto_HsmProcessJobCount;
static Crypto_JobType* test_Crypto_HsmProcessJobLastJob;
static uint32 test_Crypto_HsmLoadKeyCount;
static Crypto_KeyIdType test_Crypto_HsmLoadKeyLastId;
static uint32 test_Crypto_HsmSelfTestCount;

Std_ReturnType Crypto_Hsm_Init(const Crypto_HsmConfigType* config)
{
    test_Crypto_HsmInitCount++;
    test_Crypto_HsmInitLastConfig = config;
    return test_Crypto_HsmInitResult;
}

void Crypto_Hsm_DeInit(void)
{
    test_Crypto_HsmDeInitCount++;
}

boolean Crypto_Hsm_IsAvailable(void)
{
    return test_Crypto_HsmAvailable;
}

Crypto_HsmStateType Crypto_Hsm_GetState(void)
{
    test_Crypto_HsmGetStateCount++;
    return CRYPTO_HSM_IDLE;
}

Std_ReturnType Crypto_Hsm_ProcessJob(Crypto_JobType* job)
{
    test_Crypto_HsmProcessJobCount++;
    test_Crypto_HsmProcessJobLastJob = job;
    return E_OK;
}

Std_ReturnType Crypto_Hsm_LoadKey(Crypto_KeyIdType keyId)
{
    test_Crypto_HsmLoadKeyCount++;
    test_Crypto_HsmLoadKeyLastId = keyId;
    return E_OK;
}

Std_ReturnType Crypto_Hsm_SelfTest(void)
{
    test_Crypto_HsmSelfTestCount++;
    return E_OK;
}

/*==================================================================================================
 * Job helpers
 *==================================================================================================*/

static Crypto_AlgorithmInfoType test_Crypto_AlgInfo = {
    CRYPTO_ALGOFAM_AES, CRYPTO_ALGOMODE_GCM, 256U, 0U
};

static Crypto_JobPrimitiveInfoType test_Crypto_JobPrimitiveInfo;
static Crypto_JobPrimitiveInputOutputType test_Crypto_JobIO;
static Crypto_JobInfoType test_Crypto_JobInfo = { 1U, 1U };
static Crypto_JobType test_Crypto_Job;

static void test_Crypto_MakeJob(Crypto_ProcessingType processing, uint32 callbackId)
{
    test_Crypto_JobPrimitiveInfo.callbackId = callbackId;
    test_Crypto_JobPrimitiveInfo.algorithm = &test_Crypto_AlgInfo;
    test_Crypto_JobPrimitiveInfo.service = CRYPTO_SERVICE_ENCRYPT;
    test_Crypto_JobPrimitiveInfo.processingType = processing;
    test_Crypto_JobPrimitiveInfo.primitiveCallbackUpdateNotification = FALSE;

    memset(&test_Crypto_JobIO, 0, sizeof(test_Crypto_JobIO));

    test_Crypto_Job.jobId = 1U;
    test_Crypto_Job.jobState = CRYPTO_JOBSTATE_IDLE;
    test_Crypto_Job.jobPrimitiveInputOutput = &test_Crypto_JobIO;
    test_Crypto_Job.jobPrimitiveInfo = &test_Crypto_JobPrimitiveInfo;
    test_Crypto_Job.jobInfo = &test_Crypto_JobInfo;
    test_Crypto_Job.cryptoKeyId = 0U;
    test_Crypto_Job.targetCryptoKeyId = 0U;
    test_Crypto_Job.jobRedirectionInfoRef = 0U;
    test_Crypto_Job.targetKeyId = 0U;
}

/*==================================================================================================
 * setUp / tearDown / helpers
 *==================================================================================================*/

static void test_Crypto_ResetStubs(void)
{
    /* MbedTLS */
    test_Crypto_MbedtlsInitResult = E_OK;
    test_Crypto_MbedtlsInitCount = 0U;
    test_Crypto_MbedtlsDeInitCount = 0U;
    test_Crypto_MbedtlsProcessJobResult = E_OK;
    test_Crypto_MbedtlsProcessJobCount = 0U;
    test_Crypto_MbedtlsProcessJobLastJob = NULL_PTR;
    test_Crypto_MbedtlsRandomCount = 0U;
    test_Crypto_MbedtlsRandomLastPtr = NULL_PTR;
    test_Crypto_MbedtlsRandomLastLen = 0U;
    test_Crypto_MbedtlsKeyGenerateCount = 0U;
    test_Crypto_MbedtlsKeyGenerateLastId = 0U;
    test_Crypto_MbedtlsKeyDeriveCount = 0U;
    test_Crypto_MbedtlsKeyDeriveLastSrc = 0U;
    test_Crypto_MbedtlsKeyDeriveLastDst = 0U;
    test_Crypto_MbedtlsEcdhCount = 0U;
    test_Crypto_MbedtlsEcdhLastKeyId = 0U;
    test_Crypto_MbedtlsEcdhLastPtr = NULL_PTR;
    test_Crypto_MbedtlsEcdhLastLen = 0U;
    test_Crypto_MbedtlsSignCount = 0U;
    test_Crypto_MbedtlsSignLastKeyId = 0U;
    test_Crypto_MbedtlsSignLastDigest0 = 0U;
    test_Crypto_MbedtlsSignLastDigestLen = 0U;
    test_Crypto_MbedtlsSha256Count = 0U;
    test_Crypto_MbedtlsSha256LastPtr = NULL_PTR;
    test_Crypto_MbedtlsSha256LastLen = 0U;
    test_Crypto_MbedtlsGcmEncCount = 0U;
    test_Crypto_MbedtlsGcmEncLastKeyId = 0U;
    test_Crypto_MbedtlsGcmEncLastPt = NULL_PTR;
    test_Crypto_MbedtlsGcmEncLastPtLen = 0U;
    test_Crypto_MbedtlsGcmEncLastAad = NULL_PTR;
    test_Crypto_MbedtlsGcmEncLastAadLen = 0U;
    test_Crypto_MbedtlsGcmEncLastIv = NULL_PTR;
    test_Crypto_MbedtlsGcmDecCount = 0U;
    test_Crypto_MbedtlsGcmDecLastCt = NULL_PTR;
    test_Crypto_MbedtlsGcmDecLastCtLen = 0U;
    /* HSM */
    test_Crypto_HsmInitResult = E_NOT_OK;
    test_Crypto_HsmAvailable = FALSE;
    test_Crypto_HsmInitCount = 0U;
    test_Crypto_HsmInitLastConfig = NULL_PTR;
    test_Crypto_HsmDeInitCount = 0U;
    test_Crypto_HsmGetStateCount = 0U;
    test_Crypto_HsmProcessJobCount = 0U;
    test_Crypto_HsmProcessJobLastJob = NULL_PTR;
    test_Crypto_HsmLoadKeyCount = 0U;
    test_Crypto_HsmLoadKeyLastId = 0U;
    test_Crypto_HsmSelfTestCount = 0U;
    /* Notification */
    test_Crypto_NotifCount = 0U;
    test_Crypto_NotifLastJob = NULL_PTR;
    test_Crypto_NotifLastResult = CRYPTO_RESULT_OK;
    /* Key storage */
    memset(test_Crypto_Key0Elem0Data, 0, sizeof(test_Crypto_Key0Elem0Data));
    memset(test_Crypto_Key0Elem1Data, 0, sizeof(test_Crypto_Key0Elem1Data));
    memset(test_Crypto_Key1Elem0Data, 0, sizeof(test_Crypto_Key1Elem0Data));
    test_Crypto_Keys[0].keyState = CRYPTO_KEY_INVALID;
    test_Crypto_Keys[1].keyState = CRYPTO_KEY_INVALID;
}

void setUp(void)
{
    MockRegisters_Reset();
    Det_Mock_Reset();
    test_Crypto_ResetStubs();
}

void tearDown(void)
{
    /* Restore the genuine UNINIT state for the next test. */
    Crypto_DeInit();
}

/* Initializes the driver with the test configuration. Fails the test if the
 * init itself produces a DET report. */
static void test_Crypto_EnsureInit(void)
{
    Det_Mock_Reset();
    Crypto_DeInit(); /* silent no-op when already uninitialized */
    Crypto_Init(&test_Crypto_Config);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount); /* clean init */
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_MbedtlsInitCount);
}

/* Verifies the last DET report matches the expected API/error pair
 * (moduleId 110, instanceId 0 - the Crypto core always reports instance 0
 * except for Crypto_ProcessJob, which forwards objectId as instanceId). */
static void test_Crypto_AssertDet(uint8 expectedApiId, uint8 expectedErrorId)
{
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid); /* expected DET report */
    TEST_ASSERT_EQUAL_UINT(CRYPTO_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.InstanceId);
    TEST_ASSERT_EQUAL_UINT(expectedApiId, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL_UINT(expectedErrorId, Det_MockData.ErrorId);
}

/* Verifies that no DET report was issued at all. */
static void test_Crypto_AssertNoDet(void)
{
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid == FALSE);
}

/* Byte-wise comparison with per-byte failure location. */
static void test_Crypto_AssertBytes(const uint8* actual, const uint8* expected, uint32 len)
{
    uint32 i;
    for (i = 0U; i < len; i++)
    {
        TEST_ASSERT_EQUAL_UINT8(expected[i], actual[i]);
    }
}

/* Pointer identity check (unity.h here has no TEST_ASSERT_EQUAL_PTR). */
static void test_Crypto_AssertSamePtr(const void* expected, const void* actual)
{
    TEST_ASSERT_TRUE(expected == actual); /* pointers must be identical */
}

/*==================================================================================================
 * RFC 7693 reference vectors (verified against the linked implementation)
 *==================================================================================================*/

static const uint8 test_Crypto_Blake2b512_Abc[64] = {
    0xBA, 0x80, 0xA5, 0x3F, 0x98, 0x1C, 0x4D, 0x0D,
    0x6A, 0x27, 0x97, 0xB6, 0x9F, 0x12, 0xF6, 0xE9,
    0x4C, 0x21, 0x2F, 0x14, 0x68, 0x5A, 0xC4, 0xB7,
    0x4B, 0x12, 0xBB, 0x6F, 0xDB, 0xFF, 0xA2, 0xD1,
    0x7D, 0x87, 0xC5, 0x39, 0x2A, 0xAB, 0x79, 0x2D,
    0xC2, 0x52, 0xD5, 0xDE, 0x45, 0x33, 0xCC, 0x95,
    0x18, 0xD3, 0x8A, 0xA8, 0xDB, 0xF1, 0x92, 0x5A,
    0xB9, 0x23, 0x86, 0xED, 0xD4, 0x00, 0x99, 0x23
};

static const uint8 test_Crypto_Blake2b512_AbcKeyed[64] = {
    0x58, 0x2C, 0xBD, 0x20, 0x17, 0x7F, 0xB9, 0x6E,
    0x72, 0x14, 0x1E, 0x2A, 0xFC, 0xB6, 0xBD, 0xF0,
    0x3E, 0x89, 0x1E, 0xEE, 0x1F, 0x48, 0x71, 0x1E,
    0x14, 0x18, 0xE9, 0x66, 0x2E, 0x0B, 0xE3, 0x2B,
    0x69, 0xE1, 0xC3, 0x25, 0x59, 0xD3, 0x18, 0x97,
    0xA8, 0x83, 0x48, 0x14, 0x0A, 0x58, 0xEB, 0x48,
    0x11, 0x15, 0x37, 0xCC, 0x69, 0x3A, 0x50, 0xDC,
    0x8C, 0xA5, 0x86, 0x0E, 0x7E, 0x00, 0x5B, 0x14
};

static const uint8 test_Crypto_Blake2b512_Empty[64] = {
    0x78, 0x6A, 0x02, 0xF7, 0x42, 0x01, 0x59, 0x03,
    0xC6, 0xC6, 0xFD, 0x85, 0x25, 0x52, 0xD2, 0x72,
    0x91, 0x2F, 0x47, 0x40, 0xE1, 0x58, 0x47, 0x61,
    0x8A, 0x86, 0xE2, 0x17, 0xF7, 0x1F, 0x54, 0x19,
    0xD2, 0x5E, 0x10, 0x31, 0xAF, 0xEE, 0x58, 0x53,
    0x13, 0x89, 0x64, 0x44, 0x93, 0x4E, 0xB0, 0x4B,
    0x90, 0x3A, 0x68, 0x5B, 0x14, 0x48, 0xB7, 0x55,
    0xD5, 0x6F, 0x70, 0x1A, 0xFE, 0x9B, 0xE2, 0xCE
};

static const uint8 test_Crypto_Blake2b256_Abc[32] = {
    0xBD, 0xDD, 0x81, 0x3C, 0x63, 0x42, 0x39, 0x72,
    0x31, 0x71, 0xEF, 0x3F, 0xEE, 0x98, 0x57, 0x9B,
    0x94, 0x96, 0x4E, 0x3B, 0xB1, 0xCB, 0x3E, 0x42,
    0x72, 0x62, 0xC8, 0xC0, 0x68, 0xD5, 0x23, 0x19
};

static const uint8 test_Crypto_Blake2s256_Abc[32] = {
    0x50, 0x8C, 0x5E, 0x8C, 0x32, 0x7C, 0x14, 0xE2,
    0xE1, 0xA7, 0x2B, 0xA3, 0x4E, 0xEB, 0x45, 0x2F,
    0x37, 0x45, 0x8B, 0x20, 0x9E, 0xD6, 0x3A, 0x29,
    0x4D, 0x99, 0x9B, 0x4C, 0x86, 0x67, 0x59, 0x82
};

static const uint8 test_Crypto_Blake2s256_Empty[32] = {
    0x69, 0x21, 0x7A, 0x30, 0x79, 0x90, 0x80, 0x94,
    0xE1, 0x11, 0x21, 0xD0, 0x42, 0x35, 0x4A, 0x7C,
    0x1F, 0x55, 0xB6, 0x48, 0x2C, 0xA1, 0xA5, 0x1E,
    0x1B, 0x25, 0x0D, 0xFD, 0x1E, 0xD0, 0xEE, 0xF9
};

static const uint8 test_Crypto_Blake2s128_Abc[16] = {
    0xAA, 0x49, 0x38, 0x11, 0x9B, 0x1D, 0xC7, 0xB8,
    0x7C, 0xBA, 0xD0, 0xFF, 0xD2, 0x00, 0xD0, 0xAE
};

static const uint8 test_Crypto_Abc[3] = { 'a', 'b', 'c' };
static const uint8 test_Crypto_Key8[8] = { 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U };

/*==================================================================================================
 * Init / DeInit / GetVersionInfo
 *==================================================================================================*/

/** @req SWS_Crypto_00001 */
void test_Crypto_Init_BeforeInit_NullConfig_ShouldReportParamPointer(void) {
    Std_ReturnType ret;

    Crypto_Init(NULL_PTR);
    test_Crypto_AssertDet(CRYPTO_SID_INIT, CRYPTO_E_PARAM_POINTER);
    /* Driver must still be uninitialized (proven via a core API). */
    ret = Crypto_KeyValidSet(0U, TRUE);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT(CRYPTO_SID_KEYVALIDSET, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL_UINT(CRYPTO_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Crypto_00001 */
void test_Crypto_Init_BeforeInit_MbedtlsFailure_ShouldReportNotSupportedAndStayUninit(void) {
    Std_ReturnType ret;

    test_Crypto_MbedtlsInitResult = E_NOT_OK;
    Crypto_Init(&test_Crypto_Config);
    test_Crypto_AssertDet(CRYPTO_SID_MBEDTLS_INIT, CRYPTO_E_NOT_SUPPORTED);
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_MbedtlsInitCount);

    /* Driver stays uninitialized... */
    ret = Crypto_KeyValidSet(0U, TRUE);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT(CRYPTO_E_UNINIT, Det_MockData.ErrorId);

    /* ...and a retry with a working backend recovers. */
    Det_Mock_Reset();
    test_Crypto_MbedtlsInitCount = 0U;
    test_Crypto_MbedtlsInitResult = E_OK;
    Crypto_Init(&test_Crypto_Config);
    test_Crypto_AssertNoDet();
    ret = Crypto_KeyValidSet(0U, TRUE);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_Crypto_00001 */
void test_Crypto_Init_ValidConfig_ShouldInitializeDriver(void) {
    Std_ReturnType ret;

    Crypto_Init(&test_Crypto_Config);
    test_Crypto_AssertNoDet();
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_MbedtlsInitCount);

    /* A successful init makes core APIs work without DET reports. */
    ret = Crypto_KeyValidSet(0U, TRUE);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
}

/** @req SWS_Crypto_00001 */
void test_Crypto_Init_DoubleInit_ShouldReportAlreadyInitialized(void) {
    Std_ReturnType ret;

    Crypto_Init(&test_Crypto_Config);
    Det_Mock_Reset();
    test_Crypto_MbedtlsInitCount = 0U;

    Crypto_Init(&test_Crypto_Config);
    test_Crypto_AssertDet(CRYPTO_SID_INIT, CRYPTO_E_ALREADY_INITIALIZED);
    /* Rejected before the backend was touched again. */
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsInitCount);

    /* Driver remains functional after the rejected second init. */
    ret = Crypto_KeyValidSet(0U, TRUE);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_Crypto_00002 */
void test_Crypto_DeInit_BeforeInit_ShouldBeSilent(void) {
    Crypto_DeInit();
    test_Crypto_AssertNoDet();
    /* Backend DeInit must not run for an uninitialized driver. */
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsDeInitCount);
}

/** @req SWS_Crypto_00002 */
void test_Crypto_DeInit_AfterInit_ShouldReturnToUninitAndAllowReinit(void) {
    Std_ReturnType ret;

    Crypto_Init(&test_Crypto_Config);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsDeInitCount);

    Crypto_DeInit();
    test_Crypto_AssertNoDet(); /* DeInit never reports on success */
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_MbedtlsDeInitCount);

    /* Back to UNINIT... */
    ret = Crypto_KeyValidSet(0U, TRUE);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT(CRYPTO_E_UNINIT, Det_MockData.ErrorId);

    /* ...and a full re-init cycle works. */
    Det_Mock_Reset();
    test_Crypto_MbedtlsInitCount = 0U;
    Crypto_Init(&test_Crypto_Config);
    test_Crypto_AssertNoDet();
    ret = Crypto_KeyValidSet(0U, TRUE);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_Crypto_00003 */
void test_Crypto_GetVersionInfo_NullPtr_ShouldReportParamPointer(void) {
    Crypto_GetVersionInfo(NULL_PTR);
    /* Quirk: the DET report uses CRYPTO_SID_DEINIT (0x01) instead of a
     * dedicated version-info SID - asserted here to document source behavior. */
    test_Crypto_AssertDet(CRYPTO_SID_DEINIT, CRYPTO_E_PARAM_POINTER);
}

/** @req SWS_Crypto_00003 */
void test_Crypto_GetVersionInfo_ValidPtr_ShouldFillVersionInfo(void) {
    Std_VersionInfoType version;

    memset(&version, 0, sizeof(version));
    Crypto_GetVersionInfo(&version);
    test_Crypto_AssertNoDet();
    TEST_ASSERT_EQUAL_UINT(CRYPTO_VENDOR_ID, version.vendorID);
    TEST_ASSERT_EQUAL_UINT(CRYPTO_MODULE_ID, version.moduleID);
    TEST_ASSERT_EQUAL_UINT(CRYPTO_SW_MAJOR_VERSION, version.sw_major_version);
    TEST_ASSERT_EQUAL_UINT(CRYPTO_SW_MINOR_VERSION, version.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT(CRYPTO_SW_PATCH_VERSION, version.sw_patch_version);
}

/*==================================================================================================
 * ProcessJob / job queue / CancelJob / MainFunction
 *==================================================================================================*/

/** @req SWS_Crypto_00004 */
void test_Crypto_ProcessJob_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret;

    test_Crypto_MakeJob(CRYPTO_PROCESSING_SYNC, 0U);
    ret = Crypto_ProcessJob(0U, &test_Crypto_Job);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_PROCESSJOB, CRYPTO_E_UNINIT);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsProcessJobCount);
}

/** @req SWS_Crypto_00004 */
void test_Crypto_ProcessJob_NullJob_ShouldReportParamPointer(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    ret = Crypto_ProcessJob(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_PROCESSJOB, CRYPTO_E_PARAM_POINTER);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsProcessJobCount);
}

/** @req SWS_Crypto_00004 */
void test_Crypto_ProcessJob_InvalidObjectId_ShouldReportParamHandle(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    test_Crypto_MakeJob(CRYPTO_PROCESSING_SYNC, 0U);
    ret = Crypto_ProcessJob(99U, &test_Crypto_Job); /* numChannels == 4 */
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    /* ProcessJob forwards objectId as the DET instanceId. */
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL_UINT(CRYPTO_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL_UINT(99U, Det_MockData.InstanceId);
    TEST_ASSERT_EQUAL_UINT(CRYPTO_SID_PROCESSJOB, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL_UINT(CRYPTO_E_PARAM_HANDLE, Det_MockData.ErrorId);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsProcessJobCount);
}

/** @req SWS_Crypto_00004 */
void test_Crypto_ProcessJob_MissingJobPrimitiveInfo_ShouldReturnNotOkWithoutDet(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    test_Crypto_MakeJob(CRYPTO_PROCESSING_SYNC, 0U);
    test_Crypto_Job.jobPrimitiveInfo = NULL_PTR;
    ret = Crypto_ProcessJob(0U, &test_Crypto_Job);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertNoDet(); /* structural job validation is silent */
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsProcessJobCount);
}

/** @req SWS_Crypto_00004 */
void test_Crypto_ProcessJob_MissingJobPrimitiveInputOutput_ShouldReturnNotOkWithoutDet(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    test_Crypto_MakeJob(CRYPTO_PROCESSING_SYNC, 0U);
    test_Crypto_Job.jobPrimitiveInputOutput = NULL_PTR;
    ret = Crypto_ProcessJob(0U, &test_Crypto_Job);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertNoDet();
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsProcessJobCount);
}

/** @req SWS_Crypto_00004 */
void test_Crypto_ProcessJob_SyncSuccess_ShouldProcessAndNotify(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    test_Crypto_MakeJob(CRYPTO_PROCESSING_SYNC, 5U);
    ret = Crypto_ProcessJob(0U, &test_Crypto_Job);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_MbedtlsProcessJobCount);
    test_Crypto_AssertSamePtr(&test_Crypto_Job, test_Crypto_MbedtlsProcessJobLastJob);
    /* Job completes synchronously: state returns to IDLE... */
    TEST_ASSERT_EQUAL_UINT(CRYPTO_JOBSTATE_IDLE, test_Crypto_Job.jobState);
    /* ...and the registered callback fires with the job and OK result. */
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_NotifCount);
    test_Crypto_AssertSamePtr(&test_Crypto_Job, test_Crypto_NotifLastJob);
    TEST_ASSERT_EQUAL(CRYPTO_RESULT_OK, test_Crypto_NotifLastResult);
}

/** @req SWS_Crypto_00004 */
void test_Crypto_ProcessJob_SyncFailure_ShouldReturnNotOkQuietly(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    test_Crypto_MakeJob(CRYPTO_PROCESSING_SYNC, 5U);
    test_Crypto_MbedtlsProcessJobResult = E_NOT_OK;
    ret = Crypto_ProcessJob(0U, &test_Crypto_Job);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertNoDet(); /* backend failure is not a DET event */
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_MbedtlsProcessJobCount);
    /* Quirk: even on failure the job state is reset to IDLE and the
     * completion callback is suppressed. */
    TEST_ASSERT_EQUAL_UINT(CRYPTO_JOBSTATE_IDLE, test_Crypto_Job.jobState);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_NotifCount);
}

/** @req SWS_Crypto_00004 */
void test_Crypto_ProcessJob_AsyncSuccess_ShouldQueueJob(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    test_Crypto_MakeJob(CRYPTO_PROCESSING_ASYNC, 0U);
    ret = Crypto_ProcessJob(0U, &test_Crypto_Job);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    /* Queued, not processed: the backend has not seen the job yet. */
    TEST_ASSERT_EQUAL_UINT(CRYPTO_JOBSTATE_QUEUED, test_Crypto_Job.jobState);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsProcessJobCount);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_NotifCount);
}

/** @req SWS_Crypto_00004 */
void test_Crypto_ProcessJob_AsyncQueueFull_ShouldReportQueueFull(void) {
    Std_ReturnType ret;
    Crypto_JobType jobA;
    Crypto_JobType jobB;

    test_Crypto_EnsureInit();
    test_Crypto_MakeJob(CRYPTO_PROCESSING_ASYNC, 0U);
    jobA = test_Crypto_Job;
    test_Crypto_MakeJob(CRYPTO_PROCESSING_ASYNC, 0U);
    jobB = test_Crypto_Job;

    TEST_ASSERT_EQUAL(E_OK, Crypto_ProcessJob(0U, &jobA));
    TEST_ASSERT_EQUAL(E_OK, Crypto_ProcessJob(0U, &jobB)); /* queueSize == 2 */

    test_Crypto_MakeJob(CRYPTO_PROCESSING_ASYNC, 0U);
    ret = Crypto_ProcessJob(0U, &test_Crypto_Job);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_PROCESSJOB, CRYPTO_E_QUEUE_FULL);
    /* The rejected job was never queued. */
    TEST_ASSERT_EQUAL_UINT(CRYPTO_JOBSTATE_IDLE, test_Crypto_Job.jobState);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsProcessJobCount);
}

/** @req SWS_Crypto_00031 */
void test_Crypto_MainFunction_BeforeInit_ShouldBeSilent(void) {
    Crypto_MainFunction();
    test_Crypto_AssertNoDet();
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsProcessJobCount);
}

/** @req SWS_Crypto_00031 */
void test_Crypto_MainFunction_WithQueuedJob_ShouldNotProcessStubQueue(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    test_Crypto_MakeJob(CRYPTO_PROCESSING_ASYNC, 0U);
    ret = Crypto_ProcessJob(0U, &test_Crypto_Job);
    TEST_ASSERT_EQUAL(E_OK, ret);

    Crypto_MainFunction();
    test_Crypto_AssertNoDet();
    /* Source quirk: Crypto_QueuePush never links the job into the queue list
     * and Crypto_QueuePop always returns NULL_PTR, so MainFunction's pop
     * loop exits immediately without processing anything. */
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsProcessJobCount);
    TEST_ASSERT_EQUAL_UINT(CRYPTO_JOBSTATE_QUEUED, test_Crypto_Job.jobState);
}

/** @req SWS_Crypto_00005 */
void test_Crypto_CancelJob_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret;

    ret = Crypto_CancelJob(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_CANCELJOB, CRYPTO_E_UNINIT);
}

/** @req SWS_Crypto_00005 */
void test_Crypto_CancelJob_NullJob_ShouldReportParamPointer(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    ret = Crypto_CancelJob(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_CANCELJOB, CRYPTO_E_PARAM_POINTER);
}

/** @req SWS_Crypto_00005 */
void test_Crypto_CancelJob_WithCallback_ShouldMarkCanceledAndNotify(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    test_Crypto_MakeJob(CRYPTO_PROCESSING_SYNC, 5U);
    ret = Crypto_CancelJob(0U, &test_Crypto_Job);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    /* Quirk: the job state is set to the error code value
     * CRYPTO_E_JOB_CANCELED (0x0A), not CRYPTO_JOBSTATE_CANCELED (5). */
    TEST_ASSERT_EQUAL_UINT(CRYPTO_E_JOB_CANCELED, test_Crypto_Job.jobState);
    /* The registered callback is invoked with the cancellation code. */
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_NotifCount);
    test_Crypto_AssertSamePtr(&test_Crypto_Job, test_Crypto_NotifLastJob);
    TEST_ASSERT_EQUAL_UINT(CRYPTO_E_JOB_CANCELED, (uint32)test_Crypto_NotifLastResult);
}

/** @req SWS_Crypto_00005 */
void test_Crypto_CancelJob_WithoutCallback_ShouldMarkCanceledSilently(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    test_Crypto_MakeJob(CRYPTO_PROCESSING_SYNC, 0U); /* callbackId == 0 */
    ret = Crypto_CancelJob(0U, &test_Crypto_Job);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    TEST_ASSERT_EQUAL_UINT(CRYPTO_E_JOB_CANCELED, test_Crypto_Job.jobState);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_NotifCount);
}

/*==================================================================================================
 * Key element management
 *==================================================================================================*/

/** @req SWS_Crypto_00006 */
void test_Crypto_KeyElementSet_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret;
    const uint8 data[4] = { 1U, 2U, 3U, 4U };

    ret = Crypto_KeyElementSet(0U, TEST_CRYPTO_KEY0_ELEM0_ID, data, 4U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_KEYELEMENTSET, CRYPTO_E_UNINIT);
    /* Nothing stored. */
    TEST_ASSERT_EQUAL_UINT8(0U, test_Crypto_Key0Elem0Data[0]);
}

/** @req SWS_Crypto_00006 */
void test_Crypto_KeyElementSet_NullKeyPtr_ShouldReportParamPointer(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    ret = Crypto_KeyElementSet(0U, TEST_CRYPTO_KEY0_ELEM0_ID, NULL_PTR, 4U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_KEYELEMENTSET, CRYPTO_E_PARAM_POINTER);
}

/** @req SWS_Crypto_00006 */
void test_Crypto_KeyElementSet_ZeroLength_ShouldReportParamValue(void) {
    Std_ReturnType ret;
    const uint8 data[4] = { 1U, 2U, 3U, 4U };

    test_Crypto_EnsureInit();
    ret = Crypto_KeyElementSet(0U, TEST_CRYPTO_KEY0_ELEM0_ID, data, 0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_KEYELEMENTSET, CRYPTO_E_PARAM_VALUE);
}

/** @req SWS_Crypto_00006 */
void test_Crypto_KeyElementSet_InvalidKeyId_ShouldReturnNotOkSilently(void) {
    Std_ReturnType ret;
    const uint8 data[4] = { 1U, 2U, 3U, 4U };

    test_Crypto_EnsureInit();
    ret = Crypto_KeyElementSet(5U, TEST_CRYPTO_KEY0_ELEM0_ID, data, 4U); /* numKeys == 2 */
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertNoDet(); /* out-of-range key IDs are silent */
}

/** @req SWS_Crypto_00006 */
void test_Crypto_KeyElementSet_UnknownElementId_ShouldReturnNotOkSilently(void) {
    Std_ReturnType ret;
    const uint8 data[4] = { 1U, 2U, 3U, 4U };

    test_Crypto_EnsureInit();
    ret = Crypto_KeyElementSet(0U, 99U, data, 4U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertNoDet();
}

/** @req SWS_Crypto_00006 */
void test_Crypto_KeyElementSet_OversizeData_ShouldReportSmallBuffer(void) {
    Std_ReturnType ret;
    const uint8 data[33] = {
        0xA1U, 0xA1U, 0xA1U, 0xA1U, 0xA1U, 0xA1U, 0xA1U, 0xA1U,
        0xA1U, 0xA1U, 0xA1U, 0xA1U, 0xA1U, 0xA1U, 0xA1U, 0xA1U,
        0xA1U, 0xA1U, 0xA1U, 0xA1U, 0xA1U, 0xA1U, 0xA1U, 0xA1U,
        0xA1U, 0xA1U, 0xA1U, 0xA1U, 0xA1U, 0xA1U, 0xA1U, 0xA1U,
        0xA1U
    };

    test_Crypto_EnsureInit();
    /* Element 10 of key 0 is 32 bytes and does not allow partial access. */
    ret = Crypto_KeyElementSet(0U, TEST_CRYPTO_KEY0_ELEM0_ID, data, 33U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_KEYELEMENTSET, CRYPTO_E_SMALL_BUFFER);
    /* Nothing stored. */
    TEST_ASSERT_EQUAL_UINT8(0U, test_Crypto_Key0Elem0Data[0]);
}

/** @req SWS_Crypto_00006 */
void test_Crypto_KeyElementSet_ValidData_ShouldStoreBytes(void) {
    Std_ReturnType ret;
    const uint8 data[32] = {
        0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U, 0x88U,
        0x99U, 0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU, 0x00U,
        0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U,
        0x09U, 0x0AU, 0x0BU, 0x0CU, 0x0DU, 0x0EU, 0x0FU, 0x10U
    };

    test_Crypto_EnsureInit();
    ret = Crypto_KeyElementSet(0U, TEST_CRYPTO_KEY0_ELEM0_ID, data, 32U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    test_Crypto_AssertBytes(test_Crypto_Key0Elem0Data, data, 32U);
}

/** @req SWS_Crypto_00006 */
void test_Crypto_KeyElementSet_PartialAccessElement_ShouldStoreProvidedLength(void) {
    Std_ReturnType ret;
    uint8 out[16];
    uint32 outLen;
    const uint8 data[8] = { 0xCAU, 0xFEU, 0xBEU, 0xEFU, 0x12U, 0x34U, 0x56U, 0x78U };

    test_Crypto_EnsureInit();
    /* Element 7 (salt) of key 0 is 16 bytes but allows partial access, so a
     * short write is accepted (no SMALL_BUFFER DET). */
    ret = Crypto_KeyElementSet(0U, TEST_CRYPTO_KEY0_ELEM1_ID, data, 8U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    test_Crypto_AssertBytes(test_Crypto_Key0Elem1Data, data, 8U);
    TEST_ASSERT_EQUAL_UINT8(0U, test_Crypto_Key0Elem1Data[8]); /* tail untouched */

    /* Get always returns the full element size. */
    outLen = 16U;
    memset(out, 0, sizeof(out));
    ret = Crypto_KeyElementGet(0U, TEST_CRYPTO_KEY0_ELEM1_ID, out, &outLen);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(16U, outLen);
    test_Crypto_AssertBytes(out, data, 8U);
    TEST_ASSERT_EQUAL_UINT8(0U, out[8]);
}

/** @req SWS_Crypto_00007 */
void test_Crypto_KeyElementGet_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret;
    uint8 out[32];
    uint32 outLen = 32U;

    ret = Crypto_KeyElementGet(0U, TEST_CRYPTO_KEY0_ELEM0_ID, out, &outLen);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_KEYELEMENTGET, CRYPTO_E_UNINIT);
}

/** @req SWS_Crypto_00007 */
void test_Crypto_KeyElementGet_NullPointers_ShouldReportParamPointer(void) {
    Std_ReturnType ret;
    uint8 out[32];
    uint32 outLen = 32U;

    test_Crypto_EnsureInit();
    ret = Crypto_KeyElementGet(0U, TEST_CRYPTO_KEY0_ELEM0_ID, NULL_PTR, &outLen);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_KEYELEMENTGET, CRYPTO_E_PARAM_POINTER);

    Det_Mock_Reset();
    ret = Crypto_KeyElementGet(0U, TEST_CRYPTO_KEY0_ELEM0_ID, out, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_KEYELEMENTGET, CRYPTO_E_PARAM_POINTER);
}

/** @req SWS_Crypto_00007 */
void test_Crypto_KeyElementGet_TooSmallBuffer_ShouldReturnNotOkSilently(void) {
    Std_ReturnType ret;
    uint8 out[32];
    uint32 outLen = 16U; /* element size is 32 */

    test_Crypto_EnsureInit();
    memset(out, 0xEE, sizeof(out));
    ret = Crypto_KeyElementGet(0U, TEST_CRYPTO_KEY0_ELEM0_ID, out, &outLen);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertNoDet();
    /* Output buffer untouched. */
    TEST_ASSERT_EQUAL_UINT8(0xEEU, out[0]);
    TEST_ASSERT_EQUAL_UINT(16U, outLen);
}

/** @req SWS_Crypto_00007 */
void test_Crypto_KeyElementGet_Roundtrip_ShouldReturnStoredBytes(void) {
    Std_ReturnType ret;
    uint8 out[32];
    uint32 outLen;
    const uint8 data[32] = {
        0xDEU, 0xADU, 0xBEU, 0xEFU, 0x00U, 0x11U, 0x22U, 0x33U,
        0x44U, 0x55U, 0x66U, 0x77U, 0x88U, 0x99U, 0xAAU, 0xBBU,
        0xCCU, 0xDDU, 0xEEU, 0xFFU, 0x10U, 0x20U, 0x30U, 0x40U,
        0x50U, 0x60U, 0x70U, 0x80U, 0x90U, 0xA0U, 0xB0U, 0xC0U
    };

    test_Crypto_EnsureInit();
    TEST_ASSERT_EQUAL(E_OK, Crypto_KeyElementSet(1U, TEST_CRYPTO_KEY1_ELEM0_ID, data, 32U));

    outLen = 32U;
    memset(out, 0, sizeof(out));
    ret = Crypto_KeyElementGet(1U, TEST_CRYPTO_KEY1_ELEM0_ID, out, &outLen);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    TEST_ASSERT_EQUAL_UINT(32U, outLen);
    test_Crypto_AssertBytes(out, data, 32U);
}

/** @req SWS_Crypto_00008 */
void test_Crypto_KeyValidSet_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret;

    ret = Crypto_KeyValidSet(0U, TRUE);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_KEYVALIDSET, CRYPTO_E_UNINIT);
    TEST_ASSERT_EQUAL_UINT(CRYPTO_KEY_INVALID, test_Crypto_Keys[0].keyState);
}

/** @req SWS_Crypto_00008 */
void test_Crypto_KeyValidSet_InvalidKeyId_ShouldReturnNotOkSilently(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    ret = Crypto_KeyValidSet(7U, TRUE); /* numKeys == 2 */
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertNoDet();
}

/** @req SWS_Crypto_00008 */
void test_Crypto_KeyValidSet_ShouldUpdateKeyState(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    TEST_ASSERT_EQUAL_UINT(CRYPTO_KEY_INVALID, test_Crypto_Keys[0].keyState);

    ret = Crypto_KeyValidSet(0U, TRUE);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    TEST_ASSERT_EQUAL_UINT(CRYPTO_KEY_VALID, test_Crypto_Keys[0].keyState);

    ret = Crypto_KeyValidSet(0U, FALSE);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(CRYPTO_KEY_INVALID, test_Crypto_Keys[0].keyState);
}

/** @req SWS_Crypto_00009 */
void test_Crypto_KeyElementIdsGet_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret;
    uint32 ids[2];

    ret = Crypto_KeyElementIdsGet(0U, ids);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_KEYELEMENTIDSGET, CRYPTO_E_UNINIT);
}

/** @req SWS_Crypto_00009 */
void test_Crypto_KeyElementIdsGet_NullPointer_ShouldReportParamPointer(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    ret = Crypto_KeyElementIdsGet(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_KEYELEMENTIDSGET, CRYPTO_E_PARAM_POINTER);
}

/** @req SWS_Crypto_00009 */
void test_Crypto_KeyElementIdsGet_ShouldReturnElementIdList(void) {
    Std_ReturnType ret;
    uint32 ids[2];

    test_Crypto_EnsureInit();
    memset(ids, 0, sizeof(ids));
    ret = Crypto_KeyElementIdsGet(0U, ids);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    TEST_ASSERT_EQUAL_UINT(TEST_CRYPTO_KEY0_ELEM0_ID, ids[0]);
    TEST_ASSERT_EQUAL_UINT(TEST_CRYPTO_KEY0_ELEM1_ID, ids[1]);

    memset(ids, 0, sizeof(ids));
    ret = Crypto_KeyElementIdsGet(1U, ids);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(TEST_CRYPTO_KEY1_ELEM0_ID, ids[0]);
}

/*==================================================================================================
 * Key generation / derivation / exchange / random
 *==================================================================================================*/

/** @req SWS_Crypto_00011 */
void test_Crypto_KeyGenerate_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret;

    ret = Crypto_KeyGenerate(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_KEYGENERATE, CRYPTO_E_UNINIT);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsKeyGenerateCount);
}

/** @req SWS_Crypto_00011 */
void test_Crypto_KeyGenerate_InvalidKeyId_ShouldReportParamHandle(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    ret = Crypto_KeyGenerate(9U); /* numKeys == 2 */
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_KEYGENERATE, CRYPTO_E_PARAM_HANDLE);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsKeyGenerateCount);
}

/** @req SWS_Crypto_00011 */
void test_Crypto_KeyGenerate_Valid_ShouldRouteToMbedtls(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit(); /* HSM unavailable by default */
    ret = Crypto_KeyGenerate(1U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_MbedtlsKeyGenerateCount);
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_MbedtlsKeyGenerateLastId);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_HsmLoadKeyCount);
}

/** @req SWS_Crypto_00012 */
void test_Crypto_KeyDerive_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret;

    ret = Crypto_KeyDerive(0U, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_KEYDERIVE, CRYPTO_E_UNINIT);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsKeyDeriveCount);
}

/** @req SWS_Crypto_00012 */
void test_Crypto_KeyDerive_InvalidKeyIds_ShouldReturnNotOkSilently(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    ret = Crypto_KeyDerive(5U, 1U); /* source out of range */
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertNoDet();

    ret = Crypto_KeyDerive(0U, 5U); /* target out of range */
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertNoDet();
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsKeyDeriveCount);
}

/** @req SWS_Crypto_00012 */
void test_Crypto_KeyDerive_Valid_ShouldRouteToMbedtls(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    ret = Crypto_KeyDerive(0U, 1U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_MbedtlsKeyDeriveCount);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsKeyDeriveLastSrc);
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_MbedtlsKeyDeriveLastDst);
}

/** @req SWS_Crypto_00013 */
void test_Crypto_KeyExchangeCalcSecret_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret;

    ret = Crypto_KeyExchangeCalcSecret(0U, test_Crypto_Abc, 3U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_KEYEXCHSYNCCALCSECRET, CRYPTO_E_UNINIT);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsEcdhCount);
}

/** @req SWS_Crypto_00013 */
void test_Crypto_KeyExchangeCalcSecret_NullPartnerKey_ShouldReportParamPointer(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    ret = Crypto_KeyExchangeCalcSecret(0U, NULL_PTR, 3U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_KEYEXCHSYNCCALCSECRET, CRYPTO_E_PARAM_POINTER);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsEcdhCount);
}

/** @req SWS_Crypto_00013 */
void test_Crypto_KeyExchangeCalcSecret_Valid_ShouldRouteToMbedtls(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    ret = Crypto_KeyExchangeCalcSecret(1U, test_Crypto_Abc, 3U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_MbedtlsEcdhCount);
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_MbedtlsEcdhLastKeyId);
    test_Crypto_AssertSamePtr(test_Crypto_Abc, test_Crypto_MbedtlsEcdhLastPtr);
    TEST_ASSERT_EQUAL_UINT(3U, test_Crypto_MbedtlsEcdhLastLen);
}

/** @req SWS_Crypto_00014 */
void test_Crypto_RandomGenerate_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret;
    uint8 out[8];

    ret = Crypto_RandomGenerate(0U, out, 8U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_RANDOMGENERATE, CRYPTO_E_UNINIT);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsRandomCount);
}

/** @req SWS_Crypto_00014 */
void test_Crypto_RandomGenerate_NullResult_ShouldReportParamPointer(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    ret = Crypto_RandomGenerate(0U, NULL_PTR, 8U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_RANDOMGENERATE, CRYPTO_E_PARAM_POINTER);
}

/** @req SWS_Crypto_00014 */
void test_Crypto_RandomGenerate_ZeroLength_ShouldReportParamValue(void) {
    Std_ReturnType ret;
    uint8 out[8];

    test_Crypto_EnsureInit();
    ret = Crypto_RandomGenerate(0U, out, 0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_RANDOMGENERATE, CRYPTO_E_PARAM_VALUE);
}

/** @req SWS_Crypto_00014 */
void test_Crypto_RandomGenerate_InvalidKeyId_ShouldReturnNotOkSilently(void) {
    Std_ReturnType ret;
    uint8 out[8];

    test_Crypto_EnsureInit();
    memset(out, 0, sizeof(out));
    ret = Crypto_RandomGenerate(9U, out, 8U); /* numKeys == 2 */
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertNoDet();
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsRandomCount);
    TEST_ASSERT_EQUAL_UINT8(0U, out[0]); /* untouched */
}

/** @req SWS_Crypto_00014 */
void test_Crypto_RandomGenerate_Valid_ShouldFillBufferFromBackend(void) {
    Std_ReturnType ret;
    uint8 out[8];
    uint32 i;

    test_Crypto_EnsureInit();
    memset(out, 0, sizeof(out));
    ret = Crypto_RandomGenerate(0U, out, 8U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_MbedtlsRandomCount);
    test_Crypto_AssertSamePtr(out, test_Crypto_MbedtlsRandomLastPtr);
    TEST_ASSERT_EQUAL_UINT(8U, test_Crypto_MbedtlsRandomLastLen);
    for (i = 0U; i < 8U; i++)
    {
        TEST_ASSERT_EQUAL_UINT8((uint8)(0x5CU ^ (uint8)i), out[i]);
    }
}

/** @req SWS_Crypto_00015 */
void test_Crypto_RandomSeed_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret;

    ret = Crypto_RandomSeed(0U, test_Crypto_Abc, 3U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_RANDOMGENERATE, CRYPTO_E_UNINIT);
}

/** @req SWS_Crypto_00015 */
void test_Crypto_RandomSeed_NullEntropy_ShouldReportParamPointer(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    ret = Crypto_RandomSeed(0U, NULL_PTR, 3U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_RANDOMGENERATE, CRYPTO_E_PARAM_POINTER);
}

/** @req SWS_Crypto_00015 */
void test_Crypto_RandomSeed_Valid_ShouldReturnOk(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    ret = Crypto_RandomSeed(0U, test_Crypto_Abc, 3U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
}

/*==================================================================================================
 * HSM wrappers
 *==================================================================================================*/

/** @req SWS_Crypto_00018 */
void test_Crypto_HsmLoadKey_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret;

    ret = Crypto_HsmLoadKey(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(CRYPTO_SID_HSM_INIT, CRYPTO_E_UNINIT);
}

/** @req SWS_Crypto_00016 */
void test_Crypto_Hsm_WhenUnavailable_ShouldReportUninitState(void) {
    Std_ReturnType ret;
    uint8 idBuf[4];
    uint32 idLen = 4U;

    test_Crypto_EnsureInit(); /* HSM stub init fails -> HSM unavailable */

    TEST_ASSERT_EQUAL(FALSE, Crypto_HsmIsAvailable());
    /* Without HSM the status is CRYPTO_HSM_UNINIT and the backend is not
     * consulted for the state. */
    TEST_ASSERT_EQUAL_UINT(CRYPTO_HSM_UNINIT, Crypto_HsmGetStatus());
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_HsmGetStateCount);

    ret = Crypto_HsmLoadKey(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertNoDet();
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_HsmLoadKeyCount);

    ret = Crypto_HsmSelfTest();
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);

    ret = Crypto_HsmGetId(idBuf, &idLen);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_Crypto_00016 */
void test_Crypto_Hsm_WhenAvailable_ShouldRouteOperationsToHsm(void) {
    Std_ReturnType ret;

    /* Enable the HSM backend for this test's init. */
    test_Crypto_HsmInitResult = E_OK;
    test_Crypto_HsmAvailable = TRUE;
    test_Crypto_EnsureInit();

    TEST_ASSERT_EQUAL(TRUE, Crypto_HsmIsAvailable());
    /* Init probed the HSM with the configured hsmConfig pointer... */
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_HsmInitCount);
    test_Crypto_AssertSamePtr(&test_Crypto_Config.hsmConfig, test_Crypto_HsmInitLastConfig);
    /* ...and ran the self test once during init. */
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_HsmSelfTestCount);

    /* Status now comes from the backend. */
    TEST_ASSERT_EQUAL_UINT(CRYPTO_HSM_IDLE, Crypto_HsmGetStatus());
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_HsmGetStateCount);

    /* Key loading routes to the HSM. */
    ret = Crypto_HsmLoadKey(1U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_HsmLoadKeyCount);
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_HsmLoadKeyLastId);

    /* Key generation prefers the HSM over the MbedTLS fallback. */
    ret = Crypto_KeyGenerate(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(2U, test_Crypto_HsmLoadKeyCount);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsKeyGenerateCount);

    /* Synchronous jobs are also routed to the HSM. */
    test_Crypto_MakeJob(CRYPTO_PROCESSING_SYNC, 5U);
    ret = Crypto_ProcessJob(0U, &test_Crypto_Job);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_HsmProcessJobCount);
    test_Crypto_AssertSamePtr(&test_Crypto_Job, test_Crypto_HsmProcessJobLastJob);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsProcessJobCount);
    TEST_ASSERT_EQUAL_UINT(CRYPTO_JOBSTATE_IDLE, test_Crypto_Job.jobState);
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_NotifCount);
}

/*==================================================================================================
 * BLAKE2 one-shot hashing (real third_party implementation, RFC 7693 vectors)
 *==================================================================================================*/

/** @req SWS_Crypto_00021 */
void test_Crypto_Blake2b_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret;
    uint8 digest[64];

    memset(digest, 0, sizeof(digest));
    ret = Crypto_Blake2b(test_Crypto_Abc, 3U, NULL_PTR, 0U, 64U, digest);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA0U, CRYPTO_E_UNINIT);
    TEST_ASSERT_EQUAL_UINT8(0U, digest[0]);
}

/** @req SWS_Crypto_00021 */
void test_Crypto_Blake2b_InvalidParameters_ShouldReportDet(void) {
    Std_ReturnType ret;
    uint8 digest[64];

    test_Crypto_EnsureInit();

    ret = Crypto_Blake2b(test_Crypto_Abc, 3U, NULL_PTR, 0U, 64U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA0U, CRYPTO_E_PARAM_POINTER);

    Det_Mock_Reset();
    ret = Crypto_Blake2b(test_Crypto_Abc, 3U, NULL_PTR, 0U, 0U, digest);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA0U, CRYPTO_E_PARAM_VALUE);

    Det_Mock_Reset();
    ret = Crypto_Blake2b(test_Crypto_Abc, 3U, NULL_PTR, 0U, 65U, digest);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA0U, CRYPTO_E_PARAM_VALUE);

    Det_Mock_Reset();
    ret = Crypto_Blake2b(NULL_PTR, 3U, NULL_PTR, 0U, 64U, digest);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA0U, CRYPTO_E_PARAM_POINTER);

    Det_Mock_Reset();
    ret = Crypto_Blake2b(test_Crypto_Abc, 3U, NULL_PTR, 8U, 64U, digest);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA0U, CRYPTO_E_PARAM_POINTER);

    Det_Mock_Reset();
    ret = Crypto_Blake2b(test_Crypto_Abc, 3U, test_Crypto_Key8, 65U, 64U, digest);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA0U, CRYPTO_E_PARAM_VALUE);
}

/** @req SWS_Crypto_00021 */
void test_Crypto_Blake2b_Abc_ShouldMatchRfc7693Vector(void) {
    Std_ReturnType ret;
    uint8 digest[64];

    test_Crypto_EnsureInit();
    memset(digest, 0, sizeof(digest));
    ret = Crypto_Blake2b(test_Crypto_Abc, 3U, NULL_PTR, 0U, 64U, digest);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    test_Crypto_AssertBytes(digest, test_Crypto_Blake2b512_Abc, 64U);
}

/** @req SWS_Crypto_00021 */
void test_Crypto_Blake2b_EmptyInput_ShouldMatchEmptyVector(void) {
    Std_ReturnType ret;
    uint8 digest[64];

    test_Crypto_EnsureInit();
    memset(digest, 0, sizeof(digest));
    /* dataPtr == NULL with dataLength == 0 is a valid empty input. */
    ret = Crypto_Blake2b(NULL_PTR, 0U, NULL_PTR, 0U, 64U, digest);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    test_Crypto_AssertBytes(digest, test_Crypto_Blake2b512_Empty, 64U);
}

/** @req SWS_Crypto_00021 */
void test_Crypto_Blake2b_Keyed_ShouldMatchKeyedVector(void) {
    Std_ReturnType ret;
    uint8 digest[64];

    test_Crypto_EnsureInit();
    memset(digest, 0, sizeof(digest));
    ret = Crypto_Blake2b(test_Crypto_Abc, 3U, test_Crypto_Key8, 8U, 64U, digest);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    test_Crypto_AssertBytes(digest, test_Crypto_Blake2b512_AbcKeyed, 64U);
}

/** @req SWS_Crypto_00021 */
void test_Crypto_Blake2b_TruncatedDigest32_ShouldMatchVector(void) {
    Std_ReturnType ret;
    uint8 digest[32];

    test_Crypto_EnsureInit();
    memset(digest, 0, sizeof(digest));
    ret = Crypto_Blake2b(test_Crypto_Abc, 3U, NULL_PTR, 0U, 32U, digest);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    test_Crypto_AssertBytes(digest, test_Crypto_Blake2b256_Abc, 32U);
}

/** @req SWS_Crypto_00022 */
void test_Crypto_Blake2s_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret;
    uint8 digest[32];

    memset(digest, 0, sizeof(digest));
    ret = Crypto_Blake2s(test_Crypto_Abc, 3U, NULL_PTR, 0U, 32U, digest);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA1U, CRYPTO_E_UNINIT);
    TEST_ASSERT_EQUAL_UINT8(0U, digest[0]);
}

/** @req SWS_Crypto_00022 */
void test_Crypto_Blake2s_InvalidParameters_ShouldReportDet(void) {
    Std_ReturnType ret;
    uint8 digest[32];

    test_Crypto_EnsureInit();

    ret = Crypto_Blake2s(test_Crypto_Abc, 3U, NULL_PTR, 0U, 32U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA1U, CRYPTO_E_PARAM_POINTER);

    Det_Mock_Reset();
    ret = Crypto_Blake2s(test_Crypto_Abc, 3U, NULL_PTR, 0U, 0U, digest);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA1U, CRYPTO_E_PARAM_VALUE);

    Det_Mock_Reset();
    ret = Crypto_Blake2s(test_Crypto_Abc, 3U, NULL_PTR, 0U, 33U, digest);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA1U, CRYPTO_E_PARAM_VALUE);

    Det_Mock_Reset();
    ret = Crypto_Blake2s(NULL_PTR, 3U, NULL_PTR, 0U, 32U, digest);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA1U, CRYPTO_E_PARAM_POINTER);

    Det_Mock_Reset();
    ret = Crypto_Blake2s(test_Crypto_Abc, 3U, NULL_PTR, 8U, 32U, digest);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA1U, CRYPTO_E_PARAM_POINTER);

    Det_Mock_Reset();
    ret = Crypto_Blake2s(test_Crypto_Abc, 3U, test_Crypto_Key8, 33U, 32U, digest);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA1U, CRYPTO_E_PARAM_VALUE);
}

/** @req SWS_Crypto_00022 */
void test_Crypto_Blake2s_Abc_ShouldMatchRfc7693Vector(void) {
    Std_ReturnType ret;
    uint8 digest[32];

    test_Crypto_EnsureInit();
    memset(digest, 0, sizeof(digest));
    ret = Crypto_Blake2s(test_Crypto_Abc, 3U, NULL_PTR, 0U, 32U, digest);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    test_Crypto_AssertBytes(digest, test_Crypto_Blake2s256_Abc, 32U);
}

/** @req SWS_Crypto_00022 */
void test_Crypto_Blake2s_EmptyInput_ShouldMatchEmptyVector(void) {
    Std_ReturnType ret;
    uint8 digest[32];

    test_Crypto_EnsureInit();
    memset(digest, 0, sizeof(digest));
    ret = Crypto_Blake2s(NULL_PTR, 0U, NULL_PTR, 0U, 32U, digest);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    test_Crypto_AssertBytes(digest, test_Crypto_Blake2s256_Empty, 32U);
}

/** @req SWS_Crypto_00022 */
void test_Crypto_Blake2s_TruncatedDigest16_ShouldMatchVector(void) {
    Std_ReturnType ret;
    uint8 digest[16];

    test_Crypto_EnsureInit();
    memset(digest, 0, sizeof(digest));
    ret = Crypto_Blake2s(test_Crypto_Abc, 3U, NULL_PTR, 0U, 16U, digest);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    test_Crypto_AssertBytes(digest, test_Crypto_Blake2s128_Abc, 16U);
}

/*==================================================================================================
 * BLAKE2b incremental hashing state machine
 *==================================================================================================*/

/** @req SWS_Crypto_00023 */
void test_Crypto_Blake2bStart_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret;

    ret = Crypto_Blake2b_Start(1U, NULL_PTR, 0U, 64U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA2U, CRYPTO_E_UNINIT);
}

/** @req SWS_Crypto_00023 */
void test_Crypto_Blake2bStart_InvalidParameters_ShouldReportDet(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();

    ret = Crypto_Blake2b_Start(1U, NULL_PTR, 0U, 0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA2U, CRYPTO_E_PARAM_VALUE);

    Det_Mock_Reset();
    ret = Crypto_Blake2b_Start(1U, NULL_PTR, 0U, 65U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA2U, CRYPTO_E_PARAM_VALUE);

    Det_Mock_Reset();
    ret = Crypto_Blake2b_Start(1U, NULL_PTR, 8U, 64U); /* key length without key */
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA2U, CRYPTO_E_PARAM_POINTER);

    Det_Mock_Reset();
    ret = Crypto_Blake2b_Start(1U, test_Crypto_Key8, 65U, 64U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA2U, CRYPTO_E_PARAM_VALUE);
}

/** @req SWS_Crypto_00024 */
void test_Crypto_Blake2bUpdate_WithoutStart_ShouldReportParamState(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    ret = Crypto_Blake2b_Update(1U, test_Crypto_Abc, 3U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA3U, CRYPTO_E_PARAM_STATE);
}

/** @req SWS_Crypto_00025 */
void test_Crypto_Blake2bFinish_WithoutStart_ShouldReportParamState(void) {
    Std_ReturnType ret;
    uint8 digest[64];
    uint32 digestLen = 64U;

    test_Crypto_EnsureInit();
    ret = Crypto_Blake2b_Finish(1U, digest, &digestLen);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA4U, CRYPTO_E_PARAM_STATE);
}

/** @req SWS_Crypto_00024 */
void test_Crypto_Blake2bUpdate_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret;

    ret = Crypto_Blake2b_Update(1U, test_Crypto_Abc, 3U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA3U, CRYPTO_E_UNINIT);
}

/** @req SWS_Crypto_00025 */
void test_Crypto_Blake2bFinish_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret;
    uint8 digest[64];
    uint32 digestLen = 64U;

    ret = Crypto_Blake2b_Finish(1U, digest, &digestLen);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA4U, CRYPTO_E_UNINIT);
}

/** @req SWS_Crypto_00024 */
void test_Crypto_Blake2bUpdate_InvalidParameters_ShouldReportParamPointer(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    TEST_ASSERT_EQUAL(E_OK, Crypto_Blake2b_Start(1U, NULL_PTR, 0U, 64U));

    Det_Mock_Reset();
    ret = Crypto_Blake2b_Update(1U, NULL_PTR, 4U); /* length without data */
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA3U, CRYPTO_E_PARAM_POINTER);
}

/** @req SWS_Crypto_00025 */
void test_Crypto_Blake2bFinish_InvalidParameters_ShouldReportParamPointer(void) {
    Std_ReturnType ret;
    uint8 digest[64];
    uint32 digestLen = 64U;

    test_Crypto_EnsureInit();
    TEST_ASSERT_EQUAL(E_OK, Crypto_Blake2b_Start(1U, NULL_PTR, 0U, 64U));

    Det_Mock_Reset();
    ret = Crypto_Blake2b_Finish(1U, NULL_PTR, &digestLen);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA4U, CRYPTO_E_PARAM_POINTER);

    Det_Mock_Reset();
    ret = Crypto_Blake2b_Finish(1U, digest, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA4U, CRYPTO_E_PARAM_POINTER);
}

/** @req SWS_Crypto_00023 */
void test_Crypto_Blake2bIncremental_MultiUpdate_ShouldMatchOneshot(void) {
    Std_ReturnType ret;
    uint8 digest[64];
    uint32 digestLen;

    test_Crypto_EnsureInit();
    memset(digest, 0, sizeof(digest));

    ret = Crypto_Blake2b_Start(1U, NULL_PTR, 0U, 64U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();

    ret = Crypto_Blake2b_Update(1U, test_Crypto_Abc, 2U); /* "ab" */
    TEST_ASSERT_EQUAL(E_OK, ret);
    ret = Crypto_Blake2b_Update(1U, &test_Crypto_Abc[2], 1U); /* "c" */
    TEST_ASSERT_EQUAL(E_OK, ret);

    digestLen = 64U;
    ret = Crypto_Blake2b_Finish(1U, digest, &digestLen);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    TEST_ASSERT_EQUAL_UINT(64U, digestLen);
    /* Incremental result equals the one-shot RFC 7693 vector. */
    test_Crypto_AssertBytes(digest, test_Crypto_Blake2b512_Abc, 64U);

    /* Finish closed the context: further updates are rejected. */
    Det_Mock_Reset();
    ret = Crypto_Blake2b_Update(1U, test_Crypto_Abc, 3U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xA3U, CRYPTO_E_PARAM_STATE);
}

/** @req SWS_Crypto_00023 */
void test_Crypto_Blake2bIncremental_Keyed_ShouldMatchKeyedOneshot(void) {
    Std_ReturnType ret;
    uint8 digest[64];
    uint32 digestLen;

    test_Crypto_EnsureInit();
    memset(digest, 0, sizeof(digest));

    ret = Crypto_Blake2b_Start(1U, test_Crypto_Key8, 8U, 64U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();

    ret = Crypto_Blake2b_Update(1U, test_Crypto_Abc, 3U);
    TEST_ASSERT_EQUAL(E_OK, ret);

    digestLen = 64U;
    ret = Crypto_Blake2b_Finish(1U, digest, &digestLen);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* Matches the keyed one-shot vector (MAC construction). */
    test_Crypto_AssertBytes(digest, test_Crypto_Blake2b512_AbcKeyed, 64U);
}

/** @req SWS_Crypto_00025 */
void test_Crypto_Blake2bFinish_LengthMismatch_ShouldFailThenSucceedWithMatchingLength(void) {
    Std_ReturnType ret;
    uint8 digest[64];
    uint32 digestLen;

    test_Crypto_EnsureInit();
    memset(digest, 0, sizeof(digest));

    TEST_ASSERT_EQUAL(E_OK, Crypto_Blake2b_Start(1U, NULL_PTR, 0U, 32U));
    TEST_ASSERT_EQUAL(E_OK, Crypto_Blake2b_Update(1U, test_Crypto_Abc, 3U));

    /* Finish with a length different from the started context fails (the
     * underlying blake2b_final rejects outlen != context outlen). */
    digestLen = 64U;
    ret = Crypto_Blake2b_Finish(1U, digest, &digestLen);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertNoDet();

    /* The failed Finish left the context usable: retrying with the matching
     * length produces the correct BLAKE2b-256("abc"). */
    digestLen = 32U;
    ret = Crypto_Blake2b_Finish(1U, digest, &digestLen);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(32U, digestLen);
    test_Crypto_AssertBytes(digest, test_Crypto_Blake2b256_Abc, 32U);
}

/*==================================================================================================
 * CCC Digital Key specific functions
 *==================================================================================================*/

/** @req SWS_Crypto_00026 */
void test_Crypto_CccGenerateAttestation_NullPointers_ShouldReportParamPointer(void) {
    Std_ReturnType ret;
    uint8 sig[72];
    uint32 sigLen = 72U;

    test_Crypto_EnsureInit();

    ret = Crypto_CccGenerateAttestation(NULL_PTR, 3U, sig, &sigLen);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xF0U, CRYPTO_E_PARAM_POINTER);

    Det_Mock_Reset();
    ret = Crypto_CccGenerateAttestation(test_Crypto_Abc, 3U, NULL_PTR, &sigLen);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xF0U, CRYPTO_E_PARAM_POINTER);

    Det_Mock_Reset();
    ret = Crypto_CccGenerateAttestation(test_Crypto_Abc, 3U, sig, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertDet(0xF0U, CRYPTO_E_PARAM_POINTER);
}

/** @req SWS_Crypto_00026 */
void test_Crypto_CccGenerateAttestation_ValidChallenge_ShouldHashThenSign(void) {
    Std_ReturnType ret;
    uint8 sig[72];
    uint32 sigLen;
    uint32 i;

    test_Crypto_EnsureInit();
    memset(sig, 0, sizeof(sig));

    ret = Crypto_CccGenerateAttestation(test_Crypto_Abc, 3U, sig, &sigLen);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();

    /* The challenge was hashed first... */
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_MbedtlsSha256Count);
    test_Crypto_AssertSamePtr(test_Crypto_Abc, test_Crypto_MbedtlsSha256LastPtr);
    TEST_ASSERT_EQUAL_UINT(3U, test_Crypto_MbedtlsSha256LastLen);

    /* ...and the hash output was passed to the signing backend with the
     * configured CCC device attestation key (id 14) and 32-byte digest. */
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_MbedtlsSignCount);
    TEST_ASSERT_EQUAL_UINT(CRYPTO_KEY_ID_CCC_DEVICE_KEY, test_Crypto_MbedtlsSignLastKeyId);
    TEST_ASSERT_EQUAL_UINT(32U, test_Crypto_MbedtlsSignLastDigestLen);
    TEST_ASSERT_EQUAL_UINT8(0xD7U, test_Crypto_MbedtlsSignLastDigest0); /* stub digest pattern */

    /* The signature produced by the backend is returned to the caller. */
    TEST_ASSERT_EQUAL_UINT(64U, sigLen);
    for (i = 0U; i < 64U; i++)
    {
        TEST_ASSERT_EQUAL_UINT8((uint8)(0xE9U ^ (uint8)i), sig[i]);
    }
}

/** @req SWS_Crypto_00027 */
void test_Crypto_CccVerifyOwnerCertificate_ShouldReportFailedVerification(void) {
    Std_ReturnType ret;
    Crypto_VerifyResultType verifyResult;

    test_Crypto_EnsureInit();

    /* Certificate chain verification is not implemented: the result is
     * always a failed verification with E_NOT_OK. */
    verifyResult = CRYPTO_VERIFICATION_PASSED;
    ret = Crypto_CccVerifyOwnerCertificate(test_Crypto_Abc, 3U, &verifyResult);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(CRYPTO_VERIFICATION_FAILED, verifyResult);

    /* A NULL result pointer is tolerated (no dereference, no DET). */
    ret = Crypto_CccVerifyOwnerCertificate(test_Crypto_Abc, 3U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Crypto_AssertNoDet();
}

/** @req SWS_Crypto_00029 */
void test_Crypto_CccEncrypt_Valid_ShouldSetTagLengthAndRouteToGcm(void) {
    Std_ReturnType ret;
    const uint8 pt[8] = { 'C', 'C', 'C', 'd', 'a', 't', 'a', '1' };
    const uint8 aad[4] = { 0xA1U, 0xA2U, 0xA3U, 0xA4U };
    const uint8 iv[12] = { 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U,
                           0x06U, 0x07U, 0x08U, 0x09U, 0x0AU, 0x0BU };
    uint8 ct[8];
    uint8 tag[16];
    uint32 tagLen;
    uint32 i;

    test_Crypto_EnsureInit();
    memset(ct, 0, sizeof(ct));
    memset(tag, 0, sizeof(tag));

    tagLen = 0U;
    ret = Crypto_CccEncrypt(0U, pt, 8U, aad, 4U, iv, 12U, ct, tag, &tagLen);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();

    /* The CCC wrapper publishes the configured tag size (16 bytes). */
    TEST_ASSERT_EQUAL_UINT(CRYPTO_CFG_CCC_TAG_SIZE, tagLen);

    /* All parameters were forwarded to the AES-GCM backend. */
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_MbedtlsGcmEncCount);
    TEST_ASSERT_EQUAL_UINT(0U, test_Crypto_MbedtlsGcmEncLastKeyId);
    test_Crypto_AssertSamePtr(pt, test_Crypto_MbedtlsGcmEncLastPt);
    TEST_ASSERT_EQUAL_UINT(8U, test_Crypto_MbedtlsGcmEncLastPtLen);
    test_Crypto_AssertSamePtr(aad, test_Crypto_MbedtlsGcmEncLastAad);
    TEST_ASSERT_EQUAL_UINT(4U, test_Crypto_MbedtlsGcmEncLastAadLen);
    test_Crypto_AssertSamePtr(iv, test_Crypto_MbedtlsGcmEncLastIv);

    /* Ciphertext and tag come from the backend. */
    for (i = 0U; i < 8U; i++)
    {
        TEST_ASSERT_EQUAL_UINT8((uint8)(0x3CU ^ (uint8)i), ct[i]);
    }
    for (i = 0U; i < 16U; i++)
    {
        TEST_ASSERT_EQUAL_UINT8(0x7DU, tag[i]);
    }

    /* A NULL tagLengthPtr is tolerated (length simply not reported). */
    ret = Crypto_CccEncrypt(0U, pt, 8U, aad, 4U, iv, 12U, ct, tag, NULL_PTR);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_Crypto_00030 */
void test_Crypto_CccDecrypt_Valid_ShouldSetPlaintextLengthAndRouteToGcm(void) {
    Std_ReturnType ret;
    const uint8 ct[8] = { 0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U, 0x88U };
    const uint8 tag[16] = { 0x7DU };
    const uint8 iv[12] = { 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U,
                           0x06U, 0x07U, 0x08U, 0x09U, 0x0AU, 0x0BU };
    uint8 pt[8];
    uint32 ptLen;
    uint32 i;

    test_Crypto_EnsureInit();
    memset(pt, 0, sizeof(pt));

    ptLen = 0U;
    ret = Crypto_CccDecrypt(1U, ct, 8U, NULL_PTR, 0U, iv, 12U, tag, 16U, pt, &ptLen);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();

    /* The plaintext length mirrors the ciphertext length. */
    TEST_ASSERT_EQUAL_UINT(8U, ptLen);

    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_MbedtlsGcmDecCount);
    test_Crypto_AssertSamePtr(ct, test_Crypto_MbedtlsGcmDecLastCt);
    TEST_ASSERT_EQUAL_UINT(8U, test_Crypto_MbedtlsGcmDecLastCtLen);

    for (i = 0U; i < 8U; i++)
    {
        TEST_ASSERT_EQUAL_UINT8((uint8)(0xB4U ^ (uint8)i), pt[i]);
    }
}

/** @req SWS_Crypto_00028 */
void test_Crypto_CccDeriveSessionKey_ShouldRouteToEcdh(void) {
    Std_ReturnType ret;

    test_Crypto_EnsureInit();
    ret = Crypto_CccDeriveSessionKey(test_Crypto_Abc, 3U, 1U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    /* Routed to ECDH with the configured ephemeral key slot (id 16). */
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_MbedtlsEcdhCount);
    TEST_ASSERT_EQUAL_UINT(CRYPTO_KEY_ID_EPHEMERAL, test_Crypto_MbedtlsEcdhLastKeyId);
    test_Crypto_AssertSamePtr(test_Crypto_Abc, test_Crypto_MbedtlsEcdhLastPtr);
    TEST_ASSERT_EQUAL_UINT(3U, test_Crypto_MbedtlsEcdhLastLen);
}

/** @req SWS_Crypto_00029 */
void test_Crypto_Ccc_NoInitCheck_Quirk_ShouldRouteWithoutInitialization(void) {
    Std_ReturnType ret;
    const uint8 pt[4] = { 0x01U, 0x02U, 0x03U, 0x04U };
    const uint8 iv[12] = { 0U };
    uint8 ct[4];
    uint8 tag[16];
    uint32 tagLen = 0U;

    /* Source quirk: unlike the core APIs, the CCC helpers perform no driver
     * UNINIT validation - Crypto_CccEncrypt forwards directly to the backend
     * even before Crypto_Init. */
    memset(ct, 0, sizeof(ct));
    memset(tag, 0, sizeof(tag));
    ret = Crypto_CccEncrypt(0U, pt, 4U, NULL_PTR, 0U, iv, 12U, ct, tag, &tagLen);
    TEST_ASSERT_EQUAL(E_OK, ret);
    test_Crypto_AssertNoDet();
    TEST_ASSERT_EQUAL_UINT(1U, test_Crypto_MbedtlsGcmEncCount);
    TEST_ASSERT_EQUAL_UINT(CRYPTO_CFG_CCC_TAG_SIZE, tagLen);
}
