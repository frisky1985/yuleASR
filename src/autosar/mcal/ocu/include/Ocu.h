/**
 * @file Ocu.h
 * @brief OCU (Output Compare Unit) Driver interface following AutoSAR Classic Platform 4.4.0 standard
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: OCU Driver (OCU)
 * Layer: MCAL (Microcontroller Driver Layer)
 * ASIL Level: D
 */

#ifndef OCU_H
#define OCU_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Ocu_Cfg.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define OCU_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define OCU_MODULE_ID                   (0x7AU) /* OCU Driver Module ID */
#define OCU_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define OCU_AR_RELEASE_MINOR_VERSION    (0x04U)
#define OCU_AR_RELEASE_REVISION_VERSION (0x00U)
#define OCU_SW_MAJOR_VERSION            (0x01U)
#define OCU_SW_MINOR_VERSION            (0x00U)
#define OCU_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    VERSION CHECK
==================================================================================================*/
#if (!defined STD_TYPES_H)
    #error "Ocu.h: Std_Types.h not included"
#endif

/*==================================================================================================
*                                    CONFIGURATION VARIANTS
==================================================================================================*/
#define OCU_VARIANT_PRE_COMPILE         (0x01U)
#define OCU_VARIANT_LINK_TIME           (0x02U)
#define OCU_VARIANT_POST_BUILD          (0x03U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define OCU_SID_INIT                    (0x00U)
#define OCU_SID_DEINIT                  (0x01U)
#define OCU_SID_STARTCHANNEL            (0x02U)
#define OCU_SID_STOPCHANNEL             (0x03U)
#define OCU_SID_SETPINSTATE             (0x04U)
#define OCU_SID_SETPINACTION            (0x05U)
#define OCU_SID_SETABSOLUTETHRESHOLD    (0x06U)
#define OCU_SID_SETRELATIVETHRESHOLD    (0x07U)
#define OCU_SID_GETCOUNTER              (0x08U)
#define OCU_SID_DISABLENOTIFICATION     (0x09U)
#define OCU_SID_ENABLENOTIFICATION      (0x0AU)
#define OCU_SID_GETVERSIONINFO          (0x0BU)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define OCU_E_PARAM_POINTER             (0x01U)
#define OCU_E_PARAM_CONFIG              (0x02U)
#define OCU_E_UNINIT                    (0x03U)
#define OCU_E_ALREADY_INITIALIZED       (0x04U)
#define OCU_E_PARAM_CHANNEL             (0x05U)
#define OCU_E_PARAM_INVALID_STATE       (0x06U)
#define OCU_E_PARAM_ACTION              (0x07U)
#define OCU_E_PARAM_PIN_STATE           (0x08U)
#define OCU_E_CHANNEL_BUSY              (0x09U)
#define OCU_E_PARAM_REF_VALUE           (0x0AU)
#define OCU_E_PARAM_THRESHOLD_VALUE     (0x0BU)
#define OCU_E_INIT_FAILED               (0x0CU)
#define OCU_E_NO_TICKS_PER_CHANNEL      (0x0DU)

/*==================================================================================================
*                                    TYPE DEFINITIONS
==================================================================================================*/

/**
 * @brief OCU Channel Type
 */
typedef uint8 Ocu_ChannelType;

/**
 * @brief OCU Value Type (for compare values)
 */
typedef uint32 Ocu_ValueType;

/**
 * @brief OCU Output Pin State Type
 */
typedef enum {
    OCU_HIGH = 0x00U,
    OCU_LOW  = 0x01U
} Ocu_OutputPinStateType;

/**
 * @brief OCU Pin Action Type
 * @details Defines the action to be performed on the output pin when compare match occurs
 */
typedef enum {
    OCU_SET_HIGH = 0x00U,      /**< Set pin to HIGH on compare match */
    OCU_SET_LOW  = 0x01U,      /**< Set pin to LOW on compare match */
    OCU_TOGGLE   = 0x02U,      /**< Toggle pin on compare match */
    OCU_HOLD     = 0x03U       /**< Hold pin state (no change) */
} Ocu_PinActionType;

/**
 * @brief OCU State Type
 */
typedef enum {
    OCU_STOPPED = 0x00U,
    OCU_RUNNING = 0x01U
} Ocu_StateType;

/**
 * @brief OCU Notification Callback Type
 */
typedef void (*Ocu_NotificationType)(void);

/**
 * @brief OCU Channel Configuration Type
 */
typedef struct {
    Ocu_ChannelType ChannelId;                /**< Channel identifier */
    Ocu_OutputPinStateType DefaultPinState;   /**< Default pin state */
    Ocu_ValueType DefaultThreshold;           /**< Default compare threshold */
    Ocu_NotificationType Notification;        /**< Notification callback function */
    boolean RunningInBackground;              /**< Background running flag */
    uint32 BaseAddress;                       /**< Hardware register base address */
} Ocu_ChannelConfigType;

/**
 * @brief OCU Configuration Type
 */
typedef struct {
    const Ocu_ChannelConfigType* Channels;    /**< Pointer to channel configurations */
    uint8 NumChannels;                        /**< Number of channels */
    boolean DevErrorDetect;                   /**< Development error detection enable */
    boolean VersionInfoApi;                   /**< Version info API enable */
    boolean DeInitApi;                        /**< DeInit API enable */
    boolean PinStateApi;                      /**< SetPinState API enable */
    boolean SetPinActionApi;                  /**< SetPinAction API enable */
    boolean SetThresholdApi;                  /**< SetAbsolute/RelativeThreshold API enable */
    boolean NotificationSupported;            /**< Notification support enable */
    Ocu_ValueType MaxCounterValue;            /**< Maximum counter value */
} Ocu_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define OCU_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Ocu_ConfigType Ocu_Config;

#define OCU_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define OCU_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the OCU driver
 * @param ConfigPtr Pointer to configuration structure
 * @details This function initializes all OCU channels based on the configuration
 */
void Ocu_Init(const Ocu_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the OCU driver
 * @details Stops all channels and resets hardware to default state
 */
void Ocu_DeInit(void);

/**
 * @brief Starts an OCU channel
 * @param Channel Channel to start
 * @details Starts the output compare operation for the specified channel
 */
void Ocu_StartChannel(Ocu_ChannelType Channel);

/**
 * @brief Stops an OCU channel
 * @param Channel Channel to stop
 * @details Stops the output compare operation for the specified channel
 */
void Ocu_StopChannel(Ocu_ChannelType Channel);

/**
 * @brief Sets the pin state directly (manual control)
 * @param Channel Channel to set
 * @param PinState Pin state to set
 * @details Directly sets the output pin state without waiting for compare match
 */
void Ocu_SetPinState(Ocu_ChannelType Channel, Ocu_OutputPinStateType PinState);

/**
 * @brief Sets the pin action on compare match
 * @param Channel Channel to configure
 * @param PinAction Action to perform on compare match
 * @details Configures what action should be performed when compare match occurs
 */
void Ocu_SetPinAction(Ocu_ChannelType Channel, Ocu_PinActionType PinAction);

/**
 * @brief Sets an absolute threshold value
 * @param Channel Channel to configure
 * @param ReferenceValue Reference counter value
 * @param AbsoluteValue Absolute compare value
 * @return Result of operation
 * @retval E_OK: Success
 * @retval E_NOT_OK: Failed (e.g., value out of range)
 * @details Sets an absolute compare threshold. Compare match occurs when
 *          counter equals the absolute value.
 */
Std_ReturnType Ocu_SetAbsoluteThreshold(Ocu_ChannelType Channel,
                                        Ocu_ValueType ReferenceValue,
                                        Ocu_ValueType AbsoluteValue);

/**
 * @brief Sets a relative threshold value
 * @param Channel Channel to configure
 * @param RelativeValue Relative value to add to current counter
 * @return Result of operation
 * @retval E_OK: Success
 * @retval E_NOT_OK: Failed (e.g., overflow)
 * @details Sets a relative compare threshold. Compare match occurs when
 *          counter equals current value + relative value.
 */
Std_ReturnType Ocu_SetRelativeThreshold(Ocu_ChannelType Channel,
                                        Ocu_ValueType RelativeValue);

/**
 * @brief Gets the current counter value
 * @param Channel Channel to read
 * @return Current counter value
 * @details Returns the current value of the counter associated with the channel
 */
Ocu_ValueType Ocu_GetCounter(Ocu_ChannelType Channel);

/**
 * @brief Disables notification for a channel
 * @param Channel Channel to disable
 * @details Disables the notification callback for the specified channel
 */
void Ocu_DisableNotification(Ocu_ChannelType Channel);

/**
 * @brief Enables notification for a channel
 * @param Channel Channel to enable
 * @details Enables the notification callback for the specified channel
 */
void Ocu_EnableNotification(Ocu_ChannelType Channel);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
#if (OCU_VERSION_INFO_API == STD_ON)
void Ocu_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

#define OCU_STOP_SEC_CODE
#include "MemMap.h"

#endif /* OCU_H */
