/**
 * @file Dcm_Cfg.h
 * @brief Diagnostic Communication Manager configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef DCM_CFG_H
#define DCM_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define DCM_DEV_ERROR_DETECT            (STD_ON)
#define DCM_VERSION_INFO_API            (STD_ON)
#define DCM_RESPOND_ALL_REQUEST         (STD_ON)
#define DCM_TASK_TIME                   (STD_ON)

/*==================================================================================================
*                                    PROTOCOL CONFIGURATION
==================================================================================================*/
#define DCM_NUM_PROTOCOLS               (2U)
#define DCM_NUM_CONNECTIONS             (4U)
#define DCM_NUM_RX_PDU_IDS              (4U)
#define DCM_NUM_TX_PDU_IDS              (4U)

/*==================================================================================================
*                                    PROTOCOL TYPES
==================================================================================================*/
#define DCM_PROTOCOL_OBD_ON_CAN         (0U)
#define DCM_PROTOCOL_UDS_ON_CAN         (1U)
#define DCM_PROTOCOL_UDS_ON_FLEXRAY     (2U)
#define DCM_PROTOCOL_UDS_ON_IP          (3U)

/*==================================================================================================
*                                    SESSION CONFIGURATION
==================================================================================================*/
#define DCM_NUM_SESSIONS                (4U)

#define DCM_SESSION_DEFAULT             (DCM_DEFAULT_SESSION)
#define DCM_SESSION_PROGRAMMING         (DCM_PROGRAMMING_SESSION)
#define DCM_SESSION_EXTENDED            (DCM_EXTENDED_DIAGNOSTIC_SESSION)
#define DCM_SESSION_SAFETY              (DCM_SAFETY_SYSTEM_DIAGNOSTIC_SESSION)

/*==================================================================================================
*                                    SESSION TIMEOUTS (ms)
==================================================================================================*/
#define DCM_SESSION_DEFAULT_TIMEOUT     (5000U)
#define DCM_SESSION_PROGRAMMING_TIMEOUT (5000U)
#define DCM_SESSION_EXTENDED_TIMEOUT    (5000U)
#define DCM_SESSION_SAFETY_TIMEOUT      (5000U)

/*==================================================================================================
*                                    SECURITY LEVELS
==================================================================================================*/
#define DCM_NUM_SECURITY_LEVELS         (3U)

#define DCM_SEC_LEVEL_LOCKED            (0x00U)
#define DCM_SEC_LEVEL_1                 (0x01U)
#define DCM_SEC_LEVEL_2                 (0x02U)

/*==================================================================================================
*                                    SECURITY ATTEMPTS
==================================================================================================*/
#define DCM_SECURITY_MAX_ATTEMPTS       (3U)
#define DCM_SECURITY_DELAY_TIME_MS      (10000U)

/*==================================================================================================
*                                    SERVICES CONFIGURATION
==================================================================================================*/
#define DCM_NUM_SERVICES                (16U)

/*==================================================================================================
*                                    BUFFER SIZES
==================================================================================================*/
#define DCM_BUFFER_SIZE                 (4095U)
#define DCM_MAX_REQUEST_SIZE            (4095U)
#define DCM_MAX_RESPONSE_SIZE           (4095U)

/*==================================================================================================
*                                    TIMING PARAMETERS (ms)
==================================================================================================*/
#define DCM_P2SERVER_MAX                (50U)
#define DCM_P2STAR_SERVER_MAX           (5000U)
#define DCM_S3SERVER                    (5000U)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define DCM_MAIN_FUNCTION_PERIOD_MS     (10U)

/*==================================================================================================
*                                    OBD SUPPORT
==================================================================================================*/
#define DCM_OBD_SUPPORT                 (STD_ON)
#define DCM_OBD_NUM_PIDS                (32U)

/*==================================================================================================
*                                    ROUTINE CONTROL
==================================================================================================*/
#define DCM_ROUTINE_CONTROL_SUPPORT     (STD_ON)
#define DCM_NUM_ROUTINES                (8U)

/*==================================================================================================
*                                    DATA TRANSFER
==================================================================================================*/
#define DCM_DATA_TRANSFER_SUPPORT       (STD_ON)
#define DCM_TRANSFER_BLOCK_SIZE         (1024U)

#endif /* DCM_CFG_H */
