/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file IpduM.h
 * @brief AUTOSAR I-PDU Multiplexer Main Header
 * @version 4.4.0
 * @date 2026-05-05
 */

#ifndef IPDUM_H
#define IPDUM_H

/*==================================================================================================
 *                                       INCLUDES
 *=================================================================================================*/

#include "ComStack_Types.h"
#include "Std_Types.h"

/*==================================================================================================
 *                                    VERSION INFO
 *=================================================================================================*/

#define IPDUM_VENDOR_ID                     0x01U
#define IPDUM_MODULE_ID                     0x4DU
#define IPDUM_INSTANCE_ID                   0x00U

#define IPDUM_SW_MAJOR_VERSION              4U
#define IPDUM_SW_MINOR_VERSION              4U
#define IPDUM_SW_PATCH_VERSION              0U

/*==================================================================================================
 *                                 API SERVICE IDs
 *=================================================================================================*/

#define IPDUM_SID_INIT                      0x01U
#define IPDUM_SID_DEINIT                    0x02U
#define IPDUM_SID_GETVERSIONINFO            0x03U
#define IPDUM_SID_TRANSMIT                  0x04U
#define IPDUM_SID_RXINDICATION              0x05U
#define IPDUM_SID_TXCONFIRMATION            0x06U
#define IPDUM_SID_TRIGGERTRANSMIT           0x07U
#define IPDUM_SID_MAINFUNCTION              0x08U

/*==================================================================================================
 *                                  ERROR CODES
 *=================================================================================================*/

#define IPDUM_E_NO_ERROR                    0x00U
#define IPDUM_E_PARAM_POINTER               0x01U
#define IPDUM_E_PARAM                       0x02U
#define IPDUM_E_UNINIT                      0x03U
#define IPDUM_E_PARAM_CONFIG                0x04U
#define IPDUM_E_INIT_FAILED                 0x05U

/*==================================================================================================
 *                                  TYPE DEFINITIONS
 *=================================================================================================*/

typedef uint16 IpduM_ShortPduIdType;

typedef enum
{
    IPDUM_STATIC = 0,
    IPDUM_DYNAMIC
} IpduM_PartType;

typedef enum
{
    IPDUM_SELECTOR_BIG_ENDIAN = 0,
    IPDUM_SELECTOR_LITTLE_ENDIAN
} IpduM_SelectorEndiannessType;

typedef struct
{
    uint8 StartByte;
    uint8 StartBit;
    uint8 BitLength;
    IpduM_SelectorEndiannessType Endianness;
} IpduM_SelectorFieldType;

typedef struct
{
    PduIdType TxPduId;
    PduLengthType Length;
    uint8* SduDataPtr;
    uint8 SelectorValue;
} IpduM_DynamicPartType;

typedef struct
{
    PduIdType TxPduId;
    PduLengthType Length;
    uint8* SduDataPtr;
} IpduM_StaticPartType;

typedef struct
{
    PduIdType IpduM_PduId;
    PduIdType LowerLayerPduId;
    IpduM_StaticPartType StaticPart;
    uint8 NumDynamicParts;
    const IpduM_DynamicPartType* DynamicParts;
    IpduM_SelectorFieldType SelectorField;
} IpduM_TxMuxPduType;

typedef struct
{
    PduIdType IpduM_PduId;
    PduIdType LowerLayerPduId;
    uint8 NumStaticParts;
    const IpduM_StaticPartType* StaticParts;
    uint8 NumDynamicParts;
    const IpduM_DynamicPartType* DynamicParts;
    IpduM_SelectorFieldType SelectorField;
} IpduM_RxMuxPduType;

typedef struct
{
    uint16 NumTxMuxPdus;
    const IpduM_TxMuxPduType* TxMuxPdus;
    uint16 NumRxMuxPdus;
    const IpduM_RxMuxPduType* RxMuxPdus;
} IpduM_ConfigType;

/*==================================================================================================
 *                               FUNCTION PROTOTYPES
 *=================================================================================================*/

void IpduM_Init(const IpduM_ConfigType* ConfigPtr);
Std_ReturnType IpduM_DeInit(void);

#if (IPDUM_VERSION_INFO_API == STD_ON)
void IpduM_GetVersionInfo(Std_VersionInfoType* VersionInfo);
#endif

Std_ReturnType IpduM_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);

void IpduM_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
void IpduM_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);
Std_ReturnType IpduM_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr);

void IpduM_MainFunction(void);

/*==================================================================================================
 *                                  EXTERN DECLARATIONS
 *=================================================================================================*/

extern const IpduM_ConfigType IpduM_Config;
extern const IpduM_ConfigType* IpduM_ConfigPtr;

#endif /* IPDUM_H */
