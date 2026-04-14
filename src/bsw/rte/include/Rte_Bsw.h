/**
 * @file Rte_Bsw.h
 * @brief RTE BSW Module Interfaces following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Runtime Environment (RTE)
 * Layer: RTE (Runtime Environment)
 * Purpose: BSW module RTE interfaces for component communication
 */

#ifndef RTE_BSW_H
#define RTE_BSW_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Rte.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                    RTE BSW COMPONENT IDs
==================================================================================================*/
#define RTE_BSW_COMPONENT_ECU_MANAGER       (0x00U)
#define RTE_BSW_COMPONENT_DIAGNOSTIC        (0x01U)
#define RTE_BSW_COMPONENT_COMMUNICATION     (0x02U)
#define RTE_BSW_COMPONENT_STORAGE           (0x03U)
#define RTE_BSW_COMPONENT_IO_CONTROL        (0x04U)
#define RTE_BSW_COMPONENT_MODE_MANAGER      (0x05U)
#define RTE_BSW_COMPONENT_WATCHDOG          (0x06U)
#define RTE_BSW_COMPONENT_NVM_MANAGER       (0x07U)
#define RTE_BSW_COMPONENT_DCM_HANDLER       (0x08U)
#define RTE_BSW_COMPONENT_DEM_HANDLER       (0x09U)
#define RTE_BSW_COMPONENT_COM_HANDLER       (0x0AU)
#define RTE_BSW_COMPONENT_PDUR_HANDLER      (0x0BU)
#define RTE_BSW_COMPONENT_CANIF_HANDLER     (0x0CU)
#define RTE_BSW_COMPONENT_ETHIF_HANDLER     (0x0DU)
#define RTE_BSW_COMPONENT_LINIF_HANDLER     (0x0EU)
#define RTE_BSW_COMPONENT_FRIF_HANDLER      (0x0FU)

/*==================================================================================================
*                                    RTE BSW ECU MANAGER INTERFACE
==================================================================================================*/

/**
 * @brief ECU Manager Mode Type
 */
typedef enum {
    RTE_BSW_ECUM_MODE_STARTUP = 0,
    RTE_BSW_ECUM_MODE_RUN,
    RTE_BSW_ECUM_MODE_POST_RUN,
    RTE_BSW_ECUM_MODE_SHUTDOWN,
    RTE_BSW_ECUM_MODE_SLEEP
} Rte_Bsw_Ecum_ModeType;

/**
 * @brief ECU Manager State Type
 */
typedef enum {
    RTE_BSW_ECUM_STATE_UNINITIALIZED = 0,
    RTE_BSW_ECUM_STATE_INITIALIZED,
    RTE_BSW_ECUM_STATE_STARTED,
    RTE_BSW_ECUM_STATE_STOPPED
} Rte_Bsw_Ecum_StateType;

/**
 * @brief ECU Manager Interface Functions
 */
#define RTE_START_SEC_CODE
#include "MemMap.h"

extern Rte_StatusType Rte_Bsw_Ecum_SelectShutdownTarget(uint8 target, uint8 mode);
extern Rte_StatusType Rte_Bsw_Ecum_GetShutdownTarget(uint8* target, uint8* mode);
extern Rte_StatusType Rte_Bsw_Ecum_ReleaseRUN(uint8 user);
extern Rte_StatusType Rte_Bsw_Ecum_RequestRUN(uint8 user);
extern Rte_StatusType Rte_Bsw_Ecum_ReleasePOST_RUN(uint8 user);
extern Rte_StatusType Rte_Bsw_Ecum_RequestPOST_RUN(uint8 user);
extern Rte_StatusType Rte_Bsw_Ecum_KillAllRUNRequests(void);
extern Rte_StatusType Rte_Bsw_Ecum_SelectBootTarget(uint8 target);
extern Rte_StatusType Rte_Bsw_Ecum_GetBootTarget(uint8* target);
extern void Rte_Bsw_Ecum_SetMode(Rte_Bsw_Ecum_ModeType mode);
extern Rte_Bsw_Ecum_ModeType Rte_Bsw_Ecum_GetMode(void);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE BSW DIAGNOSTIC INTERFACE
==================================================================================================*/

/**
 * @brief Diagnostic Session Type
 */
typedef enum {
    RTE_BSW_DIAG_SESSION_DEFAULT = 0x01,
    RTE_BSW_DIAG_SESSION_PROGRAMMING = 0x02,
    RTE_BSW_DIAG_SESSION_EXTENDED = 0x03,
    RTE_BSW_DIAG_SESSION_SAFETY = 0x04
} Rte_Bsw_Diag_SessionType;

/**
 * @brief Diagnostic Security Level Type
 */
typedef uint8 Rte_Bsw_Diag_SecurityLevelType;

/**
 * @brief Diagnostic Interface Functions
 */
#define RTE_START_SEC_CODE
#include "MemMap.h"

extern Rte_StatusType Rte_Bsw_Dcm_GetSecurityLevel(Rte_Bsw_Diag_SecurityLevelType* level);
extern Rte_StatusType Rte_Bsw_Dcm_GetSessionLevel(Rte_Bsw_Diag_SessionType* session);
extern Rte_StatusType Rte_Bsw_Dcm_ResetToDefaultSession(void);
extern Rte_StatusType Rte_Bsw_Dem_SetEventStatus(uint16 eventId, uint8 status);
extern Rte_StatusType Rte_Bsw_Dem_ResetEventStatus(uint16 eventId);
extern Rte_StatusType Rte_Bsw_Dem_GetStatusOfDTC(uint32 dtc, uint8* status);
extern Rte_StatusType Rte_Bsw_Dem_ClearDTC(uint32 dtc);
extern Rte_StatusType Rte_Bsw_Dem_DisableDTCSetting(uint32 dtcGroup);
extern Rte_StatusType Rte_Bsw_Dem_EnableDTCSetting(uint32 dtcGroup);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE BSW COMMUNICATION INTERFACE
==================================================================================================*/

/**
 * @brief Communication Mode Type
 */
typedef enum {
    RTE_BSW_COM_MODE_NO_COM = 0,
    RTE_BSW_COM_MODE_SILENT_COM,
    RTE_BSW_COM_MODE_FULL_COM
} Rte_Bsw_Com_ModeType;

/**
 * @brief Communication Interface Functions
 */
#define RTE_START_SEC_CODE
#include "MemMap.h"

extern Rte_StatusType Rte_Bsw_Com_SendSignal(uint16 signalId, const void* data);
extern Rte_StatusType Rte_Bsw_Com_ReceiveSignal(uint16 signalId, void* data);
extern Rte_StatusType Rte_Bsw_Com_SendSignalGroup(uint16 signalGroupId);
extern Rte_StatusType Rte_Bsw_Com_ReceiveSignalGroup(uint16 signalGroupId);
extern Rte_StatusType Rte_Bsw_Com_InvalidateSignal(uint16 signalId);
extern Rte_StatusType Rte_Bsw_Com_InvalidateSignalGroup(uint16 signalGroupId);
extern Rte_StatusType Rte_Bsw_Com_SwitchIpduTxMode(uint16 pduId, uint8 mode);
extern Rte_StatusType Rte_Bsw_Com_TriggerIpduSend(uint16 pduId);
extern Rte_StatusType Rte_Bsw_Com_TriggerIpduSendWithMetaData(uint16 pduId, const uint8* metaData);
extern Rte_StatusType Rte_Bsw_PduR_Transmit(PduIdType pduId, const PduInfoType* pduInfo);
extern Rte_StatusType Rte_Bsw_PduR_CancelTransmit(PduIdType pduId);
extern Rte_StatusType Rte_Bsw_PduR_CancelReceive(PduIdType pduId);
extern Rte_StatusType Rte_Bsw_CanIf_SetControllerMode(uint8 controller, uint8 mode);
extern Rte_StatusType Rte_Bsw_CanIf_GetControllerMode(uint8 controller, uint8* mode);
extern Rte_StatusType Rte_Bsw_CanIf_Transmit(PduIdType pduId, const PduInfoType* pduInfo);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE BSW STORAGE INTERFACE
==================================================================================================*/

/**
 * @brief Storage Block Status Type
 */
typedef enum {
    RTE_BSW_NVM_BLOCK_STATUS_OK = 0,
    RTE_BSW_NVM_BLOCK_STATUS_PENDING,
    RTE_BSW_NVM_BLOCK_STATUS_ERROR
} Rte_Bsw_Nvm_BlockStatusType;

/**
 * @brief Storage Interface Functions
 */
#define RTE_START_SEC_CODE
#include "MemMap.h"

extern Rte_StatusType Rte_Bsw_Nvm_ReadBlock(uint16 blockId, void* data);
extern Rte_StatusType Rte_Bsw_Nvm_WriteBlock(uint16 blockId, const void* data);
extern Rte_StatusType Rte_Bsw_Nvm_RestoreBlockDefaults(uint16 blockId, void* data);
extern Rte_StatusType Rte_Bsw_Nvm_EraseNvBlock(uint16 blockId);
extern Rte_StatusType Rte_Bsw_Nvm_InvalidateNvBlock(uint16 blockId);
extern Rte_StatusType Rte_Bsw_Nvm_ReadPRAMBlock(uint16 blockId);
extern Rte_StatusType Rte_Bsw_Nvm_WritePRAMBlock(uint16 blockId);
extern Rte_StatusType Rte_Bsw_Nvm_CancelJobs(uint16 blockId);
extern Rte_StatusType Rte_Bsw_Nvm_SetRamBlockStatus(uint16 blockId, boolean status);
extern Rte_StatusType Rte_Bsw_Nvm_GetErrorStatus(uint16 blockId, uint8* status);
extern Rte_StatusType Rte_Bsw_Nvm_SetDataIndex(uint16 blockId, uint8 index);
extern Rte_StatusType Rte_Bsw_Nvm_GetDataIndex(uint16 blockId, uint8* index);
extern Rte_StatusType Rte_Bsw_Fee_Read(uint16 blockNumber, uint16 offset, uint8* data, uint16 length);
extern Rte_StatusType Rte_Bsw_Fee_Write(uint16 blockNumber, const uint8* data);
extern Rte_StatusType Rte_Bsw_Ea_Read(uint16 blockNumber, uint16 offset, uint8* data, uint16 length);
extern Rte_StatusType Rte_Bsw_Ea_Write(uint16 blockNumber, const uint8* data);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE BSW IO CONTROL INTERFACE
==================================================================================================*/

/**
 * @brief IO Channel Type
 */
typedef uint16 Rte_Bsw_Io_ChannelType;

/**
 * @brief IO Value Type
 */
typedef uint16 Rte_Bsw_Io_ValueType;

/**
 * @brief IO Control Interface Functions
 */
#define RTE_START_SEC_CODE
#include "MemMap.h"

extern Rte_StatusType Rte_Bsw_IoHwAb_AnalogRead(Rte_Bsw_Io_ChannelType channel, Rte_Bsw_Io_ValueType* value);
extern Rte_StatusType Rte_Bsw_IoHwAb_AnalogWrite(Rte_Bsw_Io_ChannelType channel, Rte_Bsw_Io_ValueType value);
extern Rte_StatusType Rte_Bsw_IoHwAb_DigitalRead(Rte_Bsw_Io_ChannelType channel, boolean* value);
extern Rte_StatusType Rte_Bsw_IoHwAb_DigitalWrite(Rte_Bsw_Io_ChannelType channel, boolean value);
extern Rte_StatusType Rte_Bsw_IoHwAb_PwmSetDuty(Rte_Bsw_Io_ChannelType channel, uint16 duty);
extern Rte_StatusType Rte_Bsw_IoHwAb_PwmSetFreqAndDuty(Rte_Bsw_Io_ChannelType channel, uint32 freq, uint16 duty);
extern Rte_StatusType Rte_Bsw_IoHwAb_SpiTransfer(uint8 deviceId, const uint8* txData, uint8* rxData, uint16 length);
extern Rte_StatusType Rte_Bsw_Adc_StartGroupConversion(uint8 group);
extern Rte_StatusType Rte_Bsw_Adc_StopGroupConversion(uint8 group);
extern Rte_StatusType Rte_Bsw_Adc_ReadGroup(uint8 group, uint16* data);
extern Rte_StatusType Rte_Bsw_Pwm_SetDutyCycle(uint8 channel, uint16 duty);
extern Rte_StatusType Rte_Bsw_Pwm_SetPeriodAndDuty(uint8 channel, uint32 period, uint16 duty);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE BSW MODE MANAGER INTERFACE
==================================================================================================*/

/**
 * @brief BSW Mode Type
 */
typedef uint32 Rte_Bsw_ModeType;

/**
 * @brief Mode Manager Interface Functions
 */
#define RTE_START_SEC_CODE
#include "MemMap.h"

extern Rte_StatusType Rte_Bsw_BswM_RequestMode(uint8 module, uint8 mode);
extern Rte_StatusType Rte_Bsw_BswM_GetMode(uint8 module, uint8* mode);
extern Rte_StatusType Rte_Bsw_BswM_SelectApplicationMode(Rte_Bsw_ModeType mode);
extern Rte_StatusType Rte_Bsw_BswM_GetApplicationMode(Rte_Bsw_ModeType* mode);
extern void Rte_Bsw_BswM_MainFunction(void);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE BSW WATCHDOG INTERFACE
==================================================================================================*/

/**
 * @brief Watchdog Interface Functions
 */
#define RTE_START_SEC_CODE
#include "MemMap.h"

extern Rte_StatusType Rte_Bsw_WdgM_CheckpointReached(uint8 supervisedEntityId, uint16 checkpointId);
extern Rte_StatusType Rte_Bsw_WdgM_GetGlobalStatus(uint8* status);
extern Rte_StatusType Rte_Bsw_WdgM_GetLocalStatus(uint8 supervisedEntityId, uint8* status);
extern Rte_StatusType Rte_Bsw_WdgM_PerformReset(void);
extern Rte_StatusType Rte_Bsw_WdgM_SetMode(uint8 mode);
extern void Rte_Bsw_Wdg_Trigger(void);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE BSW CALLBACK FUNCTIONS
==================================================================================================*/

/**
 * @brief BSW Callback Function Types
 */
typedef void (*Rte_Bsw_JobEndNotificationType)(void);
typedef void (*Rte_Bsw_JobErrorNotificationType)(void);
typedef void (*Rte_Bsw_ModeNotificationType)(uint8 mode);
typedef void (*Rte_Bsw_EventNotificationType)(uint8 event);

/**
 * @brief BSW Callback Registration Functions
 */
#define RTE_START_SEC_CODE
#include "MemMap.h"

extern Rte_StatusType Rte_Bsw_Register_JobEndNotification(Rte_Bsw_JobEndNotificationType callback);
extern Rte_StatusType Rte_Bsw_Register_JobErrorNotification(Rte_Bsw_JobErrorNotificationType callback);
extern Rte_StatusType Rte_Bsw_Register_ModeNotification(uint8 module, Rte_Bsw_ModeNotificationType callback);
extern Rte_StatusType Rte_Bsw_Register_EventNotification(uint8 module, Rte_Bsw_EventNotificationType callback);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE BSW MACRO DEFINITIONS
==================================================================================================*/

/**
 * @brief Macro to call BSW service
 */
#define RTE_BSW_CALL(service, ...) \
    Rte_Bsw_##service(__VA_ARGS__)

/**
 * @brief Macro to read BSW data
 */
#define RTE_BSW_READ(service, data) \
    Rte_Bsw_##service##_Read(data)

/**
 * @brief Macro to write BSW data
 */
#define RTE_BSW_WRITE(service, data) \
    Rte_Bsw_##service##_Write(data)

/**
 * @brief Macro to switch BSW mode
 */
#define RTE_BSW_MODE_SWITCH(module, mode) \
    Rte_Bsw_BswM_RequestMode(module, mode)

/**
 * @brief Macro to trigger watchdog
 */
#define RTE_BSW_WDG_TRIGGER() \
    Rte_Bsw_Wdg_Trigger()

/**
 * @brief Macro to reach watchdog checkpoint
 */
#define RTE_BSW_WDG_CHECKPOINT(seId, cpId) \
    Rte_Bsw_WdgM_CheckpointReached(seId, cpId)

#endif /* RTE_BSW_H */
