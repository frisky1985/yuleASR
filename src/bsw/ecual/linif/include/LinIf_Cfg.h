/**
 * @file LinIf_Cfg.h
 * @brief LIN Interface configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef LINIF_CFG_H
#define LINIF_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define LINIF_DEV_ERROR_DETECT          (STD_ON)
#define LINIF_VERSION_INFO_API          (STD_ON)
#define LINIF_TRCV_DRIVER_SUPPORTED     (STD_ON)
#define LINIF_WAKEUP_SUPPORT            (STD_ON)
#define LINIF_CANCEL_TRANSMIT_SUPPORTED (STD_ON)

/*==================================================================================================
*                                    NUMBER OF CHANNELS
==================================================================================================*/
#define LINIF_NUM_CHANNELS              (4U)
#define LINIF_NUM_PDUS                  (32U)
#define LINIF_NUM_SCHEDULES             (16U)

/*==================================================================================================
*                                    CHANNEL DEFINITIONS
==================================================================================================*/
#define LINIF_CHANNEL_0                 (0U)
#define LINIF_CHANNEL_1                 (1U)
#define LINIF_CHANNEL_2                 (2U)
#define LINIF_CHANNEL_3                 (3U)

/*==================================================================================================
*                                    LIN CHANNEL MAPPING
==================================================================================================*/
#define LINIF_CH0_LIN_CHANNEL           (0U)
#define LINIF_CH1_LIN_CHANNEL           (1U)
#define LINIF_CH2_LIN_CHANNEL           (2U)
#define LINIF_CH3_LIN_CHANNEL           (3U)

/*==================================================================================================
*                                    SCHEDULE DEFINITIONS
==================================================================================================*/
#define LINIF_SCHEDULE_NULL             (0U)
#define LINIF_SCHEDULE_DIAGNOSTIC       (1U)
#define LINIF_SCHEDULE_NORMAL           (2U)
#define LINIF_SCHEDULE_MASTER_REQ       (3U)
#define LINIF_SCHEDULE_SLAVE_RESP       (4U)

/*==================================================================================================
*                                    PDU DIRECTION
==================================================================================================*/
#define LINIF_PDU_DIRECTION_TX          (0U)
#define LINIF_PDU_DIRECTION_RX          (1U)

/*==================================================================================================
*                                    PDU TYPE
==================================================================================================*/
#define LINIF_PDU_TYPE_UNCONDITIONAL    (0U)
#define LINIF_PDU_TYPE_EVENT_TRIGGERED  (1U)
#define LINIF_PDU_TYPE_SPORADIC         (2U)
#define LINIF_PDU_TYPE_DIAGNOSTIC       (3U)

/*==================================================================================================
*                                    CHECKSUM TYPE
==================================================================================================*/
#define LINIF_CS_CLASSIC                (0U)
#define LINIF_CS_ENHANCED               (1U)

/*==================================================================================================
*                                    SCHEDULE ENTRY TYPE
==================================================================================================*/
#define LINIF_ENTRY_TYPE_UNCONDITIONAL  (0U)
#define LINIF_ENTRY_TYPE_ASSIGN_FRAME   (1U)
#define LINIF_ENTRY_TYPE_ASSIGN_NAD     (2U)
#define LINIF_ENTRY_TYPE_CONDITIONAL    (3U)
#define LINIF_ENTRY_TYPE_FREE           (4U)

/*==================================================================================================
*                                    BAUDRATE
==================================================================================================*/
#define LINIF_BAUDRATE_9600             (9600U)
#define LINIF_BAUDRATE_19200            (19200U)
#define LINIF_BAUDRATE_115200           (115200U)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define LINIF_MAIN_FUNCTION_PERIOD_MS   (5U)

/*==================================================================================================
*                                    SCHEDULE REQUEST QUEUE
==================================================================================================*/
#define LINIF_SCHEDULE_REQUEST_QUEUE_LENGTH (4U)

/*==================================================================================================
*                                    WAKEUP
==================================================================================================*/
#define LINIF_WAKEUP_DELAY_US           (150000U)  /* 150ms */

#endif /* LINIF_CFG_H */
