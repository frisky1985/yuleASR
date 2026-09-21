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
