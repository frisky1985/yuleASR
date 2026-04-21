/**
 * @file Dem_Cfg.h
 * @brief Diagnostic Event Manager configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef DEM_CFG_H
#define DEM_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define DEM_DEV_ERROR_DETECT            (STD_ON)
#define DEM_VERSION_INFO_API            (STD_ON)
#define DEM_CLEAR_DTC_SUPPORTED         (STD_ON)
#define DEM_CLEAR_DTC_LIMITATION        (STD_OFF)
#define DEM_DTC_STATUS_AVAILABILITY_MASK (0xFFU)
#define DEM_OBD_RELEVANT_SUPPORT        (STD_ON)
#define DEM_J1939_SUPPORT               (STD_OFF)

/*==================================================================================================
*                                    EVENT CONFIGURATION
==================================================================================================*/
#define DEM_NUM_EVENTS                  (128U)
#define DEM_NUM_DTCS                    (64U)

/*==================================================================================================
*                                    EVENT IDs
==================================================================================================*/
#define DEM_EVENT_ID_MIN                ((Dem_EventIdType)1U)
#define DEM_EVENT_ID_MAX                ((Dem_EventIdType)127U)

/*==================================================================================================
*                                    DTC DEFINITIONS
==================================================================================================*/
#define DEM_DTC_INVALID                 ((Dem_DtcType)0x000000U)
#define DEM_DTC_MIN                     ((Dem_DtcType)0x000001U)
#define DEM_DTC_MAX                     ((Dem_DtcType)0xFFFFFFU)

/*==================================================================================================
*                                    DTC PRIORITY
==================================================================================================*/
#define DEM_DTC_PRIORITY_MIN            (1U)
#define DEM_DTC_PRIORITY_MAX            (255U)

/*==================================================================================================
*                                    FREEZE FRAME CONFIGURATION
==================================================================================================*/
#define DEM_NUM_FREEZE_FRAME_RECORDS    (8U)
#define DEM_FREEZE_FRAME_MAX_DIDS       (16U)
#define DEM_FREEZE_FRAME_MAX_SIZE       (256U)

/*==================================================================================================
*                                    EXTENDED DATA CONFIGURATION
==================================================================================================*/
#define DEM_NUM_EXTENDED_DATA_RECORDS   (4U)
#define DEM_EXTENDED_DATA_MAX_SIZE      (128U)

/*==================================================================================================
*                                    INDICATOR CONFIGURATION
==================================================================================================*/
#define DEM_NUM_INDICATORS              (8U)

#define DEM_INDICATOR_MIL               (0U)  /* Malfunction Indicator Lamp */
#define DEM_INDICATOR_SVS               (1U)  /* Service Vehicle Soon */
#define DEM_INDICATOR_AWLS              (2U)  /* Automatic Transmission Warning Lamp */
#define DEM_INDICATOR_PL                (3U)  /* Powertrain Lamp */
#define DEM_INDICATOR_SBL               (4U)  /* Service Brake Lamp */
#define DEM_INDICATOR_AWLS2             (5U)  /* Automatic Transmission Warning Lamp 2 */
#define DEM_INDICATOR_RSL               (6U)  /* Restraint System Lamp */
#define DEM_INDICATOR_ESL               (7U)  /* Electronic Stability Lamp */

/*==================================================================================================
*                                    OPERATION CYCLE CONFIGURATION
==================================================================================================*/
#define DEM_NUM_OPERATION_CYCLES        (5U)

#define DEM_OPCYC_POWER                 (0U)
#define DEM_OPCYC_IGNITION              (1U)
#define DEM_OPCYC_WARMUP                (2U)
#define DEM_OPCYC_OBD_DCY               (3U)
#define DEM_OPCYC_OTHER                 (4U)

/*==================================================================================================
*                                    AGING CYCLE CONFIGURATION
==================================================================================================*/
#define DEM_AGING_CYCLE_THRESHOLD       (40U)
#define DEM_AGING_CYCLE_COUNTER_MAX     (255U)
#define DEM_AGING_THRESHOLD             DEM_AGING_CYCLE_THRESHOLD
#define DEM_MAX_OCCURRENCE_COUNTER      (255U)
#define DEM_DTC_GROUP_ALL               ((Dem_DtcType)0xFFFFFFU)

/*==================================================================================================
*                                    DEBOUNCE CONFIGURATION
==================================================================================================*/
#define DEM_DEBOUNCE_COUNTER_BASED      (STD_ON)
#define DEM_DEBOUNCE_TIME_BASED         (STD_ON)
#define DEM_DEBOUNCE_MONITOR_INTERNAL   (STD_ON)

/*==================================================================================================
*                                    COUNTER BASED DEBOUNCE
==================================================================================================*/
#define DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD   (127)
#define DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD   (-128)
#define DEM_DEBOUNCE_COUNTER_INCREMENT          (1)
#define DEM_DEBOUNCE_COUNTER_DECREMENT          (1)
#define DEM_DEBOUNCE_COUNTER_INCREMENT_STEP     DEM_DEBOUNCE_COUNTER_INCREMENT
#define DEM_DEBOUNCE_COUNTER_DECREMENT_STEP     DEM_DEBOUNCE_COUNTER_DECREMENT

/*==================================================================================================
*                                    TIME BASED DEBOUNCE
==================================================================================================*/
#define DEM_DEBOUNCE_TIME_FAILED_THRESHOLD_MS   (100U)
#define DEM_DEBOUNCE_TIME_PASSED_THRESHOLD_MS   (100U)

/*==================================================================================================
*                                    FDC CONFIGURATION
==================================================================================================*/
#define DEM_FDC_THRESHOLD               (80U)
#define DEM_FDC_MAX                     (127)
#define DEM_FDC_MIN                     (-128)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define DEM_MAIN_FUNCTION_PERIOD_MS     (10U)

/*==================================================================================================
*                                    MEMORY CONFIGURATION
==================================================================================================*/
#define DEM_PRIMARY_MEMORY_SIZE         (4096U)
#define DEM_MIRROR_MEMORY_SIZE          (2048U)
#define DEM_PERMANENT_MEMORY_SIZE       (1024U)

/*==================================================================================================
*                                    OBD CONFIGURATION
==================================================================================================*/
#define DEM_OBD_NUM_PIDS                (32U)
#define DEM_OBD_NUM_TIDS                (16U)
#define DEM_OBD_NUM_INFOTYPES           (8U)

/*==================================================================================================
*                                    J1939 CONFIGURATION
==================================================================================================*/
#define DEM_J1939_NUM_FMI               (32U)
#define DEM_J1939_NUM_OC                (16U)

#endif /* DEM_CFG_H */
