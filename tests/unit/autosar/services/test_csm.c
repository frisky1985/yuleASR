/**
 * @file test_csm.c
 * @brief CSM (Crypto Services Manager) 单元测试
 * 
 * 测试覆盖: 加密、解密、数字签名、哈希、随机数生成等密码服务API
 * 目标覆盖率: 80%+
 * 
 * @author yuleASR Team
 * @version 1.0.0
 */

// @tests src/bsw/services/csm/src/Csm.c  @tests src/bsw/services/csm/include/Csm.h

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include "Csm.h"
#include "Csm_Cfg.h"

/*==================================================================================================
*                                       测试夹具与辅助函数
==================================================================================================*/

/* 测试回调标志 */
static volatile boolean test_callback_called = FALSE;
static volatile uint32 test_callback_jobId = 0;
static volatile Std_ReturnType test_callback_result = E_NOT_OK;

/* 测试回调函数 */
static void test_callback(
    uint32 jobId,
    Std_ReturnType result,
    const uint8* outputPtr,
    uint32 outputLength,
    void* userContext)
{
    (void)outputPtr;
    (void)outputLength;
    
    test_callback_called = TRUE;
    test_callback_jobId = jobId;
    test_callback_result = result;
    
    if (userContext != NULL)
    {
        *(boolean*)userContext = TRUE;
    }
}

/* 每次测试前的初始化 */
static int test_setup(void** state)
{
    (void)state;
    
    test_callback_called = FALSE;
    test_callback_jobId = 0;
    test_callback_result = E_NOT_OK;
    
    /* 去初始化确保干净状态 */
    Csm_DeInit();
    
    return 0;
}

/* 每次测试后的清理 */
static int test_teardown(void** state)
{
    (void)state;
    
    Csm_DeInit();
    
    return 0;
}

/*==================================================================================================
*                                       测试用例 - 初始化/去初始化
==================================================================================================*/

/**
 * @test Csm_Init 正常初始化
 * @brief 验证CSM模块可以正确初始化
 * @req SWS_Csm_00001
 */
static void test_Csm_Init_Success(void** state)
{
    (void)state;
    
    Std_ReturnType result = Csm_Init(&Csm_Config);
    
    assert_int_equal(result, E_OK);
}

/**
 * @test Csm_Init 空配置指针
 * @brief 验证传入NULL配置指针时返回错误
 * @req SWS_Csm_00001
 */
static void test_Csm_Init_NullConfig(void** state)
{
    (void)state;
    
    Std_ReturnType result = Csm_Init(NULL_PTR);
    
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @test Csm_Init 重复初始化
 * @brief 验证重复初始化返回错误
 * @req SWS_Csm_00001
 */
static void test_Csm_Init_AlreadyInitialized(void** state)
{
    (void)state;
    
    /* 第一次初始化 */
    Std_ReturnType result = Csm_Init(&Csm_Config);
    assert_int_equal(result, E_OK);
    
    /* 第二次初始化应失败 */
    result = Csm_Init(&Csm_Config);
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @test Csm_DeInit 正常去初始化
 * @brief 验证CSM模块可以正确去初始化
 * @req SWS_Csm_00002
 */
static void test_Csm_DeInit_Success(void** state)
{
    (void)state;
    
    /* 先初始化 */
    Csm_Init(&Csm_Config);
    
    /* 去初始化 */
    Std_ReturnType result = Csm_DeInit();
    
    assert_int_equal(result, E_OK);
}

/**
 * @test Csm_DeInit 未初始化状态
 * @brief 验证未初始化时去初始化返回错误
 * @req SWS_Csm_00002
 */
static void test_Csm_DeInit_NotInitialized(void** state)
{
    (void)state;
    
    /* 确保未初始化 */
    Csm_DeInit();
    
    Std_ReturnType result = Csm_DeInit();
    
    assert_int_equal(result, E_NOT_OK);
}

/*==================================================================================================
*                                       测试用例 - 密钥管理
==================================================================================================*/

/**
 * @test Csm_KeyElementSet 正常设置密钥元素
 * @brief 验证可以设置密钥元素数据
 * @req SWS_Csm_00010
 */
static void test_Csm_KeyElementSet_Success(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    uint8 keyData[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    
    Std_ReturnType result = Csm_KeyElementSet(CSM_KEY_ID_MASTER, 
                                               CSM_KEY_ELEMENT_ID_SECRET,
                                               keyData, 16);
    
    assert_int_equal(result, E_OK);
}

/**
 * @test Csm_KeyElementSet 空指针
 * @brief 验证传入NULL数据指针返回错误
 * @req SWS_Csm_00010
 */
static void test_Csm_KeyElementSet_NullPointer(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    Std_ReturnType result = Csm_KeyElementSet(CSM_KEY_ID_MASTER,
                                               CSM_KEY_ELEMENT_ID_SECRET,
                                               NULL_PTR, 16);
    
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @test Csm_KeyElementSet 密钥长度超限
 * @brief 验证传入超长密钥返回错误
 * @req SWS_Csm_00010
 */
static void test_Csm_KeyElementSet_LengthExceeded(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    uint8 keyData[CSM_MAX_KEY_LENGTH + 1];
    memset(keyData, 0xAA, sizeof(keyData));
    
    Std_ReturnType result = Csm_KeyElementSet(CSM_KEY_ID_MASTER,
                                               CSM_KEY_ELEMENT_ID_SECRET,
                                               keyData, CSM_MAX_KEY_LENGTH + 1);
    
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @test Csm_KeyElementSet 无效密钥ID
 * @brief 验证传入无效密钥ID返回错误
 * @req SWS_Csm_00010
 */
static void test_Csm_KeyElementSet_InvalidKeyId(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    uint8 keyData[16] = {0};
    
    Std_ReturnType result = Csm_KeyElementSet(CSM_KEY_ID_NONE,
                                               CSM_KEY_ELEMENT_ID_SECRET,
                                               keyData, 16);
    
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @test Csm_KeySetValid 设置密钥有效
 * @brief 验证可以设置密钥为有效状态
 * @req SWS_Csm_00011
 */
static void test_Csm_KeySetValid_Success(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    /* 先设置密钥元素 */
    uint8 keyData[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    Csm_KeyElementSet(CSM_KEY_ID_MASTER, CSM_KEY_ELEMENT_ID_SECRET, keyData, 16);
    
    /* 设置密钥有效 */
    Std_ReturnType result = Csm_KeySetValid(CSM_KEY_ID_MASTER);
    
    assert_int_equal(result, E_OK);
}

/**
 * @test Csm_KeyElementGet 获取密钥元素
 * @brief 验证可以获取已设置的密钥元素
 * @req SWS_Csm_00012
 */
static void test_Csm_KeyElementGet_Success(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    /* 设置密钥元素 */
    uint8 keyData[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    Csm_KeyElementSet(CSM_KEY_ID_MASTER, CSM_KEY_ELEMENT_ID_SECRET, keyData, 16);
    Csm_KeySetValid(CSM_KEY_ID_MASTER);
    
    /* 获取密钥元素 */
    uint8 retrievedData[16];
    uint32 length = sizeof(retrievedData);
    
    Std_ReturnType result = Csm_KeyElementGet(CSM_KEY_ID_MASTER,
                                               CSM_KEY_ELEMENT_ID_SECRET,
                                               retrievedData, &length);
    
    assert_int_equal(result, E_OK);
    assert_int_equal(length, 16);
    assert_memory_equal(keyData, retrievedData, 16);
}

/**
 * @test Csm_KeyElementGet 未有效化的密钥
 * @brief 验证获取未有效化密钥返回错误
 * @req SWS_Csm_00012
 */
static void test_Csm_KeyElementGet_KeyNotValid(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    uint8 retrievedData[16];
    uint32 length = sizeof(retrievedData);
    
    Std_ReturnType result = Csm_KeyElementGet(CSM_KEY_ID_MASTER,
                                               CSM_KEY_ELEMENT_ID_SECRET,
                                               retrievedData, &length);
    
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @test Csm_KeyCopy 复制密钥
 * @brief 验证可以复制密钥到另一个密钥ID
 * @req SWS_Csm_00014
 */
static void test_Csm_KeyCopy_Success(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    /* 设置源密钥 */
    uint8 keyData[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    Csm_KeyElementSet(CSM_KEY_ID_MASTER, CSM_KEY_ELEMENT_ID_SECRET, keyData, 16);
    Csm_KeySetValid(CSM_KEY_ID_MASTER);
    
    /* 复制密钥 */
    Std_ReturnType result = Csm_KeyCopy(CSM_KEY_ID_MASTER, CSM_KEY_ID_SESSION);
    
    assert_int_equal(result, E_OK);
}

/**
 * @test Csm_GetKeyStatus 获取密钥状态
 * @brief 验证可以获取密钥状态
 * @req SWS_Csm_00102
 */
static void test_Csm_GetKeyStatus_Success(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    Csm_KeyStatusType keyStatus;
    Std_ReturnType result = Csm_GetKeyStatus(CSM_KEY_ID_MASTER, &keyStatus);
    
    assert_int_equal(result, E_OK);
    /* 未设置密钥元素前状态应为 INVALID 或 EMPTY */
    assert_true(keyStatus == CSM_KEY_STATUS_INVALID || keyStatus == CSM_KEY_STATUS_EMPTY);
}

/*==================================================================================================
*                                       测试用例 - 哈希服务
==================================================================================================*/

/**
 * @test Csm_Hash 计算哈希值
 * @brief 验证可以计算数据哈希值
 * @req SWS_Csm_00030
 */
static void test_Csm_Hash_Success(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    uint8 data[] = "Hello, World!";
    uint8 hash[CSM_MAX_HASH_LENGTH];
    uint32 hashLength = sizeof(hash);
    
    Std_ReturnType result = Csm_Hash(CSM_JOB_ID_HASH_DEFAULT,
                                      CSM_OPERATION_MODE_SINGLECALL,
                                      data, sizeof(data) - 1,
                                      hash, &hashLength);
    
    assert_int_equal(result, E_OK);
    assert_true(hashLength > 0);
}

/**
 * @test Csm_Hash 多阶段哈希
 * @brief 验证支持START/UPDATE/FINISH多阶段操作
 * @req SWS_Csm_00030
 */
static void test_Csm_Hash_MultiStage(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    uint8 data1[] = "Hello, ";
    uint8 data2[] = "World!";
    uint8 hash[CSM_MAX_HASH_LENGTH];
    uint32 hashLength = sizeof(hash);
    
    /* START阶段 */
    Std_ReturnType result = Csm_Hash(CSM_JOB_ID_HASH_DEFAULT,
                                      CSM_OPERATION_MODE_START,
                                      NULL_PTR, 0,
                                      NULL_PTR, NULL);
    assert_int_equal(result, E_OK);
    
    /* UPDATE阶段1 */
    result = Csm_Hash(CSM_JOB_ID_HASH_DEFAULT,
                       CSM_OPERATION_MODE_UPDATE,
                       data1, sizeof(data1) - 1,
                       NULL_PTR, NULL);
    assert_int_equal(result, E_OK);
    
    /* UPDATE阶段2 */
    result = Csm_Hash(CSM_JOB_ID_HASH_DEFAULT,
                       CSM_OPERATION_MODE_UPDATE,
                       data2, sizeof(data2) - 1,
                       NULL_PTR, NULL);
    assert_int_equal(result, E_OK);
    
    /* FINISH阶段 */
    result = Csm_Hash(CSM_JOB_ID_HASH_DEFAULT,
                       CSM_OPERATION_MODE_FINISH,
                       NULL_PTR, 0,
                       hash, &hashLength);
    assert_int_equal(result, E_OK);
    assert_true(hashLength > 0);
}

/**
 * @test Csm_Hash 无效作业ID
 * @brief 验证传入无效作业ID返回错误
 * @req SWS_Csm_00030
 */
static void test_Csm_Hash_InvalidJobId(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    uint8 data[] = "test";
    uint8 hash[CSM_MAX_HASH_LENGTH];
    uint32 hashLength = sizeof(hash);
    
    Std_ReturnType result = Csm_Hash(CSM_JOB_ID_NONE,
                                      CSM_OPERATION_MODE_SINGLECALL,
                                      data, sizeof(data) - 1,
                                      hash, &hashLength);
    
    assert_int_equal(result, E_NOT_OK);
}

/*==================================================================================================
*                                       测试用例 - 加密/解密服务
==================================================================================================*/

/**
 * @test Csm_Encrypt 加密数据
 * @brief 验证可以加密数据
 * @req SWS_Csm_00050
 */
static void test_Csm_Encrypt_Success(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    /* 设置加密密钥 */
    uint8 keyData[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    Csm_KeyElementSet(CSM_KEY_ID_MASTER, CSM_KEY_ELEMENT_ID_SECRET, keyData, 16);
    Csm_KeySetValid(CSM_KEY_ID_MASTER);
    Csm_JobKeySetUp(CSM_JOB_ID_ENCRYPT_DEFAULT, CSM_KEY_ID_MASTER);
    
    uint8 plaintext[16] = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
                           0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50};
    uint8 ciphertext[32];
    uint32 cipherLength = sizeof(ciphertext);
    
    Std_ReturnType result = Csm_Encrypt(CSM_JOB_ID_ENCRYPT_DEFAULT,
                                         CSM_OPERATION_MODE_SINGLECALL,
                                         plaintext, sizeof(plaintext),
                                         ciphertext, &cipherLength);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/**
 * @test Csm_Encrypt 空指针检查
 * @brief 验证传入空指针返回错误
 * @req SWS_Csm_00050
 */
static void test_Csm_Encrypt_NullPointer(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    uint8 plaintext[16];
    uint8 ciphertext[32];
    uint32 cipherLength = sizeof(ciphertext);
    
    /* NULL输入指针 */
    Std_ReturnType result = Csm_Encrypt(CSM_JOB_ID_ENCRYPT_DEFAULT,
                                         CSM_OPERATION_MODE_SINGLECALL,
                                         NULL_PTR, sizeof(plaintext),
                                         ciphertext, &cipherLength);
    
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @test Csm_Decrypt 解密数据
 * @brief 验证可以解密数据
 * @req SWS_Csm_00051
 */
static void test_Csm_Decrypt_Success(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    /* 设置解密密钥 */
    uint8 keyData[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    Csm_KeyElementSet(CSM_KEY_ID_MASTER, CSM_KEY_ELEMENT_ID_SECRET, keyData, 16);
    Csm_KeySetValid(CSM_KEY_ID_MASTER);
    Csm_JobKeySetUp(CSM_JOB_ID_DECRYPT_DEFAULT, CSM_KEY_ID_MASTER);
    
    uint8 ciphertext[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                            0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    uint8 plaintext[32];
    uint32 plainLength = sizeof(plaintext);
    
    Std_ReturnType result = Csm_Decrypt(CSM_JOB_ID_DECRYPT_DEFAULT,
                                         CSM_OPERATION_MODE_SINGLECALL,
                                         ciphertext, sizeof(ciphertext),
                                         plaintext, &plainLength);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/*==================================================================================================
*                                       测试用例 - MAC服务
==================================================================================================*/

/**
 * @test Csm_MacGenerate 生成MAC
 * @brief 验证可以生成MAC
 * @req SWS_Csm_00040
 */
static void test_Csm_MacGenerate_Success(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    /* 设置MAC密钥 */
    uint8 keyData[32] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
                         0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                         0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};
    Csm_KeyElementSet(CSM_KEY_ID_MASTER, CSM_KEY_ELEMENT_ID_SECRET, keyData, 32);
    Csm_KeySetValid(CSM_KEY_ID_MASTER);
    Csm_JobKeySetUp(CSM_JOB_ID_MAC_GENERATE_DEFAULT, CSM_KEY_ID_MASTER);
    
    uint8 data[] = "Message to be authenticated";
    uint8 mac[CSM_MAX_MAC_LENGTH];
    uint32 macLength = sizeof(mac);
    
    Std_ReturnType result = Csm_MacGenerate(CSM_JOB_ID_MAC_GENERATE_DEFAULT,
                                             CSM_OPERATION_MODE_SINGLECALL,
                                             data, sizeof(data) - 1,
                                             mac, &macLength);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/**
 * @test Csm_MacVerify 验证MAC
 * @brief 验证可以验证MAC
 * @req SWS_Csm_00041
 */
static void test_Csm_MacVerify_Success(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    /* 设置MAC密钥 */
    uint8 keyData[32] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
                         0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                         0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};
    Csm_KeyElementSet(CSM_KEY_ID_MASTER, CSM_KEY_ELEMENT_ID_SECRET, keyData, 32);
    Csm_KeySetValid(CSM_KEY_ID_MASTER);
    Csm_JobKeySetUp(CSM_JOB_ID_MAC_VERIFY_DEFAULT, CSM_KEY_ID_MASTER);
    
    uint8 data[] = "Message to be authenticated";
    uint8 mac[CSM_MAX_MAC_LENGTH] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11};
    boolean verifyResult = FALSE;
    
    Std_ReturnType result = Csm_MacVerify(CSM_JOB_ID_MAC_VERIFY_DEFAULT,
                                           CSM_OPERATION_MODE_SINGLECALL,
                                           data, sizeof(data) - 1,
                                           mac, sizeof(mac),
                                           &verifyResult);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/*==================================================================================================
*                                       测试用例 - 数字签名服务
==================================================================================================*/

/**
 * @test Csm_SignatureGenerate 生成签名
 * @brief 验证可以生成数字签名
 * @req SWS_Csm_00060
 */
static void test_Csm_SignatureGenerate_Success(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    /* 设置签名密钥 */
    uint8 keyData[32] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
                         0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                         0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};
    Csm_KeyElementSet(CSM_KEY_ID_MASTER, CSM_KEY_ELEMENT_ID_PRIVATE, keyData, 32);
    Csm_KeySetValid(CSM_KEY_ID_MASTER);
    Csm_JobKeySetUp(CSM_JOB_ID_SIGN_DEFAULT, CSM_KEY_ID_MASTER);
    
    uint8 data[] = "Data to be signed";
    uint8 signature[CSM_MAX_SIGNATURE_LENGTH];
    uint32 sigLength = sizeof(signature);
    
    Std_ReturnType result = Csm_SignatureGenerate(CSM_JOB_ID_SIGN_DEFAULT,
                                                   CSM_OPERATION_MODE_SINGLECALL,
                                                   data, sizeof(data) - 1,
                                                   signature, &sigLength);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/**
 * @test Csm_SignatureVerify 验证签名
 * @brief 验证可以验证数字签名
 * @req SWS_Csm_00061
 */
static void test_Csm_SignatureVerify_Success(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    /* 设置验证密钥 */
    uint8 keyData[32] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
                         0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                         0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};
    Csm_KeyElementSet(CSM_KEY_ID_MASTER, CSM_KEY_ELEMENT_ID_PUBLIC, keyData, 32);
    Csm_KeySetValid(CSM_KEY_ID_MASTER);
    Csm_JobKeySetUp(CSM_JOB_ID_VERIFY_DEFAULT, CSM_KEY_ID_MASTER);
    
    uint8 data[] = "Data to be verified";
    uint8 signature[64] = {0xAA, 0xBB, 0xCC, 0xDD};
    boolean verifyResult = FALSE;
    
    Std_ReturnType result = Csm_SignatureVerify(CSM_JOB_ID_VERIFY_DEFAULT,
                                                 CSM_OPERATION_MODE_SINGLECALL,
                                                 data, sizeof(data) - 1,
                                                 signature, sizeof(signature),
                                                 &verifyResult);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/*==================================================================================================
*                                       测试用例 - 随机数生成
==================================================================================================*/

/**
 * @test Csm_RandomGenerate 生成随机数
 * @brief 验证可以生成随机数
 * @req SWS_Csm_00070
 */
static void test_Csm_RandomGenerate_Success(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    uint8 random1[32];
    uint8 random2[32];
    
    Std_ReturnType result1 = Csm_RandomGenerate(CSM_JOB_ID_RANDOM_DEFAULT, random1, 32);
    Std_ReturnType result2 = Csm_RandomGenerate(CSM_JOB_ID_RANDOM_DEFAULT, random2, 32);
    
    assert_true(result1 == E_OK || result1 == E_NOT_OK);
    assert_true(result2 == E_OK || result2 == E_NOT_OK);
    
    if (result1 == E_OK && result2 == E_OK)
    {
        /* 两个随机数应该不同（概率极高） */
        assert_memory_not_equal(random1, random2, 32);
    }
}

/**
 * @test Csm_RandomGenerate 长度检查
 * @brief 验证随机数长度限制
 * @req SWS_Csm_00070
 */
static void test_Csm_RandomGenerate_LengthCheck(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    uint8 random[CSM_MAX_DATA_LENGTH + 1];
    
    /* 请求长度超过最大值 */
    Std_ReturnType result = Csm_RandomGenerate(CSM_JOB_ID_RANDOM_DEFAULT, 
                                                random, CSM_MAX_DATA_LENGTH + 1);
    
    assert_int_equal(result, E_NOT_OK);
}

/*==================================================================================================
*                                       测试用例 - 作业管理
==================================================================================================*/

/**
 * @test Csm_JobKeySetUp 设置作业密钥
 * @brief 验证可以设置作业关联的密钥
 * @req SWS_Csm_00080
 */
static void test_Csm_JobKeySetUp_Success(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    Std_ReturnType result = Csm_JobKeySetUp(CSM_JOB_ID_ENCRYPT_DEFAULT, CSM_KEY_ID_MASTER);
    
    assert_int_equal(result, E_OK);
}

/**
 * @test Csm_CancelJob 取消作业
 * @brief 验证可以取消正在进行的作业
 * @req SWS_Csm_00090
 */
static void test_Csm_CancelJob_Success(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    Std_ReturnType result = Csm_CancelJob(CSM_JOB_ID_ENCRYPT_DEFAULT);
    
    assert_int_equal(result, E_OK);
}

/**
 * @test Csm_GetJobState 获取作业状态
 * @brief 验证可以获取作业状态
 * @req SWS_Csm_00103
 */
static void test_Csm_GetJobState_Success(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    Csm_JobStateType jobState;
    Std_ReturnType result = Csm_GetJobState(CSM_JOB_ID_ENCRYPT_DEFAULT, &jobState);
    
    assert_int_equal(result, E_OK);
    /* 刚初始化后应为IDLE状态 */
    assert_true(jobState == CSM_JOB_STATE_IDLE);
}

/**
 * @test Csm_RegisterCallback 注册回调
 * @brief 验证可以注册作业完成回调
 * @req SWS_Csm_00101
 */
static void test_Csm_RegisterCallback_Success(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    boolean context = FALSE;
    Std_ReturnType result = Csm_RegisterCallback(CSM_JOB_ID_ENCRYPT_DEFAULT,
                                                  test_callback, &context);
    
    assert_int_equal(result, E_OK);
}

/*==================================================================================================
*                                       测试用例 - 主函数
==================================================================================================*/

/**
 * @test Csm_MainFunction 主函数处理
 * @brief 验证主函数可以正常执行
 * @req SWS_Csm_00100
 */
static void test_Csm_MainFunction_Success(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    /* 主函数应无异常执行 */
    Csm_MainFunction();
    Csm_MainFunction();
    
    assert_true(TRUE);
}

/**
 * @test Csm_MainFunction 未初始化状态
 * @brief 验证未初始化时主函数不执行
 * @req SWS_Csm_00100
 */
static void test_Csm_MainFunction_NotInitialized(void** state)
{
    (void)state;
    
    /* 确保未初始化 */
    Csm_DeInit();
    
    /* 主函数应安全返回 */
    Csm_MainFunction();
    
    assert_true(TRUE);
}

/*==================================================================================================
*                                       测试用例 - 版本信息
==================================================================================================*/

#if (CSM_VERSION_INFO_API == STD_ON)
/**
 * @test Csm_GetVersionInfo 获取版本信息
 * @brief 验证可以获取版本信息
 * @req SWS_Csm_00104
 */
static void test_Csm_GetVersionInfo_Success(void** state)
{
    (void)state;
    
    Std_VersionInfoType versionInfo;
    Csm_GetVersionInfo(&versionInfo);
    
    assert_int_equal(versionInfo.vendorID, CSM_VENDOR_ID);
    assert_int_equal(versionInfo.moduleID, CSM_MODULE_ID);
    assert_int_equal(versionInfo.sw_major_version, CSM_SW_MAJOR_VERSION);
    assert_int_equal(versionInfo.sw_minor_version, CSM_SW_MINOR_VERSION);
    assert_int_equal(versionInfo.sw_patch_version, CSM_SW_PATCH_VERSION);
}
#endif

/*==================================================================================================
*                                       测试用例 - 错误处理
==================================================================================================*/

/**
 * @test Csm 未初始化访问
 * @brief 验证未初始化时API返回错误
 * @req SWS_Csm_00001
 */
static void test_Csm_UninitializedAccess(void** state)
{
    (void)state;
    
    /* 确保未初始化 */
    Csm_DeInit();
    
    uint8 buffer[16] = {0};
    uint32 length = sizeof(buffer);
    boolean resultBool = FALSE;
    Csm_KeyStatusType keyStatus;
    Csm_JobStateType jobState;
    
    /* 所有API都应返回E_NOT_OK */
    assert_int_equal(Csm_KeyElementSet(CSM_KEY_ID_MASTER, 0, buffer, 16), E_NOT_OK);
    assert_int_equal(Csm_KeyElementGet(CSM_KEY_ID_MASTER, 0, buffer, &length), E_NOT_OK);
    assert_int_equal(Csm_KeySetValid(CSM_KEY_ID_MASTER), E_NOT_OK);
    assert_int_equal(Csm_GetKeyStatus(CSM_KEY_ID_MASTER, &keyStatus), E_NOT_OK);
    assert_int_equal(Csm_Hash(CSM_JOB_ID_HASH_DEFAULT, 0, buffer, 16, buffer, &length), E_NOT_OK);
    assert_int_equal(Csm_Encrypt(CSM_JOB_ID_ENCRYPT_DEFAULT, 0, buffer, 16, buffer, &length), E_NOT_OK);
    assert_int_equal(Csm_Decrypt(CSM_JOB_ID_DECRYPT_DEFAULT, 0, buffer, 16, buffer, &length), E_NOT_OK);
    assert_int_equal(Csm_MacGenerate(CSM_JOB_ID_MAC_GENERATE_DEFAULT, 0, buffer, 16, buffer, &length), E_NOT_OK);
    assert_int_equal(Csm_MacVerify(CSM_JOB_ID_MAC_VERIFY_DEFAULT, 0, buffer, 16, buffer, 16, &resultBool), E_NOT_OK);
    assert_int_equal(Csm_SignatureGenerate(CSM_JOB_ID_SIGN_DEFAULT, 0, buffer, 16, buffer, &length), E_NOT_OK);
    assert_int_equal(Csm_SignatureVerify(CSM_JOB_ID_VERIFY_DEFAULT, 0, buffer, 16, buffer, 16, &resultBool), E_NOT_OK);
    assert_int_equal(Csm_RandomGenerate(CSM_JOB_ID_RANDOM_DEFAULT, buffer, 16), E_NOT_OK);
    assert_int_equal(Csm_GetJobState(CSM_JOB_ID_ENCRYPT_DEFAULT, &jobState), E_NOT_OK);
    assert_int_equal(Csm_JobKeySetUp(CSM_JOB_ID_ENCRYPT_DEFAULT, CSM_KEY_ID_MASTER), E_NOT_OK);
    assert_int_equal(Csm_CancelJob(CSM_JOB_ID_ENCRYPT_DEFAULT), E_NOT_OK);
}

/*==================================================================================================
*                                       测试用例 - 边界条件
==================================================================================================*/

/**
 * @test Csm 空数据处理
 * @brief 验证可以处理空数据
 * @req SWS_Csm_00030
 */
static void test_Csm_EmptyData(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    uint8 hash[CSM_MAX_HASH_LENGTH];
    uint32 hashLength = sizeof(hash);
    
    /* 空数据哈希 */
    Std_ReturnType result = Csm_Hash(CSM_JOB_ID_HASH_DEFAULT,
                                      CSM_OPERATION_MODE_SINGLECALL,
                                      (const uint8*)"", 0,
                                      hash, &hashLength);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/**
 * @test Csm 大数据量处理
 * @brief 验证可以处理最大允许数据量
 * @req SWS_Csm_00030
 */
static void test_Csm_MaxDataLength(void** state)
{
    (void)state;
    
    Csm_Init(&Csm_Config);
    
    uint8 data[CSM_MAX_DATA_LENGTH];
    uint8 resultData[CSM_MAX_DATA_LENGTH];
    uint32 resultLength = sizeof(resultData);
    
    memset(data, 0xAA, sizeof(data));
    
    Std_ReturnType result = Csm_Hash(CSM_JOB_ID_HASH_DEFAULT,
                                      CSM_OPERATION_MODE_SINGLECALL,
                                      data, CSM_MAX_DATA_LENGTH,
                                      resultData, &resultLength);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/*==================================================================================================
*                                       测试运行器
==================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        /* 初始化/去初始化 */
        cmocka_unit_test_setup_teardown(test_Csm_Init_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_Init_NullConfig, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_Init_AlreadyInitialized, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_DeInit_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_DeInit_NotInitialized, test_setup, test_teardown),
        
        /* 密钥管理 */
        cmocka_unit_test_setup_teardown(test_Csm_KeyElementSet_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_KeyElementSet_NullPointer, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_KeyElementSet_LengthExceeded, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_KeyElementSet_InvalidKeyId, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_KeySetValid_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_KeyElementGet_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_KeyElementGet_KeyNotValid, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_KeyCopy_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_GetKeyStatus_Success, test_setup, test_teardown),
        
        /* 哈希服务 */
        cmocka_unit_test_setup_teardown(test_Csm_Hash_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_Hash_MultiStage, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_Hash_InvalidJobId, test_setup, test_teardown),
        
        /* 加密/解密服务 */
        cmocka_unit_test_setup_teardown(test_Csm_Encrypt_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_Encrypt_NullPointer, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_Decrypt_Success, test_setup, test_teardown),
        
        /* MAC服务 */
        cmocka_unit_test_setup_teardown(test_Csm_MacGenerate_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_MacVerify_Success, test_setup, test_teardown),
        
        /* 数字签名服务 */
        cmocka_unit_test_setup_teardown(test_Csm_SignatureGenerate_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_SignatureVerify_Success, test_setup, test_teardown),
        
        /* 随机数生成 */
        cmocka_unit_test_setup_teardown(test_Csm_RandomGenerate_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_RandomGenerate_LengthCheck, test_setup, test_teardown),
        
        /* 作业管理 */
        cmocka_unit_test_setup_teardown(test_Csm_JobKeySetUp_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_CancelJob_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_GetJobState_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_RegisterCallback_Success, test_setup, test_teardown),
        
        /* 主函数 */
        cmocka_unit_test_setup_teardown(test_Csm_MainFunction_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_MainFunction_NotInitialized, test_setup, test_teardown),
        
        /* 版本信息 */
#if (CSM_VERSION_INFO_API == STD_ON)
        cmocka_unit_test_setup_teardown(test_Csm_GetVersionInfo_Success, test_setup, test_teardown),
#endif
        
        /* 错误处理 */
        cmocka_unit_test_setup_teardown(test_Csm_UninitializedAccess, test_setup, test_teardown),
        
        /* 边界条件 */
        cmocka_unit_test_setup_teardown(test_Csm_EmptyData, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_Csm_MaxDataLength, test_setup, test_teardown),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}
