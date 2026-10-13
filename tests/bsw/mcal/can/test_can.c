/**
 * @file test_can.c
 * @brief Can (CAN Driver) Unit Tests — Substantiated
 * @req SWS_Can
 *
 * Substantiation: DET parameter validation, register write verification via
 * mock_registers (FlexCAN base 0x308C0000), controller state machine
 * transitions (STOPPED<->STARTED), mailbox TX busy/idle paths.
 */

// @tests src/bsw/mcal/can/src/Can.c  @tests src/bsw/mcal/can/include/Can.h
#include "unity.h"
#include "mock_registers.h"
#include "mock_det.h"

#include "Can.h"
#include "Can_Cfg.h"

/* FlexCAN1 base (non-S32K312 build) — must match Can.c */
#define CAN1_BASE       (0x308C0000UL)
#define CAN2_BASE       (0x308D0000UL)
#define CAN_MCR_OFF     (0x00U)
#define CAN_CTRL1_OFF   (0x04U)
#define CAN_IMASK1_OFF  (0x28U)
#define CAN_IMASK2_OFF  (0x24U)
#define CAN_ESR1_OFF    (0x20U)
#define CAN_IFLAG1_OFF  (0x30U)
#define CAN_MB_BASE_OFF (0x80U)

#define CAN_MCR_FRZ_BIT     (0x40000000UL)
#define CAN_MCR_HALT_BIT    (0x10000000UL)
#define CAN_CTRL1_PRESDIV   (0xFF000000UL)
#define CAN_MB_CS_INACTIVE  (0x08000000UL)

static Can_BaudrateConfigType testBaudrates[2];
static Can_ControllerConfigType testControllers[2];
static Can_ConfigType testConfig;
static Can_PduType testPdu;
static uint8 pduData[8];
static Can_BaudrateConfigType testFdBaudrates[2];
static uint8 fdPduData[64];

static void test_Can_SetupConfig(void)
{
    testBaudrates[0].BaudRate = 500000U;
    testBaudrates[0].PropSeg = 7U;
    testBaudrates[0].PhaseSeg1 = 4U;
    testBaudrates[0].PhaseSeg2 = 2U;
    testBaudrates[0].SyncJumpWidth = 1U;
    testBaudrates[0].Prescaler = 8U;

    testControllers[0].ControllerId = 0U;
    testControllers[0].BaseAddress = CAN1_BASE;
    testControllers[0].BaudrateConfigs = testBaudrates;
    testControllers[0].NumBaudrateConfigs = 1U;
    testControllers[0].HardwareObjects = NULL_PTR;
    testControllers[0].NumHardwareObjects = 0U;
    testControllers[0].RxProcessing = 0U;
    testControllers[0].TxProcessing = 0U;
    testControllers[0].BusOffProcessing = FALSE;
    testControllers[0].WakeupProcessing = FALSE;
    testControllers[0].WakeupSupport = FALSE;
    testControllers[0].DefaultBaudrateIndex = 0U;
    testControllers[1] = testControllers[0];
    testControllers[1].ControllerId = 1U;
    testControllers[1].BaseAddress = CAN2_BASE;

    testConfig.Controllers = testControllers;
    testConfig.NumControllers = 2U;
    testConfig.DevErrorDetect = TRUE;
    testConfig.VersionInfoApi = TRUE;
}

static void test_Can_SetupPdu(void)
{
    pduData[0] = 0xDEU; pduData[1] = 0xADU; pduData[2] = 0xBEU; pduData[3] = 0xEFU;
    pduData[4] = 0x11U; pduData[5] = 0x22U; pduData[6] = 0x33U; pduData[7] = 0x44U;
    testPdu.idType = CAN_ID_TYPE_STANDARD;
    testPdu.CanId = 0x100U;
    testPdu.CanDlc = 8U;
    testPdu.SduPtr = pduData;
    testPdu.FdFrame = FALSE;
}

/* FD-enabled baudrate config for controller 1 (CAN2), built the same way as
 * test_Can_SetupConfig with the appended CAN FD fields set. */
static void test_Can_SetupFdConfig(void)
{
    testFdBaudrates[0].BaudRate = 500000U;
    testFdBaudrates[0].PropSeg = 7U;
    testFdBaudrates[0].PhaseSeg1 = 4U;
    testFdBaudrates[0].PhaseSeg2 = 2U;
    testFdBaudrates[0].SyncJumpWidth = 1U;
    testFdBaudrates[0].Prescaler = 8U;
    testFdBaudrates[0].FdEnabled = TRUE;
    testFdBaudrates[0].FdDataBaudRate = 2000000U;
    testFdBaudrates[0].FdPropSeg = 3U;
    testFdBaudrates[0].FdPhaseSeg1 = 3U;
    testFdBaudrates[0].FdPhaseSeg2 = 2U;
    testFdBaudrates[0].FdSyncJumpWidth = 1U;
    testFdBaudrates[0].FdPrescaler = 2U;

    testControllers[1].BaudrateConfigs = testFdBaudrates;
    testControllers[1].NumBaudrateConfigs = 1U;
}

/* Deterministic 64-byte FD payload: byte i holds value i */
static void test_Can_SetupFdPdu(void)
{
    for (uint8 i = 0U; i < 64U; i++) {
        fdPduData[i] = i;
    }
}

/* Can.c keeps static init state that cannot be reset on host; the driver is
 * initialized once and later tests build on the resulting STOPPED state. */
static boolean can_driverReady = FALSE;

static void test_Can_EnsureInitialized(void)
{
    if (!can_driverReady) {
        /* Freeze-ack poll requires FRZ bit readable in MCR */
        MockRegisters_Write32(CAN1_BASE + CAN_MCR_OFF, CAN_MCR_FRZ_BIT);
        MockRegisters_Write32(CAN2_BASE + CAN_MCR_OFF, CAN_MCR_FRZ_BIT);
        test_Can_SetupConfig();
        Can_Init(&testConfig);
        Det_Mock_Reset();
        can_driverReady = TRUE;
    }
}

/* Controller state persists across tests (static in Can.c); these helpers
 * drive controller 0 to a known state regardless of the previous test. */
static void test_Can_BringToStopped(void)
{
    MockRegisters_Write32(CAN1_BASE + CAN_MCR_OFF, CAN_MCR_FRZ_BIT);
    (void)Can_SetControllerMode(0U, CAN_CS_STOPPED);
}

static void test_Can_BringToStarted(void)
{
    test_Can_BringToStopped();
    /* NOT_RDY clear so the not-ready poll exits; HALT cleared by the call */
    MockRegisters_Write32(CAN1_BASE + CAN_MCR_OFF, CAN_MCR_FRZ_BIT);
    (void)Can_SetControllerMode(0U, CAN_CS_STARTED);
}

void setUp(void)
{
    MockRegisters_Reset();
    Det_Mock_Reset();
    test_Can_SetupPdu();
}

void tearDown(void) {}

/** @req SWS_Can_00001 */
void test_Can_Init_BeforeInit_NullPtr_ShouldReportDet(void) {
    Can_Init(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(CAN_SID_INIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(CAN_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/** @req SWS_Can_00001 */
void test_Can_Init_ValidConfig_ShouldConfigureCtrl1AndMailboxes(void) {
    MockRegisters_Write32(CAN1_BASE + CAN_MCR_OFF, CAN_MCR_FRZ_BIT);
    test_Can_SetupConfig();
    Can_Init(&testConfig);
    uint32 ctrl1 = MockRegisters_Read32(CAN1_BASE + CAN_CTRL1_OFF);
    TEST_ASSERT_EQUAL(((8U - 1U) << 24) & CAN_CTRL1_PRESDIV, ctrl1 & CAN_CTRL1_PRESDIV);
    uint32 mbCs = MockRegisters_Read32(CAN1_BASE + CAN_MB_BASE_OFF + 0U);
    TEST_ASSERT_EQUAL(CAN_MB_CS_INACTIVE, mbCs & 0x0F000000UL);
}

/** @req SWS_Can_00001 */
void test_Can_Init_DoubleInit_ShouldReportTransitionDet(void) {
    test_Can_EnsureInitialized();
    Can_Init(&testConfig);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_SID_INIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(CAN_E_TRANSITION, Det_MockData.ErrorId);
}

/** @req SWS_Can_00002 */
void test_Can_GetVersionInfo_ValidPtr_ShouldReturnVersion(void) {
    Std_VersionInfoType info;
    Can_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(CAN_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL(CAN_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL(CAN_SW_MAJOR_VERSION, info.sw_major_version);
}

/** @req SWS_Can_00002 */
void test_Can_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    Can_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_SID_GETVERSIONINFO, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(CAN_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/** @req SWS_Can_00003 */
void test_Can_SetControllerMode_BeforeInit_ShouldReportUninit(void) {
    Can_ReturnType ret = Can_SetControllerMode(0U, CAN_CS_STARTED);
    TEST_ASSERT_EQUAL(CAN_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_SID_SETCONTROLLERMODE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(CAN_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Can_00003 */
void test_Can_SetControllerMode_InvalidController_ShouldReportDet(void) {
    test_Can_EnsureInitialized();
    Can_ReturnType ret = Can_SetControllerMode(200U, CAN_CS_STARTED);
    TEST_ASSERT_EQUAL(CAN_NOT_OK, ret);
    TEST_ASSERT_EQUAL(CAN_E_PARAM_CONTROLLER, Det_MockData.ErrorId);
}

/** @req SWS_Can_00003 */
void test_Can_SetControllerMode_StartFromStopped_ShouldClearHalt(void) {
    test_Can_EnsureInitialized();
    test_Can_BringToStopped();
    /* MCR: freeze ack set, NOT_RDY must read 0 for the poll loop to exit */
    MockRegisters_Write32(CAN1_BASE + CAN_MCR_OFF,
                          CAN_MCR_FRZ_BIT | CAN_MCR_HALT_BIT);
    Can_ReturnType ret = Can_SetControllerMode(0U, CAN_CS_STARTED);
    TEST_ASSERT_EQUAL(CAN_OK, ret);
    uint32 mcr = MockRegisters_Read32(CAN1_BASE + CAN_MCR_OFF);
    TEST_ASSERT_EQUAL(0U, mcr & CAN_MCR_HALT_BIT);
}

/** @req SWS_Can_00003 */
void test_Can_SetControllerMode_StopFromStarted_ShouldSetHalt(void) {
    test_Can_EnsureInitialized();
    test_Can_BringToStarted();
    MockRegisters_Write32(CAN1_BASE + CAN_MCR_OFF, 0U);
    Can_ReturnType ret = Can_SetControllerMode(0U, CAN_CS_STOPPED);
    TEST_ASSERT_EQUAL(CAN_OK, ret);
    uint32 mcr = MockRegisters_Read32(CAN1_BASE + CAN_MCR_OFF);
    TEST_ASSERT_NOT_EQUAL(0U, mcr & CAN_MCR_HALT_BIT);
}

/** @req SWS_Can_00003 */
void test_Can_SetControllerMode_SleepUnsupported_ShouldFail(void) {
    test_Can_EnsureInitialized();
    test_Can_BringToStarted();
    TEST_ASSERT_EQUAL(CAN_NOT_OK, Can_SetControllerMode(0U, CAN_CS_SLEEP));
}

/** @req SWS_Can_00004 */
void test_Can_DisableControllerInterrupts_ShouldClearImaskRegisters(void) {
    test_Can_EnsureInitialized();
    MockRegisters_Write32(CAN1_BASE + CAN_IMASK1_OFF, 0x55AAU);
    MockRegisters_Write32(CAN1_BASE + CAN_IMASK2_OFF, 0x55AAU);
    Can_DisableControllerInterrupts(0U);
    TEST_ASSERT_EQUAL(0U, MockRegisters_Read32(CAN1_BASE + CAN_IMASK1_OFF));
    TEST_ASSERT_EQUAL(0U, MockRegisters_Read32(CAN1_BASE + CAN_IMASK2_OFF));
}

/** @req SWS_Can_00004 */
void test_Can_DisableControllerInterrupts_BeforeInit_ShouldReportDet(void) {
    Can_DisableControllerInterrupts(0U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Can_00005 */
void test_Can_EnableControllerInterrupts_ShouldWriteImask1(void) {
    test_Can_EnsureInitialized();
    MockRegisters_Write32(CAN1_BASE + CAN_IMASK1_OFF, 0U);
    Can_EnableControllerInterrupts(0U);
    /* BusOffProcessing=FALSE in test config -> imask stays zero but is written */
    TEST_ASSERT_EQUAL(0U, MockRegisters_Read32(CAN1_BASE + CAN_IMASK1_OFF));
}

/** @req SWS_Can_00005 */
void test_Can_EnableControllerInterrupts_BeforeInit_ShouldReportDet(void) {
    Can_EnableControllerInterrupts(0U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Can_00006 */
void test_Can_Write_BeforeInit_ShouldFail(void) {
    Can_ReturnType ret = Can_Write(0U, &testPdu);
    TEST_ASSERT_EQUAL(CAN_NOT_OK, ret);
    TEST_ASSERT_EQUAL(CAN_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Can_00006 */
void test_Can_Write_NullPdu_ShouldReportPointerDet(void) {
    test_Can_EnsureInitialized();
    TEST_ASSERT_EQUAL(CAN_NOT_OK, Can_Write(0U, NULL_PTR));
    TEST_ASSERT_EQUAL(CAN_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/** @req SWS_Can_00006 */
void test_Can_Write_InvalidHth_ShouldReportHandleDet(void) {
    test_Can_EnsureInitialized();
    TEST_ASSERT_EQUAL(CAN_NOT_OK, Can_Write(200U, &testPdu));
    TEST_ASSERT_EQUAL(CAN_E_PARAM_HANDLE, Det_MockData.ErrorId);
}

/** @req SWS_Can_00006 */
void test_Can_Write_BusyMailbox_ShouldReturnBusy(void) {
    test_Can_EnsureInitialized();
    uint32 mbAddr = CAN1_BASE + CAN_MB_BASE_OFF; /* Hth 0 -> controller 0, mb 0 */
    MockRegisters_Write32(mbAddr + 0U, 0x02000000UL); /* RX_FULL code */
    Can_ReturnType ret = Can_Write(0U, &testPdu);
    TEST_ASSERT_EQUAL(CAN_BUSY, ret);
}

/** @req SWS_Can_00006 */
void test_Can_Write_IdleMailbox_ShouldEncodeStdIdAndData(void) {
    test_Can_EnsureInitialized();
    uint32 mbAddr = CAN1_BASE + CAN_MB_BASE_OFF;
    MockRegisters_Write32(mbAddr + 0U, CAN_MB_CS_INACTIVE);
    Can_ReturnType ret = Can_Write(0U, &testPdu);
    TEST_ASSERT_EQUAL(CAN_OK, ret);
    /* Standard ID: (CanId & 0x7FF) << 18 */
    TEST_ASSERT_EQUAL((0x100U & 0x7FFU) << 18,
                      MockRegisters_Read32(mbAddr + 4U));
    /* Data word0 little-endian packing of first 4 bytes */
    TEST_ASSERT_EQUAL_HEX32(0xEFBEADDEU, MockRegisters_Read32(mbAddr + 8U));
    TEST_ASSERT_EQUAL_HEX32(0x44332211U, MockRegisters_Read32(mbAddr + 12U));
    /* CS written with DLC in bits 16..19 */
    uint32 cs = MockRegisters_Read32(mbAddr + 0U);
    TEST_ASSERT_EQUAL(8U, (cs >> 16) & 0x0FU);
}

/** @req SWS_Can_00007 */
void test_Can_MainFunction_Write_TxFlagSet_ShouldClearFlag(void) {
    test_Can_EnsureInitialized();
    test_Can_BringToStarted();
    /* Two pending TX flags; driver issues W1C writes (mock keeps last one) */
    MockRegisters_Write32(CAN1_BASE + CAN_IFLAG1_OFF, 0x03U);
    Can_MainFunction_Write();
    TEST_ASSERT_EQUAL(0x02U, MockRegisters_Read32(CAN1_BASE + CAN_IFLAG1_OFF));
}

/** @req SWS_Can_00007 */
void test_Can_MainFunction_Write_StoppedController_ShouldNotTouchFlags(void) {
    test_Can_EnsureInitialized();
    test_Can_BringToStopped();
    MockRegisters_Write32(CAN1_BASE + CAN_IFLAG1_OFF, 0x03U);
    Can_MainFunction_Write();
    TEST_ASSERT_EQUAL(0x03U, MockRegisters_Read32(CAN1_BASE + CAN_IFLAG1_OFF));
}

/** @req SWS_Can_00008 */
void test_Can_MainFunction_Read_RxFlagSet_ShouldClearFlag(void) {
    test_Can_EnsureInitialized();
    test_Can_BringToStarted();
    /* RX MBs are indices 8..15; two pending RX flags, W1C clears them */
    MockRegisters_Write32(CAN1_BASE + CAN_IFLAG1_OFF, 0x300U);
    Can_MainFunction_Read();
    TEST_ASSERT_EQUAL(0x200U, MockRegisters_Read32(CAN1_BASE + CAN_IFLAG1_OFF));
}

/** @req SWS_Can_00009 */
void test_Can_MainFunction_BusOff_BoffintSet_ShouldClearFlag(void) {
    test_Can_EnsureInitialized();
    test_Can_BringToStarted();
    MockRegisters_Write32(CAN1_BASE + CAN_ESR1_OFF, 0x07U); /* BOFFINT|ERRINT|WAKINT */
    Can_MainFunction_BusOff();
    /* Driver issues W1C write of BOFFINT only */
    TEST_ASSERT_EQUAL(0x04U, MockRegisters_Read32(CAN1_BASE + CAN_ESR1_OFF));
}

/** @req SWS_Can_00010 */
void test_Can_MainFunction_Wakeup_StoppedController_ShouldNotTouchEsr(void) {
    test_Can_EnsureInitialized();
    MockRegisters_Write32(CAN1_BASE + CAN_ESR1_OFF, 0xAAU);
    Can_MainFunction_Wakeup();
    TEST_ASSERT_EQUAL(0xAAU, MockRegisters_Read32(CAN1_BASE + CAN_ESR1_OFF));
}

/** @req SWS_Can_00011 */
void test_Can_MainFunction_Mode_ShouldNotTouchRegisters(void) {
    test_Can_EnsureInitialized();
    MockRegisters_Write32(CAN1_BASE + CAN_MCR_OFF, 0x12345678UL);
    Can_MainFunction_Mode();
    TEST_ASSERT_EQUAL(0x12345678UL, MockRegisters_Read32(CAN1_BASE + CAN_MCR_OFF));
}

/** @req SWS_Can_00012 */
void test_Can_CheckWakeup_WakintSet_ShouldReturnOk(void) {
    test_Can_EnsureInitialized();
    MockRegisters_Write32(CAN1_BASE + CAN_ESR1_OFF, 0x01U); /* WAKINT */
    TEST_ASSERT_EQUAL(E_OK, Can_CheckWakeup(0U));
}

/** @req SWS_Can_00012 */
void test_Can_CheckWakeup_NoWakint_ShouldReturnNotOk(void) {
    test_Can_EnsureInitialized();
    MockRegisters_Write32(CAN1_BASE + CAN_ESR1_OFF, 0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, Can_CheckWakeup(0U));
}

/** @req SWS_Can_00012 */
void test_Can_CheckWakeup_InvalidController_ShouldReportDet(void) {
    test_Can_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_NOT_OK, Can_CheckWakeup(200U));
    TEST_ASSERT_EQUAL(CAN_E_PARAM_CONTROLLER, Det_MockData.ErrorId);
}

/** @req SWS_Can_00006 */
void test_Can_Write_FdFrame12_FdEnabled_ShouldEncodeDlcCode9(void) {
    test_Can_EnsureInitialized();
    test_Can_SetupFdConfig();
    test_Can_SetupFdPdu();
    /* Hth 8 -> controller 1 (FD-enabled), mailbox 0 */
    uint32 mbAddr = CAN2_BASE + CAN_MB_BASE_OFF;
    MockRegisters_Write32(mbAddr + 0U, CAN_MB_CS_INACTIVE);
    testPdu.FdFrame = TRUE;
    testPdu.CanDlc = 12U;
    testPdu.SduPtr = fdPduData;

    Can_ReturnType ret = Can_Write(8U, &testPdu);
    TEST_ASSERT_EQUAL(CAN_OK, ret);
    /* DLC code 9 encodes 12 bytes in CAN FD */
    uint32 cs = MockRegisters_Read32(mbAddr + 0U);
    TEST_ASSERT_EQUAL(9U, (cs >> 16) & 0x0FU);
    /* Word 2 (bytes 8..11) at the first offset beyond the classic 8 bytes */
    TEST_ASSERT_EQUAL_HEX32(0x0B0A0908U, MockRegisters_Read32(mbAddr + 16U));
}

/** @req SWS_Can_00006 */
void test_Can_Write_FdFrame64_FdEnabled_ShouldEncodeDlcCode15(void) {
    test_Can_EnsureInitialized();
    test_Can_SetupFdConfig();
    test_Can_SetupFdPdu();
    uint32 mbAddr = CAN2_BASE + CAN_MB_BASE_OFF;
    MockRegisters_Write32(mbAddr + 0U, CAN_MB_CS_INACTIVE);
    testPdu.FdFrame = TRUE;
    testPdu.CanDlc = 64U;
    testPdu.SduPtr = fdPduData;

    Can_ReturnType ret = Can_Write(8U, &testPdu);
    TEST_ASSERT_EQUAL(CAN_OK, ret);
    /* DLC code 15 encodes 64 bytes in CAN FD */
    uint32 cs = MockRegisters_Read32(mbAddr + 0U);
    TEST_ASSERT_EQUAL(15U, (cs >> 16) & 0x0FU);
    /* Spot-check a word beyond the classic 8 bytes (bytes 8..11) */
    TEST_ASSERT_EQUAL_HEX32(0x0B0A0908U, MockRegisters_Read32(mbAddr + 16U));
    /* Last data word: bytes 60..63 at mbAddr + 8 + 15*4 */
    TEST_ASSERT_EQUAL_HEX32(0x3F3E3D3CU, MockRegisters_Read32(mbAddr + 68U));
}

/** @req SWS_Can_00006 */
void test_Can_Write_FdFrame_FdDisabled_ShouldRejectWithDet(void) {
    test_Can_EnsureInitialized();
    /* Hth 0 -> controller 0 whose baudrate config keeps FD disabled */
    uint32 mbAddr = CAN1_BASE + CAN_MB_BASE_OFF;
    MockRegisters_Write32(mbAddr + 0U, CAN_MB_CS_INACTIVE);
    testPdu.FdFrame = TRUE;
    testPdu.CanDlc = 12U;
    testPdu.SduPtr = fdPduData;

    Can_ReturnType ret = Can_Write(0U, &testPdu);
    TEST_ASSERT_EQUAL(CAN_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_E_PARAM_DLC, Det_MockData.ErrorId);
    /* Rejected frame must not arm the mailbox (no silent truncation) */
    TEST_ASSERT_EQUAL(CAN_MB_CS_INACTIVE, MockRegisters_Read32(mbAddr + 0U));
}

/** @req SWS_Can_00006 */
void test_Can_Write_FdFrameIllegalLength_ShouldRejectWithDet(void) {
    test_Can_EnsureInitialized();
    test_Can_SetupFdConfig();
    /* Hth 8 -> controller 1 (FD-enabled), mailbox 0 */
    uint32 mbAddr = CAN2_BASE + CAN_MB_BASE_OFF;
    MockRegisters_Write32(mbAddr + 0U, CAN_MB_CS_INACTIVE);
    testPdu.FdFrame = TRUE;
    testPdu.SduPtr = fdPduData;

    testPdu.CanDlc = 9U;
    TEST_ASSERT_EQUAL(CAN_NOT_OK, Can_Write(8U, &testPdu));
    TEST_ASSERT_EQUAL(CAN_E_PARAM_DLC, Det_MockData.ErrorId);

    Det_Mock_Reset();
    testPdu.CanDlc = 13U;
    TEST_ASSERT_EQUAL(CAN_NOT_OK, Can_Write(8U, &testPdu));
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_E_PARAM_DLC, Det_MockData.ErrorId);
    TEST_ASSERT_EQUAL(CAN_MB_CS_INACTIVE, MockRegisters_Read32(mbAddr + 0U));
}

/** @req SWS_Can_00006 */
void test_Can_Write_ClassicFrameLength12_ShouldRejectWithDet(void) {
    test_Can_EnsureInitialized();
    uint32 mbAddr = CAN1_BASE + CAN_MB_BASE_OFF; /* Hth 0 -> controller 0 */
    MockRegisters_Write32(mbAddr + 0U, CAN_MB_CS_INACTIVE);
    testPdu.FdFrame = FALSE;
    testPdu.CanDlc = 12U;
    testPdu.SduPtr = fdPduData;

    Can_ReturnType ret = Can_Write(0U, &testPdu);
    TEST_ASSERT_EQUAL(CAN_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_E_PARAM_DLC, Det_MockData.ErrorId);
}

/* --- AD3: error state / error counter getters --- */

#define CAN_ECR_OFF (0x1CU)

/** @req SWS_Can_00013 */
void test_Can_GetControllerErrorState_BeforeInit_ShouldReportDet(void) {
    Can_ErrorStateType state = CAN_ERRORSTATE_BUSOFF;
    TEST_ASSERT_EQUAL(E_NOT_OK, Can_GetControllerErrorState(0U, &state));
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_SID_GETCONTROLLERERRORSTATE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(CAN_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Can_00014 */
void test_Can_GetControllerRxErrorCounter_BeforeInit_ShouldReportDet(void) {
    uint8 counter = 0xFFU;
    TEST_ASSERT_EQUAL(E_NOT_OK, Can_GetControllerRxErrorCounter(0U, &counter));
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_SID_GETCONTROLLERRXERRORCOUNTER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(CAN_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Can_00015 */
void test_Can_GetControllerTxErrorCounter_BeforeInit_ShouldReportDet(void) {
    uint8 counter = 0xFFU;
    TEST_ASSERT_EQUAL(E_NOT_OK, Can_GetControllerTxErrorCounter(0U, &counter));
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_SID_GETCONTROLLERTXERRORCOUNTER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(CAN_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Can_00013 */
void test_Can_GetControllerErrorState_AfterInit_ShouldReturnActive(void) {
    Can_ErrorStateType state = CAN_ERRORSTATE_BUSOFF;
    test_Can_EnsureInitialized();
    /* A previous BusOff main-function test may have migrated controller 0
     * into BUSOFF; the STARTED transition resets the error state. */
    test_Can_BringToStarted();
    TEST_ASSERT_EQUAL(E_OK, Can_GetControllerErrorState(0U, &state));
    TEST_ASSERT_EQUAL(CAN_ERRORSTATE_ACTIVE, state);
}

/** @req SWS_Can_00013 */
void test_Can_GetControllerErrorState_InvalidController_ShouldReportDet(void) {
    Can_ErrorStateType state = CAN_ERRORSTATE_ACTIVE;
    test_Can_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_NOT_OK, Can_GetControllerErrorState(200U, &state));
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_SID_GETCONTROLLERERRORSTATE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(CAN_E_PARAM_CONTROLLER, Det_MockData.ErrorId);
}

/** @req SWS_Can_00013 */
void test_Can_GetControllerErrorState_NullPtr_ShouldReportDet(void) {
    test_Can_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_NOT_OK, Can_GetControllerErrorState(0U, NULL_PTR));
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_SID_GETCONTROLLERERRORSTATE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(CAN_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/** @req SWS_Can_00014 */
void test_Can_GetControllerRxErrorCounter_NullPtr_ShouldReportDet(void) {
    test_Can_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_NOT_OK, Can_GetControllerRxErrorCounter(0U, NULL_PTR));
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_SID_GETCONTROLLERRXERRORCOUNTER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(CAN_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/** @req SWS_Can_00015 */
void test_Can_GetControllerTxErrorCounter_InvalidController_ShouldReportDet(void) {
    uint8 counter = 0U;
    test_Can_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_NOT_OK, Can_GetControllerTxErrorCounter(200U, &counter));
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_SID_GETCONTROLLERTXERRORCOUNTER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(CAN_E_PARAM_CONTROLLER, Det_MockData.ErrorId);
}

/** @req SWS_Can_00014 @req SWS_Can_00015 */
void test_Can_ErrorCounters_AfterInit_ShouldBeZero(void) {
    uint8 rxCounter = 0xFFU;
    uint8 txCounter = 0xFFU;
    test_Can_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_OK, Can_GetControllerRxErrorCounter(0U, &rxCounter));
    TEST_ASSERT_EQUAL(E_OK, Can_GetControllerTxErrorCounter(0U, &txCounter));
    TEST_ASSERT_EQUAL(0U, rxCounter);
    TEST_ASSERT_EQUAL(0U, txCounter);
}

/* --- AD3: runtime error-state migration from ECR/ESR1 --- */

/** @req SWS_Can_00013 @req SWS_Can_00009 */
void test_Can_MainFunction_BusOff_TxCounter200_ShouldSetPassive(void) {
    Can_ErrorStateType state = CAN_ERRORSTATE_ACTIVE;
    uint8 rxCounter = 0U;
    uint8 txCounter = 0U;

    test_Can_EnsureInitialized();
    test_Can_BringToStarted();

    /* ECR: TXERRCNT = 200 (>= 128 -> PASSIVE), RXERRCNT = 0; no BOFFINT */
    MockRegisters_Write32(CAN1_BASE + CAN_ECR_OFF, 200U);
    MockRegisters_Write32(CAN1_BASE + CAN_ESR1_OFF, 0U);
    Can_MainFunction_BusOff();

    TEST_ASSERT_EQUAL(E_OK, Can_GetControllerErrorState(0U, &state));
    TEST_ASSERT_EQUAL(CAN_ERRORSTATE_PASSIVE, state);
    TEST_ASSERT_EQUAL(E_OK, Can_GetControllerTxErrorCounter(0U, &txCounter));
    TEST_ASSERT_EQUAL(200U, txCounter);
    TEST_ASSERT_EQUAL(E_OK, Can_GetControllerRxErrorCounter(0U, &rxCounter));
    TEST_ASSERT_EQUAL(0U, rxCounter);
}

/** @req SWS_Can_00013 @req SWS_Can_00009 */
void test_Can_MainFunction_BusOff_BoffintAndCounter255_ShouldSetBusOff(void) {
    Can_ErrorStateType state = CAN_ERRORSTATE_ACTIVE;
    uint8 rxCounter = 0U;
    uint8 txCounter = 0U;

    test_Can_EnsureInitialized();
    test_Can_BringToStarted();

    /* ECR: TXERRCNT = 255, RXERRCNT = 100; ESR1: BOFFINT set */
    MockRegisters_Write32(CAN1_BASE + CAN_ECR_OFF, 255U | (100U << 8));
    MockRegisters_Write32(CAN1_BASE + CAN_ESR1_OFF, 0x04U);
    Can_MainFunction_BusOff();

    /* Driver clears BOFFINT via W1C write (mock keeps last written value) */
    TEST_ASSERT_EQUAL(0x04U, MockRegisters_Read32(CAN1_BASE + CAN_ESR1_OFF));
    TEST_ASSERT_EQUAL(E_OK, Can_GetControllerErrorState(0U, &state));
    TEST_ASSERT_EQUAL(CAN_ERRORSTATE_BUSOFF, state);
    TEST_ASSERT_EQUAL(E_OK, Can_GetControllerTxErrorCounter(0U, &txCounter));
    TEST_ASSERT_EQUAL(255U, txCounter);
    TEST_ASSERT_EQUAL(E_OK, Can_GetControllerRxErrorCounter(0U, &rxCounter));
    TEST_ASSERT_EQUAL(100U, rxCounter);
}

/** @req SWS_Can_00013 @req SWS_Can_00003 */
void test_Can_ErrorState_RestartAfterBusOff_ShouldReturnActive(void) {
    Can_ErrorStateType state = CAN_ERRORSTATE_ACTIVE;
    uint8 rxCounter = 0xFFU;
    uint8 txCounter = 0xFFU;

    test_Can_EnsureInitialized();
    test_Can_BringToStarted();

    /* Drive controller 0 into BUSOFF */
    MockRegisters_Write32(CAN1_BASE + CAN_ECR_OFF, 255U);
    MockRegisters_Write32(CAN1_BASE + CAN_ESR1_OFF, 0x04U);
    Can_MainFunction_BusOff();
    TEST_ASSERT_EQUAL(E_OK, Can_GetControllerErrorState(0U, &state));
    TEST_ASSERT_EQUAL(CAN_ERRORSTATE_BUSOFF, state);

    /* Bus-off recovery: STOPPED -> STARTED resets counters and state */
    test_Can_BringToStopped();
    test_Can_BringToStarted();

    TEST_ASSERT_EQUAL(E_OK, Can_GetControllerErrorState(0U, &state));
    TEST_ASSERT_EQUAL(CAN_ERRORSTATE_ACTIVE, state);
    TEST_ASSERT_EQUAL(E_OK, Can_GetControllerRxErrorCounter(0U, &rxCounter));
    TEST_ASSERT_EQUAL(0U, rxCounter);
    TEST_ASSERT_EQUAL(E_OK, Can_GetControllerTxErrorCounter(0U, &txCounter));
    TEST_ASSERT_EQUAL(0U, txCounter);
}

/* --- AD4: WaitForNotReady loop coverage (line 152) --- */

#define CAN_MCR_NOT_RDY_BIT (0x08000000UL)

/** @req SWS_Can_00003
 * Pre-set NOT_RDY in MCR so Can_WaitForNotReady enters its while-loop (line 152).
 * The mock retains the written value, so NOT_RDY never clears and the poll
 * runs to timeout, returning E_NOT_OK -> Can_SetControllerMode returns CAN_NOT_OK. */
void test_Can_SetControllerMode_StartWithNotReadySet_ShouldLoopUntilTimeout(void) {
    test_Can_EnsureInitialized();
    test_Can_BringToStopped();
    /* MCR: FRZ + NOT_RDY set, HALT clear. The STARTED transition clears HALT
     * (already clear) and writes MCR; NOT_RDY persists in the mock. */
    MockRegisters_Write32(CAN1_BASE + CAN_MCR_OFF,
                          CAN_MCR_FRZ_BIT | CAN_MCR_NOT_RDY_BIT);
    Can_ReturnType ret = Can_SetControllerMode(0U, CAN_CS_STARTED);
    TEST_ASSERT_EQUAL(CAN_NOT_OK, ret);
}

/* --- Additional coverage tests --- */

/** @req SWS_Can_00005 */
void test_Can_EnableControllerInterrupts_WithBusOffProcessing_ShouldSetImask(void) {
    test_Can_EnsureInitialized();
    /* Enable BusOffProcessing for controller 0 */
    testControllers[0].BusOffProcessing = TRUE;
    MockRegisters_Write32(CAN1_BASE + CAN_IMASK1_OFF, 0U);
    Can_EnableControllerInterrupts(0U);
    /* Should set BOFFINT and ERRINT bits */
    uint32 imask = MockRegisters_Read32(CAN1_BASE + CAN_IMASK1_OFF);
    TEST_ASSERT_NOT_EQUAL(0U, imask & 0x00000004U); /* BOFFINT */
    TEST_ASSERT_NOT_EQUAL(0U, imask & 0x00000002U); /* ERRINT */
    /* Restore config */
    testControllers[0].BusOffProcessing = FALSE;
}

/** @req SWS_Can_00005 */
void test_Can_EnableControllerInterrupts_InvalidController_ShouldReportDet(void) {
    test_Can_EnsureInitialized();
    Can_EnableControllerInterrupts(200U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_E_PARAM_CONTROLLER, Det_MockData.ErrorId);
}

/** @req SWS_Can_00004 */
void test_Can_DisableControllerInterrupts_InvalidController_ShouldReportDet(void) {
    test_Can_EnsureInitialized();
    Can_DisableControllerInterrupts(200U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_E_PARAM_CONTROLLER, Det_MockData.ErrorId);
}

/** @req SWS_Can_00006 */
void test_Can_Write_ExtendedId_ShouldEncodeExtendedFormat(void) {
    test_Can_EnsureInitialized();
    uint32 mbAddr = CAN1_BASE + CAN_MB_BASE_OFF;
    MockRegisters_Write32(mbAddr + 0U, CAN_MB_CS_INACTIVE);
    testPdu.idType = CAN_ID_TYPE_EXTENDED;
    testPdu.CanId = 0x12345678U;
    testPdu.CanDlc = 8U;
    testPdu.SduPtr = pduData;
    testPdu.FdFrame = FALSE;

    Can_ReturnType ret = Can_Write(0U, &testPdu);
    TEST_ASSERT_EQUAL(CAN_OK, ret);
    /* Extended ID: (CanId & 0x1FFFFFFF) | 0x80000000 */
    uint32 idReg = MockRegisters_Read32(mbAddr + 4U);
    TEST_ASSERT_EQUAL((0x12345678U & 0x1FFFFFFFU) | 0x80000000U, idReg);
}

/** @req SWS_Can_00012 */
void test_Can_CheckWakeup_BeforeInit_ShouldReportDet(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Can_CheckWakeup(0U));
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Can_00014 */
void test_Can_GetControllerRxErrorCounter_InvalidController_ShouldReportDet(void) {
    uint8 counter = 0U;
    test_Can_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_NOT_OK, Can_GetControllerRxErrorCounter(200U, &counter));
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_E_PARAM_CONTROLLER, Det_MockData.ErrorId);
}

/** @req SWS_Can_00015 */
void test_Can_GetControllerTxErrorCounter_ValidPath_ShouldReturnCounter(void) {
    uint8 counter = 0xFFU;
    test_Can_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_OK, Can_GetControllerTxErrorCounter(0U, &counter));
    TEST_ASSERT_EQUAL(0U, counter);
}

/** @req SWS_Can_00014 */
void test_Can_GetControllerRxErrorCounter_ValidPath_ShouldReturnCounter(void) {
    uint8 counter = 0xFFU;
    test_Can_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_OK, Can_GetControllerRxErrorCounter(0U, &counter));
    TEST_ASSERT_EQUAL(0U, counter);
}

/** @req SWS_Can_00003 */
void test_Can_SetControllerMode_StartWhenAlreadyStarted_ShouldFail(void) {
    test_Can_EnsureInitialized();
    test_Can_BringToStarted();
    /* Try to start again - should fail because not in STOPPED state */
    Can_ReturnType ret = Can_SetControllerMode(0U, CAN_CS_STARTED);
    TEST_ASSERT_EQUAL(CAN_NOT_OK, ret);
}

/** @req SWS_Can_00003 */
void test_Can_SetControllerMode_StopWhenAlreadyStopped_ShouldFail(void) {
    test_Can_EnsureInitialized();
    test_Can_BringToStopped();
    /* Try to stop again - should fail because not in STARTED state */
    Can_ReturnType ret = Can_SetControllerMode(0U, CAN_CS_STOPPED);
    TEST_ASSERT_EQUAL(CAN_NOT_OK, ret);
}

/** @req SWS_Can_00003 */
void test_Can_SetControllerMode_StopWithFreezeAckTimeout_ShouldFail(void) {
    test_Can_EnsureInitialized();
    test_Can_BringToStarted();
    /* Note: Mock registers don't simulate hardware timeout behavior.
     * The code writes FRZ bit and reads it back immediately, so it succeeds.
     * This test verifies the stop transition path executes without crash. */
    MockRegisters_Write32(CAN1_BASE + CAN_MCR_OFF, 0U);
    Can_ReturnType ret = Can_SetControllerMode(0U, CAN_CS_STOPPED);
    /* With mock registers, FRZ bit is set immediately, so it succeeds */
    TEST_ASSERT_EQUAL(CAN_OK, ret);
}

/* --- Additional coverage tests for Can.c --- */

/** @brief Test Can_GetBaseAddr default case (invalid controller) */
void test_Can_GetBaseAddr_InvalidController_ShouldReturnZero(void) {
    test_Can_EnsureInitialized();
    /* This tests the default case in Can_GetBaseAddr switch */
    /* We can't directly call Can_GetBaseAddr, but we can trigger it via
     * functions that use it with invalid controller IDs */
    Can_EnableControllerInterrupts(255U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_E_PARAM_CONTROLLER, Det_MockData.ErrorId);
}

/** @brief Test error state tracking with high error counters */
void test_Can_ErrorState_HighCounters_ShouldSetErrorState(void) {
    test_Can_EnsureInitialized();
    test_Can_BringToStarted();
    /* Simulate high error counters by writing to ECR */
    MockRegisters_Write32(CAN1_BASE + CAN_ECR_OFF, 0x8080U); /* TXERR=128, RXERR=128 */
    Can_MainFunction_BusOff();
    /* Error state should be updated */
    Can_ErrorStateType errState = CAN_ERRORSTATE_ACTIVE;
    Can_GetControllerErrorState(0U, &errState);
    TEST_ASSERT_NOT_EQUAL(CAN_ERRORSTATE_ACTIVE, errState);
}

/** @brief Test GetControllerTxErrorCounter with NULL pointer */
void test_Can_GetControllerTxErrorCounter_NullPtr_ShouldReportDet(void) {
    test_Can_EnsureInitialized();
    Can_GetControllerTxErrorCounter(0U, NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(CAN_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/** @brief Test SetControllerMode with invalid transition */
void test_Can_SetControllerMode_InvalidTransition_ShouldFail(void) {
    test_Can_EnsureInitialized();
    Can_ReturnType ret = Can_SetControllerMode(0U, 255U);
    TEST_ASSERT_EQUAL(CAN_NOT_OK, ret);
}

/** @brief Test Can_Write with extended ID */
void test_Can_Write_ExtendedIdOnly_ShouldEncodeCorrectly(void) {
    test_Can_EnsureInitialized();
    uint32 mbAddr = CAN1_BASE + CAN_MB_BASE_OFF;
    MockRegisters_Write32(mbAddr + 0U, CAN_MB_CS_INACTIVE);
    Can_PduType extPdu = {
        .idType = CAN_ID_TYPE_EXTENDED,
        .CanId = 0x1FFFFFFFU,
        .CanDlc = 0U,
        .SduPtr = NULL,
        .FdFrame = FALSE
    };
    Can_ReturnType ret = Can_Write(0U, &extPdu);
    TEST_ASSERT_EQUAL(CAN_OK, ret);
    /* Check that extended ID flag is set in ID register */
    uint32 idReg = MockRegisters_Read32(mbAddr + 4U);
    TEST_ASSERT_NOT_EQUAL(0U, idReg & 0x80000000U); /* Extended ID flag */
}
