/**
 * @file Icu.h
 * @brief ICU (Input Capture Unit) Driver interface following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: ICU Driver (ICU)
 * Layer: MCAL (Microcontroller Driver Layer)
 */

#ifndef ICU_H
#define ICU_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Icu_Cfg.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define ICU_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define ICU_MODULE_ID                   (0x16U) /* ICU Driver Module ID */
#define ICU_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define ICU_AR_RELEASE_MINOR_VERSION    (0x04U)
#define ICU_AR_RELEASE_REVISION_VERSION (0x00U)
#define ICU_SW_MAJOR_VERSION            (0x01U)
#define ICU_SW_MINOR_VERSION            (0x00U)
#define ICU_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                         CONFIGURATION VARIANTS
==================================================================================================*/
#define ICU_VARIANT_PRE_COMPILE         (0x01U)
#define ICU_VARIANT_LINK_TIME           (0x02U)
#define ICU_VARIANT_POST_BUILD          (0x03U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define ICU_SID_INIT                            (0x00U)
#define ICU_SID_DEINIT                          (0x01U)
#define ICU_SID_SETMODE                         (0x02U)
#define ICU_SID_DISABLEWAKEUP                   (0x03U)
#define ICU_SID_ENABLEWAKEUP                    (0x04U)
#define ICU_SID_CHECKWAKEUP                     (0x05U)
#define ICU_SID_SETACTIVATIONCONDITION          (0x06U)
#define ICU_SID_DISABLENOTIFICATION             (0x07U)
#define ICU_SID_ENABLENOTIFICATION              (0x08U)
#define ICU_SID_GETINPUTSTATE                   (0x09U)
#define ICU_SID_STARTTIMESTAMP                  (0x0AU)
#define ICU_SID_STOPTIMESTAMP                   (0x0BU)
#define ICU_SID_GETTIMESTAMPINDEX               (0x0CU)
#define ICU_SID_RESETEDGECOUNT                  (0x0DU)
#define ICU_SID_ENABLEEDGECOUNT                 (0x0EU)
#define ICU_SID_DISABLEEDGECOUNT                (0x0FU)
#define ICU_SID_GETEDGENUMBERS                  (0x10U)
#define ICU_SID_STARTSIGNALMEASUREMENT          (0x11U)
#define ICU_SID_STOPSIGNALMEASUREMENT           (0x12U)
#define ICU_SID_GETTIMEELAPSED                  (0x13U)
#define ICU_SID_GETDUTYCYCLEVALUES              (0x14U)
#define ICU_SID_GETVERSIONINFO                  (0x15U)
#define ICU_SID_GETINPUTLEVEL                   (0x16U)
#define ICU_SID_GETSYSTIMESTAMP                 (0x17U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define ICU_E_PARAM_CONFIG                      (0x0AU)
#define ICU_E_UNINIT                            (0x0BU)
#define ICU_E_PARAM_CHANNEL                     (0x0CU)
#define ICU_E_PARAM_ACTIVATION                  (0x0DU)
#define ICU_E_PARAM_BUFFER_SIZE                 (0x0EU)
#define ICU_E_ALREADY_INITIALIZED               (0x0FU)
#define ICU_E_PARAM_POINTER                     (0x10U)
#define ICU_E_BUSY                              (0x11U)
#define ICU_E_WAKEUP_NOT_ENABLED                (0x12U)
#define ICU_E_WAKEUP_ALREADY_ENABLED            (0x13U)
#define ICU_E_MEASUREMENT_NOT_RUNNING           (0x14U)
#define ICU_E_MEASUREMENT_RUNNING               (0x15U)
#define ICU_E_STAMP_NOT_RUNNING                 (0x16U)
#define ICU_E_EDGE_COUNTING_NOT_RUNNING         (0x17U)
#define ICU_E_EDGE_ALREADY_ENABLED              (0x18U)
#define ICU_E_EDGE_ALREADY_DISABLED             (0x19U)

/*==================================================================================================
*                                    ICU CHANNEL TYPE
==================================================================================================*/
typedef uint8 Icu_ChannelType;

/*==================================================================================================
*                                    ICU DRIVER STATE TYPE
*==================================================================================================*/
typedef enum {
    ICU_STATE_UNINIT = 0,
    ICU_STATE_INIT,
    ICU_STATE_BUSY
} Icu_StateType;

/*==================================================================================================
*                                    ICU VALUE TYPE
*==================================================================================================*/
typedef uint32 Icu_ValueType;
typedef uint32 Icu_EdgeNumberType;

/*==================================================================================================
*                                    ICU SIGNAL EDGE TYPE
*==================================================================================================*/
typedef enum {
    ICU_SIGNAL_EDGE_RISING = 0,
    ICU_SIGNAL_EDGE_FALLING,
    ICU_SIGNAL_EDGE_BOTH
} Icu_SignalEdgeType;

/*==================================================================================================
*                                    ICU INPUT STATE TYPE
==================================================================================================*/
typedef enum {
    ICU_ACTIVE = 0,
    ICU_IDLE
} Icu_InputStateType;

/*==================================================================================================
*                                    ICU ACTIVATION TYPE (Edge Detection)
==================================================================================================*/
typedef enum {
    ICU_FALLING_EDGE = 0,
    ICU_RISING_EDGE,
    ICU_BOTH_EDGES
} Icu_ActivationType;

/*==================================================================================================
*                                    ICU MODE TYPE
==================================================================================================*/
typedef enum {
    ICU_MODE_NORMAL = 0,
    ICU_MODE_SLEEP
} Icu_ModeType;

/*==================================================================================================
*                                    ICU MEASUREMENT MODE TYPE
==================================================================================================*/
typedef enum {
    ICU_MODE_SIGNAL_EDGE_DETECT = 0,
    ICU_MODE_SIGNAL_MEASUREMENT,
    ICU_MODE_TIMESTAMP,
    ICU_MODE_EDGE_COUNTER
} Icu_MeasurementModeType;

/*==================================================================================================
*                                    ICU SIGNAL MEASUREMENT PROPERTY TYPE
==================================================================================================*/
typedef enum {
    ICU_PERIOD_TIME = 0,
    ICU_HIGH_TIME,
    ICU_LOW_TIME,
    ICU_DUTY_CYCLE
} Icu_SignalMeasurementPropertyType;

/*==================================================================================================
*                                    ICU TIMESTAMP PROPERTY TYPE
==================================================================================================*/
typedef enum {
    ICU_LINEAR_BUFFER = 0,
    ICU_CIRCULAR_BUFFER
} Icu_TimestampBufferType;

/*==================================================================================================
*                                    ICU INDEX TYPE
==================================================================================================*/
typedef uint16 Icu_IndexType;

/*==================================================================================================
*                                    ICU DUTY CYCLE TYPE
==================================================================================================*/
typedef struct {
    uint16 ActiveTime;
    uint16 PeriodTime;
} Icu_DutyCycleType;

/*==================================================================================================
*                                    ICU CHANNEL CONFIG TYPE
==================================================================================================*/
typedef struct {
    Icu_ChannelType ChannelId;
    uint32 BaseAddress;
    Icu_MeasurementModeType MeasurementMode;
    Icu_ActivationType DefaultActivation;
    Icu_SignalMeasurementPropertyType SignalMeasurementProperty;
    Icu_TimestampBufferType TimestampBufferType;
    uint16 BufferSize;
    uint32* BufferPtr;
    boolean WakeupSupport;
    boolean NotificationEnabled;
    void (*NotificationFn)(void);
    uint32 ClockPrescaler;
} Icu_ChannelConfigType;

/*==================================================================================================
*                                    ICU CONFIG TYPE
==================================================================================================*/
typedef struct {
    const Icu_ChannelConfigType* Channels;
    uint8 NumChannels;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean WakeupFunctionalityApi;
    boolean DeInitApi;
    boolean SetModeApi;
    boolean DisableWakeupApi;
    boolean EnableWakeupApi;
    boolean CheckWakeupApi;
    boolean TimestampApi;
    boolean EdgeCountApi;
    boolean SignalMeasurementApi;
    Icu_ModeType DefaultMode;
} Icu_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define ICU_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Icu_ConfigType Icu_Config;

#define ICU_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define ICU_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the ICU driver
 * @param ConfigPtr Pointer to configuration structure
 */
void Icu_Init(const Icu_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the ICU driver
 */
void Icu_DeInit(void);

/**
 * @brief Sets the operation mode
 * @param Mode Mode to set (ICU_MODE_NORMAL or ICU_MODE_SLEEP)
 */
void Icu_SetMode(Icu_ModeType Mode);

/**
 * @brief Disables wakeup for a channel
 * @param Channel Channel to disable wakeup
 */
void Icu_DisableWakeup(Icu_ChannelType Channel);

/**
 * @brief Enables wakeup for a channel
 * @param Channel Channel to enable wakeup
 */
void Icu_EnableWakeup(Icu_ChannelType Channel);

/**
 * @brief Checks for wakeup events
 * @param WakeupSource Wakeup source to check
 * @return Wakeup detected flag
 */
Std_ReturnType Icu_CheckWakeup(uint32 WakeupSource);

/**
 * @brief Sets the activation condition (edge detection) for a channel
 * @param Channel Channel to configure
 * @param Activation Activation condition (falling, rising, or both edges)
 */
void Icu_SetActivationCondition(Icu_ChannelType Channel, Icu_ActivationType Activation);

/**
 * @brief Disables notification for a channel
 * @param Channel Channel to disable notification
 */
void Icu_DisableNotification(Icu_ChannelType Channel);

/**
 * @brief Enables notification for a channel
 * @param Channel Channel to enable notification
 */
void Icu_EnableNotification(Icu_ChannelType Channel);

/**
 * @brief Gets the input state of a channel
 * @param Channel Channel to check
 * @return Current input state (ICU_ACTIVE or ICU_IDLE)
 */
Icu_InputStateType Icu_GetInputState(Icu_ChannelType Channel);

/**
 * @brief Starts timestamp capture for a channel
 * @param Channel Channel to start timestamp
 * @param BufferPtr Pointer to buffer for storing timestamps
 * @param BufferSize Size of the buffer
 * @param NotifyInterval Number of captures between notifications
 */
void Icu_StartTimestamp(Icu_ChannelType Channel, uint32* BufferPtr, uint16 BufferSize, uint16 NotifyInterval);

/**
 * @brief Stops timestamp capture for a channel
 * @param Channel Channel to stop timestamp
 */
void Icu_StopTimestamp(Icu_ChannelType Channel);

/**
 * @brief Gets the current timestamp buffer index
 * @param Channel Channel to check
 * @return Current index in the timestamp buffer
 */
Icu_IndexType Icu_GetTimestampIndex(Icu_ChannelType Channel);

/**
 * @brief Resets the edge counter for a channel
 * @param Channel Channel to reset
 */
void Icu_ResetEdgeCount(Icu_ChannelType Channel);

/**
 * @brief Enables edge counting for a channel
 * @param Channel Channel to enable
 */
void Icu_EnableEdgeCount(Icu_ChannelType Channel);

/**
 * @brief Disables edge counting for a channel
 * @param Channel Channel to disable
 */
void Icu_DisableEdgeCount(Icu_ChannelType Channel);

/**
 * @brief Gets the number of counted edges
 * @param Channel Channel to check
 * @return Number of edges counted
 */
uint16 Icu_GetEdgeNumbers(Icu_ChannelType Channel);

/**
 * @brief Starts signal measurement for a channel
 * @param Channel Channel to start measurement
 * @param MeasureKind Type of measurement (period, high time, low time, duty cycle)
 */
void Icu_StartSignalMeasurement(Icu_ChannelType Channel, Icu_SignalMeasurementPropertyType MeasureKind);

/**
 * @brief Stops signal measurement for a channel
 * @param Channel Channel to stop measurement
 */
void Icu_StopSignalMeasurement(Icu_ChannelType Channel);

/**
 * @brief Gets the elapsed time for signal measurement
 * @param Channel Channel to get time from
 * @return Elapsed time in ticks
 */
uint16 Icu_GetTimeElapsed(Icu_ChannelType Channel);

/**
 * @brief Gets the duty cycle values for a channel
 * @param Channel Channel to get duty cycle from
 * @param DutyCycleValues Pointer to structure to store duty cycle values
 */
void Icu_GetDutyCycleValues(Icu_ChannelType Channel, Icu_DutyCycleType* DutyCycleValues);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void Icu_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Gets the current input level of a channel
 * @param Channel Channel to check
 * @return Current input level (0 or 1)
 */
uint8 Icu_GetInputLevel(Icu_ChannelType Channel);

/**
 * @brief Gets the system timestamp
 * @return System timestamp in ticks
 */
uint32 Icu_GetSysTimestamp(void);

#define ICU_STOP_SEC_CODE
#include "MemMap.h"

#endif /* ICU_H */
