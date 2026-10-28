/**
 * @file test_j1939nm.c
 * @brief J1939Nm (src/bsw/services/j1939nm/src/J1939Nm.c) unit tests — coverage uplift.
 *
 * Compiles the real production source by #including it into this translation
 * unit (white-box, same pattern as tests/unit/services/test_PduR_branch.c) so
 * the static channel-state array and the static AC helpers become accessible:
 *  - lifecycle (Init/DeInit/GetVersionInfo) + DET paths for every public API
 *  - address claiming state machine: IDLE -> WAITING -> CLAIMED -> repeat,
 *    CONFLICT resolution (arbitrary-capable retry / not-capable give-up) and
 *    CANNOT_CLAIM
 *  - NM state machine: WAIT_FOR_AC / NORMAL_OPERATION / BUS_OFF recovery /
 *    TX_CANNOT_CLAIM / invalid-state fallback
 *  - RxIndication PGN dispatch (Address Claimed / Request for AC)
 *
 * NOTE (observed behaviour): J1939Nm_TransmitRequestForAddressClaimed() is
 * never called by the module itself; it is exercised directly here.
 */

// @tests src/bsw/services/j1939nm/src/J1939Nm.c  @tests src/bsw/services/j1939nm/include/J1939Nm.h

#include <string.h>
#include "unity.h"
#include "J1939Nm.h"
#include "Det.h"

/*==================================================================================================
*                                  Det mock
*================================================================================================*/
static uint16 test_DetCalls;
static uint8  test_DetLastApiId;
static uint8  test_DetLastErrorId;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    test_DetCalls++;
    test_DetLastApiId = ApiId;
    test_DetLastErrorId = ErrorId;
    return E_OK;
}

/*==================================================================================================
*                                  Test configuration
*================================================================================================*/
#define TEST_NODE_NAME        ((J1939Nm_NameType)0x0123456789ABCDEFULL)   /* NAME % 100 == 95 */
#define TEST_NODE_NAME_LOWER  ((J1939Nm_NameType)0x0000000000000042ULL)   /* < our NAME  */
#define TEST_NODE_NAME_HIGHER ((J1939Nm_NameType)0xFEDCBA9876543210ULL)   /* > our NAME  */
#define TEST_PREFERRED_ADDR   ((J1939Nm_AddressType)128U)

static const J1939Nm_ChannelConfigType testChannelConfigs[1] =
{
    /* ChannelId, NodeId, Name, Address, PreferredAddress, ArbitraryAddressCapable,
       AcDelayMin, AcDelayMax, AcTimeout, BusOffRecoveryTime */
    { 0U, 0U, TEST_NODE_NAME, TEST_PREFERRED_ADDR, TEST_PREFERRED_ADDR, TRUE,
      50U, 149U, 250U, 100U }
};

static const J1939Nm_ConfigType testNmConfig =
{
    testChannelConfigs,
    1U,     /* NumChannels */
    TRUE,   /* DevErrorDetect */
    TRUE,   /* VersionInfoApi */
    TRUE,   /* NodeDetectionEnabled */
    TRUE    /* NodeMonitoringEnabled */
};

/* same node, but not capable of using an arbitrary address */
static const J1939Nm_ChannelConfigType testChannelConfigsNotCapable[1] =
{
    { 0U, 0U, TEST_NODE_NAME, TEST_PREFERRED_ADDR, TEST_PREFERRED_ADDR, FALSE,
      50U, 149U, 250U, 100U }
};

static const J1939Nm_ConfigType testNmConfigNotCapable =
{
    testChannelConfigsNotCapable,
    1U, TRUE, TRUE, TRUE, TRUE
};

/* Include the production source (white-box). */
#include "src/bsw/services/j1939nm/src/J1939Nm.c"

/*==================================================================================================
*                                  Fixtures
*================================================================================================*/
void setUp(void)
{
    test_DetCalls = 0U;
    test_DetLastApiId = 0xFFU;
    test_DetLastErrorId = 0xFFU;
    J1939Nm_Initialized = FALSE;
    J1939Nm_ConfigPtr = NULL_PTR;
    (void)memset(J1939Nm_ChannelStates, 0, sizeof(J1939Nm_ChannelStates));
}

void tearDown(void)
{
}

static void test_init(void)
{
    J1939Nm_Init(&testNmConfig);
}

static void test_init_claimed(void)
{
    J1939Nm_Init(&testNmConfig);
    J1939Nm_MainFunction();   /* WAIT_FOR_AC + IDLE -> WAITING (loads AcDelayTimer) */
    J1939Nm_ChannelStates[0].AcDelayTimer = 0U;
    J1939Nm_MainFunction();   /* WAITING (delay expired) -> CLAIMED */
}

/*==================================================================================================
*                                  Lifecycle
*================================================================================================*/
void test_Init_NullPtr_ReportsDet(void)
{
    /* NOTE (observed behaviour): Init reports DET via Det_ReportError but has
     * no return value; the module simply stays uninitialized. */
    J1939Nm_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT16(1U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(J1939NM_SID_INIT, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(J1939NM_E_INVALID_POINTER, test_DetLastErrorId);
    TEST_ASSERT_EQUAL(FALSE, J1939Nm_Initialized);
    TEST_ASSERT_NULL(J1939Nm_ConfigPtr);
}

void test_Init_Valid_ResetsChannelState(void)
{
    /* dirty state first to prove Init resets everything */
    J1939Nm_ChannelStates[0].State = J1939NM_STATE_NORMAL_OPERATION;
    J1939Nm_ChannelStates[0].AcState = J1939NM_AC_STATE_CONFLICT;
    J1939Nm_ChannelStates[0].AddressClaimed = TRUE;
    J1939Nm_ChannelStates[0].AcRetryCount = 5U;

    test_init();
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
    TEST_ASSERT_EQUAL(TRUE, J1939Nm_Initialized);
    TEST_ASSERT_EQUAL(J1939Nm_ConfigPtr, &testNmConfig);
    TEST_ASSERT_EQUAL_INT(J1939NM_STATE_WAIT_FOR_AC, J1939Nm_ChannelStates[0].State);
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_IDLE, J1939Nm_ChannelStates[0].AcState);
    TEST_ASSERT_EQUAL_UINT8(TEST_NODE_NAME & 0xFFU, J1939Nm_ChannelStates[0].Name & 0xFFU);
    TEST_ASSERT_EQUAL_UINT8(J1939NM_NULL_ADDRESS, J1939Nm_ChannelStates[0].Address);
    TEST_ASSERT_EQUAL_UINT8(TEST_PREFERRED_ADDR, J1939Nm_ChannelStates[0].PreferredAddress);
    TEST_ASSERT_EQUAL(FALSE, J1939Nm_ChannelStates[0].AddressClaimed);
    TEST_ASSERT_EQUAL(FALSE, J1939Nm_ChannelStates[0].BusOffState);
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Nm_ChannelStates[0].AcDelayTimer);
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Nm_ChannelStates[0].AcTimeoutTimer);
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Nm_ChannelStates[0].BusOffRecoveryTimer);
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Nm_ChannelStates[0].AcRepeatTimer);
    TEST_ASSERT_EQUAL_UINT8(0U, J1939Nm_ChannelStates[0].AcRetryCount);
}

void test_Init_Twice_ReinitializesWithoutDet(void)
{
    test_init_claimed();
    TEST_ASSERT_EQUAL(TRUE, J1939Nm_ChannelStates[0].AddressClaimed);
    /* NOTE (observed behaviour): re-Init while initialized reports no DET and
     * simply resets the channel state */
    test_init();
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
    TEST_ASSERT_EQUAL(FALSE, J1939Nm_ChannelStates[0].AddressClaimed);
    TEST_ASSERT_EQUAL_UINT8(0U, J1939Nm_ChannelStates[0].AcRetryCount);
}

void test_DeInit_AfterInit_ClearsState(void)
{
    test_init_claimed();
    J1939Nm_DeInit();
    TEST_ASSERT_EQUAL(FALSE, J1939Nm_Initialized);
    TEST_ASSERT_NULL(J1939Nm_ConfigPtr);
    TEST_ASSERT_EQUAL_INT(J1939NM_STATE_UNINIT, J1939Nm_ChannelStates[0].State);
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_IDLE, J1939Nm_ChannelStates[0].AcState);
}

void test_DeInit_Uninit_ReportsDet(void)
{
    J1939Nm_DeInit();
    TEST_ASSERT_EQUAL_UINT16(1U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(J1939NM_SID_DEINIT, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(J1939NM_E_NOT_INITIALIZED, test_DetLastErrorId);
}

void test_GetVersionInfo_Valid(void)
{
    Std_VersionInfoType info;

    (void)memset(&info, 0, sizeof(info));
    J1939Nm_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT16(J1939NM_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT16(J1939NM_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(J1939NM_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(J1939NM_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(J1939NM_SW_PATCH_VERSION, info.sw_patch_version);
}

void test_GetVersionInfo_Null_ReportsDet(void)
{
    J1939Nm_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(J1939NM_SID_GET_VERSION_INFO, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(J1939NM_E_INVALID_POINTER, test_DetLastErrorId);
}

/*==================================================================================================
*                                  GetState / GetBusOffState
*================================================================================================*/
void test_GetState_Errors(void)
{
    J1939Nm_StateType state = J1939NM_STATE_UNINIT;

    /* uninit */
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_GetState(0U, &state));
    TEST_ASSERT_EQUAL_UINT8(J1939NM_SID_GET_STATE, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(J1939NM_E_NOT_INITIALIZED, test_DetLastErrorId);

    /* NULL pointer */
    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_GetState(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(J1939NM_E_INVALID_POINTER, test_DetLastErrorId);

    /* invalid channel */
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_GetState(J1939NM_NUMBER_OF_CHANNELS, &state));
    TEST_ASSERT_EQUAL_UINT8(J1939NM_E_INVALID_PARAMETER, test_DetLastErrorId);
    TEST_ASSERT_EQUAL_UINT16(3U, test_DetCalls);
}

void test_GetState_Valid(void)
{
    J1939Nm_StateType state = J1939NM_STATE_UNINIT;

    test_init();
    TEST_ASSERT_EQUAL(E_OK, J1939Nm_GetState(0U, &state));
    TEST_ASSERT_EQUAL_INT(J1939NM_STATE_WAIT_FOR_AC, state);
}

void test_GetBusOffState_Errors_AndValid(void)
{
    boolean busOff = TRUE;

    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_GetBusOffState(0U, &busOff));
    TEST_ASSERT_EQUAL_UINT8(J1939NM_SID_GET_BUS_OFF_STATE, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(J1939NM_E_NOT_INITIALIZED, test_DetLastErrorId);

    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_GetBusOffState(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(J1939NM_E_INVALID_POINTER, test_DetLastErrorId);
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_GetBusOffState(J1939NM_NUMBER_OF_CHANNELS, &busOff));
    TEST_ASSERT_EQUAL_UINT8(J1939NM_E_INVALID_PARAMETER, test_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_OK, J1939Nm_GetBusOffState(0U, &busOff));
    TEST_ASSERT_EQUAL(FALSE, busOff);
}

/*==================================================================================================
*                                  SetBusOffState / BusOff recovery
*================================================================================================*/
void test_SetBusOffState_Errors(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_SetBusOffState(0U, TRUE));
    TEST_ASSERT_EQUAL_UINT8(J1939NM_SID_SET_BUS_OFF_STATE, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(J1939NM_E_NOT_INITIALIZED, test_DetLastErrorId);

    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_SetBusOffState(J1939NM_NUMBER_OF_CHANNELS, TRUE));
    TEST_ASSERT_EQUAL_UINT8(J1939NM_E_INVALID_PARAMETER, test_DetLastErrorId);
}

void test_SetBusOffState_True_EntersBusOff(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_OK, J1939Nm_SetBusOffState(0U, TRUE));
    TEST_ASSERT_EQUAL(TRUE, J1939Nm_ChannelStates[0].BusOffState);
    TEST_ASSERT_EQUAL_INT(J1939NM_STATE_BUS_OFF, J1939Nm_ChannelStates[0].State);
    /* 1000 ms / 10 ms period */
    TEST_ASSERT_EQUAL_UINT16(100U, J1939Nm_ChannelStates[0].BusOffRecoveryTimer);
}

void test_SetBusOffState_False_OnlyClearsFlag(void)
{
    test_init();
    J1939Nm_SetBusOffState(0U, TRUE);
    /* NOTE (observed behaviour): clearing the bus-off flag does not restore
     * the NM state by itself — recovery runs through MainFunction */
    TEST_ASSERT_EQUAL(E_OK, J1939Nm_SetBusOffState(0U, FALSE));
    TEST_ASSERT_EQUAL(FALSE, J1939Nm_ChannelStates[0].BusOffState);
    TEST_ASSERT_EQUAL_INT(J1939NM_STATE_BUS_OFF, J1939Nm_ChannelStates[0].State);
}

void test_MainFunction_BusOff_RecoversToWaitForAc(void)
{
    test_init();
    J1939Nm_SetBusOffState(0U, TRUE);
    J1939Nm_ChannelStates[0].BusOffRecoveryTimer = 1U;
    J1939Nm_MainFunction();   /* timer hits 0 */
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Nm_ChannelStates[0].BusOffRecoveryTimer);
    J1939Nm_MainFunction();   /* recovery expired -> restart claiming */
    TEST_ASSERT_EQUAL(FALSE, J1939Nm_ChannelStates[0].BusOffState);
    TEST_ASSERT_EQUAL_INT(J1939NM_STATE_WAIT_FOR_AC, J1939Nm_ChannelStates[0].State);
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_IDLE, J1939Nm_ChannelStates[0].AcState);
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Nm_ChannelStates[0].BusOffRecoveryTimer);
}

void test_MainFunction_BusOff_TimerCountsDown(void)
{
    test_init();
    J1939Nm_SetBusOffState(0U, TRUE);
    J1939Nm_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(99U, J1939Nm_ChannelStates[0].BusOffRecoveryTimer);
    TEST_ASSERT_EQUAL_INT(J1939NM_STATE_BUS_OFF, J1939Nm_ChannelStates[0].State);
}

/*==================================================================================================
*                                  Address / NAME accessors
*================================================================================================*/
void test_GetAddress_Errors(void)
{
    J1939Nm_AddressType address = 0U;

    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_GetAddress(0U, &address));
    TEST_ASSERT_EQUAL_UINT8(J1939NM_SID_GET_ADDRESS, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(J1939NM_E_NOT_INITIALIZED, test_DetLastErrorId);

    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_GetAddress(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(J1939NM_E_INVALID_POINTER, test_DetLastErrorId);
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_GetAddress(J1939NM_NUMBER_OF_CHANNELS, &address));
    TEST_ASSERT_EQUAL_UINT8(J1939NM_E_INVALID_PARAMETER, test_DetLastErrorId);
}

void test_GetSetAddress_Valid(void)
{
    J1939Nm_AddressType address = 0U;

    test_init();
    TEST_ASSERT_EQUAL(E_OK, J1939Nm_SetAddress(0U, 200U));
    TEST_ASSERT_EQUAL(E_OK, J1939Nm_GetAddress(0U, &address));
    TEST_ASSERT_EQUAL_UINT8(200U, address);
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_SetAddress(J1939NM_NUMBER_OF_CHANNELS, 1U));
}

void test_GetName_Errors(void)
{
    J1939Nm_NameType name = 0U;

    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_GetName(0U, &name));
    TEST_ASSERT_EQUAL_UINT8(J1939NM_SID_GET_NAME, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(J1939NM_E_NOT_INITIALIZED, test_DetLastErrorId);

    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_GetName(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(J1939NM_E_INVALID_POINTER, test_DetLastErrorId);
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_GetName(J1939NM_NUMBER_OF_CHANNELS, &name));
    TEST_ASSERT_EQUAL_UINT8(J1939NM_E_INVALID_PARAMETER, test_DetLastErrorId);
}

void test_GetSetName_Valid(void)
{
    const J1939Nm_NameType configuredName = TEST_NODE_NAME;
    const J1939Nm_NameType changedName = TEST_NODE_NAME_HIGHER;
    J1939Nm_NameType name = 0U;

    test_init();
    TEST_ASSERT_EQUAL(E_OK, J1939Nm_GetName(0U, &name));
    TEST_ASSERT_EQUAL_MEMORY(&configuredName, &name, sizeof(name));

    TEST_ASSERT_EQUAL(E_OK, J1939Nm_SetName(0U, TEST_NODE_NAME_HIGHER));
    TEST_ASSERT_EQUAL(E_OK, J1939Nm_GetName(0U, &name));
    TEST_ASSERT_EQUAL_MEMORY(&changedName, &name, sizeof(name));
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_SetName(J1939NM_NUMBER_OF_CHANNELS, 1U));
}

/*==================================================================================================
*                                  MainFunction / claiming state machine
*================================================================================================*/
void test_MainFunction_Uninit_NoOp(void)
{
    J1939Nm_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
}

void test_MainFunction_ComputesAcDelay_ThenClaims(void)
{
    test_init();
    J1939Nm_MainFunction();   /* IDLE -> WAITING, delay randomized */
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_WAITING, J1939Nm_ChannelStates[0].AcState);
    /* AcDelayMin + ((Name % 100) % (AcDelayMax - AcDelayMin + 1)) = 50 + 95 */
    TEST_ASSERT_EQUAL_UINT16(145U, J1939Nm_ChannelStates[0].AcDelayTimer);
    TEST_ASSERT_EQUAL_INT(J1939NM_STATE_WAIT_FOR_AC, J1939Nm_ChannelStates[0].State);

    J1939Nm_ChannelStates[0].AcDelayTimer = 1U;
    J1939Nm_MainFunction();   /* timer hits 0 */
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Nm_ChannelStates[0].AcDelayTimer);
    J1939Nm_MainFunction();   /* delay expired -> claim */
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_CLAIMED, J1939Nm_ChannelStates[0].AcState);
    TEST_ASSERT_EQUAL(TRUE, J1939Nm_ChannelStates[0].AddressClaimed);
    TEST_ASSERT_EQUAL_UINT8(TEST_PREFERRED_ADDR, J1939Nm_ChannelStates[0].Address);
    /* 1000 ms / 10 ms period */
    TEST_ASSERT_EQUAL_UINT16(100U, J1939Nm_ChannelStates[0].AcRepeatTimer);
    TEST_ASSERT_EQUAL_INT(J1939NM_STATE_NORMAL_OPERATION, J1939Nm_ChannelStates[0].State);
}

void test_MainFunction_Claimed_RepeatsAfterRepeatTimer(void)
{
    test_init_claimed();
    J1939Nm_ChannelStates[0].AcRepeatTimer = 2U;
    J1939Nm_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(1U, J1939Nm_ChannelStates[0].AcRepeatTimer);
    J1939Nm_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Nm_ChannelStates[0].AcRepeatTimer);
    J1939Nm_MainFunction();   /* timer 0 -> repeat claim + reload */
    TEST_ASSERT_EQUAL_UINT16(100U, J1939Nm_ChannelStates[0].AcRepeatTimer);
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_CLAIMED, J1939Nm_ChannelStates[0].AcState);
}

void test_MainFunction_WaitForAc_TransitionsToTxCannotClaim(void)
{
    test_init();
    J1939Nm_ChannelStates[0].AcState = J1939NM_AC_STATE_CANNOT_CLAIM;
    J1939Nm_MainFunction();
    TEST_ASSERT_EQUAL_INT(J1939NM_STATE_TX_CANNOT_CLAIM, J1939Nm_ChannelStates[0].State);
}

void test_MainFunction_TxCannotClaim_Stays(void)
{
    test_init();
    J1939Nm_ChannelStates[0].State = J1939NM_STATE_TX_CANNOT_CLAIM;
    J1939Nm_MainFunction();
    TEST_ASSERT_EQUAL_INT(J1939NM_STATE_TX_CANNOT_CLAIM, J1939Nm_ChannelStates[0].State);
}

void test_MainFunction_InvalidState_ResetsToUninit(void)
{
    test_init();
    J1939Nm_ChannelStates[0].State = (J1939Nm_StateType)0x63U;   /* garbage */
    J1939Nm_MainFunction();
    TEST_ASSERT_EQUAL_INT(J1939NM_STATE_UNINIT, J1939Nm_ChannelStates[0].State);
    /* NOTE (observed behaviour): once reset to UNINIT, MainFunction leaves the
     * channel there (UNINIT case does nothing) */
    J1939Nm_MainFunction();
    TEST_ASSERT_EQUAL_INT(J1939NM_STATE_UNINIT, J1939Nm_ChannelStates[0].State);
}

void test_ProcessAddressClaiming_InvalidAcState_ResetsToIdle(void)
{
    test_init();
    J1939Nm_ChannelStates[0].AcState = (J1939Nm_AcStateType)0x55U;   /* garbage */
    J1939Nm_MainFunction();
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_IDLE, J1939Nm_ChannelStates[0].AcState);
}

/*==================================================================================================
*                                  Conflict handling
*================================================================================================*/
void test_HandleAddressConflict_LowerName_KeepsClaim(void)
{
    test_init_claimed();
    /* received NAME higher -> ours is lower -> higher priority, keep address */
    J1939Nm_HandleAddressConflict(0U, TEST_NODE_NAME_HIGHER, 5U);
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_CLAIMED, J1939Nm_ChannelStates[0].AcState);
    TEST_ASSERT_EQUAL_UINT8(TEST_PREFERRED_ADDR, J1939Nm_ChannelStates[0].Address);
    TEST_ASSERT_EQUAL(TRUE, J1939Nm_ChannelStates[0].AddressClaimed);
}

void test_HandleAddressConflict_HigherName_EntersConflict(void)
{
    test_init_claimed();
    /* received NAME lower -> ours is higher -> lower priority, release */
    J1939Nm_HandleAddressConflict(0U, TEST_NODE_NAME_LOWER, 5U);
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_CONFLICT, J1939Nm_ChannelStates[0].AcState);
}

void test_HandleAddressConflict_SameName_EntersConflict(void)
{
    test_init_claimed();
    /* NOTE (observed behaviour): identical NAME is treated as a conflict too */
    J1939Nm_HandleAddressConflict(0U, TEST_NODE_NAME, 5U);
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_CONFLICT, J1939Nm_ChannelStates[0].AcState);
}

void test_HandleAddressConflict_Guards(void)
{
    test_init_claimed();
    /* uninit */
    J1939Nm_Initialized = FALSE;
    J1939Nm_HandleAddressConflict(0U, TEST_NODE_NAME_LOWER, 5U);
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_CLAIMED, J1939Nm_ChannelStates[0].AcState);

    /* invalid channel */
    J1939Nm_Initialized = TRUE;
    J1939Nm_HandleAddressConflict(J1939NM_NUMBER_OF_CHANNELS, TEST_NODE_NAME_LOWER, 5U);
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_CLAIMED, J1939Nm_ChannelStates[0].AcState);
}

void test_Conflict_ArbitraryCapable_TriesAlternateAddress(void)
{
    test_init_claimed();
    J1939Nm_ChannelStates[0].AcState = J1939NM_AC_STATE_CONFLICT;
    J1939Nm_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, J1939Nm_ChannelStates[0].AcRetryCount);
    /* (128 + 1) % 254 */
    TEST_ASSERT_EQUAL_UINT8(129U, J1939Nm_ChannelStates[0].Address);
    TEST_ASSERT_EQUAL_UINT16(50U, J1939Nm_ChannelStates[0].AcDelayTimer);   /* AcDelayMin */
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_WAITING, J1939Nm_ChannelStates[0].AcState);
}

void test_Conflict_RetryExhausted_CannotClaim(void)
{
    test_init_claimed();
    J1939Nm_ChannelStates[0].AcState = J1939NM_AC_STATE_CONFLICT;
    J1939Nm_ChannelStates[0].AcRetryCount = 10U;
    J1939Nm_MainFunction();
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_CANNOT_CLAIM, J1939Nm_ChannelStates[0].AcState);

    /* next cycle applies the CANNOT_CLAIM handling */
    J1939Nm_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(J1939NM_NULL_ADDRESS, J1939Nm_ChannelStates[0].Address);
    TEST_ASSERT_EQUAL(FALSE, J1939Nm_ChannelStates[0].AddressClaimed);
    /* NOTE (observed behaviour): from NORMAL_OPERATION the NM state does NOT
     * move to TX_CANNOT_CLAIM — only WAIT_FOR_AC does */
    TEST_ASSERT_EQUAL_INT(J1939NM_STATE_NORMAL_OPERATION, J1939Nm_ChannelStates[0].State);
}

void test_Conflict_NotCapable_ImmediatelyCannotClaim(void)
{
    J1939Nm_Init(&testNmConfigNotCapable);
    J1939Nm_MainFunction();   /* IDLE -> WAITING */
    J1939Nm_ChannelStates[0].AcDelayTimer = 0U;
    J1939Nm_MainFunction();   /* WAITING (expired) -> CLAIMED */
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_CLAIMED, J1939Nm_ChannelStates[0].AcState);

    J1939Nm_HandleAddressConflict(0U, TEST_NODE_NAME_LOWER, 5U);
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_CONFLICT, J1939Nm_ChannelStates[0].AcState);
    J1939Nm_MainFunction();   /* not arbitrary-capable -> give up */
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_CANNOT_CLAIM, J1939Nm_ChannelStates[0].AcState);
    J1939Nm_MainFunction();   /* CANNOT_CLAIM handling */
    TEST_ASSERT_EQUAL_UINT8(J1939NM_NULL_ADDRESS, J1939Nm_ChannelStates[0].Address);
    TEST_ASSERT_EQUAL(FALSE, J1939Nm_ChannelStates[0].AddressClaimed);
}

/*==================================================================================================
*                                  RxIndication
*================================================================================================*/
static void test_send_ac_rx(J1939Nm_NameType receivedName, J1939Nm_AddressType source)
{
    uint8 data[8];
    uint32 canId = J1939NM_PDU_ADDRESS_CLAIMED | (uint32)source;

    for (uint8 i = 0U; i < 8U; i++)
    {
        data[i] = (uint8)((receivedName >> (i * 8U)) & 0xFFU);
    }
    J1939Nm_RxIndication(0U, canId, data, sizeof(data));
}

void test_RxIndication_Guards(void)
{
    uint8 data[8] = { 0U };

    /* uninit */
    J1939Nm_RxIndication(0U, J1939NM_PDU_ADDRESS_CLAIMED, data, sizeof(data));
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);

    test_init();
    /* NULL data */
    J1939Nm_RxIndication(0U, J1939NM_PDU_ADDRESS_CLAIMED, NULL_PTR, sizeof(data));
    /* invalid channel */
    J1939Nm_RxIndication(J1939NM_NUMBER_OF_CHANNELS, J1939NM_PDU_ADDRESS_CLAIMED, data, sizeof(data));
    /* no crash, no state change */
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_IDLE, J1939Nm_ChannelStates[0].AcState);
}

void test_RxIndication_AddressClaimed_SameSource_HigherName_TriggersConflict(void)
{
    test_init_claimed();
    TEST_ASSERT_EQUAL_UINT8(TEST_PREFERRED_ADDR, J1939Nm_ChannelStates[0].Address);
    /* same source address as ours, NAME lower than ours -> we lose */
    test_send_ac_rx(TEST_NODE_NAME_LOWER, TEST_PREFERRED_ADDR);
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_CONFLICT, J1939Nm_ChannelStates[0].AcState);
}

void test_RxIndication_AddressClaimed_SameSource_LowerName_KeepsClaim(void)
{
    test_init_claimed();
    test_send_ac_rx(TEST_NODE_NAME_HIGHER, TEST_PREFERRED_ADDR);
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_CLAIMED, J1939Nm_ChannelStates[0].AcState);
}

void test_RxIndication_AddressClaimed_OtherSource_Ignored(void)
{
    test_init_claimed();
    test_send_ac_rx(TEST_NODE_NAME_LOWER, 77U);   /* not our address */
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_CLAIMED, J1939Nm_ChannelStates[0].AcState);
}

void test_RxIndication_RequestForAc_WithClaimedAddress_Retransmits(void)
{
    uint8 data[8] = { 0U };

    test_init_claimed();
    /* global request -> we retransmit our claim (state unchanged) */
    J1939Nm_RxIndication(0U, J1939NM_PDU_REQUEST_FOR_AC, data, sizeof(data));
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_CLAIMED, J1939Nm_ChannelStates[0].AcState);
    TEST_ASSERT_EQUAL(TRUE, J1939Nm_ChannelStates[0].AddressClaimed);
}

void test_RxIndication_RequestForAc_WithoutClaimedAddress_Ignored(void)
{
    uint8 data[8] = { 0U };

    test_init();
    J1939Nm_RxIndication(0U, J1939NM_PDU_REQUEST_FOR_AC, data, sizeof(data));
    TEST_ASSERT_EQUAL(FALSE, J1939Nm_ChannelStates[0].AddressClaimed);
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_IDLE, J1939Nm_ChannelStates[0].AcState);
}

void test_RxIndication_UnknownPgn_Ignored(void)
{
    uint8 data[8] = { 0U };

    test_init_claimed();
    J1939Nm_RxIndication(0U, 0x00F900U, data, sizeof(data));
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_CLAIMED, J1939Nm_ChannelStates[0].AcState);
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
}

/*==================================================================================================
*                                  Requests / callbacks / white-box helpers
*================================================================================================*/
void test_RequestAddressClaimed_Errors_AndValid(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_RequestAddressClaimed(0U));   /* uninit */
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);   /* NOTE: silent, no DET */

    test_init();
    TEST_ASSERT_EQUAL(E_OK, J1939Nm_RequestAddressClaimed(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_RequestAddressClaimed(J1939NM_NUMBER_OF_CHANNELS));
}

void test_RequestCannotClaimAddress_Errors_AndValid(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_RequestCannotClaimAddress(0U));   /* uninit */

    test_init();
    TEST_ASSERT_EQUAL(E_OK, J1939Nm_RequestCannotClaimAddress(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_RequestCannotClaimAddress(J1939NM_NUMBER_OF_CHANNELS));
}

void test_BusOffCbk_SetsBusOff(void)
{
    test_init();
    J1939Nm_BusOffCbk(0U);
    TEST_ASSERT_EQUAL(TRUE, J1939Nm_ChannelStates[0].BusOffState);
    TEST_ASSERT_EQUAL_INT(J1939NM_STATE_BUS_OFF, J1939Nm_ChannelStates[0].State);
    TEST_ASSERT_EQUAL_UINT16(100U, J1939Nm_ChannelStates[0].BusOffRecoveryTimer);

    /* invalid channel / uninit: no effect */
    J1939Nm_BusOffCbk(J1939NM_NUMBER_OF_CHANNELS);
    J1939Nm_Initialized = FALSE;
    J1939Nm_BusOffCbk(0U);
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
}

void test_TxConfirmation_IsNoOp(void)
{
    test_init_claimed();
    J1939Nm_TxConfirmation(0U, 42U, E_OK);
    J1939Nm_TxConfirmation(J1939NM_NUMBER_OF_CHANNELS, 42U, E_NOT_OK);
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
    TEST_ASSERT_EQUAL_INT(J1939NM_AC_STATE_CLAIMED, J1939Nm_ChannelStates[0].AcState);
}

void test_ValidateChannel_AndTransmitHelpers(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_OK, J1939Nm_ValidateChannel(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_ValidateChannel(J1939NM_NUMBER_OF_CHANNELS));
    TEST_ASSERT_EQUAL(E_OK, J1939Nm_TransmitAddressClaimed(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_TransmitAddressClaimed(J1939NM_NUMBER_OF_CHANNELS));
    TEST_ASSERT_EQUAL(E_OK, J1939Nm_TransmitCannotClaimAddress(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_TransmitCannotClaimAddress(J1939NM_NUMBER_OF_CHANNELS));
    /* NOTE (observed behaviour): TransmitRequestForAddressClaimed is dead code
     * in the module — no caller exists; exercised directly here */
    TEST_ASSERT_EQUAL(E_OK, J1939Nm_TransmitRequestForAddressClaimed(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Nm_TransmitRequestForAddressClaimed(J1939NM_NUMBER_OF_CHANNELS));
}

void test_CompareNames_ThreeOutcomes(void)
{
    TEST_ASSERT_EQUAL_INT(-1, J1939Nm_CompareNames(1U, 2U));
    TEST_ASSERT_EQUAL_INT(0, J1939Nm_CompareNames(0x1234U, 0x1234U));
    TEST_ASSERT_EQUAL_INT(1, J1939Nm_CompareNames(2U, 1U));
}

void test_BuildAcPdu_LittleEndianLayout(void)
{
    uint8 data[8];

    (void)memset(data, 0xAA, sizeof(data));
    J1939Nm_BuildAcPdu(data, TEST_NODE_NAME);
    TEST_ASSERT_EQUAL_UINT8(0xEFU, data[0]);   /* 0x...89ABCDEF */
    TEST_ASSERT_EQUAL_UINT8(0xCDU, data[1]);
    TEST_ASSERT_EQUAL_UINT8(0xABU, data[2]);
    TEST_ASSERT_EQUAL_UINT8(0x89U, data[3]);
    TEST_ASSERT_EQUAL_UINT8(0x67U, data[4]);
    TEST_ASSERT_EQUAL_UINT8(0x45U, data[5]);
    TEST_ASSERT_EQUAL_UINT8(0x23U, data[6]);
    TEST_ASSERT_EQUAL_UINT8(0x01U, data[7]);

    /* NULL is handled defensively */
    J1939Nm_BuildAcPdu(NULL_PTR, TEST_NODE_NAME);
}

/*==================================================================================================
*                                  MAIN
*================================================================================================*/
int main(void)
{
    UNITY_BEGIN();

    /* Lifecycle */
    RUN_TEST(test_Init_NullPtr_ReportsDet);
    RUN_TEST(test_Init_Valid_ResetsChannelState);
    RUN_TEST(test_Init_Twice_ReinitializesWithoutDet);
    RUN_TEST(test_DeInit_AfterInit_ClearsState);
    RUN_TEST(test_DeInit_Uninit_ReportsDet);
    RUN_TEST(test_GetVersionInfo_Valid);
    RUN_TEST(test_GetVersionInfo_Null_ReportsDet);

    /* GetState / GetBusOffState */
    RUN_TEST(test_GetState_Errors);
    RUN_TEST(test_GetState_Valid);
    RUN_TEST(test_GetBusOffState_Errors_AndValid);

    /* Bus-off */
    RUN_TEST(test_SetBusOffState_Errors);
    RUN_TEST(test_SetBusOffState_True_EntersBusOff);
    RUN_TEST(test_SetBusOffState_False_OnlyClearsFlag);
    RUN_TEST(test_MainFunction_BusOff_RecoversToWaitForAc);
    RUN_TEST(test_MainFunction_BusOff_TimerCountsDown);

    /* Address / NAME */
    RUN_TEST(test_GetAddress_Errors);
    RUN_TEST(test_GetSetAddress_Valid);
    RUN_TEST(test_GetName_Errors);
    RUN_TEST(test_GetSetName_Valid);

    /* Claiming state machine */
    RUN_TEST(test_MainFunction_Uninit_NoOp);
    RUN_TEST(test_MainFunction_ComputesAcDelay_ThenClaims);
    RUN_TEST(test_MainFunction_Claimed_RepeatsAfterRepeatTimer);
    RUN_TEST(test_MainFunction_WaitForAc_TransitionsToTxCannotClaim);
    RUN_TEST(test_MainFunction_TxCannotClaim_Stays);
    RUN_TEST(test_MainFunction_InvalidState_ResetsToUninit);
    RUN_TEST(test_ProcessAddressClaiming_InvalidAcState_ResetsToIdle);

    /* Conflict */
    RUN_TEST(test_HandleAddressConflict_LowerName_KeepsClaim);
    RUN_TEST(test_HandleAddressConflict_HigherName_EntersConflict);
    RUN_TEST(test_HandleAddressConflict_SameName_EntersConflict);
    RUN_TEST(test_HandleAddressConflict_Guards);
    RUN_TEST(test_Conflict_ArbitraryCapable_TriesAlternateAddress);
    RUN_TEST(test_Conflict_RetryExhausted_CannotClaim);
    RUN_TEST(test_Conflict_NotCapable_ImmediatelyCannotClaim);

    /* RxIndication */
    RUN_TEST(test_RxIndication_Guards);
    RUN_TEST(test_RxIndication_AddressClaimed_SameSource_HigherName_TriggersConflict);
    RUN_TEST(test_RxIndication_AddressClaimed_SameSource_LowerName_KeepsClaim);
    RUN_TEST(test_RxIndication_AddressClaimed_OtherSource_Ignored);
    RUN_TEST(test_RxIndication_RequestForAc_WithClaimedAddress_Retransmits);
    RUN_TEST(test_RxIndication_RequestForAc_WithoutClaimedAddress_Ignored);
    RUN_TEST(test_RxIndication_UnknownPgn_Ignored);

    /* Requests / callbacks / helpers */
    RUN_TEST(test_RequestAddressClaimed_Errors_AndValid);
    RUN_TEST(test_RequestCannotClaimAddress_Errors_AndValid);
    RUN_TEST(test_BusOffCbk_SetsBusOff);
    RUN_TEST(test_TxConfirmation_IsNoOp);
    RUN_TEST(test_ValidateChannel_AndTransmitHelpers);
    RUN_TEST(test_CompareNames_ThreeOutcomes);
    RUN_TEST(test_BuildAcPdu_LittleEndianLayout);

    return UNITY_END();
}
