/** @file EthIf.c
 *  @brief Ethernet Interface implementation
 *  @copyright Copyright (c) 2026 YuleTech
 *
 *  @implements AUTOSAR_SWS_EthernetInterface.pdf
 */

#include "EthIf.h"
#include "EthIf_Cfg.h"
#include "Det.h"
#include <string.h>

/* Version check */
#if defined(ETHIF_AR_RELEASE_MAJOR_VERSION) && (ETHIF_AR_RELEASE_MAJOR_VERSION != 4u)
#error "EthIf: AR major mismatch"
#endif
#if defined(ETHIF_AR_RELEASE_MINOR_VERSION) && (ETHIF_AR_RELEASE_MINOR_VERSION != 4u)
#error "EthIf: AR minor mismatch"
#endif

#define ETHIF_SID_INIT              0x00U
#define ETHIF_SID_DEINIT            0x01U
#define ETHIF_SID_TRANSMIT          0x02U
#define ETHIF_SID_RX_INDICATION     0x03U
#define ETHIF_SID_MAINFUNCTION      0x04U
#define ETHIF_SID_SET_CONTROLLER_MODE 0x05U
#define ETHIF_SID_GET_CONTROLLER_MODE 0x06U

#define ETHIF_E_PARAM_POINTER       0x10U
#define ETHIF_E_UNINIT              0x20U
#define ETHIF_E_PARAM_CONTROLLER    0x30U
#define ETHIF_E_TRANSMIT_FAILED     0x40U

#define ETHIF_MAX_CONTROLLERS       4U
#define ETHIF_MAX_FILTERS           32U

typedef enum { ETHIF_UNINIT = 0, ETHIF_INIT, ETHIF_ONLINE } EthIf_StateType;

typedef struct {
    EthIf_StateType      state;
    EthIf_ControllerMode controllerModes[ETHIF_MAX_CONTROLLERS];
    uint8                activeControllerCount;
    const EthIf_ConfigType* configPtr;
} EthIf_InternalType;

static EthIf_InternalType EthIf_State;

void EthIf_Init(const EthIf_ConfigType* ConfigPtr)
{
#if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_INIT, ETHIF_E_PARAM_POINTER);
        return;
    }
#endif
    EthIf_State.state = ETHIF_UNINIT;
    EthIf_State.activeControllerCount = 0U;
    EthIf_State.configPtr = ConfigPtr;
    memset(EthIf_State.controllerModes, 0, sizeof(EthIf_State.controllerModes));

    if (ConfigPtr->NumControllers > ETHIF_MAX_CONTROLLERS) return;
    EthIf_State.activeControllerCount = ConfigPtr->NumControllers;
    for (uint8 i = 0U; i < ConfigPtr->NumControllers; i++) {
        EthIf_State.controllerModes[i] = ETHIF_CS_STOPPED;
    }
    EthIf_State.state = ETHIF_INIT;
}

void EthIf_DeInit(void)
{
    EthIf_State.state = ETHIF_UNINIT;
    EthIf_State.activeControllerCount = 0U;
}

Std_ReturnType EthIf_Transmit(uint8 ControllerId, uint32 BufferHandle, const EthIf_PduType* PduInfoPtr)
{
#if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (EthIf_State.state < ETHIF_INIT) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_TRANSMIT, ETHIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (NULL_PTR == PduInfoPtr) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_TRANSMIT, ETHIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (ControllerId >= EthIf_State.activeControllerCount) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_TRANSMIT, ETHIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
#endif
    (void)BufferHandle;
    if (EthIf_State.controllerModes[ControllerId] == ETHIF_CS_STARTED) {
        return E_OK;
    }
    return E_NOT_OK;
}

Std_ReturnType EthIf_SetControllerMode(uint8 ControllerId, EthIf_ControllerMode Mode)
{
#if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (EthIf_State.state < ETHIF_INIT) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_SET_CONTROLLER_MODE, ETHIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (ControllerId >= EthIf_State.activeControllerCount) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_SET_CONTROLLER_MODE, ETHIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
#endif
    EthIf_State.controllerModes[ControllerId] = Mode;
    return E_OK;
}

EthIf_ControllerMode EthIf_GetControllerMode(uint8 ControllerId)
{
    if (ControllerId >= EthIf_State.activeControllerCount) return ETHIF_CS_STOPPED;
    return EthIf_State.controllerModes[ControllerId];
}

void EthIf_RxIndication(uint8 ControllerId, const EthIf_PduType* PduInfoPtr)
{
    if (NULL_PTR == PduInfoPtr) return;
    if (ControllerId >= EthIf_State.activeControllerCount) return;
    (void)ControllerId;

    /* Apply RX filtering */
    if (EthIf_State.configPtr != NULL_PTR) {
        for (uint8 i = 0U; i < EthIf_State.configPtr->NumRxFilters; i++) {
            const EthIf_RxFilterType* filter = &EthIf_State.configPtr->RxFilters[i];
            if (filter->ControllerId == ControllerId) {
                boolean match = FALSE;
                if (filter->FilterType == ETHIF_FILTER_MAC) {
                    match = (memcmp(filter->MacAddress, PduInfoPtr->MacAddress, 6) == 0U );
                } else if (filter->FilterType == ETHIF_FILTER_ETHERTYPE) {
                    match = (filter->EtherType == PduInfoPtr->EtherType);
                }
                if (match && filter->RxCallback != NULL_PTR) {
                    filter->RxCallback(ControllerId, PduInfoPtr);
                }
            }
        }
    }
}

void EthIf_TxConfirmation(uint8 ControllerId, uint32 BufferHandle)
{
    (void)ControllerId;
    (void)BufferHandle;
}

void EthIf_MainFunction(void)
{
    if (EthIf_State.state < ETHIF_INIT) return;
}

void EthIf_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (NULL_PTR == versioninfo) return;
    versioninfo->vendorID = ETHIF_VENDOR_ID;
    versioninfo->moduleID = ETHIF_MODULE_ID;
    versioninfo->sw_major_version = 1U;
    versioninfo->sw_minor_version = 0U;
    versioninfo->sw_patch_version = 0U;
}