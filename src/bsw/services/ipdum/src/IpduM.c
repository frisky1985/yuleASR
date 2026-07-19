/** @file IpduM.c
 *  @brief I-PDU Multiplexer implementation
 *  @copyright Copyright (c) 2026 YuleTech
 *
 *  @implements AUTOSAR_SWS_IPDUMultiplexer.pdf
 */

#include "IpduM.h"
#include "Det.h"

/* Version check */
#if defined(IPDUM_AR_RELEASE_MAJOR_VERSION) && (IPDUM_AR_RELEASE_MAJOR_VERSION != 4u)
#error "IpduM: AR major mismatch"
#endif
#if defined(IPDUM_AR_RELEASE_MINOR_VERSION) && (IPDUM_AR_RELEASE_MINOR_VERSION != 4u)
#error "IpduM: AR minor mismatch"
#endif

#define IPDUM_SID_INIT              0x00U
#define IPDUM_SID_DEINIT            0x01U
#define IPDUM_SID_SET_IPDU_MODE     0x02U
#define IPDUM_SID_GET_IPDU_MODE     0x03U
#define IPDUM_SID_MAINFUNCTION      0x04U

#define IPDUM_E_PARAM_POINTER       0x10U
#define IPDUM_E_UNINIT              0x20U
#define IPDUM_E_PARAM_IPDU          0x30U
#define IPDUM_E_PARAM_MODE          0x40U

typedef enum { IPDUM_UNINIT = 0, IPDUM_IDLE, IPDUM_BUSY } IpduM_StateType;
typedef struct {
    IpduM_StateType state;
    uint16 activeIpduId;
    const IpduM_ConfigType* configPtr;
} IpduM_InternalType;

static IpduM_InternalType IpduM_State = { IPDUM_UNINIT, 0U, NULL_PTR };

void IpduM_Init(const IpduM_ConfigType* ConfigPtr)
{
#if (IPDUM_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(IPDUM_MODULE_ID, 0U, IPDUM_SID_INIT, IPDUM_E_PARAM_POINTER);
        return;
    }
#endif
    IpduM_State.configPtr = ConfigPtr;
    IpduM_State.state = IPDUM_IDLE;
}

void IpduM_DeInit(void)
{
    IpduM_State.state = IPDUM_UNINIT;
    IpduM_State.configPtr = NULL_PTR;
}

Std_ReturnType IpduM_SetIpduMode(uint16 IpduId, IpduM_IpduModeType Mode)
{
#if (IPDUM_DEV_ERROR_DETECT == STD_ON)
    if (IpduM_State.state == IPDUM_UNINIT) {
        Det_ReportError(IPDUM_MODULE_ID, 0U, IPDUM_SID_SET_IPDU_MODE, IPDUM_E_UNINIT);
        return E_NOT_OK;
    }
#endif
    boolean found = FALSE;
    if (IpduM_State.configPtr != NULL_PTR) {
        for (uint8 i = 0U; i < IpduM_State.configPtr->NumIpduMappings; i++) {
            if (IpduM_State.configPtr->IpduMapping[i].IpduId == IpduId) {
                found = TRUE;
                IpduM_State.activeIpduId = IpduId;
                break;
            }
        }
    }
#if (IPDUM_DEV_ERROR_DETECT == STD_ON)
    if (!found) {
        Det_ReportError(IPDUM_MODULE_ID, 0U, IPDUM_SID_SET_IPDU_MODE, IPDUM_E_PARAM_IPDU);
        return E_NOT_OK;
    }
#endif
    (void)Mode;
    return E_OK;
}

IpduM_IpduModeType IpduM_GetIpduMode(uint16 IpduId)
{
#if (IPDUM_DEV_ERROR_DETECT == STD_ON)
    if (IpduM_State.state == IPDUM_UNINIT) {
        Det_ReportError(IPDUM_MODULE_ID, 0U, IPDUM_SID_GET_IPDU_MODE, IPDUM_E_UNINIT);
        return IPDUM_IPDU_MODE_OFF;
    }
#endif
    (void)IpduId;
    return IPDUM_IPDU_MODE_ON;
}

void IpduM_MainFunction(void)
{
    if (IpduM_State.state == IPDUM_UNINIT) return;

    /* Route PDUs based on active I-PDU mode */
    if (IpduM_State.configPtr != NULL_PTR) {
        for (uint8 i = 0U; i < IpduM_State.configPtr->NumIpduMappings; i++) {
            const IpduM_IpduMappingType* map = &IpduM_State.configPtr->IpduMapping[i];
            if (map->IpduId == IpduM_State.activeIpduId && map->RoutingCallback != NULL_PTR) {
                map->RoutingCallback(map->SourcePduId, map->DestPduId);
            }
        }
    }
}

void IpduM_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (IPDUM_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == versioninfo) {
        Det_ReportError(IPDUM_MODULE_ID, 0U, 0xFFU, IPDUM_E_PARAM_POINTER);
        return;
    }
#endif
    versioninfo->vendorID = IPDUM_VENDOR_ID;
    versioninfo->moduleID = IPDUM_MODULE_ID;
    versioninfo->sw_major_version = 1U;
    versioninfo->sw_minor_version = 0U;
    versioninfo->sw_patch_version = 0U;
}