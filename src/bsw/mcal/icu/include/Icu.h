/**
 * @file Icu.h
 * @brief ICU (Input Capture Unit) Driver interface following AutoSAR Classic Platform 4.4.0 standard
 * @version 1.0.0
 * @date 2026-04-29
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
#define ICU_MODULE_ID                   (0x10U) /* ICU Driver Module ID */
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
#define ICU_SID_INIT                        (0x00U)
#define ICU_SID_DEINIT                      (0x01U)
#define ICU_SID_SETMODE                     (0x02U)
#define ICU_SID_DISABLEWAKEUP               (0x03U)
#define ICU_SID_ENABLEWAKEUP                (0x04U)
#define ICU_SID_SETACTIVATIONCONDITION      (0x05U)
#define ICU_SID_DISABLENOTIFICATION         (0x06U)
#define ICU_SID_ENABLENOTIFICATION          (0x07U)
#define ICU_SID_GETINPUTSTATE               (0x08U)
#define ICU_SID_STARTTIMESTAMP              (0x09U)
#define ICU_SID_STOPTIMESTAMP               (0x0AU)
#define ICU_SID_GETTIMESTAMPINDEX           (0x0BU)
#define ICU_SID_RESETEDGECOUNT              (0x0CU)
#define ICU_SID_ENABLEEDGECOUNT             (0x0DU)
#define ICU_SID_DISABLEEDGECOUNT            (0x0EU)
#define ICU_SID_GETEDGENUMBERS              (0x0FU)
#define ICU_SID_STARTSIGNALMEASUREMENT      (0x10U)
#define ICU_SID_STOPSIGNALMEASUREMENT       (0x11U)
#define ICU_SID_GETTIMEELAPSED              (0x12U)
#define ICU_SID_GETDUTYCYCLEVALUES          (0x13U)
#define ICU_SID_GETVERSIONINFO              (0x14U)
#define ICU_SID_CHECKWAKEUP                 (0x15U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define ICU_E_PARAM_CHANNEL                 (0x0AU)
#define ICU_E_PARAM_BUFFER_SIZE             (0x0BU)
#define ICU_E_PARAM_POINTER                 (0x0CU)
#define ICU_E_PARAM_MODE                    (0x0DU)
#define ICU_E_PARAM_ACTIVATION              (0x0EU)
#define ICU_E_ALREADY_INITIALIZED           (0x0FU)
#define ICU_E_NOT_STARTED                   (0x10U)
#define ICU_E_BUSY_OPERATION                (0x11U)
#define ICU_E_UNINIT                        (0x12U)
#define ICU_E_PARAM_CONFIG                  (0x13U)
#define ICU_E_WAKEUP_CANNOT_BE_ENABLED      (0x14U)

/*==================================================================================================
*                                    ICU CHANNEL TYPE
==================================================================================================*/
typedef uint8 Icu_ChannelType;

/*==================================================================================================
*                                    ICU VALUE TYPE
==================================================================================================*/
typedef uint32 Icu_ValueType;

/*==================================================================================================
*                                    ICU INDEX TYPE
==================================================================================================*/
typedef uint16 Icu_IndexType;

/*==================================================================================================
*                                    ICU INPUT STATE TYPE
==================================================================================================*/
typedef enum {
    ICU_IDLE = 0,
    ICU_ACTIVE
} Icu_InputStateType;

/*==================================================================================================
*                                    ICU STATE TYPE
==================================================================================================*/
typedef enum {
    ICU_STATE_UNINITIALIZED = 0,
    ICU_STATE_INITIALIZED,
    ICU_STATE_RUNNING,
    ICU_STATE_STOPPED
} Icu_StateType;

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
*                                    ICU SIGNAL EDGE TYPE
==================================================================================================*/
typedef enum {
    ICU_RISING_EDGE = 0,
    ICU_FALLING_EDGE,
    ICU_BOTH_EDGES
} Icu_SignalEdgeType;

/*==================================================================================================
*                                    ICU SIGNAL MEASUREMENT PROPERTY TYPE
==================================================================================================*/
typedef enum {
    ICU_LOW_TIME = 0,
    ICU_HIGH_TIME,
    ICU_PERIOD_TIME,
    ICU_DUTY_CYCLE
} Icu_SignalMeasurementPropertyType;

/*==================================================================================================
*                                    ICU NOTIFICATION TYPE
==================================================================================================*/
typedef void (*Icu_NotificationType)(void);

/*==================================================================================================
*                                    ICU DUTY CYCLE TYPE
==================================================================================================*/
typedef struct {
    Icu_ValueType ActiveTime;
    Icu_ValueType PeriodTime;
} Icu_DutyCycleType;

/*==================================================================================================
*                                    ICU CHANNEL CONFIG TYPE
==================================================================================================*/
typedef struct {
    Icu_ChannelType Channel;
    uint32 BaseAddress;
    Icu_MeasurementModeType Mode;
    Icu_SignalEdgeType Edge;
    Icu_SignalMeasurementPropertyType Property;
    Icu_NotificationType Notification;
    boolean TimestampEnabled;
    Icu_IndexType TimestampBufferSize;
    boolean WakeupSupport;
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
    boolean DeInitApi;
    boolean SetModeApi;
    boolean WakeupFunctionalityApi;
    boolean DisableWakeupApi;
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
 * @param Mode Mode to set (NORMAL/SLEEP)
 */
void Icu_SetMode(Icu_ModeType Mode);

/**
 * @brief Disables wakeup capability for a channel
 * @param Channel Channel to disable
 */
void Icu_DisableWakeup(Icu_ChannelType Channel);

/**
 * @brief Enables wakeup capability for a channel
 * @param Channel Channel to enable
 */
void Icu_EnableWakeup(Icu_ChannelType Channel);

/**
 * @brief Sets the activation condition for edge detection
 * @param Channel Channel to configure
 * @param Activation Type of edge (RISING/FALLING/BOTH)
 */
void Icu_SetActivationCondition(Icu_ChannelType Channel, Icu_SignalEdgeType Activation);

/**
 * @brief Disables notification for a channel
 * @param Channel Channel to disable
 */
void Icu_DisableNotification(Icu_ChannelType Channel);

/**
 * @brief Enables notification for a channel
 * @param Channel Channel to enable
 */
void Icu_EnableNotification(Icu_ChannelType Channel);

/**
 * @brief Gets the input state of a channel
 * @param Channel Channel to check
 * @return Input state (IDLE/ACTIVE)
 */
Icu_InputStateType Icu_GetInputState(Icu_ChannelType Channel);

/**
 * @brief Starts timestamp capture
 * @param Channel Channel to start
 * @param BufferPtr Pointer to timestamp buffer
 * @param BufferSize Size of buffer
 * @param NotifyInterval Interval for notifications
 */
void Icu_StartTimestamp(Icu_ChannelType Channel, Icu_ValueType* BufferPtr, 
                        Icu_IndexType BufferSize, Icu_IndexType NotifyInterval);

/**
 * @brief Stops timestamp capture
 * @param Channel Channel to stop
 */
void Icu_StopTimestamp(Icu_ChannelType Channel);

/**
 * @brief Gets the current timestamp buffer index
 * @param Channel Channel to check
 * @return Current buffer index
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
Icu_EdgeNumberType Icu_GetEdgeNumbers(Icu_ChannelType Channel);

/**
 * @brief Starts signal measurement for a channel
 * @param Channel Channel to start
 */
void Icu_StartSignalMeasurement(Icu_ChannelType Channel);

/**
 * @brief Stops signal measurement for a channel
 * @param Channel Channel to stop
 */
void Icu_StopSignalMeasurement(Icu_ChannelType Channel);

/**
 * @brief Gets the elapsed time (period/pulse width)
 * @param Channel Channel to check
 * @return Elapsed time in ticks
 */
Icu_ValueType Icu_GetTimeElapsed(Icu_ChannelType Channel);

/**
 * @brief Gets the duty cycle values
 * @param Channel Channel to check
 * @param DutyCycleValues Pointer to duty cycle structure
 */
void Icu_GetDutyCycleValues(Icu_ChannelType Channel, Icu_DutyCycleType* DutyCycleValues);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
#if (ICU_VERSION_INFO_API == STD_ON)
void Icu_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Checks for wakeup events
 * @param Channel Channel to check
 * @return Wakeup detected flag
 */
Std_ReturnType Icu_CheckWakeup(Icu_ChannelType Channel);

#define ICU_STOP_SEC_CODE
#include "MemMap.h"

#endif /* ICU_H */
