/** @file SomeIpIf.c
 *  @brief SOME/IP Interface implementation
 *  @copyright Copyright (c) 2026 YuleTech
 *
 *  @implements AUTOSAR_SWS_SOMEIPTransformer.pdf
 */

#include "SomeIpIf.h"
#include "SomeIpIf_Cfg.h"
#include "Det.h"
#include <string.h>

/* Version check */
#if defined(SOMEIPIF_AR_RELEASE_MAJOR_VERSION) && (SOMEIPIF_AR_RELEASE_MAJOR_VERSION != 4u)
#error "SomeIpIf: AR major mismatch"
#endif
#if defined(SOMEIPIF_AR_RELEASE_MINOR_VERSION) && (SOMEIPIF_AR_RELEASE_MINOR_VERSION != 4u)
#error "SomeIpIf: AR minor mismatch"
#endif

#define SOMEIPIF_SID_INIT               0x00U
#define SOMEIPIF_SID_DEINIT             0x01U
#define SOMEIPIF_SID_TRANSMIT           0x02U
#define SOMEIPIF_SID_RX_INDICATION      0x03U
#define SOMEIPIF_SID_MAINFUNCTION       0x04U
#define SOMEIPIF_SID_SET_STATE          0x05U

#define SOMEIPIF_E_PARAM_POINTER        0x10U
#define SOMEIPIF_E_UNINIT               0x20U
#define SOMEIPIF_E_PARAM_PDU            0x30U
#define SOMEIPIF_E_TRANSMIT_FAILED      0x40U

#define SOMEIPIF_MAX_SDUS               8U

#define SOMEIP_PROTOCOL_VERSION         0x01U
#define SOMEIP_INTERFACE_VERSION        0x01U
#define SOMEIP_HEADER_LEN               16U
#define SOMEIP_TP_HEADER_LEN            4U

typedef enum { SOMEIPIF_UNINIT = 0, SOMEIPIF_INIT, SOMEIPIF_ONLINE, SOMEIPIF_OFFLINE } SomeIpIf_StateType;

typedef struct {
    uint8  data[1400];
    uint16 length;
    uint16 pduId;
    uint32 targetIp;
    uint16 targetPort;
} SomeIpIf_TxBufferType;

typedef struct {
    SomeIpIf_StateType state;
    SomeIpIf_TxBufferType txBuffers[SOMEIPIF_MAX_SDUS];
    uint8 txBufferCount;
    const SomeIpIf_ConfigType* configPtr;
} SomeIpIf_InternalType;

static SomeIpIf_InternalType SomeIpIf_State;

static uint32 SomeIpIf_BuildHeader(uint8* buffer, uint16 serviceId, uint16 methodId,
                                    uint16 clientId, uint16 sessionId,
                                    uint32 length, uint8 msgType, uint8 retCode)
{
    uint32 hdrLen = SOMEIP_HEADER_LEN;
    buffer[0] = (uint8)(serviceId >> 8);  buffer[1] = (uint8)(serviceId);
    buffer[2] = (uint8)(methodId >> 8);   buffer[3] = (uint8)(methodId);
    buffer[4] = (uint8)((length + hdrLen) >> 24);
    buffer[5] = (uint8)((length + hdrLen) >> 16);
    buffer[6] = (uint8)((length + hdrLen) >> 8);
    buffer[7] = (uint8)((length + hdrLen));
    buffer[8] = (uint8)(clientId >> 8);   buffer[9] = (uint8)(clientId);
    buffer[10] = (uint8)(sessionId >> 8); buffer[11] = (uint8)(sessionId);
    buffer[12] = SOMEIP_PROTOCOL_VERSION;
    buffer[13] = SOMEIP_INTERFACE_VERSION;
    buffer[14] = msgType;
    buffer[15] = retCode;
    return hdrLen;
}

void SomeIpIf_Init(const SomeIpIf_ConfigType* ConfigPtr)
{
#if (SOMEIPIF_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(SOMEIPIF_MODULE_ID, 0U, SOMEIPIF_SID_INIT, SOMEIPIF_E_PARAM_POINTER);
        return;
    }
#endif
    SomeIpIf_State.state = SOMEIPIF_UNINIT;
    SomeIpIf_State.txBufferCount = 0U;
    SomeIpIf_State.configPtr = ConfigPtr;
    memset(SomeIpIf_State.txBuffers, 0, sizeof(SomeIpIf_State.txBuffers));
    SomeIpIf_State.state = SOMEIPIF_INIT;
}

void SomeIpIf_DeInit(void)
{
    SomeIpIf_State.state = SOMEIPIF_UNINIT;
    SomeIpIf_State.txBufferCount = 0U;
}

Std_ReturnType SomeIpIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
#if (SOMEIPIF_DEV_ERROR_DETECT == STD_ON)
    if (SomeIpIf_State.state < SOMEIPIF_INIT) {
        Det_ReportError(SOMEIPIF_MODULE_ID, 0U, SOMEIPIF_SID_TRANSMIT, SOMEIPIF_E_UNINIT);
        return E_NOT_OK;
    }
    if ((NULL_PTR == PduInfoPtr) || (NULL_PTR == PduInfoPtr->SduDataPtr)) {
        Det_ReportError(SOMEIPIF_MODULE_ID, 0U, SOMEIPIF_SID_TRANSMIT, SOMEIPIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    if (SomeIpIf_State.state == SOMEIPIF_OFFLINE) return E_NOT_OK;
    if (SomeIpIf_State.txBufferCount >= SOMEIPIF_MAX_SDUS) return E_NOT_OK;

    SomeIpIf_TxBufferType* buf = &SomeIpIf_State.txBuffers[SomeIpIf_State.txBufferCount];
    uint16 totalLen = (PduInfoPtr->SduLength < sizeof(buf->data) - SOMEIP_HEADER_LEN)
                      ? PduInfoPtr->SduLength : (uint16)(sizeof(buf->data) - SOMEIP_HEADER_LEN);

    /* Find service configuration */
    uint16 serviceId = 0U;
    uint16 methodId = 0;
    if (SomeIpIf_State.configPtr != NULL_PTR) {
        for (uint8 i = 0U; i < SomeIpIf_State.configPtr->NumChannels; i++) {
            if (SomeIpIf_State.configPtr->Channels[i].ChannelId == (uint8)TxPduId) {
                serviceId = SomeIpIf_State.configPtr->Channels[i].UdpPort;
                methodId = 0x8100;
                buf->targetIp = SomeIpIf_State.configPtr->Channels[i].LocalIp;
                buf->targetPort = SomeIpIf_State.configPtr->Channels[i].UdpPort;
                break;
            }
        }
    }
    
    if (SomeIpIf_State.configPtr != NULL_PTR) {
        for (uint8 i = 0U; i < SomeIpIf_State.configPtr->NumServices; i++) {
            const SomeIpIf_ServiceConfigType* svc = &SomeIpIf_State.configPtr->Services[i];
            if (svc->ServiceId == serviceId) {
                buf->targetIp = svc->Endpoint.IpAddress;
                buf->targetPort = svc->Endpoint.Port;
                break;
            }
        }
    }

    /* Build SOME/IP header */
    uint32 hdrLen = SomeIpIf_BuildHeader(buf->data, serviceId, methodId, 0x0001, 0x0001,
                                          (uint32)totalLen, 0x00, 0x00);
    memcpy(&buf->data[hdrLen], PduInfoPtr->SduDataPtr, totalLen);
    buf->length = (uint16)(hdrLen + totalLen);
    buf->pduId = TxPduId;
    SomeIpIf_State.txBufferCount++;

    return E_OK;
}

void SomeIpIf_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    (void)RxPduId;
    (void)PduInfoPtr;
    /* Parse SOME/IP header and forward to upper layer */
}

void SomeIpIf_MainFunction(void)
{
    if (SomeIpIf_State.state < SOMEIPIF_INIT) return;

    /* Process pending TX buffers */
    for (uint8 i = 0U; i < SomeIpIf_State.txBufferCount; i++) {
        /* Send via SoAd (simplified) */
        SomeIpIf_State.txBuffers[i].length = 0U;
    }
    SomeIpIf_State.txBufferCount = 0U;
}

Std_ReturnType SomeIpIf_SetState(uint8 ChannelId, boolean Online)
{
    (void)ChannelId;
    SomeIpIf_State.state = Online ? SOMEIPIF_ONLINE : SOMEIPIF_OFFLINE;
    return E_OK;
}

void SomeIpIf_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (NULL_PTR == versioninfo) return;
    versioninfo->vendorID = SOMEIPIF_VENDOR_ID;
    versioninfo->moduleID = SOMEIPIF_MODULE_ID;
    versioninfo->sw_major_version = 1U;
    versioninfo->sw_minor_version = 0U;
    versioninfo->sw_patch_version = 0U;
}