/**
 * @file Rte_Cfg.h
 * @brief RTE Configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef RTE_CFG_H
#define RTE_CFG_H

/*==================================================================================================
*                                    RTE GENERAL CONFIGURATION
==================================================================================================*/
#define RTE_DEV_ERROR_DETECT            (STD_ON)
#define RTE_VERSION_INFO_API            (STD_ON)
#define RTE_MEASUREMENT_SUPPORT         (STD_ON)
#define RTE_CALIBRATION_SUPPORT         (STD_ON)

/*==================================================================================================
*                                    RTE COMPONENT CONFIGURATION
==================================================================================================*/
#define RTE_NUM_COMPONENTS              (16U)
#define RTE_NUM_INSTANCES               (32U)
#define RTE_NUM_RUNNABLES               (64U)
#define RTE_NUM_PORTS                   (128U)
#define RTE_NUM_INTERFACES              (64U)

/*==================================================================================================
*                                    RTE COMMUNICATION CONFIGURATION
==================================================================================================*/
#define RTE_NUM_SR_INTERFACES           (64U)
#define RTE_NUM_CS_INTERFACES           (32U)
#define RTE_NUM_MODE_INTERFACES         (16U)
#define RTE_NUM_TRIGGER_INTERFACES      (8U)

/*==================================================================================================
*                                    RTE DATA CONFIGURATION
==================================================================================================*/
#define RTE_NUM_DATA_ELEMENTS           (256U)
#define RTE_NUM_OPERATIONS              (128U)
#define RTE_NUM_PARAMETERS              (64U)
#define RTE_NUM_FIELD_ELEMENTS          (32U)

/*==================================================================================================
*                                    RTE MEMORY CONFIGURATION
==================================================================================================*/
#define RTE_NUM_IRV                     (64U)
#define RTE_NUM_PIM                     (32U)
#define RTE_NUM_CALPRM                  (32U)
#define RTE_NUM_MEASUREMENT             (64U)

/*==================================================================================================
*                                    RTE EXCLUSIVE AREA CONFIGURATION
==================================================================================================*/
#define RTE_NUM_EXCLUSIVE_AREAS         (16U)

/*==================================================================================================
*                                    RTE BUFFER CONFIGURATION
==================================================================================================*/
#define RTE_MAX_BUFFER_SIZE             (4096U)
#define RTE_MAX_STRING_LENGTH           (256U)
#define RTE_MAX_BYTE_ARRAY_LENGTH       (4096U)
#define RTE_MAX_QUEUE_LENGTH            (16U)

/*==================================================================================================
*                                    RTE TRANSFORMATION CONFIGURATION
==================================================================================================*/
#define RTE_NUM_TRANSFORMERS            (16U)
#define RTE_TRANSFORMER_BUFFER_SIZE     (4096U)

/*==================================================================================================
*                                    RTE SOME/IP CONFIGURATION
==================================================================================================*/
#define RTE_SOMEIP_SUPPORT              (STD_ON)
#define RTE_SOMEIP_NUM_SERVICES         (32U)
#define RTE_SOMEIP_NUM_METHODS          (64U)
#define RTE_SOMEIP_NUM_EVENTS           (32U)
#define RTE_SOMEIP_NUM_FIELDS           (16U)

/*==================================================================================================
*                                    RTE E2E CONFIGURATION
==================================================================================================*/
#define RTE_E2E_SUPPORT                 (STD_ON)
#define RTE_E2E_NUM_PROFILES            (8U)
#define RTE_E2E_NUM_DATA_IDS            (256U)

/*==================================================================================================
*                                    RTE COM CONFIGURATION
==================================================================================================*/
#define RTE_COM_SUPPORT                 (STD_ON)
#define RTE_COM_NUM_SIGNALS             (256U)
#define RTE_COM_NUM_SIGNAL_GROUPS       (64U)

/*==================================================================================================
*                                    RTE OS CONFIGURATION
==================================================================================================*/
#define RTE_OS_TASKS_NUM                (8U)
#define RTE_OS_EVENTS_NUM               (32U)
#define RTE_OS_ALARMS_NUM               (16U)

/*==================================================================================================
*                                    RTE MAIN FUNCTION PERIOD
==================================================================================================*/
#define RTE_MAIN_FUNCTION_PERIOD_MS     (10U)

/*==================================================================================================
*                                    RTE TIMEOUT CONFIGURATION
==================================================================================================*/
#define RTE_DEFAULT_TIMEOUT_MS          (100U)
#define RTE_MAX_TIMEOUT_MS              (10000U)

/*==================================================================================================
*                                    RTE COMPONENT IDs
==================================================================================================*/
#define RTE_COMPONENT_SWC_ECU_MANAGER       (0U)
#define RTE_COMPONENT_SWC_DIAGNOSTIC        (1U)
#define RTE_COMPONENT_SWC_COMMUNICATION     (2U)
#define RTE_COMPONENT_SWC_STORAGE           (3U)
#define RTE_COMPONENT_SWC_IO_CONTROL        (4U)
#define RTE_COMPONENT_SWC_MODE_MANAGER      (5U)
#define RTE_COMPONENT_SWC_WATCHDOG          (6U)
#define RTE_COMPONENT_SWC_NVM_MANAGER       (7U)
#define RTE_COMPONENT_SWC_DCM_HANDLER       (8U)
#define RTE_COMPONENT_SWC_DEM_HANDLER       (9U)
#define RTE_COMPONENT_SWC_COM_HANDLER       (10U)
#define RTE_COMPONENT_SWC_PDUR_HANDLER      (11U)
#define RTE_COMPONENT_SWC_CANIF_HANDLER     (12U)
#define RTE_COMPONENT_SWC_ETHIF_HANDLER     (13U)
#define RTE_COMPONENT_SWC_LINIF_HANDLER     (14U)
#define RTE_COMPONENT_SWC_FRIF_HANDLER      (15U)

/*==================================================================================================
*                                    RTE RUNNABLE IDs
==================================================================================================*/
#define RTE_RUNNABLE_INIT                   (0U)
#define RTE_RUNNABLE_MAIN                   (1U)
#define RTE_RUNNABLE_CYCLIC_10MS            (2U)
#define RTE_RUNNABLE_CYCLIC_100MS           (3U)
#define RTE_RUNNABLE_EVENT_RECEIVED         (4U)
#define RTE_RUNNABLE_OPERATION_CALLED       (5U)
#define RTE_RUNNABLE_MODE_SWITCHED          (6U)
#define RTE_RUNNABLE_ERROR_HANDLER          (7U)

/*==================================================================================================
*                                    RTE PORT IDs
==================================================================================================*/
#define RTE_PORT_SR_ENGINE_STATUS           (0U)
#define RTE_PORT_SR_VEHICLE_SPEED           (1U)
#define RTE_PORT_SR_BATTERY_STATUS          (2U)
#define RTE_PORT_SR_DIAG_REQUEST            (3U)
#define RTE_PORT_SR_DIAG_RESPONSE           (4U)
#define RTE_PORT_CS_DIAG_SERVICE            (5U)
#define RTE_PORT_CS_STORAGE_SERVICE         (6U)
#define RTE_PORT_CS_MODE_SERVICE            (7U)
#define RTE_PORT_MD_OPERATION_MODE          (8U)
#define RTE_PORT_MD_DIAGNOSTIC_MODE         (9U)
#define RTE_PORT_TR_WAKEUP_EVENT            (10U)
#define RTE_PORT_TR_ERROR_EVENT             (11U)

/*==================================================================================================
*                                    RTE DATA ELEMENT IDs
==================================================================================================*/
#define RTE_DE_ENGINE_RPM                   (0U)
#define RTE_DE_VEHICLE_SPEED                (1U)
#define RTE_DE_BATTERY_VOLTAGE              (2U)
#define RTE_DE_ENGINE_TEMP                  (3U)
#define RTE_DE_FUEL_LEVEL                   (4U)
#define RTE_DE_ODOMETER                     (5U)
#define RTE_DE_GEAR_POSITION                (6U)
#define RTE_DE_DOOR_STATUS                  (7U)
#define RTE_DE_DTC_STATUS                   (8U)
#define RTE_DE_DIAG_REQUEST_DATA            (9U)
#define RTE_DE_DIAG_RESPONSE_DATA           (10U)

/*==================================================================================================
*                                    RTE OPERATION IDs
==================================================================================================*/
#define RTE_OP_DIAG_READ_DATA               (0U)
#define RTE_OP_DIAG_WRITE_DATA              (1U)
#define RTE_OP_DIAG_ROUTINE_CONTROL         (2U)
#define RTE_OP_STORAGE_READ_BLOCK           (3U)
#define RTE_OP_STORAGE_WRITE_BLOCK          (4U)
#define RTE_OP_MODE_SWITCH                  (5U)
#define RTE_OP_IO_CONTROL                   (6U)

/*==================================================================================================
*                                    RTE MODE IDs
==================================================================================================*/
#define RTE_MODE_GROUP_OPERATION            (0U)
#define RTE_MODE_GROUP_DIAGNOSTIC           (1U)
#define RTE_MODE_GROUP_COMMUNICATION        (2U)
#define RTE_MODE_GROUP_STORAGE              (3U)

/*==================================================================================================
*                                    RTE MODE VALUES
==================================================================================================*/
#define RTE_MODE_OPERATION_NORMAL           (0U)
#define RTE_MODE_OPERATION_SLEEP            (1U)
#define RTE_MODE_OPERATION_STARTUP          (2U)
#define RTE_MODE_OPERATION_SHUTDOWN         (3U)

#define RTE_MODE_DIAGNOSTIC_DEFAULT         (0U)
#define RTE_MODE_DIAGNOSTIC_EXTENDED        (1U)
#define RTE_MODE_DIAGNOSTIC_PROGRAMMING     (2U)

#define RTE_MODE_COMMUNICATION_FULL         (0U)
#define RTE_MODE_COMMUNICATION_SILENT       (1U)
#define RTE_MODE_COMMUNICATION_NONE         (2U)

#define RTE_MODE_STORAGE_NORMAL             (0U)
#define RTE_MODE_STORAGE_WRITE_PROTECTED    (1U)
#define RTE_MODE_STORAGE_ERROR              (2U)

/*==================================================================================================
*                                    RTE IRV IDs
==================================================================================================*/
#define RTE_IRV_ENGINE_STATUS               (0U)
#define RTE_IRV_VEHICLE_STATUS              (1U)
#define RTE_IRV_DIAG_STATUS                 (2U)
#define RTE_IRV_STORAGE_STATUS              (3U)

/*==================================================================================================
*                                    RTE PIM IDs
==================================================================================================*/
#define RTE_PIM_CONFIG_DATA                 (0U)
#define RTE_PIM_CALIBRATION_DATA            (1U)
#define RTE_PIM_RUNTIME_DATA                (2U)

/*==================================================================================================
*                                    RTE CALPRM IDs
==================================================================================================*/
#define RTE_CALPRM_ENGINE_PARAMS            (0U)
#define RTE_CALPRM_VEHICLE_PARAMS           (1U)
#define RTE_CALPRM_DIAG_PARAMS              (2U)

/*==================================================================================================
*                                    RTE MEASUREMENT IDs
==================================================================================================*/
#define RTE_MEAS_ENGINE_PERF                (0U)
#define RTE_MEAS_VEHICLE_PERF               (1U)
#define RTE_MEAS_DIAG_PERF                  (2U)
#define RTE_MEAS_COM_PERF                   (3U)

/*==================================================================================================
*                                    RTE EXCLUSIVE AREA IDs
==================================================================================================*/
#define RTE_EA_GLOBAL                       (0U)
#define RTE_EA_COMMUNICATION                (1U)
#define RTE_EA_STORAGE                      (2U)
#define RTE_EA_DIAGNOSTIC                   (3U)

#endif /* RTE_CFG_H */
