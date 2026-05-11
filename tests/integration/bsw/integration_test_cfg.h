/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Integration Test Configuration
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-24
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#ifndef INTEGRATION_TEST_CFG_H
#define INTEGRATION_TEST_CFG_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                  TEST-SPECIFIC CONFIGURATION
==================================================================================================*/

/* Override COM configuration for smaller test footprint */
#define IT_COM_NUM_SIGNALS              (8U)
#define IT_COM_NUM_IPDUS                (8U)
#define IT_COM_NUM_IPDU_GROUPS          (2U)
#define IT_COM_MAX_IPDU_BUFFER_SIZE     (64U)

/* Override NvM configuration for testing */
#define IT_NVM_NUM_OF_NVRAM_BLOCKS      (8U)
#define IT_NVM_SIZE_STANDARD_JOB_QUEUE  (4U)
#define IT_NVM_SIZE_IMMEDIATE_JOB_QUEUE (2U)

/* Override CanIf configuration */
#define IT_CANIF_NUM_CONTROLLERS        (1U)
#define IT_CANIF_NUM_TX_PDUS            (8U)
#define IT_CANIF_NUM_RX_PDUS            (8U)
#define IT_CANIF_NUM_HRH                (2U)
#define IT_CANIF_NUM_HTH                (2U)

/*==================================================================================================
*                                    COM SIGNAL DEFINITIONS
==================================================================================================*/
#define IT_COM_SIGNAL_ENGINE_SPEED      ((Com_SignalIdType)0U)
#define IT_COM_SIGNAL_VEHICLE_SPEED     ((Com_SignalIdType)1U)
#define IT_COM_SIGNAL_THROTTLE_POSITION ((Com_SignalIdType)2U)
#define IT_COM_SIGNAL_ENGINE_TEMP       ((Com_SignalIdType)3U)

/*==================================================================================================
*                                    COM IPDU DEFINITIONS
==================================================================================================*/
#define IT_COM_IPDU_ENGINE_STATUS       ((PduIdType)0U)
#define IT_COM_IPDU_VEHICLE_STATUS      ((PduIdType)1U)

/*==================================================================================================
*                                    NVM BLOCK DEFINITIONS
==================================================================================================*/
#define IT_NVM_BLOCK_ID_ENGINE_CONFIG   ((NvM_BlockIdType)1U)
#define IT_NVM_BLOCK_ID_USER_SETTINGS   ((NvM_BlockIdType)2U)

/*==================================================================================================
*                                    CAN MOCK DEFINITIONS
==================================================================================================*/
#define IT_CAN_MOCK_MAX_MESSAGES        (16U)
#define IT_CAN_MOCK_MESSAGE_SIZE        (8U)

/*==================================================================================================
*                                    FEE MOCK DEFINITIONS
==================================================================================================*/
#define IT_FEE_MOCK_MAX_BLOCKS          (8U)
#define IT_FEE_MOCK_BLOCK_SIZE          (256U)

/*==================================================================================================
*                                    OS ALARM DEFINITIONS
==================================================================================================*/
#define IT_OS_ALARM_BSWM_10MS           ((AlarmType)0U)
#define IT_OS_ALARM_COM_10MS            ((AlarmType)1U)
#define IT_OS_ALARM_NVM_100MS           ((AlarmType)2U)
#define IT_OS_ALARM_DEM_100MS           ((AlarmType)3U)

/*==================================================================================================
*                                    DCM TEST DEFINITIONS
==================================================================================================*/
#define IT_DCM_PROTOCOL_ID              (0U)
#define IT_DCM_DIAG_REQUEST_PDUID       ((PduIdType)6U)
#define IT_DCM_DIAG_RESPONSE_PDUID      ((PduIdType)5U)

/*==================================================================================================
*                                  ENGINE CONFIG DATA TYPE
==================================================================================================*/
typedef struct
{
    uint16 maxRpm;
    uint16 idleSpeed;
    uint16 redlineRpm;
    uint8  cylinderCount;
} It_EngineConfigType;

#endif /* INTEGRATION_TEST_CFG_H */
