/*==================================================================================================
 *                              CRYPTO INTERFACE (CryIf) - TEST
 *==================================================================================================
 * FILENAME: CryIf_Test.c
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Test file for CryIf module
 *==================================================================================================
 */

#include "CryIf.h"
#include "stdio.h"

/* Test configuration */
static const CryIf_ChannelConfigType CryIf_TestChannels[] = {
    {
        .channelId = CRYIF_CHANNEL_ID_AES_ENCRYPT,
        .priority = CRYIF_CHANNEL_PRIORITY_HIGH,
        .primitive = CRYIF_CRYPTOPRIMITIVE_ENCRYPT,
        .algorithmFamily = CRYIF_ALGOFAM_AES,
        .algorithmMode = CRYIF_ALGOMODE_CBC,
        .callbackActive = FALSE
    },
    {
        .channelId = CRYIF_CHANNEL_ID_HASH_SHA256,
        .priority = CRYIF_CHANNEL_PRIORITY_NORMAL,
        .primitive = CRYIF_CRYPTOPRIMITIVE_HASH,
        .algorithmFamily = CRYIF_ALGOFAM_SHA2_256,
        .algorithmMode = CRYIF_ALGOMODE_NOT_SET,
        .callbackActive = FALSE
    }
};

static const CryIf_KeyConfigType CryIf_TestKeys[] = {
    {
        .cryIfKeyId = CRYIF_KEY_ID_AES_128,
        .keyLength = CRYIF_KEY_LENGTH_AES_128,
        .keyValid = FALSE
    },
    {
        .cryIfKeyId = CRYIF_KEY_ID_HMAC_SHA256,
        .keyLength = CRYIF_KEY_LENGTH_HMAC_SHA256,
        .keyValid = FALSE
    }
};

static const CryIf_ConfigType CryIf_TestConfig = {
    .channelConfigs = CryIf_TestChannels,
    .numChannels = 2,
    .keyConfigs = CryIf_TestKeys,
    .numKeys = 2
};

/* Test functions */
/** @req SWS_CryIf_00001 */
static void test_init_deinit(void)
{
    printf("Testing CryIf_Init/DeInit...\n");
    
    CryIf_Init(&CryIf_TestConfig);
    
    if (CryIf_Initialized != TRUE) {
        printf("  FAIL: CryIf not initialized\n");
        return;
    }
    
    CryIf_DeInit();
    
    if (CryIf_Initialized != FALSE) {
        printf("  FAIL: CryIf not deinitialized\n");
        return;
    }
    
    printf("  PASS\n");
}

static void test_version_info(void)
{
#if (CRYIF_VERSION_INFO_API == STD_ON)
    Std_VersionInfoType version;
    
    printf("Testing CryIf_GetVersionInfo...\n");
    
    CryIf_Init(&CryIf_TestConfig);
    CryIf_GetVersionInfo(&version);
    
    if (version.vendorID != CRYIF_VENDOR_ID) {
        printf("  FAIL: Wrong vendor ID\n");
        CryIf_DeInit();
        return;
    }
    
    if (version.moduleID != CRYIF_MODULE_ID) {
        printf("  FAIL: Wrong module ID\n");
        CryIf_DeInit();
        return;
    }
    
    printf("  PASS (Version: %d.%d.%d)\n", 
           version.sw_major_version,
           version.sw_minor_version,
           version.sw_patch_version);
    
    CryIf_DeInit();
#endif
}

static void test_key_management(void)
{
    uint8 testKey[CRYIF_KEY_LENGTH_AES_128] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
    };
    uint8 readKey[CRYIF_KEY_LENGTH_AES_128];
    uint32 keyLength = CRYIF_KEY_LENGTH_AES_128;
    Std_ReturnType result;
    
    printf("Testing Key Management...\n");
    
    CryIf_Init(&CryIf_TestConfig);
    
    /* Set key element */
    result = CryIf_KeyElementSet(CRYIF_KEY_ID_AES_128, CRYIF_KEY_ELEMENT_ID_KEY,
                                  testKey, CRYIF_KEY_LENGTH_AES_128);
    if (result != E_OK) {
        printf("  FAIL: KeyElementSet failed\n");
        CryIf_DeInit();
        return;
    }
    
    /* Set key valid */
    result = CryIf_KeySetValid(CRYIF_KEY_ID_AES_128);
    if (result != E_OK) {
        printf("  FAIL: KeySetValid failed\n");
        CryIf_DeInit();
        return;
    }
    
    /* Get key element */
    result = CryIf_KeyElementGet(CRYIF_KEY_ID_AES_128, CRYIF_KEY_ELEMENT_ID_KEY,
                                  readKey, &keyLength);
    if (result != E_OK) {
        printf("  FAIL: KeyElementGet failed\n");
        CryIf_DeInit();
        return;
    }
    
    /* Verify key data */
    for (uint32 i = 0; i < CRYIF_KEY_LENGTH_AES_128; i++) {
        if (testKey[i] != readKey[i]) {
            printf("  FAIL: Key data mismatch at byte %d\n", i);
            CryIf_DeInit();
            return;
        }
    }
    
    printf("  PASS\n");
    CryIf_DeInit();
}

static void test_hash(void)
{
    uint8 testData[] = "Hello, World!";
    uint8 hash[CRYIF_SHA256_SIZE];
    uint32 hashLength;
    Std_ReturnType result;
    
    printf("Testing Hash (SHA-256)...\n");
    
    CryIf_Init(&CryIf_TestConfig);
    
    result = CryIf_Hash(CRYIF_CHANNEL_ID_HASH_SHA256,
                         CRYIF_OPERATIONMODE_STREAMSTART,
                         testData, sizeof(testData) - 1,
                         hash, &hashLength);
    
    if (result != E_OK) {
        printf("  FAIL: Hash calculation failed\n");
        CryIf_DeInit();
        return;
    }
    
    if (hashLength != CRYIF_SHA256_SIZE) {
        printf("  FAIL: Wrong hash length (%d, expected %d)\n", 
               hashLength, CRYIF_SHA256_SIZE);
        CryIf_DeInit();
        return;
    }
    
    printf("  PASS (Hash: ");
    for (uint32 i = 0; i < 8; i++) {
        printf("%02X", hash[i]);
    }
    printf("...)\n");
    
    CryIf_DeInit();
}

static void test_aes_encryption(void)
{
    uint8 key[CRYIF_KEY_LENGTH_AES_128] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
    };
    uint8 iv[CRYIF_AES_IV_SIZE] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    uint8 plaintext[16] = "AES TEST DATA!!";
    uint8 ciphertext[16];
    uint8 decrypted[16];
    uint32 outLength;
    Std_ReturnType result;
    CryIf_AlgorithmInfoType algorithm;
    
    printf("Testing AES Encryption/Decryption...\n");
    
    CryIf_Init(&CryIf_TestConfig);
    
    /* Set key */
    result = CryIf_KeyElementSet(CRYIF_KEY_ID_AES_128, CRYIF_KEY_ELEMENT_ID_KEY,
                                  key, CRYIF_KEY_LENGTH_AES_128);
    if (result != E_OK) {
        printf("  FAIL: KeyElementSet failed\n");
        CryIf_DeInit();
        return;
    }
    
    result = CryIf_KeySetValid(CRYIF_KEY_ID_AES_128);
    if (result != E_OK) {
        printf("  FAIL: KeySetValid failed\n");
        CryIf_DeInit();
        return;
    }
    
    /* Initialize cipher */
    algorithm.family = CRYIF_ALGOFAM_AES;
    algorithm.mode = CRYIF_ALGOMODE_CBC;
    algorithm.keyLength = CRYIF_KEY_LENGTH_AES_128 * 8;
    algorithm.ivLength = CRYIF_AES_IV_SIZE * 8;
    
    result = CryIf_CipherInit(CRYIF_CHANNEL_ID_AES_ENCRYPT, &algorithm,
                               CRYIF_KEY_ID_AES_128, iv, CRYIF_AES_IV_SIZE);
    if (result != E_OK) {
        printf("  FAIL: CipherInit failed\n");
        CryIf_DeInit();
        return;
    }
    
    /* Encrypt */
    result = CryIf_Encrypt(CRYIF_CHANNEL_ID_AES_ENCRYPT,
                            CRYIF_OPERATIONMODE_STREAMSTART,
                            plaintext, 16,
                            ciphertext, &outLength);
    if (result != E_OK) {
        printf("  FAIL: Encrypt failed\n");
        CryIf_DeInit();
        return;
    }
    
    /* Initialize for decryption */
    result = CryIf_CipherInit(CRYIF_CHANNEL_ID_AES_ENCRYPT, &algorithm,
                               CRYIF_KEY_ID_AES_128, iv, CRYIF_AES_IV_SIZE);
    if (result != E_OK) {
        printf("  FAIL: CipherInit (decrypt) failed\n");
        CryIf_DeInit();
        return;
    }
    
    /* Decrypt */
    result = CryIf_Decrypt(CRYIF_CHANNEL_ID_AES_ENCRYPT,
                            CRYIF_OPERATIONMODE_STREAMSTART,
                            ciphertext, 16,
                            decrypted, &outLength);
    if (result != E_OK) {
        printf("  FAIL: Decrypt failed\n");
        CryIf_DeInit();
        return;
    }
    
    /* Verify decrypted matches original */
    for (uint32 i = 0; i < 16; i++) {
        if (plaintext[i] != decrypted[i]) {
            printf("  FAIL: Decrypted data mismatch at byte %d\n", i);
            CryIf_DeInit();
            return;
        }
    }
    
    printf("  PASS\n");
    CryIf_DeInit();
}

static void test_hmac(void)
{
    uint8 key[CRYIF_KEY_LENGTH_HMAC_SHA256] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20
    };
    uint8 data[] = "Test data for HMAC";
    uint8 mac[CRYIF_HMAC_SHA256_SIZE];
    uint32 macLength;
    CryIf_VerifyResultType verifyResult;
    Std_ReturnType result;
    
    printf("Testing HMAC-SHA256...\n");
    
    CryIf_Init(&CryIf_TestConfig);
    
    /* Set HMAC key */
    result = CryIf_KeyElementSet(CRYIF_KEY_ID_HMAC_SHA256, CRYIF_KEY_ELEMENT_ID_KEY,
                                  key, CRYIF_KEY_LENGTH_HMAC_SHA256);
    if (result != E_OK) {
        printf("  FAIL: KeyElementSet failed\n");
        CryIf_DeInit();
        return;
    }
    
    result = CryIf_KeySetValid(CRYIF_KEY_ID_HMAC_SHA256);
    if (result != E_OK) {
        printf("  FAIL: KeySetValid failed\n");
        CryIf_DeInit();
        return;
    }
    
    /* Configure channel for HMAC */
    CryIf_Channels[CRYIF_CHANNEL_ID_MAC_GENERATE].primitive = CRYIF_CRYPTOPRIMITIVE_MAC_GENERATE;
    CryIf_Channels[CRYIF_CHANNEL_ID_MAC_GENERATE].algorithmFamily = CRYIF_ALGOFAM_HMAC;
    CryIf_Channels[CRYIF_CHANNEL_ID_MAC_GENERATE].keyId = CRYIF_KEY_ID_HMAC_SHA256;
    
    /* Generate MAC */
    result = CryIf_MacGenerate(CRYIF_CHANNEL_ID_MAC_GENERATE,
                                CRYIF_OPERATIONMODE_STREAMSTART,
                                data, sizeof(data) - 1,
                                mac, &macLength);
    if (result != E_OK) {
        printf("  FAIL: MacGenerate failed\n");
        CryIf_DeInit();
        return;
    }
    
    /* Verify MAC */
    result = CryIf_MacVerify(CRYIF_CHANNEL_ID_MAC_GENERATE,
                              CRYIF_OPERATIONMODE_STREAMSTART,
                              data, sizeof(data) - 1,
                              mac, macLength,
                              &verifyResult);
    if (result != E_OK) {
        printf("  FAIL: MacVerify failed\n");
        CryIf_DeInit();
        return;
    }
    
    if (verifyResult != CRYIF_E_VER_OK) {
        printf("  FAIL: MAC verification failed\n");
        CryIf_DeInit();
        return;
    }
    
    printf("  PASS\n");
    CryIf_DeInit();
}

static void test_random(void)
{
    uint8 random1[16];
    uint8 random2[16];
    Std_ReturnType result;
    boolean same = TRUE;
    uint32 i;
    
    printf("Testing Random Number Generation...\n");
    
    CryIf_Init(&CryIf_TestConfig);
    
    /* Generate first random block */
    result = CryIf_RandomGenerate(CRYIF_CHANNEL_ID_RANDOM, random1, 16);
    if (result != E_OK) {
        printf("  FAIL: RandomGenerate failed\n");
        CryIf_DeInit();
        return;
    }
    
    /* Generate second random block */
    result = CryIf_RandomGenerate(CRYIF_CHANNEL_ID_RANDOM, random2, 16);
    if (result != E_OK) {
        printf("  FAIL: RandomGenerate failed\n");
        CryIf_DeInit();
        return;
    }
    
    /* Check that they're different (with very high probability) */
    for (i = 0; i < 16; i++) {
        if (random1[i] != random2[i]) {
            same = FALSE;
            break;
        }
    }
    
    if (same) {
        printf("  WARN: Random numbers are the same (possible with low probability)\n");
    } else {
        printf("  PASS\n");
    }
    
    CryIf_DeInit();
}

/* Main test entry point */
int main(void)
{
    printf("\n========================================\n");
    printf("    CryIf Module Test Suite\n");
    printf("========================================\n\n");
    
    test_init_deinit();
    test_version_info();
    test_key_management();
    test_hash();
    test_aes_encryption();
    test_hmac();
    test_random();
    
    printf("\n========================================\n");
    printf("    Test Suite Complete\n");
    printf("========================================\n\n");
    
    return 0;
}
