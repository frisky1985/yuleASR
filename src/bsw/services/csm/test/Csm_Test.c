/*==================================================================================================
 *                                CRYPTO SERVICES MANAGER (Csm)
 *==================================================================================================
 * FILENAME: Csm_Test.c
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Unit tests for Crypto Services Manager module
 *==================================================================================================
 */

#include "unity.h"
#include "Csm.h"
#include "Csm_Cfg.h"
#include "Det.h"
#include "mock_Det.h"
#include "mock_SchM_Csm.h"

/*==================================================================================================
 *                                    TEST SETUP
 *==================================================================================================*/
void setUp(void)
{
    /* Reset module state before each test */
    Csm_DeInit();
}

void tearDown(void)
{
    /* Cleanup after each test */
    Csm_DeInit();
}

/*==================================================================================================
 *                                    TEST CONFIGURATION
 *==================================================================================================*/
static const Csm_JobConfigType TestJobConfigs[CSM_NUM_JOBS] = {
    { CSM_JOB_ID_ENCRYPT_1, CSM_CRYPTO_PRIMITIVE_ENCRYPT, CSM_ALGOFAM_AES, CSM_ALGOMODE_CBC, CSM_KEY_ID_AES_128, CSM_JOB_PRIORITY_HIGH, TRUE },
    { CSM_JOB_ID_DECRYPT_1, CSM_CRYPTO_PRIMITIVE_DECRYPT, CSM_ALGOFAM_AES, CSM_ALGOMODE_CBC, CSM_KEY_ID_AES_128, CSM_JOB_PRIORITY_HIGH, TRUE },
    { CSM_JOB_ID_MAC_GENERATE_1, CSM_CRYPTO_PRIMITIVE_MAC_GENERATE, CSM_ALGOFAM_SHA2_256, CSM_ALGOMODE_ECB, CSM_KEY_ID_HMAC_SHA256, CSM_JOB_PRIORITY_NORMAL, TRUE },
    { CSM_JOB_ID_MAC_VERIFY_1, CSM_CRYPTO_PRIMITIVE_MAC_VERIFY, CSM_ALGOFAM_SHA2_256, CSM_ALGOMODE_ECB, CSM_KEY_ID_HMAC_SHA256, CSM_JOB_PRIORITY_NORMAL, TRUE },
    { CSM_JOB_ID_HASH_SHA256, CSM_CRYPTO_PRIMITIVE_HASH, CSM_ALGOFAM_SHA2_256, CSM_ALGOMODE_ECB, CSM_INVALID_KEY_ID, CSM_JOB_PRIORITY_NORMAL, FALSE },
    { CSM_JOB_ID_HASH_SHA512, CSM_CRYPTO_PRIMITIVE_HASH, CSM_ALGOFAM_SHA2_512, CSM_ALGOMODE_ECB, CSM_INVALID_KEY_ID, CSM_JOB_PRIORITY_LOW, FALSE },
    { CSM_JOB_ID_RANDOM_GENERATE, CSM_CRYPTO_PRIMITIVE_RANDOM_GENERATE, CSM_ALGOFAM_AES, CSM_ALGOMODE_ECB, CSM_INVALID_KEY_ID, CSM_JOB_PRIORITY_HIGH, FALSE },
    { CSM_JOB_ID_SIGNATURE_VERIFY, CSM_CRYPTO_PRIMITIVE_SIGNATURE_VERIFY, CSM_ALGOFAM_RSA, CSM_ALGOMODE_ECB, CSM_KEY_ID_RSA_PUBLIC, CSM_JOB_PRIORITY_NORMAL, TRUE }
};

static const Csm_KeyConfigType TestKeyConfigs[CSM_NUM_KEYS] = {
    { CSM_KEY_ID_AES_128, CSM_KEY_LENGTH_AES_128, FALSE },
    { CSM_KEY_ID_AES_256, CSM_KEY_LENGTH_AES_256, FALSE },
    { CSM_KEY_ID_HMAC_SHA256, CSM_KEY_LENGTH_HMAC_SHA256, FALSE },
    { CSM_KEY_ID_RSA_PUBLIC, CSM_KEY_LENGTH_RSA_2048, FALSE },
    { CSM_KEY_ID_RSA_PRIVATE, CSM_KEY_LENGTH_RSA_2048, FALSE },
    { CSM_KEY_ID_ECC_PUBLIC, CSM_KEY_LENGTH_ECC_P256, FALSE },
    { CSM_KEY_ID_ECC_PRIVATE, CSM_KEY_LENGTH_ECC_P256, FALSE },
    { 7u, 128u, FALSE }
};

static const Csm_ConfigType TestConfig = {
    TestJobConfigs,
    CSM_NUM_JOBS,
    TestKeyConfigs,
    CSM_NUM_KEYS,
    CSM_JOB_QUEUE_SIZE,
    (CSM_CALLBACK_SUPPORTED == STD_ON),
    (CSM_RETRY_FAILED_JOBS == STD_ON)
};

/*==================================================================================================
 *                                    TEST CASES - Init/DeInit
 *==================================================================================================*/
void test_Csm_Init_ShouldInitializeModule(void)
{
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0_Expect();
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0_Expect();
    
    Csm_Init(&TestConfig);
    
    TEST_ASSERT_TRUE(Csm_Initialized_Global);
}

void test_Csm_Init_ShouldReportError_WhenAlreadyInitialized(void)
{
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    
    Csm_Init(&TestConfig);
    
    Det_ReportError_ExpectAndReturn(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_INIT, CSM_E_ALREADY_INITIALIZED, E_OK);
    
    Csm_Init(&TestConfig);
}

void test_Csm_Init_ShouldReportError_WhenConfigNull(void)
{
    Det_ReportError_ExpectAndReturn(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_INIT, CSM_E_PARAM_POINTER, E_OK);
    
    Csm_Init(NULL);
    
    TEST_ASSERT_FALSE(Csm_Initialized_Global);
}

void test_Csm_DeInit_ShouldDeinitializeModule(void)
{
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    
    Csm_Init(&TestConfig);
    Csm_DeInit();
    
    TEST_ASSERT_FALSE(Csm_Initialized_Global);
}

void test_Csm_DeInit_ShouldReportError_WhenNotInitialized(void)
{
    Det_ReportError_ExpectAndReturn(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_DEINIT, CSM_E_UNINIT, E_OK);
    
    Csm_DeInit();
}

/*==================================================================================================
 *                                    TEST CASES - Encrypt/Decrypt
 *==================================================================================================*/
void test_Csm_Encrypt_ShouldEncryptData(void)
{
    uint8 input[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                       0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    uint8 output[16];
    uint32 outputLength = 16;
    Std_ReturnType result;
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    
    Csm_Init(&TestConfig);
    
    /* Set up key first */
    uint8 key[16] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                     0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};
    Csm_KeyElementSet(CSM_KEY_ID_AES_128, 0, key, 16);
    Csm_KeySetValid(CSM_KEY_ID_AES_128);
    
    result = Csm_Encrypt(CSM_JOB_ID_ENCRYPT_1, CSM_OPERATIONMODE_STREAMSTART,
                          input, 16, output, &outputLength);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(16, outputLength);
}

void test_Csm_Encrypt_ShouldReportError_WhenNotInitialized(void)
{
    uint8 input[16];
    uint8 output[16];
    uint32 outputLength = 16;
    
    Det_ReportError_ExpectAndReturn(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_ENCRYPT, CSM_E_UNINIT, E_OK);
    
    Csm_Encrypt(CSM_JOB_ID_ENCRYPT_1, CSM_OPERATIONMODE_STREAMSTART,
                input, 16, output, &outputLength);
}

void test_Csm_Encrypt_ShouldReportError_WhenNullPointer(void)
{
    uint8 input[16];
    uint8 output[16];
    uint32 outputLength = 16;
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    
    Csm_Init(&TestConfig);
    
    Det_ReportError_ExpectAndReturn(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_ENCRYPT, CSM_E_PARAM_POINTER, E_OK);
    
    Csm_Encrypt(CSM_JOB_ID_ENCRYPT_1, CSM_OPERATIONMODE_STREAMSTART,
                NULL, 16, output, &outputLength);
}

void test_Csm_Decrypt_ShouldDecryptData(void)
{
    uint8 encrypted[16] = {0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E,
                           0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E};
    uint8 decrypted[16];
    uint32 outputLength = 16;
    Std_ReturnType result;
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    
    Csm_Init(&TestConfig);
    
    /* Set up key */
    uint8 key[16] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                     0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};
    Csm_KeyElementSet(CSM_KEY_ID_AES_128, 0, key, 16);
    Csm_KeySetValid(CSM_KEY_ID_AES_128);
    
    result = Csm_Decrypt(CSM_JOB_ID_DECRYPT_1, CSM_OPERATIONMODE_STREAMSTART,
                          encrypted, 16, decrypted, &outputLength);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(16, outputLength);
}

/*==================================================================================================
 *                                    TEST CASES - MAC
 *==================================================================================================*/
void test_Csm_MacGenerate_ShouldGenerateMac(void)
{
    uint8 data[32] = "Hello, World! This is test data.";
    uint8 mac[32];
    uint32 macLength = 32;
    Std_ReturnType result;
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    
    Csm_Init(&TestConfig);
    
    result = Csm_MacGenerate(CSM_JOB_ID_MAC_GENERATE_1, CSM_OPERATIONMODE_STREAMSTART,
                              data, 32, mac, &macLength);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(CSM_HMAC_SIZE, macLength);
}

void test_Csm_MacVerify_ShouldVerifyMac(void)
{
    uint8 data[32] = "Hello, World! This is test data.";
    uint8 mac[32];
    uint32 macLength = 32;
    Csm_VerifyResultType verifyResult;
    Std_ReturnType result;
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    
    Csm_Init(&TestConfig);
    
    /* Generate MAC first */
    Csm_MacGenerate(CSM_JOB_ID_MAC_GENERATE_1, CSM_OPERATIONMODE_STREAMSTART,
                    data, 32, mac, &macLength);
    
    /* Verify MAC */
    result = Csm_MacVerify(CSM_JOB_ID_MAC_VERIFY_1, CSM_OPERATIONMODE_STREAMSTART,
                           data, 32, mac, macLength, &verifyResult);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(CSM_E_VER_OK, verifyResult);
}

void test_Csm_MacVerify_ShouldFailWithWrongMac(void)
{
    uint8 data[32] = "Hello, World! This is test data.";
    uint8 wrongMac[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    Csm_VerifyResultType verifyResult;
    Std_ReturnType result;
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    
    Csm_Init(&TestConfig);
    
    /* Verify with wrong MAC */
    result = Csm_MacVerify(CSM_JOB_ID_MAC_VERIFY_1, CSM_OPERATIONMODE_STREAMSTART,
                           data, 32, wrongMac, 32, &verifyResult);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(CSM_E_VER_NOT_OK, verifyResult);
}

/*==================================================================================================
 *                                    TEST CASES - Hash
 *==================================================================================================*/
void test_Csm_Hash_ShouldCalculateHash(void)
{
    uint8 data[64] = "The quick brown fox jumps over the lazy dog";
    uint8 hash[32];
    uint32 hashLength = 32;
    Std_ReturnType result;
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    
    Csm_Init(&TestConfig);
    
    result = Csm_Hash(CSM_JOB_ID_HASH_SHA256, CSM_OPERATIONMODE_STREAMSTART,
                      data, 43, hash, &hashLength);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(CSM_HASH_SHA256_SIZE, hashLength);
}

void test_Csm_Hash_ShouldReportError_WhenNullPointer(void)
{
    uint8 data[16];
    uint8 hash[32];
    uint32 hashLength = 32;
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    
    Csm_Init(&TestConfig);
    
    Det_ReportError_ExpectAndReturn(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_HASH, CSM_E_PARAM_POINTER, E_OK);
    
    Csm_Hash(CSM_JOB_ID_HASH_SHA256, CSM_OPERATIONMODE_STREAMSTART,
             NULL, 16, hash, &hashLength);
}

/*==================================================================================================
 *                                    TEST CASES - Random
 *==================================================================================================*/
void test_Csm_RandomGenerate_ShouldGenerateRandom(void)
{
    uint8 random1[16];
    uint8 random2[16];
    Std_ReturnType result1, result2;
    boolean different = FALSE;
    uint8 i;
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    
    Csm_Init(&TestConfig);
    
    result1 = Csm_RandomGenerate(CSM_JOB_ID_RANDOM_GENERATE, random1, 16);
    result2 = Csm_RandomGenerate(CSM_JOB_ID_RANDOM_GENERATE, random2, 16);
    
    TEST_ASSERT_EQUAL(E_OK, result1);
    TEST_ASSERT_EQUAL(E_OK, result2);
    
    /* Check that values are different (highly probable) */
    for (i = 0; i < 16; i++) {
        if (random1[i] != random2[i]) {
            different = TRUE;
            break;
        }
    }
    TEST_ASSERT_TRUE(different);
}

void test_Csm_RandomGenerate_ShouldReportError_WhenTooLarge(void)
{
    uint8 random[300];
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    
    Csm_Init(&TestConfig);
    
    Det_ReportError_ExpectAndReturn(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_RANDOMGENERATE, CSM_E_PARAM_LENGTH, E_OK);
    
    Csm_RandomGenerate(CSM_JOB_ID_RANDOM_GENERATE, random, 300);
}

/*==================================================================================================
 *                                    TEST CASES - Key Management
 *==================================================================================================*/
void test_Csm_KeyElementSet_ShouldSetKey(void)
{
    uint8 key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                     0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    Std_ReturnType result;
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    
    Csm_Init(&TestConfig);
    
    result = Csm_KeyElementSet(CSM_KEY_ID_AES_128, 0, key, 16);
    
    TEST_ASSERT_EQUAL(E_OK, result);
}

void test_Csm_KeySetValid_ShouldValidateKey(void)
{
    uint8 key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                     0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    Std_ReturnType result;
    uint8 retrievedKey[16];
    uint32 keyLength = 16;
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    
    Csm_Init(&TestConfig);
    
    Csm_KeyElementSet(CSM_KEY_ID_AES_128, 0, key, 16);
    result = Csm_KeySetValid(CSM_KEY_ID_AES_128);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* Should be able to retrieve key now */
    result = Csm_KeyElementGet(CSM_KEY_ID_AES_128, 0, retrievedKey, &keyLength);
    TEST_ASSERT_EQUAL(E_OK, result);
}

void test_Csm_KeyElementGet_ShouldFail_WhenKeyNotValid(void)
{
    uint8 key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                     0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    uint8 retrievedKey[16];
    uint32 keyLength = 16;
    Std_ReturnType result;
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    
    Csm_Init(&TestConfig);
    
    /* Set key but don't validate */
    Csm_KeyElementSet(CSM_KEY_ID_AES_128, 0, key, 16);
    
    /* Try to get key - should fail */
    result = Csm_KeyElementGet(CSM_KEY_ID_AES_128, 0, retrievedKey, &keyLength);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/*==================================================================================================
 *                                    TEST CASES - Cancel Job
 *==================================================================================================*/
void test_Csm_CancelJob_ShouldCancelJob(void)
{
    Std_ReturnType result;
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    
    Csm_Init(&TestConfig);
    
    result = Csm_CancelJob(CSM_JOB_ID_ENCRYPT_1);
    
    TEST_ASSERT_EQUAL(E_OK, result);
}

void test_Csm_CancelJob_ShouldReportError_WhenNotInitialized(void)
{
    Det_ReportError_ExpectAndReturn(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_CANCELJOB, CSM_E_UNINIT, E_OK);
    
    Csm_CancelJob(CSM_JOB_ID_ENCRYPT_1);
}

/*==================================================================================================
 *                                    TEST CASES - Version Info
 *==================================================================================================*/
#if (CSM_VERSION_INFO_API == STD_ON)
void test_Csm_GetVersionInfo_ShouldReturnVersion(void)
{
    Std_VersionInfoType versionInfo;
    
    Csm_GetVersionInfo(&versionInfo);
    
    TEST_ASSERT_EQUAL(CSM_VENDOR_ID, versionInfo.vendorID);
    TEST_ASSERT_EQUAL(CSM_MODULE_ID, versionInfo.moduleID);
    TEST_ASSERT_EQUAL(CSM_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    TEST_ASSERT_EQUAL(CSM_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    TEST_ASSERT_EQUAL(CSM_SW_PATCH_VERSION, versionInfo.sw_patch_version);
}

void test_Csm_GetVersionInfo_ShouldReportError_WhenNullPointer(void)
{
    Det_ReportError_ExpectAndReturn(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_GETVERSIONINFO, CSM_E_PARAM_POINTER, E_OK);
    
    Csm_GetVersionInfo(NULL);
}
#endif

/*==================================================================================================
 *                                    TEST CASES - Main Function
 *==================================================================================================*/
void test_Csm_MainFunction_ShouldProcessJobs(void)
{
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0_Ignore();
    
    Csm_Init(&TestConfig);
    
    Csm_MainFunction();
    
    /* Main function should complete without error */
    TEST_ASSERT_TRUE(TRUE);
}

void test_Csm_MainFunction_ShouldReportError_WhenNotInitialized(void)
{
    Det_ReportError_ExpectAndReturn(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_MAINFUNCTION, CSM_E_UNINIT, E_OK);
    
    Csm_MainFunction();
}

/*==================================================================================================
 *                                    TEST RUNNER
 *==================================================================================================*/
int main(void)
{
    UNITY_BEGIN();
    
    /* Init/DeInit tests */
    RUN_TEST(test_Csm_Init_ShouldInitializeModule);
    RUN_TEST(test_Csm_Init_ShouldReportError_WhenAlreadyInitialized);
    RUN_TEST(test_Csm_Init_ShouldReportError_WhenConfigNull);
    RUN_TEST(test_Csm_DeInit_ShouldDeinitializeModule);
    RUN_TEST(test_Csm_DeInit_ShouldReportError_WhenNotInitialized);
    
    /* Encrypt/Decrypt tests */
    RUN_TEST(test_Csm_Encrypt_ShouldEncryptData);
    RUN_TEST(test_Csm_Encrypt_ShouldReportError_WhenNotInitialized);
    RUN_TEST(test_Csm_Encrypt_ShouldReportError_WhenNullPointer);
    RUN_TEST(test_Csm_Decrypt_ShouldDecryptData);
    
    /* MAC tests */
    RUN_TEST(test_Csm_MacGenerate_ShouldGenerateMac);
    RUN_TEST(test_Csm_MacVerify_ShouldVerifyMac);
    RUN_TEST(test_Csm_MacVerify_ShouldFailWithWrongMac);
    
    /* Hash tests */
    RUN_TEST(test_Csm_Hash_ShouldCalculateHash);
    RUN_TEST(test_Csm_Hash_ShouldReportError_WhenNullPointer);
    
    /* Random tests */
    RUN_TEST(test_Csm_RandomGenerate_ShouldGenerateRandom);
    RUN_TEST(test_Csm_RandomGenerate_ShouldReportError_WhenTooLarge);
    
    /* Key management tests */
    RUN_TEST(test_Csm_KeyElementSet_ShouldSetKey);
    RUN_TEST(test_Csm_KeySetValid_ShouldValidateKey);
    RUN_TEST(test_Csm_KeyElementGet_ShouldFail_WhenKeyNotValid);
    
    /* Cancel job tests */
    RUN_TEST(test_Csm_CancelJob_ShouldCancelJob);
    RUN_TEST(test_Csm_CancelJob_ShouldReportError_WhenNotInitialized);
    
    /* Version info tests */
#if (CSM_VERSION_INFO_API == STD_ON)
    RUN_TEST(test_Csm_GetVersionInfo_ShouldReturnVersion);
    RUN_TEST(test_Csm_GetVersionInfo_ShouldReportError_WhenNullPointer);
#endif
    
    /* Main function tests */
    RUN_TEST(test_Csm_MainFunction_ShouldProcessJobs);
    RUN_TEST(test_Csm_MainFunction_ShouldReportError_WhenNotInitialized);
    
    return UNITY_END();
}
