/**
 * @file test_safety_ecc.c
 * @brief ECC安全机制单元测试
 * @version 1.0
 * @date 2026-04-25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unity.h"
#include "../../../src/safety/ram/ecc_encoder.h"
#include "../../../src/safety/ram/ecc_checker.h"
#include "../../../src/common/autosar_types.h"

/*==============================================================================
 * 全局状态
 *==============================================================================*/

static EccEncoderConfigType g_encoder_config;

/*==============================================================================
 * Unity Setup/Teardown
 *==============================================================================*/

void setUp(void) {
    g_encoder_config.mode = ECC_MODE_32BIT;
    g_encoder_config.use_hardware = FALSE;
    g_encoder_config.asil_level = ASIL_D;
    
    EccEncoder_Init(&g_encoder_config);
}

void tearDown(void) {
    EccEncoder_DeInit();
}

/*==============================================================================
 * 测试用例 - ECC编码器
 *==============================================================================*/

/* Test 1: 编码器初始化 */
void test_ecc_encoder_init(void) {
    Std_ReturnType result = EccEncoder_Init(&g_encoder_config);
    TEST_ASSERT_EQUAL(E_OK, result);
}

/* Test 2: 32位数据编码 */
void test_ecc_encode_32bit(void) {
    uint32_t data = 0x12345678;
    uint8_t ecc_code = 0;
    
    Std_ReturnType result = EccEncoder_Encode32(data, &ecc_code);
    TEST_ASSERT_EQUAL(E_OK, result);
    /* ECC码应该是非零的 */
    TEST_ASSERT_NOT_EQUAL(0, ecc_code);
}

/* Test 3: 验证数据一致性 */
void test_ecc_encode_consistency(void) {
    uint32_t data = 0xABCDEF00;
    uint8_t ecc1, ecc2;
    
    EccEncoder_Encode32(data, &ecc1);
    EccEncoder_Encode32(data, &ecc2);
    
    /* 相同数据应诧产生相同的ECC码 */
    TEST_ASSERT_EQUAL(ecc1, ecc2);
}

/* Test 4: 不同数据产生不同ECC */
void test_ecc_different_data(void) {
    uint32_t data1 = 0x11111111;
    uint32_t data2 = 0x22222222;
    uint8_t ecc1, ecc2;
    
    EccEncoder_Encode32(data1, &ecc1);
    EccEncoder_Encode32(data2, &ecc2);
    
    /* 不同数据应该产生不同的ECC码 */
    TEST_ASSERT_NOT_EQUAL(ecc1, ecc2);
}

/* Test 5: 批量编码 */
void test_ecc_batch_encode(void) {
    uint32_t data[10] = {0};
    uint8_t ecc_codes[10] = {0};
    
    for (int i = 0; i < 10; i++) {
        data[i] = i * 0x11111111;
    }
    
    Std_ReturnType result = EccEncoder_BatchEncode32(data, ecc_codes, 10);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* 验证所有ECC码都已计算 */
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_NOT_EQUAL(0, ecc_codes[i]);
    }
}

/* Test 6: 完整编码结果 */
void test_ecc_encode_full(void) {
    EccEncodedType result;
    uint32_t data = 0xDEADBEEF;
    
    Std_ReturnType status = EccEncoder_EncodeFull32(data, &result);
    TEST_ASSERT_EQUAL(E_OK, status);
    
    TEST_ASSERT_EQUAL(data, result.data);
    TEST_ASSERT_NOT_EQUAL(0, result.ecc_code);
    TEST_ASSERT_EQUAL(ECC_MODE_32BIT, result.mode);
}

/* Test 7: 64位数据编码 */
void test_ecc_encode_64bit(void) {
    uint64_t data = 0x123456789ABCDEF0ULL;
    uint8_t ecc_code = 0;
    
    /* 切换到64位模式 */
    g_encoder_config.mode = ECC_MODE_64BIT;
    EccEncoder_Init(&g_encoder_config);
    
    Std_ReturnType result = EccEncoder_Encode64(data, &ecc_code);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_NOT_EQUAL(0, ecc_code);
}

/* Test 8: 编码器状态检查 */
void test_ecc_encoder_state(void) {
    const EccEncoderStateType* state = EccEncoder_GetState();
    TEST_ASSERT_NOT_NULL(state);
    TEST_ASSERT_TRUE(state->initialized);
}

/* Test 9: 版本信息 */
void test_ecc_version_info(void) {
    Std_VersionInfoType version;
    EccEncoder_GetVersionInfo(&version);
    /* 验证版本号是合理的 */
    TEST_ASSERT_GREATER_THAN(0, version.vendorID);
}

/* Test 10: 奇偶校验位计算 */
void test_ecc_parity_calculation(void) {
    uint32_t value = 0x00000001;
    uint8_t parity = EccEncoder_CalculateParity(value);
    /* 0x00000001 有奇数个1，奇偶校验位应该是1 */
    TEST_ASSERT_EQUAL(1, parity);
    
    value = 0x00000003; /* 两个1 */
    parity = EccEncoder_CalculateParity(value);
    TEST_ASSERT_EQUAL(0, parity);
}

/*==============================================================================
 * 主函数
 *==============================================================================*/

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_ecc_encoder_init);
    RUN_TEST(test_ecc_encode_32bit);
    RUN_TEST(test_ecc_encode_consistency);
    RUN_TEST(test_ecc_different_data);
    RUN_TEST(test_ecc_batch_encode);
    RUN_TEST(test_ecc_encode_full);
    RUN_TEST(test_ecc_encode_64bit);
    RUN_TEST(test_ecc_encoder_state);
    RUN_TEST(test_ecc_version_info);
    RUN_TEST(test_ecc_parity_calculation);
    
    return UNITY_END();
}
