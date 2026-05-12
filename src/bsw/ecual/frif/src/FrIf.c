/**
 * @file FrIf.c
 * @brief FlexRay Interface implementation
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include "FrIf.h"
#include "FrIf_Cfg.h"
#include "PduR.h"
#include "Det.h"

#define FRIF_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static boolean FrIf_Initialized = FALSE;
static const FrIf_ConfigType* FrIf_ConfigPtr = NULL_PTR;
static FrIf_ControllerModeType FrIf_ControllerMode[FRIF_NUM_CONTROLLERS];
static FrIf_TransceiverModeType FrIf_TransceiverMode[FRIF_NUM_CONTROLLERS];

/* Timer management */
typedef struct {
    boolean Active;
    uint8 Cycle;
    uint16 Offset;
    boolean IsAbsolute;
} FrIf_TimerType;

static FrIf_TimerType FrIf_AbsoluteTimer[FRIF_NUM_CONTROLLERS][4];  /* 4 timers per controller */
static FrIf_TimerType FrIf_RelativeTimer[FRIF_NUM_CONTROLLERS][4];

/* LPDU state */
typedef struct {
    boolean Configured;
    uint8 CtrlIdx;
    uint16 SlotId;
    uint8 Cycle;
    uint8 PayloadLength;
    boolean DynamicSegment;
} FrIf_LpduStateType;

static FrIf_LpduStateType FrIf_LpduState[FRIF_NUM_LPDUS];

#define FRIF_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

#define FRIF_START_SEC_CODE
#include "MemMap.h"

void FrIf_Init(const FrIf_ConfigType* ConfigPtr)
{
    #if (FRIF_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_INIT, FRIF_E_INV_CONFIG);
        return;
    }
    if (FrIf_Initialized == TRUE) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_INIT, FRIF_E_ALREADY_INITIALIZED);
        return;
    }
    #endif

    FrIf_ConfigPtr = ConfigPtr;

    /* Initialize controllers */
    for (uint8 i = 0U; i < FRIF_NUM_CONTROLLERS; i++) {
        FrIf_ControllerMode[i] = FRIF_MODE_STANDBY;
        FrIf_TransceiverMode[i] = FRIF_TRCV_MODE_SLEEP;

        /* Initialize timers */
        for (uint8 j = 0U; j < 4U; j++) {
            FrIf_AbsoluteTimer[i][j].Active = FALSE;
            FrIf_RelativeTimer[i][j].Active = FALSE;
        }
    }

    /* Initialize LPDU states */
    for (uint16 i = 0U; i < FRIF_NUM_LPDUS; i++) {
        FrIf_LpduState[i].Configured = FALSE;
        FrIf_LpduState[i].CtrlIdx = 0xFFU;
        FrIf_LpduState[i].SlotId = 0U;
        FrIf_LpduState[i].Cycle = 0U;
        FrIf_LpduState[i].PayloadLength = 0U;
        FrIf_LpduState[i].DynamicSegment = FALSE;
    }

    /* Load LPDU configuration */
    if (ConfigPtr->Lpdus != NULL_PTR) {
        for (uint16 i = 0U; i < ConfigPtr->NumLpdus; i++) {
            uint16 lpduIdx = ConfigPtr->Lpdus[i].LpduIdx;
            if (lpduIdx < FRIF_NUM_LPDUS) {
                FrIf_LpduState[lpduIdx].Configured = TRUE;
                FrIf_LpduState[lpduIdx].CtrlIdx = ConfigPtr->Lpdus[i].CtrlIdx;
                FrIf_LpduState[lpduIdx].SlotId = ConfigPtr->Lpdus[i].SlotId;
                FrIf_LpduState[lpduIdx].Cycle = ConfigPtr->Lpdus[i].Cycle;
                FrIf_LpduState[lpduIdx].PayloadLength = ConfigPtr->Lpdus[i].PayloadLength;
                FrIf_LpduState[lpduIdx].DynamicSegment = ConfigPtr->Lpdus[i].DynamicSegment;
            }
        }
    }

    FrIf_Initialized = TRUE;
}

Std_ReturnType FrIf_ControllerInit(uint8 FrIf_CtrlIdx)
{
    #if (FRIF_DEV_ERROR_DETECT == STD_ON)
    if (FrIf_Initialized == FALSE) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_CONTROLLERINIT, FRIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (FrIf_CtrlIdx >= FRIF_NUM_CONTROLLERS) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_CONTROLLERINIT, FRIF_E_INV_CTRL_IDX);
        return E_NOT_OK;
    }
    #endif

    /* Initialize FlexRay controller */
    /* In a real implementation, this would call the FlexRay driver */

    FrIf_ControllerMode[FrIf_CtrlIdx] = FRIF_MODE_READY;

    return E_OK;
}

Std_ReturnType FrIf_SetAbsoluteTimer(uint8 FrIf_CtrlIdx,
                                      uint8 FrIf_AbsTimerIdx,
                                      uint8 FrIf_Cycle,
                                      uint16 FrIf_Offset)
{
    #if (FRIF_DEV_ERROR_DETECT == STD_ON)
    if (FrIf_Initialized == FALSE) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_SETABSOLUTETIMER, FRIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (FrIf_CtrlIdx >= FRIF_NUM_CONTROLLERS) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_SETABSOLUTETIMER, FRIF_E_INV_CTRL_IDX);
        return E_NOT_OK;
    }
    if (FrIf_AbsTimerIdx >= 4U) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_SETABSOLUTETIMER, FRIF_E_INV_TIMER_IDX);
        return E_NOT_OK;
    }
    #endif

    FrIf_AbsoluteTimer[FrIf_CtrlIdx][FrIf_AbsTimerIdx].Active = TRUE;
    FrIf_AbsoluteTimer[FrIf_CtrlIdx][FrIf_AbsTimerIdx].Cycle = FrIf_Cycle;
    FrIf_AbsoluteTimer[FrIf_CtrlIdx][FrIf_AbsTimerIdx].Offset = FrIf_Offset;
    FrIf_AbsoluteTimer[FrIf_CtrlIdx][FrIf_AbsTimerIdx].IsAbsolute = TRUE;

    /* In a real implementation, this would program the FlexRay timer */

    return E_OK;
}

Std_ReturnType FrIf_SetRelativeTimer(uint8 FrIf_CtrlIdx,
                                      uint8 FrIf_RelTimerIdx,
                                      uint16 FrIf_Offset)
{
    #if (FRIF_DEV_ERROR_DETECT == STD_ON)
    if (FrIf_Initialized == FALSE) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_SETRELATIVETIMER, FRIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (FrIf_CtrlIdx >= FRIF_NUM_CONTROLLERS) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_SETRELATIVETIMER, FRIF_E_INV_CTRL_IDX);
        return E_NOT_OK;
    }
    if (FrIf_RelTimerIdx >= 4U) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_SETRELATIVETIMER, FRIF_E_INV_TIMER_IDX);
        return E_NOT_OK;
    }
    #endif

    FrIf_RelativeTimer[FrIf_CtrlIdx][FrIf_RelTimerIdx].Active = TRUE;
    FrIf_RelativeTimer[FrIf_CtrlIdx][FrIf_RelTimerIdx].Offset = FrIf_Offset;
    FrIf_RelativeTimer[FrIf_CtrlIdx][FrIf_RelTimerIdx].IsAbsolute = FALSE;

    return E_OK;
}

Std_ReturnType FrIf_CancelAbsoluteTimer(uint8 FrIf_CtrlIdx, uint8 FrIf_AbsTimerIdx)
{
    #if (FRIF_DEV_ERROR_DETECT == STD_ON)
    if (FrIf_Initialized == FALSE) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_CANCELABSOLUTETIMER, FRIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (FrIf_CtrlIdx >= FRIF_NUM_CONTROLLERS) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_CANCELABSOLUTETIMER, FRIF_E_INV_CTRL_IDX);
        return E_NOT_OK;
    }
    if (FrIf_AbsTimerIdx >= 4U) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_CANCELABSOLUTETIMER, FRIF_E_INV_TIMER_IDX);
        return E_NOT_OK;
    }
    #endif

    FrIf_AbsoluteTimer[FrIf_CtrlIdx][FrIf_AbsTimerIdx].Active = FALSE;

    return E_OK;
}

Std_ReturnType FrIf_CancelRelativeTimer(uint8 FrIf_CtrlIdx, uint8 FrIf_RelTimerIdx)
{
    #if (FRIF_DEV_ERROR_DETECT == STD_ON)
    if (FrIf_Initialized == FALSE) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_CANCELRELATIVETIMER, FRIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (FrIf_CtrlIdx >= FRIF_NUM_CONTROLLERS) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_CANCELRELATIVETIMER, FRIF_E_INV_CTRL_IDX);
        return E_NOT_OK;
    }
    if (FrIf_RelTimerIdx >= 4U) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_CANCELRELATIVETIMER, FRIF_E_INV_TIMER_IDX);
        return E_NOT_OK;
    }
    #endif

    FrIf_RelativeTimer[FrIf_CtrlIdx][FrIf_RelTimerIdx].Active = FALSE;

    return E_OK;
}

Std_ReturnType FrIf_Transmit(PduIdType FrIf_TxPduId, const PduInfoType* FrIf_PduInfoPtr)
{
    #if (FRIF_DEV_ERROR_DETECT == STD_ON)
    if (FrIf_Initialized == FALSE) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_TRANSMIT, FRIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (FrIf_TxPduId >= FRIF_NUM_LPDUS) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_TRANSMIT, FRIF_E_INV_LPDU_IDX);
        return E_NOT_OK;
    }
    if (FrIf_PduInfoPtr == NULL_PTR) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_TRANSMIT, FRIF_E_INV_POINTER);
        return E_NOT_OK;
    }
    #endif

    /* Check LPDU is configured */
    if (FrIf_LpduState[FrIf_TxPduId].Configured == FALSE) {
        return E_NOT_OK;
    }

    /* Check controller is in active state */
    uint8 ctrlIdx = FrIf_LpduState[FrIf_TxPduId].CtrlIdx;
    if (ctrlIdx >= FRIF_NUM_CONTROLLERS) {
        return E_NOT_OK;
    }

    if (FrIf_ControllerMode[ctrlIdx] != FRIF_MODE_NORMAL_ACTIVE) {
        return E_NOT_OK;
    }

    /* In a real implementation, this would:
     * 1. Copy data to FlexRay controller buffer
     * 2. Configure frame header
     * 3. Enable transmission for the slot
     */

    return E_OK;
}

Std_ReturnType FrIf_GetPOCStatus(uint8 FrIf_CtrlIdx, FrIf_POCStatusType* FrIf_POCStatusPtr)
{
    #if (FRIF_DEV_ERROR_DETECT == STD_ON)
    if (FrIf_Initialized == FALSE) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_GETPOCSTATUS, FRIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (FrIf_CtrlIdx >= FRIF_NUM_CONTROLLERS) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_GETPOCSTATUS, FRIF_E_INV_CTRL_IDX);
        return E_NOT_OK;
    }
    if (FrIf_POCStatusPtr == NULL_PTR) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_GETPOCSTATUS, FRIF_E_INV_POINTER);
        return E_NOT_OK;
    }
    #endif

    /* In a real implementation, this would read the POC status from FlexRay controller */
    FrIf_POCStatusPtr->State = (uint8)FrIf_ControllerMode[FrIf_CtrlIdx];
    FrIf_POCStatusPtr->SubState = 0U;
    FrIf_POCStatusPtr->ColdstartNoise = 0U;
    FrIf_POCStatusPtr->WakeupStatus = 0U;

    return E_OK;
}

Std_ReturnType FrIf_GetGlobalTime(uint8 FrIf_CtrlIdx,
                                   uint8* FrIf_CyclePtr,
                                   uint16* FrIf_MacrotickPtr)
{
    #if (FRIF_DEV_ERROR_DETECT == STD_ON)
    if (FrIf_Initialized == FALSE) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_GETGLOBALTIME, FRIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (FrIf_CtrlIdx >= FRIF_NUM_CONTROLLERS) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_GETGLOBALTIME, FRIF_E_INV_CTRL_IDX);
        return E_NOT_OK;
    }
    if ((FrIf_CyclePtr == NULL_PTR) || (FrIf_MacrotickPtr == NULL_PTR)) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_GETGLOBALTIME, FRIF_E_INV_POINTER);
        return E_NOT_OK;
    }
    #endif

    /* In a real implementation, this would read the global time from FlexRay controller */
    *FrIf_CyclePtr = 0U;
    *FrIf_MacrotickPtr = 0U;

    return E_OK;
}

Std_ReturnType FrIf_AllowColdstart(uint8 FrIf_CtrlIdx)
{
    #if (FRIF_DEV_ERROR_DETECT == STD_ON)
    if (FrIf_Initialized == FALSE) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_ALLOWCOLDSTART, FRIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (FrIf_CtrlIdx >= FRIF_NUM_CONTROLLERS) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_ALLOWCOLDSTART, FRIF_E_INV_CTRL_IDX);
        return E_NOT_OK;
    }
    #endif

    #if (FRIF_COLDSTART_SUPPORT == STD_ON)
    /* Enable coldstart capability */
    FrIf_ControllerMode[FrIf_CtrlIdx] = FRIF_MODE_COLDSTART;

    /* In a real implementation, this would configure the FlexRay controller
     * to participate in coldstart */

    return E_OK;
    #else
    (void)FrIf_CtrlIdx;
    return E_NOT_OK;
    #endif
}

Std_ReturnType FrIf_HaltCommunication(uint8 FrIf_CtrlIdx)
{
    #if (FRIF_DEV_ERROR_DETECT == STD_ON)
    if (FrIf_Initialized == FALSE) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_HALTCOMMUNICATION, FRIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (FrIf_CtrlIdx >= FRIF_NUM_CONTROLLERS) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_HALTCOMMUNICATION, FRIF_E_INV_CTRL_IDX);
        return E_NOT_OK;
    }
    #endif

    FrIf_ControllerMode[FrIf_CtrlIdx] = FRIF_MODE_HALT;

    /* In a real implementation, this would halt the FlexRay communication */

    return E_OK;
}

Std_ReturnType FrIf_AbortCommunication(uint8 FrIf_CtrlIdx)
{
    #if (FRIF_DEV_ERROR_DETECT == STD_ON)
    if (FrIf_Initialized == FALSE) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_ABORTCOMMUNICATION, FRIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (FrIf_CtrlIdx >= FRIF_NUM_CONTROLLERS) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_ABORTCOMMUNICATION, FRIF_E_INV_CTRL_IDX);
        return E_NOT_OK;
    }
    #endif

    FrIf_ControllerMode[FrIf_CtrlIdx] = FRIF_MODE_STANDBY;

    /* In a real implementation, this would abort the FlexRay communication */

    return E_OK;
}

Std_ReturnType FrIf_SendWUP(uint8 FrIf_CtrlIdx)
{
    #if (FRIF_DEV_ERROR_DETECT == STD_ON)
    if (FrIf_Initialized == FALSE) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_WAKEUPCTRL, FRIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (FrIf_CtrlIdx >= FRIF_NUM_CONTROLLERS) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_WAKEUPCTRL, FRIF_E_INV_CTRL_IDX);
        return E_NOT_OK;
    }
    #endif

    #if (FRIF_WAKEUP_SUPPORT == STD_ON)
    FrIf_ControllerMode[FrIf_CtrlIdx] = FRIF_MODE_WAKEUP;

    /* In a real implementation, this would send the wakeup pattern */

    return E_OK;
    #else
    (void)FrIf_CtrlIdx;
    return E_NOT_OK;
    #endif
}

Std_ReturnType FrIf_SetWakeupChannel(uint8 FrIf_CtrlIdx, FrIf_ChannelType FrIf_ChnlIdx)
{
    #if (FRIF_DEV_ERROR_DETECT == STD_ON)
    if (FrIf_Initialized == FALSE) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_SETWAKEUPCHANNEL, FRIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (FrIf_CtrlIdx >= FRIF_NUM_CONTROLLERS) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_SETWAKEUPCHANNEL, FRIF_E_INV_CTRL_IDX);
        return E_NOT_OK;
    }
    if (FrIf_ChnlIdx > FRIF_CHANNEL_AB) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_SETWAKEUPCHANNEL, FRIF_E_INV_CHNL);
        return E_NOT_OK;
    }
    #endif

    #if (FRIF_WAKEUP_SUPPORT == STD_ON)
    /* In a real implementation, this would set the wakeup channel */
    (void)FrIf_ChnlIdx;
    return E_OK;
    #else
    (void)FrIf_CtrlIdx;
    (void)FrIf_ChnlIdx;
    return E_NOT_OK;
    #endif
}

void FrIf_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (FRIF_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(FRIF_MODULE_ID, 0U, FRIF_SID_GETVERSIONINFO, FRIF_E_INV_POINTER);
        return;
    }
    #endif

    versioninfo->vendorID = FRIF_VENDOR_ID;
    versioninfo->moduleID = FRIF_MODULE_ID;
    versioninfo->sw_major_version = FRIF_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = FRIF_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = FRIF_SW_PATCH_VERSION;
}

void FrIf_MainFunction(void)
{
    if (FrIf_Initialized == FALSE) {
        return;
    }

    /* Process each controller */
    for (uint8 i = 0U; i < FRIF_NUM_CONTROLLERS; i++) {
        /* Check timer expirations */
        for (uint8 j = 0U; j < 4U; j++) {
            if (FrIf_AbsoluteTimer[i][j].Active == TRUE) {
                /* Check if timer expired */
                /* In a real implementation, this would compare with actual FlexRay time */
            }

            if (FrIf_RelativeTimer[i][j].Active == TRUE) {
                /* Check if timer expired */
            }
        }

        /* Process pending transmissions */

        /* Handle state machine transitions */
        switch (FrIf_ControllerMode[i]) {
            case FRIF_MODE_STARTUP:
                /* Waiting for startup to complete */
                break;

            case FRIF_MODE_NORMAL_ACTIVE:
                /* Normal operation */
                break;

            case FRIF_MODE_NORMAL_PASSIVE:
                /* Passive operation (listen only) */
                break;

            default:
                break;
        }
    }

    /* Process received frames */
    /* In a real implementation, this would:
     * 1. Check for received frames in FlexRay buffers
     * 2. Route to upper layers based on LPDU configuration
     */
}

#define FRIF_STOP_SEC_CODE
#include "MemMap.h"
