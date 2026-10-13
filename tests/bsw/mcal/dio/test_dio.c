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
#define DIO_GPIO3_BASE  (0x30220000UL)
#define DIO_GPIO4_BASE  (0x30230000UL)
#define DIO_GPIO5_BASE  (0x30240000UL)
#define DIO_GPIO_DR     (0x00U)
#define DIO_GPIO_PSR_SET (0x04U)  /* PSOR: atomic bit set */
#define DIO_GPIO_PCR_CLR (0x08U)  /* PCOR: atomic bit clear */
#define DIO_GPIO_PTOR_TOG (0x0CU) /* PTOR: atomic bit toggle */
#define DIO_GPIO_PSR    (0x08U)   /* PDIR: data input */

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

/* --- Dio_GetGpioBaseAddr switch coverage (line 94) --- */

/**
 * Dio_ReadPort uses Dio_GetGpioBaseAddr with PortId directly.
 * PortId 0..4 cover DIO_PORT_A..DIO_PORT_E; PortId 5 covers default.
 */
void test_Dio_ReadPort_PortB_ShouldReadGpio2Base(void) {
    Dio_Init(&testConfig);
    MockRegisters_Write32(DIO_GPIO2_BASE + DIO_GPIO_PSR, 0x12345678U);
    Dio_PortLevelType level = Dio_ReadPort(1U); /* DIO_PORT_B */
    TEST_ASSERT_EQUAL_HEX32(0x12345678U, level);
}

void test_Dio_ReadPort_PortC_ShouldReadGpio3Base(void) {
    Dio_Init(&testConfig);
    MockRegisters_Write32(DIO_GPIO3_BASE + DIO_GPIO_PSR, 0xAABBCCDDU);
    Dio_PortLevelType level = Dio_ReadPort(2U); /* DIO_PORT_C */
    TEST_ASSERT_EQUAL_HEX32(0xAABBCCDDU, level);
}

void test_Dio_ReadPort_PortD_ShouldReadGpio4Base(void) {
    Dio_Init(&testConfig);
    MockRegisters_Write32(DIO_GPIO4_BASE + DIO_GPIO_PSR, 0x11111111U);
    Dio_PortLevelType level = Dio_ReadPort(3U); /* DIO_PORT_D */
    TEST_ASSERT_EQUAL_HEX32(0x11111111U, level);
}

void test_Dio_ReadPort_PortE_ShouldReadGpio5Base(void) {
    Dio_Init(&testConfig);
    MockRegisters_Write32(DIO_GPIO5_BASE + DIO_GPIO_PSR, 0x22222222U);
    Dio_PortLevelType level = Dio_ReadPort(4U); /* DIO_PORT_E */
    TEST_ASSERT_EQUAL_HEX32(0x22222222U, level);
}

/**
 * PortId 5 is < DIO_NUM_PORTS (8) so it passes validation, but hits the
 * default branch of Dio_GetGpioBaseAddr (baseAddr = 0).
 */
void test_Dio_ReadPort_InvalidPort5_ShouldHitDefaultBranch(void) {
    Dio_Init(&testConfig);
    /* default branch returns baseAddr=0; reads from 0+PSR offset */
    MockRegisters_Write32(0U + DIO_GPIO_PSR, 0xDEAD0000U);
    Dio_PortLevelType level = Dio_ReadPort(5U);
    TEST_ASSERT_EQUAL_HEX32(0xDEAD0000U, level);
}

void test_Dio_WritePort_PortB_ShouldWriteGpio2Base(void) {
    Dio_Init(&testConfig);
    Dio_WritePort(1U, 0xCAFEBABEU); /* DIO_PORT_B */
    TEST_ASSERT_EQUAL_HEX32(0xCAFEBABEU, MockRegisters_Read32(DIO_GPIO2_BASE + DIO_GPIO_DR));
}

void test_Dio_WritePort_PortE_ShouldWriteGpio5Base(void) {
    Dio_Init(&testConfig);
    Dio_WritePort(4U, 0x12345678U); /* DIO_PORT_E */
    TEST_ASSERT_EQUAL_HEX32(0x12345678U, MockRegisters_Read32(DIO_GPIO5_BASE + DIO_GPIO_DR));
}

/* --- ReadChannel invalid channel DET --- */

void test_Dio_ReadChannel_InvalidChannel_ShouldReportDet(void) {
    Dio_Init(&testConfig);
    /* Channel >= DIO_NUM_PORTS * DIO_NUM_CHANNELS_PER_PORT = 8*32 = 256 */
    (void)Dio_ReadChannel(256U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(DIO_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(DIO_SID_READ_CHANNEL, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(DIO_E_PARAM_INVALID_CHANNEL_ID, Det_MockData.ErrorId);
}

/* --- WriteChannel invalid channel DET --- */

void test_Dio_WriteChannel_InvalidChannel_ShouldReportDet(void) {
    Dio_Init(&testConfig);
    Dio_WriteChannel(256U, STD_HIGH);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(DIO_SID_WRITE_CHANNEL, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(DIO_E_PARAM_INVALID_CHANNEL_ID, Det_MockData.ErrorId);
}

/* --- FlipChannel uninit DET --- */

void test_Dio_FlipChannel_BeforeInit_ShouldReportDet(void) {
    (void)Dio_FlipChannel(0x0000U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(DIO_SID_FLIP_CHANNEL, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(DIO_E_UNINIT, Det_MockData.ErrorId);
}

/* --- FlipChannel invalid channel DET --- */

void test_Dio_FlipChannel_InvalidChannel_ShouldReportDet(void) {
    Dio_Init(&testConfig);
    (void)Dio_FlipChannel(256U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(DIO_SID_FLIP_CHANNEL, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(DIO_E_PARAM_INVALID_CHANNEL_ID, Det_MockData.ErrorId);
}

/* --- ReadPort uninit DET --- */

void test_Dio_ReadPort_BeforeInit_ShouldReportDet(void) {
    (void)Dio_ReadPort(0U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(DIO_SID_READ_PORT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(DIO_E_UNINIT, Det_MockData.ErrorId);
}

/* --- ReadPort invalid port DET --- */

void test_Dio_ReadPort_InvalidPort8_ShouldReportDet(void) {
    Dio_Init(&testConfig);
    (void)Dio_ReadPort(8U); /* >= DIO_NUM_PORTS */
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(DIO_SID_READ_PORT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(DIO_E_PARAM_INVALID_PORT_ID, Det_MockData.ErrorId);
}

/* --- WritePort uninit DET --- */

void test_Dio_WritePort_BeforeInit_ShouldReportDet(void) {
    Dio_WritePort(0U, 0xFFU);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(DIO_SID_WRITE_PORT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(DIO_E_UNINIT, Det_MockData.ErrorId);
}

/* --- WritePort invalid port DET --- */

void test_Dio_WritePort_InvalidPort8_ShouldReportDet(void) {
    Dio_Init(&testConfig);
    Dio_WritePort(8U, 0xFFU);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(DIO_SID_WRITE_PORT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(DIO_E_PARAM_INVALID_PORT_ID, Det_MockData.ErrorId);
}

/* --- ReadChannelGroup --- */

void test_Dio_ReadChannelGroup_BeforeInit_ShouldReportDet(void) {
    Dio_ChannelGroupType grp = { .port = 0U, .offset = 2U, .mask = 0x0000000CU };
    (void)Dio_ReadChannelGroup(&grp);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(DIO_SID_READ_CHANNEL_GROUP, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(DIO_E_UNINIT, Det_MockData.ErrorId);
}

void test_Dio_ReadChannelGroup_NullPtr_ShouldReportDet(void) {
    Dio_Init(&testConfig);
    (void)Dio_ReadChannelGroup(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(DIO_SID_READ_CHANNEL_GROUP, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(DIO_E_PARAM_POINTER, Det_MockData.ErrorId);
}

void test_Dio_ReadChannelGroup_Valid_ShouldReturnMaskedShifted(void) {
    Dio_Init(&testConfig);
    /* Preset PSR for port A: bits [4:2] = 0b101 => (0x14 & 0x1C) >> 2 = 5 */
    MockRegisters_Write32(DIO_GPIO1_BASE + DIO_GPIO_PSR, 0x00000014U);
    Dio_ChannelGroupType grp = { .port = 0U, .offset = 2U, .mask = 0x0000001CU };
    Dio_PortLevelType level = Dio_ReadChannelGroup(&grp);
    TEST_ASSERT_EQUAL_HEX32(5U, level);
}

/* --- WriteChannelGroup --- */

void test_Dio_WriteChannelGroup_BeforeInit_ShouldReportDet(void) {
    Dio_ChannelGroupType grp = { .port = 0U, .offset = 0U, .mask = 0x0FU };
    Dio_WriteChannelGroup(&grp, 0x05U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(DIO_SID_WRITE_CHANNEL_GROUP, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(DIO_E_UNINIT, Det_MockData.ErrorId);
}

void test_Dio_WriteChannelGroup_NullPtr_ShouldReportDet(void) {
    Dio_Init(&testConfig);
    Dio_WriteChannelGroup(NULL_PTR, 0x05U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(DIO_SID_WRITE_CHANNEL_GROUP, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(DIO_E_PARAM_POINTER, Det_MockData.ErrorId);
}

void test_Dio_WriteChannelGroup_Valid_ShouldDoRmw(void) {
    Dio_Init(&testConfig);
    /* Preset DR: bits[7:0] = 0xAA */
    MockRegisters_Write32(DIO_GPIO1_BASE + DIO_GPIO_DR, 0x000000AAU);
    /* Write 0x5 to bits [3:0] (mask=0x0F, offset=0): expect 0xA5 */
    Dio_ChannelGroupType grp = { .port = 0U, .offset = 0U, .mask = 0x0000000FU };
    Dio_WriteChannelGroup(&grp, 0x05U);
    uint32 dr = MockRegisters_Read32(DIO_GPIO1_BASE + DIO_GPIO_DR);
    TEST_ASSERT_EQUAL_HEX32(0x000000A5U, dr);
}

void test_Dio_WriteChannelGroup_WithOffset_ShouldDoRmw(void) {
    Dio_Init(&testConfig);
    /* Preset DR = 0xFF */
    MockRegisters_Write32(DIO_GPIO1_BASE + DIO_GPIO_DR, 0x000000FFU);
    /* Write 0x0 to bits [7:4] (mask=0xF0, offset=4): expect 0x0F */
    Dio_ChannelGroupType grp = { .port = 0U, .offset = 4U, .mask = 0x000000F0U };
    Dio_WriteChannelGroup(&grp, 0x00U);
    uint32 dr = MockRegisters_Read32(DIO_GPIO1_BASE + DIO_GPIO_DR);
    TEST_ASSERT_EQUAL_HEX32(0x0000000FU, dr);
}

/* --- MaskedWritePort --- */

void test_Dio_MaskedWritePort_BeforeInit_ShouldReportDet(void) {
    Dio_MaskedWritePort(0U, 0xFFU, 0x0FU);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(DIO_SID_MASKED_WRITE_PORT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(DIO_E_UNINIT, Det_MockData.ErrorId);
}

void test_Dio_MaskedWritePort_InvalidPort_ShouldReportDet(void) {
    Dio_Init(&testConfig);
    Dio_MaskedWritePort(8U, 0xFFU, 0x0FU);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(DIO_SID_MASKED_WRITE_PORT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(DIO_E_PARAM_INVALID_PORT_ID, Det_MockData.ErrorId);
}

void test_Dio_MaskedWritePort_Valid_ShouldDoMaskedWrite(void) {
    Dio_Init(&testConfig);
    /* Preset DR = 0xFF */
    MockRegisters_Write32(DIO_GPIO1_BASE + DIO_GPIO_DR, 0x000000FFU);
    /* Write 0x00 with mask 0x0F: lower nibble cleared, upper nibble preserved */
    Dio_MaskedWritePort(0U, 0x00U, 0x0FU);
    uint32 dr = MockRegisters_Read32(DIO_GPIO1_BASE + DIO_GPIO_DR);
    TEST_ASSERT_EQUAL_HEX32(0x000000F0U, dr);
}

void test_Dio_MaskedWritePort_ValidPartial_ShouldDoMaskedWrite(void) {
    Dio_Init(&testConfig);
    /* Preset DR = 0x00 */
    MockRegisters_Write32(DIO_GPIO1_BASE + DIO_GPIO_DR, 0x00000000U);
    /* Write 0xAB with mask 0x0F: lower nibble = 0xB */
    Dio_MaskedWritePort(0U, 0xABU, 0x0FU);
    uint32 dr = MockRegisters_Read32(DIO_GPIO1_BASE + DIO_GPIO_DR);
    TEST_ASSERT_EQUAL_HEX32(0x0000000BU, dr);
}

/* Note: ReadChannel/WriteChannel/FlipChannel use channel IDs where
 * DIO_CHANNEL_B0=256 >= DIO_NUM_PORTS*DIO_NUM_CHANNELS_PER_PORT=256,
 * so they are rejected by the range check. Switch coverage for
 * Dio_GetGpioBaseAddr is already exercised via ReadPort/WritePort tests. */
