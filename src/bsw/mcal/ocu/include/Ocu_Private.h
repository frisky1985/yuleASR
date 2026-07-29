/**
 * @file Ocu_Private.h
 * @brief OCU (Output Compare Unit) Driver private header
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: OCU Driver Private
 * Layer: MCAL (Microcontroller Driver Layer)
 * ASIL Level: D
 */

#ifndef OCU_PRIVATE_H
#define OCU_PRIVATE_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Ocu.h"
#include "Ocu_Cfg.h"
#include "Ocu_Lcfg.h"

#if (OCU_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define OCU_PRIVATE_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define OCU_PRIVATE_MODULE_ID                   (0x7AU) /* OCU Module ID */
#define OCU_PRIVATE_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define OCU_PRIVATE_AR_RELEASE_MINOR_VERSION    (0x04U)
#define OCU_PRIVATE_AR_RELEASE_REVISION_VERSION (0x00U)
#define OCU_PRIVATE_SW_MAJOR_VERSION            (0x01U)
#define OCU_PRIVATE_SW_MINOR_VERSION            (0x00U)
#define OCU_PRIVATE_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    VERSION CHECK
==================================================================================================*/
#if (OCU_AR_RELEASE_MAJOR_VERSION != OCU_PRIVATE_AR_RELEASE_MAJOR_VERSION)
    #error "Ocu_Private.h: AR major version mismatch with Ocu.h"
#endif

#if (OCU_AR_RELEASE_MINOR_VERSION != OCU_PRIVATE_AR_RELEASE_MINOR_VERSION)
    #error "Ocu_Private.h: AR minor version mismatch with Ocu.h"
#endif

/*==================================================================================================
*                                    INTERNAL MACROS
==================================================================================================*/
/**
 * @brief Module initialization states
 */
#define OCU_UNINIT                              (0x00U)
#define OCU_INITIALIZED                         (0x01U)

/**
 * @brief Channel index validation macro
 */
#define OCU_IS_VALID_CHANNEL(ch) \
    (((ch) < OCU_NUM_CHANNELS) ? TRUE : FALSE)

/**
 * @brief Check if module is initialized
 */
#define OCU_IS_INITIALIZED() \
    ((Ocu_ModuleState == OCU_INITIALIZED) ? TRUE : FALSE)

/**
 * @brief Report development error macro
 */
#if (OCU_DEV_ERROR_DETECT == STD_ON)
#define OCU_REPORT_ERROR(ApiId, ErrorId) \
    (void)Det_ReportError(OCU_MODULE_ID, 0U, (ApiId), (ErrorId))
#else
#define OCU_REPORT_ERROR(ApiId, ErrorId)
#endif

/**
 * @brief Report development error with validation
 */
#if (OCU_DEV_ERROR_DETECT == STD_ON)
#define OCU_VALIDATE(Expr, ApiId, ErrorId, RetVal) \
    do { \
        if (!(Expr)) { \
            OCU_REPORT_ERROR((ApiId), (ErrorId)); \
            return (RetVal); \
        } \
    } while(0)
#else
#define OCU_VALIDATE(Expr, ApiId, ErrorId, RetVal)
#endif

/**
 * @brief Report development error with void return
 */
#if (OCU_DEV_ERROR_DETECT == STD_ON)
#define OCU_VALIDATE_VOID(Expr, ApiId, ErrorId) \
    do { \
        if (!(Expr)) { \
            OCU_REPORT_ERROR((ApiId), (ErrorId)); \
            return; \
        } \
    } while(0)
#else
#define OCU_VALIDATE_VOID(Expr, ApiId, ErrorId)
#endif

/*==================================================================================================
*                                    INTERNAL TYPE DEFINITIONS
==================================================================================================*/
/**
 * @brief OCU Channel State Structure
 */
typedef struct {
    Ocu_StateType State;                      /**< Channel state (stopped/running) */
    Ocu_OutputPinStateType CurrentPinState;   /**< Current pin state */
    Ocu_ValueType CompareValue;               /**< Current compare value */
    Ocu_PinActionType PinAction;              /**< Current pin action */
    boolean IsRunning;                        /**< Running flag */
    boolean NotificationEnabled;              /**< Notification enable flag */
} Ocu_ChannelStateType;

/**
 * @brief OCU Hardware Register Structure
 */
typedef struct {
    volatile uint32 Control;                  /**< Control register */
    volatile uint32 Status;                   /**< Status register */
    volatile uint32 Counter;                  /**< Counter register */
    volatile uint32 Compare;                  /**< Compare value register */
    volatile uint32 Action;                   /**< Pin action register */
    volatile uint32 PinCtrl;                  /**< Pin control register */
} Ocu_HwRegisterType;

/*==================================================================================================
*                                    INTERNAL VARIABLES
==================================================================================================*/
#define OCU_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

/**
 * @brief Module initialization state
 */
extern uint8 Ocu_ModuleState;

#define OCU_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

#define OCU_START_SEC_VAR_NO_INIT_UNSPECIFIED
#include "MemMap.h"

/**
 * @brief Channel runtime states
 */
extern Ocu_ChannelStateType Ocu_ChannelState[OCU_NUM_CHANNELS];

/**
 * @brief Pointer to current configuration
 */
extern const Ocu_ConfigType* Ocu_CurrentConfig;

#define OCU_STOP_SEC_VAR_NO_INIT_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    INTERNAL FUNCTION PROTOTYPES
==================================================================================================*/
#define OCU_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initialize hardware for a channel
 * @param Channel Channel to initialize
 * @param Config Channel configuration
 */
void Ocu_HwInitChannel(Ocu_ChannelType Channel, const Ocu_ChannelConfigType* Config);

/**
 * @brief Deinitialize hardware for a channel
 * @param Channel Channel to deinitialize
 */
void Ocu_HwDeInitChannel(Ocu_ChannelType Channel);

/**
 * @brief Start hardware channel
 * @param Channel Channel to start
 */
void Ocu_HwStartChannel(Ocu_ChannelType Channel);

/**
 * @brief Stop hardware channel
 * @param Channel Channel to stop
 */
void Ocu_HwStopChannel(Ocu_ChannelType Channel);

/**
 * @brief Set hardware pin state
 * @param Channel Channel to set
 * @param PinState Pin state to set
 */
void Ocu_HwSetPinState(Ocu_ChannelType Channel, Ocu_OutputPinStateType PinState);

/**
 * @brief Set hardware pin action
 * @param Channel Channel to set
 * @param PinAction Pin action to set
 */
void Ocu_HwSetPinAction(Ocu_ChannelType Channel, Ocu_PinActionType PinAction);

/**
 * @brief Set hardware compare value
 * @param Channel Channel to set
 * @param Value Compare value
 */
void Ocu_HwSetCompareValue(Ocu_ChannelType Channel, Ocu_ValueType Value);

/**
 * @brief Get hardware counter value
 * @param Channel Channel to read
 * @return Counter value
 */
Ocu_ValueType Ocu_HwGetCounter(Ocu_ChannelType Channel);

/**
 * @brief Get hardware register base address
 * @param Channel Channel
 * @return Register base address
 */
Ocu_HwRegisterType* Ocu_HwGetRegisterBase(Ocu_ChannelType Channel);

/**
 * @brief Process compare match interrupt
 * @param Channel Channel that triggered interrupt
 */
void Ocu_ProcessCompareMatch(Ocu_ChannelType Channel);

#define OCU_STOP_SEC_CODE
#include "MemMap.h"

#endif /* OCU_PRIVATE_H */
