/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : NvM Unit Test
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-24
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include <stdio.h>
#include <string.h>
#include "test_framework.h"
#include "NvM.h"

/*==================================================================================================
*                                     TEST DATA
==================================================================================================*/
static uint8 testRomData[64] = {
    0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U,
    0x08U, 0x09U, 0x0AU, 0x0BU, 0x0CU, 0x0DU, 0x0EU, 0x0FU,
    0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U,
    0x18U, 0x19U, 0x1AU, 0x1BU, 0x1CU, 0x1DU, 0x1EU, 0x1FU,
    0x20U, 0x21U, 0x22U, 0x23U, 0x24U, 0x25U, 0x26U, 0x27U,
    0x28U, 0x29U, 0x2AU, 0x2BU, 0x2CU, 0x2DU, 0x2EU, 0x2FU,
    0x30U, 0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U,
    0x38U, 0x39U, 0x3AU, 0x3BU, 0x3CU, 0x3DU, 0x3EU, 0x3FU
};

static uint8 testRamBuffer[64];
static uint8 testWriteBuffer[64];

/*==================================================================================================
*                                     MOCK VARIABLES
==================================================================================================*/
static uint8 mock_memif_read_called = 0U;
static uint8 mock_memif_write_called = 0U;
static MemIf_StatusType mock_memif_status = MEMIF_IDLE;

static uint8 mock_det_report_error_called = 0U;
static uint16 mock_det_module_id = 0xFFFFU;
static uint8 mock_det_instance_id = 0xFFU;
static uint8 mock_det_api_id = 0xFFU;
static uint8 mock_det_error_id = 0xFFU;

/*==================================================================================================
*                                     MOCK FUNCTIONS
==================================================================================================*/

/* MemIf mocks */
Std_ReturnType MemIf_Read(MemIf_DeviceIdType DeviceIndex,
                           MemIf_BlockIdType BlockNumber,
                           uint16 BlockOffset,
                           uint8* DataBufferPtr,
                           uint16 Length)
{
    (void)DeviceIndex;
    (void)BlockNumber;
    (void)BlockOffset;
    (void)DataBufferPtr;
    (void)Length;
    mock_memif_read_called++;
    return E_OK;
}

Std_ReturnType MemIf_Write(MemIf_DeviceIdType DeviceIndex,
                            MemIf_BlockIdType BlockNumber,
                            const uint8* DataBufferPtr)
{
    (void)DeviceIndex;
    (void)BlockNumber;
    (void)DataBufferPtr;
    mock_memif_write_called++;
    return E_OK;
}

MemIf_StatusType MemIf_GetStatus(MemIf_DeviceIdType DeviceIndex)
{
    (void)DeviceIndex;
    return mock_memif_status;
}

Std_ReturnType MemIf_InvalidateBlock(MemIf_DeviceIdType DeviceIndex,
                                      MemIf_BlockIdType BlockNumber)
{
    (void)DeviceIndex;
    (void)BlockNumber;
    return E_OK;
}

Std_ReturnType MemIf_EraseImmediateBlock(MemIf_DeviceIdType DeviceIndex,
                                          MemIf_BlockIdType BlockNumber)
{
    (void)DeviceIndex;
    (void)BlockNumber;
    return E_OK;
}

/* Det mock */
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    mock_det_module_id = ModuleId;
    mock_det_instance_id = InstanceId;
    mock_det_api_id = ApiId;
    mock_det_error_id = ErrorId;
    mock_det_report_error_called++;
    return E_OK;
}

/*==================================================================================================
*                                  TEST CONFIGURATION
==================================================================================================*/
static const NvM_BlockDescriptorType testBlockDescriptors[] = {
    {
        /* Block 1: Native block with ROM defaults */
        .BlockId = NVM_BLOCK_ID_CONFIG,
        .BlockBaseNumber = 0x0001U,
        .ManagementType = NVM_BLOCK_NATIVE,
        .NumberOfNvBlocks = 1U,
        .NumberOfDataSets = 1U,
        .NvBlockLength = 64U,
        .NvBlockNum = 1U,
        .RomBlockNum = 1U,
        .InitCallback = 0U,
        .JobEndCallback = 0U,
        .CrcType = NVM_CRC_NONE,
        .BlockUseCrc = FALSE,
        .BlockUseSetRamBlockStatus = TRUE,
        .BlockWriteProt = FALSE,
        .BlockWriteOnce = FALSE,
        .BlockAutoValidation = FALSE,
        .BlockUseMirror = FALSE,
        .BlockUseCompression = FALSE,
        .RomBlockData = testRomData,
        .RamBlockData = testRamBuffer,
        .MirrorBlockData = NULL_PTR
    },
    {
        /* Block 2: Write protected block */
        .BlockId = NVM_BLOCK_ID_CALIBRATION,
        .BlockBaseNumber = 0x0002U,
        .ManagementType = NVM_BLOCK_NATIVE,
        .NumberOfNvBlocks = 1U,
        .NumberOfDataSets = 1U,
        .NvBlockLength = 64U,
        .NvBlockNum = 1U,
        .RomBlockNum = 1U,
        .InitCallback = 0U,
        .JobEndCallback = 0U,
        .CrcType = NVM_CRC_NONE,
        .BlockUseCrc = FALSE,
        .BlockUseSetRamBlockStatus = TRUE,
        .BlockWriteProt = TRUE,
        .BlockWriteOnce = FALSE,
        .BlockAutoValidation = FALSE,
        .BlockUseMirror = FALSE,
        .BlockUseCompression = FALSE,
        .RomBlockData = testRomData,
        .RamBlockData = testRamBuffer,
        .MirrorBlockData = NULL_PTR
    },
    {
        /* Block 3: Write-once block */
        .BlockId = NVM_BLOCK_ID_VIN,
        .BlockBaseNumber = 0x0003U,
        .ManagementType = NVM_BLOCK_NATIVE,
        .NumberOfNvBlocks = 1U,
        .NumberOfDataSets = 1U,
        .NvBlockLength = 17U,
        .NvBlockNum = 1U,
        .RomBlockNum = 1U,
        .InitCallback = 0U,
        .JobEndCallback = 0U,
        .CrcType = NVM_CRC_NONE,
        .BlockUseCrc = FALSE,
        .BlockUseSetRamBlockStatus = TRUE,
        .BlockWriteProt = FALSE,
        .BlockWriteOnce = TRUE,
        .BlockAutoValidation = FALSE,
        .BlockUseMirror = FALSE,
        .BlockUseCompression = FALSE,
        .RomBlockData = testRomData,
        .RamBlockData = testRamBuffer,
        .MirrorBlockData = NULL_PTR
    }
};

const NvM_ConfigType NvM_Config = {
    .BlockDescriptors = testBlockDescriptors,
    .NumBlockDescriptors = 3U,
    .NumOfNvBlocks = 3U,
    .NumOfDataSets = 1U,
    .NumOfRomBlocks = 3U,
    .MaxNumberOfWriteRetries = 3U,
    .MaxNumberOfReadRetries = 3U,
    .DevErrorDetect = TRUE,
    .VersionInfoApi = TRUE,
    .SetRamBlockStatusApi = TRUE,
    .GetErrorStatusApi = TRUE,
    .SetBlockProtectionApi = TRUE,
    .GetBlockProtectionApi = FALSE,
    .SetDataIndexApi = TRUE,
    .GetDataIndexApi = FALSE,
    .CancelJobApi = TRUE,
    .KillWriteAllApi = FALSE,
    .KillReadAllApi = FALSE,
    .RepairDamagedBlocksApi = FALSE,
    .CalcRamBlockCrc = TRUE,
    .UseCrcCompMechanism = TRUE,
    .MainFunctionPeriod = 10U
};

/*==================================================================================================
*                                  TEST HELPER FUNCTIONS
==================================================================================================*/
static void reset_mocks(void)
{
    mock_memif_read_called = 0U;
    mock_memif_write_called = 0U;
    mock_memif_status = MEMIF_IDLE;

    mock_det_report_error_called = 0U;
    mock_det_module_id = 0xFFFFU;
    mock_det_instance_id = 0xFFU;
    mock_det_api_id = 0xFFU;
    mock_det_error_id = 0xFFU;
}

static void reset_test_buffers(void)
{
    (void)memset(testRamBuffer, 0x00U, sizeof(testRamBuffer));
    (void)memset(testWriteBuffer, 0xAAU, sizeof(testWriteBuffer));
}

/*==================================================================================================
*                                    TEST CASES
==================================================================================================*/

/* Test 1: NvM_Init with valid config */
void test_nvm_init_valid_config(void)
{
    reset_mocks();
    reset_test_buffers();

    NvM_Init(&NvM_Config);

    ASSERT_EQ(0U, mock_det_report_error_called);
    TEST_PASS();
}

/* Test 2: NvM_Init with NULL config reports DET error */
void test_nvm_init_null_config(void)
{
    reset_mocks();

    NvM_Init(NULL_PTR);

    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(NVM_MODULE_ID, mock_det_module_id);
    ASSERT_EQ(NVM_E_PARAM_POINTER, mock_det_error_id);
    TEST_PASS();
}

/* Test 3: NvM_DeInit success */
/* TODO: enable after implementation */
#if 0
void test_nvm_deinit_success(void)
{
    reset_mocks();

    NvM_Init(&NvM_Config);
    NvM_DeInit();

    /* After deinit, APIs should report not initialized */
    ASSERT_EQ(0U, mock_det_report_error_called);
    TEST_PASS();
}
#endif

/* Test 4: NvM_ReadBlock queues successfully */
void test_nvm_readblock_success(void)
{
    Std_ReturnType result;

    reset_mocks();
    reset_test_buffers();
    NvM_Init(&NvM_Config);

    result = NvM_ReadBlock(NVM_BLOCK_ID_CONFIG, testRamBuffer);

    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(0U, mock_det_report_error_called);
    TEST_PASS();
}

/* Test 5: NvM_ReadBlock when not initialized reports DET error */
void test_nvm_readblock_uninit(void)
{
    Std_ReturnType result;

    reset_mocks();
    reset_test_buffers();
    /* Note: NvM is uninitialized because we don't call NvM_Init here */

    result = NvM_ReadBlock(NVM_BLOCK_ID_CONFIG, testRamBuffer);

    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(NVM_E_NOT_INITIALIZED, mock_det_error_id);
    TEST_PASS();
}

/* Test 6: NvM_WriteBlock queues successfully */
void test_nvm_writeblock_success(void)
{
    Std_ReturnType result;

    reset_mocks();
    reset_test_buffers();
    NvM_Init(&NvM_Config);

    result = NvM_WriteBlock(NVM_BLOCK_ID_CONFIG, testWriteBuffer);

    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(0U, mock_det_report_error_called);
    TEST_PASS();
}

/* Test 7: NvM_WriteBlock on write protected block returns error */
/* TODO: enable after implementation */
#if 0
void test_nvm_writeblock_writeprot(void)
{
    Std_ReturnType result;

    reset_mocks();
    reset_test_buffers();
    NvM_Init(&NvM_Config);

    result = NvM_WriteBlock(NVM_BLOCK_ID_CALIBRATION, testWriteBuffer);

    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(NVM_E_WRITE_PROTECTED, mock_det_error_id);
    TEST_PASS();
}
#endif

/* Test 8: NvM_WriteBlock on WriteOnce block after first write returns error */
/* TODO: enable after implementation */
#if 0
void test_nvm_writeblock_once_protection(void)
{
    Std_ReturnType result;

    reset_mocks();
    reset_test_buffers();
    NvM_Init(&NvM_Config);

    /* First write should succeed */
    result = NvM_WriteBlock(NVM_BLOCK_ID_VIN, testWriteBuffer);
    ASSERT_EQ(E_OK, result);

    /* Second write should fail */
    reset_mocks();
    result = NvM_WriteBlock(NVM_BLOCK_ID_VIN, testWriteBuffer);
    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(NVM_E_WRITE_PROTECTED, mock_det_error_id);
    TEST_PASS();
}
#endif

/* Test 9: NvM_RestoreBlockDefaults copies ROM data */
void test_nvm_restoreblockdefaults(void)
{
    Std_ReturnType result;
    uint8 i;

    reset_mocks();
    reset_test_buffers();
    NvM_Init(&NvM_Config);

    result = NvM_RestoreBlockDefaults(NVM_BLOCK_ID_CONFIG, testRamBuffer);

    ASSERT_EQ(E_OK, result);

    /* Process the restore job via MainFunction */
    NvM_MainFunction();

    /* Verify ROM data was copied to RAM */
    for (i = 0U; i < 64U; i++)
    {
        ASSERT_EQ(testRomData[i], testRamBuffer[i]);
    }
    TEST_PASS();
}

/* Test 10: NvM_GetErrorStatus returns block error status */
void test_nvm_geterrorstatus(void)
{
    Std_ReturnType result;
    NvM_RequestResultType requestResult;

    reset_mocks();
    NvM_Init(&NvM_Config);

    result = NvM_GetErrorStatus(NVM_BLOCK_ID_CONFIG, &requestResult);

    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(NVM_REQ_OK, requestResult);
    TEST_PASS();
}

/* Test 11: NvM_SetRamBlockStatus sets dirty flag */
void test_nvm_setramblockstatus(void)
{
    Std_ReturnType result;
    NvM_RequestResultType requestResult;

    reset_mocks();
    NvM_Init(&NvM_Config);

    /* Set block as changed (dirty) */
    result = NvM_SetRamBlockStatus(NVM_BLOCK_ID_CONFIG, TRUE);
    ASSERT_EQ(E_OK, result);

    /* WriteBlock should set DataChanged = TRUE, verify via indirect means */
    result = NvM_WriteBlock(NVM_BLOCK_ID_CONFIG, testWriteBuffer);
    ASSERT_EQ(E_OK, result);

    /* Get error status should still be OK after write queueing */
    result = NvM_GetErrorStatus(NVM_BLOCK_ID_CONFIG, &requestResult);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(NVM_REQ_PENDING, requestResult);
    TEST_PASS();
}

/* Test 12: NvM_ReadAll queues read jobs for all configured blocks */
/* TODO: enable after implementation */
#if 0
void test_nvm_readall_queues_blocks(void)
{
    Std_ReturnType result;

    reset_mocks();
    NvM_Init(&NvM_Config);

    result = NvM_ReadAll();

    ASSERT_EQ(E_OK, result);
    /* All configured blocks should have pending read jobs */
    TEST_PASS();
}
#endif

/* Test 13: NvM_WriteAll queues write jobs only for dirty blocks */
/* TODO: enable after implementation */
#if 0
void test_nvm_writeall_queues_dirty_blocks(void)
{
    Std_ReturnType result;

    reset_mocks();
    NvM_Init(&NvM_Config);

    /* Mark one block as dirty */
    (void)NvM_SetRamBlockStatus(NVM_BLOCK_ID_CONFIG, TRUE);

    result = NvM_WriteAll();

    ASSERT_EQ(E_OK, result);
    /* Only dirty blocks should have pending write jobs */
    TEST_PASS();
}
#endif

/* Test 14: NvM_CancelJobs removes pending jobs from queue */
/* TODO: enable after implementation */
#if 0
void test_nvm_canceljobs_removes_pending(void)
{
    Std_ReturnType result;

    reset_mocks();
    NvM_Init(&NvM_Config);

    /* Queue a read job */
    result = NvM_ReadBlock(NVM_BLOCK_ID_CONFIG, testRamBuffer);
    ASSERT_EQ(E_OK, result);

    /* Cancel the job */
    result = NvM_CancelJobs(NVM_BLOCK_ID_CONFIG);
    ASSERT_EQ(E_OK, result);

    /* Block should no longer be pending */
    /* Verify via GetErrorStatus or by checking queue state */
    TEST_PASS();
}
#endif

/* Test 15: NvM_MainFunction processes jobs from queue */
void test_nvm_mainfunction_processes_jobs(void)
{
    Std_ReturnType result;
    NvM_RequestResultType requestResult;

    reset_mocks();
    reset_test_buffers();
    NvM_Init(&NvM_Config);

    /* Queue a restore job (immediate queue, does not require DeviceId) */
    result = NvM_RestoreBlockDefaults(NVM_BLOCK_ID_CONFIG, testRamBuffer);
    ASSERT_EQ(E_OK, result);

    /* Process the job */
    NvM_MainFunction();

    /* Job should be completed */
    result = NvM_GetErrorStatus(NVM_BLOCK_ID_CONFIG, &requestResult);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(NVM_REQ_OK, requestResult);
    TEST_PASS();
}

/* Test 16: NvM_EraseNvBlock returns success */
void test_nvm_eraseblock_success(void)
{
    Std_ReturnType result;

    reset_mocks();
    NvM_Init(&NvM_Config);

    result = NvM_EraseNvBlock(NVM_BLOCK_ID_CONFIG);

    /* Currently returns E_OK as placeholder */
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test 17: NvM_InvalidateNvBlock returns success */
void test_nvm_invalidateblock_success(void)
{
    Std_ReturnType result;

    reset_mocks();
    NvM_Init(&NvM_Config);

    result = NvM_InvalidateNvBlock(NVM_BLOCK_ID_CONFIG);

    /* Currently returns E_OK as placeholder */
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()

    RUN_TEST(nvm_init_valid_config);
    RUN_TEST(nvm_init_null_config);
    RUN_TEST(nvm_readblock_success);
    RUN_TEST(nvm_readblock_uninit);
    RUN_TEST(nvm_writeblock_success);
    RUN_TEST(nvm_restoreblockdefaults);
    RUN_TEST(nvm_geterrorstatus);
    RUN_TEST(nvm_setramblockstatus);
    RUN_TEST(nvm_mainfunction_processes_jobs);
    RUN_TEST(nvm_eraseblock_success);
    RUN_TEST(nvm_invalidateblock_success);

TEST_MAIN_END()
