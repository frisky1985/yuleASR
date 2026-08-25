/**
 * @file test_mcal_dio.c
 * @brief Dio unit test — links real Dio.c + Dio_Lcfg.c production code
 * Uses mock_registers.h to redirect REG_READ32/REG_WRITE32 to mock storage.
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "mock_hal.h"
#include "mock_hal_config.h"
#include "Dio.h"

/* DIO register addresses — 与生产 Dio.c 在 -DS32K312 下的布局一致
 * (S32K312_SIUL2_GPIO_BASE = 0x40810000, stride 0x1000,
 *  PSR=PDIR 偏移 0x10, DR=PDOR 偏移 0x00) */
static const uint32 DIO_GPIO1_BASE = 0x40810000UL;
static const uint32 DIO_GPIO2_BASE = 0x40820000UL;
static const uint32 DIO_GPIO3_BASE = 0x40830000UL;
static const uint32 DIO_GPIO4_BASE = 0x40840000UL;
static const uint32 DIO_GPIO5_BASE = 0x40850000UL;
static const uint32 DIO_GPIO_PSR_OFF = 0x10U;  /* PDIR (Port Data Input) */
static const uint32 DIO_GPIO_DR_OFF  = 0x00U;  /* PDOR (Port Data Output) */

/* Forward declarations for production code functions */
extern void Dio_Init(const Dio_ConfigType* ConfigPtr);
extern Dio_LevelType Dio_ReadChannel(Dio_ChannelType ChannelId);
extern void Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level);
extern Dio_PortLevelType Dio_ReadPort(Dio_PortType PortId);
extern void Dio_WritePort(Dio_PortType PortId, Dio_PortLevelType Level);
extern Dio_PortLevelType Dio_ReadChannelGroup(const Dio_ChannelGroupType* ChannelGroupIdPtr);
extern void Dio_WriteChannelGroup(const Dio_ChannelGroupType* ChannelGroupIdPtr, Dio_PortLevelType Level);
extern void Dio_GetVersionInfo(Std_VersionInfoType* versioninfo);
extern Dio_LevelType Dio_FlipChannel(Dio_ChannelType ChannelId);
extern void Dio_MaskedWritePort(Dio_PortType PortId, Dio_PortLevelType Level, Dio_PortLevelType Mask);

void setUp(void) {
    mock_hal_reset();
}

void tearDown(void) {
}

/* ========= Dio_Init ========= */
/* @req SWS_Dio_00001 */
void test_Dio_Init_NullConfig(void) {
    Dio_Init(NULL); /* Should not crash; DET reports, returns */
    /* After error, module not initialized — but doesn't crash */
}

/* @req SWS_Dio_00001 */
void test_Dio_Init_Valid(void) {
    Dio_ConfigType cfg; /* dummy */
    memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
}

/* ========= Dio_ReadChannel ========= */
/* @req SWS_Dio_00002 */
void test_Dio_ReadChannel_BeforeInit_ReturnsLow(void) {
    Dio_LevelType l = Dio_ReadChannel(0);
    TEST_ASSERT_EQUAL(STD_LOW, l);
}

/* @req SWS_Dio_00002 */
void test_Dio_ReadChannel_ValidHigh(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    /* Set PSR for port 0, pin 0 */
    mock_hal_write32(DIO_GPIO1_BASE + DIO_GPIO_PSR_OFF, 0x01);
    TEST_ASSERT_EQUAL(STD_HIGH, Dio_ReadChannel(0));
}

/* @req SWS_Dio_00002 */
void test_Dio_ReadChannel_ValidLow(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    mock_hal_write32(DIO_GPIO1_BASE + DIO_GPIO_PSR_OFF, 0x00);
    TEST_ASSERT_EQUAL(STD_LOW, Dio_ReadChannel(0));
}

/* @req SWS_Dio_00002 */
void test_Dio_ReadChannel_InvalidChannel(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    TEST_ASSERT_EQUAL(STD_LOW, Dio_ReadChannel(256)); /* DIO_NUM_PORTS*DIO_NUM_CHANNELS_PER_PORT = 256 */
}

/* @req SWS_Dio_00002 */
void test_Dio_ReadChannel_InvalidChannel_255(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    TEST_ASSERT_EQUAL(STD_LOW, Dio_ReadChannel(255)); /* Valid (0-255) */
    Dio_ReadChannel(256); /* Invalid */
}

/* ========= Dio_WriteChannel ========= */
/* @req SWS_Dio_00003 */
void test_Dio_WriteChannel_ValidHigh(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    Dio_WriteChannel(0, STD_HIGH);
    uint32 v = mock_hal_read32(DIO_GPIO1_BASE + DIO_GPIO_DR_OFF);
    TEST_ASSERT_TRUE(v & 0x01);
}

/* @req SWS_Dio_00003 */
void test_Dio_WriteChannel_ValidLow(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    mock_hal_write32(DIO_GPIO1_BASE + DIO_GPIO_DR_OFF, 0x01); /* Start high */
    Dio_WriteChannel(0, STD_LOW);
    uint32 v = mock_hal_read32(DIO_GPIO1_BASE + DIO_GPIO_DR_OFF);
    TEST_ASSERT_FALSE(v & 0x01);
}

/* @req SWS_Dio_00003 */
void test_Dio_WriteChannel_BeforeInit(void) {
    Dio_WriteChannel(0, STD_HIGH); /* Should not crash, returns early */
}

/* @req SWS_Dio_00003 */
void test_Dio_WriteChannel_Invalid(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    Dio_WriteChannel(256, STD_HIGH); /* Should report error, not crash */
}

/* ========= Dio_ReadPort ========= */
/* @req SWS_Dio_00004 */
void test_Dio_ReadPort_Valid(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    mock_hal_write32(DIO_GPIO1_BASE + DIO_GPIO_PSR_OFF, 0xDEAD);
    TEST_ASSERT_EQUAL(0xDEAD, Dio_ReadPort(0));
}

/* @req SWS_Dio_00004 */
void test_Dio_ReadPort_Invalid(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    TEST_ASSERT_EQUAL(0, Dio_ReadPort(8)); /* DIO_NUM_PORTS=8, so 8 is invalid */
}

/* @req SWS_Dio_00004 */
void test_Dio_ReadPort_BeforeInit(void) {
    TEST_ASSERT_EQUAL(0, Dio_ReadPort(0));
}

/* ========= Dio_WritePort ========= */
/* @req SWS_Dio_00005 */
void test_Dio_WritePort_Valid(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    Dio_WritePort(0, 0x1234);
    TEST_ASSERT_EQUAL(0x1234, mock_hal_read32(DIO_GPIO1_BASE + DIO_GPIO_DR_OFF));
}

/* @req SWS_Dio_00005 */
void test_Dio_WritePort_Invalid(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    Dio_WritePort(8, 0x1234); /* Should report error */
}

/* ========= Dio_ReadChannelGroup ========= */
/* @req SWS_Dio_00002 */
void test_Dio_ReadChannelGroup_Valid(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    Dio_ChannelGroupType g = {0, 0, 0x0F};
    mock_hal_write32(DIO_GPIO1_BASE + DIO_GPIO_PSR_OFF, 0xAB);
    TEST_ASSERT_EQUAL(0x0B, Dio_ReadChannelGroup(&g));
}

/* @req SWS_Dio_00002 */
void test_Dio_ReadChannelGroup_Null(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    TEST_ASSERT_EQUAL(0, Dio_ReadChannelGroup(NULL));
}

/* @req SWS_Dio_00002 */
void test_Dio_ReadChannelGroup_BeforeInit(void) {
    Dio_ChannelGroupType g = {0, 0, 0x0F};
    TEST_ASSERT_EQUAL(0, Dio_ReadChannelGroup(&g));
}

/* ========= Dio_WriteChannelGroup ========= */
/* @req SWS_Dio_00003 */
void test_Dio_WriteChannelGroup_Valid(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    Dio_ChannelGroupType g = {0, 0, 0x0F};
    mock_hal_write32(DIO_GPIO1_BASE + DIO_GPIO_DR_OFF, 0xF0);
    Dio_WriteChannelGroup(&g, 0x05);
    TEST_ASSERT_EQUAL(0xF5, mock_hal_read32(DIO_GPIO1_BASE + DIO_GPIO_DR_OFF));
}

/* @req SWS_Dio_00003 */
void test_Dio_WriteChannelGroup_Null(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    Dio_WriteChannelGroup(NULL, 0x05); /* Should not crash */
}

/* @req SWS_Dio_00003 */
void test_Dio_WriteChannelGroup_BeforeInit(void) {
    Dio_ChannelGroupType g = {0, 0, 0x0F};
    Dio_WriteChannelGroup(&g, 0x05); /* Should not crash */
}

/* ========= Dio_GetVersionInfo ========= */
/* @req SWS_Dio_00008 */
void test_Dio_GetVersionInfo_Valid(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    Std_VersionInfoType vi; memset(&vi, 0, sizeof(vi));
    Dio_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL(DIO_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL(DIO_MODULE_ID, vi.moduleID);
}

/* @req SWS_Dio_00008 */
void test_Dio_GetVersionInfo_Null(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    Dio_GetVersionInfo(NULL); /* Should not crash */
}

/* ========= Dio_FlipChannel ========= */
/* @req SWS_Dio_00009 */
void test_Dio_FlipChannel_Valid_HighToLow(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    mock_hal_write32(DIO_GPIO1_BASE + DIO_GPIO_DR_OFF, 0x01);
    TEST_ASSERT_EQUAL(STD_LOW, Dio_FlipChannel(0));
}

/* @req SWS_Dio_00009 */
void test_Dio_FlipChannel_Valid_LowToHigh(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    mock_hal_write32(DIO_GPIO1_BASE + DIO_GPIO_DR_OFF, 0x00);
    TEST_ASSERT_EQUAL(STD_HIGH, Dio_FlipChannel(0));
}

/* @req SWS_Dio_00009 */
void test_Dio_FlipChannel_Invalid(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    TEST_ASSERT_EQUAL(STD_LOW, Dio_FlipChannel(256));
}

/* @req SWS_Dio_00009 */
void test_Dio_FlipChannel_BeforeInit(void) {
    TEST_ASSERT_EQUAL(STD_LOW, Dio_FlipChannel(0));
}

/* ========= Dio_MaskedWritePort ========= */
/* @req SWS_Dio_00010 */
void test_Dio_MaskedWritePort_Valid(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    mock_hal_write32(DIO_GPIO1_BASE + DIO_GPIO_DR_OFF, 0x00FF);
    Dio_MaskedWritePort(0, 0x0F00, 0xF000);
    uint32 v = mock_hal_read32(DIO_GPIO1_BASE + DIO_GPIO_DR_OFF);
    TEST_ASSERT_EQUAL(0x00FF, v); /* mask 0xF000 doesn't cover lower bits, level 0x0F00 masked to 0x0000 with mask 0xF000 */
    /* Actually: dr = 0x00FF, mask=0xF000 -> dr &= ~0xF000 = 0x00FF, dr |= (0x0F00 & 0xF000) = 0x00FF | 0x0000 = 0x00FF */
}

/* @req SWS_Dio_00010 */
void test_Dio_MaskedWritePort_Invalid(void) {
    Dio_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Dio_Init(&cfg);
    Dio_MaskedWritePort(8, 0x0F00, 0xF000); /* Should report error */
}

/* @req SWS_Dio_00010 */
void test_Dio_MaskedWritePort_BeforeInit(void) {
    Dio_MaskedWritePort(0, 0x0F00, 0xF000); /* Should not crash */
}

/* ========= Main ========= */
int main(void) {
    UnityBegin();

    /* 注: Dio_DriverInitialized 是生产模块静态变量，跨测试保留、无法在 host 重置。
     * "未初始化" 分支的用例必须排在所有 Dio_Init 之前执行（进程冷启动状态）。 */
    UnityRunTest(test_Dio_FlipChannel_BeforeInit, "Flip before init", __LINE__);
    UnityRunTest(test_Dio_ReadChannel_BeforeInit_ReturnsLow, "Read before init", __LINE__);
    UnityRunTest(test_Dio_ReadPort_BeforeInit, "Read port before init", __LINE__);
    UnityRunTest(test_Dio_WriteChannel_BeforeInit, "Write before init", __LINE__);
    UnityRunTest(test_Dio_ReadChannelGroup_BeforeInit, "Read group before init", __LINE__);
    UnityRunTest(test_Dio_WriteChannelGroup_BeforeInit, "Write group before init", __LINE__);
    UnityRunTest(test_Dio_MaskedWritePort_BeforeInit, "Masked write before init", __LINE__);

    UnityRunTest(test_Dio_Init_NullConfig, "Init with NULL", __LINE__);
    UnityRunTest(test_Dio_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Dio_ReadChannel_ValidHigh, "Read high", __LINE__);
    UnityRunTest(test_Dio_ReadChannel_ValidLow, "Read low", __LINE__);
    UnityRunTest(test_Dio_ReadChannel_InvalidChannel, "Read invalid", __LINE__);
    UnityRunTest(test_Dio_WriteChannel_ValidHigh, "Write high", __LINE__);
    UnityRunTest(test_Dio_WriteChannel_ValidLow, "Write low", __LINE__);
    UnityRunTest(test_Dio_WriteChannel_Invalid, "Write invalid", __LINE__);
    UnityRunTest(test_Dio_ReadPort_Valid, "Read port valid", __LINE__);
    UnityRunTest(test_Dio_ReadPort_Invalid, "Read port invalid", __LINE__);
    UnityRunTest(test_Dio_WritePort_Valid, "Write port valid", __LINE__);
    UnityRunTest(test_Dio_WritePort_Invalid, "Write port invalid", __LINE__);
    UnityRunTest(test_Dio_ReadChannelGroup_Valid, "Read group valid", __LINE__);
    UnityRunTest(test_Dio_ReadChannelGroup_Null, "Read group null", __LINE__);
    UnityRunTest(test_Dio_WriteChannelGroup_Valid, "Write group valid", __LINE__);
    UnityRunTest(test_Dio_WriteChannelGroup_Null, "Write group null", __LINE__);
    UnityRunTest(test_Dio_GetVersionInfo_Valid, "Version info valid", __LINE__);
    UnityRunTest(test_Dio_GetVersionInfo_Null, "Version info null", __LINE__);
    UnityRunTest(test_Dio_FlipChannel_Valid_HighToLow, "Flip high->low", __LINE__);
    UnityRunTest(test_Dio_FlipChannel_Valid_LowToHigh, "Flip low->high", __LINE__);
    UnityRunTest(test_Dio_FlipChannel_Invalid, "Flip invalid", __LINE__);
    UnityRunTest(test_Dio_MaskedWritePort_Valid, "Masked write valid", __LINE__);
    UnityRunTest(test_Dio_MaskedWritePort_Invalid, "Masked write invalid", __LINE__);

    return UnityEnd();
}
