/**
 * @file test_mcal_spi.c
 * @brief SPI unit test — links real Spi.c production code
 *
 * 对齐生产头文件 src/bsw/mcal/spi/include/Spi.h（AUTOSAR 4.4 API）:
 *   Spi_Init / Spi_DeInit / Spi_SyncTransmit(DeviceId, TxData, RxData, Length)
 *   Spi_AsyncTransmit / Spi_GetStatus / Spi_GetJobResult(void)
 *   Spi_MainFunction / Spi_IsrHandler / Spi_GetVersionInfo
 *
 * 注意: Spi.c 通过 volatile 指针直接访问 ECSPI 硬件地址（0x30820000 等，
 * 不使用 REG_READ32/REG_WRITE32 宏），在 host 上解引用会段错误。
 * 因此测试覆盖所有不触发寄存器直访的路径:
 *   - ChannelCount = 0 的配置（Spi_Init 的通道配置循环不执行）
 *   - 参数校验失败路径（未初始化 / DeviceId 越界）
 *   - 状态/结果查询与版本信息
 * Spi.c 引用的外部 DMA/GPT 符号在此提供桩实现。
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "mock_hal.h"
#include "mock_hal_config.h"
#include "Spi.h"

/* ---- 桩: Spi.c 引用的外部驱动符号（生产环境由 Dma / Gpt 驱动提供） ---- */
uint32 Gpt_GetTimeElapsed(uint8 Channel)
{
    (void)Channel;
    return 0U;
}

void Dma_ConfigTx(uint8 Channel, uint32 SrcAddr, uint32 DstAddr, uint32 Length)
{
    (void)Channel; (void)SrcAddr; (void)DstAddr; (void)Length;
}

void Dma_ConfigRx(uint8 Channel, uint32 SrcAddr, uint32 DstAddr, uint32 Length)
{
    (void)Channel; (void)SrcAddr; (void)DstAddr; (void)Length;
}

void Dma_EnableChannel(uint8 Channel)
{
    (void)Channel;
}

void Dma_DisableChannel(uint8 Channel)
{
    (void)Channel;
}

void setUp(void) { mock_hal_reset(); }
void tearDown(void) {}

/* 最小配置: ChannelCount=0（避免 Spi_Init 访问硬件寄存器） */
static void create_min_cfg(Spi_ConfigType* cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->ChannelCount = 0U;
    cfg->DeviceCount = 0U;
}

/* ========= Spi_Init ========= */
/* @req SWS_Spi_00001 */
void test_Spi_Init_NullConfig(void)
{
    Spi_Init(NULL);
    TEST_ASSERT_EQUAL(SPI_UNINIT, Spi_GetStatus());
}

/* @req SWS_Spi_00001 */
void test_Spi_Init_Valid(void)
{
    Spi_ConfigType cfg;
    create_min_cfg(&cfg);
    Spi_Init(&cfg);
    TEST_ASSERT_EQUAL(SPI_IDLE, Spi_GetStatus());
    TEST_ASSERT_EQUAL(SPI_JOB_OK, Spi_GetJobResult());
}

/* ========= Spi_DeInit =========
 * 注意: Spi_DeInit 用编译期 SPI_CHANNEL_COUNT(4) 固定遍历 Spi_BaseAddr[]
 * (Spi_Lcfg.c 中的 const 数组, 指向 ECSPI 硬件地址 0x30820000 等), 不走
 * REG_READ32/REG_WRITE32 宏, mock_hal 无法拦截 — host 上必然段错误。
 * 因此 DeInit 的寄存器访问路径无法在 host 单测覆盖 (Spi_Init 用运行时
 * ChannelCount=0 可避开, DeInit 无此机制)。守卫逻辑已由生产 Spi.c 的
 * 无条件 Spi_Initialized 检查保障。 */

/* ========= Spi_SyncTransmit ========= */
/* @req SWS_Spi_00003 */
void test_Spi_SyncTransmit_BeforeInit(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Spi_SyncTransmit(0U, NULL, NULL, 0U));
}

/* @req SWS_Spi_00003 */
void test_Spi_SyncTransmit_InvalidDevice(void)
{
    Spi_ConfigType cfg;
    create_min_cfg(&cfg);
    Spi_Init(&cfg);
    /* DeviceCount == 0 → DeviceId 0 越界 → E_NOT_OK，不触发寄存器访问 */
    TEST_ASSERT_EQUAL(E_NOT_OK, Spi_SyncTransmit(0U, NULL, NULL, 4U));
}

/* ========= Spi_AsyncTransmit ========= */
/* @req SWS_Spi_00004 */
void test_Spi_AsyncTransmit_BeforeInit(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Spi_AsyncTransmit(0U, NULL, NULL, 0U));
}

/* ========= Spi_GetStatus / Spi_GetJobResult ========= */
/* @req SWS_Spi_00006 */
void test_Spi_GetJobResult_BeforeInit(void)
{
    TEST_ASSERT_EQUAL(SPI_JOB_FAILED, Spi_GetJobResult());
}

/* @req SWS_Spi_00005 */
void test_Spi_GetStatus_Uninit(void)
{
    TEST_ASSERT_EQUAL(SPI_UNINIT, Spi_GetStatus());
}

/* ========= Spi_MainFunction / Spi_IsrHandler ========= */
/* @req SWS_Spi_00008 */
void test_Spi_MainFunction_Uninit(void)
{
    Spi_MainFunction(); /* 未初始化 → 直接返回，不崩溃 */
}

/* @req SWS_Spi_00008 */
void test_Spi_MainFunction_Idle(void)
{
    Spi_ConfigType cfg;
    create_min_cfg(&cfg);
    Spi_Init(&cfg);
    Spi_MainFunction(); /* IDLE 状态 → 直接返回 */
    TEST_ASSERT_EQUAL(SPI_IDLE, Spi_GetStatus());
}

/* @req SWS_Spi_00007 */
void test_Spi_IsrHandler_Uninit(void)
{
    Spi_IsrHandler(0U); /* 未初始化 → 直接返回，不崩溃 */
    Spi_IsrHandler(SPI_CHANNEL_COUNT); /* 越界 channel → 直接返回 */
}

/* ========= Spi_GetVersionInfo ========= */
/* @req SWS_Spi_00009 */
void test_Spi_GetVersionInfo_Valid(void)
{
    Std_VersionInfoType vi;
    memset(&vi, 0, sizeof(vi));
    Spi_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL(SPI_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL(SPI_MODULE_ID, vi.moduleID);
}

/* @req SWS_Spi_00009 */
void test_Spi_GetVersionInfo_Null(void)
{
    Spi_GetVersionInfo(NULL); /* DET 报告，不崩溃 */
}

/* ========= Main =========
 * 顺序约束: Spi_Initialized 是 Spi.c 的 static 状态, 用例间无法重置。
 * 所有未初始化 (BeforeInit/Uninit) 用例必须排在首个 Spi_Init 之前;
 * Spi_Init 后 Spi_ConfigPtr 指向用例局部 cfg, 仅限用例内有效,
 * 因此已初始化用例 (SyncTx invalid / MainFunction idle) 紧随 Init 之后。 */
int main(void)
{
    UnityBegin();
    /* --- 未初始化状态用例 (必须最先执行) --- */
    UnityRunTest(test_Spi_Init_NullConfig, "Init NULL", __LINE__);
    UnityRunTest(test_Spi_SyncTransmit_BeforeInit, "SyncTx before init", __LINE__);
    UnityRunTest(test_Spi_AsyncTransmit_BeforeInit, "AsyncTx before init", __LINE__);
    UnityRunTest(test_Spi_GetJobResult_BeforeInit, "GetJobResult before init", __LINE__);
    UnityRunTest(test_Spi_GetStatus_Uninit, "GetStatus uninit", __LINE__);
    UnityRunTest(test_Spi_MainFunction_Uninit, "MainFunction uninit", __LINE__);
    UnityRunTest(test_Spi_IsrHandler_Uninit, "IsrHandler uninit", __LINE__);
    /* --- 已初始化状态用例 (Spi_Init 之后) --- */
    UnityRunTest(test_Spi_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Spi_SyncTransmit_InvalidDevice, "SyncTx invalid device", __LINE__);
    UnityRunTest(test_Spi_MainFunction_Idle, "MainFunction idle", __LINE__);
    /* --- 无状态依赖 --- */
    UnityRunTest(test_Spi_GetVersionInfo_Valid, "Version valid", __LINE__);
    UnityRunTest(test_Spi_GetVersionInfo_Null, "Version null", __LINE__);
    return UnityEnd();
}
