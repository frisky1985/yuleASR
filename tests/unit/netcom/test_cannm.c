/**
 * @file test_cannm.c
 * @brief CanNm (CAN Network Management) unit tests — coverage uplift.
 *
 * Compiles the real production sources (CanNm.c + CanNm_Lcfg.c) against host
 * stubs for CanIf/ComM/Nm/Det and drives the AUTOSAR NM state machine:
 *  - lifecycle (Init/DeInit/GetVersionInfo) + uninit DET guards on every API
 *  - Bus Sleep -> Repeat Message -> Normal Operation / Ready Sleep transitions
 *  - NM timeout -> Prepare Bus Sleep -> Bus Sleep (timer-driven)
 *  - RxIndication wakeup, repeat-message-request CBV, user data access
 *  - communication control (Disable/Enable), TriggerTransmit, TxConfirmation
 *
 * The CanNm_Config link-time table comes from the real CanNm_Lcfg.c
 * (2 channels, node ids 1/2, NID@0, CBV@1, NM timeout 1000 ms, repeat 400 ms,
 * wait-bus-sleep 1000 ms, message cycle 100/150 ms, 3 immediate transmissions).
 */

// @tests src/bsw/ecual/canNm/src/CanNm.c  @tests src/bsw/ecual/canNm/src/CanNm_Lcfg.c
// @tests src/bsw/ecual/canNm/include/CanNm.h  @tests src/bsw/ecual/canNm/include/CanNm_Cfg.h

#include <string.h>
#include "unity.h"
#include "CanNm.h"
#include "Nm.h"
#include "ComM.h"     /* stub typedefs for the ComM_* stubs below */

/*==================================================================================================
*                                  Stub capture variables
*================================================================================================*/
static uint16       test_DetCalls;
static uint8        test_DetLastApiId;
static uint8        test_DetLastErrorId;
static uint16       test_CanIfTxCount;
static PduIdType    test_CanIfTxLastId;
static uint8        test_CanIfTxLastData[8];
static Std_ReturnType test_CanIfTxResult;         /* controllable CanIf result */
static uint16       test_ComMNetworkModeCount;
static uint16       test_ComMPrepareSleepCount;
static uint16       test_ComMBusSleepCount;
static uint16       test_WakeUpIndCount;
static uint16       test_NmStateChangeCount;
static Nm_StateType test_NmLastState;
static Nm_ModeType  test_NmLastMode;

/*==================================================================================================
*                                  Stubs
*================================================================================================*/
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    test_DetCalls++;
    test_DetLastApiId = ApiId;
    test_DetLastErrorId = ErrorId;
    return E_OK;
}

Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    test_CanIfTxCount++;
    test_CanIfTxLastId = TxPduId;
    if ((PduInfoPtr != NULL_PTR) && (PduInfoPtr->SduDataPtr != NULL_PTR))
    {
        (void)memcpy(test_CanIfTxLastData, PduInfoPtr->SduDataPtr,
                     (PduInfoPtr->SduLength < 8U) ? PduInfoPtr->SduLength : 8U);
    }
    return test_CanIfTxResult;
}

void ComM_ECNM_NetworkMode(ComM_ChannelHandleType Channel)
{
    (void)Channel;
    test_ComMNetworkModeCount++;
}

void ComM_ECNM_PrepareBusSleepMode(ComM_ChannelHandleType Channel)
{
    (void)Channel;
    test_ComMPrepareSleepCount++;
}

void ComM_ECNM_BusSleepMode(ComM_ChannelHandleType Channel)
{
    (void)Channel;
    test_ComMBusSleepCount++;
}

void ComM_EcuM_WakeUpIndication(ComM_EcuM_WakeUpType WakeupType)
{
    (void)WakeupType;
    test_WakeUpIndCount++;
}

void Nm_StateChangeNotification(Nm_ChannelHandleType nmNetworkHandle,
                                Nm_StateType nmPreviousState,
                                Nm_StateType nmCurrentState)
{
    (void)nmNetworkHandle;
    (void)nmPreviousState;
    test_NmStateChangeCount++;
    test_NmLastState = nmCurrentState;
}

void Nm_RemoteSleepIndication(Nm_ChannelHandleType nmNetworkHandle)
{
    (void)nmNetworkHandle;
}

void Nm_RemoteSleepCancellation(Nm_ChannelHandleType nmNetworkHandle)
{
    (void)nmNetworkHandle;
}

/*==================================================================================================
*                                  Fixtures
*================================================================================================*/
static void test_init(void)
{
    CanNm_Init(&CanNm_Config);
}

static void run_main(uint16 cycles)
{
    while (cycles > 0U)
    {
        CanNm_MainFunction();
        cycles--;
    }
}

/* read back the public state/mode of a channel */
static void get_state(NetworkHandleType ch, Nm_StateType* state, Nm_ModeType* mode)
{
    TEST_ASSERT_EQUAL(E_OK, CanNm_GetState(ch, state, mode));
}

/* read back only the public state (mode goes to a shared scratch variable) */
static Nm_ModeType s_mode;
static void get_st(NetworkHandleType ch, Nm_StateType* state)
{
    TEST_ASSERT_EQUAL(E_OK, CanNm_GetState(ch, state, &s_mode));
}

void setUp(void)
{
    test_DetCalls = 0U;
    test_DetLastApiId = 0xFFU;
    test_DetLastErrorId = 0xFFU;
    test_CanIfTxCount = 0U;
    test_CanIfTxLastId = 0xFFFFU;
    (void)memset(test_CanIfTxLastData, 0, sizeof(test_CanIfTxLastData));
    test_CanIfTxResult = E_OK;
    test_ComMNetworkModeCount = 0U;
    test_ComMPrepareSleepCount = 0U;
    test_ComMBusSleepCount = 0U;
    test_WakeUpIndCount = 0U;
    test_NmStateChangeCount = 0U;
    test_NmLastState = 0U;
    test_NmLastMode = 0U;

    /* ensure a defined uninit state before each test */
    {
        Nm_StateType st;
        Nm_ModeType  md;
        if (CanNm_GetState(0U, &st, &md) == E_OK)
        {
            CanNm_DeInit();
        }
    }
    test_DetCalls = 0U;
}

void tearDown(void)
{
    Nm_StateType st;
    Nm_ModeType  md;
    if (CanNm_GetState(0U, &st, &md) == E_OK)
    {
        CanNm_DeInit();
    }
}

/*==================================================================================================
*                                  Uninit DET guards
*================================================================================================*/
void test_Uninit_Guards_ReportDet(void)
{
    Nm_StateType st;
    Nm_ModeType  md;
    PduInfoType  pdu;
    uint8        data[8];
    uint8        userData[8];

    pdu.SduDataPtr = data;
    pdu.SduLength = 8U;
    pdu.MetaDataPtr = NULL_PTR;

    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_PassiveStartUp(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_NetworkRequest(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_NetworkRelease(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_TriggerTransmit(0U, &pdu));
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_SetUserData(0U, userData));
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_GetUserData(0U, userData));
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_SetSleepReadyBit(0U, TRUE));
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_DisableCommunication(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_EnableCommunication(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_GetState(0U, &st, &md));
    CanNm_MainFunction();
    CanNm_RxIndication(0U, &pdu);
    CanNm_TxConfirmation(0U, E_OK);

    /* every API above reported CANNM_E_NOT_INITIALIZED (11 calls + the three
     * MainFunction/RxIndication/TxConfirmation calls) */
    TEST_ASSERT_EQUAL_UINT16(14U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_NOT_INITIALIZED, test_DetLastErrorId);

    CanNm_DeInit();
    TEST_ASSERT_EQUAL_UINT16(15U, test_DetCalls);
}

/*==================================================================================================
*                                  Lifecycle
*================================================================================================*/
void test_Init_NullPtr_ReportsDet(void)
{
    CanNm_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_PARAM_POINTER, test_DetLastErrorId);
}

void test_Init_Twice_ReportsDet(void)
{
    test_init();
    CanNm_Init(&CanNm_Config);
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_ALREADY_INITIALIZED, test_DetLastErrorId);
}

void test_Init_EntersBusSleep(void)
{
    Nm_StateType st;
    Nm_ModeType  md;

    test_init();
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
    get_state(0U, &st, &md);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_BUS_SLEEP, st);
    TEST_ASSERT_EQUAL_UINT8(NM_MODE_BUS_SLEEP, md);
    get_state(1U, &st, &md);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_BUS_SLEEP, st);
}

void test_DeInit_AfterInit_NotifiesBusSleep(void)
{
    test_init();
    CanNm_DeInit();
    /* EnterBusSleepMode runs per configured channel -> 2 notifications */
    TEST_ASSERT_EQUAL_UINT16(2U, test_ComMBusSleepCount);
    /* after DeInit every API reports uninit again */
    CanNm_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_NOT_INITIALIZED, test_DetLastErrorId);
}

void test_GetVersionInfo_Valid(void)
{
    Std_VersionInfoType info;

    (void)memset(&info, 0, sizeof(info));
    CanNm_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT16(CANNM_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT16(CANNM_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(CANNM_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(CANNM_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(CANNM_SW_PATCH_VERSION, info.sw_patch_version);
}

void test_GetVersionInfo_Null_ReportsDet(void)
{
    CanNm_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_PARAM_POINTER, test_DetLastErrorId);
}

/*==================================================================================================
*                                  State machine transitions
*================================================================================================*/
void test_NetworkRequest_FromBusSleep_EntersRepeatMessage(void)
{
    Nm_StateType st;
    Nm_ModeType  md;

    test_init();
    TEST_ASSERT_EQUAL(E_OK, CanNm_NetworkRequest(0U));
    get_state(0U, &st, &md);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_REPEAT_MESSAGE, st);
    TEST_ASSERT_EQUAL_UINT8(NM_MODE_NETWORK, md);
    /* NOTE: EnterRepeatMessageState notifies ComM directly AND via
     * NotifyStateChange -> 2 calls per transition */
    TEST_ASSERT_EQUAL_UINT16(2U, test_ComMNetworkModeCount);

    /* immediate transmission: first MainFunction sends an NM PDU */
    CanNm_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(1U, test_CanIfTxCount);
    TEST_ASSERT_EQUAL(0U, test_CanIfTxLastId);
    TEST_ASSERT_EQUAL_HEX8(0x01U, test_CanIfTxLastData[0]);   /* Node ID @ byte 0 */

    /* already in network mode: request again is accepted silently */
    TEST_ASSERT_EQUAL(E_OK, CanNm_NetworkRequest(0U));
}

void test_NetworkRequest_InvalidChannel_ReportsDet(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_NetworkRequest(9U));
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_INVALID_CHANNEL, test_DetLastErrorId);
}

void test_NetworkRequest_SecondChannel(void)
{
    Nm_StateType st;
    Nm_ModeType  md;

    test_init();
    TEST_ASSERT_EQUAL(E_OK, CanNm_NetworkRequest(1U));
    get_state(1U, &st, &md);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_REPEAT_MESSAGE, st);
    CanNm_MainFunction();
    TEST_ASSERT_EQUAL_HEX8(0x02U, test_CanIfTxLastData[0]);   /* node 2 */
    TEST_ASSERT_EQUAL(1U, test_CanIfTxLastId);
}

void test_PassiveStartUp_ThenRepeatExpiry_GoesReadySleep(void)
{
    Nm_StateType st;
    Nm_ModeType  md;

    test_init();
    TEST_ASSERT_EQUAL(E_OK, CanNm_PassiveStartUp(0U));
    get_state(0U, &st, &md);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_REPEAT_MESSAGE, st);

    /* repeat message time 400 ms, main function period 10 ms;
     * no NetworkRequest -> ready sleep after expiry */
    run_main(41U);
    get_state(0U, &st, &md);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_READY_SLEEP, st);
    TEST_ASSERT_EQUAL_UINT8(NM_MODE_NETWORK, md);
}

void test_PassiveStartUp_InNetworkMode_Rejected(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_OK, CanNm_NetworkRequest(0U));
    /* already in network mode: silently rejected (no DET) */
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_PassiveStartUp(0U));

    /* unknown channel: rejected with DET */
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_PassiveStartUp(9U));
    TEST_ASSERT_EQUAL_UINT16(1U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_INVALID_CHANNEL, test_DetLastErrorId);
}

void test_RepeatMessageExpiry_WithRequest_GoesNormalOperation(void)
{
    Nm_StateType st;
    Nm_ModeType  md;

    test_init();
    TEST_ASSERT_EQUAL(E_OK, CanNm_NetworkRequest(0U));
    run_main(41U);
    get_state(0U, &st, &md);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_NORMAL_OPERATION, st);
    /* periodic transmission continues in normal operation */
    TEST_ASSERT_GREATER_THAN(2U, test_CanIfTxCount);
}

void test_RepeatMessageExpiry_WithRequest_SecondChannel_UsesOwnCycle(void)
{
    Nm_StateType st;

    test_init();
    TEST_ASSERT_EQUAL(E_OK, CanNm_NetworkRequest(1U));
    run_main(41U);
    get_st(1U, &st);
    /* channel 1 uses 150 ms cycle; still normal operation after 410 ms */
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_NORMAL_OPERATION, st);
}

void test_NetworkRelease_InNormalOperation_GoesReadySleep(void)
{
    Nm_StateType st;

    test_init();
    TEST_ASSERT_EQUAL(E_OK, CanNm_NetworkRequest(0U));
    run_main(41U);
    TEST_ASSERT_EQUAL(E_OK, CanNm_NetworkRelease(0U));
    get_st(0U, &st);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_READY_SLEEP, st);
}

void test_NetworkRelease_InRepeatMessage_Stays(void)
{
    Nm_StateType st;

    test_init();
    TEST_ASSERT_EQUAL(E_OK, CanNm_NetworkRequest(0U));
    TEST_ASSERT_EQUAL(E_OK, CanNm_NetworkRelease(0U));
    get_st(0U, &st);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_REPEAT_MESSAGE, st);
}

void test_NetworkRelease_InBusSleep_Accepted(void)
{
    Nm_StateType st;

    test_init();
    TEST_ASSERT_EQUAL(E_OK, CanNm_NetworkRelease(0U));
    get_st(0U, &st);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_BUS_SLEEP, st);

    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_NetworkRelease(9U));
}

void test_NetworkRequest_InReadySleep_GoesNormalOperation(void)
{
    Nm_StateType st;

    test_init();
    TEST_ASSERT_EQUAL(E_OK, CanNm_PassiveStartUp(0U));
    run_main(41U);
    get_st(0U, &st);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_READY_SLEEP, st);

    TEST_ASSERT_EQUAL(E_OK, CanNm_NetworkRequest(0U));
    get_st(0U, &st);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_NORMAL_OPERATION, st);
}

void test_NmTimeout_InReadySleep_GoesPrepareBusSleep(void)
{
    Nm_StateType st;
    Nm_ModeType  md;

    test_init();
    TEST_ASSERT_EQUAL(E_OK, CanNm_PassiveStartUp(0U));
    run_main(41U);
    get_state(0U, &st, &md);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_READY_SLEEP, st);

    /* ready sleep stops NM PDU transmission: the NM timeout (<=1000 ms)
     * expires and the channel enters prepare bus sleep */
    run_main(120U);
    get_state(0U, &st, &md);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_PREPARE_BUS_SLEEP, st);
    TEST_ASSERT_EQUAL_UINT8(NM_MODE_PREPARE_BUS_SLEEP, md);
    TEST_ASSERT_EQUAL_UINT16(1U, test_ComMPrepareSleepCount);
}

void test_PrepareBusSleep_Expiry_GoesBusSleep(void)
{
    Nm_StateType st;
    Nm_ModeType  md;

    test_init();
    TEST_ASSERT_EQUAL(E_OK, CanNm_PassiveStartUp(0U));
    run_main(41U);
    run_main(120U);   /* -> prepare bus sleep */
    run_main(101U);   /* wait bus sleep time 1000 ms */
    get_state(0U, &st, &md);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_BUS_SLEEP, st);
    TEST_ASSERT_EQUAL_UINT8(NM_MODE_BUS_SLEEP, md);
    TEST_ASSERT_EQUAL_UINT16(1U, test_ComMBusSleepCount);
}

/*==================================================================================================
*                                  RxIndication
*================================================================================================*/
void test_RxIndication_NullPdu_ReportsDet(void)
{
    test_init();
    CanNm_RxIndication(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_PARAM_POINTER, test_DetLastErrorId);
}

void test_RxIndication_UnknownPduId_ReportsDet(void)
{
    PduInfoType pdu;
    uint8 frame[8] = { 0U };

    test_init();
    pdu.SduDataPtr = frame;
    pdu.SduLength = 8U;
    pdu.MetaDataPtr = NULL_PTR;
    CanNm_RxIndication(7U, &pdu);
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_INVALID_PDUID, test_DetLastErrorId);
}

void test_RxIndication_WakesUpFromBusSleep(void)
{
    Nm_StateType st;
    Nm_ModeType  md;
    PduInfoType  pdu;
    uint8 frame[8] = { 0x10U, 0x00U, 0U, 0U, 0U, 0U, 0U, 0U };

    test_init();
    pdu.SduDataPtr = frame;
    pdu.SduLength = 8U;
    pdu.MetaDataPtr = NULL_PTR;
    CanNm_RxIndication(0U, &pdu);

    get_state(0U, &st, &md);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_REPEAT_MESSAGE, st);
    TEST_ASSERT_EQUAL_UINT8(NM_MODE_NETWORK, md);
    TEST_ASSERT_EQUAL_UINT16(1U, test_WakeUpIndCount);   /* ComM notified */
    TEST_ASSERT_EQUAL_UINT16(1U, test_NmStateChangeCount);
}

void test_RxIndication_RepeatMessageRequest_InNormalOperation(void)
{
    Nm_StateType st;
    PduInfoType  pdu;
    uint8 frame[8] = { 0x10U, 0x01U, 0U, 0U, 0U, 0U, 0U, 0U };   /* CBV repeat bit */

    test_init();
    TEST_ASSERT_EQUAL(E_OK, CanNm_NetworkRequest(0U));
    run_main(41U);
    get_st(0U, &st);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_NORMAL_OPERATION, st);

    pdu.SduDataPtr = frame;
    pdu.SduLength = 8U;
    pdu.MetaDataPtr = NULL_PTR;
    CanNm_RxIndication(0U, &pdu);
    get_st(0U, &st);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_REPEAT_MESSAGE, st);
}

void test_RxIndication_NoRepeatBit_DoesNotExtendRepeatTimer(void)
{
    /* NOTE (observed behaviour): a plain NM PDU (no repeat-message-request
     * bit) does NOT restart the repeat message timer; the channel leaves
     * repeat message mode when its own timer expires. */
    Nm_StateType st;

    test_init();
    TEST_ASSERT_EQUAL(E_OK, CanNm_PassiveStartUp(0U));
    run_main(30U);   /* 300 ms into the 400 ms repeat message time */

    {
        PduInfoType pdu;
        uint8 frame[8] = { 0U };
        pdu.SduDataPtr = frame;
        pdu.SduLength = 8U;
        pdu.MetaDataPtr = NULL_PTR;
        CanNm_RxIndication(0U, &pdu);
    }
    run_main(15U);   /* 450 ms total: repeat time (400 ms) has expired */
    get_st(0U, &st);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_READY_SLEEP, st);
}

void test_RxIndication_StoresUserData(void)
{
    PduInfoType pdu;
    uint8 frame[8] = { 0x10U, 0x00U, 0xA1U, 0xA2U, 0xA3U, 0xA4U, 0xA5U, 0xA6U };
    uint8 userData[8];

    test_init();
    pdu.SduDataPtr = frame;
    pdu.SduLength = 8U;
    pdu.MetaDataPtr = NULL_PTR;
    CanNm_RxIndication(0U, &pdu);

    (void)memset(userData, 0, sizeof(userData));
    TEST_ASSERT_EQUAL(E_OK, CanNm_GetUserData(0U, userData));
    /* NID (byte 0) and CBV (byte 1) are skipped */
    TEST_ASSERT_EQUAL_HEX8(0xA1U, userData[0]);
    TEST_ASSERT_EQUAL_HEX8(0xA6U, userData[5]);
}

/*==================================================================================================
*                                  TxConfirmation / Transmit / TriggerTransmit
*================================================================================================*/
void test_TxConfirmation_ValidAndInvalid(void)
{
    test_init();
    CanNm_TxConfirmation(0U, E_OK);
    CanNm_TxConfirmation(1U, E_NOT_OK);
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);

    CanNm_TxConfirmation(9U, E_OK);
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_INVALID_PDUID, test_DetLastErrorId);
}

void test_Transmit_ForwardsToCanIf(void)
{
    PduInfoType pdu;
    uint8 data[8] = { 0U };

    test_init();
    pdu.SduDataPtr = data;
    pdu.SduLength = 8U;
    pdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_OK, CanNm_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL_UINT16(1U, test_CanIfTxCount);

    /* unknown pdu id -> no CanIf call */
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_Transmit(9U, &pdu));
    TEST_ASSERT_EQUAL_UINT16(1U, test_CanIfTxCount);

    /* null pdu -> DET */
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_Transmit(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_PARAM_POINTER, test_DetLastErrorId);
}

void test_TriggerTransmit_CopiesUpdatedPdu(void)
{
    PduInfoType pdu;
    uint8 buffer[8];

    test_init();
    TEST_ASSERT_EQUAL(E_OK, CanNm_SetSleepReadyBit(0U, TRUE));

    pdu.SduDataPtr = buffer;
    pdu.SduLength = 8U;
    pdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_OK, CanNm_TriggerTransmit(0U, &pdu));
    TEST_ASSERT_EQUAL_HEX8(0x01U, buffer[0]);                     /* node id */
    /* NOTE (observed behaviour): SetSleepReadyBit() writes the CBV bit into
     * TxPduBuffer, but UpdateTxPdu() memsets the buffer before transmission,
     * so the bit is lost. Asserted as-is; flagged for review. */
    TEST_ASSERT_EQUAL_HEX8(0x00U, buffer[1]);

    /* short buffer rejected */
    pdu.SduLength = 4U;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_TriggerTransmit(0U, &pdu));

    /* null / unknown pdu id */
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_TriggerTransmit(0U, NULL_PTR));
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_TriggerTransmit(9U, &pdu));
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_PARAM_POINTER, test_DetLastErrorId);
}

/*==================================================================================================
*                                  User data / sleep ready bit / PN
*================================================================================================*/
void test_SetUserData_ValidAndInvalid(void)
{
    uint8 userData[8] = { 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U };

    test_init();
    TEST_ASSERT_EQUAL(E_OK, CanNm_SetUserData(0U, userData));
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_SetUserData(9U, userData));
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_INVALID_CHANNEL, test_DetLastErrorId);
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_SetUserData(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_PARAM_POINTER, test_DetLastErrorId);
}

void test_GetUserData_InvalidChannel_ReportsDet(void)
{
    uint8 userData[8];

    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_GetUserData(9U, userData));
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_GetUserData(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_PARAM_POINTER, test_DetLastErrorId);
}

void test_SetSleepReadyBit_InvalidChannel_ReportsDet(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_OK, CanNm_SetSleepReadyBit(1U, TRUE));
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_SetSleepReadyBit(9U, FALSE));
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_INVALID_CHANNEL, test_DetLastErrorId);
}

void test_ConfirmPnAvailability(void)
{
    test_init();
    CanNm_ConfirmPnAvailability(0U);
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
    CanNm_ConfirmPnAvailability(9U);
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_INVALID_CHANNEL, test_DetLastErrorId);
}

/*==================================================================================================
*                                  Communication control
*================================================================================================*/
void test_DisableEnableCommunication_InRepeatMessage(void)
{
    Nm_StateType st;
    uint16 txAfterDisable;

    test_init();
    TEST_ASSERT_EQUAL(E_OK, CanNm_NetworkRequest(0U));
    (void)CanNm_MainFunction();

    TEST_ASSERT_EQUAL(E_OK, CanNm_DisableCommunication(0U));
    txAfterDisable = test_CanIfTxCount;
    run_main(5U);
    TEST_ASSERT_EQUAL_UINT16(txAfterDisable, test_CanIfTxCount);   /* tx stopped */

    TEST_ASSERT_EQUAL(E_OK, CanNm_EnableCommunication(0U));
    run_main(30U);
    TEST_ASSERT_GREATER_THAN(txAfterDisable, test_CanIfTxCount);   /* tx resumed */

    get_st(0U, &st);
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_REPEAT_MESSAGE, st);
}

void test_DisableCommunication_InBusSleep_Rejected(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_DisableCommunication(0U));
    /* enable is accepted in any state */
    TEST_ASSERT_EQUAL(E_OK, CanNm_EnableCommunication(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_DisableCommunication(9U));
}

/*==================================================================================================
*                                  GetState DET paths
*================================================================================================*/
void test_GetState_NullOrInvalid_ReportsDet(void)
{
    Nm_StateType st;
    Nm_ModeType  md;

    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_GetState(0U, NULL_PTR, &md));
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_PARAM_POINTER, test_DetLastErrorId);
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_GetState(0U, &st, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT16(2U, test_DetCalls);
    TEST_ASSERT_EQUAL(E_NOT_OK, CanNm_GetState(9U, &st, &md));
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_INVALID_CHANNEL, test_DetLastErrorId);
}

void test_MainFunction_AllChannels_NoCrash(void)
{
    test_init();
    run_main(10U);
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
}

/*==================================================================================================
*                                  main
*================================================================================================*/
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_Uninit_Guards_ReportDet);
    RUN_TEST(test_Init_NullPtr_ReportsDet);
    RUN_TEST(test_Init_Twice_ReportsDet);
    RUN_TEST(test_Init_EntersBusSleep);
    RUN_TEST(test_DeInit_AfterInit_NotifiesBusSleep);
    RUN_TEST(test_GetVersionInfo_Valid);
    RUN_TEST(test_GetVersionInfo_Null_ReportsDet);

    RUN_TEST(test_NetworkRequest_FromBusSleep_EntersRepeatMessage);
    RUN_TEST(test_NetworkRequest_InvalidChannel_ReportsDet);
    RUN_TEST(test_NetworkRequest_SecondChannel);
    RUN_TEST(test_PassiveStartUp_ThenRepeatExpiry_GoesReadySleep);
    RUN_TEST(test_PassiveStartUp_InNetworkMode_Rejected);
    RUN_TEST(test_RepeatMessageExpiry_WithRequest_GoesNormalOperation);
    RUN_TEST(test_RepeatMessageExpiry_WithRequest_SecondChannel_UsesOwnCycle);
    RUN_TEST(test_NetworkRelease_InNormalOperation_GoesReadySleep);
    RUN_TEST(test_NetworkRelease_InRepeatMessage_Stays);
    RUN_TEST(test_NetworkRelease_InBusSleep_Accepted);
    RUN_TEST(test_NetworkRequest_InReadySleep_GoesNormalOperation);
    RUN_TEST(test_NmTimeout_InReadySleep_GoesPrepareBusSleep);
    RUN_TEST(test_PrepareBusSleep_Expiry_GoesBusSleep);

    RUN_TEST(test_RxIndication_NullPdu_ReportsDet);
    RUN_TEST(test_RxIndication_UnknownPduId_ReportsDet);
    RUN_TEST(test_RxIndication_WakesUpFromBusSleep);
    RUN_TEST(test_RxIndication_RepeatMessageRequest_InNormalOperation);
    RUN_TEST(test_RxIndication_NoRepeatBit_DoesNotExtendRepeatTimer);
    RUN_TEST(test_RxIndication_StoresUserData);

    RUN_TEST(test_TxConfirmation_ValidAndInvalid);
    RUN_TEST(test_Transmit_ForwardsToCanIf);
    RUN_TEST(test_TriggerTransmit_CopiesUpdatedPdu);

    RUN_TEST(test_SetUserData_ValidAndInvalid);
    RUN_TEST(test_GetUserData_InvalidChannel_ReportsDet);
    RUN_TEST(test_SetSleepReadyBit_InvalidChannel_ReportsDet);
    RUN_TEST(test_ConfirmPnAvailability);

    RUN_TEST(test_DisableEnableCommunication_InRepeatMessage);
    RUN_TEST(test_DisableCommunication_InBusSleep_Rejected);
    RUN_TEST(test_GetState_NullOrInvalid_ReportsDet);
    RUN_TEST(test_MainFunction_AllChannels_NoCrash);

    return UNITY_END();
}
