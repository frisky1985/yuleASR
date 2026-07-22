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
 * @file IpduM_Lcfg.c
 * @brief AUTOSAR I-PDU Multiplexer Link-Time Configuration
 * @version 4.4.0
 * @date 2026-05-05
 */

#include "IpduM.h"
#include "IpduM_Cfg.h"

/*==================================================================================================
 *                                    MEMORY SECTIONS
 *=================================================================================================*/

#define IPDUM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
 *                              TX MULTIPLEXED PDU CONFIGURATIONS
 *=================================================================================================*/

/* Static data for Tx PDU 0 */
static uint8 IpduM_TxMuxPdu0_StaticData[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/* Dynamic parts for Tx PDU 0 */
static uint8 IpduM_TxMuxPdu0_Dyn0_Data[8] = {0x00};
static uint8 IpduM_TxMuxPdu0_Dyn1_Data[8] = {0x00};
static uint8 IpduM_TxMuxPdu0_Dyn2_Data[8] = {0x00};
static uint8 IpduM_TxMuxPdu0_Dyn3_Data[8] = {0x00};

static const IpduM_DynamicPartType IpduM_TxMuxPdu0_DynamicParts[4] =
{
    {
        /* TxPduId */ IPDUM_COM_TX_PDU_0,
        /* Length */ 8U,
        /* SduDataPtr */ IpduM_TxMuxPdu0_Dyn0_Data,
        /* SelectorValue */ IPDUM_SELECTOR_VALUE_0
    },
    {
        /* TxPduId */ IPDUM_COM_TX_PDU_1,
        /* Length */ 8U,
        /* SduDataPtr */ IpduM_TxMuxPdu0_Dyn1_Data,
        /* SelectorValue */ IPDUM_SELECTOR_VALUE_1
    },
    {
        /* TxPduId */ IPDUM_COM_TX_PDU_2,
        /* Length */ 8U,
        /* SduDataPtr */ IpduM_TxMuxPdu0_Dyn2_Data,
        /* SelectorValue */ IPDUM_SELECTOR_VALUE_2
    },
    {
        /* TxPduId */ IPDUM_COM_TX_PDU_3,
        /* Length */ 8U,
        /* SduDataPtr */ IpduM_TxMuxPdu0_Dyn3_Data,
        /* SelectorValue */ IPDUM_SELECTOR_VALUE_3
    }
};

static const IpduM_TxMuxPduType IpduM_TxMuxPdu_0 =
{
    /* IpduM_PduId */ IPDUM_TX_MUX_PDU_0,
    /* LowerLayerPduId */ 0U,  /* PduR Tx PDU */
    
    /* StaticPart */
    {
        /* TxPduId */ 0U,
        /* Length */ 8U,
        /* SduDataPtr */ IpduM_TxMuxPdu0_StaticData
    },
    
    /* NumDynamicParts */ 4U,
    /* DynamicParts */ IpduM_TxMuxPdu0_DynamicParts,
    
    /* SelectorField */
    {
        /* StartByte */ 0U,
        /* StartBit */ 0U,
        /* BitLength */ 8U,
        /* Endianness */ IPDUM_SELECTOR_BIG_ENDIAN
    }
};

/* Tx PDU 1 - Longer PDU with 16-bit selector */
static uint8 IpduM_TxMuxPdu1_StaticData[16] = {0};

static uint8 IpduM_TxMuxPdu1_Dyn0_Data[16] = {0};
static uint8 IpduM_TxMuxPdu1_Dyn1_Data[16] = {0};

static const IpduM_DynamicPartType IpduM_TxMuxPdu1_DynamicParts[2] =
{
    {
        IPDUM_COM_TX_PDU_0,
        16U,
        IpduM_TxMuxPdu1_Dyn0_Data,
        0x00U
    },
    {
        IPDUM_COM_TX_PDU_1,
        16U,
        IpduM_TxMuxPdu1_Dyn1_Data,
        0x01U
    }
};

static const IpduM_TxMuxPduType IpduM_TxMuxPdu_1 =
{
    IPDUM_TX_MUX_PDU_1,
    1U,  /* PduR Tx PDU */
    
    {
        0U,
        16U,
        IpduM_TxMuxPdu1_StaticData
    },
    
    2U,
    IpduM_TxMuxPdu1_DynamicParts,
    
    {
        0U,
        0U,
        8U,  /* 8-bit selector */
        IPDUM_SELECTOR_BIG_ENDIAN
    }
};

/* Array of all Tx Mux PDUs */
static const IpduM_TxMuxPduType* const IpduM_TxMuxPdus[IPDUM_MAX_TX_MUX_PDUS] =
{
    &IpduM_TxMuxPdu_0,
    &IpduM_TxMuxPdu_1,
    NULL_PTR,  /* Reserved */
    NULL_PTR   /* Reserved */
};

/*==================================================================================================
 *                              RX MULTIPLEXED PDU CONFIGURATIONS
 *=================================================================================================*/

/* Static parts for Rx PDU 0 */
static uint8 IpduM_RxMuxPdu0_StaticData[8] = {0};

static const IpduM_StaticPartType IpduM_RxMuxPdu0_StaticParts[1] =
{
    {
        /* TxPduId - Upper layer PDU ID */
        IPDUM_COM_RX_PDU_0,
        /* Length */
        8U,
        /* SduDataPtr */
        IpduM_RxMuxPdu0_StaticData
    }
};

/* Dynamic parts for Rx PDU 0 */
static uint8 IpduM_RxMuxPdu0_Dyn0_Data[8] = {0};
static uint8 IpduM_RxMuxPdu0_Dyn1_Data[8] = {0};

static const IpduM_DynamicPartType IpduM_RxMuxPdu0_DynamicParts[2] =
{
    {
        IPDUM_COM_RX_PDU_0,
        8U,
        IpduM_RxMuxPdu0_Dyn0_Data,
        IPDUM_SELECTOR_VALUE_0
    },
    {
        IPDUM_COM_RX_PDU_1,
        8U,
        IpduM_RxMuxPdu0_Dyn1_Data,
        IPDUM_SELECTOR_VALUE_1
    }
};

static const IpduM_RxMuxPduType IpduM_RxMuxPdu_0 =
{
    /* IpduM_PduId */ IPDUM_RX_MUX_PDU_0,
    /* LowerLayerPduId */ 0U,  /* PduR Rx PDU */
    
    /* NumStaticParts */ 1U,
    /* StaticParts */ IpduM_RxMuxPdu0_StaticParts,
    
    /* NumDynamicParts */ 2U,
    /* DynamicParts */ IpduM_RxMuxPdu0_DynamicParts,
    
    /* SelectorField */
    {
        0U,
        0U,
        8U,
        IPDUM_SELECTOR_BIG_ENDIAN
    }
};

/* Rx PDU 1 */
static uint8 IpduM_RxMuxPdu1_StaticData[16] = {0};

static const IpduM_StaticPartType IpduM_RxMuxPdu1_StaticParts[1] =
{
    {
        IPDUM_COM_RX_PDU_2,
        16U,
        IpduM_RxMuxPdu1_StaticData
    }
};

static uint8 IpduM_RxMuxPdu1_Dyn0_Data[16] = {0};

static const IpduM_DynamicPartType IpduM_RxMuxPdu1_DynamicParts[1] =
{
    {
        IPDUM_COM_RX_PDU_3,
        16U,
        IpduM_RxMuxPdu1_Dyn0_Data,
        0x00U
    }
};

static const IpduM_RxMuxPduType IpduM_RxMuxPdu_1 =
{
    IPDUM_RX_MUX_PDU_1,
    1U,  /* PduR Rx PDU */
    
    1U,
    IpduM_RxMuxPdu1_StaticParts,
    
    1U,
    IpduM_RxMuxPdu1_DynamicParts,
    
    {
        0U,
        0U,
        8U,
        IPDUM_SELECTOR_BIG_ENDIAN
    }
};

/* Array of all Rx Mux PDUs */
static const IpduM_RxMuxPduType* const IpduM_RxMuxPdus[IPDUM_MAX_RX_MUX_PDUS] =
{
    &IpduM_RxMuxPdu_0,
    &IpduM_RxMuxPdu_1,
    NULL_PTR,
    NULL_PTR
};

/*==================================================================================================
 *                              MAIN CONFIGURATION
 *=================================================================================================*/

const IpduM_ConfigType IpduM_Config =
{
    /* NumTxMuxPdus */ 2U,
    /* TxMuxPdus */ (const IpduM_TxMuxPduType*)IpduM_TxMuxPdu_0,  /* Pointer to first element */
    
    /* NumRxMuxPdus */ 2U,
    /* RxMuxPdus */ (const IpduM_RxMuxPduType*)IpduM_RxMuxPdu_0
};

/* Pointer to active configuration */
const IpduM_ConfigType* IpduM_ConfigPtr = &IpduM_Config;

#define IPDUM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"
extern const IpduM_ConfigType* IpduM_ConfigPtr;
extern const IpduM_ConfigType IpduM_Config;

/*==================================================================================================
 *                                       END OF FILE
 *=================================================================================================*/
