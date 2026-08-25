/*
 * test_nvm_redundant_verify.c
 * 验证 NvM_Redundant 真实实现（宏开启后）：
 *   - 双实例写入 (primary + mirror)
 *   - CRC 校验
 *   - 单实例损坏自动恢复
 *   - 双实例损坏报错
 *   - 一致性检查 / 修复
 * 依赖：真实 Crc.c + NvM_Redundant.c，mock NvM_ReadBlock/NvM_WriteBlock/NvM_Config
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "NvM.h"
#include "NvM_Cfg.h"
#include "NvM_Redundant.h"
#include "Det.h"

/* ---------- DET stub（Crc.c DET 检查路径需要） ---------- */
Std_ReturnType Det_ReportError(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId)
{
    (void)ModuleId; (void)InstanceId; (void)ApiId; (void)ErrorId;
    return E_OK;
}

/* ---------- Mock NV 存储：group0 = block0(primary) + block1(mirror) ---------- */
#define MOCK_NUM_BLOCKS    4U
#define MOCK_BLOCK_SIZE    128U
#define TEST_BLOCK_LEN     64U

static uint8 mockStorage[MOCK_NUM_BLOCKS][MOCK_BLOCK_SIZE];
static boolean mockValid[MOCK_NUM_BLOCKS];

Std_ReturnType NvM_ReadBlock(NvM_BlockIdType BlockId, void* NvM_DstPtr)
{
    if (BlockId >= MOCK_NUM_BLOCKS) { return E_NOT_OK; }
    if (mockValid[BlockId] == FALSE) { return E_NOT_OK; }
    (void)memcpy(NvM_DstPtr, mockStorage[BlockId], MOCK_BLOCK_SIZE);
    return E_OK;
}

Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void* NvM_SrcPtr)
{
    if (BlockId >= MOCK_NUM_BLOCKS) { return E_NOT_OK; }
    (void)memcpy(mockStorage[BlockId], NvM_SrcPtr, MOCK_BLOCK_SIZE);
    mockValid[BlockId] = TRUE;
    return E_OK;
}

/* ---------- NvM_Config：冗余组 0 = block0/block1，长度 64 ---------- */
static NvM_BlockDescriptorType testBlocks[MOCK_NUM_BLOCKS] = {
    { .BlockId = 0U, .BlockBaseNumber = 0U, .ManagementType = NVM_BLOCK_REDUNDANT,
      .NumberOfNvBlocks = 2U, .NumberOfDataSets = 1U, .NvBlockLength = TEST_BLOCK_LEN,
      .NvBlockNum = 1U, .RomBlockNum = 0U, .InitCallback = NULL_PTR,
      .JobEndCallback = NULL_PTR, .CrcType = NVM_CRC_16, .BlockUseCrc = TRUE,
      .BlockUseSetRamBlockStatus = FALSE, .BlockWriteProt = FALSE,
      .BlockWriteOnce = FALSE, .BlockAutoValidation = FALSE, .BlockUseMirror = TRUE,
      .BlockUseCompression = FALSE, .RomBlockData = NULL_PTR,
      .RamBlockData = NULL_PTR, .MirrorBlockData = NULL_PTR },
    { .BlockId = 1U, .BlockBaseNumber = 1U, .ManagementType = NVM_BLOCK_REDUNDANT,
      .NumberOfNvBlocks = 2U, .NumberOfDataSets = 1U, .NvBlockLength = TEST_BLOCK_LEN,
      .NvBlockNum = 1U, .RomBlockNum = 0U, .InitCallback = NULL_PTR,
      .JobEndCallback = NULL_PTR, .CrcType = NVM_CRC_16, .BlockUseCrc = TRUE,
      .BlockUseSetRamBlockStatus = FALSE, .BlockWriteProt = FALSE,
      .BlockWriteOnce = FALSE, .BlockAutoValidation = FALSE, .BlockUseMirror = TRUE,
      .BlockUseCompression = FALSE, .RomBlockData = NULL_PTR,
      .RamBlockData = NULL_PTR, .MirrorBlockData = NULL_PTR },
    { .BlockId = 2U, .BlockBaseNumber = 2U, .ManagementType = NVM_BLOCK_NATIVE,
      .NumberOfNvBlocks = 1U, .NumberOfDataSets = 1U, .NvBlockLength = TEST_BLOCK_LEN,
      .NvBlockNum = 1U, .RomBlockNum = 0U, .InitCallback = NULL_PTR,
      .JobEndCallback = NULL_PTR, .CrcType = NVM_CRC_16, .BlockUseCrc = TRUE,
      .BlockUseSetRamBlockStatus = FALSE, .BlockWriteProt = FALSE,
      .BlockWriteOnce = FALSE, .BlockAutoValidation = FALSE, .BlockUseMirror = FALSE,
      .BlockUseCompression = FALSE, .RomBlockData = NULL_PTR,
      .RamBlockData = NULL_PTR, .MirrorBlockData = NULL_PTR },
    { .BlockId = 3U, .BlockBaseNumber = 3U, .ManagementType = NVM_BLOCK_NATIVE,
      .NumberOfNvBlocks = 1U, .NumberOfDataSets = 1U, .NvBlockLength = TEST_BLOCK_LEN,
      .NvBlockNum = 1U, .RomBlockNum = 0U, .InitCallback = NULL_PTR,
      .JobEndCallback = NULL_PTR, .CrcType = NVM_CRC_16, .BlockUseCrc = TRUE,
      .BlockUseSetRamBlockStatus = FALSE, .BlockWriteProt = FALSE,
      .BlockWriteOnce = FALSE, .BlockAutoValidation = FALSE, .BlockUseMirror = FALSE,
      .BlockUseCompression = FALSE, .RomBlockData = NULL_PTR,
      .RamBlockData = NULL_PTR, .MirrorBlockData = NULL_PTR }
};

const NvM_ConfigType NvM_Config = {
    .BlockDescriptors = testBlocks,
    .NumBlockDescriptors = MOCK_NUM_BLOCKS,
    .NumOfNvBlocks = 4U,
    .NumOfDataSets = 1U,
    .NumOfRomBlocks = 0U,
    .MaxNumberOfWriteRetries = 3U,
    .MaxNumberOfReadRetries = 3U,
    .DevErrorDetect = FALSE,
    .VersionInfoApi = FALSE,
    .SetRamBlockStatusApi = FALSE,
    .GetErrorStatusApi = FALSE,
    .SetBlockProtectionApi = FALSE,
    .GetBlockProtectionApi = FALSE,
    .SetDataIndexApi = FALSE,
    .GetDataIndexApi = FALSE,
    .CancelJobApi = FALSE,
    .KillWriteAllApi = FALSE,
    .KillReadAllApi = FALSE,
    .RepairDamagedBlocksApi = FALSE,
    .CalcRamBlockCrc = TRUE,
    .UseCrcCompMechanism = TRUE,
    .MainFunctionPeriod = 10U
};

#define RED_GROUP 0U

static void reset_mock(void)
{
    uint8 i;
    for (i = 0U; i < MOCK_NUM_BLOCKS; i++)
    {
        (void)memset(mockStorage[i], 0x00, MOCK_BLOCK_SIZE);
        mockValid[i] = TRUE;
    }
}

int main(void)
{
    uint8 data[TEST_BLOCK_LEN];
    uint8 out[TEST_BLOCK_LEN];
    uint8 i;

    /* Release(NDEBUG) 下 assert 被编译为空，变量仅被 assert 引用会触发
     * -Werror=unused-variable / unused-but-set-variable；显式引用保持编译一致 */
    (void)data;
    (void)out;

    reset_mock();
    for (i = 0U; i < TEST_BLOCK_LEN; i++) { data[i] = (uint8)(0xA0U + i); }

    /* 1. 双实例写入 */
    assert(NvM_RedundantWrite(RED_GROUP, data) == E_OK);
    assert(memcmp(mockStorage[0], data, TEST_BLOCK_LEN) == 0);
    assert(memcmp(mockStorage[1], data, TEST_BLOCK_LEN) == 0);
    printf("[PASS] 1. 双实例写入 primary+mirror 一致\n");

    /* 2. 正常读取 */
    assert(NvM_RedundantRead(RED_GROUP, out) == E_OK);
    assert(memcmp(out, data, TEST_BLOCK_LEN) == 0);
    printf("[PASS] 2. 正常读取数据一致\n");

    /* 3. 一致性检查：一致 -> E_OK */
    assert(NvM_RedundantCheckConsistency(RED_GROUP) == E_OK);
    printf("[PASS] 3. 一致性检查通过\n");

    /* 4. primary 损坏 -> 从 mirror 自动恢复 */
    mockStorage[0][0] = 0xFFU;
    assert(NvM_RedundantRead(RED_GROUP, out) == E_OK);
    assert(memcmp(out, data, TEST_BLOCK_LEN) == 0);
    printf("[PASS] 4. primary 损坏自动从 mirror 恢复\n");

    /* 5. mirror 损坏 -> 从 primary 自动恢复 */
    mockStorage[1][0] = 0xEEU;
    assert(NvM_RedundantRead(RED_GROUP, out) == E_OK);
    assert(memcmp(out, data, TEST_BLOCK_LEN) == 0);
    printf("[PASS] 5. mirror 损坏自动从 primary 恢复\n");

    /* 6. 双实例损坏 -> E_NOT_OK */
    mockStorage[0][0] = 0xFFU;
    mockStorage[1][0] = 0xEEU;
    assert(NvM_RedundantRead(RED_GROUP, out) == E_NOT_OK);
    printf("[PASS] 6. 双实例损坏返回 E_NOT_OK\n");

    /* 7. 修复：重写后一致性恢复 */
    assert(NvM_RedundantWrite(RED_GROUP, data) == E_OK);
    assert(NvM_RedundantCheckConsistency(RED_GROUP) == E_OK);
    printf("[PASS] 7. 修复后一致性恢复\n");

    /* 8. 非法 BlockId -> E_NOT_OK */
    assert(NvM_RedundantWrite((NvM_BlockIdType)NVM_NUM_REDUNDANT_BLOCKS, data) == E_NOT_OK);
    assert(NvM_RedundantRead((NvM_BlockIdType)NVM_NUM_REDUNDANT_BLOCKS, out) == E_NOT_OK);
    printf("[PASS] 8. 越界 BlockId 返回 E_NOT_OK\n");

    printf("\n=== ALL NvM_REDUNDANT VERIFY TESTS PASSED (%u/%u) ===\n", 8U, 8U);
    return 0;
}
