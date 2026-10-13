/*==================================================================================================
 * PduR Branch Coverage Test
 *
 * Goal: Maximize branch coverage of PduR.c (target > 90%)
 * Strategy: #include PduR.c directly to access STATIC (static) functions,
 *           provide mock/stub implementations for all external dependencies.
 *
 * (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
 *==================================================================================================*/

#include <unity.h>
#include <string.h>
#include "Std_Types.h"
#include "ComStack_Types.h"
#include "PduR.h"
#include "PduR_Cfg.h"
#include "Det.h"

/* ── Det mock tracking ───────────────────────────────────────────────────── */
static uint16 det_last_error = 0xFFFF;
static uint8  det_last_api_id = 0xFF;
static uint16 det_last_module_id = 0;
static uint8  det_last_instance_id = 0;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId,
                               uint8 ApiId, uint8 ErrorId)
{
    det_last_module_id   = ModuleId;
    det_last_instance_id = InstanceId;
    det_last_api_id      = ApiId;
    det_last_error       = ErrorId;
    return E_OK;
}

void Det_Init(const Det_ConfigType* ConfigPtr) { (void)ConfigPtr; }
void Det_Start(void) {}

/* ── CanIf mock ──────────────────────────────────────────────────────────── */
static uint32     mock_CanIf_Transmit_Count = 0;
static PduIdType  mock_CanIf_Transmit_LastPduId = 0;
static Std_ReturnType mock_CanIf_Transmit_RetVal = E_OK;

Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    (void)PduInfoPtr;
    mock_CanIf_Transmit_Count++;
    mock_CanIf_Transmit_LastPduId = TxPduId;
    return mock_CanIf_Transmit_RetVal;
}

static uint32    mock_CanIf_CancelTransmit_Count = 0;
static PduIdType mock_CanIf_CancelTransmit_LastPduId = 0;

Std_ReturnType CanIf_CancelTransmit(PduIdType TxPduId)
{
    mock_CanIf_CancelTransmit_Count++;
    mock_CanIf_CancelTransmit_LastPduId = TxPduId;
    return E_OK;
}

/* ── Com mock ────────────────────────────────────────────────────────────── */
static uint32    mock_Com_RxIndication_Count = 0;
static PduIdType mock_Com_RxIndication_LastPduId = 0;

void Com_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    (void)PduInfoPtr;
    mock_Com_RxIndication_Count++;
    mock_Com_RxIndication_LastPduId = RxPduId;
}

static uint32         mock_Com_TxConfirmation_Count = 0;
static PduIdType      mock_Com_TxConfirmation_LastPduId = 0;
static Std_ReturnType mock_Com_TxConfirmation_LastResult = E_NOT_OK;

void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    mock_Com_TxConfirmation_Count++;
    mock_Com_TxConfirmation_LastPduId = TxPduId;
    mock_Com_TxConfirmation_LastResult = result;
}

static uint32    mock_Com_TriggerTransmit_Count = 0;
static PduIdType mock_Com_TriggerTransmit_LastPduId = 0;

Std_ReturnType Com_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)
{
    (void)PduInfoPtr;
    mock_Com_TriggerTransmit_Count++;
    mock_Com_TriggerTransmit_LastPduId = TxPduId;
    return E_OK;
}

/* ── Dcm mock ────────────────────────────────────────────────────────────── */
static uint32 mock_Dcm_RxIndication_Count = 0;

void Dcm_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    (void)RxPduId; (void)PduInfoPtr;
    mock_Dcm_RxIndication_Count++;
}

static uint32         mock_Dcm_TxConfirmation_Count = 0;
static PduIdType      mock_Dcm_TxConfirmation_LastPduId = 0;
static Std_ReturnType mock_Dcm_TxConfirmation_LastResult = E_NOT_OK;

void Dcm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    mock_Dcm_TxConfirmation_Count++;
    mock_Dcm_TxConfirmation_LastPduId = TxPduId;
    mock_Dcm_TxConfirmation_LastResult = result;
}

static uint32 mock_Dcm_TriggerTransmit_Count = 0;

Std_ReturnType Dcm_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)
{
    (void)TxPduId; (void)PduInfoPtr;
    mock_Dcm_TriggerTransmit_Count++;
    return E_OK;
}

/* ── Include PduR source directly to access STATIC functions ─────────────── */
#include "PduR.c"
#include "PduR_Lcfg.c"

/* ── Custom test routing configurations ──────────────────────────────────── */
static uint8 test_sdu_buf[64];

/* Dest: unknown module (exercises default branch in RouteToDestination) */
static const PduR_DestPduConfigType TestDest_Unknown[] = {
    { .DestPduId = 99, .DestModule = 200,
      .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE, .FifoDepth = 0 }
};

/* Dest: COM (for TxConfirmation multi-dest: COM + DCM) */
static const PduR_DestPduConfigType TestDest_Com[] = {
    { .DestPduId = 10, .DestModule = PDUR_MODULE_COM,
      .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE, .FifoDepth = 0 }
};

/* Dest: DCM (reserved for future multi-dest TxConfirmation test) */
/* static const PduR_DestPduConfigType TestDest_Dcm[] = {
    { .DestPduId = 11, .DestModule = PDUR_MODULE_DCM,
      .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE, .FifoDepth = 0 }
}; */

/* Dest: CanIf (for COM-sourced path -> CanIf, used in TxConfirmation fallback) */
static const PduR_DestPduConfigType TestDest_CanIf[] = {
    { .DestPduId = 0, .DestModule = PDUR_MODULE_CANIF,
      .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE, .FifoDepth = 0 }
};

/* Test routing paths */
static const PduR_RoutingPathConfigType TestPaths[] = {
    /* 0: COM src -> unknown dest module */
    { .SrcPdu = { .SourcePduId = 50, .SourceModule = PDUR_MODULE_COM, .SduLength = 8 },
      .DestPdus = TestDest_Unknown, .NumDestPdus = 1,
      .PathType = PDUR_ROUTING_PATH_DIRECT, .GatewayOperation = FALSE },
    /* 1: COM src -> COM + DCM dests (multi-dest) */
    { .SrcPdu = { .SourcePduId = 51, .SourceModule = PDUR_MODULE_COM, .SduLength = 8 },
      .DestPdus = TestDest_Com, .NumDestPdus = 1,
      .PathType = PDUR_ROUTING_PATH_DIRECT, .GatewayOperation = FALSE },
    /* 2: COM src -> CanIf dest (for TxConfirmation fallback via COM path) */
    { .SrcPdu = { .SourcePduId = 52, .SourceModule = PDUR_MODULE_COM, .SduLength = 8 },
      .DestPdus = TestDest_CanIf, .NumDestPdus = 1,
      .PathType = PDUR_ROUTING_PATH_DIRECT, .GatewayOperation = FALSE },
    /* 3: DCM src -> CanIf dest (for TxConfirmation fallback via DCM path) */
    { .SrcPdu = { .SourcePduId = 53, .SourceModule = PDUR_MODULE_DCM, .SduLength = 8 },
      .DestPdus = TestDest_CanIf, .NumDestPdus = 1,
      .PathType = PDUR_ROUTING_PATH_DIRECT, .GatewayOperation = FALSE },
};

static const PduR_ConfigType TestConfig1 = {
    .RoutingPaths = TestPaths,
    .NumRoutingPaths = 4,
    .RoutingPathGroups = NULL_PTR,
    .NumRoutingPathGroups = 0,
    .DevErrorDetect = TRUE,
    .VersionInfoApi = TRUE
};

/* ── setUp / tearDown ────────────────────────────────────────────────────── */
void setUp(void)
{
    det_last_error = 0xFFFF;
    det_last_api_id = 0xFF;
    det_last_module_id = 0;
    det_last_instance_id = 0;

    mock_CanIf_Transmit_Count = 0;
    mock_CanIf_Transmit_LastPduId = 0;
    mock_CanIf_Transmit_RetVal = E_OK;
    mock_CanIf_CancelTransmit_Count = 0;
    mock_CanIf_CancelTransmit_LastPduId = 0;
    mock_Com_RxIndication_Count = 0;
    mock_Com_RxIndication_LastPduId = 0;
    mock_Com_TxConfirmation_Count = 0;
    mock_Com_TxConfirmation_LastPduId = 0;
    mock_Com_TxConfirmation_LastResult = E_NOT_OK;
    mock_Com_TriggerTransmit_Count = 0;
    mock_Com_TriggerTransmit_LastPduId = 0;
    mock_Dcm_RxIndication_Count = 0;
    mock_Dcm_TxConfirmation_Count = 0;
    mock_Dcm_TxConfirmation_LastPduId = 0;
    mock_Dcm_TxConfirmation_LastResult = E_NOT_OK;
    mock_Dcm_TriggerTransmit_Count = 0;

    memset(test_sdu_buf, 0, sizeof(test_sdu_buf));
    memset(&PduR_InternalState, 0, sizeof(PduR_InternalState));
}

void tearDown(void) {}

/* ================================================================================
 * PduR_Init Tests (1-4)
 * ==============================================================================*/

void test_01_Init_NullPtr_ReportsDetError(void)
{
    PduR_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(PDUR_E_PARAM_POINTER, det_last_error);
    TEST_ASSERT_EQUAL(PDUR_SID_INIT, det_last_api_id);
}

void test_02_Init_ValidConfig_StateBecomesInit(void)
{
    PduR_Init(&PduR_Config);
    TEST_ASSERT_EQUAL(E_OK, PduR_Transmit(PDUR_COM_TX_ENGINE_STATUS,
        &(const PduInfoType){.SduDataPtr = test_sdu_buf, .MetaDataPtr = NULL, .SduLength = 8}));
}

void test_03_Init_AllPathStatesEnabled(void)
{
    PduR_Init(&PduR_Config);
    for (uint8 i = 0; i < PDUR_NUMBER_OF_ROUTING_PATHS; i++) {
        TEST_ASSERT_TRUE(PduR_InternalState.PathStates[i].IsEnabled);
    }
}

void test_04_Init_FifoQueuesCleared(void)
{
    PduR_Init(&PduR_Config);
    for (uint8 i = 0; i < PDUR_NUMBER_OF_ROUTING_PATHS; i++) {
        TEST_ASSERT_EQUAL(0, PduR_InternalState.PathStates[i].FifoQueue.Count);
        TEST_ASSERT_EQUAL(0, PduR_InternalState.PathStates[i].FifoQueue.Head);
        TEST_ASSERT_EQUAL(0, PduR_InternalState.PathStates[i].FifoQueue.Tail);
    }
}

/* ================================================================================
 * PduR_DeInit Tests (5-7)
 * ==============================================================================*/

void test_05_DeInit_Uninitialized_ReportsDetError(void)
{
    PduR_DeInit();
    TEST_ASSERT_EQUAL(PDUR_E_UNINIT, det_last_error);
    TEST_ASSERT_EQUAL(PDUR_SID_DEINIT, det_last_api_id);
}

void test_06_DeInit_AfterInit_ConfigCleared(void)
{
    PduR_Init(&PduR_Config);
    PduR_DeInit();
    /* After DeInit, state is UNINIT; Transmit should fail with UNINIT DET error */
    uint8 buf[8] = {0};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 8};
    det_last_error = 0xFFFF;
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_Transmit(PDUR_COM_TX_ENGINE_STATUS, &info));
    TEST_ASSERT_EQUAL(PDUR_E_UNINIT, det_last_error);
}

void test_07_DeInit_AfterInit_StateUninit(void)
{
    PduR_Init(&PduR_Config);
    PduR_DeInit();
    PduR_DeInit();
    TEST_ASSERT_EQUAL(PDUR_E_UNINIT, det_last_error);
}

/* ================================================================================
 * PduR_Transmit Tests (8-16)
 * ==============================================================================*/

void test_08_Transmit_Uninit_ReportsDetError(void)
{
    PduInfoType info = {.SduDataPtr = test_sdu_buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_Transmit(0, &info));
    TEST_ASSERT_EQUAL(PDUR_E_UNINIT, det_last_error);
}

void test_09_Transmit_NullPduInfo_ReportsDetError(void)
{
    PduR_Init(&PduR_Config);
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_Transmit(0, NULL_PTR));
    TEST_ASSERT_EQUAL(PDUR_E_PARAM_POINTER, det_last_error);
}

void test_10_Transmit_ComToCanIf_RoutesSuccessfully(void)
{
    PduR_Init(&PduR_Config);
    PduInfoType info = {.SduDataPtr = test_sdu_buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_OK, PduR_Transmit(PDUR_COM_TX_ENGINE_STATUS, &info));
    TEST_ASSERT_EQUAL(1, mock_CanIf_Transmit_Count);
    TEST_ASSERT_EQUAL(0, mock_CanIf_Transmit_LastPduId);
}

void test_11_Transmit_DcmToCanIf_RoutesSuccessfully(void)
{
    PduR_Init(&PduR_Config);
    PduInfoType info = {.SduDataPtr = test_sdu_buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_OK, PduR_Transmit(PDUR_DCM_TX_DIAG_RESPONSE, &info));
    TEST_ASSERT_EQUAL(1, mock_CanIf_Transmit_Count);
    TEST_ASSERT_EQUAL(5, mock_CanIf_Transmit_LastPduId);
}

void test_12_Transmit_DisabledRouting_ReturnsNotOk(void)
{
    PduR_Init(&PduR_Config);
    PduR_DisableRouting(0);
    PduInfoType info = {.SduDataPtr = test_sdu_buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_Transmit(PDUR_COM_TX_ENGINE_STATUS, &info));
    TEST_ASSERT_EQUAL(0, mock_CanIf_Transmit_Count);
}

void test_13_Transmit_NoRoutingPath_ReportsDetError(void)
{
    PduR_Init(&PduR_Config);
    PduInfoType info = {.SduDataPtr = test_sdu_buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_Transmit(99, &info));
    TEST_ASSERT_EQUAL(PDUR_E_ROUTING_PATH_NOT_FOUND, det_last_error);
}

void test_14_Transmit_CanIfReturnsNotOk_ReturnsNotOk(void)
{
    PduR_Init(&PduR_Config);
    mock_CanIf_Transmit_RetVal = E_NOT_OK;
    PduInfoType info = {.SduDataPtr = test_sdu_buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_Transmit(PDUR_COM_TX_ENGINE_STATUS, &info));
}

void test_15_Transmit_DcmDisabled_ReturnsNotOk(void)
{
    PduR_Init(&PduR_Config);
    /* Directly disable path 5 (DisableRouting API only covers groups 0-3) */
    PduR_InternalState.PathStates[5].IsEnabled = FALSE;
    PduInfoType info = {.SduDataPtr = test_sdu_buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_Transmit(PDUR_DCM_TX_DIAG_RESPONSE, &info));
    TEST_ASSERT_EQUAL(0, mock_CanIf_Transmit_Count);
}

void test_16_Transmit_ComFallbackToDcm_Disabled_ReturnsNotOk(void)
{
    PduR_Init(&TestConfig1);
    PduR_DisableRouting(1);
    PduInfoType info = {.SduDataPtr = test_sdu_buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_Transmit(51, &info));
}

/* ================================================================================
 * PduR_RxIndication Tests (17-22)
 * ==============================================================================*/

void test_17_RxIndication_Uninit_ReportsDetError(void)
{
    uint8 buf[8] = {0};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 8};
    PduR_RxIndication(0, &info);
    TEST_ASSERT_EQUAL(PDUR_E_UNINIT, det_last_error);
}

void test_18_RxIndication_NullPduInfo_ReportsDetError(void)
{
    PduR_Init(&PduR_Config);
    PduR_RxIndication(PDUR_COM_RX_ENGINE_CMD, NULL_PTR);
    TEST_ASSERT_EQUAL(PDUR_E_PARAM_POINTER, det_last_error);
}

void test_19_RxIndication_CanIfToCom_RoutesSuccessfully(void)
{
    PduR_Init(&PduR_Config);
    uint8 buf[8] = {0};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 8};
    PduR_RxIndication(PDUR_COM_RX_ENGINE_CMD, &info);
    TEST_ASSERT_EQUAL(1, mock_Com_RxIndication_Count);
    TEST_ASSERT_EQUAL(PDUR_COM_RX_ENGINE_CMD, mock_Com_RxIndication_LastPduId);
}

void test_20_RxIndication_CanIfToDcm_RoutesSuccessfully(void)
{
    PduR_Init(&PduR_Config);
    uint8 buf[8] = {0};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 8};
    PduR_RxIndication(PDUR_DCM_RX_DIAG_REQUEST, &info);
    TEST_ASSERT_EQUAL(1, mock_Dcm_RxIndication_Count);
}

void test_21_RxIndication_DisabledPath_ReturnsSilently(void)
{
    PduR_Init(&PduR_Config);
    PduR_DisableRouting(3);
    uint8 buf[8] = {0};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 8};
    PduR_RxIndication(PDUR_COM_RX_ENGINE_CMD, &info);
    TEST_ASSERT_EQUAL(0, mock_Com_RxIndication_Count);
}

void test_22_RxIndication_NoRoutingPath_ReportsDetError(void)
{
    PduR_Init(&PduR_Config);
    uint8 buf[8] = {0};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 8};
    PduR_RxIndication(99, &info);
    TEST_ASSERT_EQUAL(PDUR_E_ROUTING_PATH_NOT_FOUND, det_last_error);
}

/* ================================================================================
 * PduR_TxConfirmation Tests (23-31)
 * ==============================================================================*/

void test_23_TxConfirmation_Uninit_ReportsDetError(void)
{
    PduR_TxConfirmation(0, E_OK);
    TEST_ASSERT_EQUAL(PDUR_E_UNINIT, det_last_error);
}

void test_24_TxConfirmation_CanIfPathToDcm_ForwardsConfirmation(void)
{
    PduR_Init(&PduR_Config);
    PduR_TxConfirmation(PDUR_DCM_RX_DIAG_REQUEST, E_OK);
    TEST_ASSERT_EQUAL(1, mock_Dcm_TxConfirmation_Count);
}

void test_25_TxConfirmation_DisabledPath_ReturnsSilently(void)
{
    PduR_Init(&PduR_Config);
    /* Directly disable path 6 (DisableRouting API only covers groups 0-3) */
    PduR_InternalState.PathStates[6].IsEnabled = FALSE;
    PduR_TxConfirmation(PDUR_DCM_RX_DIAG_REQUEST, E_OK);
    TEST_ASSERT_EQUAL(0, mock_Dcm_TxConfirmation_Count);
}

void test_26_TxConfirmation_FallbackComPath_ForwardsToCom(void)
{
    PduR_Init(&TestConfig1);
    PduR_TxConfirmation(52, E_OK);
    TEST_ASSERT_EQUAL(1, mock_Com_TxConfirmation_Count);
    TEST_ASSERT_EQUAL(52, mock_Com_TxConfirmation_LastPduId);
}

void test_27_TxConfirmation_FallbackDcmPath_ForwardsToDcm(void)
{
    PduR_Init(&TestConfig1);
    PduR_TxConfirmation(53, E_OK);
    TEST_ASSERT_EQUAL(1, mock_Dcm_TxConfirmation_Count);
    TEST_ASSERT_EQUAL(53, mock_Dcm_TxConfirmation_LastPduId);
}

void test_28_TxConfirmation_FallbackCom_Disabled_ReturnsSilently(void)
{
    PduR_Init(&TestConfig1);
    PduR_DisableRouting(2);
    PduR_TxConfirmation(52, E_OK);
    TEST_ASSERT_EQUAL(0, mock_Com_TxConfirmation_Count);
}

void test_29_TxConfirmation_FallbackDcm_Disabled_ReturnsSilently(void)
{
    PduR_Init(&TestConfig1);
    PduR_DisableRouting(3);
    PduR_TxConfirmation(53, E_OK);
    TEST_ASSERT_EQUAL(0, mock_Dcm_TxConfirmation_Count);
}

void test_30_TxConfirmation_NoPathFound_NoCallback(void)
{
    PduR_Init(&PduR_Config);
    PduR_TxConfirmation(99, E_OK);
    TEST_ASSERT_EQUAL(0, mock_Com_TxConfirmation_Count);
    TEST_ASSERT_EQUAL(0, mock_Dcm_TxConfirmation_Count);
}

void test_31_TxConfirmation_FallbackCom_DestNotCanIf_NoCallback(void)
{
    PduR_Init(&TestConfig1);
    PduR_TxConfirmation(51, E_OK);
    TEST_ASSERT_EQUAL(0, mock_Com_TxConfirmation_Count);
}

/* ================================================================================
 * PduR_TriggerTransmit Tests (32-36)
 * ==============================================================================*/

void test_32_TriggerTransmit_Uninit_ReportsDetError(void)
{
    PduInfoType info = {.SduDataPtr = test_sdu_buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_TriggerTransmit(0, &info));
    TEST_ASSERT_EQUAL(PDUR_E_UNINIT, det_last_error);
}

void test_33_TriggerTransmit_NullPduInfo_ReportsDetError(void)
{
    PduR_Init(&PduR_Config);
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_TriggerTransmit(0, NULL_PTR));
    TEST_ASSERT_EQUAL(PDUR_E_PARAM_POINTER, det_last_error);
}

void test_34_TriggerTransmit_CanIfPathToCom_ForwardsToCom(void)
{
    PduR_Init(&PduR_Config);
    PduInfoType info = {.SduDataPtr = test_sdu_buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_OK, PduR_TriggerTransmit(PDUR_COM_RX_ENGINE_CMD, &info));
    TEST_ASSERT_EQUAL(1, mock_Com_TriggerTransmit_Count);
}

void test_35_TriggerTransmit_CanIfPathToDcm_ForwardsToDcm(void)
{
    PduR_Init(&PduR_Config);
    PduInfoType info = {.SduDataPtr = test_sdu_buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_OK, PduR_TriggerTransmit(PDUR_DCM_RX_DIAG_REQUEST, &info));
    TEST_ASSERT_EQUAL(1, mock_Dcm_TriggerTransmit_Count);
}

void test_36_TriggerTransmit_NoPath_ReturnsNotOk(void)
{
    PduR_Init(&PduR_Config);
    PduInfoType info = {.SduDataPtr = test_sdu_buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_TriggerTransmit(99, &info));
}

/* ================================================================================
 * PduR_CancelTransmitRequest Tests (37-40)
 * ==============================================================================*/

void test_37_CancelTransmit_Uninit_ReportsDetError(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_CancelTransmitRequest(0));
    TEST_ASSERT_EQUAL(PDUR_E_UNINIT, det_last_error);
}

void test_38_CancelTransmit_ComPath_CancelsViaCanIf(void)
{
    PduR_Init(&PduR_Config);
    TEST_ASSERT_EQUAL(E_OK, PduR_CancelTransmitRequest(PDUR_COM_TX_ENGINE_STATUS));
    TEST_ASSERT_EQUAL(1, mock_CanIf_CancelTransmit_Count);
    TEST_ASSERT_EQUAL(0, mock_CanIf_CancelTransmit_LastPduId);
}

void test_39_CancelTransmit_NoPath_ReturnsNotOk(void)
{
    PduR_Init(&PduR_Config);
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_CancelTransmitRequest(99));
    TEST_ASSERT_EQUAL(0, mock_CanIf_CancelTransmit_Count);
}

void test_40_CancelTransmit_DcmPath_NoCanIfDest_ReturnsNotOk(void)
{
    PduR_Init(&PduR_Config);
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_CancelTransmitRequest(PDUR_DCM_RX_DIAG_REQUEST));
    TEST_ASSERT_EQUAL(0, mock_CanIf_CancelTransmit_Count);
}

/* ================================================================================
 * PduR_CancelReceiveRequest & ChangeParameter Tests (41-42)
 * ==============================================================================*/

void test_41_CancelReceive_AlwaysReturnsNotOk(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_CancelReceiveRequest(0));
}

void test_42_ChangeParameter_AlwaysReturnsNotOk(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_ChangeParameterRequest(0, (TPParameterType)1, 100));
}

/* ================================================================================
 * PduR_EnableRouting / DisableRouting Tests (43-48)
 * ==============================================================================*/

void test_43_EnableRouting_ValidId_EnablesPath(void)
{
    PduR_Init(&PduR_Config);
    PduR_InternalState.PathStates[0].IsEnabled = FALSE;
    PduR_EnableRouting(0);
    TEST_ASSERT_TRUE(PduR_InternalState.PathStates[0].IsEnabled);
}

void test_44_EnableRouting_InvalidId_NoEffect(void)
{
    PduR_Init(&PduR_Config);
    PduR_InternalState.PathStates[0].IsEnabled = FALSE;
    PduR_EnableRouting(PDUR_NUMBER_OF_ROUTING_PATH_GROUPS);
    TEST_ASSERT_FALSE(PduR_InternalState.PathStates[0].IsEnabled);
}

void test_45_DisableRouting_ValidId_DisablesPath(void)
{
    PduR_Init(&PduR_Config);
    PduR_DisableRouting(0);
    TEST_ASSERT_FALSE(PduR_InternalState.PathStates[0].IsEnabled);
}

void test_46_DisableRouting_InvalidId_NoEffect(void)
{
    PduR_Init(&PduR_Config);
    PduR_DisableRouting(PDUR_NUMBER_OF_ROUTING_PATH_GROUPS);
    TEST_ASSERT_TRUE(PduR_InternalState.PathStates[0].IsEnabled);
}

void test_47_EnableRouting_BoundaryId_LastValid(void)
{
    PduR_Init(&PduR_Config);
    PduR_InternalState.PathStates[PDUR_NUMBER_OF_ROUTING_PATH_GROUPS - 1].IsEnabled = FALSE;
    PduR_EnableRouting(PDUR_NUMBER_OF_ROUTING_PATH_GROUPS - 1);
    TEST_ASSERT_TRUE(PduR_InternalState.PathStates[PDUR_NUMBER_OF_ROUTING_PATH_GROUPS - 1].IsEnabled);
}

void test_48_DisableRouting_BoundaryId_LastValid(void)
{
    PduR_Init(&PduR_Config);
    PduR_DisableRouting(PDUR_NUMBER_OF_ROUTING_PATH_GROUPS - 1);
    TEST_ASSERT_FALSE(PduR_InternalState.PathStates[PDUR_NUMBER_OF_ROUTING_PATH_GROUPS - 1].IsEnabled);
}

/* ================================================================================
 * PduR_GetVersionInfo Tests (49-51)
 * ==============================================================================*/

void test_49_GetVersionInfo_NullPtr_ReportsDetError(void)
{
    PduR_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL(PDUR_E_PARAM_POINTER, det_last_error);
}

void test_50_GetVersionInfo_ValidPtr_FillsCorrectly(void)
{
    Std_VersionInfoType vi = {0};
    PduR_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL(PDUR_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL(PDUR_MODULE_ID, vi.moduleID);
    TEST_ASSERT_EQUAL(PDUR_SW_MAJOR_VERSION, vi.sw_major_version);
    TEST_ASSERT_EQUAL(PDUR_SW_MINOR_VERSION, vi.sw_minor_version);
    TEST_ASSERT_EQUAL(PDUR_SW_PATCH_VERSION, vi.sw_patch_version);
}

void test_51_GetVersionInfo_Idempotent(void)
{
    Std_VersionInfoType vi1 = {0}, vi2 = {0};
    PduR_GetVersionInfo(&vi1);
    PduR_GetVersionInfo(&vi2);
    TEST_ASSERT_EQUAL(vi1.vendorID, vi2.vendorID);
    TEST_ASSERT_EQUAL(vi1.moduleID, vi2.moduleID);
}

/* ================================================================================
 * PduR_MainFunction Tests (52-55)
 * ==============================================================================*/

void test_52_MainFunction_Uninit_DoesNothing(void)
{
    PduR_MainFunction();
    TEST_ASSERT_EQUAL(0, mock_CanIf_Transmit_Count);
}

void test_53_MainFunction_Init_EmptyFifos_NoAction(void)
{
    PduR_Init(&PduR_Config);
    PduR_MainFunction();
    TEST_ASSERT_EQUAL(0, mock_CanIf_Transmit_Count);
}

void test_54_MainFunction_WithFifoEntry_RetriesRouting(void)
{
    PduR_Init(&PduR_Config);
    PduR_FifoQueueType* fifo = &PduR_InternalState.PathStates[0].FifoQueue;
    uint8 buf[8] = {1,2,3,4,5,6,7,8};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 8};
    PduR_FifoPush(fifo, &info);
    PduR_MainFunction();
    TEST_ASSERT_EQUAL(1, mock_CanIf_Transmit_Count);
    TEST_ASSERT_EQUAL(0, fifo->Count);
}

void test_55_MainFunction_MultiplePaths_ProcessesAll(void)
{
    PduR_Init(&PduR_Config);
    uint8 buf[8] = {0};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 8};
    PduR_FifoPush(&PduR_InternalState.PathStates[0].FifoQueue, &info);
    PduR_FifoPush(&PduR_InternalState.PathStates[1].FifoQueue, &info);
    PduR_MainFunction();
    TEST_ASSERT_EQUAL(0, PduR_InternalState.PathStates[0].FifoQueue.Count);
    TEST_ASSERT_EQUAL(0, PduR_InternalState.PathStates[1].FifoQueue.Count);
}

/* ================================================================================
 * PduR_FindRoutingPath Tests (56-60)
 * ==============================================================================*/

void test_56_FindRoutingPath_NullConfig_ReturnsNotOk(void)
{
    PduR_Init(&PduR_Config);
    PduR_InternalState.ConfigPtr = NULL_PTR;
    uint8 idx = 0xFF;
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_FindRoutingPath(0, PDUR_MODULE_COM, &idx));
}

void test_57_FindRoutingPath_Match_Found(void)
{
    PduR_Init(&PduR_Config);
    uint8 idx = 0xFF;
    TEST_ASSERT_EQUAL(E_OK, PduR_FindRoutingPath(PDUR_COM_TX_ENGINE_STATUS, PDUR_MODULE_COM, &idx));
    TEST_ASSERT_EQUAL(0, idx);
}

void test_58_FindRoutingPath_NoMatch_ReturnsNotOk(void)
{
    PduR_Init(&PduR_Config);
    uint8 idx = 0xFF;
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_FindRoutingPath(99, PDUR_MODULE_COM, &idx));
}

void test_59_FindRoutingPath_WrongModule_ReturnsNotOk(void)
{
    PduR_Init(&PduR_Config);
    uint8 idx = 0xFF;
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_FindRoutingPath(PDUR_COM_TX_ENGINE_STATUS, PDUR_MODULE_DCM, &idx));
}

void test_60_FindRoutingPath_DcmMatch_Found(void)
{
    PduR_Init(&PduR_Config);
    uint8 idx = 0xFF;
    TEST_ASSERT_EQUAL(E_OK, PduR_FindRoutingPath(PDUR_DCM_TX_DIAG_RESPONSE, PDUR_MODULE_DCM, &idx));
    TEST_ASSERT_EQUAL(5, idx);
}

/* ================================================================================
 * PduR_RoutePdu Tests (61-63)
 * ==============================================================================*/

void test_61_RoutePdu_NullPathPtr_ReturnsNotOk(void)
{
    uint8 buf[8] = {0};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_RoutePdu(NULL_PTR, &info));
}

void test_62_RoutePdu_NullPduInfoPtr_ReturnsNotOk(void)
{
    PduR_Init(&PduR_Config);
    TEST_ASSERT_EQUAL(E_NOT_OK,
        PduR_RoutePdu(&PduR_Config.RoutingPaths[0], NULL_PTR));
}

void test_63_RoutePdu_ValidPathAndInfo_ReturnsOk(void)
{
    PduR_Init(&PduR_Config);
    uint8 buf[8] = {0};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_OK, PduR_RoutePdu(&PduR_Config.RoutingPaths[0], &info));
}

/* ================================================================================
 * PduR_RouteToDestination Tests (64-69)
 * ==============================================================================*/

void test_64_RouteToDest_NullDestPtr_ReturnsNotOk(void)
{
    uint8 buf[8] = {0};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_RouteToDestination(NULL_PTR, &info));
}

void test_65_RouteToDest_NullPduInfoPtr_ReturnsNotOk(void)
{
    PduR_DestPduConfigType dest = {.DestPduId = 0, .DestModule = PDUR_MODULE_CANIF,
        .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE, .FifoDepth = 0};
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_RouteToDestination(&dest, NULL_PTR));
}

void test_66_RouteToDest_CanIf_CallsTransmit(void)
{
    PduR_DestPduConfigType dest = {.DestPduId = 5, .DestModule = PDUR_MODULE_CANIF,
        .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE, .FifoDepth = 0};
    uint8 buf[8] = {0};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_OK, PduR_RouteToDestination(&dest, &info));
    TEST_ASSERT_EQUAL(1, mock_CanIf_Transmit_Count);
    TEST_ASSERT_EQUAL(5, mock_CanIf_Transmit_LastPduId);
}

void test_67_RouteToDest_Com_CallsRxIndication(void)
{
    PduR_DestPduConfigType dest = {.DestPduId = 10, .DestModule = PDUR_MODULE_COM,
        .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE, .FifoDepth = 0};
    uint8 buf[8] = {0};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_OK, PduR_RouteToDestination(&dest, &info));
    TEST_ASSERT_EQUAL(1, mock_Com_RxIndication_Count);
    TEST_ASSERT_EQUAL(10, mock_Com_RxIndication_LastPduId);
}

void test_68_RouteToDest_Dcm_CallsRxIndication(void)
{
    PduR_DestPduConfigType dest = {.DestPduId = 11, .DestModule = PDUR_MODULE_DCM,
        .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE, .FifoDepth = 0};
    uint8 buf[8] = {0};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_OK, PduR_RouteToDestination(&dest, &info));
    TEST_ASSERT_EQUAL(1, mock_Dcm_RxIndication_Count);
}

void test_69_RouteToDest_UnknownModule_ReturnsNotOk(void)
{
    PduR_DestPduConfigType dest = {.DestPduId = 0, .DestModule = 200,
        .Processing = PDUR_DESTPDU_PROCESSING_IMMEDIATE, .FifoDepth = 0};
    uint8 buf[8] = {0};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_RouteToDestination(&dest, &info));
}

/* ================================================================================
 * PduR_FifoInit Tests (70-71)
 * ==============================================================================*/

void test_70_FifoInit_NullPtr_NoCrash(void)
{
    PduR_FifoInit(NULL_PTR);
}

void test_71_FifoInit_ValidPtr_AllFieldsCleared(void)
{
    PduR_FifoQueueType fifo;
    fifo.Count = 5; fifo.Head = 3; fifo.Tail = 3;
    PduR_FifoInit(&fifo);
    TEST_ASSERT_EQUAL(0, fifo.Head);
    TEST_ASSERT_EQUAL(0, fifo.Tail);
    TEST_ASSERT_EQUAL(0, fifo.Count);
    for (uint8 i = 0; i < PDUR_MAX_FIFO_DEPTH; i++) {
        TEST_ASSERT_FALSE(fifo.Entries[i].IsValid);
    }
}

/* ================================================================================
 * PduR_FifoPush Tests (72-77)
 * ==============================================================================*/

void test_72_FifoPush_NullFifo_ReturnsNotOk(void)
{
    uint8 buf[8] = {0};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_FifoPush(NULL_PTR, &info));
}

void test_73_FifoPush_NullInfo_ReturnsNotOk(void)
{
    PduR_FifoQueueType fifo;
    PduR_FifoInit(&fifo);
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_FifoPush(&fifo, NULL_PTR));
}

void test_74_FifoPush_ValidData_Succeeds(void)
{
    PduR_FifoQueueType fifo;
    PduR_FifoInit(&fifo);
    uint8 buf[8] = {1,2,3,4,5,6,7,8};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 8};
    TEST_ASSERT_EQUAL(E_OK, PduR_FifoPush(&fifo, &info));
    TEST_ASSERT_EQUAL(1, fifo.Count);
    TEST_ASSERT_EQUAL(1, fifo.Tail);
    TEST_ASSERT_TRUE(fifo.Entries[0].IsValid);
    TEST_ASSERT_EQUAL(8, fifo.Entries[0].PduInfo.SduLength);
    TEST_ASSERT_EQUAL_MEMORY(buf, fifo.Entries[0].SduData, 8);
}

void test_75_FifoPush_Full_ReturnsNotOk(void)
{
    PduR_FifoQueueType fifo;
    PduR_FifoInit(&fifo);
    uint8 buf[8] = {0};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 8};
    for (uint8 i = 0; i < PDUR_MAX_FIFO_DEPTH; i++) {
        TEST_ASSERT_EQUAL(E_OK, PduR_FifoPush(&fifo, &info));
    }
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_FifoPush(&fifo, &info));
    TEST_ASSERT_EQUAL(PDUR_MAX_FIFO_DEPTH, fifo.Count);
}

void test_76_FifoPush_NullSduDataPtr_SduDataPtrSetNull(void)
{
    PduR_FifoQueueType fifo;
    PduR_FifoInit(&fifo);
    PduInfoType info = {.SduDataPtr = NULL_PTR, .MetaDataPtr = NULL, .SduLength = 0};
    TEST_ASSERT_EQUAL(E_OK, PduR_FifoPush(&fifo, &info));
    TEST_ASSERT_NULL(fifo.Entries[0].PduInfo.SduDataPtr);
}

void test_77_FifoPush_ZeroLength_SduDataPtrSetNull(void)
{
    PduR_FifoQueueType fifo;
    PduR_FifoInit(&fifo);
    uint8 buf[1] = {0};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 0};
    TEST_ASSERT_EQUAL(E_OK, PduR_FifoPush(&fifo, &info));
    TEST_ASSERT_NULL(fifo.Entries[0].PduInfo.SduDataPtr);
}

/* ================================================================================
 * PduR_FifoPop Tests (78-83)
 * ==============================================================================*/

void test_78_FifoPop_NullFifo_ReturnsNotOk(void)
{
    PduInfoType out;
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_FifoPop(NULL_PTR, &out));
}

void test_79_FifoPop_NullOut_ReturnsNotOk(void)
{
    PduR_FifoQueueType fifo;
    PduR_FifoInit(&fifo);
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_FifoPop(&fifo, NULL_PTR));
}

void test_80_FifoPop_Empty_ReturnsNotOk(void)
{
    PduR_FifoQueueType fifo;
    PduR_FifoInit(&fifo);
    PduInfoType out;
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_FifoPop(&fifo, &out));
}

void test_81_FifoPop_ValidEntry_ReturnsOkAndData(void)
{
    PduR_FifoQueueType fifo;
    PduR_FifoInit(&fifo);
    uint8 buf[8] = {10,20,30,40,50,60,70,80};
    PduInfoType in = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 8};
    PduR_FifoPush(&fifo, &in);

    PduInfoType out = {0};
    TEST_ASSERT_EQUAL(E_OK, PduR_FifoPop(&fifo, &out));
    TEST_ASSERT_EQUAL(8, out.SduLength);
    TEST_ASSERT_NOT_NULL(out.SduDataPtr);
    TEST_ASSERT_EQUAL_MEMORY(buf, out.SduDataPtr, 8);
    TEST_ASSERT_EQUAL(0, fifo.Count);
}

void test_82_FifoPop_InvalidEntry_ReturnsNotOk(void)
{
    PduR_FifoQueueType fifo;
    PduR_FifoInit(&fifo);
    fifo.Count = 1;
    fifo.Entries[0].IsValid = FALSE;
    PduInfoType out;
    TEST_ASSERT_EQUAL(E_NOT_OK, PduR_FifoPop(&fifo, &out));
}

void test_83_FifoPop_FIFOOrder_FirstInFirstOut(void)
{
    PduR_FifoQueueType fifo;
    PduR_FifoInit(&fifo);
    uint8 buf1[4] = {1,1,1,1};
    uint8 buf2[4] = {2,2,2,2};
    PduInfoType in1 = {.SduDataPtr = buf1, .MetaDataPtr = NULL, .SduLength = 4};
    PduInfoType in2 = {.SduDataPtr = buf2, .MetaDataPtr = NULL, .SduLength = 4};
    PduR_FifoPush(&fifo, &in1);
    PduR_FifoPush(&fifo, &in2);

    PduInfoType out = {0};
    PduR_FifoPop(&fifo, &out);
    TEST_ASSERT_EQUAL_MEMORY(buf1, out.SduDataPtr, 4);
    PduR_FifoPop(&fifo, &out);
    TEST_ASSERT_EQUAL_MEMORY(buf2, out.SduDataPtr, 4);
}

/* ================================================================================
 * PduR_FifoIsEmpty Tests (84-86)
 * ==============================================================================*/

void test_84_FifoIsEmpty_NullPtr_ReturnsTrue(void)
{
    TEST_ASSERT_TRUE(PduR_FifoIsEmpty(NULL_PTR));
}

void test_85_FifoIsEmpty_Empty_ReturnsTrue(void)
{
    PduR_FifoQueueType fifo;
    PduR_FifoInit(&fifo);
    TEST_ASSERT_TRUE(PduR_FifoIsEmpty(&fifo));
}

void test_86_FifoIsEmpty_NotEmpty_ReturnsFalse(void)
{
    PduR_FifoQueueType fifo;
    PduR_FifoInit(&fifo);
    uint8 buf[4] = {0};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 4};
    PduR_FifoPush(&fifo, &info);
    TEST_ASSERT_FALSE(PduR_FifoIsEmpty(&fifo));
}

/* ================================================================================
 * PduR_FifoIsFull Tests (87-89)
 * ==============================================================================*/

void test_87_FifoIsFull_NullPtr_ReturnsTrue(void)
{
    TEST_ASSERT_TRUE(PduR_FifoIsFull(NULL_PTR));
}

void test_88_FifoIsFull_NotFull_ReturnsFalse(void)
{
    PduR_FifoQueueType fifo;
    PduR_FifoInit(&fifo);
    TEST_ASSERT_FALSE(PduR_FifoIsFull(&fifo));
}

void test_89_FifoIsFull_Full_ReturnsTrue(void)
{
    PduR_FifoQueueType fifo;
    PduR_FifoInit(&fifo);
    uint8 buf[4] = {0};
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 4};
    for (uint8 i = 0; i < PDUR_MAX_FIFO_DEPTH; i++) {
        PduR_FifoPush(&fifo, &info);
    }
    TEST_ASSERT_TRUE(PduR_FifoIsFull(&fifo));
}

/* ================================================================================
 * PduR_FifoPush Large Data (truncation) Test (90)
 * ==============================================================================*/

void test_90_FifoPush_LargeData_TruncatesToBufferSize(void)
{
    PduR_FifoQueueType fifo;
    PduR_FifoInit(&fifo);
    uint8 buf[128];
    memset(buf, 0xAA, sizeof(buf));
    PduInfoType info = {.SduDataPtr = buf, .MetaDataPtr = NULL, .SduLength = 128};
    TEST_ASSERT_EQUAL(E_OK, PduR_FifoPush(&fifo, &info));
    TEST_ASSERT_EQUAL(128, fifo.Entries[0].PduInfo.SduLength);
    uint16 maxBuf = PDUR_MAX_DESTINATIONS_PER_PATH * 8U;
    TEST_ASSERT_EQUAL_MEMORY(buf, fifo.Entries[0].SduData, maxBuf);
}

/* ================================================================================
 * Main
 * ==============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    /* Init */
    RUN_TEST(test_01_Init_NullPtr_ReportsDetError);
    RUN_TEST(test_02_Init_ValidConfig_StateBecomesInit);
    RUN_TEST(test_03_Init_AllPathStatesEnabled);
    RUN_TEST(test_04_Init_FifoQueuesCleared);
    /* DeInit */
    RUN_TEST(test_05_DeInit_Uninitialized_ReportsDetError);
    RUN_TEST(test_06_DeInit_AfterInit_ConfigCleared);
    RUN_TEST(test_07_DeInit_AfterInit_StateUninit);
    /* Transmit */
    RUN_TEST(test_08_Transmit_Uninit_ReportsDetError);
    RUN_TEST(test_09_Transmit_NullPduInfo_ReportsDetError);
    RUN_TEST(test_10_Transmit_ComToCanIf_RoutesSuccessfully);
    RUN_TEST(test_11_Transmit_DcmToCanIf_RoutesSuccessfully);
    RUN_TEST(test_12_Transmit_DisabledRouting_ReturnsNotOk);
    RUN_TEST(test_13_Transmit_NoRoutingPath_ReportsDetError);
    RUN_TEST(test_14_Transmit_CanIfReturnsNotOk_ReturnsNotOk);
    RUN_TEST(test_15_Transmit_DcmDisabled_ReturnsNotOk);
    RUN_TEST(test_16_Transmit_ComFallbackToDcm_Disabled_ReturnsNotOk);
    /* RxIndication */
    RUN_TEST(test_17_RxIndication_Uninit_ReportsDetError);
    RUN_TEST(test_18_RxIndication_NullPduInfo_ReportsDetError);
    RUN_TEST(test_19_RxIndication_CanIfToCom_RoutesSuccessfully);
    RUN_TEST(test_20_RxIndication_CanIfToDcm_RoutesSuccessfully);
    RUN_TEST(test_21_RxIndication_DisabledPath_ReturnsSilently);
    RUN_TEST(test_22_RxIndication_NoRoutingPath_ReportsDetError);
    /* TxConfirmation */
    RUN_TEST(test_23_TxConfirmation_Uninit_ReportsDetError);
    RUN_TEST(test_24_TxConfirmation_CanIfPathToDcm_ForwardsConfirmation);
    RUN_TEST(test_25_TxConfirmation_DisabledPath_ReturnsSilently);
    RUN_TEST(test_26_TxConfirmation_FallbackComPath_ForwardsToCom);
    RUN_TEST(test_27_TxConfirmation_FallbackDcmPath_ForwardsToDcm);
    RUN_TEST(test_28_TxConfirmation_FallbackCom_Disabled_ReturnsSilently);
    RUN_TEST(test_29_TxConfirmation_FallbackDcm_Disabled_ReturnsSilently);
    RUN_TEST(test_30_TxConfirmation_NoPathFound_NoCallback);
    RUN_TEST(test_31_TxConfirmation_FallbackCom_DestNotCanIf_NoCallback);
    /* TriggerTransmit */
    RUN_TEST(test_32_TriggerTransmit_Uninit_ReportsDetError);
    RUN_TEST(test_33_TriggerTransmit_NullPduInfo_ReportsDetError);
    RUN_TEST(test_34_TriggerTransmit_CanIfPathToCom_ForwardsToCom);
    RUN_TEST(test_35_TriggerTransmit_CanIfPathToDcm_ForwardsToDcm);
    RUN_TEST(test_36_TriggerTransmit_NoPath_ReturnsNotOk);
    /* CancelTransmitRequest */
    RUN_TEST(test_37_CancelTransmit_Uninit_ReportsDetError);
    RUN_TEST(test_38_CancelTransmit_ComPath_CancelsViaCanIf);
    RUN_TEST(test_39_CancelTransmit_NoPath_ReturnsNotOk);
    RUN_TEST(test_40_CancelTransmit_DcmPath_NoCanIfDest_ReturnsNotOk);
    /* CancelReceive & ChangeParameter */
    RUN_TEST(test_41_CancelReceive_AlwaysReturnsNotOk);
    RUN_TEST(test_42_ChangeParameter_AlwaysReturnsNotOk);
    /* EnableRouting / DisableRouting */
    RUN_TEST(test_43_EnableRouting_ValidId_EnablesPath);
    RUN_TEST(test_44_EnableRouting_InvalidId_NoEffect);
    RUN_TEST(test_45_DisableRouting_ValidId_DisablesPath);
    RUN_TEST(test_46_DisableRouting_InvalidId_NoEffect);
    RUN_TEST(test_47_EnableRouting_BoundaryId_LastValid);
    RUN_TEST(test_48_DisableRouting_BoundaryId_LastValid);
    /* GetVersionInfo */
    RUN_TEST(test_49_GetVersionInfo_NullPtr_ReportsDetError);
    RUN_TEST(test_50_GetVersionInfo_ValidPtr_FillsCorrectly);
    RUN_TEST(test_51_GetVersionInfo_Idempotent);
    /* MainFunction */
    RUN_TEST(test_52_MainFunction_Uninit_DoesNothing);
    RUN_TEST(test_53_MainFunction_Init_EmptyFifos_NoAction);
    RUN_TEST(test_54_MainFunction_WithFifoEntry_RetriesRouting);
    RUN_TEST(test_55_MainFunction_MultiplePaths_ProcessesAll);
    /* FindRoutingPath */
    RUN_TEST(test_56_FindRoutingPath_NullConfig_ReturnsNotOk);
    RUN_TEST(test_57_FindRoutingPath_Match_Found);
    RUN_TEST(test_58_FindRoutingPath_NoMatch_ReturnsNotOk);
    RUN_TEST(test_59_FindRoutingPath_WrongModule_ReturnsNotOk);
    RUN_TEST(test_60_FindRoutingPath_DcmMatch_Found);
    /* RoutePdu */
    RUN_TEST(test_61_RoutePdu_NullPathPtr_ReturnsNotOk);
    RUN_TEST(test_62_RoutePdu_NullPduInfoPtr_ReturnsNotOk);
    RUN_TEST(test_63_RoutePdu_ValidPathAndInfo_ReturnsOk);
    /* RouteToDestination */
    RUN_TEST(test_64_RouteToDest_NullDestPtr_ReturnsNotOk);
    RUN_TEST(test_65_RouteToDest_NullPduInfoPtr_ReturnsNotOk);
    RUN_TEST(test_66_RouteToDest_CanIf_CallsTransmit);
    RUN_TEST(test_67_RouteToDest_Com_CallsRxIndication);
    RUN_TEST(test_68_RouteToDest_Dcm_CallsRxIndication);
    RUN_TEST(test_69_RouteToDest_UnknownModule_ReturnsNotOk);
    /* FifoInit */
    RUN_TEST(test_70_FifoInit_NullPtr_NoCrash);
    RUN_TEST(test_71_FifoInit_ValidPtr_AllFieldsCleared);
    /* FifoPush */
    RUN_TEST(test_72_FifoPush_NullFifo_ReturnsNotOk);
    RUN_TEST(test_73_FifoPush_NullInfo_ReturnsNotOk);
    RUN_TEST(test_74_FifoPush_ValidData_Succeeds);
    RUN_TEST(test_75_FifoPush_Full_ReturnsNotOk);
    RUN_TEST(test_76_FifoPush_NullSduDataPtr_SduDataPtrSetNull);
    RUN_TEST(test_77_FifoPush_ZeroLength_SduDataPtrSetNull);
    /* FifoPop */
    RUN_TEST(test_78_FifoPop_NullFifo_ReturnsNotOk);
    RUN_TEST(test_79_FifoPop_NullOut_ReturnsNotOk);
    RUN_TEST(test_80_FifoPop_Empty_ReturnsNotOk);
    RUN_TEST(test_81_FifoPop_ValidEntry_ReturnsOkAndData);
    RUN_TEST(test_82_FifoPop_InvalidEntry_ReturnsNotOk);
    RUN_TEST(test_83_FifoPop_FIFOOrder_FirstInFirstOut);
    /* FifoIsEmpty */
    RUN_TEST(test_84_FifoIsEmpty_NullPtr_ReturnsTrue);
    RUN_TEST(test_85_FifoIsEmpty_Empty_ReturnsTrue);
    RUN_TEST(test_86_FifoIsEmpty_NotEmpty_ReturnsFalse);
    /* FifoIsFull */
    RUN_TEST(test_87_FifoIsFull_NullPtr_ReturnsTrue);
    RUN_TEST(test_88_FifoIsFull_NotFull_ReturnsFalse);
    RUN_TEST(test_89_FifoIsFull_Full_ReturnsTrue);
    /* Large data truncation */
    RUN_TEST(test_90_FifoPush_LargeData_TruncatesToBufferSize);

    return UNITY_END();
}
