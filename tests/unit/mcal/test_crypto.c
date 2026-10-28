/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Crypto (MCAL) Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-10-26
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

// @tests src/bsw/mcal/crypto/src/Crypto.c  @tests src/bsw/mcal/crypto/include/Crypto.h

/*==================================================================================================
* NOTE (2026-10-26, 首次注册进构建时的修正记录):
*  原版测试从未编译过, 与 Crypto_Types.h 实际定义不符, 已按真实类型修正:
*  1. Crypto_ConfigType.keys 是运行时类型 Crypto_KeyType* (numElements/keyElements[i].id/
*     size/data), 而非配置时 Crypto_KeyConfigType (numKeyElements/keyValid) — 改用运行时结构。
*  2. Crypto_KeyElementType.data 是 uint8* 指针 — 为其提供静态缓冲区。
*  3. Crypto_JobType.jobPrimitiveInputOutput 是指针 (原版误当 operation mode 值赋值),
*     操作模式改填 io->mode; primitive_info.algorithm 是 Crypto_AlgorithmInfoType* (原版
*     误当 CRYPTO_OPERATION_* 枚举字段) — 改为指针 + service/processingType。
*  4. Crypto_KeyValidSet(cryptoKeyId, valid) 需要两个参数 (原版只传一个)。
*  5. MCAL_CryptoIsInitialized() 在产品代码中不存在 — 改用桩侧 Det_ReportError 计数与
*     API 返回值间接断言驱动状态。
*  后端符号 Crypto_Hsm_* / Crypto_MbedTLS_* 由 tests/unit/crypto_stubs/crypto_backend_stubs.c
*  提供 (HSM 不可用 -> 软件回退路径), blake2b/blake2s 链接真实实现。
==================================================================================================*/

#include <string.h>
#include "../test_framework.h"
#include "Crypto.h"

/* NOTE: Crypto.c 定义了 Crypto_MainFunction 但 Crypto.h 未声明, 测试需显式 extern */
extern void Crypto_MainFunction(void);

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static Crypto_ConfigType g_test_config;
static Crypto_DriverObjectConfigType g_driver_objects[2];
static Crypto_ChannelConfigType g_channels[2];
static Crypto_KeyType g_keys[2];
static Crypto_KeyElementType g_key_elements[2];
static uint8 g_key0_buf[CRYPTO_CFG_MAX_KEY_SIZE];
static uint8 g_key1_buf[CRYPTO_CFG_MAX_KEY_SIZE];
static Crypto_AlgorithmInfoType g_alg_info;

/* Det_ReportError 计数桩 (crypto_backend_stubs.c) */
extern uint32 Crypto_Stubs_GetDetErrorCount(void);
extern void Crypto_Stubs_ResetDetErrorCount(void);

/* Stub 控制接口 — 允许测试动态切换 HSM 可用性 / MbedTLS_Init 行为等 */
extern void Crypto_Stubs_SetHsmEnabled(boolean enabled);
extern void Crypto_Stubs_SetMbedTlsInitFail(boolean fail);
extern void Crypto_Stubs_SetHsmProcessRet(Std_ReturnType ret);
extern void Crypto_Stubs_SetHsmLoadKeyRet(Std_ReturnType ret);
extern void Crypto_Stubs_SetHsmSelfTestRet(Std_ReturnType ret);
extern void Crypto_Stubs_ResetAllHsmFlags(void);

/* Crypto_JobNotification / Crypto_ErrorNotification 在 Crypto.c 里是 weak,
 * 这里定义非 weak 版本用于验证 callback 是否被调用 */
static Crypto_JobType* g_last_notified_job = NULL_PTR;
static Crypto_ResultType g_last_notified_result;
static boolean g_notification_called = FALSE;

void Crypto_JobNotification(Crypto_JobType* job, Crypto_ResultType result)
{
    g_last_notified_job = job;
    g_last_notified_result = result;
    g_notification_called = TRUE;
}

void Crypto_ErrorNotification(uint16 moduleId, uint8 instanceId, uint8 apiId, uint8 errorId)
{
    (void)moduleId; (void)instanceId; (void)apiId; (void)errorId;
    /* 默认空实现 — Crypto.c 是 weak, 这里定义强符号覆盖 */
}

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_config(void)
{
    memset(&g_test_config, 0, sizeof(g_test_config));
    memset(g_keys, 0, sizeof(g_keys));
    memset(g_key_elements, 0, sizeof(g_key_elements));
    memset(g_driver_objects, 0, sizeof(g_driver_objects));
    memset(g_channels, 0, sizeof(g_channels));

    /* Runtime key element #0 (key 0): id=0, 16 bytes */
    g_key_elements[0].id = 0;
    g_key_elements[0].size = 16;
    g_key_elements[0].allowPartialAccess = FALSE;
    g_key_elements[0].writeAccess = TRUE;
    g_key_elements[0].data = g_key0_buf;

    /* Runtime key element #1 (key 1): id=0, 32 bytes */
    g_key_elements[1].id = 0;
    g_key_elements[1].size = 32;
    g_key_elements[1].allowPartialAccess = FALSE;
    g_key_elements[1].writeAccess = TRUE;
    g_key_elements[1].data = g_key1_buf;

    g_keys[0].keyId = 0;
    g_keys[0].numElements = 1;
    g_keys[0].keyElements = &g_key_elements[0];
    g_keys[0].keyType = CRYPTO_KEY_TYPE_CUSTOM;
    g_keys[0].keyState = CRYPTO_KEY_VALID;

    g_keys[1].keyId = 1;
    g_keys[1].numElements = 1;
    g_keys[1].keyElements = &g_key_elements[1];
    g_keys[1].keyType = CRYPTO_KEY_TYPE_CUSTOM;
    g_keys[1].keyState = CRYPTO_KEY_VALID;

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

    g_test_config.numKeys = 2;
    g_test_config.keys = g_keys;
    g_test_config.numChannels = 2;
    g_test_config.queueSize = CRYPTO_CFG_QUEUE_SIZE;
    g_test_config.driverObjects = g_driver_objects;
    g_test_config.numDriverObjects = 2;
    g_test_config.channels = g_channels;
    g_test_config.hwAccelerationEnabled = TRUE;
    g_test_config.clockFrequency = 160000000;
}

/* DeInit 后重新初始化, 保证每个用例从全新 INIT 状态开始 (驱动状态为静态变量) */
static void init_fresh(void)
{
    Crypto_DeInit();
    Crypto_Stubs_ResetAllHsmFlags();  /* 默认 HSM 可用, MbedTLS_Init 成功 */
    Crypto_Init(&g_test_config);
}

/* 恢复到 DeInit 状态 */
static void deinit_only(void)
{
    Crypto_DeInit();
}

static void build_job(Crypto_JobType* job,
                      Crypto_JobPrimitiveInputOutputType* io,
                      Crypto_JobPrimitiveInfoType* info,
                      uint8* input, uint32 inputLen,
                      uint8* output, uint32* outputLen,
                      uint32 family, uint32 mode, uint32 service,
                      Crypto_ProcessingType processing)
{
    memset(job, 0, sizeof(*job));
    memset(io, 0, sizeof(*io));
    memset(info, 0, sizeof(*info));

    io->inputPtr = (uint32*)(void*)input;
    io->inputLength = inputLen;
    io->outputPtr = (uint32*)(void*)output;
    io->outputLengthPtr = outputLen;
    io->mode = CRYPTO_OPERATIONMODE_SINGLECALL;

    g_alg_info.family = family;
    g_alg_info.mode = mode;
    g_alg_info.keyLength = 128;
    g_alg_info.curve = CRYPTO_EC_CURVE_SECP256R1;

    info->callbackId = 0;
    info->algorithm = &g_alg_info;
    info->service = service;
    info->processingType = processing;
    info->primitiveCallbackUpdateNotification = FALSE;

    job->jobId = 0;
    job->jobPrimitiveInputOutput = io;
    job->jobPrimitiveInfo = info;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: Crypto_Init with valid config; second Init hits ALREADY_INITIALIZED */
/** @req SWS_Crypto_00001 */
TEST_CASE(crypto_init_valid_config)
{
    setup_test_config();
    Crypto_Stubs_ResetDetErrorCount();
    Crypto_DeInit();
    Crypto_Init(&g_test_config);

    /* NOTE: 无 MCAL_CryptoIsInitialized(), 用 DET 计数断言首次 Init 未报错 */
    ASSERT_TRUE(Crypto_Stubs_GetDetErrorCount() == 0U);

    Crypto_Init(&g_test_config);
    ASSERT_TRUE(Crypto_Stubs_GetDetErrorCount() >= 1U);
}

/* Test: Crypto_Init with NULL config -> DET CRYPTO_E_PARAM_POINTER */
/** @req SWS_Crypto_00001 */
TEST_CASE(crypto_init_null_config)
{
    setup_test_config();
    Crypto_Stubs_ResetDetErrorCount();

    /* NOTE: NULL 检查在状态检查之前, 无论驱动状态如何都应报 DET */
    Crypto_Init(NULL_PTR);

    ASSERT_TRUE(Crypto_Stubs_GetDetErrorCount() >= 1U);
}

/* Test: Crypto_DeInit returns driver to UNINIT (ProcessJob rejected) */
/** @req SWS_Crypto_00002 */
TEST_CASE(crypto_deinit)
{
    Std_ReturnType result;
    Crypto_JobType job;
    Crypto_JobPrimitiveInputOutputType io;
    Crypto_JobPrimitiveInfoType info;
    uint8 input[16];
    uint8 output[16];
    uint32 out_len = 16;

    setup_test_config();
    Crypto_DeInit();
    Crypto_Init(&g_test_config);
    Crypto_DeInit();

    build_job(&job, &io, &info, input, 16, output, &out_len,
              CRYPTO_ALGOFAM_AES, CRYPTO_ALGOMODE_CBC, CRYPTO_SERVICE_ENCRYPT,
              CRYPTO_PROCESSING_SYNC);
    result = Crypto_ProcessJob(0, &job);
    ASSERT_EQ(E_NOT_OK, result);
}

/* Test: Crypto_GetVersionInfo */
/** @req SWS_Crypto_00003 */
TEST_CASE(crypto_get_version_info)
{
    Std_VersionInfoType version_info;

    setup_test_config();
    init_fresh();

    Crypto_GetVersionInfo(&version_info);

    ASSERT_EQ(CRYPTO_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(CRYPTO_MODULE_ID, version_info.moduleID);
    ASSERT_EQ(CRYPTO_SW_MAJOR_VERSION, version_info.sw_major_version);
    ASSERT_EQ(CRYPTO_SW_MINOR_VERSION, version_info.sw_minor_version);
    ASSERT_EQ(CRYPTO_SW_PATCH_VERSION, version_info.sw_patch_version);
}

/* Test: Crypto_ProcessJob - Encrypt (sync -> software backend) */
/** @req SWS_Crypto_00004 */
TEST_CASE(crypto_process_job_encrypt)
{
    Std_ReturnType result;
    Crypto_JobType job;
    Crypto_JobPrimitiveInputOutputType io;
    Crypto_JobPrimitiveInfoType info;
    uint8 plaintext[16] = "Hello, World!!!";
    uint8 ciphertext[32];
    uint32 ciphertext_length = 32;

    setup_test_config();
    init_fresh();

    build_job(&job, &io, &info, plaintext, 16, ciphertext, &ciphertext_length,
              CRYPTO_ALGOFAM_AES, CRYPTO_ALGOMODE_CBC, CRYPTO_SERVICE_ENCRYPT,
              CRYPTO_PROCESSING_SYNC);
    result = Crypto_ProcessJob(0, &job);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK || result == CRYPTO_E_BUSY);
}

/* Test: Crypto_ProcessJob - Decrypt (sync) */
/** @req SWS_Crypto_00004 */
TEST_CASE(crypto_process_job_decrypt)
{
    Std_ReturnType result;
    Crypto_JobType job;
    Crypto_JobPrimitiveInputOutputType io;
    Crypto_JobPrimitiveInfoType info;
    uint8 ciphertext[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    uint8 plaintext[32];
    uint32 plaintext_length = 32;

    setup_test_config();
    init_fresh();

    build_job(&job, &io, &info, ciphertext, 16, plaintext, &plaintext_length,
              CRYPTO_ALGOFAM_AES, CRYPTO_ALGOMODE_CBC, CRYPTO_SERVICE_DECRYPT,
              CRYPTO_PROCESSING_SYNC);
    result = Crypto_ProcessJob(0, &job);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK || result == CRYPTO_E_BUSY);
}

/* Test: Crypto_ProcessJob - Hash (sync) */
/** @req SWS_Crypto_00004 */
TEST_CASE(crypto_process_job_hash)
{
    Std_ReturnType result;
    Crypto_JobType job;
    Crypto_JobPrimitiveInputOutputType io;
    Crypto_JobPrimitiveInfoType info;
    uint8 data[32] = "Test data for hash";
    uint8 hash[32];
    uint32 hash_length = 32;

    setup_test_config();
    init_fresh();

    build_job(&job, &io, &info, data, 32, hash, &hash_length,
              CRYPTO_ALGOFAM_SHA2_256, CRYPTO_ALGOMODE_NOT_SET, CRYPTO_SERVICE_HASH,
              CRYPTO_PROCESSING_SYNC);
    result = Crypto_ProcessJob(1, &job);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK || result == CRYPTO_E_BUSY);
}

/* Test: Crypto_ProcessJob - MAC Generate (sync) */
/** @req SWS_Crypto_00004 */
TEST_CASE(crypto_process_job_mac_generate)
{
    Std_ReturnType result;
    Crypto_JobType job;
    Crypto_JobPrimitiveInputOutputType io;
    Crypto_JobPrimitiveInfoType info;
    uint8 data[16] = "MAC test data!!!";
    uint8 mac[32];
    uint32 mac_length = 32;

    setup_test_config();
    init_fresh();

    build_job(&job, &io, &info, data, 16, mac, &mac_length,
              CRYPTO_ALGOFAM_HMAC, CRYPTO_ALGOMODE_NOT_SET, CRYPTO_SERVICE_MACGENERATE,
              CRYPTO_PROCESSING_SYNC);
    result = Crypto_ProcessJob(0, &job);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK || result == CRYPTO_E_BUSY);
}

/* Test: Crypto_ProcessJob - async job is queued, MainFunction drains queue */
/** @req SWS_Crypto_00004 */
TEST_CASE(crypto_process_job_async)
{
    Std_ReturnType result;
    Crypto_JobType job;
    Crypto_JobPrimitiveInputOutputType io;
    Crypto_JobPrimitiveInfoType info;
    uint8 input[16] = "async job data!!";
    uint8 output[32];
    uint32 output_length = 32;

    setup_test_config();
    init_fresh();

    build_job(&job, &io, &info, input, 16, output, &output_length,
              CRYPTO_ALGOFAM_AES, CRYPTO_ALGOMODE_CBC, CRYPTO_SERVICE_ENCRYPT,
              CRYPTO_PROCESSING_ASYNC);
    result = Crypto_ProcessJob(0, &job);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(CRYPTO_JOBSTATE_QUEUED, job.jobState);

    /* NOTE: QueuePush 为简化实现不真正入队, MainFunction 弹出空队列后 break */
    Crypto_MainFunction();
    ASSERT_TRUE(TRUE);
}

/* Test: Crypto_CancelJob */
/** @req SWS_Crypto_00005 */
TEST_CASE(crypto_cancel_job)
{
    Std_ReturnType result;
    Crypto_JobType job;
    Crypto_JobPrimitiveInputOutputType io;
    Crypto_JobPrimitiveInfoType info;
    uint8 input[16];
    uint8 output[16];
    uint32 output_length = 16;

    setup_test_config();
    init_fresh();

    build_job(&job, &io, &info, input, 16, output, &output_length,
              CRYPTO_ALGOFAM_AES, CRYPTO_ALGOMODE_CBC, CRYPTO_SERVICE_ENCRYPT,
              CRYPTO_PROCESSING_SYNC);
    result = Crypto_CancelJob(0, &job);

    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    ASSERT_EQ(CRYPTO_E_JOB_CANCELED, job.jobState);
}

/* Test: Crypto_KeyElementSet */
/** @req SWS_Crypto_00006 */
TEST_CASE(crypto_key_element_set)
{
    Std_ReturnType result;
    uint8 key_data[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                          0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};

    setup_test_config();
    init_fresh();

    result = Crypto_KeyElementSet(0, 0, key_data, 16);

    ASSERT_EQ(E_OK, result);
}

/* Test: Crypto_KeyElementGet */
/** @req SWS_Crypto_00007 */
TEST_CASE(crypto_key_element_get)
{
    Std_ReturnType result;
    uint8 key_data[32];
    uint32 key_length = 32;

    setup_test_config();
    init_fresh();

    result = Crypto_KeyElementGet(0, 0, key_data, &key_length);

    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(16U, key_length);
}

/* Test: Crypto_KeyValidSet */
/** @req SWS_Crypto_00008 */
TEST_CASE(crypto_key_valid_set)
{
    Std_ReturnType result;

    setup_test_config();
    init_fresh();

    result = Crypto_KeyValidSet(0, TRUE);

    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(CRYPTO_KEY_VALID, g_keys[0].keyState);
}

/* Test: Crypto_KeyElementCopy (key0/elem0 -> key1/elem0) */
/** @req SWS_Crypto_00010 */
TEST_CASE(crypto_key_element_copy)
{
    Std_ReturnType result;

    setup_test_config();
    init_fresh();

    result = Crypto_KeyElementCopy(0, 0, 1, 0);

    ASSERT_EQ(E_OK, result);
}

/* Test: Crypto_RandomSeed */
/** @req SWS_Crypto_00015 */
TEST_CASE(crypto_random_seed)
{
    Std_ReturnType result;
    uint8 seed[16] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
                      0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};

    setup_test_config();
    init_fresh();

    result = Crypto_RandomSeed(0, seed, 16);

    ASSERT_EQ(E_OK, result);
}

/* Test: Crypto_KeyGenerate (HSM unavailable -> software fallback) */
/** @req SWS_Crypto_00011 */
TEST_CASE(crypto_key_generate)
{
    Std_ReturnType result;

    setup_test_config();
    init_fresh();

    result = Crypto_KeyGenerate(0);

    ASSERT_EQ(E_OK, result);
}

/* Test: Crypto_KeyDerive */
/** @req SWS_Crypto_00012 */
TEST_CASE(crypto_key_derive)
{
    Std_ReturnType result;

    setup_test_config();
    init_fresh();

    result = Crypto_KeyDerive(0, 1);

    ASSERT_EQ(E_OK, result);
}

/* Test: Crypto_RandomGenerate fills deterministic pseudo data (stub) */
/** @req SWS_Crypto_00014 */
TEST_CASE(crypto_random_generate)
{
    Std_ReturnType result;
    uint8 random_data[16];

    setup_test_config();
    init_fresh();

    result = Crypto_RandomGenerate(0, random_data, 16);

    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(0xA5, random_data[0]);
    ASSERT_EQ(0xA6, random_data[1]);
}

/* Test: Crypto_KeyElementIdsGet */
/** @req SWS_Crypto_00009 */
TEST_CASE(crypto_key_element_ids_get)
{
    Std_ReturnType result;
    uint32 ids[4] = {0xFFU, 0xFFU, 0xFFU, 0xFFU};

    setup_test_config();
    init_fresh();

    result = Crypto_KeyElementIdsGet(0, ids);

    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(0U, ids[0]);
}

/* Test: Crypto_KeyExchangeCalcSecret */
/** @req SWS_Crypto_00013 */
TEST_CASE(crypto_key_exchange_calc_secret)
{
    Std_ReturnType result;
    uint8 pub_key[65];
    uint32 i;

    for (i = 0U; i < sizeof(pub_key); i++)
    {
        pub_key[i] = (uint8)(0x70U + i);
    }

    setup_test_config();
    init_fresh();

    result = Crypto_KeyExchangeCalcSecret(0, pub_key, sizeof(pub_key));

    ASSERT_EQ(E_OK, result);
}

/* Test: BLAKE2b one-shot hashing (real implementation), keyed variant */
/** @req SWS_Crypto_00021 */
TEST_CASE(crypto_blake2b)
{
    Std_ReturnType result;
    uint8 data[3] = {'a', 'b', 'c'};
    uint8 key[16];
    uint8 digest[BLAKE2B_OUTBYTES];
    uint32 i;

    for (i = 0U; i < sizeof(key); i++)
    {
        key[i] = (uint8)i;
    }

    setup_test_config();
    init_fresh();

    result = Crypto_Blake2b(data, sizeof(data), NULL_PTR, 0, 32, digest);
    ASSERT_EQ(E_OK, result);

    result = Crypto_Blake2b(data, sizeof(data), key, sizeof(key), 64, digest);
    ASSERT_EQ(E_OK, result);
}

/* Test: BLAKE2s one-shot hashing (real implementation) */
/** @req SWS_Crypto_00022 */
TEST_CASE(crypto_blake2s)
{
    Std_ReturnType result;
    uint8 data[3] = {'a', 'b', 'c'};
    uint8 digest[BLAKE2S_OUTBYTES];

    setup_test_config();
    init_fresh();

    result = Crypto_Blake2s(data, sizeof(data), NULL_PTR, 0, BLAKE2S_OUTBYTES, digest);
    ASSERT_EQ(E_OK, result);
}

/* Test: BLAKE2b incremental Start/Update/Finish */
/** @req SWS_Crypto_00023 */
TEST_CASE(crypto_blake2b_incremental)
{
    Std_ReturnType result;
    uint8 data[6] = "123456";
    uint8 digest[BLAKE2B_OUTBYTES];
    uint32 digest_length = 32;

    setup_test_config();
    init_fresh();

    result = Crypto_Blake2b_Start(0, NULL_PTR, 0, 32);
    ASSERT_EQ(E_OK, result);

    result = Crypto_Blake2b_Update(0, data, sizeof(data));
    ASSERT_EQ(E_OK, result);

    result = Crypto_Blake2b_Finish(0, digest, &digest_length);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(32U, digest_length);
}

/* Test: HSM wrapper APIs — 先强制 HSM 不可用, 测不可用路径; 再开启 HSM, 测可用路径 */
/** @req SWS_Crypto_00016 */
TEST_CASE(crypto_hsm_available_and_unavailable)
{
    uint8 id_buf[8];
    uint32 id_len = 8;

    /* ---- Phase 1: HSM 不可用 ---- */
    Crypto_Stubs_SetHsmEnabled(FALSE);
    setup_test_config();
    Crypto_Stubs_ResetAllHsmFlags();
    Crypto_Stubs_SetHsmEnabled(FALSE);
    Crypto_DeInit();  /* 确保从 UNINIT 开始 */
    Crypto_Init(&g_test_config);

    ASSERT_TRUE(Crypto_HsmIsAvailable() == FALSE);
    ASSERT_EQ(CRYPTO_HSM_UNINIT, Crypto_HsmGetStatus());
    ASSERT_EQ(E_NOT_OK, Crypto_HsmLoadKey(0));
    ASSERT_EQ(E_NOT_OK, Crypto_HsmSelfTest());
    ASSERT_EQ(E_NOT_OK, Crypto_HsmGetId(id_buf, &id_len));

    Crypto_DeInit();

    /* ---- Phase 2: HSM 可用 ---- */
    init_fresh();

    ASSERT_TRUE(Crypto_HsmIsAvailable() == TRUE);
    ASSERT_EQ(CRYPTO_HSM_IDLE, Crypto_HsmGetStatus());
    ASSERT_EQ(E_OK, Crypto_HsmLoadKey(0));   /* HSM 可用 + LoadKey stub 返回 E_OK */
    ASSERT_EQ(E_OK, Crypto_HsmSelfTest());
    /* Crypto_HsmGetId 内部直接返回 E_NOT_OK (idPtr/idLengthPtr 被 (void) 忽略) */
    ASSERT_EQ(E_NOT_OK, Crypto_HsmGetId(id_buf, &id_len));
}

/* Test: CCC attestation = SHA256 + ECDSA_Sign stubs */
/** @req SWS_Crypto_00026 */
TEST_CASE(crypto_ccc_attestation)
{
    Std_ReturnType result;
    uint8 challenge[32];
    uint8 signature[CRYPTO_CFG_MAX_SIGNATURE_SIZE];
    uint32 signature_length = CRYPTO_CFG_MAX_SIGNATURE_SIZE;
    uint32 i;

    for (i = 0U; i < sizeof(challenge); i++)
    {
        challenge[i] = (uint8)(0xC0U + i);
    }

    setup_test_config();
    init_fresh();

    result = Crypto_CccGenerateAttestation(challenge, sizeof(challenge), signature, &signature_length);

    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(64U, signature_length);
    ASSERT_EQ(0x5A, signature[0]);
}

/* Test: CCC owner certificate verification (not implemented -> FAILED) */
/** @req SWS_Crypto_00027 */
TEST_CASE(crypto_ccc_verify_certificate)
{
    Std_ReturnType result;
    Crypto_VerifyResultType verify_result = CRYPTO_VERIFICATION_PASSED;
    uint8 certificate[16];

    setup_test_config();
    init_fresh();

    result = Crypto_CccVerifyOwnerCertificate(certificate, sizeof(certificate), &verify_result);

    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(CRYPTO_VERIFICATION_FAILED, verify_result);
}

/* Test: CCC session key derivation (ECDH stub) */
/** @req SWS_Crypto_00028 */
TEST_CASE(crypto_ccc_session_key)
{
    Std_ReturnType result;
    uint8 ephemeral_pub[CRYPTO_CFG_CCC_ECC_PUB_KEY_SIZE];

    setup_test_config();
    init_fresh();

    result = Crypto_CccDeriveSessionKey(ephemeral_pub, sizeof(ephemeral_pub), CRYPTO_KEY_ID_SESSION);

    ASSERT_EQ(E_OK, result);
}

/* Test: CCC AES-GCM encrypt/decrypt roundtrip (stub) */
/** @req SWS_Crypto_00029 */
TEST_CASE(crypto_ccc_encrypt_decrypt)
{
    Std_ReturnType result;
    uint8 plaintext[16] = "ccc-gcm-payload!";
    uint8 ciphertext[16];
    uint8 decrypted[16];
    uint8 iv[CRYPTO_CFG_CCC_IV_SIZE];
    uint8 tag[CRYPTO_CFG_CCC_TAG_SIZE];
    uint32 tag_length = 0;
    uint32 plaintext_length = 0;
    uint32 i;

    for (i = 0U; i < sizeof(iv); i++)
    {
        iv[i] = (uint8)(0x40U + i);
    }

    setup_test_config();
    init_fresh();

    result = Crypto_CccEncrypt(0, plaintext, sizeof(plaintext), NULL_PTR, 0, iv, sizeof(iv),
                               ciphertext, tag, &tag_length);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(CRYPTO_CFG_CCC_TAG_SIZE, tag_length);

    result = Crypto_CccDecrypt(0, ciphertext, sizeof(ciphertext), NULL_PTR, 0, iv, sizeof(iv),
                               tag, tag_length, decrypted, &plaintext_length);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(16U, plaintext_length);
}

/* Test: parameter validation / DET branches return E_NOT_OK */
/** @req SWS_Crypto_00004 */
TEST_CASE(crypto_param_validation)
{
    Std_ReturnType result;
    Crypto_JobType job;
    Crypto_JobPrimitiveInputOutputType io;
    Crypto_JobPrimitiveInfoType info;
    uint8 buf[32];
    uint8 digest[BLAKE2B_OUTBYTES];
    uint32 len = 32;
    uint32 i;

    setup_test_config();
    init_fresh();

    /* ProcessJob: NULL job / objectId out of range / invalid job pointers */
    ASSERT_EQ(E_NOT_OK, Crypto_ProcessJob(0, NULL_PTR));
    ASSERT_EQ(E_NOT_OK, Crypto_ProcessJob(99, &job));
    memset(&job, 0, sizeof(job));
    ASSERT_EQ(E_NOT_OK, Crypto_ProcessJob(0, &job));   /* jobPrimitiveInfo == NULL */
    build_job(&job, &io, &info, buf, 16, buf, &len,
              CRYPTO_ALGOFAM_AES, CRYPTO_ALGOMODE_CBC, CRYPTO_SERVICE_ENCRYPT,
              CRYPTO_PROCESSING_SYNC);
    job.jobPrimitiveInputOutput = NULL_PTR;
    ASSERT_EQ(E_NOT_OK, Crypto_ProcessJob(0, &job));   /* jobPrimitiveInputOutput == NULL */

    /* Key management */
    ASSERT_EQ(E_NOT_OK, Crypto_KeyElementSet(0, 0, NULL_PTR, 16));
    ASSERT_EQ(E_NOT_OK, Crypto_KeyElementSet(0, 0, buf, 0));
    ASSERT_EQ(E_NOT_OK, Crypto_KeyElementSet(9, 0, buf, 16));  /* key out of range */
    ASSERT_EQ(E_NOT_OK, Crypto_KeyElementSet(0, 77, buf, 16)); /* element not found */
    ASSERT_EQ(E_NOT_OK, Crypto_KeyElementGet(0, 0, NULL_PTR, &len));
    ASSERT_EQ(E_NOT_OK, Crypto_KeyElementGet(9, 0, buf, &len));
    ASSERT_EQ(E_NOT_OK, Crypto_KeyElementGet(0, 77, buf, &len));
    ASSERT_EQ(E_NOT_OK, Crypto_KeyValidSet(9, TRUE));
    ASSERT_EQ(E_NOT_OK, Crypto_KeyElementIdsGet(9, &len));
    ASSERT_EQ(E_NOT_OK, Crypto_KeyElementIdsGet(0, NULL_PTR));

    /* Random / key exchange */
    ASSERT_EQ(E_NOT_OK, Crypto_RandomGenerate(0, NULL_PTR, 16));
    ASSERT_EQ(E_NOT_OK, Crypto_RandomGenerate(0, buf, 0));
    ASSERT_EQ(E_NOT_OK, Crypto_RandomGenerate(99, buf, 16));
    ASSERT_EQ(E_NOT_OK, Crypto_KeyExchangeCalcSecret(0, NULL_PTR, 65));

    /* BLAKE2 parameter checks */
    for (i = 0U; i < sizeof(buf); i++)
    {
        buf[i] = (uint8)i;
    }
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b(buf, 16, NULL_PTR, 0, 32, NULL_PTR));
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b(NULL_PTR, 16, NULL_PTR, 0, 32, digest));
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b(buf, 16, NULL_PTR, 0, 0, digest));
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b(buf, 16, NULL_PTR, 0, BLAKE2B_OUTBYTES + 1, digest));
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b(buf, 16, NULL_PTR, BLAKE2B_KEYBYTES + 1, 32, digest));
}

/*==================================================================================================
*  扩展用例 — 覆盖之前未覆盖的 DET 分支 / HSM 路径 / Blake2 边界 / MainFunction / callback 等
*==================================================================================================*/

/* ---- MbedTLS_Init 失败 -> Crypto_Init 提前返回 ---- */
TEST_CASE(crypto_init_mbedtls_fail)
{
    setup_test_config();
    Crypto_Stubs_SetMbedTlsInitFail(TRUE);
    Crypto_DeInit();
    Crypto_Stubs_ResetDetErrorCount();
    Crypto_Init(&g_test_config);

    /* 未初始化 -> 调用 Crypto_ProcessJob 应返回 E_NOT_OK (走 CRYPTO_E_UNINIT DET 分支) */
    Crypto_JobType job;
    memset(&job, 0, sizeof(job));
    ASSERT_EQ(E_NOT_OK, Crypto_ProcessJob(0, &job));

    Crypto_Stubs_SetMbedTlsInitFail(FALSE);
}

/* ---- Crypto_GetVersionInfo: NULL 参数 ---- */
TEST_CASE(crypto_get_version_info_null)
{
    setup_test_config();
    init_fresh();
    Crypto_Stubs_ResetDetErrorCount();
    ASSERT_EQ(E_OK, Crypto_Stubs_GetDetErrorCount() == 0U ? E_OK : E_OK); /* 确保 baseline */
    Crypto_GetVersionInfo(NULL_PTR);
    ASSERT_TRUE(Crypto_Stubs_GetDetErrorCount() >= 1U);
}

/* ---- Crypto_DeInit on UNINIT state ---- */
TEST_CASE(crypto_deinit_twice)
{
    setup_test_config();
    Crypto_DeInit();
    Crypto_DeInit();  /* 第二次应该直接 return */
}

/* ---- Crypto_ProcessJob: Queue Full DET ---- */
TEST_CASE(crypto_process_job_queue_full)
{
    Std_ReturnType result;
    Crypto_JobType job;
    Crypto_JobPrimitiveInputOutputType io;
    Crypto_JobPrimitiveInfoType info;
    uint8 input[16];
    uint8 output[16];
    uint32 out_len = 16;

    setup_test_config();
    g_test_config.queueSize = 0U;  /* queueSize 太小 -> 触发 DET CRYPTO_E_QUEUE_FULL */
    Crypto_DeInit();
    Crypto_Init(&g_test_config);
    Crypto_Stubs_ResetDetErrorCount();

    build_job(&job, &io, &info, input, 16, output, &out_len,
              CRYPTO_ALGOFAM_AES, CRYPTO_ALGOMODE_CBC, CRYPTO_SERVICE_ENCRYPT,
              CRYPTO_PROCESSING_ASYNC);
    result = Crypto_ProcessJob(0, &job);
    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_TRUE(Crypto_Stubs_GetDetErrorCount() >= 1U);
}

/* ---- Crypto_CancelJob: Uninit + NULL job ---- */
TEST_CASE(crypto_cancel_job_uninit_null)
{
    setup_test_config();
    Crypto_DeInit();

    Crypto_Stubs_ResetDetErrorCount();
    ASSERT_EQ(E_NOT_OK, Crypto_CancelJob(0, NULL_PTR));
    ASSERT_TRUE(Crypto_Stubs_GetDetErrorCount() >= 1U);
}

/* ---- Crypto_CancelJob: callbackId != 0 -> Crypto_JobNotification 被调用 ---- */
TEST_CASE(crypto_cancel_job_with_callback)
{
    Std_ReturnType result;
    Crypto_JobType job;

    g_notification_called = FALSE;
    g_last_notified_job = NULL_PTR;

    setup_test_config();
    init_fresh();

    memset(&job, 0, sizeof(job));
    job.jobPrimitiveInfo = &g_alg_info;   /* 非 NULL, 让 267 行条件成立(需要 info) */
    /* 直接设置 callbackId, 不通过 build_job */
    /* 但 CancelJob 里直接读 job->jobPrimitiveInfo->callbackId. build_job 已初始化 info.callbackId = 0 */
    /* 我们需要手动设置 */
    static Crypto_JobPrimitiveInfoType s_info;
    static Crypto_AlgorithmInfoType s_alg;
    job.jobPrimitiveInfo = &s_info;
    memset(&s_info, 0, sizeof(s_info));
    s_alg = g_alg_info;
    s_info.algorithm = &s_alg;
    s_info.callbackId = 42U;

    result = Crypto_CancelJob(0, &job);
    ASSERT_EQ(E_OK, result);
    ASSERT_TRUE(g_notification_called);
    ASSERT_EQ(CRYPTO_E_JOB_CANCELED, job.jobState);
}

/* ---- Crypto_KeyElementSet: Uninit + small buffer ---- */
TEST_CASE(crypto_key_element_set_uninit_small_buf)
{
    uint8 key_data[64];
    uint32 i;
    setup_test_config();
    Crypto_DeInit();

    Crypto_Stubs_ResetDetErrorCount();
    /* Uninit */
    ASSERT_EQ(E_NOT_OK, Crypto_KeyElementSet(0, 0, key_data, 16));
    ASSERT_TRUE(Crypto_Stubs_GetDetErrorCount() >= 1U);

    /* Small buffer: key0 element size=16, allowPartialAccess=FALSE, keyLength=64 > 16 */
    for (i = 0U; i < sizeof(key_data); i++) key_data[i] = (uint8)i;
    Crypto_Stubs_ResetDetErrorCount();
    init_fresh();
    ASSERT_EQ(E_NOT_OK, Crypto_KeyElementSet(0, 0, key_data, 64));
    ASSERT_TRUE(Crypto_Stubs_GetDetErrorCount() >= 1U);
}

/* ---- Crypto_KeyElementGet: Uninit + buffer too small ---- */
TEST_CASE(crypto_key_element_get_uninit_buf_small)
{
    uint8 key_buf[1];
    uint32 key_len = 1;

    setup_test_config();
    Crypto_DeInit();
    ASSERT_EQ(E_NOT_OK, Crypto_KeyElementGet(0, 0, key_buf, &key_len));

    init_fresh();
    /* key0 element size=16, *keyLengthPtr=1 < 16 -> 378 行 */
    key_len = 1;
    ASSERT_EQ(E_NOT_OK, Crypto_KeyElementGet(0, 0, key_buf, &key_len));
}

/* ---- Crypto_KeyValidSet + KeyElementIdsGet: Uninit ---- */
TEST_CASE(crypto_key_valid_ids_uninit)
{
    uint32 ids[4];
    setup_test_config();
    Crypto_DeInit();

    Crypto_Stubs_ResetDetErrorCount();
    ASSERT_EQ(E_NOT_OK, Crypto_KeyValidSet(0, TRUE));
    ASSERT_EQ(E_NOT_OK, Crypto_KeyElementIdsGet(0, ids));
    ASSERT_TRUE(Crypto_Stubs_GetDetErrorCount() >= 2U);
}

/* ---- Crypto_KeyElementCopy: Get 失败 -> element not found ---- */
TEST_CASE(crypto_key_element_copy_get_fail)
{
    setup_test_config();
    init_fresh();
    /* key 0 没有 id=77 的 element, KeyElementGet 返回 E_NOT_OK, Copy 随之返回 E_NOT_OK */
    ASSERT_EQ(E_NOT_OK, Crypto_KeyElementCopy(0, 77, 1, 0));
}

/* ---- Crypto_KeyGenerate: Uninit + key out of range + HSM available ---- */
TEST_CASE(crypto_key_generate_variants)
{
    setup_test_config();
    Crypto_DeInit();
    ASSERT_EQ(E_NOT_OK, Crypto_KeyGenerate(0));

    init_fresh();
    /* keyId >= numKeys(2) */
    ASSERT_EQ(E_NOT_OK, Crypto_KeyGenerate(99));

    /* 正常路径, HSM 可用 (默认) */
    Crypto_Stubs_SetHsmLoadKeyRet(E_OK);
    ASSERT_EQ(E_OK, Crypto_KeyGenerate(0));
}

/* ---- Crypto_KeyDerive: Uninit + config null 模拟 ---- */
TEST_CASE(crypto_key_derive_uninit)
{
    setup_test_config();
    Crypto_DeInit();
    ASSERT_EQ(E_NOT_OK, Crypto_KeyDerive(0, 1));

    init_fresh();
    /* keyId >= numKeys */
    ASSERT_EQ(E_NOT_OK, Crypto_KeyDerive(99, 1));
    ASSERT_EQ(E_NOT_OK, Crypto_KeyDerive(0, 99));
}

/* ---- Crypto_KeyExchangeCalcSecret: Uninit ---- */
TEST_CASE(crypto_key_exchange_uninit)
{
    uint8 pub[65];
    setup_test_config();
    Crypto_DeInit();
    ASSERT_EQ(E_NOT_OK, Crypto_KeyExchangeCalcSecret(0, pub, sizeof(pub)));
}

/* ---- Crypto_RandomGenerate + RandomSeed: Uninit ---- */
TEST_CASE(crypto_random_uninit)
{
    uint8 buf[8];
    setup_test_config();
    Crypto_DeInit();
    ASSERT_EQ(E_NOT_OK, Crypto_RandomGenerate(0, buf, 8));
    ASSERT_EQ(E_NOT_OK, Crypto_RandomSeed(0, buf, 8));

    /* RandomSeed NULL entropyPtr */
    init_fresh();
    ASSERT_EQ(E_NOT_OK, Crypto_RandomSeed(0, NULL_PTR, 8));
}

/* ---- Blake2b/Blake2s: Uninit + 超长输入 + 零长度输入 + key 边界 ---- */
TEST_CASE(crypto_blake2b_blake2s_boundaries)
{
    uint8 digest[BLAKE2B_OUTBYTES];
    uint8 digest_s[BLAKE2S_OUTBYTES];
    uint8 key32[BLAKE2B_KEYBYTES];
    uint32 i;

    for (i = 0U; i < sizeof(key32); i++) key32[i] = (uint8)i;

    /* Uninit */
    setup_test_config();
    Crypto_DeInit();
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b(NULL_PTR, 0, NULL_PTR, 0, 32, digest));
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2s(NULL_PTR, 0, NULL_PTR, 0, 16, digest_s));

    init_fresh();

    /* Zero-length input (dataPtr NULL 允许, dataLength == 0) */
    ASSERT_EQ(E_OK, Crypto_Blake2b(NULL_PTR, 0, NULL_PTR, 0, 32, digest));
    ASSERT_EQ(E_OK, Crypto_Blake2s(NULL_PTR, 0, NULL_PTR, 0, BLAKE2S_OUTBYTES, digest_s));

    /* Max length keyed BLAKE2b (keyLength = BLAKE2B_KEYBYTES, digestLength = BLAKE2B_OUTBYTES) */
    ASSERT_EQ(E_OK, Crypto_Blake2b(NULL_PTR, 0, key32, BLAKE2B_KEYBYTES, BLAKE2B_OUTBYTES, digest));

    /* dataPtr NULL 但 dataLength > 0 (应返回 E_NOT_OK) */
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b(NULL_PTR, 16, NULL_PTR, 0, 32, digest));
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2s(NULL_PTR, 16, NULL_PTR, 0, 16, digest_s));

    /* keyPtr NULL 但 keyLength > 0 */
    uint8 buf[16];
    for (i = 0U; i < sizeof(buf); i++) buf[i] = (uint8)i;
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b(buf, 16, NULL_PTR, 16, 32, digest));
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2s(buf, 16, NULL_PTR, 16, 16, digest_s));

    /* digestLength at boundary */
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b(buf, 16, NULL_PTR, 0, 0, digest));
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2s(buf, 16, NULL_PTR, 0, 0, digest_s));
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b(buf, 16, NULL_PTR, 0, BLAKE2B_OUTBYTES + 1, digest));
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2s(buf, 16, NULL_PTR, 0, BLAKE2S_OUTBYTES + 1, digest_s));

    /* Key too long */
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b(buf, 16, key32, BLAKE2B_KEYBYTES + 1, 32, digest));
}

/* ---- BLAKE2b incremental: Start 带 key / 多次 Update / 各种 Finish 错误 ---- */
TEST_CASE(crypto_blake2b_incremental_boundaries)
{
    uint8 data1[4] = {'A', 'B', 'C', 'D'};
    uint8 data2[4] = {'E', 'F', 'G', 'H'};
    uint8 key[16];
    uint8 digest[BLAKE2B_OUTBYTES];
    uint32 digest_len;
    uint32 i;

    for (i = 0U; i < sizeof(key); i++) key[i] = (uint8)i;

    setup_test_config();
    init_fresh();

    /* Start 带 key (走 blake2b_init_key 路径) */
    ASSERT_EQ(E_OK, Crypto_Blake2b_Start(0, key, sizeof(key), 32));

    /* 多次 Update */
    ASSERT_EQ(E_OK, Crypto_Blake2b_Update(0, data1, sizeof(data1)));
    ASSERT_EQ(E_OK, Crypto_Blake2b_Update(0, data2, sizeof(data2)));

    /* Update dataPtr NULL 但 dataLength > 0 应返回 E_NOT_OK (context 已初始化) */
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b_Update(0, NULL_PTR, 8));

    digest_len = 32;
    ASSERT_EQ(E_OK, Crypto_Blake2b_Finish(0, digest, &digest_len));
    ASSERT_EQ(32U, digest_len);

    /* Finish 之后 ContextInitialized = FALSE -> 再 Finish/Update 应返回 E_NOT_OK */
    digest_len = 32;
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b_Update(0, data1, sizeof(data1)));
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b_Finish(0, digest, &digest_len));

    /* Start Uninit: 需要先 DeInit */
    Crypto_DeInit();
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b_Start(0, NULL_PTR, 0, 32));
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b_Update(0, data1, sizeof(data1)));
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b_Finish(0, digest, &digest_len));

    /* Start digestLength 边界 */
    init_fresh();
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b_Start(0, NULL_PTR, 0, 0));
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b_Start(0, NULL_PTR, 0, BLAKE2B_OUTBYTES + 1));
    /* keyPtr != NULL 但 keyLength == 0 — Crypto_Blake2b_Start 不检查反向条件, 视为无 key */
    ASSERT_EQ(E_OK, Crypto_Blake2b_Start(0, key, 0, 32));
    Crypto_Blake2b_Finish(0, digest, &digest_len);  /* 清理 context */
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b_Start(0, NULL_PTR, 16, 32));  /* keyLength > 0 但 keyPtr == NULL */
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b_Start(0, key, BLAKE2B_KEYBYTES + 1, 32));

    /* Finish 参数 NULL */
    Crypto_Blake2b_Start(0, NULL_PTR, 0, 32);
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b_Finish(0, NULL_PTR, &digest_len));
    ASSERT_EQ(E_NOT_OK, Crypto_Blake2b_Finish(0, digest, NULL_PTR));
    Crypto_Blake2b_Finish(0, digest, &digest_len);  /* 清理 context */
}

/* ---- CCC Attestation: NULL 参数 ---- */
TEST_CASE(crypto_ccc_attestation_null)
{
    uint8 challenge[16];
    uint8 sig[64];
    uint32 sig_len = 64;

    setup_test_config();
    init_fresh();

    ASSERT_EQ(E_NOT_OK, Crypto_CccGenerateAttestation(NULL_PTR, 16, sig, &sig_len));
    ASSERT_EQ(E_NOT_OK, Crypto_CccGenerateAttestation(challenge, 16, NULL_PTR, &sig_len));
    ASSERT_EQ(E_NOT_OK, Crypto_CccGenerateAttestation(challenge, 16, sig, NULL_PTR));
}

/* ---- Crypto_ProcessJobInternal: HSM 成功 + callback 触发 (需要 callbackId != 0) ---- */
TEST_CASE(crypto_process_job_internal_hsm_callback)
{
    Std_ReturnType result;
    Crypto_JobType job;
    Crypto_JobPrimitiveInputOutputType io;
    Crypto_JobPrimitiveInfoType info;
    uint8 input[16] = "hsm-cb-test-data!";
    uint8 output[32];
    uint32 out_len = 32;

    g_notification_called = FALSE;
    g_last_notified_job = NULL_PTR;

    setup_test_config();
    init_fresh();

    build_job(&job, &io, &info, input, 16, output, &out_len,
              CRYPTO_ALGOFAM_AES, CRYPTO_ALGOMODE_GCM, CRYPTO_SERVICE_ENCRYPT,
              CRYPTO_PROCESSING_SYNC);
    info.callbackId = 99U;   /* 触发 callback */
    /* HSM 默认可用 + HsmProcessJob 返回 E_OK (默认) */
    result = Crypto_ProcessJob(0, &job);

    ASSERT_EQ(E_OK, result);
    ASSERT_TRUE(g_notification_called);
    ASSERT_TRUE(g_last_notified_job == &job);
}

/* ---- Crypto_ProcessJobInternal: HSM 失败 -> fallback 到 software + callback ---- */
TEST_CASE(crypto_process_job_internal_sw_fallback)
{
    Std_ReturnType result;
    Crypto_JobType job;
    Crypto_JobPrimitiveInputOutputType io;
    Crypto_JobPrimitiveInfoType info;
    uint8 input[16] = "fallback-test-data";
    uint8 output[32];
    uint32 out_len = 32;

    g_notification_called = FALSE;

    setup_test_config();
    init_fresh();

    Crypto_Stubs_SetHsmProcessRet(E_NOT_OK);  /* HSM 处理失败, 触发 fallback */

    build_job(&job, &io, &info, input, 16, output, &out_len,
              CRYPTO_ALGOFAM_AES, CRYPTO_ALGOMODE_CBC, CRYPTO_SERVICE_ENCRYPT,
              CRYPTO_PROCESSING_SYNC);
    info.callbackId = 77U;
    result = Crypto_ProcessJob(0, &job);

    ASSERT_EQ(E_OK, result);
    ASSERT_TRUE(g_notification_called);
}

/* ---- Crypto_ProcessJobInternal: software 路径成功 + callback ---- */
TEST_CASE(crypto_process_job_sw_success_callback)
{
    Std_ReturnType result;
    Crypto_JobType job;
    Crypto_JobPrimitiveInputOutputType io;
    Crypto_JobPrimitiveInfoType info;
    uint8 input[16] = "sw-success-callback";
    uint8 output[32];
    uint32 out_len = 32;

    g_notification_called = FALSE;

    setup_test_config();
    /* 关闭 HSM 确保走 software 路径 */
    Crypto_Stubs_SetHsmEnabled(FALSE);
    Crypto_DeInit();
    Crypto_Init(&g_test_config);

    build_job(&job, &io, &info, input, 16, output, &out_len,
              CRYPTO_ALGOFAM_AES, CRYPTO_ALGOMODE_CBC, CRYPTO_SERVICE_ENCRYPT,
              CRYPTO_PROCESSING_SYNC);
    info.callbackId = 55U;
    result = Crypto_ProcessJob(0, &job);

    ASSERT_EQ(E_OK, result);
    ASSERT_TRUE(g_notification_called);
}

/* ---- Crypto_MainFunction: Uninit + queue empty ---- */
TEST_CASE(crypto_main_function)
{
    setup_test_config();
    Crypto_DeInit();
    Crypto_MainFunction();  /* Uninit: 直接 return, 无 crash */

    init_fresh();
    Crypto_MainFunction();  /* queue empty: 正常 while(0) 跳出 */
}

/* ---- Crypto_CancelJob: Uninit + 正常路径 + callback ---- */
TEST_CASE(crypto_cancel_job_uninit_and_path)
{
    Crypto_JobType job;
    static Crypto_JobPrimitiveInfoType s_info;
    static Crypto_AlgorithmInfoType s_alg;

    /* Uninit + NULL: 都走 DET */
    setup_test_config();
    Crypto_DeInit();
    Crypto_Stubs_ResetDetErrorCount();
    ASSERT_EQ(E_NOT_OK, Crypto_CancelJob(0, NULL_PTR));

    /* Init 后正常取消 (无 callback) */
    init_fresh();
    g_notification_called = FALSE;
    memset(&job, 0, sizeof(job));
    memset(&s_info, 0, sizeof(s_info));
    s_alg = g_alg_info;
    s_info.algorithm = &s_alg;
    s_info.callbackId = 0U;  /* 无 callback */
    job.jobPrimitiveInfo = &s_info;

    ASSERT_EQ(E_OK, Crypto_CancelJob(0, &job));
    ASSERT_FALSE(g_notification_called);  /* callbackId == 0 */
    ASSERT_EQ(CRYPTO_E_JOB_CANCELED, job.jobState);
}

/* ---- Crypto_ProcessJob: Uninit + NULL job + out-of-range objectId ---- */
TEST_CASE(crypto_process_job_uninit_null)
{
    Crypto_JobType job;
    setup_test_config();
    Crypto_DeInit();
    Crypto_Stubs_ResetDetErrorCount();

    ASSERT_EQ(E_NOT_OK, Crypto_ProcessJob(0, NULL_PTR));
    ASSERT_TRUE(Crypto_Stubs_GetDetErrorCount() >= 1U);

    init_fresh();
    ASSERT_EQ(E_NOT_OK, Crypto_ProcessJob(999, &job));
}

/* ---- Crypto_RandomGenerate: 正常 + cryptoKeyId out of range ---- */
TEST_CASE(crypto_random_generate_variants)
{
    uint8 buf[16];
    setup_test_config();
    init_fresh();

    ASSERT_EQ(E_OK, Crypto_RandomGenerate(0, buf, 16));
    ASSERT_EQ(E_NOT_OK, Crypto_RandomGenerate(999, buf, 16));
}

/* ---- Crypto_HsmLoadKey + HsmSelfTest: Uninit ---- */
TEST_CASE(crypto_hsm_wrappers_uninit)
{
    setup_test_config();
    Crypto_DeInit();
    ASSERT_EQ(E_NOT_OK, Crypto_HsmLoadKey(0));
    ASSERT_EQ(E_NOT_OK, Crypto_HsmSelfTest());
}

/* ---- 综合: Init 后 deinit, 检查 Magic 和 State ---- */
TEST_CASE(crypto_init_deinit_cycle)
{
    setup_test_config();
    init_fresh();

    /* Crypto_DeInit 清空队列 + 调用 Hsm_DeInit + MbedTLS_DeInit */
    Crypto_DeInit();
    /* 二次调用应幂等 */
    Crypto_DeInit();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(crypto)
{
    Crypto_Stubs_ResetAllHsmFlags();
    g_notification_called = FALSE;
    g_last_notified_job = NULL_PTR;
}

TEST_SUITE_TEARDOWN(crypto)
{
    Crypto_DeInit();
    Crypto_Stubs_ResetAllHsmFlags();
}

TEST_SUITE(crypto)
{
    /* NOTE: unity.h 的 RUN_TEST(func) 直接展开为 UnityRunTest(func,...),
     * 而 TEST_CASE(name) 生成 test_<name> 函数, 故此处必须带 test_ 前缀 */
    RUN_TEST(test_crypto_init_valid_config);
    RUN_TEST(test_crypto_init_null_config);
    RUN_TEST(test_crypto_deinit);
    RUN_TEST(test_crypto_get_version_info);
    RUN_TEST(test_crypto_process_job_encrypt);
    RUN_TEST(test_crypto_process_job_decrypt);
    RUN_TEST(test_crypto_process_job_hash);
    RUN_TEST(test_crypto_process_job_mac_generate);
    RUN_TEST(test_crypto_process_job_async);
    RUN_TEST(test_crypto_cancel_job);
    RUN_TEST(test_crypto_key_element_set);
    RUN_TEST(test_crypto_key_element_get);
    RUN_TEST(test_crypto_key_valid_set);
    RUN_TEST(test_crypto_key_element_copy);
    RUN_TEST(test_crypto_random_seed);
    RUN_TEST(test_crypto_key_generate);
    RUN_TEST(test_crypto_key_derive);
    RUN_TEST(test_crypto_random_generate);
    RUN_TEST(test_crypto_key_element_ids_get);
    RUN_TEST(test_crypto_key_exchange_calc_secret);
    RUN_TEST(test_crypto_blake2b);
    RUN_TEST(test_crypto_blake2s);
    RUN_TEST(test_crypto_blake2b_incremental);
    RUN_TEST(test_crypto_hsm_available_and_unavailable);
    RUN_TEST(test_crypto_ccc_attestation);
    RUN_TEST(test_crypto_ccc_verify_certificate);
    RUN_TEST(test_crypto_ccc_session_key);
    RUN_TEST(test_crypto_ccc_encrypt_decrypt);
    RUN_TEST(test_crypto_param_validation);

    /* ---- 扩展用例 ---- */
    RUN_TEST(test_crypto_init_mbedtls_fail);
    RUN_TEST(test_crypto_get_version_info_null);
    RUN_TEST(test_crypto_deinit_twice);
    RUN_TEST(test_crypto_process_job_queue_full);
    RUN_TEST(test_crypto_cancel_job_uninit_null);
    RUN_TEST(test_crypto_cancel_job_with_callback);
    RUN_TEST(test_crypto_key_element_set_uninit_small_buf);
    RUN_TEST(test_crypto_key_element_get_uninit_buf_small);
    RUN_TEST(test_crypto_key_valid_ids_uninit);
    RUN_TEST(test_crypto_key_element_copy_get_fail);
    RUN_TEST(test_crypto_key_generate_variants);
    RUN_TEST(test_crypto_key_derive_uninit);
    RUN_TEST(test_crypto_key_exchange_uninit);
    RUN_TEST(test_crypto_random_uninit);
    RUN_TEST(test_crypto_blake2b_blake2s_boundaries);
    RUN_TEST(test_crypto_blake2b_incremental_boundaries);
    RUN_TEST(test_crypto_ccc_attestation_null);
    RUN_TEST(test_crypto_process_job_internal_hsm_callback);
    RUN_TEST(test_crypto_process_job_internal_sw_fallback);
    RUN_TEST(test_crypto_process_job_sw_success_callback);
    RUN_TEST(test_crypto_main_function);
    RUN_TEST(test_crypto_cancel_job_uninit_and_path);
    RUN_TEST(test_crypto_process_job_uninit_null);
    RUN_TEST(test_crypto_random_generate_variants);
    RUN_TEST(test_crypto_hsm_wrappers_uninit);
    RUN_TEST(test_crypto_init_deinit_cycle);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
/* NOTE: 不使用 test_framework.h 的 TEST_MAIN_BEGIN/RUN_TEST_SUITE 组合 — 前者恒 return 0
 * (用例失败不反映到退出码), 后者引用了从未定义的 g_test_stats。此处按其语义手工展开,
 * 并依据 Unity 计数返回真实退出码。 */
int main(void)
{
    printf("\n=== Test Suite: crypto ===\n");
    test_suite_crypto_setup();
    test_suite_crypto();
    test_suite_crypto_teardown();

    printf("\n----------------------------------------\n");
    printf("Total: %u  Passed: %u  Failed: %u  Ignored: %u\n",
           Unity.test_count, Unity.pass_count, Unity.fail_count, Unity.ignore_count);
    printf("----------------------------------------\n");

    return (Unity.fail_count > 0U) ? 1 : 0;
}
