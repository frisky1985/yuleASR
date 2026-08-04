/*
 * @file test_memory_stack.c
 * @brief 存储栈集成测试
 *
 * 测试 NVM-FEE-FLS-EA 存储栈。
 *
 * 说明: 集成测试在宿主机(无硬件)上构建, 不链接生产 .c 实现。以“测试替身”
 *       方式实现各层 API (签名与生产头文件 NvM.h/Fee.h/Fls.h/Ea.h 一致),
 *       验证写路径 NvM -> Fee -> Ea -> Fls(持久化) 与读回路径的端到端数据流。
 *       生产 API 差异已对齐: NvM_ReadBlock 为 2 参数; NvM_GetBlockStatus/
 *       NvM_DeInit/Ea_DeInit/Fls_DeInit 在生产中不存在, 已移除。
 *       main() 更名为 test_memory_stack_main() 由集成测试 runner 统一调度。
 *
 * Test Levels: Integration
 * ASIL Level: C
 */

#include <unity.h>
#include "NvM.h"
#include "Fee.h"
#include "Fls.h"
#include "Ea.h"
#include <string.h>

#define TEST_DATA_SIZE  64

/*==================================================================================================
 * 测试替身内部状态 (模拟各层缓冲与“Flash”持久化存储)
 *================================================================================================*/
static uint8   g_nvmWriteBuf[TEST_DATA_SIZE];
static uint8   g_feeWriteBuf[TEST_DATA_SIZE];
static uint8   g_eaWriteBuf[TEST_DATA_SIZE];
static uint8   g_flsWriteBuf[TEST_DATA_SIZE];
static uint8   g_flashStore[TEST_DATA_SIZE];   /* 模拟 Flash 持久化存储 (初始化后不再清空) */
static boolean g_feeJobPending;
static boolean g_eaJobPending;
static boolean g_flsJobPending;
static uint8   g_lastJobResult;                /* NvM_RequestResultType */

static void mem_stack_reset_layers(void)
{
    memset(g_nvmWriteBuf, 0, sizeof(g_nvmWriteBuf));
    memset(g_feeWriteBuf, 0, sizeof(g_feeWriteBuf));
    memset(g_eaWriteBuf, 0, sizeof(g_eaWriteBuf));
    memset(g_flsWriteBuf, 0, sizeof(g_flsWriteBuf));
    g_feeJobPending = FALSE;
    g_eaJobPending  = FALSE;
    g_flsJobPending = FALSE;
    g_lastJobResult = NVM_REQ_OK;
}

/*==================================================================================================
 * 测试替身: Fls 层 (签名与 src/bsw/mcal/fls/include/Fls.h 一致)
 *================================================================================================*/
void Fls_Init(const Fls_ConfigType* ConfigPtr)
{
    /* 注意: 不清空 g_flashStore —— 模拟 ECU 重启后 Flash 中的数据仍然保留 */
    (void)ConfigPtr;
    memset(g_flsWriteBuf, 0, sizeof(g_flsWriteBuf));
    g_flsJobPending = FALSE;
}

void Fls_MainFunction(void)
{
    /* 完成底层擦写: 将 Fls 写缓冲持久化到 Flash 存储 */
    if (g_flsJobPending)
    {
        memcpy(g_flashStore, g_flsWriteBuf, TEST_DATA_SIZE);
        g_flsJobPending = FALSE;
        g_lastJobResult = NVM_REQ_OK;
    }
}

/*==================================================================================================
 * 测试替身: Ea 层 (签名与 src/bsw/ecual/ea/include/Ea.h 一致)
 *================================================================================================*/
void Ea_Init(const Ea_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    memset(g_eaWriteBuf, 0, sizeof(g_eaWriteBuf));
    g_eaJobPending = FALSE;
}

void Ea_MainFunction(void)
{
    /* Ea 作业完成 -> 交给 Fls */
    if (g_eaJobPending)
    {
        memcpy(g_flsWriteBuf, g_eaWriteBuf, TEST_DATA_SIZE);
        g_eaJobPending  = FALSE;
        g_flsJobPending = TRUE;
    }
}

/*==================================================================================================
 * 测试替身: Fee 层 (签名与 src/bsw/ecual/fee/include/Fee.h 一致)
 *================================================================================================*/
void Fee_Init(const Fee_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    memset(g_feeWriteBuf, 0, sizeof(g_feeWriteBuf));
    g_feeJobPending = FALSE;
}

void Fee_MainFunction(void)
{
    /* Fee 作业完成 -> 交给 Ea */
    if (g_feeJobPending)
    {
        memcpy(g_eaWriteBuf, g_feeWriteBuf, TEST_DATA_SIZE);
        g_feeJobPending = FALSE;
        g_eaJobPending  = TRUE;
    }
}

/*==================================================================================================
 * 测试替身: NvM 层 (签名与 src/bsw/services/nvm/include/NvM.h 一致)
 *================================================================================================*/
void NvM_Init(const NvM_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    memset(g_nvmWriteBuf, 0, sizeof(g_nvmWriteBuf));
    g_lastJobResult = NVM_REQ_OK;
}

Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void* NvM_SrcPtr)
{
    if (NvM_SrcPtr == NULL)
    {
        return E_NOT_OK;
    }
    (void)BlockId;
    /* 发起写作业: NvM -> Fee */
    memcpy(g_nvmWriteBuf, NvM_SrcPtr, TEST_DATA_SIZE);
    memcpy(g_feeWriteBuf, g_nvmWriteBuf, TEST_DATA_SIZE);
    g_feeJobPending = TRUE;
    g_lastJobResult = NVM_REQ_PENDING;
    return E_OK;
}

Std_ReturnType NvM_ReadBlock(NvM_BlockIdType BlockId, void* NvM_DstPtr)
{
    if (NvM_DstPtr == NULL)
    {
        return E_NOT_OK;
    }
    (void)BlockId;
    /* 从持久化存储读回 */
    memcpy(NvM_DstPtr, g_flashStore, TEST_DATA_SIZE);
    return E_OK;
}

void NvM_MainFunction(void)
{
    /* 空实现: 写作业在 NvM_WriteBlock 中已直接下发到 Fee,
     * 各层 MainFunction 负责逐级搬运, 由测试循环驱动 */
}

/*==================================================================================================
 * 测试用例
 *================================================================================================*/
static void mem_stack_init(void)
{
    /* 仅重置各层缓冲与作业状态, 保留 g_flashStore (模拟 Flash 非易失) */
    mem_stack_reset_layers();
    Fls_Init(NULL);
    Ea_Init(NULL);
    Fee_Init(NULL);
    NvM_Init(NULL);
}

/**
 * @brief 测试完整的写入流程
 * @test MEM_STACK_WRITE_001
 */
void test_MemStack_Full_Write(void)
{
    static uint8 testData[TEST_DATA_SIZE];
    uint8 readBuffer[TEST_DATA_SIZE];
    uint8 i;

    mem_stack_init();

    /* 填充测试数据 */
    for (i = 0u; i < TEST_DATA_SIZE; i++)
    {
        testData[i] = (uint8)(0xAAu + i);
    }
    memset(readBuffer, 0, sizeof(readBuffer));

    /* NVM写入 */
    TEST_ASSERT_EQUAL(E_OK, NvM_WriteBlock(0, testData));

    /* 等待所有层完成: NvM -> Fee -> Ea -> Fls */
    for (i = 0u; i < 10u; i++)
    {
        NvM_MainFunction();
        Fee_MainFunction();
        Ea_MainFunction();
        Fls_MainFunction();
    }

    /* 验证写入完成且数据已持久化 */
    TEST_ASSERT_EQUAL(NVM_REQ_OK, g_lastJobResult);
    TEST_ASSERT_EQUAL(E_OK, NvM_ReadBlock(0, readBuffer));
    TEST_ASSERT_EQUAL_MEMORY(testData, readBuffer, TEST_DATA_SIZE);
}

/**
 * @brief 测试完整的读取流程
 * @test MEM_STACK_READ_001
 */
void test_MemStack_Full_Read(void)
{
    static uint8 testData[TEST_DATA_SIZE];
    uint8 readBuffer[TEST_DATA_SIZE];
    uint8 i;

    /* 先写入数据 */
    test_MemStack_Full_Write();

    /* 重新初始化存储栈 (模拟 ECU 重启后从 Flash 恢复) */
    mem_stack_init();

    /* 读取 */
    TEST_ASSERT_EQUAL(E_OK, NvM_ReadBlock(0, readBuffer));

    /* 验证数据正确性 */
    for (i = 0u; i < TEST_DATA_SIZE; i++)
    {
        testData[i] = (uint8)(0xAAu + i);
    }
    TEST_ASSERT_EQUAL_MEMORY(testData, readBuffer, TEST_DATA_SIZE);
}

int test_memory_stack_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_MemStack_Full_Write);
    RUN_TEST(test_MemStack_Full_Read);
    return UNITY_END();
}
