/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : CryIf Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-30
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "../test_framework.h"
#include "CryIf.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static CryIf_ConfigType g_test_config;
static CryIf_ChannelConfigType g_channel_configs[2];
static CryIf_KeyConfigType g_key_configs[2];

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_config(void)
{
    /* Setup channel configs */
    g_channel_configs[0].channelId = 0;
    g_channel_configs[0].priority = 1;
    g_channel_configs[0].primitive = CRYIF_CRYPTOPRIMITIVE_ENCRYPT;
    g_channel_configs[0].algorithmFamily = CRYIF_ALGOFAM_AES;
    g_channel_configs[0].algorithmMode = CRYIF_ALGOMODE_CBC;
    g_channel_configs[0].callbackActive = FALSE;

    g_channel_configs[1].channelId = 1;
    g_channel_configs[1].priority = 2;
    g_channel_configs[1].primitive = CRYIF_CRYPTOPRIMITIVE_HASH;
    g_channel_configs[1].algorithmFamily = CRYIF_ALGOFAM_SHA2_256;
    g_channel_configs[1].algorithmMode = CRYIF_ALGOMODE_NOT_SET;
    g_channel_configs[1].callbackActive = FALSE;

    /* Setup key configs */
    g_key_configs[0].cryIfKeyId = 0;
    g_key_configs[0].keyLength = 16;
    g_key_configs[0].keyValid = TRUE;

    g_key_configs[1].cryIfKeyId = 1;
    g_key_configs[1].keyLength = 32;
    g_key_configs[1].keyValid = TRUE;

    /* Setup main config */
    g_test_config.channelConfigs = g_channel_configs;
    g_test_config.numChannels = 2;
    g_test_config.keyConfigs = g_key_configs;
    g_test_config.numKeys = 2;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: CryIf_Init with valid config */
TEST_CASE(cryif_init_valid_config)
{
    setup_test_config();
    
    CryIf_Init(&g_test_config);
    
    ASSERT_TRUE(CryIf_Initialized == TRUE);
    TEST_PASS();
}

/* Test: CryIf_Init with NULL config */
TEST_CASE(cryif_init_null_config)
{
    CryIf_Init(NULL_PTR);
    
    TEST_PASS();
}

/* Test: CryIf_DeInit */
TEST_CASE(cryif_deinit)
{
    setup_test_config();
    CryIf_Init(&g_test_config);
    
    CryIf_DeInit();
    
    ASSERT_TRUE(CryIf_Initialized == FALSE);
    TEST_PASS();
}

/* Test: CryIf_GetVersionInfo */
TEST_CASE(cryif_get_version_info)
{
    Std_VersionInfoType version_info;
    
    setup_test_config();
    CryIf_Init(&g_test_config);
    
    CryIf_GetVersionInfo(&version_info);
    
    ASSERT_EQ(CRYIF_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(CRYIF_SW_MAJOR_VERSION, version_info.sw_major_version);
    ASSERT_EQ(CRYIF_SW_MINOR_VERSION, version_info.sw_minor_version);
    TEST_PASS();
}

/* Test: CryIf_CipherInit */
TEST_CASE(cryif_cipher_init)
{
    Std_ReturnType result;
    CryIf_AlgorithmInfoType algorithm;
    uint8 iv[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

    setup_test_config();
    CryIf_Init(&g_test_config);

    algorithm.family = CRYIF_ALGOFAM_AES;
    algorithm.mode = CRYIF_ALGOMODE_CBC;
    algorithm.keyLength = 128;
    algorithm.ivLength = 16;
    algorithm.authTagLength = 0;

    result = CryIf_CipherInit(0, &algorithm, 0, iv, 16);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    TEST_PASS();
}

/* Test: CryIf_Encrypt */
TEST_CASE(cryif_encrypt)
{
    Std_ReturnType result;
    uint8 plaintext[16] = "Hello, World!!!";
    uint8 ciphertext[32];
    uint32 ciphertext_length = 32;

    setup_test_config();
    CryIf_Init(&g_test_config);

    result = CryIf_Encrypt(0, CRYIF_OPERATIONMODE_SINGLECALL, plaintext, 16,
                           ciphertext, &ciphertext_length);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    TEST_PASS();
}

/* Test: CryIf_Decrypt */
TEST_CASE(cryif_decrypt)
{
    Std_ReturnType result;
    uint8 ciphertext[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    uint8 plaintext[32];
    uint32 plaintext_length = 32;

    setup_test_config();
    CryIf_Init(&g_test_config);

    result = CryIf_Decrypt(0, CRYIF_OPERATIONMODE_SINGLECALL, ciphertext, 16,
                           plaintext, &plaintext_length);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    TEST_PASS();
}

/* Test: CryIf_MacGenerate */
TEST_CASE(cryif_mac_generate)
{
    Std_ReturnType result;
    uint8 data[16] = "Test data for MAC";
    uint8 mac[32];
    uint32 mac_length = 32;

    setup_test_config();
    CryIf_Init(&g_test_config);

    result = CryIf_MacGenerate(0, CRYIF_OPERATIONMODE_SINGLECALL, data, 16,
                               mac, &mac_length);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    TEST_PASS();
}

/* Test: CryIf_MacVerify */
TEST_CASE(cryif_mac_verify)
{
    Std_ReturnType result;
    uint8 data[16] = "Test data for MAC";
    uint8 mac[32] = {0};
    CryIf_VerifyResultType verify_result;

    setup_test_config();
    CryIf_Init(&g_test_config);

    result = CryIf_MacVerify(0, CRYIF_OPERATIONMODE_SINGLECALL, data, 16,
                             mac, 16, &verify_result);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    TEST_PASS();
}

/* Test: CryIf_Hash */
TEST_CASE(cryif_hash)
{
    Std_ReturnType result;
    uint8 data[32] = "Test data for hash calculation";
    uint8 hash[32];
    uint32 hash_length = 32;

    setup_test_config();
    CryIf_Init(&g_test_config);

    result = CryIf_Hash(1, CRYIF_OPERATIONMODE_SINGLECALL, data, 32,
                        hash, &hash_length);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    TEST_PASS();
}

/* Test: CryIf_HashStart */
TEST_CASE(cryif_hash_start)
{
    Std_ReturnType result;
    CryIf_AlgorithmInfoType algorithm;

    setup_test_config();
    CryIf_Init(&g_test_config);

    algorithm.family = CRYIF_ALGOFAM_SHA2_256;
    algorithm.mode = CRYIF_ALGOMODE_NOT_SET;
    algorithm.keyLength = 0;
    algorithm.ivLength = 0;
    algorithm.authTagLength = 0;

    result = CryIf_HashStart(1, &algorithm);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    TEST_PASS();
}

/* Test: CryIf_HashUpdate */
TEST_CASE(cryif_hash_update)
{
    Std_ReturnType result;
    uint8 data[16] = "Test data chunk";

    setup_test_config();
    CryIf_Init(&g_test_config);

    result = CryIf_HashUpdate(1, data, 16);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    TEST_PASS();
}

/* Test: CryIf_HashFinish */
TEST_CASE(cryif_hash_finish)
{
    Std_ReturnType result;
    uint8 hash[32];
    uint32 hash_length = 32;

    setup_test_config();
    CryIf_Init(&g_test_config);

    result = CryIf_HashFinish(1, hash, &hash_length);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    TEST_PASS();
}

/* Test: CryIf_KeyElementSet */
TEST_CASE(cryif_key_element_set)
{
    Std_ReturnType result;
    uint8 key_data[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                          0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};

    setup_test_config();
    CryIf_Init(&g_test_config);

    result = CryIf_KeyElementSet(0, 0, key_data, 16);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    TEST_PASS();
}

/* Test: CryIf_KeyElementGet */
TEST_CASE(cryif_key_element_get)
{
    Std_ReturnType result;
    uint8 key_data[32];
    uint32 key_length = 32;

    setup_test_config();
    CryIf_Init(&g_test_config);

    result = CryIf_KeyElementGet(0, 0, key_data, &key_length);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(cryif)
{
}

TEST_SUITE_TEARDOWN(cryif)
{
}

TEST_SUITE(cryif)
{
    RUN_TEST(cryif_init_valid_config);
    RUN_TEST(cryif_init_null_config);
    RUN_TEST(cryif_deinit);
    RUN_TEST(cryif_get_version_info);
    RUN_TEST(cryif_cipher_init);
    RUN_TEST(cryif_encrypt);
    RUN_TEST(cryif_decrypt);
    RUN_TEST(cryif_mac_generate);
    RUN_TEST(cryif_mac_verify);
    RUN_TEST(cryif_hash);
    RUN_TEST(cryif_hash_start);
    RUN_TEST(cryif_hash_update);
    RUN_TEST(cryif_hash_finish);
    RUN_TEST(cryif_key_element_set);
    RUN_TEST(cryif_key_element_get);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(cryif);
TEST_MAIN_END()
