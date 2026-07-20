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
 * @file IpduM.c
 * @brief AUTOSAR I-PDU Multiplexer Implementation
 * @version 4.4.0
 * @date 2026-05-05
 */

#include "IpduM.h"
#include "IpduM_Cfg.h"
#include "PduR.h"
#include "Det.h"
#include "Com.h"

#if (IPDUM_DEV_ERROR_DETECT == STD_ON)
#define IPDUM_DET_REPORT_ERROR(ApiId, ErrorId) \
    Det_ReportError(IPDUM_MODULE_ID, IPDUM_INSTANCE_ID, (ApiId), (ErrorId))
#else
#define IPDUM_DET_REPORT_ERROR(ApiId, ErrorId) ((void)0)
#endif

/* Module initialization state */
static boolean IpduM_InitStatus = FALSE;

/*==================================================================================================
 *                                      LOCAL FUNCTIONS
 *=================================================================================================*/

/**
 * @brief Extract selector value from PDU data
 */
static uint8 IpduM_ExtractSelector(const uint8* DataPtr, const IpduM_SelectorFieldType* Selector)
{
    uint8 SelectorValue = 0U;
    uint8 ByteIndex = Selector->StartByte;
    uint8 BitOffset = Selector->StartBit;
    uint8 BitLength = Selector->BitLength;
    
    if (Selector->Endianness == IPDUM_SELECTOR_BIG_ENDIAN)
    {
        /* Big endian: MSB first */
        uint8 i;
        for (i = 0; i < BitLength; i++)
        {
            uint8 BitPos = (7U - ((BitOffset + i) % 8U));
            uint8 BytePos = ByteIndex + ((BitOffset + i) / 8U);
            
            if (DataPtr[BytePos] & (1U << BitPos))
            {
                SelectorValue |= (1U << (BitLength - 1U - i));
            }
        }
    }
    else
    {
        /* Little endian: LSB first */
        uint8 i;
        for (i = 0; i < BitLength; i++)
        {
            uint8 BitPos = ((BitOffset + i) % 8U);
            uint8 BytePos = ByteIndex + ((BitOffset + i) / 8U);
            
            if (DataPtr[BytePos] & (1U << BitPos))
            {
                SelectorValue |= (1U << i);
            }
        }
    }
    
    return SelectorValue;
}

/**
 * @brief Insert selector value into PDU data
 */
static void IpduM_InsertSelector(uint8* DataPtr, const IpduM_SelectorFieldType* Selector, uint8 Value)
{
    uint8 ByteIndex = Selector->StartByte;
    uint8 BitOffset = Selector->StartBit;
    uint8 BitLength = Selector->BitLength;
    
    if (Selector->Endianness == IPDUM_SELECTOR_BIG_ENDIAN)
    {
        /* Big endian: MSB first */
        uint8 i;
        for (i = 0; i < BitLength; i++)
        {
            uint8 BitPos = (7U - ((BitOffset + i) % 8U));
            uint8 BytePos = ByteIndex + ((BitOffset + i) / 8U);
            
            if (Value & (1U << (BitLength - 1U - i)))
            {
                DataPtr[BytePos] |= (1U << BitPos);
            }
            else
            {
                DataPtr[BytePos] &= ~(1U << BitPos);
            }
        }
    }
    else
    {
        /* Little endian: LSB first */
        uint8 i;
        for (i = 0; i < BitLength; i++)
        {
            uint8 BitPos = ((BitOffset + i) % 8U);
            uint8 BytePos = ByteIndex + ((BitOffset + i) / 8U);
            
            if (Value & (1U << i))
            {
                DataPtr[BytePos] |= (1U << BitPos);
            }
            else
            {
                DataPtr[BytePos] &= ~(1U << BitPos);
            }
        }
    }
}

/**
 * @brief Find Tx Mux PDU configuration by upper layer PDU ID
 */
static const IpduM_TxMuxPduType* IpduM_FindTxMuxPdu(PduIdType TxPduId)
{
    uint16 i;
    const IpduM_ConfigType* Config = IpduM_ConfigPtr;
    
    for (i = 0; i < Config->NumTxMuxPdus; i++)
    {
        if (Config->TxMuxPdus[i].IpduM_PduId == TxPduId)
        {
            return &Config->TxMuxPdus[i];
        }
    }
    return NULL_PTR;
}

/**
 * @brief Find Rx Mux PDU configuration by lower layer PDU ID
 */
static const IpduM_RxMuxPduType* IpduM_FindRxMuxPdu(PduIdType RxPduId)
{
    uint16 i;
    const IpduM_ConfigType* Config = IpduM_ConfigPtr;
    
    for (i = 0; i < Config->NumRxMuxPdus; i++)
    {
        if (Config->RxMuxPdus[i].LowerLayerPduId == RxPduId)
        {
            return &Config->RxMuxPdus[i];
        }
    }
    return NULL_PTR;
}

/**
 * @brief Find dynamic part by selector value
 */
static const IpduM_DynamicPartType* IpduM_FindDynamicPart(
    const IpduM_DynamicPartType* DynamicParts,
    uint8 NumDynamicParts,
    uint8 SelectorValue)
{
    uint8 i;
    for (i = 0; i < NumDynamicParts; i++)
    {
        if (DynamicParts[i].SelectorValue == SelectorValue)
        {
            return &DynamicParts[i];
        }
    }
    return NULL_PTR;
}

/*==================================================================================================
 *                                       API FUNCTIONS
 *=================================================================================================*/

/**
 * @brief Initialize I-PDU Multiplexer
 */
void IpduM_Init(const IpduM_ConfigType* ConfigPtr)
{
    if (ConfigPtr == NULL_PTR)
    {
        IPDUM_DET_REPORT_ERROR(IPDUM_SID_INIT, IPDUM_E_PARAM_POINTER);
        return;
    }
    
    IpduM_ConfigPtr = ConfigPtr;
    IpduM_InitStatus = TRUE;
}

/**
 * @brief De-initialize I-PDU Multiplexer
 */
Std_ReturnType IpduM_DeInit(void)
{
    if (IpduM_InitStatus == FALSE)
    {
        IPDUM_DET_REPORT_ERROR(IPDUM_SID_DEINIT, IPDUM_E_UNINIT);
        return E_NOT_OK;
    }
    
    IpduM_InitStatus = FALSE;
    return E_OK;
}

/**
 * @brief Get version information
 */
#if (IPDUM_VERSION_INFO_API == STD_ON)
void IpduM_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
    if (VersionInfo == NULL_PTR)
    {
        IPDUM_DET_REPORT_ERROR(IPDUM_SID_GETVERSIONINFO, IPDUM_E_PARAM_POINTER);
        return;
    }
    
    VersionInfo->vendorID = IPDUM_VENDOR_ID;
    VersionInfo->moduleID = IPDUM_MODULE_ID;
    VersionInfo->sw_major_version = IPDUM_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = IPDUM_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = IPDUM_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Transmit multiplexed PDU
 */
Std_ReturnType IpduM_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    Std_ReturnType Result = E_NOT_OK;
    const IpduM_TxMuxPduType* TxMuxPdu;
    const IpduM_DynamicPartType* DynamicPart;
    uint8 SelectorValue;
    uint8 MuxPduData[IPDUM_MAX_PDU_LENGTH];
    PduInfoType MuxPduInfo;
    
    if (IpduM_InitStatus == FALSE)
    {
        IPDUM_DET_REPORT_ERROR(IPDUM_SID_TRANSMIT, IPDUM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (PduInfoPtr == NULL_PTR)
    {
        IPDUM_DET_REPORT_ERROR(IPDUM_SID_TRANSMIT, IPDUM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    /* Find Tx Mux PDU configuration */
    TxMuxPdu = IpduM_FindTxMuxPdu(TxPduId);
    if (TxMuxPdu == NULL_PTR)
    {
        IPDUM_DET_REPORT_ERROR(IPDUM_SID_TRANSMIT, IPDUM_E_PARAM);
        return E_NOT_OK;
    }
    
    /* Extract selector value from PDU data */
    SelectorValue = IpduM_ExtractSelector(PduInfoPtr->SduDataPtr, &TxMuxPdu->SelectorField);
    
    /* Find matching dynamic part */
    DynamicPart = IpduM_FindDynamicPart(TxMuxPdu->DynamicParts, 
                                         TxMuxPdu->NumDynamicParts, 
                                         SelectorValue);
    if (DynamicPart == NULL_PTR)
    {
        return E_NOT_OK;
    }
    
    /* Prepare multiplexed PDU:
     * 1. Copy static part (if configured)
     * 2. Copy dynamic part data
     * 3. Insert selector value
     */
    if (TxMuxPdu->StaticPart.Length > 0U )
    {
        /* Copy static part */
        uint8 i;
        for (i = 0; i < TxMuxPdu->StaticPart.Length; i++)
        {
            MuxPduData[i] = TxMuxPdu->StaticPart.SduDataPtr[i];
        }
    }
    
    /* Copy dynamic part data */
    {
        uint8 i;
        for (i = 0; i < DynamicPart->Length; i++)
        {
            MuxPduData[i] = PduInfoPtr->SduDataPtr[i];
        }
    }
    
    /* Insert selector value */
    IpduM_InsertSelector(MuxPduData, &TxMuxPdu->SelectorField, SelectorValue);
    
    /* Send to lower layer (PduR) */
    MuxPduInfo.SduDataPtr = MuxPduData;
    MuxPduInfo.SduLength = DynamicPart->Length;
    
    Result = PduR_IpduMTransmit(TxMuxPdu->LowerLayerPduId, &MuxPduInfo);
    
    return Result;
}

/**
 * @brief Receive indication from lower layer
 */
void IpduM_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    const IpduM_RxMuxPduType* RxMuxPdu;
    const IpduM_StaticPartType* StaticPart;
    const IpduM_DynamicPartType* DynamicPart;
    uint8 SelectorValue;
    uint8 DemuxPduData[IPDUM_MAX_PDU_LENGTH];
    PduInfoType DemuxPduInfo;
    
    if (IpduM_InitStatus == FALSE)
    {
        IPDUM_DET_REPORT_ERROR(IPDUM_SID_RXINDICATION, IPDUM_E_UNINIT);
        return;
    }
    
    if (PduInfoPtr == NULL_PTR)
    {
        IPDUM_DET_REPORT_ERROR(IPDUM_SID_RXINDICATION, IPDUM_E_PARAM_POINTER);
        return;
    }
    
    /* Find Rx Mux PDU configuration */
    RxMuxPdu = IpduM_FindRxMuxPdu(RxPduId);
    if (RxMuxPdu == NULL_PTR)
    {
        return;
    }
    
    /* Extract selector value */
    SelectorValue = IpduM_ExtractSelector(PduInfoPtr->SduDataPtr, &RxMuxPdu->SelectorField);
    
    /* Find matching dynamic part */
    DynamicPart = IpduM_FindDynamicPart(RxMuxPdu->DynamicParts,
                                         RxMuxPdu->NumDynamicParts,
                                         SelectorValue);
    if (DynamicPart == NULL_PTR)
    {
        return;
    }
    
    /* Demultiplex PDU:
     * Extract dynamic part data and forward to COM
     */
    {
        uint8 i;
        for (i = 0; i < DynamicPart->Length; i++)
        {
            DemuxPduData[i] = PduInfoPtr->SduDataPtr[i];
        }
    }
    
    /* Forward to upper layer (COM) */
    DemuxPduInfo.SduDataPtr = DemuxPduData;
    DemuxPduInfo.SduLength = DynamicPart->Length;
    
    PduR_IpduMRxIndication(DynamicPart->TxPduId, &DemuxPduInfo);
}

/**
 * @brief Tx confirmation from lower layer
 */
void IpduM_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    const IpduM_TxMuxPduType* TxMuxPdu;
    
    if (IpduM_InitStatus == FALSE)
    {
        IPDUM_DET_REPORT_ERROR(IPDUM_SID_TXCONFIRMATION, IPDUM_E_UNINIT);
        return;
    }
    
    /* Find Tx Mux PDU configuration */
    TxMuxPdu = IpduM_FindTxMuxPdu(TxPduId);
    if (TxMuxPdu == NULL_PTR)
    {
        return;
    }
    
    /* Forward confirmation to upper layer (COM) */
    PduR_IpduMTxConfirmation(TxMuxPdu->IpduM_PduId, result);
}

/**
 * @brief Trigger transmit from lower layer
 */
Std_ReturnType IpduM_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)
{
    if (IpduM_InitStatus == FALSE)
    {
        IPDUM_DET_REPORT_ERROR(IPDUM_SID_TRIGGERTRANSMIT, IPDUM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (PduInfoPtr == NULL_PTR)
    {
        IPDUM_DET_REPORT_ERROR(IPDUM_SID_TRIGGERTRANSMIT, IPDUM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    /* Trigger transmit logic - forward to COM */
    return Com_TriggerTransmit(TxPduId, PduInfoPtr);
}

/**
 * @brief Main function - cyclic processing
 */
void IpduM_MainFunction(void)
{
    /* IpduM has no cyclic processing needs */
    /* This function is provided for API completeness */
}
