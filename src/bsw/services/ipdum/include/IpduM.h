/**
 * @file IpduM.h
 * @brief I-PDU Multiplexer - AUTOSAR Services Module
 * @version 1.0.0
 * @date 2026-07-19
 */

#ifndef IPDUM_H
#define IPDUM_H

#include "Std_Types.h"
#include "ComStack_Types.h"
#include "IpduM_Cfg.h"

#define IPDUM_MODULE_ID             0x38U
#define IPDUM_VENDOR_ID             0x0055U
#define IPDUM_MAX_STATIC_PARTS      8U

typedef enum {
    IPDUM_IPDU_MODE_OFF = 0,
    IPDUM_IPDU_MODE_ON,
    IPDUM_IPDU_MODE_ALTERNATE
} IpduM_IpduModeType;

typedef struct {
    uint16 SourcePduId;
    uint16 DestPduId;
    uint8  SelectorPosition;
} IpduM_StaticPartType;

typedef struct {
    uint16 IpduId;
    PduIdType SourcePduId;
    PduIdType DestPduId;
    void (*RoutingCallback)(PduIdType SourceId, PduIdType DestId);
} IpduM_IpduMappingType;

typedef struct {
    uint16 NumStaticParts;
    const IpduM_StaticPartType* StaticParts;
    uint16 NumIpduMappings;
    const IpduM_IpduMappingType* IpduMapping;
} IpduM_ConfigType;

void IpduM_Init(const IpduM_ConfigType* ConfigPtr);
void IpduM_DeInit(void);
Std_ReturnType IpduM_SetIpduMode(uint16 IpduId, IpduM_IpduModeType Mode);
IpduM_IpduModeType IpduM_GetIpduMode(uint16 IpduId);
void IpduM_MainFunction(void);
void IpduM_GetVersionInfo(Std_VersionInfoType* versioninfo);

#endif /* IPDUM_H */