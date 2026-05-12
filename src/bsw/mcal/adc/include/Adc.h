/**
 * @file Adc.h
 * @brief ADC Driver interface following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: ADC Driver (ADC)
 * Layer: MCAL (Microcontroller Driver Layer)
 */

#ifndef ADC_H
#define ADC_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Adc_Cfg.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define ADC_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define ADC_MODULE_ID                   (0x0BU) /* ADC Driver Module ID */
#define ADC_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define ADC_AR_RELEASE_MINOR_VERSION    (0x04U)
#define ADC_AR_RELEASE_REVISION_VERSION (0x00U)
#define ADC_SW_MAJOR_VERSION            (0x01U)
#define ADC_SW_MINOR_VERSION            (0x00U)
#define ADC_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                         CONFIGURATION VARIANTS
==================================================================================================*/
#define ADC_VARIANT_PRE_COMPILE         (0x01U)
#define ADC_VARIANT_LINK_TIME           (0x02U)
#define ADC_VARIANT_POST_BUILD          (0x03U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define ADC_SID_INIT                    (0x00U)
#define ADC_SID_DEINIT                  (0x01U)
#define ADC_SID_STARTGROUPCONVERSION    (0x02U)
#define ADC_SID_STOPGROUPCONVERSION     (0x03U)
#define ADC_SID_READGROUP               (0x04U)
#define ADC_SID_ENABLEHARDWARETRIGGER   (0x05U)
#define ADC_SID_DISABLEHARDWARETRIGGER  (0x06U)
#define ADC_SID_ENABLEGROUPNOTIFICATION (0x07U)
#define ADC_SID_DISABLEGROUPNOTIFICATION (0x08U)
#define ADC_SID_GETGROUPSTATUS          (0x09U)
#define ADC_SID_GETVERSIONINFO          (0x0AU)
#define ADC_SID_GETSTREAMLASTPOINTER    (0x0BU)
#define ADC_SID_SETUPRESULTBUFFER       (0x0CU)
#define ADC_SID_SELFGROUPCHECK          (0x0DU)
#define ADC_SID_SETPOWERSTATE           (0x0EU)
#define ADC_SID_GETTARGETPOWERSTATE     (0x0FU)
#define ADC_SID_GETCURRENTPOWERSTATE    (0x10U)
#define ADC_SID_PREPAREPOWERSTATE       (0x11U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define ADC_E_UNINIT                    (0x0AU)
#define ADC_E_BUSY                      (0x0BU)
#define ADC_E_IDLE                      (0x0CU)
#define ADC_E_ALREADY_INITIALIZED       (0x0DU)
#define ADC_E_PARAM_POINTER             (0x0EU)
#define ADC_E_PARAM_GROUP               (0x0FU)
#define ADC_E_PARAM_CHANNEL             (0x10U)
#define ADC_E_PARAM_CONFIG              (0x11U)
#define ADC_E_PARAM_TRIGGERSOURCE       (0x12U)
#define ADC_E_PARAM_BUFFER              (0x13U)
#define ADC_E_NOTIF_CAPABILITY          (0x14U)
#define ADC_E_POWER_STATE_NOT_SUPPORTED (0x15U)
#define ADC_E_TRANSITION_NOT_POSSIBLE   (0x16U)
#define ADC_E_PERIPHERAL_NOT_PREPARED   (0x17U)

/*==================================================================================================
*                                    ADC HARDWARE UNIT TYPE
==================================================================================================*/
typedef uint8 Adc_HWUnitType;

/*==================================================================================================
*                                    ADC CHANNEL TYPE
==================================================================================================*/
typedef uint8 Adc_ChannelType;

/*==================================================================================================
*                                    ADC GROUP TYPE
==================================================================================================*/
typedef uint8 Adc_GroupType;

/*==================================================================================================
*                                    ADC STREAM NUMBER TYPE
==================================================================================================*/
typedef uint8 Adc_StreamNumSampleType;

/*==================================================================================================
*                                    ADC VALUE TYPE
==================================================================================================*/
typedef uint16 Adc_ValueGroupType;

/*==================================================================================================
*                                    ADC STATUS TYPE
==================================================================================================*/
typedef enum {
    ADC_IDLE = 0,
    ADC_BUSY,
    ADC_STREAM_COMPLETED
} Adc_StatusType;

/*==================================================================================================
*                                    ADC TRIGGER SOURCE TYPE
==================================================================================================*/
typedef enum {
    ADC_TRIGG_SRC_SW = 0,
    ADC_TRIGG_SRC_HW
} Adc_TriggerSourceType;

/*==================================================================================================
*                                    ADC CONVERSION MODE TYPE
==================================================================================================*/
typedef enum {
    ADC_CONV_MODE_ONESHOT = 0,
    ADC_CONV_MODE_CONTINUOUS
} Adc_ConversionModeType;

/*==================================================================================================
*                                    ADC STREAM BUFFER MODE TYPE
==================================================================================================*/
typedef enum {
    ADC_STREAM_BUFFER_LINEAR = 0,
    ADC_STREAM_BUFFER_CIRCULAR
} Adc_StreamBufferModeType;

/*==================================================================================================
*                                    ADC GROUP ACCESS MODE TYPE
==================================================================================================*/
typedef enum {
    ADC_ACCESS_MODE_SINGLE = 0,
    ADC_ACCESS_MODE_STREAMING
} Adc_GroupAccessModeType;

/*==================================================================================================
*                                    ADC SAMPLING TIME TYPE
==================================================================================================*/
typedef enum {
    ADC_SAMPLING_TIME_3CYCLES = 0,
    ADC_SAMPLING_TIME_15CYCLES,
    ADC_SAMPLING_TIME_28CYCLES,
    ADC_SAMPLING_TIME_56CYCLES,
    ADC_SAMPLING_TIME_84CYCLES,
    ADC_SAMPLING_TIME_112CYCLES,
    ADC_SAMPLING_TIME_144CYCLES,
    ADC_SAMPLING_TIME_480CYCLES
} Adc_SamplingTimeType;

/*==================================================================================================
*                                    ADC RESOLUTION TYPE
==================================================================================================*/
typedef enum {
    ADC_RESOLUTION_6BIT = 0,
    ADC_RESOLUTION_8BIT,
    ADC_RESOLUTION_10BIT,
    ADC_RESOLUTION_12BIT
} Adc_ResolutionType;

/*==================================================================================================
*                                    ADC POWER STATE TYPE
==================================================================================================*/
typedef enum {
    ADC_FULL_POWER = 0,
    ADC_LOW_POWER
} Adc_PowerStateType;

/*==================================================================================================
*                                    ADC POWER STATE REQUEST RESULT TYPE
==================================================================================================*/
typedef enum {
    ADC_SERVICE_ACCEPTED = 0,
    ADC_NOT_INIT,
    ADC_SEQUENCE_ERROR,
    ADC_HW_FAILURE,
    ADC_POWER_STATE_NOT_SUPP,
    ADC_TRANS_NOT_POSSIBLE
} Adc_PowerStateRequestResultType;

/*==================================================================================================
*                                    ADC CHANNEL CONFIG TYPE
==================================================================================================*/
typedef struct {
    Adc_ChannelType ChannelId;
    Adc_SamplingTimeType SamplingTime;
    uint8 ChannelInput;
} Adc_ChannelConfigType;

/*==================================================================================================
*                                    ADC GROUP CONFIG TYPE
==================================================================================================*/
typedef struct {
    Adc_GroupType GroupId;
    Adc_HWUnitType HwUnit;
    const Adc_ChannelType* Channels;
    uint8 NumChannels;
    Adc_TriggerSourceType TriggerSource;
    Adc_ConversionModeType ConversionMode;
    Adc_GroupAccessModeType AccessMode;
    Adc_StreamBufferModeType BufferMode;
    Adc_StreamNumSampleType NumSamples;
    Adc_ResolutionType Resolution;
    boolean GroupNotification;
    void (*NotificationFn)(void);
} Adc_GroupConfigType;

/*==================================================================================================
*                                    ADC HW UNIT CONFIG TYPE
==================================================================================================*/
typedef struct {
    Adc_HWUnitType HwUnitId;
    uint32 BaseAddress;
    uint32 ClockFrequency;
    Adc_ResolutionType DefaultResolution;
} Adc_HWUnitConfigType;

/*==================================================================================================
*                                    ADC CONFIG TYPE
==================================================================================================*/
typedef struct {
    const Adc_HWUnitConfigType* HwUnits;
    uint8 NumHwUnits;
    const Adc_GroupConfigType* Groups;
    uint8 NumGroups;
    const Adc_ChannelConfigType* Channels;
    uint8 NumChannels;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean DeInitApi;
    boolean PowerStateSupported;
} Adc_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define ADC_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Adc_ConfigType Adc_Config;

#define ADC_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define ADC_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the ADC driver
 * @param ConfigPtr Pointer to configuration structure
 */
void Adc_Init(const Adc_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the ADC driver
 */
void Adc_DeInit(void);

/**
 * @brief Starts group conversion
 * @param Group Group to start
 */
void Adc_StartGroupConversion(Adc_GroupType Group);

/**
 * @brief Stops group conversion
 * @param Group Group to stop
 */
void Adc_StopGroupConversion(Adc_GroupType Group);

/**
 * @brief Reads group conversion results
 * @param Group Group to read
 * @param DataBufferPtr Pointer to data buffer
 * @return Result of operation
 */
Std_ReturnType Adc_ReadGroup(Adc_GroupType Group, Adc_ValueGroupType* DataBufferPtr);

/**
 * @brief Enables hardware trigger for a group
 * @param Group Group to enable
 */
void Adc_EnableHardwareTrigger(Adc_GroupType Group);

/**
 * @brief Disables hardware trigger for a group
 * @param Group Group to disable
 */
void Adc_DisableHardwareTrigger(Adc_GroupType Group);

/**
 * @brief Enables group notification
 * @param Group Group to enable
 */
void Adc_EnableGroupNotification(Adc_GroupType Group);

/**
 * @brief Disables group notification
 * @param Group Group to disable
 */
void Adc_DisableGroupNotification(Adc_GroupType Group);

/**
 * @brief Gets group status
 * @param Group Group to check
 * @return Group status
 */
Adc_StatusType Adc_GetGroupStatus(Adc_GroupType Group);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void Adc_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Gets stream last pointer
 * @param Group Group to check
 * @param PtrToSamplePtr Pointer to sample pointer
 * @return Number of valid samples
 */
Adc_StreamNumSampleType Adc_GetStreamLastPointer(Adc_GroupType Group, Adc_ValueGroupType** PtrToSamplePtr);

/**
 * @brief Sets up result buffer
 * @param Group Group to setup
 * @param DataBufferPtr Pointer to data buffer
 * @return Result of operation
 */
Std_ReturnType Adc_SetupResultBuffer(Adc_GroupType Group, Adc_ValueGroupType* DataBufferPtr);

/**
 * @brief Self group check
 * @param Group Group to check
 * @return Result of check
 */
Std_ReturnType Adc_SelfGroupCheck(Adc_GroupType Group);

/**
 * @brief Sets power state
 * @param PowerState Power state to set
 * @param Result Result of operation
 */
void Adc_SetPowerState(Adc_PowerStateType PowerState, Adc_PowerStateRequestResultType* Result);

/**
 * @brief Gets target power state
 * @param TargetPowerState Pointer to store target power state
 * @param Result Result of operation
 */
void Adc_GetTargetPowerState(Adc_PowerStateType* TargetPowerState, Adc_PowerStateRequestResultType* Result);

/**
 * @brief Gets current power state
 * @param CurrentPowerState Pointer to store current power state
 * @param Result Result of operation
 */
void Adc_GetCurrentPowerState(Adc_PowerStateType* CurrentPowerState, Adc_PowerStateRequestResultType* Result);

/**
 * @brief Prepares power state transition
 * @param PowerState Target power state
 * @param Result Result of operation
 */
void Adc_PreparePowerState(Adc_PowerStateType PowerState, Adc_PowerStateRequestResultType* Result);

#define ADC_STOP_SEC_CODE
#include "MemMap.h"

#endif /* ADC_H */
