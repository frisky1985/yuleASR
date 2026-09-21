/**
 * @file test_adc.c
 * @brief Adc (ADC Driver) Unit Tests — Substantiated
 * @req SWS_Adc
 *
 * Substantiation: DET parameter validation (module/service/error IDs), register
 * write verification via mock_registers (ADC1 base 0x30610000, non-S32K312 build):
 * CFG/GC programming in Init, HC0 channel select, ADCONV/ADTRG control bits,
 * conversion result capture (R0 masked to 12 bit) and group status transitions.
 *
 * Note on polling loops: Adc_Init polls GS.ADACT and Adc_StartGroupConversion
 * polls HS.COCO0. Unwritten mock registers read 0, so Init's poll exits on its
 * own; StartGroupConversion needs HS.COCO0 preset to 1 before the call.
 */

// @tests src/bsw/mcal/adc/src/Adc.c  @tests src/bsw/mcal/adc/include/Adc.h
#include "unity.h"
#include "mock_registers.h"
#include "mock_det.h"

#include "Adc.h"
#include "Adc_Cfg.h"

/* ADC1 base address (non-S32K312 build) — must match Adc.c */
#define ADC1_BASE           (0x30610000UL)
#define ADC_HC0_OFF         (0x00U)
#define ADC_HS_OFF          (0x04U)
#define ADC_R0_OFF          (0x08U)
#define ADC_CFG_OFF         (0x0CU)
#define ADC_GC_OFF          (0x10U)
#define ADC_GS_OFF          (0x14U)

#define ADC_GC_ADACKEN      (0x00000001UL)
#define ADC_GC_ADCONV       (0x00000004UL)
#define ADC_GC_ADTRG        (0x00000008UL)
#define ADC_GC_ADCO         (0x00000080UL)
#define ADC_HS_COCO0        (0x00000001UL)
/* value Adc_Init programs into CFG: IPG clock | 12-bit mode | clock div by 2 */
#define ADC_CFG_INIT_VALUE  (0x29UL)

static Adc_HWUnitConfigType  testHwUnits[2];
static Adc_ChannelConfigType testChannelConfigs[4];
static Adc_GroupConfigType   testGroups[ADC_NUM_GROUPS];
static Adc_ChannelType       group0Channels[2] = { ADC_CHANNEL_3, ADC_CHANNEL_5 };
static Adc_ChannelType       group1Channels[1] = { ADC_CHANNEL_7 };
static Adc_ChannelType       group2Channels[1] = { ADC_CHANNEL_1 };
static Adc_ConfigType        testConfig;

static uint32 notificationCount = 0U;
static void test_Adc_NotificationCallback(void) { notificationCount++; }

/* Adc.c keeps the initialized flag in a static variable. The driver can be
 * DeInit'd and re-initialized, so the tests track the current state with a
 * local flag (same pattern as test_can.c / test_gpt.c). */
static boolean adc_driverReady = FALSE;

static void test_Adc_SetupConfig(void)
{
    testHwUnits[0].HwUnitId = ADC_HWUNIT_0;
    testHwUnits[0].BaseAddress = ADC1_BASE;
    testHwUnits[0].ClockFrequency = ADC_CLOCK_FREQUENCY_HZ;
    testHwUnits[0].DefaultResolution = ADC_DEFAULT_RESOLUTION;
    /* Init iterates over ADC_NUM_HW_UNITS (=2) HwUnits entries; HW unit 1 has
     * no base-address mapping in Adc.c and must be skipped without side effects. */
    testHwUnits[1].HwUnitId = ADC_HWUNIT_1;
    testHwUnits[1].BaseAddress = 0U;
    testHwUnits[1].ClockFrequency = ADC_CLOCK_FREQUENCY_HZ;
    testHwUnits[1].DefaultResolution = ADC_DEFAULT_RESOLUTION;

    for (uint8 i = 0U; i < 4U; i++) {
        testChannelConfigs[i].ChannelId = (Adc_ChannelType)i;
        testChannelConfigs[i].SamplingTime = ADC_DEFAULT_SAMPLING_TIME;
        testChannelConfigs[i].ChannelInput = 0U;
    }

    for (uint8 g = 0U; g < ADC_NUM_GROUPS; g++) {
        testGroups[g].GroupId = g;
        testGroups[g].HwUnit = ADC_HWUNIT_0;
        testGroups[g].Channels = group0Channels;
        testGroups[g].NumChannels = 0U;
        testGroups[g].TriggerSource = ADC_TRIGG_SRC_SW;
        testGroups[g].ConversionMode = ADC_CONV_MODE_ONESHOT;
        testGroups[g].AccessMode = ADC_ACCESS_MODE_SINGLE;
        testGroups[g].BufferMode = ADC_STREAM_BUFFER_LINEAR;
        testGroups[g].NumSamples = ADC_STREAM_NUM_SAMPLES;
        testGroups[g].Resolution = ADC_RESOLUTION_12BIT;
        testGroups[g].GroupNotification = FALSE;
        testGroups[g].NotificationFn = NULL_PTR;
    }
    /* group 0: two software-triggered channels, no notification */
    testGroups[0].Channels = group0Channels;
    testGroups[0].NumChannels = 2U;
    /* group 1: hardware trigger source (for Enable/DisableHardwareTrigger) */
    testGroups[1].Channels = group1Channels;
    testGroups[1].NumChannels = 1U;
    testGroups[1].TriggerSource = ADC_TRIGG_SRC_HW;
    /* group 2: conversion with notification callback */
    testGroups[2].Channels = group2Channels;
    testGroups[2].NumChannels = 1U;
    testGroups[2].GroupNotification = TRUE;
    testGroups[2].NotificationFn = test_Adc_NotificationCallback;

    testConfig.HwUnits = testHwUnits;
    testConfig.NumHwUnits = 2U;
    testConfig.Groups = testGroups;
    testConfig.NumGroups = ADC_NUM_GROUPS;
    testConfig.Channels = testChannelConfigs;
    testConfig.NumChannels = 4U;
    testConfig.DevErrorDetect = TRUE;
    testConfig.VersionInfoApi = TRUE;
    testConfig.DeInitApi = TRUE;
    testConfig.PowerStateSupported = FALSE;
}

static void test_Adc_EnsureInitialized(void)
{
    if (!adc_driverReady) {
        test_Adc_SetupConfig();
        Adc_Init(&testConfig);   /* GS.ADACT reads 0 -> calibration poll exits */
        Det_Mock_Reset();
        adc_driverReady = TRUE;
    }
}

/* No group stays BUSY (conversion completes synchronously), so DeInit always
 * proceeds and returns the driver to the uninitialized state. */
static void test_Adc_ResetDriver(void)
{
    if (adc_driverReady) {
        Adc_DeInit();
        adc_driverReady = FALSE;
    }
    Det_Mock_Reset();
}

void setUp(void)
{
    MockRegisters_Reset();
    Det_Mock_Reset();
    notificationCount = 0U;
}

void tearDown(void) {}

/* --- Uninitialized driver (runner executes these first) --- */

/** @req SWS_Adc_00001 */
void test_Adc_Init_BeforeInit_NullPtr_ShouldReportDet(void) {
    Adc_Init(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_INIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_PARAM_CONFIG, Det_MockData.ErrorId);
}

/** @req SWS_Adc_00002 */
void test_Adc_DeInit_BeforeInit_ShouldReportDet(void) {
    Adc_DeInit();
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_DEINIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_UNINIT, Det_MockData.ErrorId);
    /* rejected before touching the hardware */
    TEST_ASSERT_EQUAL_UINT32(0U, MockRegisters_Read32(ADC1_BASE + ADC_GC_OFF));
}

/** @req SWS_Adc_00003 */
void test_Adc_StartGroupConversion_BeforeInit_ShouldReportDet(void) {
    Adc_StartGroupConversion(0U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_STARTGROUPCONVERSION, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Adc_00004 */
void test_Adc_StopGroupConversion_BeforeInit_ShouldReportDet(void) {
    Adc_StopGroupConversion(0U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_STOPGROUPCONVERSION, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Adc_00005 */
void test_Adc_ReadGroup_BeforeInit_ShouldReturnNotOk(void) {
    Adc_ValueGroupType buffer[2] = {0U, 0U};
    Std_ReturnType ret = Adc_ReadGroup(0U, buffer);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_READGROUP, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Adc_00006 */
void test_Adc_EnableHardwareTrigger_BeforeInit_ShouldReportDet(void) {
    Adc_EnableHardwareTrigger(0U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_ENABLEHARDWARETRIGGER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_UNINIT, Det_MockData.ErrorId);
    TEST_ASSERT_EQUAL_UINT32(0U, MockRegisters_Read32(ADC1_BASE + ADC_GC_OFF) & ADC_GC_ADTRG);
}

/** @req SWS_Adc_00007 */
void test_Adc_DisableHardwareTrigger_BeforeInit_ShouldReportDet(void) {
    Adc_DisableHardwareTrigger(0U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_DISABLEHARDWARETRIGGER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Adc_00010 */
void test_Adc_GetGroupStatus_BeforeInit_ShouldReturnIdle(void) {
    TEST_ASSERT_EQUAL(ADC_IDLE, Adc_GetGroupStatus(0U));
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_GETGROUPSTATUS, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Adc_00012 */
void test_Adc_GetStreamLastPointer_BeforeInit_ShouldReturnZero(void) {
    Adc_ValueGroupType* streamPtr = NULL_PTR;
    Adc_StreamNumSampleType count = Adc_GetStreamLastPointer(0U, &streamPtr);
    TEST_ASSERT_EQUAL_UINT32(0U, count);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_GETSTREAMLASTPOINTER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Adc_00013 */
void test_Adc_SetupResultBuffer_BeforeInit_ShouldReturnNotOk(void) {
    Adc_ValueGroupType buffer[2] = {0U, 0U};
    Std_ReturnType ret = Adc_SetupResultBuffer(0U, buffer);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_SETUPRESULTBUFFER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_UNINIT, Det_MockData.ErrorId);
}

/* --- Init / DeInit --- */

/** @req SWS_Adc_00001 */
void test_Adc_Init_ValidConfig_ShouldProgramRegisters(void) {
    test_Adc_ResetDriver();   /* in case an earlier test already initialized */
    test_Adc_SetupConfig();
    Adc_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    /* CFG: IPG clock | 12-bit mode | clock divide by 2 */
    TEST_ASSERT_EQUAL_UINT32(ADC_CFG_INIT_VALUE, MockRegisters_Read32(ADC1_BASE + ADC_CFG_OFF));
    /* GC: ADACKEN | ADCONV after the calibration kick-off */
    TEST_ASSERT_EQUAL_UINT32((ADC_GC_ADACKEN | ADC_GC_ADCONV),
                             MockRegisters_Read32(ADC1_BASE + ADC_GC_OFF));
    /* all group statuses start at idle */
    TEST_ASSERT_EQUAL(ADC_IDLE, Adc_GetGroupStatus(0U));
    adc_driverReady = TRUE;
}

/** @req SWS_Adc_00001 */
void test_Adc_Init_DoubleInit_ShouldReportDet(void) {
    test_Adc_EnsureInitialized();
    uint32 gcBefore = MockRegisters_Read32(ADC1_BASE + ADC_GC_OFF);
    Adc_Init(&testConfig);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_INIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_ALREADY_INITIALIZED, Det_MockData.ErrorId);
    /* rejected: registers keep the values from the first Init */
    TEST_ASSERT_EQUAL_UINT32(gcBefore, MockRegisters_Read32(ADC1_BASE + ADC_GC_OFF));
}

/** @req SWS_Adc_00002 */
void test_Adc_DeInit_AfterInit_ShouldDisableAdc(void) {
    test_Adc_EnsureInitialized();   /* leaves GC = ADACKEN | ADCONV */
    Adc_DeInit();
    adc_driverReady = FALSE;
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    /* GC cleared by DeInit */
    TEST_ASSERT_EQUAL_UINT32(0U, MockRegisters_Read32(ADC1_BASE + ADC_GC_OFF));
    /* driver can be initialized again afterwards */
    Adc_Init(&testConfig);
    adc_driverReady = TRUE;
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL_UINT32((ADC_GC_ADACKEN | ADC_GC_ADCONV),
                             MockRegisters_Read32(ADC1_BASE + ADC_GC_OFF));
}

/* --- StartGroupConversion --- */

/** @req SWS_Adc_00003 */
void test_Adc_StartGroupConversion_InvalidGroup_ShouldReportDet(void) {
    test_Adc_EnsureInitialized();
    Adc_StartGroupConversion(ADC_NUM_GROUPS);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_STARTGROUPCONVERSION, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_PARAM_GROUP, Det_MockData.ErrorId);
}

/** @req SWS_Adc_00003 */
void test_Adc_StartGroupConversion_AfterInit_ShouldCompleteAndStoreResults(void) {
    test_Adc_EnsureInitialized();
    /* preset conversion-complete flag and a raw result with bits above 12 */
    MockRegisters_Write32(ADC1_BASE + ADC_HS_OFF, ADC_HS_COCO0);
    MockRegisters_Write32(ADC1_BASE + ADC_R0_OFF, 0xF123U);
    Adc_StartGroupConversion(0U);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    /* last channel of group 0 (channel 5) selected in HC0 */
    TEST_ASSERT_EQUAL_UINT32(5U, MockRegisters_Read32(ADC1_BASE + ADC_HC0_OFF));
    /* software trigger: ADCONV set, ADTRG cleared */
    TEST_ASSERT_EQUAL_UINT32(ADC_GC_ADCONV,
                             MockRegisters_Read32(ADC1_BASE + ADC_GC_OFF) & ADC_GC_ADCONV);
    TEST_ASSERT_EQUAL_UINT32(0U,
                             MockRegisters_Read32(ADC1_BASE + ADC_GC_OFF) & ADC_GC_ADTRG);
    /* group completed synchronously */
    TEST_ASSERT_EQUAL(ADC_STREAM_COMPLETED, Adc_GetGroupStatus(0U));
    /* results readable, masked to 12 bits */
    Adc_ValueGroupType buffer[2] = {0U, 0U};
    TEST_ASSERT_EQUAL(E_OK, Adc_ReadGroup(0U, buffer));
    TEST_ASSERT_EQUAL_UINT16(0x123U, buffer[0]);
    TEST_ASSERT_EQUAL_UINT16(0x123U, buffer[1]);
    /* group 0 has no notification configured */
    TEST_ASSERT_EQUAL_UINT32(0U, notificationCount);
}

/** @req SWS_Adc_00003 */
void test_Adc_StartGroupConversion_NotificationEnabled_ShouldCallCallback(void) {
    test_Adc_EnsureInitialized();
    MockRegisters_Write32(ADC1_BASE + ADC_HS_OFF, ADC_HS_COCO0);
    MockRegisters_Write32(ADC1_BASE + ADC_R0_OFF, 0x0ABCU);
    Adc_StartGroupConversion(2U);   /* group 2 has GroupNotification enabled */
    TEST_ASSERT_EQUAL_UINT32(1U, notificationCount);
    TEST_ASSERT_EQUAL(ADC_STREAM_COMPLETED, Adc_GetGroupStatus(2U));
}

/* --- StopGroupConversion --- */

/** @req SWS_Adc_00004 */
void test_Adc_StopGroupConversion_InvalidGroup_ShouldReportDet(void) {
    test_Adc_EnsureInitialized();
    Adc_StopGroupConversion(ADC_NUM_GROUPS);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_STOPGROUPCONVERSION, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_PARAM_GROUP, Det_MockData.ErrorId);
}

/** @req SWS_Adc_00004 */
void test_Adc_StopGroupConversion_NotBusy_ShouldReturnSilently(void) {
    test_Adc_EnsureInitialized();
    MockRegisters_Write32(ADC1_BASE + ADC_HS_OFF, ADC_HS_COCO0);
    MockRegisters_Write32(ADC1_BASE + ADC_R0_OFF, 0xF123U);
    Adc_StartGroupConversion(0U);   /* completes synchronously, never BUSY */
    Adc_StopGroupConversion(0U);
    /* group is not BUSY -> stop returns without touching hardware or DET */
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL_UINT32(ADC_GC_ADCONV,
                             MockRegisters_Read32(ADC1_BASE + ADC_GC_OFF) & ADC_GC_ADCONV);
    TEST_ASSERT_EQUAL(ADC_STREAM_COMPLETED, Adc_GetGroupStatus(0U));
}

/* --- ReadGroup --- */

/** @req SWS_Adc_00005 */
void test_Adc_ReadGroup_InvalidGroup_ShouldReturnNotOk(void) {
    test_Adc_EnsureInitialized();
    Adc_ValueGroupType buffer[2] = {0U, 0U};
    Std_ReturnType ret = Adc_ReadGroup(ADC_NUM_GROUPS, buffer);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_READGROUP, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_PARAM_GROUP, Det_MockData.ErrorId);
}

/** @req SWS_Adc_00005 */
void test_Adc_ReadGroup_NullBuffer_ShouldReturnNotOk(void) {
    test_Adc_EnsureInitialized();
    Std_ReturnType ret = Adc_ReadGroup(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_READGROUP, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/* --- Hardware trigger --- */

/** @req SWS_Adc_00006 */
void test_Adc_EnableHardwareTrigger_HwGroup_ShouldSetAdtrg(void) {
    test_Adc_EnsureInitialized();
    Adc_EnableHardwareTrigger(1U);   /* group 1 configured with HW trigger source */
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL_UINT32(ADC_GC_ADTRG,
                             MockRegisters_Read32(ADC1_BASE + ADC_GC_OFF) & ADC_GC_ADTRG);
}

/** @req SWS_Adc_00006 */
void test_Adc_EnableHardwareTrigger_SwGroup_ShouldNotSetAdtrg(void) {
    test_Adc_EnsureInitialized();
    Adc_EnableHardwareTrigger(0U);   /* group 0 configured with SW trigger source */
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL_UINT32(0U,
                             MockRegisters_Read32(ADC1_BASE + ADC_GC_OFF) & ADC_GC_ADTRG);
}

/** @req SWS_Adc_00006 */
void test_Adc_EnableHardwareTrigger_InvalidGroup_ShouldReportDet(void) {
    test_Adc_EnsureInitialized();
    Adc_EnableHardwareTrigger(ADC_NUM_GROUPS);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_ENABLEHARDWARETRIGGER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_PARAM_GROUP, Det_MockData.ErrorId);
}

/** @req SWS_Adc_00007 */
void test_Adc_DisableHardwareTrigger_ShouldClearAdtrg(void) {
    test_Adc_EnsureInitialized();
    MockRegisters_Write32(ADC1_BASE + ADC_GC_OFF,
                          ADC_GC_ADACKEN | ADC_GC_ADCONV | ADC_GC_ADTRG);
    Adc_DisableHardwareTrigger(1U);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL_UINT32(0U,
                             MockRegisters_Read32(ADC1_BASE + ADC_GC_OFF) & ADC_GC_ADTRG);
}

/** @req SWS_Adc_00007 */
void test_Adc_DisableHardwareTrigger_InvalidGroup_ShouldReportDet(void) {
    test_Adc_EnsureInitialized();
    Adc_DisableHardwareTrigger(ADC_NUM_GROUPS);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_DISABLEHARDWARETRIGGER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_PARAM_GROUP, Det_MockData.ErrorId);
}

/* --- Group notification --- */

/** @req SWS_Adc_00008 */
void test_Adc_EnableGroupNotification_ValidGroup_ShouldNotReportDet(void) {
    test_Adc_EnsureInitialized();
    Adc_EnableGroupNotification(0U);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Adc_00008 */
void test_Adc_EnableGroupNotification_InvalidGroup_ShouldReportDet(void) {
    test_Adc_EnsureInitialized();
    Adc_EnableGroupNotification(ADC_NUM_GROUPS);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_ENABLEGROUPNOTIFICATION, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_PARAM_GROUP, Det_MockData.ErrorId);
}

/** @req SWS_Adc_00009 */
void test_Adc_DisableGroupNotification_InvalidGroup_ShouldReportDet(void) {
    test_Adc_EnsureInitialized();
    Adc_DisableGroupNotification(ADC_NUM_GROUPS);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_DISABLEGROUPNOTIFICATION, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_PARAM_GROUP, Det_MockData.ErrorId);
}

/* --- GetGroupStatus --- */

/** @req SWS_Adc_00010 */
void test_Adc_GetGroupStatus_InvalidGroup_ShouldReportDet(void) {
    test_Adc_EnsureInitialized();
    TEST_ASSERT_EQUAL(ADC_IDLE, Adc_GetGroupStatus(ADC_NUM_GROUPS));
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_GETGROUPSTATUS, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_PARAM_GROUP, Det_MockData.ErrorId);
}

/* --- GetVersionInfo --- */

/** @req SWS_Adc_00011 */
void test_Adc_GetVersionInfo_ValidPtr_ShouldFillFields(void) {
    Std_VersionInfoType info;
    info.vendorID = 0U;
    info.moduleID = 0U;
    info.sw_major_version = 0U;
    info.sw_minor_version = 0U;
    info.sw_patch_version = 0U;
    Adc_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL_UINT16(ADC_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT16(ADC_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(ADC_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(ADC_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(ADC_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_Adc_00011 */
void test_Adc_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    Adc_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_GETVERSIONINFO, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/* --- GetStreamLastPointer --- */

/** @req SWS_Adc_00012 */
void test_Adc_GetStreamLastPointer_AfterConversion_ShouldReturnCountAndBuffer(void) {
    test_Adc_EnsureInitialized();
    MockRegisters_Write32(ADC1_BASE + ADC_HS_OFF, ADC_HS_COCO0);
    MockRegisters_Write32(ADC1_BASE + ADC_R0_OFF, 0xF123U);
    Adc_StartGroupConversion(0U);
    Adc_ValueGroupType* streamPtr = NULL_PTR;
    Adc_StreamNumSampleType count = Adc_GetStreamLastPointer(0U, &streamPtr);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    /* group 0 has two channels */
    TEST_ASSERT_EQUAL_UINT32(2U, count);
    TEST_ASSERT_NOT_NULL(streamPtr);
    /* stream buffer exposes the captured (12-bit masked) results */
    TEST_ASSERT_EQUAL_UINT16(0x123U, streamPtr[0]);
    TEST_ASSERT_EQUAL_UINT16(0x123U, streamPtr[1]);
}

/** @req SWS_Adc_00012 */
void test_Adc_GetStreamLastPointer_InvalidGroup_ShouldReportDet(void) {
    test_Adc_EnsureInitialized();
    Adc_ValueGroupType* streamPtr = NULL_PTR;
    Adc_StreamNumSampleType count = Adc_GetStreamLastPointer(ADC_NUM_GROUPS, &streamPtr);
    TEST_ASSERT_EQUAL_UINT32(0U, count);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_GETSTREAMLASTPOINTER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_PARAM_GROUP, Det_MockData.ErrorId);
}

/** @req SWS_Adc_00012 */
void test_Adc_GetStreamLastPointer_NullPtr_ShouldReportDet(void) {
    test_Adc_EnsureInitialized();
    Adc_StreamNumSampleType count = Adc_GetStreamLastPointer(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(0U, count);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_GETSTREAMLASTPOINTER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/* --- SetupResultBuffer --- */

/** @req SWS_Adc_00013 */
void test_Adc_SetupResultBuffer_Valid_ShouldReturnOk(void) {
    test_Adc_EnsureInitialized();
    Adc_ValueGroupType buffer[2] = {0U, 0U};
    Std_ReturnType ret = Adc_SetupResultBuffer(0U, buffer);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Adc_00013 */
void test_Adc_SetupResultBuffer_InvalidGroup_ShouldReturnNotOk(void) {
    test_Adc_EnsureInitialized();
    Adc_ValueGroupType buffer[2] = {0U, 0U};
    Std_ReturnType ret = Adc_SetupResultBuffer(ADC_NUM_GROUPS, buffer);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_SETUPRESULTBUFFER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_PARAM_GROUP, Det_MockData.ErrorId);
}

/** @req SWS_Adc_00013 */
void test_Adc_SetupResultBuffer_NullBuffer_ShouldReturnNotOk(void) {
    test_Adc_EnsureInitialized();
    Std_ReturnType ret = Adc_SetupResultBuffer(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ADC_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ADC_SID_SETUPRESULTBUFFER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ADC_E_PARAM_POINTER, Det_MockData.ErrorId);
}
