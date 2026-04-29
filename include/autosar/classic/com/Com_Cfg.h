/*
 * Com_Cfg.h
 * AUTOSAR COM Module - Configuration Header
 * 
 * Updated for T014: Configuration Tools and Engine Examples
 * 
 * Features:
 * - 19 engine and vehicle signals
 * - 4 IPDUs with transmission mode support
 * - 3 IPDU Groups (Engine, Chassis, Body)
 * - 3 Signal Groups
 * - TMC support
 */

#ifndef COM_CFG_H
#define COM_CFG_H

/*==================[Pre-compile Configuration]============================*/

/* Development Error Detection */
#define COM_DEV_ERROR_DETECT                STD_ON

/* Version Info API */
#define COM_VERSION_INFO_API                STD_ON

/* Enable Signal Group Array API */
#define COM_ENABLE_SIGNAL_GROUP_ARRAY_API   STD_ON

/* Enable MD for TMC Always/None */
#define COM_ENABLE_MDT_FOR_CYCLIC_TRANSMISSION STD_OFF

/* Optimization for supported platforms */
#define COM_OPTIMIZE_FOR_SIZE               STD_OFF
#define COM_OPTIMIZE_FOR_SPEED              STD_ON

/*==================[Configuration Constants]===============================*/

/* Maximum number of elements */
#define COM_MAX_SIGNALS                     128u
#define COM_MAX_SIGNAL_GROUPS               32u
#define COM_MAX_IPDUS                       64u
#define COM_MAX_IPDU_GROUPS                 16u

/* Maximum buffer sizes */
#define COM_MAX_IPDU_LENGTH                 64u
#define COM_MAX_SHADOW_BUFFER_SIZE          256u
#define COM_MAX_RETRY_QUEUE_SIZE            16u  /*!< Maximum retry queue entries */

/* Transmission Confirmation Defaults */
#define COM_DEFAULT_TX_TIMEOUT              100u /*!< Default TX timeout in ms */
#define COM_DEFAULT_MAX_RETRIES             3u   /*!< Default max retry count */
#define COM_RETRY_DELAY_MS                  10u  /*!< Delay between retries in ms */

/*==================[Symbolic Names]========================================*/

/* IPdu Group IDs */
#define ComConf_ComIPduGroup_EngineGroup    0u
#define ComConf_ComIPduGroup_ChassisGroup   1u
#define ComConf_ComIPduGroup_BodyGroup      2u

/* IPdu IDs */
#define ComConf_ComIPdu_EngineData          0u
#define ComConf_ComIPdu_EngineStatus        1u
#define ComConf_ComIPdu_VehicleSpeed        2u
#define ComConf_ComIPdu_BodyControl         3u

/* Signal IDs */
#define ComConf_ComSignal_EngineSpeed       0u
#define ComConf_ComSignal_CoolantTemp       1u
#define ComConf_ComSignal_ThrottlePosition  2u
#define ComConf_ComSignal_EngineTorque      3u
#define ComConf_ComSignal_EngineState       4u
#define ComConf_ComSignal_BatteryVoltage    5u
#define ComConf_ComSignal_OilPressure       6u
#define ComConf_ComSignal_OilTemp           7u
#define ComConf_ComSignal_FuelLevel         8u
#define ComConf_ComSignal_IntakeAirTemp     9u
#define ComConf_ComSignal_VehicleSpeed      10u
#define ComConf_ComSignal_WheelSpeed_FL     11u
#define ComConf_ComSignal_WheelSpeed_FR     12u
#define ComConf_ComSignal_GearPosition      13u
#define ComConf_ComSignal_TransmissionMode  14u
#define ComConf_ComSignal_ParkingBrake      15u
#define ComConf_ComSignal_TurnSignalLeft    16u
#define ComConf_ComSignal_TurnSignalRight   17u
#define ComConf_ComSignal_Headlights        18u

/* Signal Group IDs */
#define ComConf_ComSignalGroup_EngineCoreInfo       0u
#define ComConf_ComSignalGroup_EngineDiagnostics    1u
#define ComConf_ComSignalGroup_VehicleDynamics      2u

/*==================[T013: Error Handling Configuration]===================*/

/* Error Handling Module Enable */
#define COM_ERROR_HANDLING_ENABLE               STD_ON

/* Error Statistics Collection */
#define COM_ERROR_STATISTICS_ENABLE             STD_ON

/* Error Log Configuration */
#define COM_MAX_ERROR_LOG_ENTRIES               16u
#define COM_ERROR_LOG_WRAP_MODE                 STD_ON

/* Tx Queue Overflow Strategy (Default) */
/* Options: COM_TXQUEUE_REJECT_NEWEST, COM_TXQUEUE_DROP_OLDEST,
 *          COM_TXQUEUE_DROP_NEWEST, COM_TXQUEUE_REJECT_OLDEST */
#define COM_DEFAULT_OVERFLOW_STRATEGY           COM_TXQUEUE_REJECT_NEWEST

/* Error Rate Monitoring */
#define COM_ERROR_RATE_WINDOW_MS                1000u   /*!< Error rate window in ms */
#define COM_MAX_ERRORS_PER_WINDOW               10u     /*!< Max errors per window */

/* ASIL-D Safety Configuration */
#define COM_ERROR_REDUNDANT_COUNTERS            STD_ON  /*!< Enable redundant counters */
#define COM_ERROR_CHECKSUM_ENABLE               STD_ON  /*!< Enable statistics checksum */

/*==================[External Declarations]=================================*/

/* Configuration structure declared in Com_Lcfg.c */
extern const Com_ConfigType ComConfig;

/* Error handling configuration - defined in Com_Lcfg.c */
struct Com_ErrorHandlingConfigType;
extern const struct Com_ErrorHandlingConfigType Com_ErrorHandlingConfig[COM_MAX_IPDUS];

#endif /* COM_CFG_H */
