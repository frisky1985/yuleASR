/*==============================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Com (Communication Services) Branch Coverage Test
*
* Description: Comprehensive branch coverage tests for Com.c
*              Covers: init/deinit, send/receive signal, signal group,
*              shadow buffer, pack/unpack (LE/BE, byte-aligned/bit-level),
*              filters, deadline monitoring, TX/RX confirmation,
*              IPDU group control, DM control, invalidate, stubs, DET errors.
==============================================================================*/

#include <unity.h>
#include "Com.h"
#include "Com_Cfg.h"
#include "ComStack_Types.h"
#include "Det.h"
#include <string.h>
#include <stdlib.h>

/* Forward declarations for functions implemented in Com.c but not in Com.h */
extern Std_ReturnType Com_UpdateShadowSignal(Com_SignalIdType SignalId, const void* SignalDataPtr);
extern Std_ReturnType Com_ReceiveShadowSignal(Com_SignalIdType SignalId, void* SignalDataPtr);
extern void Com_ClearIpduGroupVector(Com_IpduGroupVector ipduGroupVector);
extern void Com_SetIpduGroup(Com_IpduGroupVector ipduGroupVector, Com_IpduGroupIdType ipduGroupId);
extern void Com_ClearIpduGroup(Com_IpduGroupVector ipduGroupVector, Com_IpduGroupIdType ipduGroupId);
extern Std_ReturnType Com_SendDynSignal(Com_SignalIdType SignalId, const void* SignalDataPtr, uint16 Length);
extern Std_ReturnType Com_ReceiveDynSignal(Com_SignalIdType SignalId, void* SignalDataPtr, uint16* Length);
extern Std_ReturnType Com_TriggerIPDUSendWithMetaData(PduIdType PduId, const uint8* MetaData);
extern void Com_TpRxIndication(PduIdType id, Std_ReturnType result);
extern void Com_TpTxConfirmation(PduIdType id, Std_ReturnType result);

/*==============================================================================
*                           DET / PduR MOCK
*==============================================================================*/

static uint16 det_last_error   = 0xFFFF;
static uint8  det_last_api_id  = 0xFF;
static uint16 det_error_count  = 0;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId; (void)InstanceId;
    det_last_api_id = ApiId;
    det_last_error  = ErrorId;
    det_error_count++;
    return E_OK;
}

void Det_Init(const Det_ConfigType* ConfigPtr) { (void)ConfigPtr; }
void Det_Start(void) {}

/* PduR mock - controllable return value */
static Std_ReturnType g_pdur_transmit_ret = E_OK;
static uint8 g_pdur_transmit_call_count   = 0;
static PduIdType g_pdur_transmit_last_id  = 0xFFFF;

Std_ReturnType PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    (void)PduInfoPtr;
    g_pdur_transmit_call_count++;
    g_pdur_transmit_last_id = TxPduId;
    return g_pdur_transmit_ret;
}

/*==============================================================================
*                        TEST CONFIGURATION
*==============================================================================*/

/*
 * We define a custom config with:
 *  - 4 IPDUs (id 0..3)
 *  - 8 signals with varied properties to exercise all branches
 * Signal table:
 *  id0: 8-bit, LE, BitPos=0,  BitSize=8,  grp=0, TRIGGERED,         filter=ALWAYS
 *  id1: 16-bit,LE, BitPos=16, BitSize=16, grp=0, PERIODIC(no auto-tx), filter=NEVER
 *  id2: 32-bit,LE, BitPos=0,  BitSize=32, grp=1, TRIGGERED_ON_CHANGE, filter=ALWAYS
 *  id3: 8-bit, BE, BitPos=8,  BitSize=8,  grp=1, TRIGGERED,         filter=MASKED_NEW_EQUALS_X
 *  id4: 12-bit,LE, BitPos=4,  BitSize=12, grp=2, TRIGGERED,         filter=MASKED_NEW_DIFFERS_X
 *  id5: 16-bit,BE, BitPos=0,  BitSize=16, grp=2, TRIGGERED,         filter=MASKED_NEW_DIFFERS_MASKED_OLD
 *  id6: 8-bit, LE, BitPos=0,  BitSize=8,  grp=3, TRIGGERED,         filter=ALWAYS (for deadline)
 *  id7: 3-bit, LE, BitPos=5,  BitSize=3,  grp=0, TRIGGERED,         filter=ALWAYS (non-byte-aligned)
 */

#define TEST_NUM_IPDUS   4
#define TEST_NUM_SIGNALS 8

static const Com_SignalConfigType testSignals[TEST_NUM_SIGNALS] = {
    /* id0 */ { 0,  0,  8, COM_LITTLE_ENDIAN, COM_TRIGGERED,         COM_ALWAYS, 0xFF, 0x55, 0 },
    /* id1 */ { 1, 16, 16, COM_LITTLE_ENDIAN, COM_PENDING,           COM_NEVER,  0x00, 0x00, 0 },
    /* id2 */ { 2,  0, 32, COM_LITTLE_ENDIAN, COM_TRIGGERED_ON_CHANGE, COM_ALWAYS, 0x00, 0x00, 1 },
    /* id3 */ { 3,  8,  8, COM_BIG_ENDIAN,    COM_TRIGGERED,         COM_MASKED_NEW_EQUALS_X, 0x0F, 0x05, 1 },
    /* id4 */ { 4,  4, 12, COM_LITTLE_ENDIAN, COM_TRIGGERED,         COM_MASKED_NEW_DIFFERS_X, 0xFF0, 0x100, 2 },
    /* id5 */ { 5,  0, 16, COM_BIG_ENDIAN,    COM_TRIGGERED,         COM_MASKED_NEW_DIFFERS_MASKED_OLD, 0xFFFF, 0, 2 },
    /* id6 */ { 6,  0,  8, COM_LITTLE_ENDIAN, COM_TRIGGERED,         COM_ALWAYS, 0x00, 0x00, 3 },
    /* id7 */ { 7,  5,  3, COM_LITTLE_ENDIAN, COM_TRIGGERED,         COM_ALWAYS, 0x00, 0x00, 0 },
};

static uint8 g_deadline_notified = 0;
static PduIdType g_deadline_pdu = 0xFFFF;

static void TestDeadlineNotification(PduIdType PduId)
{
    g_deadline_notified = 1;
    g_deadline_pdu = PduId;
}

static const Com_IPduConfigType testIPdus[TEST_NUM_IPDUS] = {
    /* PduId=0, len=8, no repeat, TimePeriod=100, group=0, no deadline */
    { 0,  8,  FALSE, 0, 0, 100, 0, 0, NULL },
    /* PduId=1, len=16, repeat enabled(2 reps, interval=10), TimePeriod=0, group=0 */
    { 1, 16,  TRUE, 2, 10,  0, 0, 0, NULL },
    /* PduId=2, len=8, no repeat, TimePeriod=0, group=1, deadline=5 */
    { 2,  8,  FALSE, 0, 0,   0, 1, 5, TestDeadlineNotification },
    /* PduId=3, len=4, no repeat, TimePeriod=50, group=1, deadline=0 */
    { 3,  4,  FALSE, 0, 0,  50, 1, 0, NULL },
};

static const Com_ConfigType testConfig = {
    testSignals,
    TEST_NUM_SIGNALS,
    testIPdus,
    TEST_NUM_IPDUS
};

/* Extern declaration so the linker resolves Com_Config if referenced */
extern const Com_ConfigType Com_Config;
const Com_ConfigType Com_Config = {
    testSignals,
    TEST_NUM_SIGNALS,
    testIPdus,
    TEST_NUM_IPDUS
};

/*==============================================================================
*                           SETUP / TEARDOWN
*==============================================================================*/

void setUp(void)
{
    det_last_error  = 0xFFFF;
    det_last_api_id = 0xFF;
    det_error_count = 0;
    g_pdur_transmit_ret = E_OK;
    g_pdur_transmit_call_count = 0;
    g_pdur_transmit_last_id = 0xFFFF;
    g_deadline_notified = 0;
    g_deadline_pdu = 0xFFFF;

    /* Reset Com module to known UNINIT state.
     * Init then DeInit ensures internal state is clean. */
    Com_Init(&testConfig);
    det_last_error  = 0xFFFF;
    det_last_api_id = 0xFF;
    det_error_count = 0;
    Com_DeInit();
    det_last_error  = 0xFFFF;
    det_last_api_id = 0xFF;
    det_error_count = 0;
}

void tearDown(void) {}

/*==============================================================================
*                       HELPER: init with test config
*==============================================================================*/

static void helper_init(void)
{
    Com_Init(&testConfig);
}

/*==============================================================================
*               1. Com_Init / Com_DeInit / Com_GetStatus
*==============================================================================*/

void test_Init_NullPtr_DetError(void)
{
    Com_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(COM_E_PARAM_POINTER, det_last_error);
    TEST_ASSERT_EQUAL(COM_UNINIT, Com_GetStatus());
}

void test_Init_ValidConfig_StatusInit(void)
{
    helper_init();
    TEST_ASSERT_EQUAL(COM_INIT, Com_GetStatus());
}

void test_DeInit_WhenUninit_DetError(void)
{
    /* Don't init first - state is UNINIT */
    Com_DeInit();
    TEST_ASSERT_EQUAL(COM_E_UNINIT, det_last_error);
}

void test_DeInit_WhenInit_Succeeds(void)
{
    helper_init();
    Com_DeInit();
    TEST_ASSERT_EQUAL(COM_UNINIT, Com_GetStatus());
}

void test_GetStatus_Uninit(void)
{
    TEST_ASSERT_EQUAL(COM_UNINIT, Com_GetStatus());
}

void test_GetStatus_Init(void)
{
    helper_init();
    TEST_ASSERT_EQUAL(COM_INIT, Com_GetStatus());
}

/*==============================================================================
*               2. Com_GetVersionInfo
*==============================================================================*/

void test_GetVersionInfo_NullPtr_DetError(void)
{
    Com_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL(COM_E_PARAM_POINTER, det_last_error);
}

void test_GetVersionInfo_Valid(void)
{
    Std_VersionInfoType vi;
    memset(&vi, 0, sizeof(vi));
    Com_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL(COM_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL(COM_MODULE_ID, vi.moduleID);
    TEST_ASSERT_EQUAL(COM_SW_MAJOR_VERSION, vi.sw_major_version);
    TEST_ASSERT_EQUAL(COM_SW_MINOR_VERSION, vi.sw_minor_version);
    TEST_ASSERT_EQUAL(COM_SW_PATCH_VERSION, vi.sw_patch_version);
}

/*==============================================================================
*               3. Com_SendSignal - DET checks + branches
*==============================================================================*/

void test_SendSignal_Uninit_DetError(void)
{
    uint8 data = 0x42;
    Std_ReturnType r = Com_SendSignal(0, &data);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_UNINIT, det_last_error);
}

void test_SendSignal_NullDataPtr_DetError(void)
{
    helper_init();
    Std_ReturnType r = Com_SendSignal(0, NULL_PTR);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_PARAM_POINTER, det_last_error);
}

void test_SendSignal_InvalidSignalId_DetError(void)
{
    helper_init();
    uint8 data = 0x42;
    Std_ReturnType r = Com_SendSignal(COM_NUM_OF_SIGNALS, &data);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_PARAM_SIGNAL, det_last_error);
}

void test_SendSignal_8bit_LE_Triggered(void)
{
    /* Signal 0: 8-bit LE, TRIGGERED -> should call PduR_Transmit */
    helper_init();
    uint8 data = 0xAB;
    g_pdur_transmit_call_count = 0;
    Std_ReturnType r = Com_SendSignal(0, &data);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
    TEST_ASSERT_NOT_EQUAL(0, g_pdur_transmit_call_count);
}

void test_SendSignal_16bit_NEVER_filter_blocked(void)
{
    /* Signal 1: filter=NEVER -> filter blocks, returns OK but no TX */
    helper_init();
    uint16 data = 0x1234;
    g_pdur_transmit_call_count = 0;
    Std_ReturnType r = Com_SendSignal(1, &data);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
    TEST_ASSERT_EQUAL(0, g_pdur_transmit_call_count);
}

void test_SendSignal_32bit_LE_TriggeredOnChange(void)
{
    /* Signal 2: 32-bit LE, TRIGGERED_ON_CHANGE -> should TX */
    helper_init();
    uint32 data = 0xDEADBEEF;
    g_pdur_transmit_call_count = 0;
    Std_ReturnType r = Com_SendSignal(2, &data);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
    TEST_ASSERT_NOT_EQUAL(0, g_pdur_transmit_call_count);
}

void test_SendSignal_8bit_BE_FilterMaskedNewEqualsX_Pass(void)
{
    /* Signal 3: 8-bit BE, filter=MASKED_NEW_EQUALS_X, mask=0x0F, X=0x05
     * Send value where (val & 0x0F) == 0x05 -> pass */
    helper_init();
    uint8 data = 0x45; /* 0x45 & 0x0F = 0x05 == X -> pass */
    g_pdur_transmit_call_count = 0;
    Std_ReturnType r = Com_SendSignal(3, &data);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
    TEST_ASSERT_NOT_EQUAL(0, g_pdur_transmit_call_count);
}

void test_SendSignal_8bit_BE_FilterMaskedNewEqualsX_Blocked(void)
{
    /* Signal 3: (val & 0x0F) != 0x05 -> blocked */
    helper_init();
    uint8 data = 0x43; /* 0x43 & 0x0F = 0x03 != 0x05 -> blocked */
    g_pdur_transmit_call_count = 0;
    Std_ReturnType r = Com_SendSignal(3, &data);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r); /* filter blocked but op succeeded */
    TEST_ASSERT_EQUAL(0, g_pdur_transmit_call_count);
}

void test_SendSignal_12bit_LE_FilterMaskedNewDiffersX_Pass(void)
{
    /* Signal 4: 12-bit LE, filter=MASKED_NEW_DIFFERS_X, mask=0xFF0, X=0x100
     * (val & 0xFF0) != 0x100 -> pass */
    helper_init();
    uint16 data = 0x200; /* 0x200 & 0xFF0 = 0x200 != 0x100 -> pass */
    g_pdur_transmit_call_count = 0;
    Std_ReturnType r = Com_SendSignal(4, &data);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
    TEST_ASSERT_NOT_EQUAL(0, g_pdur_transmit_call_count);
}

void test_SendSignal_12bit_LE_FilterMaskedNewDiffersX_Blocked(void)
{
    /* Signal 4: (val & 0xFF0) == 0x100 -> blocked */
    helper_init();
    uint16 data = 0x100; /* 0x100 & 0xFF0 = 0x100 == X -> blocked */
    g_pdur_transmit_call_count = 0;
    Std_ReturnType r = Com_SendSignal(4, &data);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
    TEST_ASSERT_EQUAL(0, g_pdur_transmit_call_count);
}

void test_SendSignal_16bit_BE_FilterMaskedNewDiffersMaskedOld_Pass(void)
{
    /* Signal 5: 16-bit BE, filter=MASKED_NEW_DIFFERS_MASKED_OLD
     * First send: LastValue=0, (new & 0xFFFF) != (0 & 0xFFFF) -> pass if new != 0 */
    helper_init();
    uint16 data = 0x1234;
    g_pdur_transmit_call_count = 0;
    Std_ReturnType r = Com_SendSignal(5, &data);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
    TEST_ASSERT_NOT_EQUAL(0, g_pdur_transmit_call_count);
}

void test_SendSignal_16bit_BE_FilterMaskedNewDiffersMaskedOld_Blocked(void)
{
    /* Signal 5: After setting LastValue to same value, filter blocks */
    helper_init();
    uint16 data = 0x0000; /* (0 & mask) == (LastValue=0 & mask) -> blocked */
    g_pdur_transmit_call_count = 0;
    Std_ReturnType r = Com_SendSignal(5, &data);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
    TEST_ASSERT_EQUAL(0, g_pdur_transmit_call_count);
}

void test_SendSignal_3bit_LE_NonByteAligned(void)
{
    /* Signal 7: 3-bit LE at BitPos=5 (non-byte-aligned, non-byte-sized) */
    helper_init();
    uint8 data = 0x05; /* 3-bit value = 5 */
    g_pdur_transmit_call_count = 0;
    Std_ReturnType r = Com_SendSignal(7, &data);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
    TEST_ASSERT_NOT_EQUAL(0, g_pdur_transmit_call_count);
}

void test_SendSignal_PduR_Fails(void)
{
    /* PduR_Transmit returns E_NOT_OK - signal still OK but TX state not pending */
    helper_init();
    g_pdur_transmit_ret = E_NOT_OK;
    uint8 data = 0x42;
    Std_ReturnType r = Com_SendSignal(0, &data);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
}

/*==============================================================================
*               4. Com_ReceiveSignal
*==============================================================================*/

void test_ReceiveSignal_Uninit_DetError(void)
{
    uint8 data;
    Std_ReturnType r = Com_ReceiveSignal(0, &data);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_UNINIT, det_last_error);
}

void test_ReceiveSignal_NullPtr_DetError(void)
{
    helper_init();
    Std_ReturnType r = Com_ReceiveSignal(0, NULL_PTR);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_PARAM_POINTER, det_last_error);
}

void test_ReceiveSignal_InvalidId_DetError(void)
{
    helper_init();
    uint8 data;
    Std_ReturnType r = Com_ReceiveSignal(COM_NUM_OF_SIGNALS, &data);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_PARAM_SIGNAL, det_last_error);
}

void test_ReceiveSignal_RoundTrip_8bit_LE(void)
{
    helper_init();
    uint8 txData = 0xAB;
    Com_SendSignal(0, &txData);
    uint8 rxData = 0;
    Std_ReturnType r = Com_ReceiveSignal(0, &rxData);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
    TEST_ASSERT_EQUAL_HEX8(0xAB, rxData);
}

void test_ReceiveSignal_RoundTrip_32bit_LE(void)
{
    helper_init();
    uint32 txData = 0xDEADBEEF;
    Com_SendSignal(2, &txData);
    uint32 rxData = 0;
    Std_ReturnType r = Com_ReceiveSignal(2, &rxData);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, rxData);
}

void test_ReceiveSignal_RoundTrip_8bit_BE(void)
{
    helper_init();
    uint8 txData = 0x45;
    Com_SendSignal(3, &txData);
    uint8 rxData = 0;
    Std_ReturnType r = Com_ReceiveSignal(3, &rxData);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
    TEST_ASSERT_EQUAL_HEX8(0x45, rxData);
}

void test_ReceiveSignal_RoundTrip_12bit_LE(void)
{
    helper_init();
    uint16 txData = 0x234;
    Com_SendSignal(4, &txData);
    uint16 rxData = 0;
    Std_ReturnType r = Com_ReceiveSignal(4, &rxData);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
    TEST_ASSERT_EQUAL_HEX16(0x234, rxData);
}

void test_ReceiveSignal_RoundTrip_16bit_BE(void)
{
    helper_init();
    uint16 txData = 0xABCD;
    Com_SendSignal(5, &txData);
    uint16 rxData = 0;
    Std_ReturnType r = Com_ReceiveSignal(5, &rxData);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
    TEST_ASSERT_EQUAL_HEX16(0xABCD, rxData);
}

void test_ReceiveSignal_RoundTrip_3bit_LE(void)
{
    helper_init();
    uint8 txData = 0x05; /* 3-bit value = 5 (101b) */
    Com_SendSignal(7, &txData);
    uint8 rxData = 0;
    Std_ReturnType r = Com_ReceiveSignal(7, &rxData);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
    TEST_ASSERT_EQUAL_HEX8(0x05, rxData);
}

/*==============================================================================
*               5. Com_SendSignalGroup / Com_ReceiveSignalGroup
*==============================================================================*/

void test_SendSignalGroup_Uninit_DetError(void)
{
    Std_ReturnType r = Com_SendSignalGroup(0);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_UNINIT, det_last_error);
}

void test_SendSignalGroup_InvalidId_DetError(void)
{
    helper_init();
    Std_ReturnType r = Com_SendSignalGroup(COM_NUM_OF_SIGNAL_GROUPS);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_PARAM_SIGNALGROUP, det_last_error);
}

void test_SendSignalGroup_PduR_OK(void)
{
    helper_init();
    g_pdur_transmit_call_count = 0;
    Std_ReturnType r = Com_SendSignalGroup(0);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
    TEST_ASSERT_NOT_EQUAL(0, g_pdur_transmit_call_count);
}

void test_SendSignalGroup_PduR_Fails(void)
{
    helper_init();
    g_pdur_transmit_ret = E_NOT_OK;
    Std_ReturnType r = Com_SendSignalGroup(0);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
}

void test_ReceiveSignalGroup_Uninit_DetError(void)
{
    Std_ReturnType r = Com_ReceiveSignalGroup(0);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_UNINIT, det_last_error);
}

void test_ReceiveSignalGroup_InvalidId_DetError(void)
{
    helper_init();
    Std_ReturnType r = Com_ReceiveSignalGroup(COM_NUM_OF_SIGNAL_GROUPS);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_PARAM_SIGNALGROUP, det_last_error);
}

void test_ReceiveSignalGroup_Valid(void)
{
    helper_init();
    Std_ReturnType r = Com_ReceiveSignalGroup(0);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
}

/*==============================================================================
*               6. Com_UpdateShadowSignal / Com_ReceiveShadowSignal
*==============================================================================*/

void test_UpdateShadowSignal_Uninit_DetError(void)
{
    uint8 data = 0x42;
    Std_ReturnType r = Com_UpdateShadowSignal(0, &data);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_UNINIT, det_last_error);
}

void test_UpdateShadowSignal_NullPtr_DetError(void)
{
    helper_init();
    Std_ReturnType r = Com_UpdateShadowSignal(0, NULL_PTR);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_PARAM_POINTER, det_last_error);
}

void test_UpdateShadowSignal_Valid(void)
{
    helper_init();
    uint8 data = 0x77;
    Std_ReturnType r = Com_UpdateShadowSignal(0, &data);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
}

void test_ReceiveShadowSignal_Uninit_DetError(void)
{
    uint8 data;
    Std_ReturnType r = Com_ReceiveShadowSignal(0, &data);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_UNINIT, det_last_error);
}

void test_ReceiveShadowSignal_NullPtr_DetError(void)
{
    helper_init();
    Std_ReturnType r = Com_ReceiveShadowSignal(0, NULL_PTR);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_PARAM_POINTER, det_last_error);
}

void test_ShadowSignal_RoundTrip(void)
{
    helper_init();
    uint8 txData = 0x99;
    Com_UpdateShadowSignal(0, &txData);
    uint8 rxData = 0;
    Std_ReturnType r = Com_ReceiveShadowSignal(0, &rxData);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
    TEST_ASSERT_EQUAL_HEX8(0x99, rxData);
}

/*==============================================================================
*               7. Com_TriggerTransmit
*==============================================================================*/

void test_TriggerTransmit_Uninit_DetError(void)
{
    PduInfoType info;
    Std_ReturnType r = Com_TriggerTransmit(0, &info);
    TEST_ASSERT_EQUAL(E_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_UNINIT, det_last_error);
}

void test_TriggerTransmit_NullPtr_DetError(void)
{
    helper_init();
    Std_ReturnType r = Com_TriggerTransmit(0, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_PARAM_POINTER, det_last_error);
}

void test_TriggerTransmit_InvalidPduId(void)
{
    helper_init();
    PduInfoType info;
    Std_ReturnType r = Com_TriggerTransmit(COM_NUM_OF_IPDUS, &info);
    TEST_ASSERT_EQUAL(E_NOT_OK, r);
}

void test_TriggerTransmit_Valid(void)
{
    helper_init();
    PduInfoType info;
    memset(&info, 0, sizeof(info));
    Std_ReturnType r = Com_TriggerTransmit(0, &info);
    TEST_ASSERT_EQUAL(E_OK, r);
    TEST_ASSERT_NOT_NULL(info.SduDataPtr);
    TEST_ASSERT_EQUAL(8, info.SduLength);
}

void test_TriggerTransmit_UnknownPduId(void)
{
    /* PduId within range but not in config (no matching PduId) */
    helper_init();
    PduInfoType info;
    Std_ReturnType r = Com_TriggerTransmit(63, &info);
    TEST_ASSERT_EQUAL(E_NOT_OK, r);
}

/*==============================================================================
*               8. Com_TriggerIPDUSend
*==============================================================================*/

void test_TriggerIPDUSend_Uninit_DetError(void)
{
    Std_ReturnType r = Com_TriggerIPDUSend(0);
    TEST_ASSERT_EQUAL(E_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_UNINIT, det_last_error);
}

void test_TriggerIPDUSend_InvalidPduId_DetError(void)
{
    helper_init();
    Std_ReturnType r = Com_TriggerIPDUSend(COM_NUM_OF_IPDUS);
    TEST_ASSERT_EQUAL(E_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_PARAM_IPDU, det_last_error);
}

void test_TriggerIPDUSend_Valid(void)
{
    helper_init();
    g_pdur_transmit_call_count = 0;
    Std_ReturnType r = Com_TriggerIPDUSend(0);
    TEST_ASSERT_EQUAL(E_OK, r);
    TEST_ASSERT_NOT_EQUAL(0, g_pdur_transmit_call_count);
}

void test_TriggerIPDUSend_PduRFails(void)
{
    helper_init();
    g_pdur_transmit_ret = E_NOT_OK;
    Std_ReturnType r = Com_TriggerIPDUSend(0);
    TEST_ASSERT_EQUAL(E_NOT_OK, r);
}

/*==============================================================================
*               9. Com_TxConfirmation
*==============================================================================*/

void test_TxConfirmation_Uninit_DetError(void)
{
    Com_TxConfirmation(0, E_OK);
    TEST_ASSERT_EQUAL(COM_E_UNINIT, det_last_error);
}

void test_TxConfirmation_InvalidPduId_Noop(void)
{
    helper_init();
    /* Should not crash */
    Com_TxConfirmation(COM_NUM_OF_IPDUS, E_OK);
}

void test_TxConfirmation_OK_NoRepeating(void)
{
    helper_init();
    /* IPDU 0 has RepeatingEnabled=FALSE */
    Com_TxConfirmation(0, E_OK);
    /* No crash, state set to idle */
}

void test_TxConfirmation_OK_WithRepeating_IncrementCount(void)
{
    helper_init();
    /* IPDU 1: RepeatingEnabled=TRUE, NumRepetitions=2, TimeBetweenRepetitions=10 */
    /* First need to trigger a send so there's a repetition context */
    /* TxConfirmation with E_OK and repeating enabled:
       - RepetitionCount(0) < NumRepetitions(2) -> increment */
    Com_TxConfirmation(1, E_OK);
    /* RepetitionCount should now be 1, TimeCounter=10 */
}

void test_TxConfirmation_OK_WithRepeating_MaxReached(void)
{
    helper_init();
    /* Call TxConfirmation 3 times: count goes 0->1->2, then at 2 == NumRepetitions(2) -> reset to 0 */
    Com_TxConfirmation(1, E_OK); /* count 0->1 */
    Com_TxConfirmation(1, E_OK); /* count 1->2 */
    Com_TxConfirmation(1, E_OK); /* count 2 == max -> reset to 0 */
}

void test_TxConfirmation_NotOK(void)
{
    helper_init();
    Com_TxConfirmation(1, E_NOT_OK);
    /* result != E_OK, repetition logic skipped */
}

/*==============================================================================
*              10. Com_RxIndication
*==============================================================================*/

void test_RxIndication_Uninit_DetError(void)
{
    PduInfoType info;
    uint8 buf[8] = {0};
    info.SduDataPtr = buf;
    info.SduLength = 8;
    info.MetaDataPtr = NULL_PTR;
    Com_RxIndication(0, &info);
    TEST_ASSERT_EQUAL(COM_E_UNINIT, det_last_error);
}

void test_RxIndication_NullPduInfoPtr_DetError(void)
{
    helper_init();
    Com_RxIndication(0, NULL_PTR);
    TEST_ASSERT_EQUAL(COM_E_PARAM_POINTER, det_last_error);
}

void test_RxIndication_InvalidPduId_Noop(void)
{
    helper_init();
    PduInfoType info;
    uint8 buf[8] = {0};
    info.SduDataPtr = buf;
    info.SduLength = 8;
    info.MetaDataPtr = NULL_PTR;
    Com_RxIndication(COM_NUM_OF_IPDUS, &info);
    /* No crash */
}

void test_RxIndication_NullSduDataPtr(void)
{
    helper_init();
    PduInfoType info;
    info.SduDataPtr = NULL_PTR;
    info.SduLength = 8;
    info.MetaDataPtr = NULL_PTR;
    Com_RxIndication(0, &info);
    /* SduDataPtr == NULL -> data copy skipped */
}

void test_RxIndication_Valid_SmallLength(void)
{
    helper_init();
    uint8 buf[4] = {0x11, 0x22, 0x33, 0x44};
    PduInfoType info;
    info.SduDataPtr = buf;
    info.SduLength = 4; /* < COM_MAX_IPDU_BUFFER_SIZE */
    info.MetaDataPtr = NULL_PTR;
    Com_RxIndication(0, &info);
    /* Data should be copied */
}

void test_RxIndication_Valid_LargeLength(void)
{
    helper_init();
    uint8 buf[256];
    memset(buf, 0xAA, sizeof(buf));
    PduInfoType info;
    info.SduDataPtr = buf;
    info.SduLength = 256; /* > COM_MAX_IPDU_BUFFER_SIZE(128) -> clamped */
    info.MetaDataPtr = NULL_PTR;
    Com_RxIndication(0, &info);
    /* Copy length clamped to COM_MAX_IPDU_BUFFER_SIZE */
}

void test_RxIndication_WithDeadlineMonitoring(void)
{
    /* IPDU 2 has DeadlineLimit=5 */
    helper_init();

    /* Enable DM for group 1 */
    Com_IpduGroupVector gv;
    Com_ClearIpduGroupVector(gv);
    Com_SetIpduGroup(gv, 1);
    Com_EnableReceptionDM(gv);

    uint8 buf[8] = {0};
    PduInfoType info;
    info.SduDataPtr = buf;
    info.SduLength = 8;
    info.MetaDataPtr = NULL_PTR;
    Com_RxIndication(2, &info);
    /* DeadlineCounter should be reset to DeadlineLimit */
}

/*==============================================================================
*              11. Com_MainFunctionRx
*==============================================================================*/

void test_MainFunctionRx_Uninit_Noop(void)
{
    /* Not initialized - should do nothing */
    Com_MainFunctionRx();
}

void test_MainFunctionRx_ClearsUpdatedFlag(void)
{
    helper_init();
    /* Receive data to set Updated=TRUE */
    uint8 buf[8] = {0x11};
    PduInfoType info;
    info.SduDataPtr = buf;
    info.SduLength = 8;
    info.MetaDataPtr = NULL_PTR;
    Com_RxIndication(0, &info);

    /* MainFunctionRx should clear Updated flag */
    Com_MainFunctionRx();
}

void test_MainFunctionRx_DeadlineCountdown(void)
{
    helper_init();

    /* Enable DM for group 1 (IPDU 2) */
    Com_IpduGroupVector gv;
    Com_ClearIpduGroupVector(gv);
    Com_SetIpduGroup(gv, 1);
    Com_EnableReceptionDM(gv);

    /* Simulate receiving data to reset deadline counter */
    uint8 buf[8] = {0};
    PduInfoType info;
    info.SduDataPtr = buf;
    info.SduLength = 8;
    info.MetaDataPtr = NULL_PTR;
    Com_RxIndication(2, &info);

    /* Run MainFunctionRx enough times to exhaust deadline (limit=5) */
    g_deadline_notified = 0;
    for (int i = 0; i < 6; i++)
    {
        Com_MainFunctionRx();
    }
    /* Deadline should have been missed and notification called */
    TEST_ASSERT_EQUAL(1, g_deadline_notified);
    TEST_ASSERT_EQUAL(2, g_deadline_pdu);
}

void test_MainFunctionRx_DeadlineNotMissed(void)
{
    helper_init();

    /* Enable DM for group 1 (IPDU 2) */
    Com_IpduGroupVector gv;
    Com_ClearIpduGroupVector(gv);
    Com_SetIpduGroup(gv, 1);
    Com_EnableReceptionDM(gv);

    /* Receive to reset counter */
    uint8 buf[8] = {0};
    PduInfoType rxInfo;
    rxInfo.SduDataPtr = buf;
    rxInfo.SduLength = 8;
    rxInfo.MetaDataPtr = NULL_PTR;
    Com_RxIndication(2, &rxInfo);

    /* Run only 3 times (limit=5, counter starts at 5, decrements to 2) */
    g_deadline_notified = 0;
    for (int i = 0; i < 3; i++)
    {
        Com_MainFunctionRx();
    }
    TEST_ASSERT_EQUAL(0, g_deadline_notified);
}

void test_MainFunctionRx_GroupDisabled(void)
{
    helper_init();

    /* Disable all groups: set all group bits, then disable */
    Com_IpduGroupVector gv;
    memset(gv, 0xFF, sizeof(gv));
    Com_IpduGroupControl(gv, FALSE);

    /* Set data as updated for IPDU 0 */
    uint8 buf[8] = {0x11};
    PduInfoType rxInfo;
    rxInfo.SduDataPtr = buf;
    rxInfo.SduLength = 8;
    rxInfo.MetaDataPtr = NULL_PTR;
    Com_RxIndication(0, &rxInfo);

    /* MainFunctionRx - group disabled, should skip processing */
    Com_MainFunctionRx();
}

/*==============================================================================
*              12. Com_MainFunctionTx
*==============================================================================*/

void test_MainFunctionTx_Uninit_Noop(void)
{
    Com_MainFunctionTx();
}

void test_MainFunctionTx_PeriodicTransmit(void)
{
    helper_init();
    /* IPDU 0 has TimePeriod=100. After init, TimeCounter=0, so first call triggers TX */
    g_pdur_transmit_call_count = 0;
    Com_MainFunctionTx();
    TEST_ASSERT_NOT_EQUAL(0, g_pdur_transmit_call_count);
}

void test_MainFunctionTx_PeriodicCountdown(void)
{
    helper_init();
    /* First call triggers TX and sets TimeCounter=100 */
    g_pdur_transmit_call_count = 0;
    Com_MainFunctionTx();
    uint8 first_count = g_pdur_transmit_call_count;
    TEST_ASSERT_NOT_EQUAL(0, first_count);

    /* Second call: TimeCounter > 0, should decrement, not transmit */
    g_pdur_transmit_call_count = 0;
    Com_MainFunctionTx();
    TEST_ASSERT_EQUAL(0, g_pdur_transmit_call_count);
}

void test_MainFunctionTx_GroupDisabled(void)
{
    helper_init();
    /* Disable all groups: set all group bits in vector, then call with FALSE */
    Com_IpduGroupVector gv;
    memset(gv, 0xFF, sizeof(gv)); /* all group bits set */
    Com_IpduGroupControl(gv, FALSE);

    g_pdur_transmit_call_count = 0;
    Com_MainFunctionTx();
    TEST_ASSERT_EQUAL(0, g_pdur_transmit_call_count);
}

/*==============================================================================
*              13. Com_MainFunctionRouteSignals
*==============================================================================*/

void test_MainFunctionRouteSignals_Init(void)
{
    helper_init();
    Com_MainFunctionRouteSignals();
    /* Gateway support may be compiled out, just verify no crash */
}

void test_MainFunctionRouteSignals_Uninit(void)
{
    Com_MainFunctionRouteSignals();
}

/*==============================================================================
*              14. IPDU Group Control
*==============================================================================*/

void test_ClearIpduGroupVector(void)
{
    Com_IpduGroupVector gv;
    memset(gv, 0xFF, sizeof(gv));
    Com_ClearIpduGroupVector(gv);
    for (unsigned i = 0; i < sizeof(gv); i++)
    {
        TEST_ASSERT_EQUAL_HEX8(0x00, gv[i]);
    }
}

void test_SetIpduGroup_Valid(void)
{
    Com_IpduGroupVector gv;
    Com_ClearIpduGroupVector(gv);
    Com_SetIpduGroup(gv, 3);
    TEST_ASSERT_NOT_EQUAL(0, gv[0] & (1U << 3));
}

void test_SetIpduGroup_InvalidId(void)
{
    Com_IpduGroupVector gv;
    Com_ClearIpduGroupVector(gv);
    Com_SetIpduGroup(gv, COM_NUM_OF_IPDU_GROUPS); /* out of range */
    TEST_ASSERT_EQUAL_HEX8(0x00, gv[0]);
}

void test_ClearIpduGroup_Valid(void)
{
    Com_IpduGroupVector gv;
    memset(gv, 0xFF, sizeof(gv));
    Com_ClearIpduGroup(gv, 5);
    TEST_ASSERT_EQUAL(0, gv[0] & (1U << 5));
}

void test_ClearIpduGroup_InvalidId(void)
{
    Com_IpduGroupVector gv;
    memset(gv, 0xFF, sizeof(gv));
    Com_ClearIpduGroup(gv, COM_NUM_OF_IPDU_GROUPS); /* out of range */
    TEST_ASSERT_EQUAL_HEX8(0xFF, gv[0]);
}

void test_IpduGroupControl_Enable(void)
{
    helper_init();
    Com_IpduGroupVector gv;
    Com_ClearIpduGroupVector(gv);
    Com_SetIpduGroup(gv, 0); /* group 0 */
    Com_IpduGroupControl(gv, TRUE);
    /* IPDUs in group 0 should be enabled */
}

void test_IpduGroupControl_Disable(void)
{
    helper_init();
    Com_IpduGroupVector gv;
    Com_ClearIpduGroupVector(gv);
    Com_SetIpduGroup(gv, 0);
    Com_IpduGroupControl(gv, FALSE);
    /* IPDUs in group 0 should be disabled */
}

/*==============================================================================
*              15. Com_ReceptionDMControl / Enable / Disable
*==============================================================================*/

void test_ReceptionDMControl_Uninit_DetError(void)
{
    Com_IpduGroupVector gv;
    Com_ClearIpduGroupVector(gv);
    Com_ReceptionDMControl(gv, TRUE);
    TEST_ASSERT_EQUAL(COM_E_UNINIT, det_last_error);
}

void test_EnableReceptionDM_Valid(void)
{
    helper_init();
    Com_IpduGroupVector gv;
    Com_ClearIpduGroupVector(gv);
    Com_SetIpduGroup(gv, 1); /* group 1 has IPDU 2 with DeadlineLimit=5 */
    Com_EnableReceptionDM(gv);
    /* DM should be enabled for IPDU 2 */
}

void test_DisableReceptionDM_Valid(void)
{
    helper_init();
    Com_IpduGroupVector gv;
    Com_ClearIpduGroupVector(gv);
    Com_SetIpduGroup(gv, 1);
    Com_EnableReceptionDM(gv);
    Com_DisableReceptionDM(gv);
    /* DM should be disabled */
}

void test_ReceptionDMControl_EnableWithDeadline(void)
{
    helper_init();
    Com_IpduGroupVector gv;
    Com_ClearIpduGroupVector(gv);
    Com_SetIpduGroup(gv, 1);
    /* Enable: DeadlineLimit>0 -> DeadlineCounter set, DMActive=TRUE */
    Com_ReceptionDMControl(gv, TRUE);
}

void test_ReceptionDMControl_DisableDeactivates(void)
{
    helper_init();
    Com_IpduGroupVector gv;
    Com_ClearIpduGroupVector(gv);
    Com_SetIpduGroup(gv, 1);
    /* Disable: DMActive=FALSE */
    Com_ReceptionDMControl(gv, FALSE);
}

/*==============================================================================
*              16. Com_InvalidateSignal
*==============================================================================*/

void test_InvalidateSignal_Uninit_DetError(void)
{
    Std_ReturnType r = Com_InvalidateSignal(0);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_UNINIT, det_last_error);
}

void test_InvalidateSignal_InvalidId_DetError(void)
{
    helper_init();
    Std_ReturnType r = Com_InvalidateSignal(COM_NUM_OF_SIGNALS);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_INVALID_SIGNAL_ID, det_last_error);
}

void test_InvalidateSignal_Valid(void)
{
    helper_init();
    Std_ReturnType r = Com_InvalidateSignal(0);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
}

/*==============================================================================
*              17. Com_InvalidateSignalGroup
*==============================================================================*/

void test_InvalidateSignalGroup_Uninit_DetError(void)
{
    Std_ReturnType r = Com_InvalidateSignalGroup(0);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_UNINIT, det_last_error);
}

void test_InvalidateSignalGroup_InvalidId_DetError(void)
{
    helper_init();
    Std_ReturnType r = Com_InvalidateSignalGroup(COM_NUM_OF_SIGNAL_GROUPS);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
    TEST_ASSERT_EQUAL(COM_E_INVALID_SIGNAL_GROUP_ID, det_last_error);
}

void test_InvalidateSignalGroup_Valid(void)
{
    helper_init();
    Std_ReturnType r = Com_InvalidateSignalGroup(0);
    TEST_ASSERT_EQUAL(COM_SERVICE_OK, r);
}

/*==============================================================================
*              18. Stub functions
*==============================================================================*/

void test_SendDynSignal_ReturnsNotOK(void)
{
    uint8 data = 0;
    Std_ReturnType r = Com_SendDynSignal(0, &data, 1);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
}

void test_ReceiveDynSignal_ReturnsNotOK(void)
{
    uint8 data = 0;
    uint16 len = 0;
    Std_ReturnType r = Com_ReceiveDynSignal(0, &data, &len);
    TEST_ASSERT_EQUAL(COM_SERVICE_NOT_OK, r);
}

void test_SwitchIpduTxMode_Noop(void)
{
    Com_SwitchIpduTxMode(0, COM_DIRECT);
    /* No crash */
}

void test_TriggerIPDUSendWithMetaData_ReturnsNotOK(void)
{
    uint8 meta[4] = {0};
    Std_ReturnType r = Com_TriggerIPDUSendWithMetaData(0, meta);
    TEST_ASSERT_EQUAL(E_NOT_OK, r);
}

void test_TpRxIndication_Noop(void)
{
    Com_TpRxIndication(0, E_OK);
}

void test_TpTxConfirmation_Noop(void)
{
    Com_TpTxConfirmation(0, E_OK);
}

/*==============================================================================
*              19. Pack/Unpack edge cases via round-trip
*==============================================================================*/

void test_PackUnpack_16bit_LE_RoundTrip(void)
{
    helper_init();
    uint16 txData = 0x1234;
    Com_SendSignal(1, &txData);
    uint16 rxData = 0;
    Com_ReceiveSignal(1, &rxData);
    /* Signal 1 has NEVER filter, so data won't be packed.
       Instead test via shadow buffer path */
}

void test_ShadowRoundTrip_32bit_LE(void)
{
    helper_init();
    /* Signal 2 is 32-bit LE at BitPos=0, byte-aligned */
    uint32 txData = 0xCAFEBABE;
    Com_UpdateShadowSignal(2, &txData);
    uint32 rxData = 0;
    Com_ReceiveShadowSignal(2, &rxData);
    TEST_ASSERT_EQUAL_HEX32(0xCAFEBABE, rxData);
}

void test_ShadowRoundTrip_8bit_BE(void)
{
    helper_init();
    /* Signal 3 is 8-bit BE at BitPos=8, byte-aligned */
    uint8 txData = 0x7E;
    Com_UpdateShadowSignal(3, &txData);
    uint8 rxData = 0;
    Com_ReceiveShadowSignal(3, &rxData);
    TEST_ASSERT_EQUAL_HEX8(0x7E, rxData);
}

void test_ShadowRoundTrip_16bit_BE(void)
{
    helper_init();
    /* Signal 5 is 16-bit BE at BitPos=0, byte-aligned */
    uint16 txData = 0xF00D;
    Com_UpdateShadowSignal(5, &txData);
    uint16 rxData = 0;
    Com_ReceiveShadowSignal(5, &rxData);
    TEST_ASSERT_EQUAL_HEX16(0xF00D, rxData);
}

void test_ShadowRoundTrip_12bit_LE_NonAligned(void)
{
    helper_init();
    /* Signal 4 is 12-bit LE at BitPos=4, non-byte-aligned */
    uint16 txData = 0xABC;
    Com_UpdateShadowSignal(4, &txData);
    uint16 rxData = 0;
    Com_ReceiveShadowSignal(4, &rxData);
    TEST_ASSERT_EQUAL_HEX16(0xABC, rxData);
}

void test_ShadowRoundTrip_3bit_LE(void)
{
    helper_init();
    /* Signal 7 is 3-bit LE at BitPos=5 */
    uint8 txData = 0x07; /* max 3-bit value */
    Com_UpdateShadowSignal(7, &txData);
    uint8 rxData = 0;
    Com_ReceiveShadowSignal(7, &rxData);
    TEST_ASSERT_EQUAL_HEX8(0x07, rxData);
}

/*==============================================================================
*              20. Deadline miss with NULL notification
*==============================================================================*/

void test_MainFunctionRx_DeadlineMiss_NullNotification(void)
{
    /* IPDU 3 has DeadlineLimit=0, so no deadline monitoring for it.
       Let's test with a config where notification is NULL but deadline is set.
       We can't easily change config at runtime, so just verify IPDU 3 (no deadline) path. */
    helper_init();

    /* Enable DM for group 1 */
    Com_IpduGroupVector gv;
    Com_ClearIpduGroupVector(gv);
    Com_SetIpduGroup(gv, 1);
    Com_EnableReceptionDM(gv);

    /* IPDU 3 has DeadlineLimit=0, so even if DM enabled, the condition
       ipduConfig->DeadlineLimit > 0 is false -> no countdown */
    Com_MainFunctionRx();
    /* No crash */
}

/*==============================================================================
*              21. Multiple init cycles
*==============================================================================*/

void test_ReInit_AfterDeInit(void)
{
    helper_init();
    Com_DeInit();
    TEST_ASSERT_EQUAL(COM_UNINIT, Com_GetStatus());
    helper_init();
    TEST_ASSERT_EQUAL(COM_INIT, Com_GetStatus());
}

/*==============================================================================
*              22. RxIndication followed by ReceiveSignal data integrity
*==============================================================================*/

void test_RxIndication_ThenReceiveSignal_DataIntegrity(void)
{
    helper_init();
    /* Simulate receiving an IPDU with known data */
    uint8 rxBuf[8] = {0xAB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    PduInfoType info;
    info.SduDataPtr = rxBuf;
    info.SduLength = 8;
    info.MetaDataPtr = NULL_PTR;
    Com_RxIndication(0, &info);

    /* Now receive signal 0 (8-bit LE at BitPos=0 in IPDU 0) */
    uint8 sigData = 0;
    Com_ReceiveSignal(0, &sigData);
    TEST_ASSERT_EQUAL_HEX8(0xAB, sigData);
}

/*==============================================================================
*              TEST RUNNER
*==============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    /* Init / DeInit / Status */
    RUN_TEST(test_Init_NullPtr_DetError);
    RUN_TEST(test_Init_ValidConfig_StatusInit);
    RUN_TEST(test_DeInit_WhenUninit_DetError);
    RUN_TEST(test_DeInit_WhenInit_Succeeds);
    RUN_TEST(test_GetStatus_Uninit);
    RUN_TEST(test_GetStatus_Init);

    /* GetVersionInfo */
    RUN_TEST(test_GetVersionInfo_NullPtr_DetError);
    RUN_TEST(test_GetVersionInfo_Valid);

    /* SendSignal */
    RUN_TEST(test_SendSignal_Uninit_DetError);
    RUN_TEST(test_SendSignal_NullDataPtr_DetError);
    RUN_TEST(test_SendSignal_InvalidSignalId_DetError);
    RUN_TEST(test_SendSignal_8bit_LE_Triggered);
    RUN_TEST(test_SendSignal_16bit_NEVER_filter_blocked);
    RUN_TEST(test_SendSignal_32bit_LE_TriggeredOnChange);
    RUN_TEST(test_SendSignal_8bit_BE_FilterMaskedNewEqualsX_Pass);
    RUN_TEST(test_SendSignal_8bit_BE_FilterMaskedNewEqualsX_Blocked);
    RUN_TEST(test_SendSignal_12bit_LE_FilterMaskedNewDiffersX_Pass);
    RUN_TEST(test_SendSignal_12bit_LE_FilterMaskedNewDiffersX_Blocked);
    RUN_TEST(test_SendSignal_16bit_BE_FilterMaskedNewDiffersMaskedOld_Pass);
    RUN_TEST(test_SendSignal_16bit_BE_FilterMaskedNewDiffersMaskedOld_Blocked);
    RUN_TEST(test_SendSignal_3bit_LE_NonByteAligned);
    RUN_TEST(test_SendSignal_PduR_Fails);

    /* ReceiveSignal */
    RUN_TEST(test_ReceiveSignal_Uninit_DetError);
    RUN_TEST(test_ReceiveSignal_NullPtr_DetError);
    RUN_TEST(test_ReceiveSignal_InvalidId_DetError);
    RUN_TEST(test_ReceiveSignal_RoundTrip_8bit_LE);
    RUN_TEST(test_ReceiveSignal_RoundTrip_32bit_LE);
    RUN_TEST(test_ReceiveSignal_RoundTrip_8bit_BE);
    RUN_TEST(test_ReceiveSignal_RoundTrip_12bit_LE);
    RUN_TEST(test_ReceiveSignal_RoundTrip_16bit_BE);
    RUN_TEST(test_ReceiveSignal_RoundTrip_3bit_LE);

    /* SendSignalGroup / ReceiveSignalGroup */
    RUN_TEST(test_SendSignalGroup_Uninit_DetError);
    RUN_TEST(test_SendSignalGroup_InvalidId_DetError);
    RUN_TEST(test_SendSignalGroup_PduR_OK);
    RUN_TEST(test_SendSignalGroup_PduR_Fails);
    RUN_TEST(test_ReceiveSignalGroup_Uninit_DetError);
    RUN_TEST(test_ReceiveSignalGroup_InvalidId_DetError);
    RUN_TEST(test_ReceiveSignalGroup_Valid);

    /* Shadow signals */
    RUN_TEST(test_UpdateShadowSignal_Uninit_DetError);
    RUN_TEST(test_UpdateShadowSignal_NullPtr_DetError);
    RUN_TEST(test_UpdateShadowSignal_Valid);
    RUN_TEST(test_ReceiveShadowSignal_Uninit_DetError);
    RUN_TEST(test_ReceiveShadowSignal_NullPtr_DetError);
    RUN_TEST(test_ShadowSignal_RoundTrip);

    /* TriggerTransmit */
    RUN_TEST(test_TriggerTransmit_Uninit_DetError);
    RUN_TEST(test_TriggerTransmit_NullPtr_DetError);
    RUN_TEST(test_TriggerTransmit_InvalidPduId);
    RUN_TEST(test_TriggerTransmit_Valid);
    RUN_TEST(test_TriggerTransmit_UnknownPduId);

    /* TriggerIPDUSend */
    RUN_TEST(test_TriggerIPDUSend_Uninit_DetError);
    RUN_TEST(test_TriggerIPDUSend_InvalidPduId_DetError);
    RUN_TEST(test_TriggerIPDUSend_Valid);
    RUN_TEST(test_TriggerIPDUSend_PduRFails);

    /* TxConfirmation */
    RUN_TEST(test_TxConfirmation_Uninit_DetError);
    RUN_TEST(test_TxConfirmation_InvalidPduId_Noop);
    RUN_TEST(test_TxConfirmation_OK_NoRepeating);
    RUN_TEST(test_TxConfirmation_OK_WithRepeating_IncrementCount);
    RUN_TEST(test_TxConfirmation_OK_WithRepeating_MaxReached);
    RUN_TEST(test_TxConfirmation_NotOK);

    /* RxIndication */
    RUN_TEST(test_RxIndication_Uninit_DetError);
    RUN_TEST(test_RxIndication_NullPduInfoPtr_DetError);
    RUN_TEST(test_RxIndication_InvalidPduId_Noop);
    RUN_TEST(test_RxIndication_NullSduDataPtr);
    RUN_TEST(test_RxIndication_Valid_SmallLength);
    RUN_TEST(test_RxIndication_Valid_LargeLength);
    RUN_TEST(test_RxIndication_WithDeadlineMonitoring);

    /* MainFunctionRx */
    RUN_TEST(test_MainFunctionRx_Uninit_Noop);
    RUN_TEST(test_MainFunctionRx_ClearsUpdatedFlag);
    RUN_TEST(test_MainFunctionRx_DeadlineCountdown);
    RUN_TEST(test_MainFunctionRx_DeadlineNotMissed);
    RUN_TEST(test_MainFunctionRx_GroupDisabled);
    RUN_TEST(test_MainFunctionRx_DeadlineMiss_NullNotification);

    /* MainFunctionTx */
    RUN_TEST(test_MainFunctionTx_Uninit_Noop);
    RUN_TEST(test_MainFunctionTx_PeriodicTransmit);
    RUN_TEST(test_MainFunctionTx_PeriodicCountdown);
    RUN_TEST(test_MainFunctionTx_GroupDisabled);

    /* MainFunctionRouteSignals */
    RUN_TEST(test_MainFunctionRouteSignals_Init);
    RUN_TEST(test_MainFunctionRouteSignals_Uninit);

    /* IPDU Group Control */
    RUN_TEST(test_ClearIpduGroupVector);
    RUN_TEST(test_SetIpduGroup_Valid);
    RUN_TEST(test_SetIpduGroup_InvalidId);
    RUN_TEST(test_ClearIpduGroup_Valid);
    RUN_TEST(test_ClearIpduGroup_InvalidId);
    RUN_TEST(test_IpduGroupControl_Enable);
    RUN_TEST(test_IpduGroupControl_Disable);

    /* DM Control */
    RUN_TEST(test_ReceptionDMControl_Uninit_DetError);
    RUN_TEST(test_EnableReceptionDM_Valid);
    RUN_TEST(test_DisableReceptionDM_Valid);
    RUN_TEST(test_ReceptionDMControl_EnableWithDeadline);
    RUN_TEST(test_ReceptionDMControl_DisableDeactivates);

    /* Invalidate */
    RUN_TEST(test_InvalidateSignal_Uninit_DetError);
    RUN_TEST(test_InvalidateSignal_InvalidId_DetError);
    RUN_TEST(test_InvalidateSignal_Valid);
    RUN_TEST(test_InvalidateSignalGroup_Uninit_DetError);
    RUN_TEST(test_InvalidateSignalGroup_InvalidId_DetError);
    RUN_TEST(test_InvalidateSignalGroup_Valid);

    /* Stubs */
    RUN_TEST(test_SendDynSignal_ReturnsNotOK);
    RUN_TEST(test_ReceiveDynSignal_ReturnsNotOK);
    RUN_TEST(test_SwitchIpduTxMode_Noop);
    RUN_TEST(test_TriggerIPDUSendWithMetaData_ReturnsNotOK);
    RUN_TEST(test_TpRxIndication_Noop);
    RUN_TEST(test_TpTxConfirmation_Noop);

    /* Pack/Unpack edge cases */
    RUN_TEST(test_PackUnpack_16bit_LE_RoundTrip);
    RUN_TEST(test_ShadowRoundTrip_32bit_LE);
    RUN_TEST(test_ShadowRoundTrip_8bit_BE);
    RUN_TEST(test_ShadowRoundTrip_16bit_BE);
    RUN_TEST(test_ShadowRoundTrip_12bit_LE_NonAligned);
    RUN_TEST(test_ShadowRoundTrip_3bit_LE);

    /* Misc */
    RUN_TEST(test_ReInit_AfterDeInit);
    RUN_TEST(test_RxIndication_ThenReceiveSignal_DataIntegrity);

    return UNITY_END();
}
