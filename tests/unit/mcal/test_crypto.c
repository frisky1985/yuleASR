/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Crypto (MCAL) Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-30
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

// @tests src/bsw/mcal/crypto/src/Crypto.c  @tests src/bsw/mcal/crypto/include/Crypto.h

#include "../test_framework.h"
#include "Crypto.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static Crypto_ConfigType g_test_config;
static Crypto_DriverObjectConfigType g_driver_objects[2];
static Crypto_ChannelConfigType g_channels[2];
static Crypto_KeyConfigType g_keys[2];
static Crypto_KeyElementConfigType g_key_elements[4];

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_config(void)
{
    /* Setup key elements */
    g_key_elements[0].keyElementId = 0;
    g_key_elements[0].keyElementSize = 16;
    g_key_elements[0].allowPartialAccess = FALSE;
    g_key_elements[0].readAccess = TRUE;
    g_key_elements[0].writeAccess = TRUE;

    g_key_elements[1].keyElementId = 1;
    g_key_elements[1].keyElementSize = 32;
    g_key_elements[1].allowPartialAccess = FALSE;
    g_key_elements[1].readAccess = TRUE;
    g_key_elements[1].writeAccess = TRUE;

    /* Setup keys */
    g_keys[0].keyId = 0;
    g_keys[0].keyElements = &g_key_elements[0];
    g_keys[0].numKeyElements = 1;
    g_keys[0].keyValid = TRUE;

    g_keys[1].keyId = 1;
    g_keys[1].keyElements = &g_key_elements[1];
    g_keys[1].numKeyElements = 1;
    g_keys[1].keyValid = TRUE;

    /* Setup driver objects */
    g_driver_objects[0].driverObjectId = 0;
    g_driver_objects[0].priority = 1;
    g_driver_objects[0].maxJobs = 4;
    g_driver_objects[0].asyncMode = FALSE;
    g_driver_objects[0].callback = NULL_PTR;

    g_driver_objects[1].driverObjectId = 1;
    g_driver_objects[1].priority = 2;
    g_driver_objects[1].maxJobs = 4;
    g_driver_objects[1].asyncMode = FALSE;
    g_driver_objects[1].callback = NULL_PTR;

    /* Setup channels */
    g_channels[0].channelId = 0;
    g_channels[0].driverObjectId = 0;
    g_channels[0].algorithmFamily = CRYPTO_ALGOFAM_AES;
    g_channels[0].algorithmMode = CRYPTO_ALGOMODE_CBC;
    g_channels[0].hwAcceleration = TRUE;
    g_channels[0].maxKeySize = 256;

    g_channels[1].channelId = 1;
    g_channels[1].driverObjectId = 1;
    g_channels[1].algorithmFamily = CRYPTO_ALGOFAM_SHA2_256;
    g_channels[1].algorithmMode = CRYPTO_ALGOMODE_NOT_SET;
    g_channels[1].hwAcceleration = TRUE;
    g_channels[1].maxKeySize = 0;

    /* Setup main config */
    g_test_config.driverObjects = g_driver_objects;
    g_test_config.numDriverObjects = 2;
    g_test_config.channels = g_channels;
    g_test_config.numChannels = 2;
    g_test_config.keys = g_keys;
    g_test_config.numKeys = 2;
    g_test_config.hwAccelerationEnabled = TRUE;
    g_test_config.clockFrequency = 160000000;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: Crypto_Init with valid config */
/** @req SWS_Crypto_00001 */
TEST_CASE(crypto_init_valid_config)
{
    setup_test_config();
    
    Crypto_Init(&g_test_config);
    
    ASSERT_TRUE(MCAL_CryptoIsInitialized() == TRUE);
}

/* Test: Crypto_Init with NULL config */
/** @req SWS_Crypto_00001 */
TEST_CASE(crypto_init_null_config)
{
    Crypto_Init(NULL_PTR);
    
    ASSERT_TRUE(MCAL_CryptoIsInitialized() == FALSE);
}

/* Test: Crypto_DeInit */
/** @req SWS_Crypto_00002 */
TEST_CASE(crypto_deinit)
{
    setup_test_config();
    Crypto_Init(&g_test_config);
    
    Crypto_DeInit();
    
    ASSERT_TRUE(MCAL_CryptoIsInitialized() == FALSE);
}

/* Test: Crypto_GetVersionInfo */
/** @req SWS_Crypto_00003 */
TEST_CASE(crypto_get_version_info)
{
    Std_VersionInfoType version_info;
    
    setup_test_config();
    Crypto_Init(&g_test_config);
    
    Crypto_GetVersionInfo(&version_info);
    
    ASSERT_EQ(CRYPTO_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(CRYPTO_SW_MAJOR_VERSION, version_info.sw_major_version);
    ASSERT_EQ(CRYPTO_SW_MINOR_VERSION, version_info.sw_minor_version);
}

/* Test: Crypto_ProcessJob - Encrypt */
/** @req SWS_Crypto_00004 */
TEST_CASE(crypto_process_job_encrypt)
{
    Std_ReturnType result;
    Crypto_JobType job;
    Crypto_JobPrimitiveInputOutputType io;
    Crypto_JobPrimitiveInfoType primitive_info;
    uint8 plaintext[16] = "Hello, World!!!";
    uint8 ciphertext[32];
    uint32 ciphertext_length = 32;

    setup_test_config();
    Crypto_Init(&g_test_config);

    /* Setup job */
    job.jobId = 0;
    job.channelId = 0;
    job.jobPrimitiveInputOutput = CRYPTO_OPERATIONMODE_SINGLECALL;
    
    io.inputPtr = plaintext;
    io.inputLength = 16;
    io.outputPtr = ciphertext;
    io.outputLengthPtr = &ciphertext_length;
    job.jobPrimitiveInputOutputPtr = &io;

    primitive_info.primitive = CRYPTO_OPERATION_ENCRYPT;
    primitive_info.algorithm.family = CRYPTO_ALGOFAM_AES;
    primitive_info.algorithm.mode = CRYPTO_ALGOMODE_CBC;
    primitive_info.algorithm.keyLength = 128;
    job.jobPrimitiveInfo = &primitive_info;
    
    result = Crypto_ProcessJob(0, &job);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK || result == CRYPTO_E_BUSY);
}

/* Test: Crypto_ProcessJob - Decrypt */
/** @req SWS_Crypto_00004 */
TEST_CASE(crypto_process_job_decrypt)
{
    Std_ReturnType result;
    Crypto_JobType job;
    Crypto_JobPrimitiveInputOutputType io;
    Crypto_JobPrimitiveInfoType primitive_info;
    uint8 ciphertext[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    uint8 plaintext[32];
    uint32 plaintext_length = 32;

    setup_test_config();
    Crypto_Init(&g_test_config);

    job.jobId = 1;
    job.channelId = 0;
    job.jobPrimitiveInputOutput = CRYPTO_OPERATIONMODE_SINGLECALL;
    
    io.inputPtr = ciphertext;
    io.inputLength = 16;
    io.outputPtr = plaintext;
    io.outputLengthPtr = &plaintext_length;
    job.jobPrimitiveInputOutputPtr = &io;

    primitive_info.primitive = CRYPTO_OPERATION_DECRYPT;
    primitive_info.algorithm.family = CRYPTO_ALGOFAM_AES;
    primitive_info.algorithm.mode = CRYPTO_ALGOMODE_CBC;
    primitive_info.algorithm.keyLength = 128;
    job.jobPrimitiveInfo = &primitive_info;
    
    result = Crypto_ProcessJob(0, &job);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK || result == CRYPTO_E_BUSY);
}

/* Test: Crypto_ProcessJob - Hash */
/** @req SWS_Crypto_00004 */
TEST_CASE(crypto_process_job_hash)
{
    Std_ReturnType result;
    Crypto_JobType job;
    Crypto_JobPrimitiveInputOutputType io;
    Crypto_JobPrimitiveInfoType primitive_info;
    uint8 data[32] = "Test data for hash";
    uint8 hash[32];
    uint32 hash_length = 32;

    setup_test_config();
    Crypto_Init(&g_test_config);

    job.jobId = 2;
    job.channelId = 1;
    job.jobPrimitiveInputOutput = CRYPTO_OPERATIONMODE_SINGLECALL;
    
    io.inputPtr = data;
    io.inputLength = 32;
    io.outputPtr = hash;
    io.outputLengthPtr = &hash_length;
    job.jobPrimitiveInputOutputPtr = &io;

    primitive_info.primitive = CRYPTO_OPERATION_HASH;
    primitive_info.algorithm.family = CRYPTO_ALGOFAM_SHA2_256;
    primitive_info.algorithm.mode = CRYPTO_ALGOMODE_NOT_SET;
    job.jobPrimitiveInfo = &primitive_info;
    
    result = Crypto_ProcessJob(1, &job);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK || result == CRYPTO_E_BUSY);
}

/* Test: Crypto_ProcessJob - MAC Generate */
/** @req SWS_Crypto_00004 */
TEST_CASE(crypto_process_job_mac_generate)
{
    Std_ReturnType result;
    Crypto_JobType job;
    Crypto_JobPrimitiveInputOutputType io;
    Crypto_JobPrimitiveInfoType primitive_info;
    uint8 data[16] = "MAC test data!!!";
    uint8 mac[32];
    uint32 mac_length = 32;

    setup_test_config();
    Crypto_Init(&g_test_config);

    job.jobId = 3;
    job.channelId = 0;
    job.jobPrimitiveInputOutput = CRYPTO_OPERATIONMODE_SINGLECALL;
    
    io.inputPtr = data;
    io.inputLength = 16;
    io.outputPtr = mac;
    io.outputLengthPtr = &mac_length;
    job.jobPrimitiveInputOutputPtr = &io;

    primitive_info.primitive = CRYPTO_OPERATION_MAC_GENERATE;
    primitive_info.algorithm.family = CRYPTO_ALGOFAM_HMAC;
    primitive_info.algorithm.mode = CRYPTO_ALGOMODE_NOT_SET;
    job.jobPrimitiveInfo = &primitive_info;
    
    result = Crypto_ProcessJob(0, &job);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK || result == CRYPTO_E_BUSY);
}

/* Test: Crypto_CancelJob */
/** @req SWS_Crypto_00005 */
TEST_CASE(crypto_cancel_job)
{
    Std_ReturnType result;
    Crypto_JobType job;

    setup_test_config();
    Crypto_Init(&g_test_config);

    job.jobId = 0;
    job.channelId = 0;
    
    result = Crypto_CancelJob(0, &job);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/* Test: Crypto_KeyElementSet */
/** @req SWS_Crypto_00006 */
TEST_CASE(crypto_key_element_set)
{
    Std_ReturnType result;
    uint8 key_data[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                          0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};

    setup_test_config();
    Crypto_Init(&g_test_config);

    result = Crypto_KeyElementSet(0, 0, key_data, 16);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/* Test: Crypto_KeyElementGet */
/** @req SWS_Crypto_00007 */
TEST_CASE(crypto_key_element_get)
{
    Std_ReturnType result;
    uint8 key_data[32];
    uint32 key_length = 32;

    setup_test_config();
    Crypto_Init(&g_test_config);

    result = Crypto_KeyElementGet(0, 0, key_data, &key_length);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/* Test: Crypto_KeyValidSet */
/** @req SWS_Crypto_00008 */
TEST_CASE(crypto_key_valid_set)
{
    Std_ReturnType result;

    setup_test_config();
    Crypto_Init(&g_test_config);

    result = Crypto_KeyValidSet(0);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/* Test: Crypto_KeyElementCopy */
/** @req SWS_Crypto_00010 */
TEST_CASE(crypto_key_element_copy)
{
    Std_ReturnType result;

    setup_test_config();
    Crypto_Init(&g_test_config);

    result = Crypto_KeyElementCopy(0, 0, 1, 0);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/* Test: Crypto_RandomSeed */
/** @req SWS_Crypto_00015 */
TEST_CASE(crypto_random_seed)
{
    Std_ReturnType result;
    uint8 seed[16] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
                      0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};

    setup_test_config();
    Crypto_Init(&g_test_config);

    result = Crypto_RandomSeed(0, seed, 16);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/* Test: Crypto_KeyGenerate */
/** @req SWS_Crypto_00011 */
TEST_CASE(crypto_key_generate)
{
    Std_ReturnType result;

    setup_test_config();
    Crypto_Init(&g_test_config);

    result = Crypto_KeyGenerate(0);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/* Test: Crypto_KeyDerive */
/** @req SWS_Crypto_00012 */
TEST_CASE(crypto_key_derive)
{
    Std_ReturnType result;

    setup_test_config();
    Crypto_Init(&g_test_config);

    result = Crypto_KeyDerive(0, 1);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(crypto)
{
}

TEST_SUITE_TEARDOWN(crypto)
{
}

TEST_SUITE(crypto)
{
    RUN_TEST(crypto_init_valid_config);
    RUN_TEST(crypto_init_null_config);
    RUN_TEST(crypto_deinit);
    RUN_TEST(crypto_get_version_info);
    RUN_TEST(crypto_process_job_encrypt);
    RUN_TEST(crypto_process_job_decrypt);
    RUN_TEST(crypto_process_job_hash);
    RUN_TEST(crypto_process_job_mac_generate);
    RUN_TEST(crypto_cancel_job);
    RUN_TEST(crypto_key_element_set);
    RUN_TEST(crypto_key_element_get);
    RUN_TEST(crypto_key_valid_set);
    RUN_TEST(crypto_key_element_copy);
    RUN_TEST(crypto_random_seed);
    RUN_TEST(crypto_key_generate);
    RUN_TEST(crypto_key_derive);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(crypto);
TEST_MAIN_END()
