/** @file BswM.c
 *  @brief BSW Mode Manager implementation
 *  @copyright Copyright (c) 2026 YuleTech
 *
 *  @implements AUTOSAR_SWS_BSWModeManager.pdf
 */

#include "BswM.h"
#include "Det.h"

/* Version check */
#if defined(BSWM_AR_RELEASE_MAJOR_VERSION) && (BSWM_AR_RELEASE_MAJOR_VERSION != 4u)
#error "BswM: AR major mismatch"
#endif
#if defined(BSWM_AR_RELEASE_MINOR_VERSION) && (BSWM_AR_RELEASE_MINOR_VERSION != 4u)
#error "BswM: AR minor mismatch"
#endif

#define BSWM_SID_INIT               0x00U
#define BSWM_SID_DEINIT             0x01U
#define BSWM_SID_MAINFUNCTION       0x02U
#define BSWM_SID_REQUEST_MODE       0x03U
#define BSWM_SID_GET_CURRENT_MODE   0x04U
#define BSWM_SID_GET_REQUESTED_MODE 0x05U

#define BSWM_E_PARAM_POINTER        0x10U
#define BSWM_E_UNINIT               0x20U
#define BSWM_E_PARAM_MODE           0x30U
#define BSWM_E_MODE_REQUEST_REJECT  0x40U

typedef enum { BSWM_INTERNAL_UNINIT = 0, BSWM_INTERNAL_INIT } BswM_InternalStateType;

typedef struct {
    BswM_InternalStateType  internalState;
    BswM_ModeType           currentMode;
    BswM_ModeType           requestedMode;
    uint16                  modeRequestMask;
    const BswM_ConfigType*  configPtr;
} BswM_InternalType;

static BswM_InternalType BswM_State = {
    BSWM_INTERNAL_UNINIT,
    BSWM_MODE_VALUE_OFF,
    BSWM_MODE_VALUE_OFF,
    0U,
    NULL_PTR
};

void BswM_Init(const BswM_ConfigType* ConfigPtr)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_INIT, BSWM_E_PARAM_POINTER);
        return;
    }
#endif
    BswM_State.configPtr = ConfigPtr;
    BswM_State.currentMode = BSWM_MODE_VALUE_OFF;
    BswM_State.requestedMode = BSWM_MODE_VALUE_OFF;
    BswM_State.modeRequestMask = 0U;
    BswM_State.internalState = BSWM_INTERNAL_INIT;
}

void BswM_DeInit(void)
{
    BswM_State.internalState = BSWM_INTERNAL_UNINIT;
    BswM_State.configPtr = NULL_PTR;
}

Std_ReturnType BswM_RequestMode(uint8 SwCompositionId, BswM_ModeType Mode)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_REQUEST_MODE, BSWM_E_UNINIT);
        return E_NOT_OK;
    }
#endif
    (void)SwCompositionId;
    BswM_State.requestedMode = Mode;
    BswM_State.modeRequestMask |= (uint16)(1U << (uint8)Mode);
    return E_OK;
}

BswM_ModeType BswM_GetCurrentMode(void)
{
    return BswM_State.currentMode;
}

BswM_ModeType BswM_GetRequestedMode(void)
{
    return BswM_State.requestedMode;
}

void BswM_MainFunction(void)
{
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT || BswM_State.configPtr == NULL_PTR) return;

    if (BswM_State.modeRequestMask != 0U) {
        BswM_State.currentMode = BswM_State.requestedMode;

        for (uint8 i = 0U; i < BswM_State.configPtr->NumActionLists; i++) {
            (void)BswM_State.configPtr->ActionLists[i];
        }

        BswM_State.modeRequestMask = 0U;
    }
}

void BswM_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == versioninfo) {
        Det_ReportError(BSWM_MODULE_ID, 0U, 0xFFU, BSWM_E_PARAM_POINTER);
        return;
    }
#endif
    versioninfo->vendorID = BSWM_VENDOR_ID;
    versioninfo->moduleID = BSWM_MODULE_ID;
    versioninfo->sw_major_version = 1U;
    versioninfo->sw_minor_version = 0U;
    versioninfo->sw_patch_version = 0U;
}