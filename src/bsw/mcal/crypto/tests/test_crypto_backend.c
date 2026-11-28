/**********************************************************************************************************************
 * @file       test_crypto_backend.c
 * @brief      Host unit tests for the Crypto backend dispatch layer
 * @author     YuleTech AutoSAR Team
 * @date       2026-11-28
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 *      Verifies that the backend vtable selects mbedTLS by default, rejects
 *      the HSE provider when it is not compiled in, supports runtime provider
 *      selection, and that dispatching through the active provider reaches the
 *      real mbedTLS software implementation.
 *********************************************************************************************************************/

#include <string.h>

#include "unity.h"
#include "Crypto_Backend.h"

/**********************************************************************************************************************
 * TEST DOUBLE PROVIDER
 *********************************************************************************************************************/
static boolean FakeInitCalled = FALSE;
static boolean FakeShaCalled = FALSE;

static Std_ReturnType Fake_Init(void)
{
    FakeInitCalled = TRUE;
    return E_OK;
}

static Std_ReturnType Fake_Sha256(const uint8* data, uint32 dataLen, uint8* digest)
{
    (void)data;
    (void)dataLen;
    (void)digest;
    FakeShaCalled = TRUE;
    return E_OK;
}

static const Crypto_BackendOpsType FakeOps = {
    .Init = Fake_Init,
    .Sha256 = Fake_Sha256
};

static const Crypto_BackendProviderType FakeProvider = {
    "fake",
    FALSE,
    &FakeOps
};

/**********************************************************************************************************************
 * TEST RNG STUB
 * Crypto_MbedTLS.c references the public Crypto_RandomGenerate() (normally
 * defined by Crypto.c, which is not linked here).  The backend tests never
 * exercise the key-generation paths, so a minimal deterministic stub suffices.
 *********************************************************************************************************************/
Std_ReturnType Crypto_RandomGenerate(Crypto_KeyIdType cryptoKeyId, uint8* resultPtr, uint32 resultLength)
{
    uint32 i;

    (void)cryptoKeyId;

    if ((resultPtr == NULL_PTR) || (resultLength == 0U))
    {
        return E_NOT_OK;
    }
    for (i = 0U; i < resultLength; i++)
    {
        resultPtr[i] = 0x01U;
    }
    return E_OK;
}

/**********************************************************************************************************************
 * UNITY HOOKS
 *********************************************************************************************************************/
void setUp(void)
{
    (void)Crypto_Backend_Select(NULL_PTR);
    FakeInitCalled = FALSE;
    FakeShaCalled = FALSE;
}

void tearDown(void)
{
    (void)Crypto_Backend_Select(NULL_PTR);
}

/**********************************************************************************************************************
 * TESTS
 *********************************************************************************************************************/
static void test_default_provider_is_software(void)
{
    TEST_ASSERT_TRUE(Crypto_Backend_GetActive() == Crypto_Backend_GetSoftware());
    TEST_ASSERT_EQUAL_STRING("mbedtls-sw", Crypto_Backend_GetActive()->name);
    TEST_ASSERT_FALSE(Crypto_Backend_GetActive()->hsmBacked);
}

static void test_hse_rejected_when_unavailable(void)
{
    TEST_ASSERT_FALSE(Crypto_Backend_IsHseAvailable());
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_Backend_Select(Crypto_Backend_GetHse()));
    TEST_ASSERT_TRUE(Crypto_Backend_GetActive() == Crypto_Backend_GetSoftware());
}

static void test_select_null_restores_software(void)
{
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_Backend_Select(&FakeProvider));
    TEST_ASSERT_TRUE(Crypto_Backend_GetActive() == &FakeProvider);

    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_Backend_Select(NULL_PTR));
    TEST_ASSERT_TRUE(Crypto_Backend_GetActive() == Crypto_Backend_GetSoftware());
}

static void test_dispatch_uses_active_provider(void)
{
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_Backend_Select(&FakeProvider));

    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_Backend_Init());
    TEST_ASSERT_TRUE(FakeInitCalled);

    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_Backend_Sha256(NULL_PTR, 0U, NULL_PTR));
    TEST_ASSERT_TRUE(FakeShaCalled);
}

static void test_software_sha256_dispatch(void)
{
    /* SHA-256("abc") reference digest */
    static const uint8 expected[32] = {
        0xBAU, 0x78U, 0x16U, 0xBFU, 0x8FU, 0x01U, 0xCFU, 0xEAU,
        0x41U, 0x41U, 0x40U, 0xDEU, 0x5DU, 0xAEU, 0x22U, 0x23U,
        0xB0U, 0x03U, 0x61U, 0xA3U, 0x96U, 0x17U, 0x7AU, 0x9CU,
        0xB4U, 0x10U, 0xFFU, 0x61U, 0xF2U, 0x00U, 0x15U, 0xADU
    };
    static const uint8 message[3] = { 'a', 'b', 'c' };
    uint8 digest[32];

    (void)memset(digest, 0, sizeof(digest));

    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_Backend_Init());
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_Backend_Sha256(message, (uint32)sizeof(message), digest));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, digest, (uint32)sizeof(expected));

    Crypto_Backend_DeInit();
}

/**********************************************************************************************************************
 * MAIN
 *********************************************************************************************************************/
int main(void)
{
    UnityBegin();

    RUN_TEST(test_default_provider_is_software);
    RUN_TEST(test_hse_rejected_when_unavailable);
    RUN_TEST(test_select_null_restores_software);
    RUN_TEST(test_dispatch_uses_active_provider);
    RUN_TEST(test_software_sha256_dispatch);

    return UnityEnd();
}

/**********************************************************************************************************************
 * END OF FILE
 *********************************************************************************************************************/
