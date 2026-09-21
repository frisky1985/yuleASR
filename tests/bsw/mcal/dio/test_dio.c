/**
 * @file test_dio.c
 * @brief Dio (Digital I/O) Unit Tests — Substantiated
 * @req SWS_Dio
 *
 * Substantiation: each test verifies concrete behavior via mock registers
 * and DET parameter validation, replacing prior TEST_ASSERT_TRUE(1) stubs.
 */

#include "unity.h"
#include "mock_registers.h"
#include "mock_det.h"

#include "Dio.h"
#include "Dio_Cfg.h"

/* GPIO base addresses (must match Dio.c non-S32K312 build) */
#define DIO_GPIO1_BASE  (0x30200000UL)
#define DIO_GPIO2_BASE  (0x30210000UL)
#define DIO_GPIO_DR     (0x00U)
#define DIO_GPIO_PSR    (0x08U)

static Dio_ConfigType testConfig;

void setUp(void) {
    MockRegisters_Reset();
    Det_Mock_Reset();
}

void tearDown(void) {}

/* --- Init --- */

/** @req SWS_Dio_00001 */
void test_Dio_Init_NullPtr_ShouldReportDet(void) {
    Dio_Init(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(DIO_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(DIO_SID_INIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(DIO_E_PARAM_CONFIG, Det_MockData.ErrorId);
}

/** @req SWS_Dio_00001 */
void test_Dio_Init_ValidConfig_ShouldEnableSubsequentApi(void) {
    Dio_Init(&testConfig);
    /* After init, ReadChannel on valid channel should NOT report E_UNINIT */
    MockRegisters_Write32(DIO_GPIO1_BASE + DIO_GPIO_PSR, 0x00U);
    (void)Dio_ReadChannel(0x0000U);
    TEST_ASSERT_EQUAL(0U, Det_MockData.CallCount);
}

/** @req SWS_Dio_00001 */
void test_Dio_Init_DoubleInit_ShouldKeepDriverOperational(void) {
    Dio_Init(&testConfig);
    /* Dio_Init has no double-init DET check: a repeated call is accepted
     * silently and must leave the driver operational. */
    Dio_Init(&testConfig);
    MockRegisters_Write32(DIO_GPIO1_BASE + DIO_GPIO_PSR, 0x00000008U);
    TEST_ASSERT_EQUAL(STD_HIGH, Dio_ReadChannel(0x0003U));
    TEST_ASSERT_EQUAL(0U, Det_MockData.CallCount);
}

/* --- ReadChannel --- */

/** @req SWS_Dio_00002 */
void test_Dio_ReadChannel_BeforeInit_ShouldReportDet(void) {
    (void)Dio_ReadChannel(0x0000U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(DIO_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(DIO_SID_READ_CHANNEL, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(DIO_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Dio_00002 */
void test_Dio_ReadChannel_PinHigh_ShouldReturnStdHigh(void) {
    Dio_Init(&testConfig);
    /* Preset PSR: port 0, pin 2 = HIGH */
    MockRegisters_Write32(DIO_GPIO1_BASE + DIO_GPIO_PSR, 0x00000004U);
    Dio_LevelType level = Dio_ReadChannel(0x0002U);
    TEST_ASSERT_EQUAL(STD_HIGH, level);
}

/** @req SWS_Dio_00002 */
void test_Dio_ReadChannel_PinLow_ShouldReturnStdLow(void) {
    Dio_Init(&testConfig);
    /* Preset PSR: port 0, pin 2 = LOW */
    MockRegisters_Write32(DIO_GPIO1_BASE + DIO_GPIO_PSR, 0x00000000U);
    Dio_LevelType level = Dio_ReadChannel(0x0002U);
    TEST_ASSERT_EQUAL(STD_LOW, level);
}

/* --- WriteChannel --- */

/** @req SWS_Dio_00003 */
void test_Dio_WriteChannel_BeforeInit_ShouldReportDet(void) {
    Dio_WriteChannel(0x0000U, STD_HIGH);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(DIO_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Dio_00003 */
void test_Dio_WriteChannel_High_ShouldSetBit(void) {
    Dio_Init(&testConfig);
    /* Preset DR to 0 */
    MockRegisters_Write32(DIO_GPIO1_BASE + DIO_GPIO_DR, 0x00000000U);
    Dio_WriteChannel(0x0003U, STD_HIGH);
    uint32 dr = MockRegisters_Read32(DIO_GPIO1_BASE + DIO_GPIO_DR);
    TEST_ASSERT_NOT_EQUAL(0U, dr & (1U << 3));
}

/** @req SWS_Dio_00003 */
void test_Dio_WriteChannel_Low_ShouldClearBit(void) {
    Dio_Init(&testConfig);
    /* Preset DR with bit 5 set */
    MockRegisters_Write32(DIO_GPIO1_BASE + DIO_GPIO_DR, 0x00000020U);
    Dio_WriteChannel(0x0005U, STD_LOW);
    uint32 dr = MockRegisters_Read32(DIO_GPIO1_BASE + DIO_GPIO_DR);
    TEST_ASSERT_EQUAL(0U, dr & (1U << 5));
}

/* --- ReadPort --- */

/** @req SWS_Dio_00004 */
void test_Dio_ReadPort_AfterInit_ShouldReturnPsrValue(void) {
    Dio_Init(&testConfig);
    MockRegisters_Write32(DIO_GPIO1_BASE + DIO_GPIO_PSR, 0xABCD1234U);
    Dio_PortLevelType level = Dio_ReadPort(0U);
    TEST_ASSERT_EQUAL_HEX32(0xABCD1234U, level);
}

/* --- WritePort --- */

/** @req SWS_Dio_00005 */
void test_Dio_WritePort_AfterInit_ShouldWriteDr(void) {
    Dio_Init(&testConfig);
    Dio_WritePort(0U, 0xDEADBEEFU);
    uint32 dr = MockRegisters_Read32(DIO_GPIO1_BASE + DIO_GPIO_DR);
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEFU, dr);
}

/* --- GetVersionInfo --- */

/** @req SWS_Dio_00008 */
void test_Dio_GetVersionInfo_ValidPtr_ShouldReturnCorrectVersion(void) {
    Std_VersionInfoType info;
    Dio_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(DIO_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL(DIO_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL(DIO_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL(DIO_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL(DIO_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_Dio_00008 */
void test_Dio_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    Dio_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(DIO_SID_GET_VERSION_INFO, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(DIO_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/* --- FlipChannel --- */

/** @req SWS_Dio_00009 */
void test_Dio_FlipChannel_LowToHigh_ShouldSetBitAndReturnHigh(void) {
    Dio_Init(&testConfig);
    MockRegisters_Write32(DIO_GPIO1_BASE + DIO_GPIO_DR, 0x00000000U);
    Dio_LevelType result = Dio_FlipChannel(0x0004U);
    TEST_ASSERT_EQUAL(STD_HIGH, result);
    uint32 dr = MockRegisters_Read32(DIO_GPIO1_BASE + DIO_GPIO_DR);
    TEST_ASSERT_NOT_EQUAL(0U, dr & (1U << 4));
}

/** @req SWS_Dio_00009 */
void test_Dio_FlipChannel_HighToLow_ShouldClearBitAndReturnLow(void) {
    Dio_Init(&testConfig);
    MockRegisters_Write32(DIO_GPIO1_BASE + DIO_GPIO_DR, 0x00000010U);
    Dio_LevelType result = Dio_FlipChannel(0x0004U);
    TEST_ASSERT_EQUAL(STD_LOW, result);
    uint32 dr = MockRegisters_Read32(DIO_GPIO1_BASE + DIO_GPIO_DR);
    TEST_ASSERT_EQUAL(0U, dr & (1U << 4));
}
