/**
 * @file test_storage_manager.c
 * @brief Swc_StorageManager host unit tests (main + assert style)
 *
 * Covers init/deinit, block read/write round-trips, CRC/length handling,
 * write protection, invalidate/erase, capacity limits, statistics and write
 * cycle maintenance. RTE port APIs and Det are mocked in this file.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "Swc_StorageManager.h"

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("  FAILED: %s at line %d\n", #cond, __LINE__); \
            return -1; \
        } \
    } while (0)

#define TEST_ASSERT_EQ(a, b) TEST_ASSERT((a) == (b))

static int tests_run = 0;
static int tests_passed = 0;

/* ============================================================================
 * RTE / DET mocks
 * ============================================================================ */
static Std_ReturnType mockWriteResult;
static uint32 mockTime;

static Swc_StorageBlockStatusType lastWrittenStatus;
static int statusWrites;
static int detCalls;
static uint8 lastDetApi;
static uint8 lastDetError;

Std_ReturnType Rte_Write_SWC_STORAGEMANAGER_PORT_BLOCK_STATUS_P(const void* data)
{
    lastWrittenStatus = *(const Swc_StorageBlockStatusType*)data;
    statusWrites++;
    return mockWriteResult;
}

uint32 Rte_GetTime(void)
{
    return mockTime;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId,
                               uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    detCalls++;
    lastDetApi = ApiId;
    lastDetError = ErrorId;
    return E_OK;
}

/* ============================================================================
 * Helpers
 * ============================================================================ */
static void fresh_init(void)
{
    Swc_StorageManager_Deinit();
    Swc_StorageManager_Init();
}

static void mock_reset(void)
{
    memset(&lastWrittenStatus, 0, sizeof(lastWrittenStatus));
    mockWriteResult = E_OK;
    mockTime = 1000U;
    statusWrites = 0;
    detCalls = 0;
    lastDetApi = 0U;
    lastDetError = 0U;
}

/* ============================================================================
 * Tests
 * ============================================================================ */
static int test_uninitialized_paths(void)
{
    uint8 buffer[8];
    Swc_StorageBlockStatusType status;
    Swc_StorageStatisticsType stats;

    mock_reset();

    Swc_StorageManager_100ms();
    Swc_StorageManager_WriteCycle();
    Swc_StorageManager_MainFunction();
    TEST_ASSERT_EQ(detCalls, 0);

    TEST_ASSERT_EQ(Swc_StorageManager_ReadBlock(1U, NULL_PTR, 4U), STORAGE_RESULT_INVALID_DATA);
    TEST_ASSERT_EQ(Swc_StorageManager_ReadBlock(1U, buffer, 0U), STORAGE_RESULT_INVALID_DATA);
    TEST_ASSERT_EQ(Swc_StorageManager_ReadBlock(1U, buffer, 4U), STORAGE_RESULT_NOT_OK);

    TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(1U, NULL_PTR, 4U), STORAGE_RESULT_INVALID_DATA);
    TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(1U, buffer, 0U), STORAGE_RESULT_INVALID_DATA);
    TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(1U, buffer, 257U), STORAGE_RESULT_INVALID_DATA);
    TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(1U, buffer, 4U), STORAGE_RESULT_NOT_OK);

    TEST_ASSERT_EQ(Swc_StorageManager_GetBlockStatus(1U, NULL_PTR), STORAGE_RESULT_INVALID_DATA);
    TEST_ASSERT_EQ(Swc_StorageManager_GetBlockStatus(1U, &status), STORAGE_RESULT_NOT_OK);
    TEST_ASSERT_EQ(Swc_StorageManager_InvalidateBlock(1U), STORAGE_RESULT_NOT_OK);
    TEST_ASSERT_EQ(Swc_StorageManager_EraseBlock(1U), STORAGE_RESULT_NOT_OK);
    TEST_ASSERT_EQ(Swc_StorageManager_GetStatistics(NULL_PTR), STORAGE_RESULT_INVALID_DATA);
    TEST_ASSERT_EQ(Swc_StorageManager_GetStatistics(&stats), STORAGE_RESULT_NOT_OK);
    TEST_ASSERT_EQ(Swc_StorageManager_SetWriteProtection(1U, TRUE), STORAGE_RESULT_NOT_OK);

    Swc_StorageManager_Deinit(); /* no-op */
    return 0;
}

static int test_init_and_deinit(void)
{
    Swc_StorageStatisticsType stats;

    mock_reset();
    Swc_StorageManager_Init();
    TEST_ASSERT_EQ(detCalls, 1);
    TEST_ASSERT_EQ(lastDetApi, 0x01U);
    TEST_ASSERT_EQ(lastDetError, RTE_E_OK);

    Swc_StorageManager_Init();
    TEST_ASSERT_EQ(detCalls, 1);

    TEST_ASSERT_EQ(Swc_StorageManager_GetStatistics(&stats), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(stats.memoryTotal, 32U * 256U);
    TEST_ASSERT_EQ(stats.memoryUsed, 0U);
    TEST_ASSERT_EQ(stats.readOperations, 0U);

    Swc_StorageManager_Deinit();
    TEST_ASSERT_EQ(Swc_StorageManager_GetStatistics(&stats), STORAGE_RESULT_NOT_OK);

    Swc_StorageManager_Init();
    TEST_ASSERT_EQ(Swc_StorageManager_GetStatistics(&stats), STORAGE_RESULT_OK);
    return 0;
}

static int test_write_read_roundtrip(void)
{
    uint8 writeBuf[16];
    uint8 readBuf[32];
    Swc_StorageBlockStatusType status;
    Swc_StorageStatisticsType stats;
    uint16 i;

    mock_reset();
    fresh_init();
    for (i = 0; i < sizeof(writeBuf); i++) {
        writeBuf[i] = (uint8)(0xA0U + i);
    }
    mockTime = 4242U;

    TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(5U, writeBuf, 10U), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(statusWrites, 1);
    TEST_ASSERT_EQ(lastWrittenStatus.blockId, 5U);
    TEST_ASSERT_EQ(lastWrittenStatus.state, STORAGE_BLOCK_VALID);
    TEST_ASSERT_EQ(lastWrittenStatus.dataLength, 10U);
    TEST_ASSERT_EQ(lastWrittenStatus.writeCycleCounter, 1U);
    TEST_ASSERT_EQ(lastWrittenStatus.lastWriteTime, 4242U);
    TEST_ASSERT(lastWrittenStatus.crc != 0U);

    TEST_ASSERT_EQ(Swc_StorageManager_GetBlockStatus(5U, &status), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(status.state, STORAGE_BLOCK_VALID);
    TEST_ASSERT_EQ(status.dataLength, 10U);

    /* Requested length larger than stored data is clamped */
    memset(readBuf, 0, sizeof(readBuf));
    TEST_ASSERT_EQ(Swc_StorageManager_ReadBlock(5U, readBuf, sizeof(readBuf)), STORAGE_RESULT_OK);
    for (i = 0; i < 10U; i++) {
        TEST_ASSERT_EQ(readBuf[i], writeBuf[i]);
    }
    TEST_ASSERT_EQ(readBuf[10], 0U);

    /* Shorter read */
    memset(readBuf, 0, sizeof(readBuf));
    TEST_ASSERT_EQ(Swc_StorageManager_ReadBlock(5U, readBuf, 4U), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(readBuf[0], 0xA0U);
    TEST_ASSERT_EQ(readBuf[3], 0xA3U);
    TEST_ASSERT_EQ(readBuf[4], 0U);

    /* Rewrite existing block does not create a new one */
    TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(5U, writeBuf, 4U), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(Swc_StorageManager_GetBlockStatus(5U, &status), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(status.writeCycleCounter, 2U);
    TEST_ASSERT_EQ(status.dataLength, 4U);

    /* Statistics are only refreshed by the 100ms runnable */
    Swc_StorageManager_100ms();
    TEST_ASSERT_EQ(Swc_StorageManager_GetStatistics(&stats), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(stats.memoryUsed, 256U);
    TEST_ASSERT_EQ(stats.writeOperations, 2U);
    TEST_ASSERT_EQ(stats.readOperations, 2U);
    return 0;
}

static int test_invalid_block_and_state(void)
{
    uint8 buffer[8];
    uint8 data[8] = { 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U };
    Swc_StorageStatisticsType stats;
    Swc_StorageBlockStatusType status;

    mock_reset();
    fresh_init();

    TEST_ASSERT_EQ(Swc_StorageManager_ReadBlock(77U, buffer, 4U), STORAGE_RESULT_INVALID_BLOCK);
    TEST_ASSERT_EQ(Swc_StorageManager_GetBlockStatus(77U, &status), STORAGE_RESULT_INVALID_BLOCK);
    TEST_ASSERT_EQ(Swc_StorageManager_InvalidateBlock(77U), STORAGE_RESULT_INVALID_BLOCK);
    TEST_ASSERT_EQ(Swc_StorageManager_EraseBlock(77U), STORAGE_RESULT_INVALID_BLOCK);
    TEST_ASSERT_EQ(Swc_StorageManager_SetWriteProtection(77U, TRUE), STORAGE_RESULT_INVALID_BLOCK);

    /* Erased (EMPTY) block cannot be read and counts as a read error */
    TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(2U, data, 8U), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(Swc_StorageManager_EraseBlock(2U), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(Swc_StorageManager_ReadBlock(2U, buffer, 4U), STORAGE_RESULT_NOT_OK);

    /* Invalidated block cannot be read */
    TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(3U, data, 8U), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(Swc_StorageManager_InvalidateBlock(3U), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(Swc_StorageManager_GetBlockStatus(3U, &status), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(status.state, STORAGE_BLOCK_INVALID);
    TEST_ASSERT_EQ(Swc_StorageManager_ReadBlock(3U, buffer, 4U), STORAGE_RESULT_NOT_OK);

    TEST_ASSERT_EQ(Swc_StorageManager_GetStatistics(&stats), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(stats.readErrors, 2U);
    TEST_ASSERT_EQ(stats.eraseOperations, 1U);
    return 0;
}

static int test_write_protection(void)
{
    uint8 data[4] = { 1U, 2U, 3U, 4U };
    uint8 buffer[4];
    Swc_StorageBlockStatusType status;

    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(9U, data, 4U), STORAGE_RESULT_OK);

    TEST_ASSERT_EQ(Swc_StorageManager_SetWriteProtection(9U, TRUE), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(9U, data, 4U), STORAGE_RESULT_WRITE_PROTECTED);
    TEST_ASSERT_EQ(Swc_StorageManager_InvalidateBlock(9U), STORAGE_RESULT_WRITE_PROTECTED);
    TEST_ASSERT_EQ(Swc_StorageManager_EraseBlock(9U), STORAGE_RESULT_WRITE_PROTECTED);

    /* Read is still allowed while protected */
    TEST_ASSERT_EQ(Swc_StorageManager_ReadBlock(9U, buffer, 4U), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(buffer[3], 4U);

    TEST_ASSERT_EQ(Swc_StorageManager_SetWriteProtection(9U, FALSE), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(9U, data, 2U), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(Swc_StorageManager_GetBlockStatus(9U, &status), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(status.dataLength, 2U);
    return 0;
}

static int test_erase_block(void)
{
    uint8 data[8] = { 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U };
    Swc_StorageBlockStatusType status;
    Swc_StorageStatisticsType stats;

    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(11U, data, 8U), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(Swc_StorageManager_EraseBlock(11U), STORAGE_RESULT_OK);

    TEST_ASSERT_EQ(Swc_StorageManager_GetBlockStatus(11U, &status), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(status.state, STORAGE_BLOCK_EMPTY);
    TEST_ASSERT_EQ(status.dataLength, 0U);
    TEST_ASSERT_EQ(status.crc, 0U);
    /* Write cycle counter is preserved across erase */
    TEST_ASSERT_EQ(status.writeCycleCounter, 1U);

    /* Block can be reused after erase */
    TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(11U, data, 3U), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(Swc_StorageManager_GetBlockStatus(11U, &status), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(status.state, STORAGE_BLOCK_VALID);
    TEST_ASSERT_EQ(status.writeCycleCounter, 2U);

    TEST_ASSERT_EQ(Swc_StorageManager_GetStatistics(&stats), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(stats.eraseOperations, 1U);
    return 0;
}

static int test_capacity_limit(void)
{
    uint8 data[4] = { 0U };
    uint16 id;

    mock_reset();
    fresh_init();

    for (id = 0U; id < 32U; id++) {
        TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(id, data, 4U), STORAGE_RESULT_OK);
    }
    TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(32U, data, 4U), STORAGE_RESULT_MEMORY_FULL);

    /* Existing blocks are still writable at capacity */
    TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(0U, data, 2U), STORAGE_RESULT_OK);

    Swc_StorageManager_100ms();
    {
        Swc_StorageStatisticsType stats;
        TEST_ASSERT_EQ(Swc_StorageManager_GetStatistics(&stats), STORAGE_RESULT_OK);
        TEST_ASSERT_EQ(stats.memoryUsed, 32U * 256U);
    }
    return 0;
}

static int test_write_cycle_maintenance(void)
{
    uint8 byte = 0x5AU;
    uint32 i;
    Swc_StorageBlockStatusType status;

    mock_reset();
    fresh_init();

    TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(21U, &byte, 1U), STORAGE_RESULT_OK);
    for (i = 0U; i < 100000U; i++) {
        TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(21U, &byte, 1U), STORAGE_RESULT_OK);
    }
    TEST_ASSERT_EQ(Swc_StorageManager_GetBlockStatus(21U, &status), STORAGE_RESULT_OK);
    TEST_ASSERT_EQ(status.writeCycleCounter, 100001U);

    detCalls = 0;
    Swc_StorageManager_100ms();
    TEST_ASSERT(detCalls >= 1);
    TEST_ASSERT_EQ(lastDetApi, 0x50U);

    /* WriteCycle runnable walks blocks with non-zero counters */
    Swc_StorageManager_WriteCycle();
    return 0;
}

static int test_mainfunction_dispatch(void)
{
    uint8 data[4] = { 1U, 2U, 3U, 4U };
    int i;

    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(1U, data, 4U), STORAGE_RESULT_OK);

    for (i = 0; i < 5; i++) {
        Swc_StorageManager_MainFunction();
    }
    /* No crash, block still readable */
    {
        uint8 buffer[4];
        TEST_ASSERT_EQ(Swc_StorageManager_ReadBlock(1U, buffer, 4U), STORAGE_RESULT_OK);
        TEST_ASSERT_EQ(buffer[0], 1U);
    }
    return 0;
}

static int test_deinit_clears_blocks(void)
{
    uint8 data[4] = { 1U, 2U, 3U, 4U };
    Swc_StorageBlockStatusType status;

    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(Swc_StorageManager_WriteBlock(6U, data, 4U), STORAGE_RESULT_OK);

    Swc_StorageManager_Deinit();
    Swc_StorageManager_Init();
    TEST_ASSERT_EQ(Swc_StorageManager_GetBlockStatus(6U, &status), STORAGE_RESULT_INVALID_BLOCK);
    return 0;
}

/* ============================================================================
 * Runner
 * ============================================================================ */
typedef struct {
    const char* name;
    int (*func)(void);
} TestCaseType;

static const TestCaseType testCases[] = {
    { "uninitialized_paths", test_uninitialized_paths },
    { "init_deinit", test_init_and_deinit },
    { "write_read_roundtrip", test_write_read_roundtrip },
    { "invalid_block_and_state", test_invalid_block_and_state },
    { "write_protection", test_write_protection },
    { "erase_block", test_erase_block },
    { "capacity_limit", test_capacity_limit },
    { "write_cycle_maintenance", test_write_cycle_maintenance },
    { "mainfunction_dispatch", test_mainfunction_dispatch },
    { "deinit_clears_blocks", test_deinit_clears_blocks }
};

int main(void)
{
    size_t i;
    size_t numTests = sizeof(testCases) / sizeof(testCases[0]);

    printf("=== StorageManager unit tests ===\n");
    for (i = 0; i < numTests; i++) {
        int result;
        tests_run++;
        printf("[%zu/%zu] %s ... ", i + 1, numTests, testCases[i].name);
        result = testCases[i].func();
        if (result == 0) {
            tests_passed++;
            printf("PASS\n");
        } else {
            printf("FAIL\n");
        }
    }
    printf("=== %d/%d passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
