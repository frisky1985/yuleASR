/**
 * @file SomeIpSd.c
 * @brief SOME/IP Service Discovery Implementation
 */

#include "SomeIpSd.h"
#include "SomeIpSd_Cfg.h"
#include "Det.h"
#include <string.h>

typedef enum {
    SD_STATE_UNINIT = 0,
    SD_STATE_INIT
} SomeIpSd_StateType;

static SomeIpSd_StateType SomeIpSd_State = SD_STATE_UNINIT;
static uint16 SomeIpSd_SessionId = 0x0001U;

void SomeIpSd_Init(const void* ConfigPtr) {
    (void)ConfigPtr;
    SomeIpSd_State = SD_STATE_INIT;
    SomeIpSd_SessionId = 0x0001U;
}

void SomeIpSd_DeInit(void) {
    SomeIpSd_State = SD_STATE_UNINIT;
}

static void SomeIpSd_BuildHeader(uint8* Header, uint8 MsgType) {
    Header[0] = 0xFF; Header[1] = 0xFF; /* Service ID 0xFFFF */
    Header[2] = 0x81; Header[3] = 0x00; /* Method ID 0x8100 */
    Header[4] = 0x00; Header[5] = 0x00; Header[6] = 0x00; Header[7] = 0x08; /* Length */
    Header[8] = 0x00; Header[9] = 0x00; /* Client ID */
    Header[10] = (uint8)(SomeIpSd_SessionId >> 8);
    Header[11] = (uint8)(SomeIpSd_SessionId);
    Header[12] = SOMEIPSD_PROTOCOL_VERSION;
    Header[13] = SOMEIPSD_INTERFACE_VERSION;
    Header[14] = MsgType;
    Header[15] = 0x00; /* Return Code */
}

Std_ReturnType SomeIpSd_FindService(uint16 ServiceId, uint16 InstanceId) {
    uint8 msg[32];
    
    if (SomeIpSd_State != SD_STATE_INIT) {
        return E_NOT_OK;
    }
    
    SomeIpSd_BuildHeader(msg, 0x02);
    
    /* Entry: Find Service */
    msg[16] = 0x00; /* Type = Find Service */
    msg[17] = 0x00; /* Index 1st options */
    msg[18] = 0x00; /* Index 2nd options */
    msg[19] = 0x01; /* # of opt 1 + # of opt 2 */
    msg[20] = (uint8)(ServiceId >> 8);
    msg[21] = (uint8)(ServiceId);
    msg[22] = (uint8)(InstanceId >> 8);
    msg[23] = (uint8)(InstanceId);
    msg[24] = 0x01; /* Major Version */
    msg[25] = 0x00; msg[26] = 0x00; msg[27] = 0x00; /* TTL */
    msg[28] = 0x00; msg[29] = 0x00; msg[30] = 0x00; msg[31] = 0x00; /* Minor */
    
    return E_OK;
}

Std_ReturnType SomeIpSd_OfferService(uint16 ServiceId, uint16 InstanceId) {
    (void)ServiceId;
    (void)InstanceId;
    return E_OK;
}

Std_ReturnType SomeIpSd_StopOffer(uint16 ServiceId, uint16 InstanceId) {
    (void)ServiceId;
    (void)InstanceId;
    return E_OK;
}

Std_ReturnType SomeIpSd_SubscribeEventGroup(uint16 ServiceId, uint16 EventGroupId) {
    (void)ServiceId;
    (void)EventGroupId;
    return E_OK;
}

void SomeIpSd_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
    (void)RxPduId;
    if (PduInfoPtr != NULL_PTR) {
        /* Parse SD message */
    }
}

void SomeIpSd_MainFunction(void) {
    if (SomeIpSd_State != SD_STATE_INIT) {
        return;
    }
    
    /* Process service discovery */
}
