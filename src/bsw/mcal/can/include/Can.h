/**
 * @file Can.h
 * @brief CAN Driver interface following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: CAN Driver (CAN)
 * Layer: MCAL (Microcontroller Driver Layer)
 */

#ifndef CAN_H
#define CAN_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Can_Cfg.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define CAN_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define CAN_MODULE_ID                   (0x50U) /* CAN Driver Module ID */
#define CAN_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define CAN_AR_RELEASE_MINOR_VERSION    (0x04U)
#define CAN_AR_RELEASE_REVISION_VERSION (0x00U)
#define CAN_SW_MAJOR_VERSION            (0x01U)
#define CAN_SW_MINOR_VERSION            (0x00U)
#define CAN_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                         CONFIGURATION VARIANTS
==================================================================================================*/
#define CAN_VARIANT_PRE_COMPILE         (0x01U)
#define CAN_VARIANT_LINK_TIME           (0x02U)
#define CAN_VARIANT_POST_BUILD          (0x03U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define CAN_SID_INIT                    (0x00U)
#define CAN_SID_GETVERSIONINFO          (0x01U)
#define CAN_SID_SETCONTROLLERMODE       (0x03U)
#define CAN_SID_DISABLECONTROLLERINTERRUPTS (0x04U)
#define CAN_SID_ENABLECONTROLLERINTERRUPTS  (0x05U)
#define CAN_SID_WRITECANID              (0x06U)
#define CAN_SID_WRITECANDATA            (0x07U)
#define CAN_SID_WRITECANREMOTE          (0x08U)
#define CAN_SID_MAINFUNCTIONWRITE       (0x09U)
#define CAN_SID_MAINFUNCTIONREAD        (0x0AU)
#define CAN_SID_MAINFUNCTIONBUSOFF      (0x0BU)
#define CAN_SID_MAINFUNCTIONWAKEUP      (0x0CU)
#define CAN_SID_MAINFUNCTIONMODE        (0x0DU)
#define CAN_SID_CHECKWAKEUP             (0x0EU)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define CAN_E_PARAM_POINTER             (0x01U)
#define CAN_E_PARAM_HANDLE              (0x02U)
#define CAN_E_PARAM_DLC                 (0x03U)
#define CAN_E_PARAM_CONTROLLER          (0x04U)
#define CAN_E_UNINIT                    (0x05U)
#define CAN_E_TRANSITION                (0x06U)
#define CAN_E_PARAM_BAUDRATE            (0x07U)
#define CAN_E_ICOM_CONFIG_INVALID       (0x08U)
#define CAN_E_INIT_FAILED               (0x09U)
#define CAN_E_FATAL                     (0x0AU)

/*==================================================================================================
*                                    CONTROLLER STATES
==================================================================================================*/
typedef enum {
    CAN_CS_UNINIT = 0,
    CAN_CS_STARTED,
    CAN_CS_STOPPED,
    CAN_CS_SLEEP
} Can_ControllerStateType;

/*==================================================================================================
*                                    CAN HARDWARE OBJECT TYPES
==================================================================================================*/
typedef enum {
    CAN_HOH_TYPE_RECEIVE = 0,
    CAN_HOH_TYPE_TRANSMIT
} Can_HohTypeType;

/*==================================================================================================
*                                    CAN ID TYPES
==================================================================================================*/
typedef enum {
    CAN_ID_TYPE_STANDARD = 0,
    CAN_ID_TYPE_EXTENDED
} Can_IdTypeType;

/*==================================================================================================
*                                    CAN RETURN TYPES
==================================================================================================*/
typedef enum {
    CAN_OK = 0,
    CAN_NOT_OK,
    CAN_BUSY
} Can_ReturnType;

/*==================================================================================================
*                                    CAN OBJECT HANDLE TYPE
==================================================================================================*/
typedef uint16 Can_HwHandleType;

/*==================================================================================================
*                                    CAN PDU TYPE
==================================================================================================*/
typedef struct {
    Can_IdTypeType idType;
    uint32 CanId;
    uint8 CanDlc;
    const uint8* SduPtr;
} Can_PduType;

/*==================================================================================================
*                                    CAN HARDWARE OBJECT CONFIG TYPE
==================================================================================================*/
typedef struct {
    Can_HwHandleType Hoh;
    Can_HohTypeType HohType;
    Can_IdTypeType IdType;
    uint32 FirstId;
    uint32 LastId;
    uint8 ObjectId;
    boolean Filtering;
} Can_HardwareObjectType;

/*==================================================================================================
*                                    CAN BAUDRATE CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint32 BaudRate;
    uint32 PropSeg;
    uint32 PhaseSeg1;
    uint32 PhaseSeg2;
    uint32 SyncJumpWidth;
    uint32 Prescaler;
} Can_BaudrateConfigType;

/*==================================================================================================
*                                    CAN CONTROLLER CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint8 ControllerId;
    uint32 BaseAddress;
    const Can_BaudrateConfigType* BaudrateConfigs;
    uint8 NumBaudrateConfigs;
    const Can_HardwareObjectType* HardwareObjects;
    uint8 NumHardwareObjects;
    uint32 RxProcessing;
    uint32 TxProcessing;
    boolean BusOffProcessing;
    boolean WakeupProcessing;
    boolean WakeupSupport;
    uint8 DefaultBaudrateIndex;
} Can_ControllerConfigType;

/*==================================================================================================
*                                    CAN CONFIG TYPE
==================================================================================================*/
typedef struct {
    const Can_ControllerConfigType* Controllers;
    uint8 NumControllers;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
} Can_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define CAN_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Can_ConfigType Can_Config;

#define CAN_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define CAN_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the CAN driver
 * @param Config Pointer to configuration structure
 */
void Can_Init(const Can_ConfigType* Config);

/**
 * @brief Returns version information
 * @param versioninfo Pointer to version info structure
 */
void Can_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Sets the controller mode
 * @param Controller CAN controller to change
 * @param Transition Requested transition
 * @return Result of operation
 */
Can_ReturnType Can_SetControllerMode(uint8 Controller, Can_ControllerStateType Transition);

/**
 * @brief Disables CAN controller interrupts
 * @param Controller CAN controller
 */
void Can_DisableControllerInterrupts(uint8 Controller);

/**
 * @brief Enables CAN controller interrupts
 * @param Controller CAN controller
 */
void Can_EnableControllerInterrupts(uint8 Controller);

/**
 * @brief Writes CAN message to hardware
 * @param Hth Hardware transmit handle
 * @param PduInfo Pointer to PDU info
 * @return Result of operation
 */
Can_ReturnType Can_Write(Can_HwHandleType Hth, const Can_PduType* PduInfo);

/**
 * @brief Main function for write processing
 */
void Can_MainFunction_Write(void);

/**
 * @brief Main function for read processing
 */
void Can_MainFunction_Read(void);

/**
 * @brief Main function for bus-off processing
 */
void Can_MainFunction_BusOff(void);

/**
 * @brief Main function for wakeup processing
 */
void Can_MainFunction_Wakeup(void);

/**
 * @brief Main function for mode transition processing
 */
void Can_MainFunction_Mode(void);

/**
 * @brief Checks for wakeup events
 * @param Controller CAN controller to check
 * @return Wakeup detected flag
 */
Std_ReturnType Can_CheckWakeup(uint8 Controller);

#define CAN_STOP_SEC_CODE
#include "MemMap.h"

#endif /* CAN_H */
