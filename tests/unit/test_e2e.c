/*
 * @file test_e2e.c
 * @brief E2E (End-to-End) 保护库单元测试
 * 
 * 测试范围:
 * - Profile 1/1A: CRC8 + 计数器
 * - Profile 2/2A: CRC8 + 计数器 + 数据ID
 * - Profile 4: CRC16 + 计数器 + 长度
 * - Profile 5: CRC32 + 计数器
 * - 数据损坏检测
 * - 重复数据检测
 * - 顺序错乱检测
 *
 * AUTOSAR Standard: R22-11
 * ASIL Level: D
 */

// @tests src/bsw/services/e2e/src/E2E.c  @tests src/bsw/services/e2e/include/E2E.h

#include <unity.h>
#include "E2E.h"
#include "Crc.h"
#include <string.h>

/* ================================ 测试数据 ================================ */

#define TEST_DATA_SIZE  64

static uint8 testData[TEST_DATA_SIZE];
static E2E_P01ConfigType p01Config;
static E2E_P01CheckStateType p01CheckState;
static E2E_P01ProtectStateType p01ProtectState;

static E2E_P02ConfigType p02Config;
static E2E_P02CheckStateType p02CheckState;
static E2E_P02ProtectStateType p02ProtectState;

static E2E_P04ConfigType p04Config;
static E2E_P04CheckStateType p04CheckState;
static E2E_P04ProtectStateType p04ProtectState;

static E2E_P05ConfigType p05Config;
static E2E_P05CheckStateType p05CheckState;
static E2E_P05ProtectStateType p05ProtectState;

void setUp(void) {
    memset(testData, 0xAA, sizeof(testData));
    
    /* Profile 1 配置 */
    p01Config.CRCOffset = 0;
    p01Config.CounterOffset = 8;
    p01Config.DataID = 0x1234;
    p01Config.DataIDMode = E2E_P01_DATAID_BOTH;
    p01Config.DataLength = 16;
    p01Config.MaxDeltaCounterInit = 1;
    
    /* Profile 2 配置 */
    p02Config.CRCOffset = 0;
    p02Config.CounterOffset = 8;
    p02Config.DataIDList[0] = 0x01;
    p02Config.DataLength = 16;
    p02Config.MaxDeltaCounterInit = 1;
    
    /* Profile 4 配置 */
    p04Config.CRCOffset = 0;
    p04Config.CounterOffset = 16;
    p04Config.DataID = 0x12345678;
    p04Config.MinDataLength = 32;
    p04Config.MaxDataLength = 256;
    p04Config.MaxDeltaCounter = 1;
    
    /* Profile 5 配置 */
    p05Config.CRCOffset = 0;
    p05Config.CounterOffset = 32;
    p05Config.DataID = 0x12345678;
    p05Config.DataLength = 64;
    p05Config.MaxDeltaCounterInit = 1;
}

void tearDown(void) {
    /* 清理 */
}

/* ================================ Profile 1 测试 ================================ */

/**
 * @brief 测试Profile 1 CRC计算正确性
 * @test E2E_P01_001
 */
void test_E2E_P01_CRC_Calculation(void) {
    E2E_P01ProtectStateType state;
    uint8 data[16] = {0};
    
    /* 保护数据 */
    Std_ReturnType result = E2E_P01Protect(&p01Config, &state, data);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* 验证CRC字节不为0 */
    TEST_ASSERT_NOT_EQUAL(0, data[0]);
    
    /* 检查数据 */
    E2E_P01CheckStateType checkState;
    E2E_CheckResultType checkResult;
    result = E2E_P01Check(&p01Config, &checkState, data, &checkResult);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(E2E_P01STATUS_OK, checkResult);
}

/**
 * @brief 测试Profile 1 计数器递增
 * @test E2E_P01_002
 */
void test_E2E_P01_Counter_Increment(void) {
    uint8 data[16] = {0};
    uint8 counter1, counter2;
    
    /* 第一次保护 */
    E2E_P01Protect(&p01Config, &p01ProtectState, data);
    counter1 = (data[1] >> 4) & 0x0F;
    
    /* 第二次保护 */
    memset(data, 0, sizeof(data));
    E2E_P01Protect(&p01Config, &p01ProtectState, data);
    counter2 = (data[1] >> 4) & 0x0F;
    
    /* 验证计数器递增 */
    TEST_ASSERT_EQUAL((counter1 + 1) % 16, counter2);
}

/**
 * @brief 测试Profile 1 数据损坏检测
 * @test E2E_P01_003
 */
void test_E2E_P01_Data_Corruption_Detection(void) {
    uint8 data[16] = {0};
    E2E_P01CheckStateType checkState;
    E2E_CheckResultType checkResult;
    
    /* 保护数据 */
    E2E_P01Protect(&p01Config, &p01ProtectState, data);
    
    /* 模拟数据损坏 */
    data[5] ^= 0xFF;
    
    /* 检查数据 */
    E2E_P01Check(&p01Config, &checkState, data, &checkResult);
    
    /* 验证检测到错误 */
    TEST_ASSERT_EQUAL(E2E_P01STATUS_ERROR, checkResult);
}

/**
 * @brief 测试Profile 1 顺序错乱检测
 * @test E2E_P01_004
 */
void test_E2E_P01_Sequence_Error_Detection(void) {
    uint8 data1[16] = {0};
    uint8 data2[16] = {0};
    E2E_P01CheckStateType checkState;
    E2E_CheckResultType checkResult;
    
    /* 保护两个连续数据 */
    E2E_P01Protect(&p01Config, &p01ProtectState, data1);
    E2E_P01Protect(&p01Config, &p01ProtectState, data2);
    
    /* 交换顺序 */
    E2E_P01Check(&p01Config, &checkState, data2, &checkResult);
    
    /* 第一个数据应该带来序错误 */
    TEST_ASSERT_EQUAL(E2E_P01STATUS_WRONGSEQUENCE, checkResult);
}

/* ================================ Profile 2 测试 ================================ */

/**
 * @brief 测试Profile 2 保护
 * @test E2E_P02_001
 */
void test_E2E_P02_Protection(void) {
    uint8 data[16] = {0};
    E2E_P02CheckStateType checkState;
    E2E_CheckResultType checkResult;
    
    Std_ReturnType result = E2E_P02Protect(&p02Config, &p02ProtectState, data);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    result = E2E_P02Check(&p02Config, &checkState, data, &checkResult);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(E2E_P02STATUS_OK, checkResult);
}

/* ================================ Profile 4 测试 ================================ */

/**
 * @brief 测试Profile 4 CRC16计算
 * @test E2E_P04_001
 */
void test_E2E_P04_CRC16_Calculation(void) {
    uint8 data[64] = {0};
    E2E_P04CheckStateType checkState;
    E2E_CheckResultType checkResult;
    
    Std_ReturnType result = E2E_P04Protect(&p04Config, &p04ProtectState, data, 64);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* 验证CRC16占用2个字节 */
    TEST_ASSERT_NOT_EQUAL(0, data[0]);
    TEST_ASSERT_NOT_EQUAL(0, data[1]);
    
    result = E2E_P04Check(&p04Config, &checkState, data, 64, &checkResult);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(E2E_P04STATUS_OK, checkResult);
}

/**
 * @brief 测试Profile 4 变长数据处理
 * @test E2E_P04_002
 */
void test_E2E_P04_Variable_Length(void) {
    uint8 data[128] = {0};
    E2E_P04CheckStateType checkState;
    E2E_CheckResultType checkResult;
    
    /* 不同长度的数据 */
    for (uint16 len = 32; len <= 128; len += 16) {
        memset(data, 0, sizeof(data));
        Std_ReturnType result = E2E_P04Protect(&p04Config, &p04ProtectState, data, len);
        TEST_ASSERT_EQUAL(E_OK, result);
        
        result = E2E_P04Check(&p04Config, &checkState, data, len, &checkResult);
        TEST_ASSERT_EQUAL(E_OK, result);
        TEST_ASSERT_EQUAL(E2E_P04STATUS_OK, checkResult);
    }
}

/* ================================ Profile 5 测试 ================================ */

/**
 * @brief 测试Profile 5 CRC32计算
 * @test E2E_P05_001
 */
void test_E2E_P05_CRC32_Calculation(void) {
    uint8 data[64] = {0};
    E2E_P05CheckStateType checkState;
    E2E_CheckResultType checkResult;
    
    Std_ReturnType result = E2E_P05Protect(&p05Config, &p05ProtectState, data);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* 验证CRC32占用4个字节 */
    TEST_ASSERT_NOT_EQUAL(0, *(uint32*)&data[0]);
    
    result = E2E_P05Check(&p05Config, &checkState, data, &checkResult);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(E2E_P05STATUS_OK, checkResult);
}

/* ================================ CRC测试 ================================ */

/**
 * @brief 测试CRC8计算
 * @test CRC_001
 */
void test_CRC8_Calculation(void) {
    uint8 data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint8 crc = Crc_CalculateCRC8(data, sizeof(data), 0xFF, TRUE);
    
    /* 验证CRC值不为0且正确 */
    TEST_ASSERT_NOT_EQUAL(0, crc);
    
    /* 验证CRC检查 */
    uint8 checkData[6];
    memcpy(checkData, data, sizeof(data));
    checkData[5] = crc;
    uint8 verify = Crc_CalculateCRC8(checkData, sizeof(checkData), 0xFF, TRUE);
    TEST_ASSERT_EQUAL(0, verify);
}

/**
 * @brief 测试CRC16计算
 * @test CRC_002
 */
void test_CRC16_Calculation(void) {
    uint8 data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint16 crc = Crc_CalculateCRC16(data, sizeof(data), 0xFFFF, TRUE);
    
    TEST_ASSERT_NOT_EQUAL(0, crc);
}

/**
 * @brief 测试CRC32计算
 * @test CRC_003
 */
void test_CRC32_Calculation(void) {
    uint8 data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint32 crc = Crc_CalculateCRC32(data, sizeof(data), 0xFFFFFFFF, TRUE);
    
    TEST_ASSERT_NOT_EQUAL(0, crc);
}

/* ================================ 主函数 ================================ */

int main(void) {
    UNITY_BEGIN();
    
    /* Profile 1测试 */
    RUN_TEST(test_E2E_P01_CRC_Calculation);
    RUN_TEST(test_E2E_P01_Counter_Increment);
    RUN_TEST(test_E2E_P01_Data_Corruption_Detection);
    RUN_TEST(test_E2E_P01_Sequence_Error_Detection);
    
    /* Profile 2测试 */
    RUN_TEST(test_E2E_P02_Protection);
    
    /* Profile 4测试 */
    RUN_TEST(test_E2E_P04_CRC16_Calculation);
    RUN_TEST(test_E2E_P04_Variable_Length);
    
    /* Profile 5测试 */
    RUN_TEST(test_E2E_P05_CRC32_Calculation);
    
    /* CRC测试 */
    RUN_TEST(test_CRC8_Calculation);
    RUN_TEST(test_CRC16_Calculation);
    RUN_TEST(test_CRC32_Calculation);
    
    return UNITY_END();
}
