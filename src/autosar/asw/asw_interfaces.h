/**
 * @file asw_interfaces.h
 * @brief Application Software Layer Interface Definitions
 * @version 1.0.0
 * @date 2026-04-19
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Classic Platform 4.x - Application Software Component Interfaces
 * Purpose: Common interface definitions for ASW components
 */

#ifndef ASW_INTERFACES_H
#define ASW_INTERFACES_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Rte_Type.h"

/*==================================================================================================
*                                    ENGINE CONTROL INTERFACES
==================================================================================================*/

/**
 * @brief Engine state data element
 */
typedef uint8 EngineState_DE;
#define ENGINE_STATE_OFF_DE                 0x00
#define ENGINE_STATE_CRANKING_DE            0x01
#define ENGINE_STATE_RUNNING_DE             0x02
#define ENGINE_STATE_STOPPING_DE            0x03
#define ENGINE_STATE_FAULT_DE               0x04

/**
 * @brief Engine parameters data element
 */
typedef struct {
    uint16 engineSpeed_DE;
    uint16 engineLoad_DE;
    sint16 engineTemperature_DE;
    uint16 throttlePosition_DE;
    uint16 fuelInjectionTime_DE;
    uint16 ignitionAdvance_DE;
} EngineParameters_DE;

/**
 * @brief Engine control output data element
 */
typedef struct {
    uint16 fuelPulseWidth_DE;
    uint16 ignitionTiming_DE;
    uint16 idleSpeedTarget_DE;
    boolean fuelCutoff_DE;
    boolean ignitionCutoff_DE;
} EngineControlOutput_DE;

/*==================================================================================================
*                                    VEHICLE DYNAMICS INTERFACES
==================================================================================================*/

/**
 * @brief VDC state data element
 */
typedef uint8 VdcState_DE;
#define VDC_STATE_INACTIVE_DE               0x00
#define VDC_STATE_ACTIVE_DE                 0x01
#define VDC_STATE_INTERVENING_DE            0x02
#define VDC_STATE_FAULT_DE                  0x03

/**
 * @brief Vehicle motion data element
 */
typedef struct {
    uint16 vehicleSpeed_DE;
    sint16 longitudinalAccel_DE;
    sint16 lateralAccel_DE;
    sint16 yawRate_DE;
    sint16 steeringAngle_DE;
    uint16 wheelSpeedFL_DE;
    uint16 wheelSpeedFR_DE;
    uint16 wheelSpeedRL_DE;
    uint16 wheelSpeedRR_DE;
} VehicleMotion_DE;

/**
 * @brief VDC output data element
 */
typedef struct {
    sint16 brakeForceFront_DE;
    sint16 brakeForceRear_DE;
    sint16 brakeForceLeft_DE;
    sint16 brakeForceRight_DE;
    sint16 torqueReduction_DE;
    boolean stabilityIntervention_DE;
    boolean tractionControlActive_DE;
} VdcOutput_DE;

/*==================================================================================================
*                                    DIAGNOSTIC INTERFACES
==================================================================================================*/

/**
 * @brief Diagnostic session data element
 */
typedef uint8 DiagnosticSession_DE;
#define DIAG_SESSION_DEFAULT_DE             0x01
#define DIAG_SESSION_PROGRAMMING_DE         0x02
#define DIAG_SESSION_EXTENDED_DE            0x03
#define DIAG_SESSION_SAFETY_SYSTEM_DE       0x04

/**
 * @brief Security level data element
 */
typedef uint8 SecurityLevel_DE;
#define SECURITY_LOCKED_DE                  0x00
#define SECURITY_LEVEL_1_DE                 0x01
#define SECURITY_LEVEL_2_DE                 0x02
#define SECURITY_LEVEL_3_DE                 0x03

/**
 * @brief DTC status data element
 */
typedef struct {
    uint32 dtcCode_DE;
    uint8 statusByte_DE;
    uint8 faultDetectionCounter_DE;
    uint8 occurrenceCounter_DE;
    uint32 agingCounter_DE;
    uint32 lastOccurrenceTime_DE;
} DtcStatus_DE;

/*==================================================================================================
*                                    COMMUNICATION INTERFACES
==================================================================================================*/

/**
 * @brief Communication state data element
 */
typedef uint8 CommState_DE;
#define COMM_STATE_OFF_DE                   0x00
#define COMM_STATE_INIT_DE                  0x01
#define COMM_STATE_READY_DE                 0x02
#define COMM_STATE_ACTIVE_DE                0x03
#define COMM_STATE_FAULT_DE                 0x04

/**
 * @brief Signal value data element
 */
typedef struct {
    uint16 signalId_DE;
    uint64 value_DE;
    uint32 timestamp_DE;
    boolean isValid_DE;
    boolean isUpdated_DE;
} SignalValue_DE;

/**
 * @brief PDU information data element
 */
typedef struct {
    uint16 pduId_DE;
    uint8 busType_DE;
    uint8 length_DE;
    uint8 data_DE[64];
    uint32 timestamp_DE;
} PduInfo_DE;

/*==================================================================================================
*                                    STORAGE INTERFACES
==================================================================================================*/

/**
 * @brief Storage block state data element
 */
typedef uint8 StorageBlockState_DE;
#define STORAGE_BLOCK_EMPTY_DE              0x00
#define STORAGE_BLOCK_VALID_DE              0x01
#define STORAGE_BLOCK_INVALID_DE            0x02
#define STORAGE_BLOCK_INCONSISTENT_DE       0x03
#define STORAGE_BLOCK_WRITING_DE            0x04

/**
 * @brief Storage block status data element
 */
typedef struct {
    uint16 blockId_DE;
    uint8 state_DE;
    uint32 writeCycleCounter_DE;
    uint32 lastWriteTime_DE;
    uint16 dataLength_DE;
    uint16 crc_DE;
} StorageBlockStatus_DE;

/*==================================================================================================
*                                    IO CONTROL INTERFACES
==================================================================================================*/

/**
 * @brief IO state data element
 */
typedef uint8 IOState_DE;
#define IO_STATE_INACTIVE_DE                0x00
#define IO_STATE_ACTIVE_DE                  0x01
#define IO_STATE_FAULT_DE                   0x02
#define IO_STATE_TEST_DE                    0x03

/**
 * @brief Digital IO value data element
 */
typedef struct {
    uint16 channelId_DE;
    boolean value_DE;
    uint32 timestamp_DE;
    boolean isValid_DE;
} DigitalIOValue_DE;

/**
 * @brief Analog IO value data element
 */
typedef struct {
    uint16 channelId_DE;
    uint16 rawValue_DE;
    uint16 physicalValue_DE;
    uint32 timestamp_DE;
    boolean isValid_DE;
} AnalogIOValue_DE;

/**
 * @brief PWM IO value data element
 */
typedef struct {
    uint16 channelId_DE;
    uint16 dutyCycle_DE;
    uint16 frequency_DE;
    uint32 timestamp_DE;
    boolean isValid_DE;
} PwmIOValue_DE;

/*==================================================================================================
*                                    MODE MANAGER INTERFACES
==================================================================================================*/

/**
 * @brief System mode data element
 */
typedef uint8 SystemMode_DE;
#define SYSTEM_MODE_OFF_DE                  0x00
#define SYSTEM_MODE_INIT_DE                 0x01
#define SYSTEM_MODE_STANDBY_DE              0x02
#define SYSTEM_MODE_NORMAL_DE               0x03
#define SYSTEM_MODE_DIAGNOSTIC_DE           0x04
#define SYSTEM_MODE_SLEEP_DE                0x05
#define SYSTEM_MODE_EMERGENCY_DE            0x06

/**
 * @brief System state data element
 */
typedef uint8 SystemState_DE;
#define SYSTEM_STATE_OFF_DE                 0x00
#define SYSTEM_STATE_INITIALIZING_DE        0x01
#define SYSTEM_STATE_READY_DE               0x02
#define SYSTEM_STATE_RUNNING_DE             0x03
#define SYSTEM_STATE_DEGRADED_DE            0x04
#define SYSTEM_STATE_SHUTDOWN_DE            0x05
#define SYSTEM_STATE_ERROR_DE               0x06

/**
 * @brief Mode transition request data element
 */
typedef struct {
    uint8 targetMode_DE;
    uint8 requestSource_DE;
    uint32 requestTime_DE;
    uint8 priority_DE;
    boolean isForced_DE;
} ModeTransitionRequest_DE;

/*==================================================================================================
*                                    WATCHDOG INTERFACES
==================================================================================================*/

/**
 * @brief Watchdog status data element
 */
typedef uint8 WatchdogStatus_DE;
#define WDG_STATUS_OK_DE                    0x00
#define WDG_STATUS_EXPIRED_DE               0x01
#define WDG_STATUS_STOPPED_DE               0x02
#define WDG_STATUS_FAULT_DE                 0x03

/**
 * @brief Alive state data element
 */
typedef uint8 AliveState_DE;
#define ALIVE_STATE_CORRECT_DE              0x00
#define ALIVE_STATE_INCORRECT_DE            0x01
#define ALIVE_STATE_EXPIRED_DE              0x02
#define ALIVE_STATE_DEACTIVATED_DE          0x03

/**
 * @brief Supervised entity status data element
 */
typedef struct {
    uint8 entityId_DE;
    uint8 state_DE;
    uint16 aliveIndications_DE;
    uint16 missedIndications_DE;
    uint32 lastAliveTime_DE;
    uint32 supervisionCycle_DE;
} SupervisedEntityStatus_DE;

#endif /* ASW_INTERFACES_H */
