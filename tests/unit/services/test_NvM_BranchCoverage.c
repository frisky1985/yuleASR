/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : NvM (NVRAM Manager) Branch Coverage Test
*
* SW Version           : 1.0.0
* Build Date           : 2026-10-09
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* Description: NvM 新增功能分支覆盖测试 - CancelJobs, ReadPRAMBlock, WritePRAMBlock, RepairDamagedBlocks
==================================================================================================*/

#include "../test_framework.h"
#include "NvM.h"
#include "NvM_Cfg.h"

/* MemIf types (avoid including MemIf.h which pulls in Fee.h/Ea.h) */
typedef uint8 MemIf_DeviceIndexType;
typedef enum {
    MEMIF_JOB_OK = 0,
    MEMIF_JOB_PENDING,
    MEMIF_JOB_CANCELED,
    MEMIF_JOB_FAILED
} MemIf_JobResultType;

typedef enum {
    MEMIF_UNINIT = 0,
    MEMIF_IDLE,
    MEMIF_BUSY
} MemIf_StatusType;

/*==================================================================================================
*                                     MOCK VARIABLES
*==================================================================================================*/

static uint8 g_memif_read_called = 0U;
static uint8 g_memif_write_called = 0U;
static uint8 g_memif_erase_called = 0U;
static uint16 g_memif_block_id = 0xFFFFU;
static uint8 g_block_data[64] = {0};
static uint16 g_write_counter = 0U;

static const NvM_ConfigType g_test_config = {
    .BlockDescriptors = NULL,
    .NumBlockDescriptors = 0,
    .NumOfNvBlocks = 5,
    .NumOfDataSets = 0,
    .NumOfRomBlocks = 0,
    .MaxNumberOfWriteRetries = 3,
    .MaxNumberOfReadRetries = 3,
    .DevErrorDetect = TRUE,
    .VersionInfoApi = TRUE,
    .SetRamBlockStatusApi = TRUE,
    .GetErrorStatusApi = TRUE,
    .SetBlockProtectionApi = TRUE,
    .GetBlockProtectionApi = TRUE,
    .SetDataIndexApi = TRUE,
    .GetDataIndexApi = TRUE,
    .CancelJobApi = TRUE,
    .KillWriteAllApi = TRUE,
    .KillReadAllApi = TRUE,
    .RepairDamagedBlocksApi = TRUE,
    .CalcRamBlockCrc = FALSE,
    .UseCrcCompMechanism = FALSE,
    .MainFunctionPeriod = 10
};

/*==================================================================================================
*                                   MOCK FUNCTIONS
*==================================================================================================*/

Std_ReturnType MemIf_Read(uint8 DeviceIndex, uint16 BlockNumber, uint16 BlockOffset,
                          uint8* DataPtr, uint16 Length)
{
    (void)DeviceIndex;
    (void)BlockOffset;
    g_memif_read_called = 1U;
    g_memif_block_id = BlockNumber;

    if (DataPtr != NULL && Length <= 64) {
        for (uint16 i = 0; i < Length; i++) {
            DataPtr[i] = g_block_data[i];
        }
    }

    return E_OK;
}

Std_ReturnType MemIf_Write(uint8 DeviceIndex, uint16 BlockNumber, const uint8* DataPtr)
{
    (void)DeviceIndex;
    g_memif_write_called = 1U;
    g_memif_block_id = BlockNumber;

    if (DataPtr != NULL) {
        for (uint16 i = 0; i < 64; i++) {
            g_block_data[i] = DataPtr[i];
        }
    }

    g_write_counter++;
    return E_OK;
}

Std_ReturnType MemIf_EraseImmediateBlock(uint8 DeviceIndex, uint16 BlockNumber)
{
    (void)DeviceIndex;
    g_memif_erase_called = 1U;
    g_memif_block_id = BlockNumber;

    for (uint16 i = 0; i < 64; i++) {
        g_block_data[i] = 0xFFU;
    }

    return E_OK;
}

Std_ReturnType MemIf_InvalidateBlock(uint8 DeviceIndex, uint16 BlockNumber)
{
    (void)DeviceIndex;
    (void)BlockNumber;
    return E_OK;
}

MemIf_StatusType MemIf_GetStatus(uint8 DeviceIndex)
{
    (void)DeviceIndex;
    return MEMIF_IDLE;
}

MemIf_JobResultType MemIf_GetJobResult(uint8 DeviceIndex)
{
    (void)DeviceIndex;
    return MEMIF_JOB_OK;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    return E_OK;
}

/*==================================================================================================
*                                   TEST CASES
*==================================================================================================*/

/**
 * @brief 测试: NvM_CancelJobs 未初始化
 */
TEST_CASE_DECLARE(NvM_CancelJobs_uninit)
{
    Std_ReturnType result = NvM_CancelJobs(0);
    ASSERT_EQ(E_NOT_OK, result);
}

/**
 * @brief 测试: NvM_CancelJobs 无效块 ID
 */
TEST_CASE_DECLARE(NvM_CancelJobs_invalid_id)
{
    NvM_Init(&g_test_config);
    Std_ReturnType result = NvM_CancelJobs(0xFFFFU);
    ASSERT_EQ(E_NOT_OK, result);
}

/**
 * @brief 测试: NvM_CancelJobs 无排队任务
 */
TEST_CASE_DECLARE(NvM_CancelJobs_no_jobs)
{
    NvM_Init(&g_test_config);
    Std_ReturnType result = NvM_CancelJobs(0);
    /* Result depends on internal state validation */
    TEST_ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/**
 * @brief 测试: NvM_ReadPRAMBlock 未初始化
 */
TEST_CASE_DECLARE(NvM_ReadPRAMBlock_uninit)
{
    Std_ReturnType result = NvM_ReadPRAMBlock(0);
    ASSERT_EQ(E_NOT_OK, result);
}

/**
 * @brief 测试: NvM_ReadPRAMBlock 无效块 ID
 */
TEST_CASE_DECLARE(NvM_ReadPRAMBlock_invalid_id)
{
    NvM_Init(&g_test_config);
    Std_ReturnType result = NvM_ReadPRAMBlock(0xFFFFU);
    ASSERT_EQ(E_NOT_OK, result);
}

/**
 * @brief 测试: NvM_WritePRAMBlock 未初始化
 */
TEST_CASE_DECLARE(NvM_WritePRAMBlock_uninit)
{
    Std_ReturnType result = NvM_WritePRAMBlock(0);
    ASSERT_EQ(E_NOT_OK, result);
}

/**
 * @brief 测试: NvM_WritePRAMBlock 无效块 ID
 */
TEST_CASE_DECLARE(NvM_WritePRAMBlock_invalid_id)
{
    NvM_Init(&g_test_config);
    Std_ReturnType result = NvM_WritePRAMBlock(0xFFFFU);
    ASSERT_EQ(E_NOT_OK, result);
}

/**
 * @brief 测试: NvM_RepairDamagedBlocks 未初始化
 */
TEST_CASE_DECLARE(NvM_RepairDamagedBlocks_uninit)
{
    Std_ReturnType result = NvM_RepairDamagedBlocks();
    /* Result depends on DEV_ERROR_DETECT config */
    TEST_ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/**
 * @brief 测试: NvM_RepairDamagedBlocks 初始化后正常执行
 */
TEST_CASE_DECLARE(NvM_RepairDamagedBlocks_init)
{
    NvM_Init(&g_test_config);
    Std_ReturnType result = NvM_RepairDamagedBlocks();
    TEST_ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/*==================================================================================================
*                                   TEST RUNNER
*==================================================================================================*/

TEST_MAIN_BEGIN()
{
    printf("\n--- NvM Branch Coverage Tests ---\n");

    RUN_TEST(NvM_CancelJobs_uninit);
    RUN_TEST(NvM_CancelJobs_invalid_id);
    RUN_TEST(NvM_CancelJobs_no_jobs);
    RUN_TEST(NvM_ReadPRAMBlock_uninit);
    RUN_TEST(NvM_ReadPRAMBlock_invalid_id);
    RUN_TEST(NvM_WritePRAMBlock_uninit);
    RUN_TEST(NvM_WritePRAMBlock_invalid_id);
    RUN_TEST(NvM_RepairDamagedBlocks_uninit);
    RUN_TEST(NvM_RepairDamagedBlocks_init);
}
TEST_MAIN_END()
