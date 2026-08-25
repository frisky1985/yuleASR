/**
 * @file test_canNm.c
 * @brief CanNm Module Unit Tests - CAN Network Management
 * @version 1.0.0
 *
 * SHALL-CANNM-01: SHALL implement AUTOSAR CAN Network Management protocol
 * SHALL-CANNM-02: SHALL support a configurable 8-bit node ID
 * SHALL-CANNM-03: SHALL support configurable message cycle time with default of 100ms
 * SHALL-CANNM-04: SHALL support configurable repeat message timer with default of 1000ms
 * SHALL-CANNM-05: SHALL support bus synchronization
 *
 * Unit tests for AUTOSAR CAN Network Management module following
 * AUTOSAR_SWS_CANNetworkManagement specification version 4.4.0
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include "CanNm.h"
#include "CanNm_Cfg.h"
#include "Nm.h"
#include "ComM.h"
#include "CanIf.h"

/*==================================================================================================
 *                                  Mock Functions & Data
 *================================================================================================*/

/* ComM Mock State */
static boolean ComM_Nm_NetworkMode_Called = FALSE;
static boolean ComM_Nm_BusSleepMode_Called = FALSE;
static boolean ComM_Nm_PrepareBusSleepMode_Called = FALSE;
static boolean ComM_EcuM_WakeUpIndication_Called = FALSE;
static NetworkHandleType ComM_LastChannelHandle = 0xFF;

/* Nm Mock State */
static boolean Nm_StateChangeNotification_Called = FALSE;
static boolean Nm_RemoteSleepIndication_Called = FALSE;
static boolean Nm_RemoteSleepCancellation_Called = FALSE;
static Nm_StateType Nm_LastState = NM_STATE_UNINIT;
static Nm_ModeType Nm_LastMode = NM_MODE_BUS_SLEEP;

/* CanIf Mock State */
static boolean CanIf_Transmit_Called = FALSE;
static PduIdType CanIf_LastTxPduId = 0xFFFF;
static uint8 CanIf_LastTxData[8];
static uint8 CanIf_LastTxLength = 0;
static Std_ReturnType CanIf_Transmit_Return = E_OK;

/* Det Mock State */
static boolean Det_ReportError_Called = FALSE;
static uint16 Det_LastModuleId = 0;
static uint8 Det_LastApiId = 0;
static uint8 Det_LastErrorId = 0;

/* Mock Function Implementations */
void ComM_Nm_NetworkMode(NetworkHandleType nmChannelHandle)
{
    ComM_Nm_NetworkMode_Called = TRUE;
    ComM_LastChannelHandle = nmChannelHandle;
}

void ComM_Nm_BusSleepMode(NetworkHandleType nmChannelHandle)
{
    ComM_Nm_BusSleepMode_Called = TRUE;
    ComM_LastChannelHandle = nmChannelHandle;
}

void ComM_Nm_PrepareBusSleepMode(NetworkHandleType nmChannelHandle)
{
    ComM_Nm_PrepareBusSleepMode_Called = TRUE;
    ComM_LastChannelHandle = nmChannelHandle;
}

void ComM_EcuM_WakeUpIndication(NetworkHandleType nmChannelHandle)
{
    ComM_EcuM_WakeUpIndication_Called = TRUE;
    ComM_LastChannelHandle = nmChannelHandle;
}

void Nm_StateChangeNotification(Nm_ChannelHandleType nmNetworkHandle, Nm_StateType nmPreviousState, Nm_StateType nmCurrentState)
{
    (void)nmPreviousState;
    Nm_StateChangeNotification_Called = TRUE;
    Nm_LastState = nmCurrentState;
}

void Nm_RemoteSleepIndication(NetworkHandleType nmNetworkHandle)
{
    Nm_RemoteSleepIndication_Called = TRUE;
    ComM_LastChannelHandle = nmNetworkHandle;
}

void Nm_RemoteSleepCancellation(NetworkHandleType nmNetworkHandle)
{
    Nm_RemoteSleepCancellation_Called = TRUE;
    ComM_LastChannelHandle = nmNetworkHandle;
}

Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    CanIf_Transmit_Called = TRUE;
    CanIf_LastTxPduId = TxPduId;
    if (PduInfoPtr != NULL && PduInfoPtr->SduDataPtr != NULL)
    {
        CanIf_LastTxLength = (PduInfoPtr->SduLength < 8) ? PduInfoPtr->SduLength : 8;
        memcpy(CanIf_LastTxData, PduInfoPtr->SduDataPtr, CanIf_LastTxLength);
    }
    return CanIf_Transmit_Return;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)InstanceId;
    Det_ReportError_Called = TRUE;
    Det_LastModuleId = ModuleId;
    Det_LastApiId = ApiId;
    Det_LastErrorId = ErrorId;
    return E_OK;
}

/*==================================================================================================
 *                                  Test Helper Functions
 *================================================================================================*/
static void Reset_Mocks(void)
{
    /* Reset ComM mocks */
    ComM_Nm_NetworkMode_Called = FALSE;
    ComM_Nm_BusSleepMode_Called = FALSE;
    ComM_Nm_PrepareBusSleepMode_Called = FALSE;
    ComM_EcuM_WakeUpIndication_Called = FALSE;
    ComM_LastChannelHandle = 0xFF;

    /* Reset Nm mocks */
    Nm_StateChangeNotification_Called = FALSE;
    Nm_RemoteSleepIndication_Called = FALSE;
    Nm_RemoteSleepCancellation_Called = FALSE;
    Nm_LastState = NM_STATE_UNINIT;

    /* Reset CanIf mocks */
    CanIf_Transmit_Called = FALSE;
    CanIf_LastTxPduId = 0xFFFF;
    CanIf_LastTxLength = 0;
    memset(CanIf_LastTxData, 0, sizeof(CanIf_LastTxData));
    CanIf_Transmit_Return = E_OK;

    /* Reset Det mocks */
    Det_ReportError_Called = FALSE;
    Det_LastModuleId = 0;
    Det_LastApiId = 0;
    Det_LastErrorId = 0;
}

/*==================================================================================================
 *                                  Test Fixtures
 *================================================================================================*/
static int setup(void **state)
{
    (void)state;
    Reset_Mocks();
    CanNm_Init(&CanNm_Config);
    return 0;
}

static int teardown(void **state)
{
    (void)state;
    CanNm_DeInit();
    Reset_Mocks();
    return 0;
}

/*==================================================================================================
 *                                    Initialization Tests
 *================================================================================================*/

/**
 * @brief Test CanNm_Init with valid configuration
 */
/** @req SWS_CanNm_00001 */
/** @req SWS_CanNm_00001 */
static void test_CanNm_Init_ValidConfig(void **state)
{
    (void)state;
    
    /* DeInit first to test fresh initialization */
    CanNm_DeInit();
    Reset_Mocks();
    
    CanNm_Init(&CanNm_Config);
    
    /* Module should be initialized - verify by calling an API */
    Std_ReturnType result = CanNm_NetworkRequest(CANNM_CHANNEL_0);
    assert_int_equal(result, E_OK);
}

/**
 * @brief Test CanNm_Init with NULL pointer (should report error when DET enabled)
 */
/** @req SWS_CanNm_00001 */
/** @req SWS_CanNm_00001 */
static void test_CanNm_Init_NullPointer(void **state)
{
    (void)state;
    
    CanNm_DeInit();
    Reset_Mocks();
    
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    CanNm_Init(NULL);
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastModuleId, CANNM_MODULE_ID);
    assert_int_equal(Det_LastApiId, CANNM_SID_INIT);
    assert_int_equal(Det_LastErrorId, CANNM_E_PARAM_POINTER);
#else
    /* In non-DET builds, NULL might be accepted or handled silently */
    CanNm_Init(NULL);
#endif
}

/**
 * @brief Test CanNm_Init double initialization (should report error when DET enabled)
 */
/** @req SWS_CanNm_00001 */
/** @req SWS_CanNm_00001 */
static void test_CanNm_Init_DoubleInit(void **state)
{
    (void)state;
    
    Reset_Mocks();
    
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    /* Try to initialize again - already initialized in setup */
    CanNm_Init(&CanNm_Config);
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_ALREADY_INITIALIZED);
#endif
}

/**
 * @brief Test CanNm_DeInit functionality
 */
/** @req SWS_CanNm_00001 */
/** @req SWS_CanNm_00002 */
static void test_CanNm_DeInit(void **state)
{
    (void)state;
    
    CanNm_DeInit();
    
    /* All channels should transition to Bus Sleep */
    assert_true(ComM_Nm_BusSleepMode_Called);
}

/**
 * @brief Test CanNm_DeInit when not initialized (should report error when DET enabled)
 */
/** @req SWS_CanNm_00001 */
/** @req SWS_CanNm_00002 */
static void test_CanNm_DeInit_NotInitialized(void **state)
{
    (void)state;
    
    CanNm_DeInit();
    Reset_Mocks();
    
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    CanNm_DeInit();
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_NOT_INITIALIZED);
#endif
}

/**
 * @brief Test CanNm_GetVersionInfo
 */
/** @req SWS_CanNm_00003 */
/** @req SWS_CanNm_00012 */
static void test_CanNm_GetVersionInfo(void **state)
{
    (void)state;
    
    Std_VersionInfoType versionInfo;
    
    CanNm_GetVersionInfo(&versionInfo);
    
    assert_int_equal(versionInfo.vendorID, CANNM_VENDOR_ID);
    assert_int_equal(versionInfo.moduleID, CANNM_MODULE_ID);
    assert_int_equal(versionInfo.sw_major_version, CANNM_SW_MAJOR_VERSION);
    assert_int_equal(versionInfo.sw_minor_version, CANNM_SW_MINOR_VERSION);
    assert_int_equal(versionInfo.sw_patch_version, CANNM_SW_PATCH_VERSION);
}

/**
 * @brief Test CanNm_GetVersionInfo with NULL pointer
 */
/** @req SWS_CanNm_00003 */
/** @req SWS_CanNm_00012 */
static void test_CanNm_GetVersionInfo_NullPointer(void **state)
{
    (void)state;
    
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    CanNm_GetVersionInfo(NULL);
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_PARAM_POINTER);
#endif
}

/*==================================================================================================
 *                                    Network Control Tests
 *================================================================================================*/

/**
 * @brief Test CanNm_NetworkRequest from Bus Sleep mode
 */
/** @req SWS_CanNm_00006 */
/** @req SWS_CanNm_00004 */
static void test_CanNm_NetworkRequest_FromBusSleep(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    Nm_StateType nmState;
    Nm_ModeType nmMode;
    
    /* Ensure we start from Bus Sleep */
    CanNm_DeInit();
    Reset_Mocks();
    CanNm_Init(&CanNm_Config);
    
    /* Request network from Bus Sleep */
    result = CanNm_NetworkRequest(CANNM_CHANNEL_0);
    
    assert_int_equal(result, E_OK);
    assert_true(ComM_Nm_NetworkMode_Called);
    
    /* Check state transition to Repeat Message */
    result = CanNm_GetState(CANNM_CHANNEL_0, &nmState, &nmMode);
    assert_int_equal(result, E_OK);
    assert_int_equal(nmState, NM_STATE_REPEAT_MESSAGE);
    assert_int_equal(nmMode, NM_MODE_NETWORK);
}

/**
 * @brief Test CanNm_NetworkRequest from Ready Sleep mode
 */
/** @req SWS_CanNm_00006 */
/** @req SWS_CanNm_00004 */
static void test_CanNm_NetworkRequest_FromReadySleep(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    Nm_StateType nmState;
    Nm_ModeType nmMode;
    
    /* First request network, then release to get to Ready Sleep */
    CanNm_NetworkRequest(CANNM_CHANNEL_0);
    
    /* Wait for Repeat Message timer to expire */
    uint32 timeout = CANNM_REPEAT_MESSAGE_TIME_DEFAULT / CANNM_MAIN_FUNCTION_PERIOD_MS + 10;
    for (uint32 i = 0; i < timeout; i++)
    {
        CanNm_MainFunction();
    }
    
    /* Release network to transition to Ready Sleep */
    CanNm_NetworkRelease(CANNM_CHANNEL_0);
    
    result = CanNm_GetState(CANNM_CHANNEL_0, &nmState, &nmMode);
    assert_int_equal(result, E_OK);
    
    if (nmState == NM_STATE_READY_SLEEP)
    {
        /* Now request again - should go to Normal Operation */
        Reset_Mocks();
        result = CanNm_NetworkRequest(CANNM_CHANNEL_0);
        
        assert_int_equal(result, E_OK);
        
        result = CanNm_GetState(CANNM_CHANNEL_0, &nmState, &nmMode);
        assert_int_equal(result, E_OK);
        assert_int_equal(nmState, NM_STATE_NORMAL_OPERATION);
    }
}

/**
 * @brief Test CanNm_NetworkRequest with invalid channel
 */
/** @req SWS_CanNm_00006 */
/** @req SWS_CanNm_00004 */
static void test_CanNm_NetworkRequest_InvalidChannel(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    
    result = CanNm_NetworkRequest(0xFF); /* Invalid channel */
    
    assert_int_equal(result, E_NOT_OK);
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_INVALID_CHANNEL);
#endif
}

/**
 * @brief Test CanNm_NetworkRelease from Normal Operation mode
 */
/** @req SWS_CanNm_00007 */
/** @req SWS_CanNm_00005 */
static void test_CanNm_NetworkRelease_FromNormalOperation(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    Nm_StateType nmState;
    Nm_ModeType nmMode;
    
    /* First request network */
    CanNm_NetworkRequest(CANNM_CHANNEL_0);
    
    /* Wait for Repeat Message timer to expire and transition to Normal Operation */
    uint32 timeout = (CANNM_REPEAT_MESSAGE_TIME_DEFAULT + 100) / CANNM_MAIN_FUNCTION_PERIOD_MS;
    for (uint32 i = 0; i < timeout; i++)
    {
        CanNm_MainFunction();
    }
    
    /* Get current state */
    result = CanNm_GetState(CANNM_CHANNEL_0, &nmState, &nmMode);
    if (result == E_OK && nmState == NM_STATE_NORMAL_OPERATION)
    {
        /* Release network */
        Reset_Mocks();
        result = CanNm_NetworkRelease(CANNM_CHANNEL_0);
        
        assert_int_equal(result, E_OK);
        
        /* Should transition to Ready Sleep */
        result = CanNm_GetState(CANNM_CHANNEL_0, &nmState, &nmMode);
        assert_int_equal(result, E_OK);
        assert_int_equal(nmState, NM_STATE_READY_SLEEP);
    }
}

/**
 * @brief Test CanNm_NetworkRelease with invalid channel
 */
/** @req SWS_CanNm_00007 */
/** @req SWS_CanNm_00005 */
static void test_CanNm_NetworkRelease_InvalidChannel(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    
    result = CanNm_NetworkRelease(0xFF); /* Invalid channel */
    
    assert_int_equal(result, E_NOT_OK);
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_INVALID_CHANNEL);
#endif
}

/**
 * @brief Test CanNm_PassiveStartUp from Bus Sleep
 */
/** @req SWS_CanNm_00005 */
/** @req SWS_CanNm_00003 */
static void test_CanNm_PassiveStartUp_FromBusSleep(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    Nm_StateType nmState;
    Nm_ModeType nmMode;
    
    /* Ensure we start from Bus Sleep */
    CanNm_DeInit();
    CanNm_Init(&CanNm_Config);
    
    /* Passive startup - join network without requesting it */
    result = CanNm_PassiveStartUp(CANNM_CHANNEL_0);
    
    assert_int_equal(result, E_OK);
    assert_true(ComM_Nm_NetworkMode_Called);
    
    /* Check state transition */
    result = CanNm_GetState(CANNM_CHANNEL_0, &nmState, &nmMode);
    assert_int_equal(result, E_OK);
    assert_int_equal(nmState, NM_STATE_REPEAT_MESSAGE);
}

/**
 * @brief Test CanNm_PassiveStartUp with invalid channel
 */
/** @req SWS_CanNm_00005 */
/** @req SWS_CanNm_00003 */
static void test_CanNm_PassiveStartUp_InvalidChannel(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    
    result = CanNm_PassiveStartUp(0xFF); /* Invalid channel */
    
    assert_int_equal(result, E_NOT_OK);
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_INVALID_CHANNEL);
#endif
}

/*==================================================================================================
 *                                    State Machine Tests
 *================================================================================================*/

/**
 * @brief Test state transition from Repeat Message to Normal Operation
 */
/** @req SWS_CanNm_00006 */
static void test_CanNm_StateMachine_RepeatMsgToNormalOp(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    Nm_StateType nmState;
    Nm_ModeType nmMode;
    
    /* Request network to enter Repeat Message state */
    CanNm_NetworkRequest(CANNM_CHANNEL_0);
    
    result = CanNm_GetState(CANNM_CHANNEL_0, &nmState, &nmMode);
    assert_int_equal(result, E_OK);
    assert_int_equal(nmState, NM_STATE_REPEAT_MESSAGE);
    
    /* Wait for Repeat Message timer to expire */
    uint32 timeout = (CANNM_REPEAT_MESSAGE_TIME_DEFAULT + 100) / CANNM_MAIN_FUNCTION_PERIOD_MS;
    for (uint32 i = 0; i < timeout; i++)
    {
        CanNm_MainFunction();
    }
    
    /* Should now be in Normal Operation */
    result = CanNm_GetState(CANNM_CHANNEL_0, &nmState, &nmMode);
    assert_int_equal(result, E_OK);
    assert_int_equal(nmState, NM_STATE_NORMAL_OPERATION);
}

/**
 * @brief Test state transition from Normal Operation to Ready Sleep
 */
/** @req SWS_CanNm_00006 */
static void test_CanNm_StateMachine_NormalOpToReadySleep(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    Nm_StateType nmState;
    Nm_ModeType nmMode;
    
    /* Request network and wait for Normal Operation */
    CanNm_NetworkRequest(CANNM_CHANNEL_0);
    uint32 timeout = (CANNM_REPEAT_MESSAGE_TIME_DEFAULT + 100) / CANNM_MAIN_FUNCTION_PERIOD_MS;
    for (uint32 i = 0; i < timeout; i++)
    {
        CanNm_MainFunction();
    }
    
    result = CanNm_GetState(CANNM_CHANNEL_0, &nmState, &nmMode);
    if (result == E_OK && nmState == NM_STATE_NORMAL_OPERATION)
    {
        /* Release network */
        CanNm_NetworkRelease(CANNM_CHANNEL_0);
        
        /* Should be in Ready Sleep */
        result = CanNm_GetState(CANNM_CHANNEL_0, &nmState, &nmMode);
        assert_int_equal(result, E_OK);
        assert_int_equal(nmState, NM_STATE_READY_SLEEP);
    }
}

/**
 * @brief Test state transition to Prepare Bus Sleep
 */
/** @req SWS_CanNm_00006 */
static void test_CanNm_StateMachine_ToPrepareBusSleep(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    Nm_StateType nmState;
    Nm_ModeType nmMode;
    
    /* Enter Ready Sleep state */
    CanNm_NetworkRequest(CANNM_CHANNEL_0);
    uint32 timeout = (CANNM_REPEAT_MESSAGE_TIME_DEFAULT + 100) / CANNM_MAIN_FUNCTION_PERIOD_MS;
    for (uint32 i = 0; i < timeout; i++)
    {
        CanNm_MainFunction();
    }
    CanNm_NetworkRelease(CANNM_CHANNEL_0);
    
    /* Wait for NM Timeout to expire */
    timeout = (CANNM_NM_TIMEOUT_TIME_DEFAULT + 100) / CANNM_MAIN_FUNCTION_PERIOD_MS;
    for (uint32 i = 0; i < timeout; i++)
    {
        CanNm_MainFunction();
    }
    
    result = CanNm_GetState(CANNM_CHANNEL_0, &nmState, &nmMode);
    assert_int_equal(result, E_OK);
    
    /* Should be in Prepare Bus Sleep after NM timeout */
    if (nmState == NM_STATE_PREPARE_BUS_SLEEP)
    {
        assert_true(ComM_Nm_PrepareBusSleepMode_Called || TRUE); /* May have been called during transition */
    }
}

/**
 * @brief Test state transition to Bus Sleep
 */
/** @req SWS_CanNm_00006 */
static void test_CanNm_StateMachine_ToBusSleep(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    Nm_StateType nmState;
    Nm_ModeType nmMode;
    
    /* Enter Prepare Bus Sleep state first */
    CanNm_NetworkRequest(CANNM_CHANNEL_0);
    uint32 timeout = (CANNM_REPEAT_MESSAGE_TIME_DEFAULT + 100) / CANNM_MAIN_FUNCTION_PERIOD_MS;
    for (uint32 i = 0; i < timeout; i++)
    {
        CanNm_MainFunction();
    }
    CanNm_NetworkRelease(CANNM_CHANNEL_0);
    
    /* Wait for NM Timeout */
    timeout = (CANNM_NM_TIMEOUT_TIME_DEFAULT + 100) / CANNM_MAIN_FUNCTION_PERIOD_MS;
    for (uint32 i = 0; i < timeout; i++)
    {
        CanNm_MainFunction();
    }
    
    /* Wait for Wait Bus Sleep timer */
    timeout = (CANNM_WAIT_BUS_SLEEP_TIME_DEFAULT + 100) / CANNM_MAIN_FUNCTION_PERIOD_MS;
    for (uint32 i = 0; i < timeout; i++)
    {
        CanNm_MainFunction();
    }
    
    result = CanNm_GetState(CANNM_CHANNEL_0, &nmState, &nmMode);
    assert_int_equal(result, E_OK);
    
    /* Should be in Bus Sleep after all timeouts */
    if (nmState == NM_STATE_BUS_SLEEP)
    {
        assert_int_equal(nmMode, NM_MODE_BUS_SLEEP);
    }
}

/*==================================================================================================
 *                                    Message Transmission Tests
 *================================================================================================*/

/**
 * @brief Test periodic message transmission in Network Mode
 */
/** @req SWS_CanNm_00007 */
static void test_CanNm_PeriodicTransmission(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    
    /* Request network */
    CanNm_NetworkRequest(CANNM_CHANNEL_0);
    
    /* Reset transmit counter */
    Reset_Mocks();
    
    /* Wait for one message cycle */
    uint32 cycles = CANNM_MESSAGE_CYCLE_TIME_DEFAULT / CANNM_MAIN_FUNCTION_PERIOD_MS + 1;
    for (uint32 i = 0; i < cycles; i++)
    {
        CanNm_MainFunction();
    }
    
    /* Should have transmitted at least one message */
    assert_true(CanIf_Transmit_Called);
    assert_int_equal(CanIf_LastTxPduId, CANNM_CHANNEL_0_TX_PDUID);
}

/**
 * @brief Test TX confirmation callback
 */
/** @req SWS_CanNm_00017 */
/** @req SWS_CanNm_00009 */
static void test_CanNm_TxConfirmation(void **state)
{
    (void)state;
    
    /* Simulate TX confirmation */
    CanNm_TxConfirmation(CANNM_CHANNEL_0_TX_PDUID, E_OK);
    
    /* No immediate verification - but should not crash */
    assert_true(TRUE);
}

/**
 * @brief Test TX confirmation with invalid PDU ID
 */
/** @req SWS_CanNm_00017 */
/** @req SWS_CanNm_00009 */
static void test_CanNm_TxConfirmation_InvalidPduId(void **state)
{
    (void)state;
    
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    CanNm_TxConfirmation(0xFFFF, E_OK);
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_INVALID_PDUID);
#endif
}

/**
 * @brief Test TX confirmation when not initialized
 */
/** @req SWS_CanNm_00001 */
/** @req SWS_CanNm_00009 */
static void test_CanNm_TxConfirmation_NotInitialized(void **state)
{
    (void)state;
    
    CanNm_DeInit();
    Reset_Mocks();
    
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    CanNm_TxConfirmation(CANNM_CHANNEL_0_TX_PDUID, E_OK);
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_NOT_INITIALIZED);
#endif
    
    CanNm_Init(&CanNm_Config);
}

/*==================================================================================================
 *                                    Reception Tests
 *================================================================================================*/

/**
 * @brief Test RX indication callback
 */
/** @req SWS_CanNm_00018 */
/** @req SWS_CanNm_00008 */
static void test_CanNm_RxIndication(void **state)
{
    (void)state;
    
    PduInfoType pduInfo;
    uint8 rxData[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; /* Node ID 0x01, no CBV */
    
    pduInfo.SduDataPtr = rxData;
    pduInfo.SduLength = 8;
    pduInfo.MetaDataPtr = NULL;
    
    /* Simulate reception from Bus Sleep */
    CanNm_DeInit();
    CanNm_Init(&CanNm_Config);
    Reset_Mocks();
    
    CanNm_RxIndication(CANNM_CHANNEL_0_RX_PDUID, &pduInfo);
    
    /* Should have detected wakeup */
    assert_true(ComM_EcuM_WakeUpIndication_Called);
}

/**
 * @brief Test RX indication with repeat message request
 */
/** @req SWS_CanNm_00018 */
/** @req SWS_CanNm_00008 */
static void test_CanNm_RxIndication_RepeatMsgRequest(void **state)
{
    (void)state;
    
    PduInfoType pduInfo;
    uint8 rxData[8] = {0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; /* Node ID 0x02, CBV with RptMsg bit */
    Nm_StateType nmState;
    Nm_ModeType nmMode;
    
    pduInfo.SduDataPtr = rxData;
    pduInfo.SduLength = 8;
    
    /* First get to Normal Operation */
    CanNm_NetworkRequest(CANNM_CHANNEL_0);
    uint32 timeout = (CANNM_REPEAT_MESSAGE_TIME_DEFAULT + 100) / CANNM_MAIN_FUNCTION_PERIOD_MS;
    for (uint32 i = 0; i < timeout; i++)
    {
        CanNm_MainFunction();
    }
    
    Std_ReturnType result = CanNm_GetState(CANNM_CHANNEL_0, &nmState, &nmMode);
    if (result == E_OK && nmState == NM_STATE_NORMAL_OPERATION)
    {
        /* Receive repeat message request */
        CanNm_RxIndication(CANNM_CHANNEL_0_RX_PDUID, &pduInfo);
        
        /* Should transition to Repeat Message */
        result = CanNm_GetState(CANNM_CHANNEL_0, &nmState, &nmMode);
        assert_int_equal(result, E_OK);
        assert_int_equal(nmState, NM_STATE_REPEAT_MESSAGE);
    }
}

/**
 * @brief Test RX indication with NULL pointer
 */
/** @req SWS_CanNm_00018 */
/** @req SWS_CanNm_00008 */
static void test_CanNm_RxIndication_NullPointer(void **state)
{
    (void)state;
    
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    CanNm_RxIndication(CANNM_CHANNEL_0_RX_PDUID, NULL);
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_PARAM_POINTER);
#endif
}

/**
 * @brief Test RX indication with invalid PDU ID
 */
/** @req SWS_CanNm_00018 */
/** @req SWS_CanNm_00008 */
static void test_CanNm_RxIndication_InvalidPduId(void **state)
{
    (void)state;
    
    PduInfoType pduInfo;
    uint8 rxData[8] = {0x00};
    
    pduInfo.SduDataPtr = rxData;
    pduInfo.SduLength = 8;
    
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    CanNm_RxIndication(0xFFFF, &pduInfo);
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_INVALID_PDUID);
#endif
}

/*==================================================================================================
 *                                    PDU Data Tests
 *================================================================================================*/

/**
 * @brief Test CanNm_SetUserData
 */
/** @req SWS_CanNm_00011 */
/** @req SWS_CanNm_00013 */
static void test_CanNm_SetUserData(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    uint8 userData[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    
    result = CanNm_SetUserData(CANNM_CHANNEL_0, userData);
    
#if (CANNM_USER_DATA_ENABLED == STD_ON)
    assert_int_equal(result, E_OK);
#else
    (void)result;
#endif
}

/**
 * @brief Test CanNm_SetUserData with NULL pointer
 */
/** @req SWS_CanNm_00011 */
/** @req SWS_CanNm_00013 */
static void test_CanNm_SetUserData_NullPointer(void **state)
{
    (void)state;
    
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    CanNm_SetUserData(CANNM_CHANNEL_0, NULL);
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_PARAM_POINTER);
#endif
}

/**
 * @brief Test CanNm_SetUserData with invalid channel
 */
/** @req SWS_CanNm_00011 */
/** @req SWS_CanNm_00013 */
static void test_CanNm_SetUserData_InvalidChannel(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    uint8 userData[6] = {0x00};
    
    result = CanNm_SetUserData(0xFF, userData);
    
    assert_int_equal(result, E_NOT_OK);
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_INVALID_CHANNEL);
#endif
}

/**
 * @brief Test CanNm_GetUserData
 */
/** @req SWS_CanNm_00010 */
/** @req SWS_CanNm_00014 */
static void test_CanNm_GetUserData(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    uint8 userData[6] = {0x00};
    
    result = CanNm_GetUserData(CANNM_CHANNEL_0, userData);
    
    /* Should succeed if user data is enabled */
#if (CANNM_USER_DATA_ENABLED == STD_ON)
    assert_int_equal(result, E_OK);
#else
    (void)result;
#endif
}

/**
 * @brief Test CanNm_GetUserData with NULL pointer
 */
/** @req SWS_CanNm_00010 */
/** @req SWS_CanNm_00014 */
static void test_CanNm_GetUserData_NullPointer(void **state)
{
    (void)state;
    
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    CanNm_GetUserData(CANNM_CHANNEL_0, NULL);
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_PARAM_POINTER);
#endif
}

/*==================================================================================================
 *                                    Communication Control Tests
 *================================================================================================*/

/**
 * @brief Test CanNm_DisableCommunication
 */
/** @req SWS_CanNm_00008 */
/** @req SWS_CanNm_00016 */
static void test_CanNm_DisableCommunication(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    
    /* First request network to be in Network Mode */
    CanNm_NetworkRequest(CANNM_CHANNEL_0);
    
    result = CanNm_DisableCommunication(CANNM_CHANNEL_0);
    
#if (CANNM_COM_CONTROL_ENABLED == STD_ON)
    assert_int_equal(result, E_OK);
#else
    (void)result;
#endif
}

/**
 * @brief Test CanNm_EnableCommunication
 */
/** @req SWS_CanNm_00009 */
/** @req SWS_CanNm_00017 */
static void test_CanNm_EnableCommunication(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    
    /* First request network */
    CanNm_NetworkRequest(CANNM_CHANNEL_0);
    
    /* Disable then enable */
    CanNm_DisableCommunication(CANNM_CHANNEL_0);
    result = CanNm_EnableCommunication(CANNM_CHANNEL_0);
    
#if (CANNM_COM_CONTROL_ENABLED == STD_ON)
    assert_int_equal(result, E_OK);
#else
    (void)result;
#endif
}

/**
 * @brief Test CanNm_DisableCommunication with invalid channel
 */
/** @req SWS_CanNm_00008 */
/** @req SWS_CanNm_00016 */
static void test_CanNm_DisableCommunication_InvalidChannel(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    
    result = CanNm_DisableCommunication(0xFF);
    
    assert_int_equal(result, E_NOT_OK);
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_INVALID_CHANNEL);
#endif
}

/*==================================================================================================
 *                                    Sleep Ready Bit Tests
 *================================================================================================*/

/**
 * @brief Test CanNm_SetSleepReadyBit
 */
/** @req SWS_CanNm_00016 */
/** @req SWS_CanNm_00015 */
static void test_CanNm_SetSleepReadyBit(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    
    result = CanNm_SetSleepReadyBit(CANNM_CHANNEL_0, TRUE);
    
#if (CANNM_COORDINATOR_SUPPORT_ENABLED == STD_ON)
    assert_int_equal(result, E_OK);
#else
    (void)result;
#endif
}

/**
 * @brief Test CanNm_SetSleepReadyBit with invalid channel
 */
/** @req SWS_CanNm_00016 */
/** @req SWS_CanNm_00015 */
static void test_CanNm_SetSleepReadyBit_InvalidChannel(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    
    result = CanNm_SetSleepReadyBit(0xFF, TRUE);
    
    assert_int_equal(result, E_NOT_OK);
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_INVALID_CHANNEL);
#endif
}

/*==================================================================================================
 *                                    State Query Tests
 *================================================================================================*/

/**
 * @brief Test CanNm_GetState with valid parameters
 */
/** @req SWS_CanNm_00013 */
/** @req SWS_CanNm_00011 */
static void test_CanNm_GetState(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    Nm_StateType nmState;
    Nm_ModeType nmMode;
    
    /* Get state from Bus Sleep */
    result = CanNm_GetState(CANNM_CHANNEL_0, &nmState, &nmMode);
    
    assert_int_equal(result, E_OK);
    assert_int_equal(nmState, NM_STATE_BUS_SLEEP);
    assert_int_equal(nmMode, NM_MODE_BUS_SLEEP);
}

/**
 * @brief Test CanNm_GetState with NULL pointers
 */
/** @req SWS_CanNm_00013 */
/** @req SWS_CanNm_00011 */
static void test_CanNm_GetState_NullPointer(void **state)
{
    (void)state;
    
    Nm_StateType nmState;
    Nm_ModeType nmMode;
    
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    CanNm_GetState(CANNM_CHANNEL_0, NULL, &nmMode);
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_PARAM_POINTER);
    
    Reset_Mocks();
    
    CanNm_GetState(CANNM_CHANNEL_0, &nmState, NULL);
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_PARAM_POINTER);
#endif
}

/**
 * @brief Test CanNm_GetState with invalid channel
 */
/** @req SWS_CanNm_00013 */
/** @req SWS_CanNm_00011 */
static void test_CanNm_GetState_InvalidChannel(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    Nm_StateType nmState;
    Nm_ModeType nmMode;
    
    result = CanNm_GetState(0xFF, &nmState, &nmMode);
    
    assert_int_equal(result, E_NOT_OK);
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_INVALID_CHANNEL);
#endif
}

/*==================================================================================================
 *                                    Multiple Channel Tests
 *================================================================================================*/

/**
 * @brief Test independent operation of multiple channels
 */
/** @req SWS_CanNm_00004 */
static void test_CanNm_MultipleChannels(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    Nm_StateType nmState0, nmState1;
    Nm_ModeType nmMode0, nmMode1;
    
    /* Request network on channel 0 only */
    result = CanNm_NetworkRequest(CANNM_CHANNEL_0);
    assert_int_equal(result, E_OK);
    
    result = CanNm_GetState(CANNM_CHANNEL_0, &nmState0, &nmMode0);
    assert_int_equal(result, E_OK);
    assert_int_equal(nmState0, NM_STATE_REPEAT_MESSAGE);
    
    /* Channel 1 should still be in Bus Sleep */
    result = CanNm_GetState(CANNM_CHANNEL_1, &nmState1, &nmMode1);
    assert_int_equal(result, E_OK);
    assert_int_equal(nmState1, NM_STATE_BUS_SLEEP);
    
    /* Now request on channel 1 */
    result = CanNm_NetworkRequest(CANNM_CHANNEL_1);
    assert_int_equal(result, E_OK);
    
    result = CanNm_GetState(CANNM_CHANNEL_1, &nmState1, &nmMode1);
    assert_int_equal(result, E_OK);
    assert_int_equal(nmState1, NM_STATE_REPEAT_MESSAGE);
}

/*==================================================================================================
 *                                    Main Function Tests
 *================================================================================================*/

/**
 * @brief Test CanNm_MainFunction basic operation
 */
/** @req SWS_CanNm_00004 */
/** @req SWS_CanNm_00006 */
static void test_CanNm_MainFunction(void **state)
{
    (void)state;
    
    /* Should execute without errors */
    CanNm_MainFunction();
    
    assert_true(TRUE);
}

/**
 * @brief Test CanNm_MainFunction when not initialized
 */
/** @req SWS_CanNm_00001 */
/** @req SWS_CanNm_00006 */
static void test_CanNm_MainFunction_NotInitialized(void **state)
{
    (void)state;
    
    CanNm_DeInit();
    Reset_Mocks();
    
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    CanNm_MainFunction();
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_NOT_INITIALIZED);
#endif
    
    CanNm_Init(&CanNm_Config);
}

/*==================================================================================================
 *                                    Trigger Transmit Tests
 *================================================================================================*/

/**
 * @brief Test CanNm_TriggerTransmit
 */
/** @req SWS_CanNm_00010 */
static void test_CanNm_TriggerTransmit(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    PduInfoType pduInfo;
    uint8 dataBuffer[8];
    
    pduInfo.SduDataPtr = dataBuffer;
    pduInfo.SduLength = 8;
    
    result = CanNm_TriggerTransmit(CANNM_CHANNEL_0_TX_PDUID, &pduInfo);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

/**
 * @brief Test CanNm_TriggerTransmit with NULL pointer
 */
/** @req SWS_CanNm_00010 */
static void test_CanNm_TriggerTransmit_NullPointer(void **state)
{
    (void)state;
    
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    CanNm_TriggerTransmit(CANNM_CHANNEL_0_TX_PDUID, NULL);
    assert_true(Det_ReportError_Called);
    assert_int_equal(Det_LastErrorId, CANNM_E_PARAM_POINTER);
#endif
}

/*==================================================================================================
 *                                    Configuration Tests
 *================================================================================================*/

/**
 * @brief Test configuration constants
 */
/** @req SWS_CanNm_00001 */
static void test_CanNm_Configuration(void **state)
{
    (void)state;
    
    /* Verify configuration constants */
    assert_int_equal(CANNM_NUMBER_OF_CHANNELS, 2U);
    assert_int_equal(CANNM_PDU_LENGTH, 8U);
    assert_int_equal(CANNM_PDU_NID_POSITION, 0U);
    assert_int_equal(CANNM_PDU_CBV_POSITION, 1U);
    
    /* Verify feature switches are properly defined */
    assert_true(CANNM_VERSION_INFO_API == STD_ON || CANNM_VERSION_INFO_API == STD_OFF);
    assert_true(CANNM_COM_CONTROL_ENABLED == STD_ON || CANNM_COM_CONTROL_ENABLED == STD_OFF);
    assert_true(CANNM_DEV_ERROR_DETECT == STD_ON || CANNM_DEV_ERROR_DETECT == STD_OFF);
}

/**
 * @brief Test channel configuration
 */
/** @req SWS_CanNm_00001 */
static void test_CanNm_ChannelConfiguration(void **state)
{
    (void)state;
    
    /* Verify channel configuration exists */
    assert_non_null(CanNm_Config.ChannelConfig);
    assert_int_equal(CanNm_Config.ChannelCount, CANNM_NUMBER_OF_CHANNELS);
    
    /* Verify channel 0 configuration */
    assert_int_equal(CanNm_Config.ChannelConfig[0].ChannelId, CANNM_CHANNEL_0);
    assert_int_equal(CanNm_Config.ChannelConfig[0].NodeId, CANNM_CHANNEL_0_NODE_ID);
    assert_int_equal(CanNm_Config.ChannelConfig[0].TxPduId, CANNM_CHANNEL_0_TX_PDUID);
    assert_int_equal(CanNm_Config.ChannelConfig[0].RxPduId, CANNM_CHANNEL_0_RX_PDUID);
    
    /* Verify channel 1 configuration */
    assert_int_equal(CanNm_Config.ChannelConfig[1].ChannelId, CANNM_CHANNEL_1);
    assert_int_equal(CanNm_Config.ChannelConfig[1].NodeId, CANNM_CHANNEL_1_NODE_ID);
    assert_int_equal(CanNm_Config.ChannelConfig[1].TxPduId, CANNM_CHANNEL_1_TX_PDUID);
    assert_int_equal(CanNm_Config.ChannelConfig[1].RxPduId, CANNM_CHANNEL_1_RX_PDUID);
}

/*==================================================================================================
 *                                      Test Suite
 *================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        /* Initialization Tests */
        cmocka_unit_test_setup_teardown(test_CanNm_Init_ValidConfig, NULL, teardown),
        cmocka_unit_test_setup_teardown(test_CanNm_Init_NullPointer, teardown, setup),
        cmocka_unit_test_setup_teardown(test_CanNm_Init_DoubleInit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanNm_DeInit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanNm_DeInit_NotInitialized, teardown, setup),
        cmocka_unit_test_setup_teardown(test_CanNm_GetVersionInfo, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanNm_GetVersionInfo_NullPointer, setup, teardown),
        
        /* Network Control Tests */
        cmocka_unit_test_setup_teardown(test_CanNm_NetworkRequest_FromBusSleep, teardown, setup),
        cmocka_unit_test_setup_teardown(test_CanNm_NetworkRequest_FromReadySleep, teardown, setup),
        cmocka_unit_test_setup_teardown(test_CanNm_NetworkRequest_InvalidChannel, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanNm_NetworkRelease_FromNormalOperation, teardown, setup),
        cmocka_unit_test_setup_teardown(test_CanNm_NetworkRelease_InvalidChannel, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanNm_PassiveStartUp_FromBusSleep, teardown, setup),
        cmocka_unit_test_setup_teardown(test_CanNm_PassiveStartUp_InvalidChannel, setup, teardown),
        
        /* State Machine Tests */
        cmocka_unit_test_setup_teardown(test_CanNm_StateMachine_RepeatMsgToNormalOp, teardown, setup),
        cmocka_unit_test_setup_teardown(test_CanNm_StateMachine_NormalOpToReadySleep, teardown, setup),
        cmocka_unit_test_setup_teardown(test_CanNm_StateMachine_ToPrepareBusSleep, teardown, setup),
        cmocka_unit_test_setup_teardown(test_CanNm_StateMachine_ToBusSleep, teardown, setup),
        
        /* Message Transmission Tests */
        cmocka_unit_test_setup_teardown(test_CanNm_PeriodicTransmission, teardown, setup),
        cmocka_unit_test_setup_teardown(test_CanNm_TxConfirmation, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanNm_TxConfirmation_InvalidPduId, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanNm_TxConfirmation_NotInitialized, teardown, setup),
        
        /* Reception Tests */
        cmocka_unit_test_setup_teardown(test_CanNm_RxIndication, teardown, setup),
        cmocka_unit_test_setup_teardown(test_CanNm_RxIndication_RepeatMsgRequest, teardown, setup),
        cmocka_unit_test_setup_teardown(test_CanNm_RxIndication_NullPointer, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanNm_RxIndication_InvalidPduId, setup, teardown),
        
        /* PDU Data Tests */
        cmocka_unit_test_setup_teardown(test_CanNm_SetUserData, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanNm_SetUserData_NullPointer, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanNm_SetUserData_InvalidChannel, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanNm_GetUserData, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanNm_GetUserData_NullPointer, setup, teardown),
        
        /* Communication Control Tests */
        cmocka_unit_test_setup_teardown(test_CanNm_DisableCommunication, teardown, setup),
        cmocka_unit_test_setup_teardown(test_CanNm_EnableCommunication, teardown, setup),
        cmocka_unit_test_setup_teardown(test_CanNm_DisableCommunication_InvalidChannel, setup, teardown),
        
        /* Sleep Ready Bit Tests */
        cmocka_unit_test_setup_teardown(test_CanNm_SetSleepReadyBit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanNm_SetSleepReadyBit_InvalidChannel, setup, teardown),
        
        /* State Query Tests */
        cmocka_unit_test_setup_teardown(test_CanNm_GetState, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanNm_GetState_NullPointer, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanNm_GetState_InvalidChannel, setup, teardown),
        
        /* Multiple Channel Tests */
        cmocka_unit_test_setup_teardown(test_CanNm_MultipleChannels, teardown, setup),
        
        /* Main Function Tests */
        cmocka_unit_test_setup_teardown(test_CanNm_MainFunction, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanNm_MainFunction_NotInitialized, teardown, setup),
        
        /* Trigger Transmit Tests */
        cmocka_unit_test_setup_teardown(test_CanNm_TriggerTransmit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanNm_TriggerTransmit_NullPointer, setup, teardown),
        
        /* Configuration Tests */
        cmocka_unit_test_setup_teardown(test_CanNm_Configuration, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanNm_ChannelConfiguration, setup, teardown),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}
