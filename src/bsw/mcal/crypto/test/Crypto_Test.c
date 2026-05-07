/**=================================================================================================
 * @file Crypto_Test.c
 * @brief Unit tests for Crypto Driver
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *==================================================================================================*/

#include "Crypto.h"
#include <stdio.h>
#include <string.h>

/*==================================================================================================
 *                                    TEST MACROS
 *==================================================================================================*/
#define TEST_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            printf("FAIL: %s:%d - %s\n", __FILE__, __LINE__, #expr); \
            return E_NOT_OK; \
        } \
    } while(0)

#define TEST_PASS(name) \
    do { \
        printf("PASS: %s\n", name); \
    } while(0)

/*==================================================================================================
 *                                    TEST DATA
 *==================================================================================================*/
/* AES-256 test key */
static const uint8 testAesKey256[32] = {
    0x60, 0x3D, 0xEB, 0x10, 0x15, 0xCA, 0x71, 0xBE,
    0x2B, 0x73, 0xAE, 0xF0, 0x85, 0x7D, 0x77, 0x81,
    0x1F, 0x35, 0x2C, 0x07, 0x3B, 0x61, 0x08, 0xD7,
    0x2D, 0x98, 0x10, 0xA3, 0x09, 0x14, 0xDF, 0xF4
};

/* Test IV */
static const uint8 testIv[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

/* Test plaintext (16 bytes for AES block) */
static const uint8 testPlaintext[] = "Hello, World!!!!";

/* HMAC test key */
static const uint8 testHmacKey[32] = {
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b
};

/*==================================================================================================
 *                                    TEST FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Test initialization
 */
static Std_ReturnType Test_Init(void)
{
    printf("\n=== Test: Initialization ===\n");
    
    /* Test initialization */
    Crypto_Init(&Crypto_Config);
    
    TEST_PASS("Initialization");
    return E_OK;
}

/**
 * @brief Test deinitialization
 */
static Std_ReturnType Test_DeInit(void)
{
    printf("\n=== Test: DeInitialization ===\n");
    
    /* Test deinitialization */
    Crypto_DeInit();
    
    TEST_PASS("DeInitialization");
    return E_OK;
}

/**
 * @brief Test version info API
 */
#if (CRYPTO_VERSION_INFO_API == STD_ON)
static Std_ReturnType Test_VersionInfo(void)
{
    Std_VersionInfoType versionInfo;
    
    printf("\n=== Test: Version Info ===\n");
    
    Crypto_GetVersionInfo(&versionInfo);
    
    TEST_ASSERT(versionInfo.vendorID == CRYPTO_VENDOR_ID);
    TEST_ASSERT(versionInfo.moduleID == CRYPTO_MODULE_ID);
    TEST_ASSERT(versionInfo.sw_major_version == CRYPTO_SW_MAJOR_VERSION);
    TEST_ASSERT(versionInfo.sw_minor_version == CRYPTO_SW_MINOR_VERSION);
    
    printf("Vendor ID: 0x%04X\n", versionInfo.vendorID);
    printf("Module ID: 0x%04X\n", versionInfo.moduleID);
    printf("Version: %d.%d.%d\n", versionInfo.sw_major_version,
           versionInfo.sw_minor_version, versionInfo.sw_patch_version);
    
    TEST_PASS("Version Info");
    return E_OK;
}
#endif

/**
 * @brief Test key element set/get
 */
static Std_ReturnType Test_KeyElement(void)
{
    uint8 keyBuffer[32];
    uint32 keyLength = sizeof(keyBuffer);
    Std_ReturnType result;
    
    printf("\n=== Test: Key Element Set/Get ===\n");
    
    /* Set key element */
    result = Crypto_KeyElementSet(CRYPTO_KEY_ID_AES_MASTER,
                                   CRYPTO_KEY_ELEMENT_AES_KEY,
                                   testAesKey256, sizeof(testAesKey256));
    TEST_ASSERT(result == E_OK);
    
    /* Get key element */
    result = Crypto_KeyElementGet(CRYPTO_KEY_ID_AES_MASTER,
                                   CRYPTO_KEY_ELEMENT_AES_KEY,
                                   keyBuffer, &keyLength);
    TEST_ASSERT(result == E_OK);
    TEST_ASSERT(keyLength == sizeof(testAesKey256));
    TEST_ASSERT(memcmp(keyBuffer, testAesKey256, keyLength) == 0);
    
    TEST_PASS("Key Element Set/Get");
    return E_OK;
}

/**
 * @brief Test AES encryption
 */
static Std_ReturnType Test_AesEncrypt(void)
{
    uint8 ciphertext[32];
    uint32 cipherLen = sizeof(ciphertext);
    Std_ReturnType result;
    Crypto_AlgorithmInfoType algo;
    
    printf("\n=== Test: AES-256-CBC Encryption ===\n");
    
    /* Set up key */
    result = Crypto_KeyElementSet(CRYPTO_KEY_ID_AES_MASTER,
                                   CRYPTO_KEY_ELEMENT_AES_KEY,
                                   testAesKey256, sizeof(testAesKey256));
    TEST_ASSERT(result == E_OK);
    result = Crypto_KeyValidSet(CRYPTO_KEY_ID_AES_MASTER);
    TEST_ASSERT(result == E_OK);
    
    /* Configure algorithm */
    algo.family = CRYPTO_ALGOFAM_AES;
    algo.mode = CRYPTO_ALGOMODE_CBC;
    algo.keyLength = 256;
    algo.ivLength = 16;
    algo.authTagLength = 0;
    
    /* Encrypt */
    result = Crypto_HwAesEncrypt(CRYPTO_CHANNEL_AES_0,
                                  CRYPTO_OPERATIONMODE_SINGLECALL,
                                  &algo,
                                  CRYPTO_KEY_ID_AES_MASTER,
                                  testIv,
                                  testPlaintext,
                                  sizeof(testPlaintext) - 1,
                                  ciphertext,
                                  &cipherLen);
    
    TEST_ASSERT(result == E_OK);
    TEST_ASSERT(cipherLen > 0);
    
    printf("Encrypted %lu bytes -> %lu bytes\n",
           (unsigned long)(sizeof(testPlaintext) - 1),
           (unsigned long)cipherLen);
    
    TEST_PASS("AES-256-CBC Encryption");
    return E_OK;
}

/**
 * @brief Test AES decryption
 */
static Std_ReturnType Test_AesDecrypt(void)
{
    uint8 ciphertext[32];
    uint8 plaintext[32];
    uint32 cipherLen = sizeof(ciphertext);
    uint32 plainLen = sizeof(plaintext);
    Std_ReturnType result;
    Crypto_AlgorithmInfoType algo;
    
    printf("\n=== Test: AES-256-CBC Decryption ===\n");
    
    /* Set up key */
    result = Crypto_KeyElementSet(CRYPTO_KEY_ID_AES_MASTER,
                                   CRYPTO_KEY_ELEMENT_AES_KEY,
                                   testAesKey256, sizeof(testAesKey256));
    TEST_ASSERT(result == E_OK);
    result = Crypto_KeyValidSet(CRYPTO_KEY_ID_AES_MASTER);
    TEST_ASSERT(result == E_OK);
    
    /* Configure algorithm */
    algo.family = CRYPTO_ALGOFAM_AES;
    algo.mode = CRYPTO_ALGOMODE_CBC;
    algo.keyLength = 256;
    algo.ivLength = 16;
    algo.authTagLength = 0;
    
    /* First encrypt */
    result = Crypto_HwAesEncrypt(CRYPTO_CHANNEL_AES_0,
                                  CRYPTO_OPERATIONMODE_SINGLECALL,
                                  &algo,
                                  CRYPTO_KEY_ID_AES_MASTER,
                                  testIv,
                                  testPlaintext,
                                  sizeof(testPlaintext) - 1,
                                  ciphertext,
                                  &cipherLen);
    TEST_ASSERT(result == E_OK);
    
    /* Then decrypt */
    result = Crypto_HwAesDecrypt(CRYPTO_CHANNEL_AES_0,
                                  CRYPTO_OPERATIONMODE_SINGLECALL,
                                  &algo,
                                  CRYPTO_KEY_ID_AES_MASTER,
                                  testIv,
                                  ciphertext,
                                  cipherLen,
                                  plaintext,
                                  &plainLen);
    
    TEST_ASSERT(result == E_OK);
    TEST_ASSERT(plainLen == sizeof(testPlaintext) - 1);
    TEST_ASSERT(memcmp(plaintext, testPlaintext, plainLen) == 0);
    
    printf("Decrypted %lu bytes -> %lu bytes\n",
           (unsigned long)cipherLen,
           (unsigned long)plainLen);
    printf("Plaintext: %.*s\n", (int)plainLen, plaintext);
    
    TEST_PASS("AES-256-CBC Decryption");
    return E_OK;
}

/**
 * @brief Test SHA-256 hash
 */
static Std_ReturnType Test_Sha256(void)
{
    const char* testData = "The quick brown fox jumps over the lazy dog";
    uint8 hash[32];
    uint32 hashLen = sizeof(hash);
    Std_ReturnType result;
    
    printf("\n=== Test: SHA-256 Hash ===\n");
    
    result = Crypto_HwHashSha256(CRYPTO_CHANNEL_HASH_0,
                                  CRYPTO_OPERATIONMODE_SINGLECALL,
                                  (const uint8*)testData,
                                  strlen(testData),
                                  hash,
                                  &hashLen);
    
    TEST_ASSERT(result == E_OK);
    TEST_ASSERT(hashLen == CRYPTO_SHA256_SIZE);
    
    printf("Hash result (%lu bytes): ", (unsigned long)hashLen);
    for (uint32 i = 0; i < hashLen; i++) {
        printf("%02X", hash[i]);
    }
    printf("\n");
    
    /* Expected SHA-256: d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592 */
    const uint8 expectedHash[32] = {
        0xd7, 0xa8, 0xfb, 0xb3, 0x07, 0xd7, 0x80, 0x94,
        0x69, 0xca, 0x9a, 0xbc, 0xb0, 0x08, 0x2e, 0x4f,
        0x8d, 0x56, 0x51, 0xe4, 0x6d, 0x3c, 0xdb, 0x76,
        0x2d, 0x02, 0xd0, 0xbf, 0x37, 0xc9, 0xe5, 0x92
    };
    
    TEST_ASSERT(memcmp(hash, expectedHash, 32) == 0);
    
    TEST_PASS("SHA-256 Hash");
    return E_OK;
}

/**
 * @brief Test HMAC generation
 */
static Std_ReturnType Test_HmacGenerate(void)
{
    const char* testData = "Hi There";
    uint8 mac[32];
    uint32 macLen = sizeof(mac);
    Std_ReturnType result;
    
    printf("\n=== Test: HMAC-SHA256 Generation ===\n");
    
    /* Set HMAC key */
    result = Crypto_KeyElementSet(CRYPTO_KEY_ID_HMAC_MASTER,
                                   CRYPTO_KEY_ELEMENT_HMAC_KEY,
                                   testHmacKey, sizeof(testHmacKey));
    TEST_ASSERT(result == E_OK);
    result = Crypto_KeyValidSet(CRYPTO_KEY_ID_HMAC_MASTER);
    TEST_ASSERT(result == E_OK);
    
    result = Crypto_HwHmacGenerate(CRYPTO_CHANNEL_HMAC_0,
                                    CRYPTO_OPERATIONMODE_SINGLECALL,
                                    CRYPTO_KEY_ID_HMAC_MASTER,
                                    (const uint8*)testData,
                                    strlen(testData),
                                    mac,
                                    &macLen);
    
    TEST_ASSERT(result == E_OK);
    TEST_ASSERT(macLen > 0);
    
    printf("HMAC result (%lu bytes): ", (unsigned long)macLen);
    for (uint32 i = 0; i < macLen; i++) {
        printf("%02X", mac[i]);
    }
    printf("\n");
    
    TEST_PASS("HMAC-SHA256 Generation");
    return E_OK;
}

/**
 * @brief Test random number generation
 */
static Std_ReturnType Test_Random(void)
{
    uint8 random1[32];
    uint8 random2[32];
    Std_ReturnType result;
    
    printf("\n=== Test: Random Number Generation ===\n");
    
    result = Crypto_HwRandomGenerate(random1, sizeof(random1));
    TEST_ASSERT(result == E_OK);
    
    result = Crypto_HwRandomGenerate(random2, sizeof(random2));
    TEST_ASSERT(result == E_OK);
    
    /* Two random numbers should be different (with high probability) */
    int diff = memcmp(random1, random2, sizeof(random1));
    printf("Random numbers are %s\n", diff != 0 ? "different" : "SAME (unexpected)");
    
    printf("Random 1: ");
    for (uint32 i = 0; i < sizeof(random1); i++) {
        printf("%02X", random1[i]);
    }
    printf("\n");
    
    printf("Random 2: ");
    for (uint32 i = 0; i < sizeof(random2); i++) {
        printf("%02X", random2[i]);
    }
    printf("\n");
    
    TEST_PASS("Random Number Generation");
    return E_OK;
}

/**
 * @brief Test key generation
 */
static Std_ReturnType Test_KeyGenerate(void)
{
    Std_ReturnType result;
    uint8 keyBuffer[32];
    uint32 keyLength = sizeof(keyBuffer);
    
    printf("\n=== Test: Key Generation ===\n");
    
    /* Generate a new key */
    result = Crypto_KeyGenerate(CRYPTO_KEY_ID_AES_SESSION);
    TEST_ASSERT(result == E_OK);
    
    /* Verify key was generated */
    result = Crypto_KeyElementGet(CRYPTO_KEY_ID_AES_SESSION,
                                   CRYPTO_KEY_ELEMENT_AES_KEY,
                                   keyBuffer, &keyLength);
    TEST_ASSERT(result == E_OK);
    TEST_ASSERT(keyLength == 32);
    
    printf("Generated key (%lu bytes): ", (unsigned long)keyLength);
    for (uint32 i = 0; i < keyLength; i++) {
        printf("%02X", keyBuffer[i]);
    }
    printf("\n");
    
    TEST_PASS("Key Generation");
    return E_OK;
}

/**
 * @brief Test key copy
 */
static Std_ReturnType Test_KeyCopy(void)
{
    Std_ReturnType result;
    uint8 srcKey[32];
    uint8 dstKey[32];
    uint32 srcLen = sizeof(srcKey);
    uint32 dstLen = sizeof(dstKey);
    
    printf("\n=== Test: Key Copy ===\n");
    
    /* Set source key */
    result = Crypto_KeyElementSet(CRYPTO_KEY_ID_AES_MASTER,
                                   CRYPTO_KEY_ELEMENT_AES_KEY,
                                   testAesKey256, sizeof(testAesKey256));
    TEST_ASSERT(result == E_OK);
    
    /* Copy key */
    result = Crypto_KeyCopy(CRYPTO_KEY_ID_AES_MASTER, CRYPTO_KEY_ID_AES_STORAGE);
    TEST_ASSERT(result == E_OK);
    
    /* Verify copy */
    result = Crypto_KeyElementGet(CRYPTO_KEY_ID_AES_MASTER,
                                   CRYPTO_KEY_ELEMENT_AES_KEY,
                                   srcKey, &srcLen);
    TEST_ASSERT(result == E_OK);
    
    result = Crypto_KeyElementGet(CRYPTO_KEY_ID_AES_STORAGE,
                                   CRYPTO_KEY_ELEMENT_AES_KEY,
                                   dstKey, &dstLen);
    TEST_ASSERT(result == E_OK);
    
    TEST_ASSERT(srcLen == dstLen);
    TEST_ASSERT(memcmp(srcKey, dstKey, srcLen) == 0);
    
    TEST_PASS("Key Copy");
    return E_OK;
}

/**
 * @brief Test key derivation
 */
static Std_ReturnType Test_KeyDerive(void)
{
    Std_ReturnType result;
    
    printf("\n=== Test: Key Derivation ===\n");
    
    /* Set base key with salt */
    result = Crypto_KeyElementSet(CRYPTO_KEY_ID_DERIVE_BASE,
                                   CRYPTO_KEY_ELEMENT_AES_KEY,
                                   testAesKey256, sizeof(testAesKey256));
    TEST_ASSERT(result == E_OK);
    
    uint8 salt[32] = {0xAA, 0xBB, 0xCC, 0xDD};
    result = Crypto_KeyElementSet(CRYPTO_KEY_ID_DERIVE_BASE,
                                   CRYPTO_KEY_ELEMENT_SALT,
                                   salt, sizeof(salt));
    TEST_ASSERT(result == E_OK);
    
    /* Derive key */
    result = Crypto_KeyDerive(CRYPTO_KEY_ID_DERIVE_BASE, CRYPTO_KEY_ID_DERIVED_1);
    TEST_ASSERT(result == E_OK);
    
    /* Verify derived key */
    uint8 derivedKey[32];
    uint32 derivedLen = sizeof(derivedKey);
    result = Crypto_KeyElementGet(CRYPTO_KEY_ID_DERIVED_1,
                                   CRYPTO_KEY_ELEMENT_AES_KEY,
                                   derivedKey, &derivedLen);
    TEST_ASSERT(result == E_OK);
    
    printf("Derived key (%lu bytes): ", (unsigned long)derivedLen);
    for (uint32 i = 0; i < derivedLen; i++) {
        printf("%02X", derivedKey[i]);
    }
    printf("\n");
    
    TEST_PASS("Key Derivation");
    return E_OK;
}

/**
 * @brief Test job processing
 */
static Std_ReturnType Test_ProcessJob(void)
{
    Std_ReturnType result;
    
    printf("\n=== Test: Process Job ===\n");
    
    /* Create a simple hash job */
    Crypto_JobPrimitiveInfoType primitiveInfo = {
        .primitive = CRYPTO_OPERATION_HASH,
        .algorithm = {
            .family = CRYPTO_ALGOFAM_SHA2_256,
            .mode = CRYPTO_ALGOMODE_NOT_SET,
            .keyLength = 0,
            .ivLength = 0,
            .authTagLength = 0
        },
        .resultLength = CRYPTO_SHA256_SIZE
    };
    
    const char* testData = "Test data for hash job";
    uint8 output[32];
    uint32 outputLen = sizeof(output);
    
    Crypto_JobPrimitiveInputOutputType io = {
        .inputPtr = (const uint8*)testData,
        .inputLength = strlen(testData),
        .outputPtr = output,
        .outputLengthPtr = &outputLen
    };
    
    Crypto_JobType job = {
        .jobId = 0,
        .channelId = CRYPTO_CHANNEL_HASH_0,
        .jobPrimitiveInputOutput = CRYPTO_OPERATIONMODE_SINGLECALL,
        .jobPrimitiveInputOutputPtr = &io,
        .jobPrimitiveInfo = &primitiveInfo,
        .jobKeyId = NULL_PTR,
        .jobState = CRYPTO_JOBSTATE_IDLE,
        .processingType = CRYPTO_PROCESSING_SYNC
    };
    
    result = Crypto_ProcessJob(CRYPTO_DRIVER_OBJECT_HASH_ID, &job);
    TEST_ASSERT(result == E_OK);
    TEST_ASSERT(job.jobState == CRYPTO_JOBSTATE_COMPLETED);
    
    printf("Job processed successfully, hash length: %lu\n", (unsigned long)outputLen);
    
    TEST_PASS("Process Job");
    return E_OK;
}

/*==================================================================================================
 *                                    MAIN TEST FUNCTION
 *==================================================================================================*/

/**
 * @brief Run all tests
 */
void Crypto_RunAllTests(void)
{
    int passed = 0;
    int failed = 0;
    
    printf("\n");
    printf("========================================\n");
    printf("   Crypto Driver Unit Tests\n");
    printf("========================================\n");
    
    /* Initialize */
    if (Test_Init() == E_OK) passed++; else failed++;
    
    /* Run tests */
#if (CRYPTO_VERSION_INFO_API == STD_ON)
    if (Test_VersionInfo() == E_OK) passed++; else failed++;
#endif
    if (Test_KeyElement() == E_OK) passed++; else failed++;
    if (Test_KeyGenerate() == E_OK) passed++; else failed++;
    if (Test_KeyCopy() == E_OK) passed++; else failed++;
    if (Test_KeyDerive() == E_OK) passed++; else failed++;
    if (Test_AesEncrypt() == E_OK) passed++; else failed++;
    if (Test_AesDecrypt() == E_OK) passed++; else failed++;
    if (Test_Sha256() == E_OK) passed++; else failed++;
    if (Test_HmacGenerate() == E_OK) passed++; else failed++;
    if (Test_Random() == E_OK) passed++; else failed++;
    if (Test_ProcessJob() == E_OK) passed++; else failed++;
    
    /* Deinitialize */
    if (Test_DeInit() == E_OK) passed++; else failed++;
    
    /* Summary */
    printf("\n");
    printf("========================================\n");
    printf("   Test Summary\n");
    printf("========================================\n");
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);
    printf("Total:  %d\n", passed + failed);
    printf("========================================\n");
}

/*==================================================================================================
 *                                    STUB FUNCTIONS
 *==================================================================================================*/

/* Stub for SchM functions - in real implementation these would be provided by the OS */
void SchM_Enter_Crypto_CRYPTO_EXCLUSIVE_AREA_0(void) {}
void SchM_Exit_Crypto_CRYPTO_EXCLUSIVE_AREA_0(void) {}
void SchM_Enter_Crypto_CRYPTO_EXCLUSIVE_AREA_1(void) {}
void SchM_Exit_Crypto_CRYPTO_EXCLUSIVE_AREA_1(void) {}
void SchM_Enter_Crypto_CRYPTO_EXCLUSIVE_AREA_2(void) {}
void SchM_Exit_Crypto_CRYPTO_EXCLUSIVE_AREA_2(void) {}

/* Stub for Det_ReportError - in real implementation this would report to the development error tracer */
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    return E_OK;
}

/*==================================================================================================
 *                                    MAIN ENTRY POINT
 *==================================================================================================*/
#ifdef CRYPTO_TEST_STANDALONE
int main(void)
{
    Crypto_RunAllTests();
    return 0;
}
#endif
