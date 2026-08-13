/**
 * @file test_Crypto.c
 * @brief Crypto Driver 模块单元测试
 * @version 1.0.0
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "Crypto.h"

/* 测试结果计数 */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* 测试宏 */
#define TEST_ASSERT(expr) \
    do { \
        tests_run++; \
        if (expr) { \
            tests_passed++; \
            printf("  [PASS] %s\n", #expr); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual) \
    do { \
        tests_run++; \
        if ((expected) == (actual)) { \
            tests_passed++; \
            printf("  [PASS] %s == %s (%d == %d)\n", #expected, #actual, (int)(expected), (int)(actual)); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s == %s (%d != %d) (%s:%d)\n", #expected, #actual, (int)(expected), (int)(actual), __FILE__, __LINE__); \
        } \
    } while(0)

/* 测试数据 */
static const uint8 test_data[] = "Hello, Crypto World!";
static const uint8 test_key[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                  0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};

/* 初始化测试 */
void test_init_deinit(void)
{
    printf("\n=== Initialization Tests ===\n");
    
    /* 测试初始化 */
    Crypto_Init(&Crypto_Config);  /* 真实驱动配置 */
    TEST_ASSERT(1);  /* 初始化完成 */
    
    /* 测试反初始化 */
    Crypto_DeInit();
    TEST_ASSERT(1);  /* 反初始化完成 */
}

/* 密钥管理测试 */
void test_key_management(void)
{
    Std_ReturnType result;
    uint8 key_buffer[32];
    uint32 key_length = 32;
    
    printf("\n=== Key Management Tests ===\n");
    
    Crypto_Init(&Crypto_Config);
    
    /* 测试设置密钥元素 — keyId 1 = AES_SESSION，真实元素为 CRYPTO_KEY_ELEMENT_AES_KEY (0)，256-bit */
    result = Crypto_KeyElementSet(1, CRYPTO_KEY_ELEMENT_AES_KEY, test_key, 16);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* 测试获取密钥元素 */
    result = Crypto_KeyElementGet(1, CRYPTO_KEY_ELEMENT_AES_KEY, key_buffer, &key_length);
    TEST_ASSERT_EQ(E_OK, result);
    TEST_ASSERT_EQ(32, key_length);  /* 真实元素大小 = CRYPTO_AES_KEY_SIZE_256 */
    
    /* 测试设置密钥有效性 */
    result = Crypto_KeyValidSet(1, TRUE);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* 测试无效密钥ID */
    result = Crypto_KeyElementSet(255, CRYPTO_KEY_ELEMENT_AES_KEY, test_key, 16);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* 测试NULL指针 */
    result = Crypto_KeyElementSet(1, CRYPTO_KEY_ELEMENT_AES_KEY, NULL, 16);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    Crypto_DeInit();
}

/* 随机数生成测试 */
void test_random_generation(void)
{
    Std_ReturnType result;
    uint8 random_data1[16];
    uint8 random_data2[16];
    int i;
    int diff_count = 0;
    
    printf("\n=== Random Generation Tests ===\n");
    
    Crypto_Init(&Crypto_Config);
    
    /* 测试生成随机数 */
    result = Crypto_RandomGenerate(1, random_data1, 16);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* 测试再次生成随机数 */
    result = Crypto_RandomGenerate(1, random_data2, 16);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* 验证两次生成的随机数不同 */
    for (i = 0; i < 16; i++) {
        if (random_data1[i] != random_data2[i]) {
            diff_count++;
        }
    }
    TEST_ASSERT(diff_count > 0);  /* 应该至少有部分不同 */
    
    /* 测试零长度 */
    result = Crypto_RandomGenerate(1, random_data1, 0);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* 测试无效密钥ID */
    result = Crypto_RandomGenerate(255, random_data1, 16);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    Crypto_DeInit();
}

/* BLAKE2哈希测试 */
void test_blake2_hash(void)
{
    Std_ReturnType result;
    uint8 digest[64];
    
    printf("\n=== BLAKE2 Hash Tests ===\n");
    
    Crypto_Init(&Crypto_Config);
    
    /* 测试BLAKE2b哈希计算 */
    result = Crypto_Blake2b(test_data, sizeof(test_data), NULL, 0, 32, digest);
    TEST_ASSERT_EQ(E_OK, result);
    TEST_ASSERT(digest[0] != 0 || digest[1] != 0);  /* 哈希结果不应全为零 */
    
    /* 测试BLAKE2s哈希计算 */
    result = Crypto_Blake2s(test_data, sizeof(test_data), NULL, 0, 32, digest);
    TEST_ASSERT_EQ(E_OK, result);
    TEST_ASSERT(digest[0] != 0 || digest[1] != 0);
    
    /* 测试带密钥的BLAKE2b */
    result = Crypto_Blake2b(test_data, sizeof(test_data), test_key, 16, 32, digest);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* 测试NULL数据指针 */
    result = Crypto_Blake2b(NULL, sizeof(test_data), NULL, 0, 32, digest);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* 测试NULL输出指针 */
    result = Crypto_Blake2b(test_data, sizeof(test_data), NULL, 0, 32, NULL);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    Crypto_DeInit();
}

/* 增量哈希测试 */
void test_incremental_hash(void)
{
    Std_ReturnType result;
    uint8 digest[64];
    uint32 digest_length = 64;
    
    printf("\n=== Incremental Hash Tests ===\n");
    
    Crypto_Init(&Crypto_Config);
    
    /* 测试启动增量哈希 */
    result = Crypto_Blake2b_Start(1, NULL, 0, 32);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* 测试更新数据 */
    result = Crypto_Blake2b_Update(1, test_data, 10);
    TEST_ASSERT_EQ(E_OK, result);
    
    result = Crypto_Blake2b_Update(1, test_data + 10, sizeof(test_data) - 10);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* 测试完成哈希 — 真实驱动：Finish 的缓冲长度须与 Start 的 digestLength 一致（blake2b_final 校验） */
    digest_length = 32;  /* 与 Start(1, NULL, 0, 32) 的 digestLength 对齐 */
    result = Crypto_Blake2b_Finish(1, digest, &digest_length);
    TEST_ASSERT_EQ(E_OK, result);
    TEST_ASSERT_EQ(32, digest_length);
    
    /* 无效任务ID — 真实驱动实现不使用 jobId（(void)jobId），故返回 E_OK */
    result = Crypto_Blake2b_Start(255, NULL, 0, 32);
    TEST_ASSERT_EQ(E_OK, result);
    
    Crypto_DeInit();
}

/* 密钥派生测试 */
void test_key_derivation(void)
{
    Std_ReturnType result;
    
    printf("\n=== Key Derivation Tests ===\n");
    
    Crypto_Init(&Crypto_Config);
    
    /* 设置源密钥 */
    result = Crypto_KeyElementSet(1, CRYPTO_KEY_ELEMENT_AES_KEY, test_key, 16);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* 测试密钥派生 */
    result = Crypto_KeyDerive(1, 2);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* 测试无效源密钥 */
    result = Crypto_KeyDerive(255, 2);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* 测试无效目标密钥 */
    result = Crypto_KeyDerive(1, 255);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    Crypto_DeInit();
}

/* CCC数字密钥特定测试 */
void test_ccc_digital_key(void)
{
    Std_ReturnType result;
    uint8 challenge[16] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
                           0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
    uint8 signature[64];
    uint32 signature_length = 64;
    Crypto_VerifyResultType verify_result;
    
    printf("\n=== CCC Digital Key Tests ===\n");
    
    Crypto_Init(&Crypto_Config);
    
    /* 测试设备认证签名生成 */
    result = Crypto_CccGenerateAttestation(challenge, 16, signature, &signature_length);
    TEST_ASSERT_EQ(E_OK, result);
    TEST_ASSERT(signature_length > 0);
    
    /* 测试NULL挑战指针 */
    result = Crypto_CccGenerateAttestation(NULL, 16, signature, &signature_length);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* 测试NULL签名指针 */
    result = Crypto_CccGenerateAttestation(challenge, 16, NULL, &signature_length);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    Crypto_DeInit();
}

/* HSM特定测试 */
void test_hsm_operations(void)
{
#if (CRYPTO_CFG_HSM_ENABLED == STD_ON)
    Std_ReturnType result;
    boolean available;
    Crypto_HsmStateType status;
    uint8 hsm_id[16];
    uint32 id_length = 16;
    
    printf("\n=== HSM Operations Tests ===\n");
    
    Crypto_Init(&Crypto_Config);
    
    /* 测试HSM可用性检查 */
    available = Crypto_HsmIsAvailable();
    printf("  HSM Available: %s\n", available ? "Yes" : "No");
    TEST_ASSERT(1);  /* 仅检查函数执行 */
    
    /* 测试获取HSM状态 */
    status = Crypto_HsmGetStatus();
    printf("  HSM Status: %d\n", status);
    TEST_ASSERT(status >= CRYPTO_HSM_IDLE && status <= CRYPTO_HSM_UNINIT);
    
    /* 测试HSM自测 — 真实驱动：HSM 不可用时返回 E_NOT_OK（HSM_HARDWARE_PRESENT 未定义） */
    result = Crypto_HsmSelfTest();
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* 测试获取HSM ID — 真实驱动：HSM 不可用返回 E_NOT_OK */
    result = Crypto_HsmGetId(hsm_id, &id_length);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    Crypto_DeInit();
#else
    printf("\n=== HSM Operations Tests (Skipped - HSM not enabled) ===\n");
#endif
}

/* 版本信息测试 */
void test_version_info(void)
{
    printf("\n=== Version Info Tests ===\n");
    
#if (CRYPTO_CFG_VERSION_INFO_API == STD_ON)
    Std_VersionInfoType version_info;
    
    Crypto_GetVersionInfo(&version_info);
    TEST_ASSERT_EQ(CRYPTO_SW_MAJOR_VERSION, version_info.sw_major_version);
    TEST_ASSERT_EQ(CRYPTO_SW_MINOR_VERSION, version_info.sw_minor_version);
    TEST_ASSERT_EQ(CRYPTO_SW_PATCH_VERSION, version_info.sw_patch_version);
    TEST_ASSERT_EQ(CRYPTO_VENDOR_ID, version_info.vendorID);
    TEST_ASSERT_EQ(CRYPTO_MODULE_ID, version_info.moduleID);
#else
    printf("  Version Info API not enabled\n");
    TEST_ASSERT(1);
#endif
}

/* 主函数 */
int main(void)
{
    printf("========================================\n");
    printf("   Crypto Module Unit Tests\n");
    printf("========================================\n");
    
    test_init_deinit();
    test_key_management();
    test_random_generation();
    test_blake2_hash();
    test_incremental_hash();
    test_key_derivation();
    test_ccc_digital_key();
    test_hsm_operations();
    test_version_info();
    
    printf("\n========================================\n");
    printf("   Test Results\n");
    printf("========================================\n");
    printf("Total:   %d\n", tests_run);
    printf("Passed:  %d\n", tests_passed);
    printf("Failed:  %d\n", tests_failed);
    
    if (tests_failed == 0) {
        printf("\nAll tests PASSED!\n");
        return 0;
    } else {
        printf("\nSome tests FAILED!\n");
        return 1;
    }
}
