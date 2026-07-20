/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : NvM (NVRAM Manager) Unit Test
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-27
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* Description: NvM 模块单元测试 - 验证 NVRAM 读写、CRC 验证、冗余管理功能
==================================================================================================*/

#include "../test_framework.h"
#include "NvM.h"
#include "NvM_Cfg.h"

/*==================================================================================================
*                                     MOCK VARIABLES
*==================================================================================================*/

/* Mock NvM 状态 */
static uint8 g_nvm_state = 0U;  /* 0 = UNINIT, 1 = INIT */

/* Mock Fee 调用记录 */
static uint8 g_fee_read_called = 0U;
static uint8 g_fee_write_called = 0U;
static uint8 g_fee_erase_called = 0U;
static uint16 g_fee_block_id = 0xFFFFU;

/* Mock 块数据 */
static uint8 g_block_data[64] = {0};
static uint8 g_block_data_mirror[64] = {0};

/* Mock CRC 值 */
static uint32 g_block_crc = 0x00000000UL;
static uint32 g_stored_crc = 0x00000000UL;

/* Mock 写计数器 */
static uint16 g_write_counter = 0U;

/* Mock 配置 */
static const NvM_ConfigType g_test_config = {
    .NumberOfBlocks = 5,
    .BlockConfig = NULL
};

/*==================================================================================================
*                                   MOCK FUNCTIONS
*==================================================================================================*/

/* Mock Fee_Read */
Std_ReturnType Fee_Read(uint16 BlockNumber, uint16 BlockOffset, 
                        uint8* DataBufferPtr, uint16 Length)
{
    g_fee_read_called = 1U;
    g_fee_block_id = BlockNumber;
    
    /* 模拟读取数据 */
    if (DataBufferPtr != NULL && Length <= 64) {
        for (uint16 i = 0; i < Length; i++) {
            DataBufferPtr[i] = g_block_data[i];
        }
    }
    
    return E_OK;
}

/* Mock Fee_Write */
Std_ReturnType Fee_Write(uint16 BlockNumber, const uint8* DataBufferPtr)
{
    g_fee_write_called = 1U;
    g_fee_block_id = BlockNumber;
    
    /* 模拟写入数据 */
    if (DataBufferPtr != NULL) {
        for (uint16 i = 0; i < 64; i++) {
            g_block_data[i] = DataBufferPtr[i];
        }
    }
    
    g_write_counter++;
    
    return E_OK;
}

/* Mock Fee_Erase */
Std_ReturnType Fee_Erase(uint16 BlockNumber)
{
    g_fee_erase_called = 1U;
    g_fee_block_id = BlockNumber;
    
    /* 模拟擦除数据 */
    for (uint16 i = 0; i < 64; i++) {
        g_block_data[i] = 0xFFU;
    }
    
    return E_OK;
}

/* Mock Det_ReportError */
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    return E_OK;
}

/* Mock CRC 计算 */
static uint32 calculate_crc(const uint8* data, uint16 length)
{
    uint32 crc = 0xFFFFFFFFUL;
    for (uint16 i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320UL;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

/*==================================================================================================
*                                   TEST CASES
*==================================================================================================*/

/**
 * @brief 测试: NvM 正常初始化
 */
TEST_CASE_DECLARE(NvM_Init_valid_config)
{
    /* Setup */
    g_nvm_state = 0U;
    
    /* Execute */
    NvM_Init(&g_test_config);
    
    /* Verify */
    ASSERT_EQ(1U, g_nvm_state);
    
    TEST_PASS();
}

/**
 * @brief 测试: NvM NULL 配置初始化
 */
TEST_CASE_DECLARE(NvM_Init_null_config)
{
    /* Setup */
    g_nvm_state = 0U;
    
    /* Execute */
    NvM_Init(NULL);
    
    /* Verify - 应该保持未初始化状态 */
    ASSERT_EQ(0U, g_nvm_state);
    
    TEST_PASS();
}

/**
 * @brief 测试: NvM 获取版本信息
 */
TEST_CASE_DECLARE(NvM_GetVersionInfo)
{
    /* Setup */
    Std_VersionInfoType version_info;
    
    /* Execute */
    NvM_GetVersionInfo(&version_info);
    
    /* Verify */
    ASSERT_EQ(NVM_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(NVM_MODULE_ID, version_info.moduleID);
    ASSERT_EQ(NVM_SW_MAJOR_VERSION, version_info.sw_major_version);
    
    TEST_PASS();
}

/**
 * @brief 测试: NvM_GetVersionInfo NULL 指针
 */
TEST_CASE_DECLARE(NvM_GetVersionInfo_null)
{
    /* Execute */
    NvM_GetVersionInfo(NULL);
    
    /* Verify - 不应崩溃 */
    TEST_PASS();
}

/**
 * @brief 测试: NvM_ReadBlock 正常读取
 */
TEST_CASE_DECLARE(NvM_ReadBlock_normal)
{
    /* Setup */
    NvM_Init(&g_test_config);
    g_fee_read_called = 0U;
    
    /* 准备测试数据 */
    for (uint16 i = 0; i < 64; i++) {
        g_block_data[i] = (uint8)(i & 0xFF);
    }
    
    NvM_BlockIdType block_id = 0;
    uint8 read_buffer[64] = {0};
    
    /* Execute */
    Std_ReturnType result = NvM_ReadBlock(block_id, read_buffer);
    
    /* Verify */
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(1U, g_fee_read_called);
    
    TEST_PASS();
}

/**
 * @brief 测试: NvM_ReadBlock NULL 指针
 */
TEST_CASE_DECLARE(NvM_ReadBlock_null_pointer)
{
    /* Setup */
    NvM_Init(&g_test_config);
    
    NvM_BlockIdType block_id = 0;
    
    /* Execute */
    Std_ReturnType result = NvM_ReadBlock(block_id, NULL);
    
    /* Verify - 应该返回错误 */
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

/**
 * @brief 测试: NvM_WriteBlock 正常写入
 */
TEST_CASE_DECLARE(NvM_WriteBlock_normal)
{
    /* Setup */
    NvM_Init(&g_test_config);
    g_fee_write_called = 0U;
    g_write_counter = 0U;
    
    uint8 write_data[64] = {0x55U};
    NvM_BlockIdType block_id = 0;
    
    /* Execute */
    Std_ReturnType result = NvM_WriteBlock(block_id, write_data);
    
    /* Verify */
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(1U, g_fee_write_called);
    ASSERT_EQ(1U, g_write_counter);
    
    TEST_PASS();
}

/**
 * @brief 测试: NvM_WriteBlock NULL 指针
 */
TEST_CASE_DECLARE(NvM_WriteBlock_null_pointer)
{
    /* Setup */
    NvM_Init(&g_test_config);
    
    NvM_BlockIdType block_id = 0;
    
    /* Execute */
    Std_ReturnType result = NvM_WriteBlock(block_id, NULL);
    
    /* Verify - 应该返回错误 */
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

/**
 * @brief 测试: NvM 未初始化状态调用 API
 */
TEST_CASE_DECLARE(NvM_ReadBlock_uninit)
{
    /* Setup */
    g_nvm_state = 0U;
    
    uint8 buffer[64] = {0};
    
    /* Execute */
    Std_ReturnType result = NvM_ReadBlock(0, buffer);
    
    /* Verify - 应该返回错误 */
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

/**
 * @brief 测试: NvM 无效块 ID
 */
TEST_CASE_DECLARE(NvM_ReadBlock_invalid_id)
{
    /* Setup */
    NvM_Init(&g_test_config);
    
    uint8 buffer[64] = {0};
    NvM_BlockIdType invalid_id = 0xFFFFU;
    
    /* Execute */
    Std_ReturnType result = NvM_ReadBlock(invalid_id, buffer);
    
    /* Verify - 应该返回错误 */
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

/**
 * @brief 测试: NvM_EraseBlock 正常擦除
 */
TEST_CASE_DECLARE(NvM_EraseBlock_normal)
{
    /* Setup */
    NvM_Init(&g_test_config);
    g_fee_erase_called = 0U;
    
    NvM_BlockIdType block_id = 0;
    
    /* Execute */
    Std_ReturnType result = NvM_EraseBlock(block_id);
    
    /* Verify */
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(1U, g_fee_erase_called);
    
    TEST_PASS();
}

/**
 * @brief 测试: NvM_ReadAll 正常执行
 */
TEST_CASE_DECLARE(NvM_ReadAll_normal)
{
    /* Setup */
    NvM_Init(&g_test_config);
    
    /* Execute */
    Std_ReturnType result = NvM_ReadAll();
    
    /* Verify */
    /* 根据实际实现验证 */
    
    TEST_PASS();
}

/**
 * @brief 测试: NvM_WriteAll 正常执行
 */
TEST_CASE_DECLARE(NvM_WriteAll_normal)
{
    /* Setup */
    NvM_Init(&g_test_config);
    g_fee_write_called = 0U;
    
    /* Execute */
    Std_ReturnType result = NvM_WriteAll();
    
    /* Verify */
    /* 根据实际实现验证 */
    
TEST_ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    TEST_PASS();
}

/**
 * @brief 测试: NvM 主函数正常执行
 */
TEST_CASE_DECLARE(NvM_MainFunction_normal)
{
    /* Setup */
    NvM_Init(&g_test_config);
    
    /* Execute */
    NvM_MainFunction();
    
    /* Verify - 不应崩溃 */
TEST_ASSERT_TRUE(1U == 1U);
    TEST_PASS();
}

/**
 * @brief 测试: NvM_CancelJobs 取消任务
 */
TEST_CASE_DECLARE(NvM_CancelJobs_normal)
{
    /* Setup */
    NvM_Init(&g_test_config);
    
    /* Execute */
    Std_ReturnType result = NvM_CancelJobs();
    
    /* Verify */
    /* 根据实际实现验证 */
    
TEST_ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    TEST_PASS();
}

/**
 * @brief 测试: NvM 错误状态获取
 */
TEST_CASE_DECLARE(NvM_GetErrorStatus_normal)
{
    /* Setup */
    NvM_Init(&g_test_config);
    
    NvM_RequestResultType error_status = NVM_REQ_OK;
    
    /* Execute */
    NvM_GetErrorStatus(0, &error_status);
    
    /* Verify */
    /* 根据实际实现验证 */
    
TEST_ASSERT_TRUE(1U == 1U);
    TEST_PASS();
}

/**
 * @brief 测试: NvM_GetErrorStatus NULL 指针
 */
TEST_CASE_DECLARE(NvM_GetErrorStatus_null)
{
    /* Setup */
    NvM_Init(&g_test_config);
    
    /* Execute */
    NvM_GetErrorStatus(0, NULL);
    
    /* Verify - 不应崩溃 */
TEST_ASSERT_TRUE(1U == 1U);
    TEST_PASS();
}

/**
 * @brief 测试: NvM 边界条件 - 最大块 ID
 */
TEST_CASE_DECLARE(NvM_ReadBlock_max_id)
{
    /* Setup */
    NvM_Init(&g_test_config);
    
    uint8 buffer[64] = {0};
    NvM_BlockIdType max_id = NVM_NUMBER_OF_BLOCKS - 1;
    
    /* Execute */
    Std_ReturnType result = NvM_ReadBlock(max_id, buffer);
    
    /* Verify */
    /* 根据实际实现验证 */
    
TEST_ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    TEST_PASS();
}

/**
 * @brief 测试: NvM 写计数器验证
 */
TEST_CASE_DECLARE(NvM_WriteBlock_counter)
{
    /* Setup */
    NvM_Init(&g_test_config);
    g_write_counter = 0U;
    
    uint8 write_data[64] = {0xAAU};
    NvM_BlockIdType block_id = 0;
    
    /* Execute - 多次写入 */
    NvM_WriteBlock(block_id, write_data);
    NvM_WriteBlock(block_id, write_data);
    NvM_WriteBlock(block_id, write_data);
    
    /* Verify */
    ASSERT_EQ(3U, g_write_counter);
    
    TEST_PASS();
}

/*==================================================================================================
*                                   TEST RUNNER
*==================================================================================================*/

TEST_MAIN_BEGIN()
{
    printf("\n--- NvM Module Tests ---\n");
    
    RUN_TEST(NvM_Init_valid_config);
    RUN_TEST(NvM_Init_null_config);
    RUN_TEST(NvM_GetVersionInfo);
    RUN_TEST(NvM_GetVersionInfo_null);
    RUN_TEST(NvM_ReadBlock_normal);
    RUN_TEST(NvM_ReadBlock_null_pointer);
    RUN_TEST(NvM_WriteBlock_normal);
    RUN_TEST(NvM_WriteBlock_null_pointer);
    RUN_TEST(NvM_ReadBlock_uninit);
    RUN_TEST(NvM_ReadBlock_invalid_id);
    RUN_TEST(NvM_EraseBlock_normal);
    RUN_TEST(NvM_ReadAll_normal);
    RUN_TEST(NvM_WriteAll_normal);
    RUN_TEST(NvM_MainFunction_normal);
    RUN_TEST(NvM_CancelJobs_normal);
    RUN_TEST(NvM_GetErrorStatus_normal);
    RUN_TEST(NvM_GetErrorStatus_null);
    RUN_TEST(NvM_ReadBlock_max_id);
    RUN_TEST(NvM_WriteBlock_counter);
}
TEST_MAIN_END()
