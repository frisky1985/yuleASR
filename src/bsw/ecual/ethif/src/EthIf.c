/**
 * @file EthIf.c
 * @brief Ethernet Interface implementation
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include "EthIf.h"
#include "EthIf_Cfg.h"
#include "Det.h"

#define ETHIF_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static boolean EthIf_Initialized = FALSE;
static const EthIf_ConfigType* EthIf_ConfigPtr = NULL_PTR;
static EthIf_ControllerModeType EthIf_ControllerMode[ETHIF_NUM_CONTROLLERS];
static EthIf_LinkStateType EthIf_LinkState[ETHIF_NUM_CONTROLLERS];
static EthIf_MacAddrType EthIf_CurrentMacAddr[ETHIF_NUM_CONTROLLERS];
static boolean EthIf_EgressTimestampEnabled[ETHIF_NUM_CONTROLLERS];

#define ETHIF_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

#define ETHIF_START_SEC_CODE
#include "MemMap.h"

void EthIf_Init(const EthIf_ConfigType* CfgPtr)
{
    #if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (CfgPtr == NULL_PTR) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_INIT, ETHIF_E_INV_CONFIG);
        return;
    }
    if (EthIf_Initialized == TRUE) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_INIT, ETHIF_E_ALREADY_INITIALIZED);
        return;
    }
    #endif

    EthIf_ConfigPtr = CfgPtr;

    /* Initialize all controllers */
    for (uint8 i = 0U; i < ETHIF_NUM_CONTROLLERS; i++) {
        EthIf_ControllerMode[i] = ETHIF_MODE_DOWN;
        EthIf_LinkState[i] = ETHIF_LINK_STATE_DOWN;
        EthIf_EgressTimestampEnabled[i] = FALSE;

        /* Copy default MAC address */
        if (i < CfgPtr->NumControllers) {
            for (uint8 j = 0U; j < 6U; j++) {
                EthIf_CurrentMacAddr[i][j] = CfgPtr->Controllers[i].PhysAddr[j];
            }
        }
    }

    EthIf_Initialized = TRUE;
}

void EthIf_ControllerInit(uint8 CtrlIdx, uint8 CfgIdx)
{
    #if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (EthIf_Initialized == FALSE) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_CONTROLLERINIT, ETHIF_E_UNINIT);
        return;
    }
    if (CtrlIdx >= ETHIF_NUM_CONTROLLERS) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_CONTROLLERINIT, ETHIF_E_INV_CTRL_IDX);
        return;
    }
    #endif

    /* Initialize the specific controller */
    /* In a real implementation, this would call the Ethernet driver */

    (void)CfgIdx;
}

Std_ReturnType EthIf_SetControllerMode(uint8 CtrlIdx, EthIf_ControllerModeType CtrlMode)
{
    #if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (EthIf_Initialized == FALSE) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_SETCONTROLLERMODE, ETHIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (CtrlIdx >= ETHIF_NUM_CONTROLLERS) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_SETCONTROLLERMODE, ETHIF_E_INV_CTRL_IDX);
        return E_NOT_OK;
    }
    #endif

    Std_ReturnType result = E_OK;

    switch (CtrlMode) {
        case ETHIF_MODE_DOWN:
            /* Disable controller */
            EthIf_ControllerMode[CtrlIdx] = ETHIF_MODE_DOWN;
            EthIf_LinkState[CtrlIdx] = ETHIF_LINK_STATE_DOWN;
            break;

        case ETHIF_MODE_ACTIVE:
            /* Enable controller */
            EthIf_ControllerMode[CtrlIdx] = ETHIF_MODE_ACTIVE;
            /* Link state would be determined by actual hardware */
            EthIf_LinkState[CtrlIdx] = ETHIF_LINK_STATE_ACTIVE;
            break;

        default:
            result = E_NOT_OK;
            break;
    }

    return result;
}

Std_ReturnType EthIf_GetControllerMode(uint8 CtrlIdx, EthIf_ControllerModeType* CtrlModePtr)
{
    #if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (EthIf_Initialized == FALSE) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GETCONTROLLERMODE, ETHIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (CtrlIdx >= ETHIF_NUM_CONTROLLERS) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GETCONTROLLERMODE, ETHIF_E_INV_CTRL_IDX);
        return E_NOT_OK;
    }
    if (CtrlModePtr == NULL_PTR) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GETCONTROLLERMODE, ETHIF_E_INV_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif

    *CtrlModePtr = EthIf_ControllerMode[CtrlIdx];
    return E_OK;
}

void EthIf_GetPhysAddr(uint8 CtrlIdx, uint8* PhysAddrPtr)
{
    #if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (EthIf_Initialized == FALSE) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GETPHYSADDR, ETHIF_E_UNINIT);
        return;
    }
    if (CtrlIdx >= ETHIF_NUM_CONTROLLERS) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GETPHYSADDR, ETHIF_E_INV_CTRL_IDX);
        return;
    }
    if (PhysAddrPtr == NULL_PTR) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GETPHYSADDR, ETHIF_E_INV_PARAM_POINTER);
        return;
    }
    #endif

    for (uint8 i = 0U; i < 6U; i++) {
        PhysAddrPtr[i] = EthIf_CurrentMacAddr[CtrlIdx][i];
    }
}

void EthIf_SetPhysAddr(uint8 CtrlIdx, const uint8* PhysAddr)
{
    #if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (EthIf_Initialized == FALSE) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_SETPHYSADDR, ETHIF_E_UNINIT);
        return;
    }
    if (CtrlIdx >= ETHIF_NUM_CONTROLLERS) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_SETPHYSADDR, ETHIF_E_INV_CTRL_IDX);
        return;
    }
    if (PhysAddr == NULL_PTR) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_SETPHYSADDR, ETHIF_E_INV_PARAM_POINTER);
        return;
    }
    #endif

    for (uint8 i = 0U; i < 6U; i++) {
        EthIf_CurrentMacAddr[CtrlIdx][i] = PhysAddr[i];
    }

    /* In a real implementation, this would update the hardware MAC address */
}

Std_ReturnType EthIf_Transmit(uint8 CtrlIdx,
                               EthIf_FrameType FrameType,
                               const uint8* TxBufferPtrEq,
                               uint16 LenByte)
{
    #if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (EthIf_Initialized == FALSE) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_TRANSMIT, ETHIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (CtrlIdx >= ETHIF_NUM_CONTROLLERS) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_TRANSMIT, ETHIF_E_INV_CTRL_IDX);
        return E_NOT_OK;
    }
    if (TxBufferPtrEq == NULL_PTR) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_TRANSMIT, ETHIF_E_INV_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (LenByte > ETHIF_MTU_DEFAULT) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_TRANSMIT, ETHIF_E_INV_MTU);
        return E_NOT_OK;
    }
    #endif

    /* Check controller is active */
    if (EthIf_ControllerMode[CtrlIdx] != ETHIF_MODE_ACTIVE) {
        return E_NOT_OK;
    }

    /* Check link is up */
    if (EthIf_LinkState[CtrlIdx] != ETHIF_LINK_STATE_ACTIVE) {
        return E_NOT_OK;
    }

    /* In a real implementation, this would:
     * 1. Build Ethernet frame with MAC header
     * 2. Call Ethernet driver to transmit
     */

    (void)FrameType;

    return E_OK;
}

void EthIf_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GETVERSIONINFO, ETHIF_E_INV_PARAM_POINTER);
        return;
    }
    #endif

    versioninfo->vendorID = ETHIF_VENDOR_ID;
    versioninfo->moduleID = ETHIF_MODULE_ID;
    versioninfo->sw_major_version = ETHIF_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = ETHIF_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = ETHIF_SW_PATCH_VERSION;
}

Std_ReturnType EthIf_GetCurrentTime(uint8 CtrlIdx, EthIf_TimestampType* timeStampPtr)
{
    #if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (EthIf_Initialized == FALSE) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GETCURRENTTIME, ETHIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (CtrlIdx >= ETHIF_NUM_CONTROLLERS) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GETCURRENTTIME, ETHIF_E_INV_CTRL_IDX);
        return E_NOT_OK;
    }
    if (timeStampPtr == NULL_PTR) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GETCURRENTTIME, ETHIF_E_INV_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif

    #if (ETHIF_GET_CURRENT_TIME_API == STD_ON)
    /* In a real implementation, this would read from PTP hardware clock */
    timeStampPtr->seconds = 0U;
    timeStampPtr->nanoseconds = 0U;
    return E_OK;
    #else
    (void)timeStampPtr;
    return E_NOT_OK;
    #endif
}

void EthIf_EnableEgressTimeStamp(uint8 CtrlIdx, uint8 BufIdx)
{
    #if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (EthIf_Initialized == FALSE) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_ENABLEEGRESSTIMESTAMP, ETHIF_E_UNINIT);
        return;
    }
    if (CtrlIdx >= ETHIF_NUM_CONTROLLERS) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_ENABLEEGRESSTIMESTAMP, ETHIF_E_INV_CTRL_IDX);
        return;
    }
    #endif

    #if (ETHIF_ENABLE_EGRESS_TIMESTAMP_API == STD_ON)
    EthIf_EgressTimestampEnabled[CtrlIdx] = TRUE;
    (void)BufIdx;
    #else
    (void)CtrlIdx;
    (void)BufIdx;
    #endif
}

void EthIf_GetEgressTimeStamp(uint8 CtrlIdx,
                               uint8 BufIdx,
                               EthIf_TimestampType* timeStampPtr,
                               EthIf_TimestampQualityType* timeStampQualityPtr)
{
    #if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (EthIf_Initialized == FALSE) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GETEGRESSTIMESTAMP, ETHIF_E_UNINIT);
        return;
    }
    if (CtrlIdx >= ETHIF_NUM_CONTROLLERS) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GETEGRESSTIMESTAMP, ETHIF_E_INV_CTRL_IDX);
        return;
    }
    if ((timeStampPtr == NULL_PTR) || (timeStampQualityPtr == NULL_PTR)) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GETEGRESSTIMESTAMP, ETHIF_E_INV_PARAM_POINTER);
        return;
    }
    #endif

    #if (ETHIF_GET_EGRESS_TIMESTAMP_API == STD_ON)
    /* In a real implementation, this would read the egress timestamp from hardware */
    timeStampPtr->seconds = 0U;
    timeStampPtr->nanoseconds = 0U;
    *timeStampQualityPtr = ETHIF_TIMESTAMP_VALID;
    (void)BufIdx;
    #else
    (void)CtrlIdx;
    (void)BufIdx;
    (void)timeStampPtr;
    (void)timeStampQualityPtr;
    #endif
}

void EthIf_GetIngressTimeStamp(uint8 CtrlIdx,
                                const uint8* DataPtr,
                                EthIf_TimestampType* timeStampPtr,
                                EthIf_TimestampQualityType* timeStampQualityPtr)
{
    #if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (EthIf_Initialized == FALSE) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GETINGRESSTIMESTAMP, ETHIF_E_UNINIT);
        return;
    }
    if (CtrlIdx >= ETHIF_NUM_CONTROLLERS) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GETINGRESSTIMESTAMP, ETHIF_E_INV_CTRL_IDX);
        return;
    }
    if ((DataPtr == NULL_PTR) || (timeStampPtr == NULL_PTR) || (timeStampQualityPtr == NULL_PTR)) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GETINGRESSTIMESTAMP, ETHIF_E_INV_PARAM_POINTER);
        return;
    }
    #endif

    #if (ETHIF_GET_INGRESS_TIMESTAMP_API == STD_ON)
    /* In a real implementation, this would read the ingress timestamp from hardware */
    timeStampPtr->seconds = 0U;
    timeStampPtr->nanoseconds = 0U;
    *timeStampQualityPtr = ETHIF_TIMESTAMP_VALID;
    #else
    (void)CtrlIdx;
    (void)DataPtr;
    (void)timeStampPtr;
    (void)timeStampQualityPtr;
    #endif
}

void EthIf_MainFunction(void)
{
    if (EthIf_Initialized == FALSE) {
        return;
    }

    /* Periodic processing for each controller */
    for (uint8 i = 0U; i < ETHIF_NUM_CONTROLLERS; i++) {
        /* Check link state */
        /* In a real implementation, this would poll the PHY status */

        /* Process pending transmissions/receptions */

        /* Handle time synchronization if enabled */
        #if (ETHIF_TIME_SYNC_ENABLED == STD_ON)
        /* gPTP processing */
        #endif
    }
}

void EthIf_RxIndication(uint8 CtrlIdx,
                         EthIf_FrameType FrameType,
                         boolean IsBroadcast,
                         const uint8* PhysAddrPtr,
                         const uint8* DataPtr,
                         uint16 LenByte)
{
    if (EthIf_Initialized == FALSE) {
        return;
    }

    if (CtrlIdx >= ETHIF_NUM_CONTROLLERS) {
        return;
    }

    /* Route frame to appropriate upper layer based on FrameType */
    switch (FrameType) {
        case ETHIF_FRAMETYPE_IPV4:
        case ETHIF_FRAMETYPE_IPV6:
            /* Route to TCP/IP stack */
            break;

        case ETHIF_FRAMETYPE_ARP:
            /* Route to ARP handler */
            break;

        case ETHIF_FRAMETYPE_SOMEIP:
            /* Route to SOME/IP module */
            break;

        default:
            /* Unknown frame type - drop */
            break;
    }

    (void)IsBroadcast;
    (void)PhysAddrPtr;
    (void)DataPtr;
    (void)LenByte;
}

void EthIf_TxConfirmation(uint8 CtrlIdx, uint8 BufIdx)
{
    if (EthIf_Initialized == FALSE) {
        return;
    }

    if (CtrlIdx >= ETHIF_NUM_CONTROLLERS) {
        return;
    }

    /* Handle transmission confirmation */
    /* Notify upper layer modules */

    (void)BufIdx;
}

#define ETHIF_STOP_SEC_CODE
#include "MemMap.h"
