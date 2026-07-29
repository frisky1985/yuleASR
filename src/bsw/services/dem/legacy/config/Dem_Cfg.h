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

/***********************************************************************************************************************
 * File:        Dem_Cfg.h
 * Description: Dem (Diagnostic Event Manager) compile-time configuration
 *              AUTOSAR Specification 4.4.0 compliant
 **********************************************************************************************************************/

#ifndef DEM_CFG_H
#define DEM_CFG_H

/*==================================================================================================
 *                                      VERSION INFO
==================================================================================================*/
#define DEM_CFG_VENDOR_ID                      (100U)
#define DEM_CFG_MODULE_ID                      (54U)
#define DEM_CFG_MAJOR_VERSION                  (1U)
#define DEM_CFG_MINOR_VERSION                  (0U)
#define DEM_CFG_PATCH_VERSION                  (0U)

/*==================================================================================================
 *                                      GENERAL CONFIGURATION
==================================================================================================*/

/* Development error detection */
#define DEM_DEV_ERROR_DETECT                   STD_ON

/* Version info API */
#define DEM_VERSION_INFO_API                   STD_ON

/* Debug support */
#define DEM_DEBUG_SUPPORT                      STD_OFF

/* Support for freeze frames */
#define DEM_CFG_FreezeFrameSupport             STD_ON

/* Support for extended data */
#define DEM_CFG_ExtendedDataSupport            STD_ON

/* Support for aging */
#define DEM_CFG_AgingSupport                   STD_ON

/* Support for operation cycles */
#define DEM_CFG_OperationCycleSupport          STD_ON

/* Support for indicator */
#define DEM_CFG_IndicatorSupport               STD_ON

/* Support for event debounce */
#define DEM_CFG_EventDebounceSupport           STD_ON

/* Support for OBD (On-Board Diagnostics) */
#define DEM_CFG_OBDSupport                     STD_OFF

/* Support for J1939 */
#define DEM_CFG_J1939Support                   STD_OFF

/* Support for BswM notification */
#define DEM_CFG_BswMSupport                    STD_ON

/* Support for Fim */
#define DEM_CFG_FimSupport                     STD_ON

/* Support for Dcm */
#define DEM_CFG_DcmSupport                     STD_ON

/*==================================================================================================
 *                                      EVENT CONFIGURATION
==================================================================================================*/

/* Maximum number of events */
#define DEM_CFG_MAX_NUMBER_EVENTS              (100U)

/* Maximum number of DTCs */
#define DEM_CFG_MAX_NUMBER_DTCS                (80U)

/* Maximum number of freeze frame records per DTC */
#define DEM_CFG_MAX_FREEZEFRAME_RECORDS        (3U)

/* Maximum number of extended data records per DTC */
#define DEM_CFG_MAX_EXTENDED_DATA_RECORDS      (2U)

/* Maximum size of freeze frame data */
#define DEM_CFG_MAX_FREEZEFRAME_SIZE           (50U)

/* Maximum size of extended data */
#define DEM_CFG_MAX_EXTENDED_DATA_SIZE         (20U)

/* Maximum number of configured indicators */
#define DEM_CFG_MAX_INDICATORS                 (8U)

/* Maximum number of indicator attributes per event */
#define DEM_CFG_MAX_INDICATOR_ATTRIBUTES       (2U)

/* Maximum number of operation cycles */
#define DEM_CFG_MAX_OPERATION_CYCLES           (4U)

/* Maximum number of debounce algorithms */
#define DEM_CFG_MAX_DEBOUNCE_ALGORITHMS        (50U)

/* Event queue size */
#define DEM_CFG_EVENT_QUEUE_SIZE               (20U)

/*==================================================================================================
 *                                      DTC CONFIGURATION
==================================================================================================*/

/* DTC format - OBD or UDS */
#define DEM_CFG_DTC_FORMAT                     DEM_DTC_FORMAT_UDS

/* Primary memory entry limit */
#define DEM_CFG_PRIMARY_MEMORY_MAX_ENTRIES     (50U)

/* Mirror memory entry limit */
#define DEM_CFG_MIRROR_MEMORY_MAX_ENTRIES      (10U)

/* Permanent DTC memory limit */
#define DEM_CFG_PERMANENT_MEMORY_MAX_ENTRIES   (5U)

/* Number of user defined memories */
#define DEM_CFG_USER_DEFINED_MEMORY            (0U)

/* Aging counter threshold */
#define DEM_CFG_AGING_COUNTER_THRESHOLD        (40U)

/* Aging cycle threshold */
#define DEM_CFG_AGING_CYCLE_THRESHOLD          (40U)

/*==================================================================================================
 *                                      DEBOUNCE CONFIGURATION
==================================================================================================*/

/* Counter based debounce support */
#define DEM_CFG_DEBOUNCE_COUNTER_SUPPORT       STD_ON

/* Time based debounce support */
#define DEM_CFG_DEBOUNCE_TIME_SUPPORT          STD_ON

/* Monitor internal debounce support */
#define DEM_CFG_DEBOUNCE_MONITOR_SUPPORT       STD_ON

/* Maximum debounce counter value */
#define DEM_CFG_DEBOUNCE_COUNTER_MAX           (127)

/* Minimum debounce counter value */
#define DEM_CFG_DEBOUNCE_COUNTER_MIN           (-128)

/* Debounce counter failed threshold */
#define DEM_CFG_DEBOUNCE_COUNTER_FAILED        (127)

/* Debounce counter passed threshold */
#define DEM_CFG_DEBOUNCE_COUNTER_PASSED        (-128)

/*==================================================================================================
 *                                      CALLBACK CONFIGURATION
==================================================================================================*/

/* Callback on event status changed */
#define DEM_CFG_CALLBACK_ON_EVC_STATUS_CHANGED STD_ON

/* Callback on event data changed */
#define DEM_CFG_CALLBACK_ON_EVC_DATA_CHANGED   STD_OFF

/* Callback on DTC status changed */
#define DEM_CFG_CALLBACK_ON_DTC_STATUS_CHANGED STD_ON

/* Callback on monitor status changed */
#define DEM_CFG_CALLBACK_ON_MONITOR_STATUS_CHANGED STD_OFF

/* Callback on operation cycle status changed */
#define DEM_CFG_CALLBACK_ON_CYCLE_STATUS_CHANGED STD_ON

/*==================================================================================================
 *                                      TASK CONFIGURATION
==================================================================================================*/

/* Dem main function period in ms */
#define DEM_CFG_MAIN_FUNCTION_PERIOD_MS        (10U)

/* Debounce task period in ms */
#define DEM_CFG_DEBOUNCE_TASK_PERIOD_MS        (10U)

/* NVM write retry count */
#define DEM_CFG_NVM_WRITE_RETRY                (3U)

/* NVM read retry count */
#define DEM_CFG_NVM_READ_RETRY                 (3U)

/* NVM write timeout in ms */
#define DEM_CFG_NVM_WRITE_TIMEOUT_MS           (100U)

/* NVM read timeout in ms */
#define DEM_CFG_NVM_READ_TIMEOUT_MS            (50U)

/*==================================================================================================
 *                                      TIMEOUT CONFIGURATION
==================================================================================================*/

/* Clear DTC timeout in ms */
#define DEM_CFG_CLEAR_DTC_TIMEOUT_MS           (10000U)

/* Disable DTC record update timeout in ms */
#define DEM_CFG_DISABLE_DTC_RECORD_TIMEOUT_MS  (5000U)

/* Get DTC status timeout in ms */
#define DEM_CFG_GET_DTC_STATUS_TIMEOUT_MS      (1000U)

/*==================================================================================================
 *                                      SPECIAL CONFIGURATION
==================================================================================================*/

/* Support for combined DTCs */
#define DEM_CFG_COMBINED_DTCS_SUPPORT          STD_OFF

/* Support for multiple event memories */
#define DEM_CFG_MULTIPLE_EVENT_MEMORIES        STD_OFF

/* Support for time-based debounce */
#define DEM_CFG_TIME_BASED_DEBOUNCE            STD_ON

/* Support for frequency based debounce */
#define DEM_CFG_FREQUENCY_BASED_DEBOUNCE       STD_OFF

/* Support for storing conditions */
#define DEM_CFG_STORAGE_CONDITIONS_SUPPORT     STD_OFF

/* Support for enable conditions */
#define DEM_CFG_ENABLE_CONDITIONS_SUPPORT      STD_OFF

/* Support for component available */
#define DEM_CFG_COMPONENT_AVAILABLE_SUPPORT    STD_OFF

/* Support for PID 41 monitoring */
#define DEM_CFG_PID41_SUPPORT                  STD_OFF

/* Support for PID 1C monitoring */
#define DEM_CFG_PID1C_SUPPORT                  STD_OFF

/* Support for WWH-OBD */
#define DEM_CFG_WWH_OBD_SUPPORT                STD_OFF

/* Support for worldwide harmonized OBD */
#define DEM_CFG_WORLD_WIDE_HARMONIZED_OBD      STD_OFF

/* Support for emission-related DTCs only */
#define DEM_CFG_EMISSION_RELATED_DTCS_ONLY     STD_OFF

/* Support for clear DTC limit */
#define DEM_CFG_CLEAR_DTCLIMIT_SUPPORT         STD_OFF

/* Support for warning indicator */
#define DEM_CFG_WARNING_INDICATOR_SUPPORTED    STD_ON

/* Support for MIL indicator */
#define DEM_CFG_MIL_INDICATOR_SUPPORTED        STD_ON

/* Support for service only tracking */
#define DEM_CFG_SERVICE_ONLY_TRACKING          STD_OFF

#endif /* DEM_CFG_H */
