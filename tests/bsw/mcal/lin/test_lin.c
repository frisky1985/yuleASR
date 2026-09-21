/**
 * @file test_lin.c
 * @brief Lin Unit Tests (substantiated)
 * @version 1.0.0
 * @date 2026-08-25
 *
 * Links the real production source src/bsw/mcal/lin/src/Lin.c against the
 * mock_det error hook. The LIN driver is a pure state machine (no register
 * access), so the tests focus on DET parameter validation and the channel
 * status transitions.
 *
 * Notes on driver behaviour covered here:
 *  - Lin_DeInit() fully resets the module state, so every test re-initialises
 *    the driver through test_Lin_EnsureInitialized() (DeInit + Init cycle).
 *  - Tests that must observe the genuine uninitialized state are named
 *    *BeforeInit* so the generated runner executes them first.
 *  - Several APIs report no DET at all (Lin_SendResponse, Lin_WakeUpInternal,
 *    Lin_GoToSleepInternal, Lin_CheckWakeup, Lin_GetVersionInfo); the tests
 *    document this actual behaviour.
 */

// @tests src/bsw/mcal/lin/src/Lin.c  @tests src/bsw/mcal/lin/include/Lin.h

#include "unity.h"
#include "mock_registers.h"
#include "mock_det.h"

#include "Lin.h"
#include "Lin_Cfg.h"

#define TEST_LIN_INSTANCE_ID        (0x00U)

/*===========================================================================
 * Test configuration
 *==========================================================================*/
static Lin_ChannelConfigType test_ChannelConfigs[LIN_MAX_CHANNELS];
static Lin_ConfigType test_Config;

static void test_Lin_SetupConfig(uint8 numChannels)
{
    uint8 i;

    for (i = 0U; i < LIN_MAX_CHANNELS; i++) {
        test_ChannelConfigs[i].LinChannelBaudRate = LIN_BAUDRATE_19200;
        test_ChannelConfigs[i].LinChannelId = i;
        test_ChannelConfigs[i].LinChannelWakeupSupport = TRUE;
        test_ChannelConfigs[i].LinChannelSleepMode = 0U;
    }

    test_Config.ChannelConfigPtr = test_ChannelConfigs;
    test_Config.NumChannels = numChannels;
    test_Config.DevErrorDetect = TRUE;
    test_Config.VersionInfoApi = TRUE;
}

/*===========================================================================
 * Driver (re-)initialisation helper
 * Lin_DeInit clears the static init flag, so a DeInit/Init cycle restores a
 * canonical post-Init state for every test.
 *==========================================================================*/
static boolean test_LinDriverReady = FALSE;

static void test_Lin_EnsureInitialized(void)
{
    if (test_LinDriverReady) {
        Lin_DeInit();
    }
    test_Lin_SetupConfig(LIN_MAX_CHANNELS);
    Lin_Init(&test_Config);
    Det_Mock_Reset();
    test_LinDriverReady = TRUE;
}

/*===========================================================================
 * Unity fixture
 *==========================================================================*/
void setUp(void)
{
    MockRegisters_Reset();
    Det_Mock_Reset();
}

void tearDown(void)
{
}

/*===========================================================================
 * DET assertion helper
 *==========================================================================*/
static void test_Lin_ExpectDet(uint8 ApiId, uint8 ErrorId)
{
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL_UINT32(LIN_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL_UINT32(TEST_LIN_INSTANCE_ID, Det_MockData.InstanceId);
    TEST_ASSERT_EQUAL_UINT32(ApiId, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL_UINT32(ErrorId, Det_MockData.ErrorId);
    TEST_ASSERT_EQUAL_UINT32(1U, Det_MockData.CallCount);
}

/*===========================================================================
 * Uninitialized-driver tests (executed first by the generated runner)
 *==========================================================================*/

/** @req SWS_Lin_00001 */
void test_Lin_BeforeInit_Init_NullPtr_ShouldReportDet(void)
{
    Lin_Init(NULL_PTR);
    test_Lin_ExpectDet(LIN_INIT_SID, LIN_E_INVALID_POINTER);
}

/** @req SWS_Lin_00002 */
void test_Lin_BeforeInit_DeInit_ShouldReportUninit(void)
{
    Lin_DeInit();
    test_Lin_ExpectDet(LIN_DEINIT_SID, LIN_E_UNINIT);
}

/** @req SWS_Lin_00004 */
void test_Lin_BeforeInit_SendFrame_Uninit_ShouldReportDet(void)
{
    uint8 sdu[2U] = {0x11U, 0x22U};
    Lin_PduType pdu;
    pdu.Pid = 0x10U;
    pdu.FrameType = LIN_FRAMETYPE_UNCONDITIONAL;
    pdu.FrameResponse = LIN_MASTER_RESPONSE;
    pdu.Length = 2U;
    pdu.ChecksumType = LIN_ENHANCED_CS;
    pdu.SduPtr = sdu;

    Std_ReturnType ret = Lin_SendFrame(0U, &pdu);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Lin_ExpectDet(LIN_SENDFRAME_SID, LIN_E_UNINIT);
}

/** @req SWS_Lin_00010 */
void test_Lin_BeforeInit_GetStatus_Uninit_ShouldReportDet(void)
{
    uint8* sduPtr = NULL_PTR;

    Lin_StatusType status = Lin_GetStatus(0U, &sduPtr);
    TEST_ASSERT_EQUAL(LIN_NOT_OK, status);
    test_Lin_ExpectDet(LIN_GETSTATUS_SID, LIN_E_UNINIT);
}

/** @req SWS_Lin_00006 */
void test_Lin_BeforeInit_WakeUp_Uninit_ShouldReportDet(void)
{
    Std_ReturnType ret = Lin_WakeUp(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Lin_ExpectDet(LIN_WAKEUP_SID, LIN_E_UNINIT);
}

/** @req SWS_Lin_00008 */
void test_Lin_BeforeInit_GoToSleep_Uninit_ShouldReportDet(void)
{
    Std_ReturnType ret = Lin_GoToSleep(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Lin_ExpectDet(LIN_GOTOSLEEP_SID, LIN_E_UNINIT);
}

/** @req SWS_Lin_00003 */
void test_Lin_BeforeInit_GetVersionInfo_NullPtr_ShouldNotReportDet(void)
{
    /* Actual source behaviour: GetVersionInfo silently ignores a NULL
     * pointer (no DET report despite LIN_DEV_ERROR_DETECT = STD_ON). */
    Lin_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Lin_00005 */
void test_Lin_BeforeInit_SendResponse_Uninit_ShouldReturnNotOkWithoutDet(void)
{
    /* Actual source behaviour: SendResponse has no DET reporting at all. */
    Std_ReturnType ret = Lin_SendResponse(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Lin_00009 */
void test_Lin_BeforeInit_InternalApis_Uninit_ShouldReturnNotOkWithoutDet(void)
{
    /* Actual source behaviour: the internal helper APIs perform no DET
     * reporting, they only return E_NOT_OK when not initialised. */
    TEST_ASSERT_EQUAL(E_NOT_OK, Lin_WakeUpInternal(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, Lin_GoToSleepInternal(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, Lin_CheckWakeup(0U));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/*===========================================================================
 * Initialisation / de-initialisation
 *==========================================================================*/

/** @req SWS_Lin_00001 */
void test_Lin_Init_AllChannelsStartInSleep(void)
{
    test_Lin_EnsureInitialized();

    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(LIN_CH_SLEEP, Lin_GetStatus(0U, NULL_PTR));
    TEST_ASSERT_EQUAL(LIN_CH_SLEEP, Lin_GetStatus(1U, NULL_PTR));
}

/** @req SWS_Lin_00001 */
void test_Lin_Init_PartialConfig_UnconfiguredChannelRejected(void)
{
    uint8 sdu[2U] = {0x11U, 0x22U};
    Lin_PduType pdu;
    pdu.Pid = 0x10U;
    pdu.FrameType = LIN_FRAMETYPE_UNCONDITIONAL;
    pdu.FrameResponse = LIN_MASTER_RESPONSE;
    pdu.Length = 2U;
    pdu.ChecksumType = LIN_ENHANCED_CS;
    pdu.SduPtr = sdu;

    test_Lin_EnsureInitialized();
    /* re-init with only channel 0 configured */
    Lin_DeInit();
    test_Lin_SetupConfig(1U);
    Lin_Init(&test_Config);
    Det_Mock_Reset();

    TEST_ASSERT_EQUAL(LIN_CH_SLEEP, Lin_GetStatus(0U, NULL_PTR));
    /* channel 1 is beyond NumChannels -> treated as unconfigured channel */
    Std_ReturnType ret = Lin_SendFrame(1U, &pdu);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Lin_ExpectDet(LIN_SENDFRAME_SID, LIN_E_INVALID_CHANNEL);

    /* channel 0 still works */
    Det_Mock_Reset();
    ret = Lin_SendFrame(0U, &pdu);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Lin_00001 */
void test_Lin_Init_RepeatedInit_ResetsChannelState(void)
{
    uint8 sdu[2U] = {0x11U, 0x22U};
    Lin_PduType pdu;
    pdu.Pid = 0x10U;
    pdu.FrameType = LIN_FRAMETYPE_UNCONDITIONAL;
    pdu.FrameResponse = LIN_MASTER_RESPONSE;
    pdu.Length = 2U;
    pdu.ChecksumType = LIN_ENHANCED_CS;
    pdu.SduPtr = sdu;

    test_Lin_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_OK, Lin_SendFrame(0U, &pdu));
    TEST_ASSERT_EQUAL(LIN_TX_BUSY, Lin_GetStatus(0U, NULL_PTR));

    /* Actual source behaviour: Lin_Init has no double-init check; a repeated
     * init is accepted and resets all channel states. */
    Lin_Init(&test_Config);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(LIN_CH_SLEEP, Lin_GetStatus(0U, NULL_PTR));
}

/** @req SWS_Lin_00002 */
void test_Lin_DeInit_ShouldResetModuleAndAllowReinit(void)
{
    test_Lin_EnsureInitialized();

    Lin_DeInit();
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);

    /* module is genuinely de-initialised */
    uint8* sduPtr = NULL_PTR;
    Lin_StatusType status = Lin_GetStatus(0U, &sduPtr);
    TEST_ASSERT_EQUAL(LIN_NOT_OK, status);
    test_Lin_ExpectDet(LIN_GETSTATUS_SID, LIN_E_UNINIT);

    /* driver can be initialised again */
    Det_Mock_Reset();
    test_Lin_SetupConfig(LIN_MAX_CHANNELS);
    Lin_Init(&test_Config);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(LIN_CH_SLEEP, Lin_GetStatus(0U, NULL_PTR));
}

/*===========================================================================
 * Frame transmission
 *==========================================================================*/

/** @req SWS_Lin_00004 */
void test_Lin_SendFrame_InvalidChannel_ShouldReportDet(void)
{
    uint8 sdu[2U] = {0x11U, 0x22U};
    Lin_PduType pdu;
    pdu.Pid = 0x10U;
    pdu.FrameType = LIN_FRAMETYPE_UNCONDITIONAL;
    pdu.FrameResponse = LIN_MASTER_RESPONSE;
    pdu.Length = 2U;
    pdu.ChecksumType = LIN_ENHANCED_CS;
    pdu.SduPtr = sdu;

    test_Lin_EnsureInitialized();

    Std_ReturnType ret = Lin_SendFrame(LIN_MAX_CHANNELS, &pdu);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Lin_ExpectDet(LIN_SENDFRAME_SID, LIN_E_INVALID_CHANNEL);
}

/** @req SWS_Lin_00004 */
void test_Lin_SendFrame_NullPdu_ShouldReportDet(void)
{
    test_Lin_EnsureInitialized();

    Std_ReturnType ret = Lin_SendFrame(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Lin_ExpectDet(LIN_SENDFRAME_SID, LIN_E_INVALID_POINTER);
}

/** @req SWS_Lin_00004 */
void test_Lin_SendFrame_Valid_ShouldSetChannelTxBusy(void)
{
    uint8 sdu[LIN_MAX_FRAME_LENGTH] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
    Lin_PduType pdu;
    pdu.Pid = 0x2AU;
    pdu.FrameType = LIN_FRAMETYPE_UNCONDITIONAL;
    pdu.FrameResponse = LIN_MASTER_RESPONSE;
    pdu.Length = LIN_MAX_FRAME_LENGTH;
    pdu.ChecksumType = LIN_ENHANCED_CS;
    pdu.SduPtr = sdu;

    test_Lin_EnsureInitialized();

    Std_ReturnType ret = Lin_SendFrame(0U, &pdu);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(LIN_TX_BUSY, Lin_GetStatus(0U, NULL_PTR));
    /* other channel untouched */
    TEST_ASSERT_EQUAL(LIN_CH_SLEEP, Lin_GetStatus(1U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Lin_00004 */
void test_Lin_SendFrame_ChannelBusy_ShouldReturnNotOkWithoutDet(void)
{
    uint8 sdu[2U] = {0x11U, 0x22U};
    Lin_PduType pdu;
    pdu.Pid = 0x10U;
    pdu.FrameType = LIN_FRAMETYPE_UNCONDITIONAL;
    pdu.FrameResponse = LIN_MASTER_RESPONSE;
    pdu.Length = 2U;
    pdu.ChecksumType = LIN_ENHANCED_CS;
    pdu.SduPtr = sdu;

    test_Lin_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_OK, Lin_SendFrame(0U, &pdu));
    TEST_ASSERT_EQUAL(LIN_TX_BUSY, Lin_GetStatus(0U, NULL_PTR));

    /* second frame on the busy channel is rejected without DET */
    Std_ReturnType ret = Lin_SendFrame(0U, &pdu);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    /* channel remains busy */
    TEST_ASSERT_EQUAL(LIN_TX_BUSY, Lin_GetStatus(0U, NULL_PTR));
}

/** @req SWS_Lin_00004 */
void test_Lin_SendFrame_LengthBeyondMaxFrame_CappedWithoutCrash(void)
{
    uint8 sdu[LIN_MAX_FRAME_LENGTH] = {0U};
    Lin_PduType pdu;
    pdu.Pid = 0x10U;
    pdu.FrameType = LIN_FRAMETYPE_UNCONDITIONAL;
    pdu.FrameResponse = LIN_MASTER_RESPONSE;
    pdu.Length = 255U; /* far beyond LIN_MAX_FRAME_LENGTH */
    pdu.ChecksumType = LIN_CLASSIC_CS;
    pdu.SduPtr = sdu;

    test_Lin_EnsureInitialized();

    Std_ReturnType ret = Lin_SendFrame(0U, &pdu);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(LIN_TX_BUSY, Lin_GetStatus(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Lin_00005 */
void test_Lin_SendResponse_Valid_ShouldReturnOkWithoutStateChange(void)
{
    uint8 sdu[2U] = {0x33U, 0x44U};
    Lin_PduType pdu;
    pdu.Pid = 0x10U;
    pdu.FrameType = LIN_FRAMETYPE_UNCONDITIONAL;
    pdu.FrameResponse = LIN_SLAVE_RESPONSE;
    pdu.Length = 2U;
    pdu.ChecksumType = LIN_ENHANCED_CS;
    pdu.SduPtr = sdu;

    test_Lin_EnsureInitialized();

    Std_ReturnType ret = Lin_SendResponse(1U, &pdu);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* SendResponse does not alter the channel status */
    TEST_ASSERT_EQUAL(LIN_CH_SLEEP, Lin_GetStatus(1U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Lin_00005 */
void test_Lin_DisableResponse_Valid_ShouldReturnOk(void)
{
    test_Lin_EnsureInitialized();

    Std_ReturnType ret = Lin_DisableResponse(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/*===========================================================================
 * Wake-up / sleep transitions
 *==========================================================================*/

/** @req SWS_Lin_00006 */
void test_Lin_WakeUp_Valid_ShouldSetOperational(void)
{
    test_Lin_EnsureInitialized();

    Std_ReturnType ret = Lin_WakeUp(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(LIN_OPERATIONAL, Lin_GetStatus(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Lin_00006 */
void test_Lin_WakeUp_InvalidChannel_ShouldReportDet(void)
{
    test_Lin_EnsureInitialized();

    Std_ReturnType ret = Lin_WakeUp(LIN_MAX_CHANNELS);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Lin_ExpectDet(LIN_WAKEUP_SID, LIN_E_INVALID_CHANNEL);
}

/** @req SWS_Lin_00008 */
void test_Lin_GoToSleep_Valid_ShouldSetChannelSleep(void)
{
    test_Lin_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_OK, Lin_WakeUp(0U));
    TEST_ASSERT_EQUAL(LIN_OPERATIONAL, Lin_GetStatus(0U, NULL_PTR));

    Std_ReturnType ret = Lin_GoToSleep(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(LIN_CH_SLEEP, Lin_GetStatus(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Lin_00008 */
void test_Lin_GoToSleep_InvalidChannel_ShouldReportDet(void)
{
    test_Lin_EnsureInitialized();

    Std_ReturnType ret = Lin_GoToSleep(LIN_MAX_CHANNELS);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Lin_ExpectDet(LIN_GOTOSLEEP_SID, LIN_E_INVALID_CHANNEL);
}

/** @req SWS_Lin_00009 */
void test_Lin_WakeUpInternal_Valid_ShouldSetOperationalWithoutDet(void)
{
    test_Lin_EnsureInitialized();

    Std_ReturnType ret = Lin_WakeUpInternal(1U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(LIN_OPERATIONAL, Lin_GetStatus(1U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Lin_00009 */
void test_Lin_GoToSleepInternal_Valid_ShouldSetSleepWithoutDet(void)
{
    test_Lin_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_OK, Lin_WakeUp(0U));

    Std_ReturnType ret = Lin_GoToSleepInternal(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(LIN_CH_SLEEP, Lin_GetStatus(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Lin_00009 */
void test_Lin_CheckWakeup_Valid_ShouldReturnOk(void)
{
    test_Lin_EnsureInitialized();

    Std_ReturnType ret = Lin_CheckWakeup(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Lin_00013 */
void test_Lin_WakeUpConfirmation_ShouldSetOperational(void)
{
    test_Lin_EnsureInitialized();

    Lin_WakeUpConfirmation(0U);
    TEST_ASSERT_EQUAL(LIN_OPERATIONAL, Lin_GetStatus(0U, NULL_PTR));
    /* out-of-range channel is silently ignored */
    Lin_WakeUpConfirmation(LIN_MAX_CHANNELS);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/*===========================================================================
 * Status query
 *==========================================================================*/

/** @req SWS_Lin_00010 */
void test_Lin_GetStatus_ShouldProvideRxBufferPointer(void)
{
    uint8* sduPtr = NULL_PTR;

    test_Lin_EnsureInitialized();

    Lin_StatusType status = Lin_GetStatus(0U, &sduPtr);
    TEST_ASSERT_EQUAL(LIN_CH_SLEEP, status);
    TEST_ASSERT_NOT_NULL(sduPtr);
    /* the returned pointer addresses the internal RX buffer (writable) */
    sduPtr[0U] = 0xABU;
    TEST_ASSERT_EQUAL_UINT32(0xABU, sduPtr[0U]);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Lin_00010 */
void test_Lin_GetStatus_InvalidChannel_ShouldReportDet(void)
{
    uint8* sduPtr = NULL_PTR;

    test_Lin_EnsureInitialized();

    Lin_StatusType status = Lin_GetStatus(LIN_MAX_CHANNELS, &sduPtr);
    TEST_ASSERT_EQUAL(LIN_NOT_OK, status);
    test_Lin_ExpectDet(LIN_GETSTATUS_SID, LIN_E_INVALID_CHANNEL);
}

/*===========================================================================
 * ISR hooks
 *==========================================================================*/

/** @req SWS_Lin_00015 */
void test_Lin_IsrHooks_ShouldUpdateChannelStatus(void)
{
    test_Lin_EnsureInitialized();

    Lin_IsrTx(0U);
    TEST_ASSERT_EQUAL(LIN_TX_OK, Lin_GetStatus(0U, NULL_PTR));

    Lin_IsrRx(0U);
    TEST_ASSERT_EQUAL(LIN_RX_OK, Lin_GetStatus(0U, NULL_PTR));

    Lin_IsrErr(0U);
    TEST_ASSERT_EQUAL(LIN_TX_ERROR, Lin_GetStatus(0U, NULL_PTR));

    /* out-of-range channel is silently ignored */
    Lin_IsrTx(LIN_MAX_CHANNELS);
    Lin_IsrRx(LIN_MAX_CHANNELS);
    Lin_IsrErr(LIN_MAX_CHANNELS);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Lin_00014 */
void test_Lin_WakeUpFrameIndication_ShouldNotReportDet(void)
{
    test_Lin_EnsureInitialized();

    Lin_WakeUpFrameIndication();
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/*===========================================================================
 * Version info
 *==========================================================================*/

/** @req SWS_Lin_00003 */
void test_Lin_GetVersionInfo_ValidPtr_ShouldFillVersionFields(void)
{
    Std_VersionInfoType info;

    test_Lin_EnsureInitialized();
    Lin_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT32(0x0001U, info.vendorID); /* LIN_VENDOR_ID */
    TEST_ASSERT_EQUAL_UINT32(LIN_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT32(LIN_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT32(LIN_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT32(LIN_SW_PATCH_VERSION, info.sw_patch_version);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}
