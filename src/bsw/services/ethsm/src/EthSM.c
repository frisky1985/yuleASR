/** @file EthSM.c
 *  @brief Ethernet State Manager implementation
 *  @copyright Copyright (c) 2026 YuleTech
 *
 *  @implements AUTOSAR_SWS_EthernetStateManager.pdf
 */

#include "EthSM.h"
#include "Det.h"

/* Version check */
#if defined(ETHSM_AR_RELEASE_MAJOR_VERSION) && (ETHSM_AR_RELEASE_MAJOR_VERSION != 4u)
#error "EthSM: AR major mismatch"
#endif
#if defined(ETHSM_AR_RELEASE_MINOR_VERSION) && (ETHSM_AR_RELEASE_MINOR_VERSION != 4u)
#error "EthSM: AR minor mismatch"
#endif

#define ETHSM_SID_INIT              0x00U
#define ETHSM_SID_DEINIT            0x01U
#define ETHSM_SID_START            0x02U
#define ETHSM_SID_STOP             0x03U
#define ETHSM_SID_SET_STATE         0x04U
#define ETHSM_SID_GET_STATE         0x05U
#define ETHSM_SID_MAINFUNCTION      0x06U

#define ETHSM_E_PARAM_POINTER       0x10U
#define ETHSM_E_UNINIT              0x20U
#define ETHSM_E_TRANSITION          0x30U
#define ETHSM_E_PARAM_STATE         0x40U

typedef enum {
    ETHSM_INTERNAL_UNINIT = 0,
    ETHSM_INTERNAL_INIT,
    ETHSM_INTERNAL_WAITING
} EthSM_InternalStateType;

typedef struct {
    EthSM_InternalStateType internalState;
    EthSM_StateType currentState;
    EthSM_StateType targetState;
    uint32 transitionTimeout;
    uint32 tickCounter;
    const EthSM_ConfigType* configPtr;
} EthSM_InternalType;

static EthSM_InternalType EthSM_State = { ETHSM_INTERNAL_UNINIT, ETHSM_STATE_OFF, ETHSM_STATE_OFF, 0U, 0U, NULL_PTR };

void EthSM_Init(const EthSM_ConfigType* ConfigPtr)
{
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(ETHSM_MODULE_ID, 0U, ETHSM_SID_INIT, ETHSM_E_PARAM_POINTER);
        return;
    }
#endif
    EthSM_State.configPtr = ConfigPtr;
    EthSM_State.currentState = ETHSM_STATE_OFF;
    EthSM_State.targetState = ETHSM_STATE_OFF;
    EthSM_State.tickCounter = 0U;
    EthSM_State.transitionTimeout = 0U;
    EthSM_State.internalState = ETHSM_INTERNAL_INIT;
}

void EthSM_DeInit(void)
{
    EthSM_State.internalState = ETHSM_INTERNAL_UNINIT;
    EthSM_State.configPtr = NULL_PTR;
}

Std_ReturnType EthSM_Start(void)
{
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
    if (EthSM_State.internalState == ETHSM_INTERNAL_UNINIT) {
        Det_ReportError(ETHSM_MODULE_ID, 0U, ETHSM_SID_START, ETHSM_E_UNINIT);
        return E_NOT_OK;
    }
#endif
    EthSM_State.targetState = ETHSM_STATE_ON;
    EthSM_State.transitionTimeout = 1000U;
    return E_OK;
}

Std_ReturnType EthSM_Stop(void)
{
    if (EthSM_State.internalState == ETHSM_INTERNAL_UNINIT) return E_NOT_OK;
    EthSM_State.targetState = ETHSM_STATE_OFF;
    EthSM_State.transitionTimeout = 500U;
    return E_OK;
}

EthSM_StateType EthSM_GetState(void)
{
    return EthSM_State.currentState;
}

Std_ReturnType EthSM_SetState(EthSM_StateType State)
{
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
    if (EthSM_State.internalState == ETHSM_INTERNAL_UNINIT) {
        Det_ReportError(ETHSM_MODULE_ID, 0U, ETHSM_SID_SET_STATE, ETHSM_E_UNINIT);
        return E_NOT_OK;
    }
    if (State > ETHSM_STATE_SLEEP) {
        Det_ReportError(ETHSM_MODULE_ID, 0U, ETHSM_SID_SET_STATE, ETHSM_E_PARAM_STATE);
        return E_NOT_OK;
    }
#endif
    /* Validate transition */
    switch (EthSM_State.currentState) {
        case ETHSM_STATE_OFF:
            if (State == ETHSM_STATE_ON) { EthSM_State.targetState = State; return E_OK; }
            break;
        case ETHSM_STATE_ON:
            if (State == ETHSM_STATE_OFF || State == ETHSM_STATE_SLEEP) { EthSM_State.targetState = State; return E_OK; }
            break;
        case ETHSM_STATE_SLEEP:
            if (State == ETHSM_STATE_ON) { EthSM_State.targetState = State; return E_OK; }
            break;
        default:
            break;
    }
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
    Det_ReportError(ETHSM_MODULE_ID, 0U, ETHSM_SID_SET_STATE, ETHSM_E_TRANSITION);
#endif
    return E_NOT_OK;
}

void EthSM_MainFunction(void)
{
    if (EthSM_State.internalState == ETHSM_INTERNAL_UNINIT || EthSM_State.configPtr == NULL_PTR) return;

    EthSM_State.tickCounter++;

    /* Process state transition */
    if (EthSM_State.currentState != EthSM_State.targetState) {
        if (EthSM_State.transitionTimeout > 0U && EthSM_State.tickCounter >= EthSM_State.transitionTimeout) {
            /* Transition complete */
            EthSM_State.currentState = EthSM_State.targetState;
            EthSM_State.transitionTimeout = 0U;
        }
    }
}

void EthSM_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == versioninfo) { Det_ReportError(ETHSM_MODULE_ID, 0U, 0xFFU, ETHSM_E_PARAM_POINTER); return; }
#endif
    versioninfo->vendorID = ETHSM_VENDOR_ID;
    versioninfo->moduleID = ETHSM_MODULE_ID;
    versioninfo->sw_major_version = 1U;
    versioninfo->sw_minor_version = 0U;
    versioninfo->sw_patch_version = 0U;
}