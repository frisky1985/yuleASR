/*
 * @file test_nvm_redundant.c
 * @brief NVM Redundant Storage 单元测试
 * 
 * 测试范围:
 * - 双备份写入
 * - CRC校验
 * - 自动故障检测
 * - 自动恢复
 * - 数据完整性
 * - 写入耐久性
 *
 * AUTOSAR Standard: R22-11
 * ASIL Level: C
 */

#include <unity.h>
#include "NvM.h"
#include "NvM_Redundant.h"
#include "Crc.h"
#include <string.h>

/* ================================ 测试数据 ================================ */

#define TEST_BLOCK_ID       1
#define TEST_DATA_SIZE      64
#define TEST_BACKUP_COUNT   2

static uint8 testData[TEST_DATA_SIZE];
static uint8 readBuffer[TEST_DATA_SIZE];
static boolean recoveryPerformed;
static NvM_BlockIdType recoveredBlockId;

void setUp(void) {
    memset(testData, 0xAA, sizeof(testData));
    memset(readBuffer, 0, sizeof(readBuffer));
    recoveryPerformed = FALSE;
    recoveredBlockId = 0;
    
    NvM_Init();
    NvM_Redundant_Init();
}

void tearDown(void) {
    NvM_Redundant_DeInit();
    NvM_DeInit();
}

/* ================================ Callback ================================ */

void NvM_Redundant_RecoveryIndication(NvM_BlockIdType BlockId) {
    recoveryPerformed = TRUE;
    recoveredBlockId = BlockId;
}

/* ================================ 写入测试 ================================ */

/**
 * @brief 测试双备份写入
 * @test NVM_RED_WRITE_001
 */
void test_NvM_Redundant_Dual_Write(void) {
    /* 写入数据 */
    Std_ReturnType result = NvM_Redundant_WriteBlock(TEST_BLOCK_ID, testData);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* 等待写入完成 */
    NvM_MainFunction();
    
    /* 验证Primary和Backup都已写入 */
    TEST_ASSERT_TRUE(NvM_Redundant_IsPrimaryValid(TEST_BLOCK_ID));
    TEST_ASSERT_TRUE(NvM_Redundant_IsBackupValid(TEST_BLOCK_ID));
}

/**
 * @brief 测试CRC校验
 * @test NVM_RED_WRITE_002
 */
void test_NvM_Redundant_CRC_Validation(void) {
    /* 写入数据 */
    NvM_Redundant_WriteBlock(TEST_BLOCK_ID, testData);
    NvM_MainFunction();
    
    /* 读取Primary并验证CRC */
    uint8 primaryData[TEST_DATA_SIZE];
    uint32 primaryCrc;
    NvM_Redundant_ReadPrimary(TEST_BLOCK_ID, primaryData, &primaryCrc);
    
    uint32 calculatedCrc = Crc_CalculateCRC32(primaryData, TEST_DATA_SIZE, 0xFFFFFFFF, TRUE);
    TEST_ASSERT_EQUAL_UINT32(calculatedCrc, primaryCrc);
}

/* ================================ 读取测试 ================================ */

/**
 * @brief 测试正常读取
 * @test NVM_RED_READ_001
 */
void test_NvM_Redundant_Read_Normal(void) {
    /* 先写入数据 */
    NvM_Redundant_WriteBlock(TEST_BLOCK_ID, testData);
    NvM_MainFunction();
    
    /* 读取数据 */
    uint16 length = TEST_DATA_SIZE;
    Std_ReturnType result = NvM_Redundant_ReadBlock(TEST_BLOCK_ID, readBuffer, &length);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL_UINT16(TEST_DATA_SIZE, length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(testData, readBuffer, TEST_DATA_SIZE);
}

/**
 * @brief 测试Primary损坏时从Backup恢复
 * @test NVM_RED_READ_002
 */
void test_NvM_Redundant_Recovery_From_Backup(void) {
    /* 写入数据 */
    NvM_Redundant_WriteBlock(TEST_BLOCK_ID, testData);
    NvM_MainFunction();
    
    /* 模拟Primary损坏 */
    NvM_Redundant_SimulatePrimaryCorruption(TEST_BLOCK_ID);
    
    /* 读取数据 - 应从Backup恢复 */
    uint16 length = TEST_DATA_SIZE;
    Std_ReturnType result = NvM_Redundant_ReadBlock(TEST_BLOCK_ID, readBuffer, &length);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_TRUE(recoveryPerformed);
    TEST_ASSERT_EQUAL(TEST_BLOCK_ID, recoveredBlockId);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(testData, readBuffer, TEST_DATA_SIZE);
}

/**
 * @brief 测试双份都损坏时报错
 * @test NVM_RED_READ_003
 */
void test_NvM_Redundant_Both_Corrupted(void) {
    /* 写入数据 */
    NvM_Redundant_WriteBlock(TEST_BLOCK_ID, testData);
    NvM_MainFunction();
    
    /* 模拟双份都损坏 */
    NvM_Redundant_SimulatePrimaryCorruption(TEST_BLOCK_ID);
    NvM_Redundant_SimulateBackupCorruption(TEST_BLOCK_ID);
    
    /* 读取数据 - 应失败 */
    uint16 length = TEST_DATA_SIZE;
    Std_ReturnType result = NvM_Redundant_ReadBlock(TEST_BLOCK_ID, readBuffer, &length);
    
    TEST_ASSERT_NOT_EQUAL(E_OK, result);
}

/* ================================ 故障检测测试 ================================ */

/**
 * @brief 测试CRC错误检测
 * @test NVM_RED_FAULT_001
 */
void test_NvM_Redundant_CRC_Error_Detection(void) {
    /* 写入数据 */
    NvM_Redundant_WriteBlock(TEST_BLOCK_ID, testData);
    NvM_MainFunction();
    
    /* 模拟CRC错误 */
    NvM_Redundant_SimulateCRCError(TEST_BLOCK_ID);
    
    /* 验证损坏检测 */
    TEST_ASSERT_FALSE(NvM_Redundant_IsPrimaryValid(TEST_BLOCK_ID));
}

/**
 * @brief 测试数据比较（双份不一致）
 * @test NVM_RED_FAULT_002
 */
void test_NvM_Redundant_Data_Mismatch(void) {
    /* 写入数据 */
    NvM_Redundant_WriteBlock(TEST_BLOCK_ID, testData);
    NvM_MainFunction();
    
    /* 模拟数据不一致 */
    NvM_Redundant_SimulateDataMismatch(TEST_BLOCK_ID);
    
    /* 执行检查 */
    NvM_Redundant_CheckConsistency(TEST_BLOCK_ID);
    
    /* 验证检测到不一致 */
    TEST_ASSERT_EQUAL(NVM_RED_DATA_MISMATCH, NvM_Redundant_GetBlockStatus(TEST_BLOCK_ID));
}

/* ================================ 主函数 ================================ */

int main(void) {
    UNITY_BEGIN();
    
    /* 写入测试 */
    RUN_TEST(test_NvM_Redundant_Dual_Write);
    RUN_TEST(test_NvM_Redundant_CRC_Validation);
    
    /* 读取测试 */
    RUN_TEST(test_NvM_Redundant_Read_Normal);
    RUN_TEST(test_NvM_Redundant_Recovery_From_Backup);
    RUN_TEST(test_NvM_Redundant_Both_Corrupted);
    
    /* 故障检测测试 */
    RUN_TEST(test_NvM_Redundant_CRC_Error_Detection);
    RUN_TEST(test_NvM_Redundant_Data_Mismatch);
    
    return UNITY_END();
}
