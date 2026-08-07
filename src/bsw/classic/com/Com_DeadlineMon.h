/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/*
 * Com_DeadlineMon.h
 * AUTOSAR COM Module - Deadline Monitoring (死线监控)
 * According to AUTOSAR SWS COM 4.4.0 - SWS_Com_00500
 *
 * T012: 死线监控实现
 * - 接收超时检测和错误处理
 * - ComIPduRxTimeout 配置参数支持
 * - ComErrorHook 回调机制
 * - ComIPduRxDefaultValue 默认值替代
 *
 * ASIL-D Safety Level - 需要错误检测和冗余检查
 */

#ifndef COM_DEADLINEMON_H
#define COM_DEADLINEMON_H

/*==================[Includes]=============================================*/

#include "Com_Private.h"

/*==================[Version Information]==================================*/

#define COM_DM_SW_MAJOR_VERSION   0x01u
#define COM_DM_SW_MINOR_VERSION   0x00u
#define COM_DM_SW_PATCH_VERSION   0x00u

/*==================[Service IDs]==========================================*/

#define COM_SERVICE_ID_DM_INIT              0x20u
#define COM_SERVICE_ID_DM_DEINIT            0x21u
#define COM_SERVICE_ID_DM_START_TIMER       0x22u
#define COM_SERVICE_ID_DM_STOP_TIMER        0x23u
#define COM_SERVICE_ID_DM_CHECK_TIMEOUT     0x24u
#define COM_SERVICE_ID_DM_HANDLE_TIMEOUT    0x25u
#define COM_SERVICE_ID_DM_PROCESS           0x26u
#define COM_SERVICE_ID_DM_RX_INDICATION     0x27u

/*==================[ASIL-D Safety Error Codes]============================*/

#define COM_E_DM_INVALID_TIMER_VALUE        0x30u
#define COM_E_DM_TIMER_OVERFLOW             0x31u
#define COM_E_DM_INVALID_PDU_ID             0x32u
#define COM_E_DM_RUNTIME_CORRUPTION         0x33u

/*==================[Type Definitions]=====================================*/

/**
 * @brief Deadline Monitoring Runtime Data
 *
 * Runtime state for deadline monitoring
 * Note: Com_DmStateType, Com_DmActionType, Com_DmRxConfigType
 * are defined in Com_Types.h
 */
typedef struct {
    uint32 Timer;               /*!< Current timeout countdown value */
    Com_DmStateType State;      /*!< Current DM state */
    uint32 TimeoutCounter;      /*!< Number of timeouts occurred (diagnostic) */
    boolean TimeoutProcessed;   /*!< Timeout has been processed this cycle */
} Com_DmRunTimeType;

/*==================[Global Variables]=====================================*/

/* Deadline monitoring runtime data array */
extern Com_DmRunTimeType Com_DmRunTimeData[COM_MAX_IPDUS];

/* Deadline monitoring module initialized flag */
extern boolean Com_DmInitialized;

/* ASIL-D: Redundant state for safety check */
extern boolean Com_DmInitialized_Redundant;

/*==================[API Functions]========================================*/

/**
 * @brief Initialize Deadline Monitoring module
 *
 * Called during Com_Init to initialize all deadline monitoring
 * timers and state machines.
 *
 * @req SWS_Com_00500
 * @ASIL-D: Safety critical initialization with redundancy check
 */
extern void Com_Dm_Init(void);

/**
 * @brief De-initialize Deadline Monitoring module
 *
 * Resets all deadline monitoring state.
 */
extern void Com_Dm_DeInit(void);

/**
 * @brief Start deadline monitoring timer for an Rx I-PDU
 *
 * Called when a PDU is received (PduR_ComRxIndication) to
 * restart the timeout timer.
 *
 * @param PduId I-PDU identifier
 * @param Timeout Timeout value in ms (0 = use configured value)
 *
 * @req SWS_Com_00500
 * @ASIL-D: Dual-check for timer value validity
 */
extern void Com_Dm_StartTimer(Com_IPduIdType PduId, uint32 Timeout);

/**
 * @brief Stop deadline monitoring timer for an Rx I-PDU
 *
 * Stops the monitoring timer without triggering timeout action.
 *
 * @param PduId I-PDU identifier
 */
extern void Com_Dm_StopTimer(Com_IPduIdType PduId);

/**
 * @brief Process deadline monitoring in Com_MainFunctionRx
 *
 * Checks all Rx I-PDUs for timeout conditions and handles them
 * according to configuration.
 *
 * @req SWS_Com_00500
 * @ASIL-D: Redundant state validation
 */
extern void Com_Dm_ProcessTimers(void);

/**
 * @brief Handle Rx indication - start/restart timer
 *
 * Called from PduR_ComRxIndication to handle deadline monitoring
 * for received PDUs.
 *
 * @param PduId I-PDU identifier
 * @param DmConfig Pointer to DM configuration
 *
 * @req SWS_Com_00500
 */
extern void Com_Dm_HandleRxIndication(Com_IPduIdType PduId, 
                                       const Com_DmRxConfigType* DmConfig);

/**
 * @brief Handle timeout occurrence
 *
 * Executes the configured timeout action (ErrorHook, default value, etc.)
 *
 * @param PduId I-PDU identifier
 * @param DmConfig Pointer to DM configuration
 *
 * @ASIL-D: Redundant execution check
 */
extern void Com_Dm_HandleTimeout(Com_IPduIdType PduId,
                                  const Com_DmRxConfigType* DmConfig);

/**
 * @brief Apply default value substitution
 *
 * Copies the configured default value to the I-PDU buffer.
 *
 * @param PduId I-PDU identifier
 * @param DmConfig Pointer to DM configuration
 *
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Com_Dm_ApplyDefaultValue(Com_IPduIdType PduId,
                                                const Com_DmRxConfigType* DmConfig);

/**
 * @brief Get deadline monitoring state for an I-PDU
 *
 * @param PduId I-PDU identifier
 * @return Current DM state
 */
extern Com_DmStateType Com_Dm_GetState(Com_IPduIdType PduId);

/**
 * @brief Check if deadline monitoring is initialized
 *
 * @return TRUE if initialized, FALSE otherwise
 */
extern boolean Com_Dm_IsInitialized(void);

/**
 * @brief ASIL-D Safety Check: Validate runtime integrity
 *
 * Performs integrity checks on deadline monitoring runtime data.
 *
 * @return E_OK if integrity check passed, E_NOT_OK otherwise
 */
extern Std_ReturnType Com_Dm_ValidateIntegrity(void);

/*==================[Integration Macros]===================================*/

/**
 * @brief Macro to integrate deadline monitoring in Com_MainFunctionRx
 *
 * Usage: Add COM_DM_PROCESS_IN_MAINFUNCTIONRX() at the start of
 * Com_MainFunctionRx processing loop.
 */
#define COM_DM_PROCESS_IN_MAINFUNCTIONRX() \
    do { \
        if (Com_Dm_IsInitialized() == TRUE) { \
            Com_Dm_ProcessTimers(); \
        } \
    } while(0)

/**
 * @brief Macro to integrate deadline monitoring in RxIndication
 *
 * Usage: Call COM_DM_HANDLE_RX_INDICATION(PduId, DmConfig) when
 * a PDU is received.
 */
#define COM_DM_HANDLE_RX_INDICATION(PduId, DmConfig) \
    do { \
        if (Com_Dm_IsInitialized() && (DmConfig) != NULL_PTR && \
            (DmConfig)->EnableDeadlineMonitoring) { \
            Com_Dm_HandleRxIndication((PduId), (DmConfig)); \
        } \
    } while(0)

/*==================[End of File]==========================================*/

#endif /* COM_DEADLINEMON_H */
