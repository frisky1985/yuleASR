/**********************************************************************************************************************
 * @file       test_crypto_hsm.c
 * @brief      Host unit tests for the HSM crypto backend (Crypto_Hsm.c)
 * @author     YuleTech AutoSAR Team
 * @date       2026-11-08
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 *      White-box host tests.  The production source is included directly so
 *      that the internal stubs (UnloadKey / GenerateRandom / SecureBootVerify)
 *      can be exercised as well.
 *
 *      Crypto_Hsm.c compiles two flavours: without HSM_HARDWARE_PRESENT the
 *      driver always reports "HSM not available"; with it the state machine
 *      reaches READY.  Both are covered: this file defines HSM_HARDWARE_PRESENT
 *      to test the operational paths, and the uninit/guard paths remain
 *      reachable through DeInit.
 *********************************************************************************************************************/

#define HSM_HARDWARE_PRESENT    (1)

#include "../src/Crypto_Hsm.c"

#include <string.h>

#include "unity.h"

/**********************************************************************************************************************
 * UNITY HOOKS
 *********************************************************************************************************************/

void setUp(void)
{
    Crypto_Hsm_DeInit();
}

void tearDown(void)
{
    Crypto_Hsm_DeInit();
}

/**********************************************************************************************************************
 * INIT / STATUS
 *********************************************************************************************************************/

static void test_hsm_init_deinit(void)
{
    Crypto_HsmConfigType config;

    (void)memset(&config, 0, sizeof(config));

    /* NULL config rejected, state stays uninitialised */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_Hsm_Init(NULL_PTR));
    TEST_ASSERT_EQUAL_INT(CRYPTO_HSM_UNINIT, (int)Crypto_Hsm_GetState());
    TEST_ASSERT_FALSE(Crypto_Hsm_IsAvailable());

    /* Valid config brings HSM to READY/IDLE */
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_Hsm_Init(&config));
    TEST_ASSERT_EQUAL_INT(CRYPTO_HSM_IDLE, (int)Crypto_Hsm_GetState());
    TEST_ASSERT_TRUE(Crypto_Hsm_IsAvailable());

    /* DeInit returns to UNINIT */
    Crypto_Hsm_DeInit();
    TEST_ASSERT_EQUAL_INT(CRYPTO_HSM_UNINIT, (int)Crypto_Hsm_GetState());
    TEST_ASSERT_FALSE(Crypto_Hsm_IsAvailable());

    /* Second DeInit is a no-op */
    Crypto_Hsm_DeInit();
    TEST_ASSERT_EQUAL_INT(CRYPTO_HSM_UNINIT, (int)Crypto_Hsm_GetState());
}

/**********************************************************************************************************************
 * PROCESS JOB
 *********************************************************************************************************************/

static void test_hsm_process_job(void)
{
    static const uint32 services[] = {
        CRYPTO_SERVICE_ENCRYPT,
        CRYPTO_SERVICE_DECRYPT,
        CRYPTO_SERVICE_HASH,
        CRYPTO_SERVICE_MACGENERATE,          /* not handled -> default */
        CRYPTO_SERVICE_SIGN,
        CRYPTO_SERVICE_VERIFY,
        CRYPTO_SERVICE_RANDOMGENERATE,
        CRYPTO_SERVICE_KEYEXCHANGECALCSECRET,
        CRYPTO_SERVICE_KEYDERIVE,            /* not handled -> default */
        0xFFU
    };
    Crypto_HsmConfigType          config;
    Crypto_JobPrimitiveInfoType   info;
    Crypto_JobType                job;
    uint32                        i;

    (void)memset(&config, 0, sizeof(config));
    (void)memset(&info, 0, sizeof(info));
    (void)memset(&job, 0, sizeof(job));
    job.jobPrimitiveInfo = &info;

    /* Uninitialised driver must reject jobs without touching them */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_Hsm_ProcessJob(&job));

    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_Hsm_Init(&config));

    /* NULL job */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_Hsm_ProcessJob(NULL_PTR));

    /* Every service dispatches and leaves the state machine back in READY */
    for (i = 0U; i < (uint32)(sizeof(services) / sizeof(services[0])); i++) {
        info.service = services[i];
        TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_Hsm_ProcessJob(&job));
        TEST_ASSERT_EQUAL_INT(CRYPTO_HSM_IDLE, (int)Crypto_Hsm_GetState());
        TEST_ASSERT_TRUE(Crypto_Hsm_IsAvailable());
    }
}

/**********************************************************************************************************************
 * KEY MANAGEMENT / SELF TEST / RANDOM / SECURE BOOT
 *********************************************************************************************************************/

static void test_hsm_operations_ready(void)
{
    Crypto_HsmConfigType   config;
    uint8                  buffer[8];
    uint8                  hash[4];
    uint8                  signature[8];
    Crypto_VerifyResultType verifyResult = CRYPTO_VERIFICATION_PASSED;

    (void)memset(&config, 0, sizeof(config));
    (void)memset(buffer, 0, sizeof(buffer));
    (void)memset(hash, 0, sizeof(hash));
    (void)memset(signature, 0, sizeof(signature));

    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_Hsm_Init(&config));

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_Hsm_LoadKey(1U));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_Hsm_UnloadKey(1U));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_Hsm_SelfTest());

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_Hsm_GenerateRandom(buffer, 8U));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_Hsm_GenerateRandom(NULL_PTR, 8U));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_Hsm_GenerateRandom(buffer, 0U));

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK,
        Crypto_Hsm_SecureBootVerify(hash, 4U, signature, 8U, &verifyResult));
    TEST_ASSERT_EQUAL_INT(CRYPTO_VERIFICATION_FAILED, (int)verifyResult);

    /* NULL verifyResult must not be dereferenced */
    verifyResult = CRYPTO_VERIFICATION_PASSED;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK,
        Crypto_Hsm_SecureBootVerify(hash, 4U, signature, 8U, NULL_PTR));
    TEST_ASSERT_EQUAL_INT(CRYPTO_VERIFICATION_PASSED, (int)verifyResult);
}

static void test_hsm_operations_uninit(void)
{
    Crypto_JobPrimitiveInfoType info;
    Crypto_JobType              job;
    uint8                       buffer[8];
    uint8                       hash[4];
    uint8                       signature[8];
    Crypto_VerifyResultType     verifyResult = CRYPTO_VERIFICATION_PASSED;

    (void)memset(&info, 0, sizeof(info));
    (void)memset(&job, 0, sizeof(job));
    (void)memset(buffer, 0, sizeof(buffer));
    (void)memset(hash, 0, sizeof(hash));
    (void)memset(signature, 0, sizeof(signature));
    job.jobPrimitiveInfo = &info;

    /* All state-guarded APIs reject access while uninitialised */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_Hsm_LoadKey(0U));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_Hsm_UnloadKey(0U));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_Hsm_SelfTest());
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_Hsm_GenerateRandom(buffer, 8U));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK,
        Crypto_Hsm_SecureBootVerify(hash, 4U, signature, 8U, &verifyResult));
    TEST_ASSERT_EQUAL_INT(CRYPTO_VERIFICATION_PASSED, (int)verifyResult);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_Hsm_ProcessJob(&job));
}

/**********************************************************************************************************************
 * MAIN
 *********************************************************************************************************************/

int main(void)
{
    UnityBegin();

    RUN_TEST(test_hsm_init_deinit);
    RUN_TEST(test_hsm_process_job);
    RUN_TEST(test_hsm_operations_ready);
    RUN_TEST(test_hsm_operations_uninit);

    return UnityEnd();
}

/**********************************************************************************************************************
 * END OF FILE
 *********************************************************************************************************************/
