/**
 * @file Swc_DiagnosticManager.h
 * @brief Diagnostic Manager Software Component Header
 * @version 1.0.0
 * @date 2026-04-19
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Classic Platform 4.x - Application Software Component
 * Purpose: Diagnostic services and DTC management at application layer
 */

#ifndef SWC_DIAGNOSTICMANAGER_H
#define SWC_DIAGNOSTICMANAGER_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Rte_Swc.h"
#include "Std_Types.h"

/*==================================================================================================
*                                    COMPONENT TYPE DEFINITIONS
==================================================================================================*/

/**
 * @brief Diagnostic session type
 */
typedef enum {
    DIAG_SESSION_DEFAULT = 0x01,
    DIAG_SESSION_PROGRAMMING = 0x02,
    DIAG_SESSION_EXTENDED = 0x03,
    DIAG_SESSION_SAFETY_SYSTEM = 0x04
} Swc_DiagnosticSessionType;

/**
 * @brief Security access level
 */
typedef enum {
    SECURITY_LOCKED = 0,
    SECURITY_LEVEL_1,           /* Customer level */
    SECURITY_LEVEL_2,           /* Engineering level */
    SECURITY_LEVEL_3            /* Manufacturer level */
} Swc_SecurityLevelType;

/**
 * @brief Diagnostic request structure
 */
typedef struct {
    uint8 serviceId;
    uint8 subFunction;
    uint8 dataLength;
    uint8 data[256];
} Swc_DiagnosticRequestType;

/**
 * @brief Diagnostic response structure
 */
typedef struct {
    uint8 responseId;
    uint8 dataLength;
    uint8 data[256];
    uint8 negativeResponseCode;
} Swc_DiagnosticResponseType;

/**
 * @brief DTC status structure
 */
typedef struct {
    uint32 dtcCode;
    uint8 statusByte;
    uint8 faultDetectionCounter;
    uint8 occurrenceCounter;
    uint32 agingCounter;
    uint32 lastOccurrenceTime;
} Swc_DtcStatusType;

/**
 * @brief Diagnostic manager status
 */
typedef struct {
    Swc_DiagnosticSessionType currentSession;
    Swc_SecurityLevelType securityLevel;
    uint8 activeProtocol;
    boolean communicationEnabled;
    uint32 sessionTimeout;
    uint32 securityTimeout;
} Swc_DiagnosticManagerStatusType;

/*==================================================================================================
*                                    RUNNABLE IDS
==================================================================================================*/
#define RTE_RUNNABLE_DiagnosticManager_Init          0x21
#define RTE_RUNNABLE_DiagnosticManager_50ms          0x22
#define RTE_RUNNABLE_DiagnosticManager_ProcessReq    0x23

/*==================================================================================================
*                                    PORT IDS
==================================================================================================*/
#define SWC_DIAGNOSTICMANAGER_PORT_SESSION_P         0x21
#define SWC_DIAGNOSTICMANAGER_PORT_SECURITY_P        0x22
#define SWC_DIAGNOSTICMANAGER_PORT_DTC_STATUS_P      0x23
#define SWC_DIAGNOSTICMANAGER_PORT_DIAG_REQUEST_R    0x24
#define SWC_DIAGNOSTICMANAGER_PORT_DIAG_RESPONSE_P   0x25

/*==================================================================================================
*                                    COMPONENT API
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the Diagnostic Manager component
 */
extern void Swc_DiagnosticManager_Init(void);

/**
 * @brief 50ms cyclic runnable - diagnostic management
 */
extern void Swc_DiagnosticManager_50ms(void);

/**
 * @brief Process diagnostic request runnable
 */
extern void Swc_DiagnosticManager_ProcessRequest(void);

/**
 * @brief Changes diagnostic session
 * @param session Target session
 * @return RTE status
 */
extern Rte_StatusType Swc_DiagnosticManager_ChangeSession(Swc_DiagnosticSessionType session);

/**
 * @brief Gets current diagnostic session
 * @param session Pointer to store session
 * @return RTE status
 */
extern Rte_StatusType Swc_DiagnosticManager_GetSession(Swc_DiagnosticSessionType* session);

/**
 * @brief Unlocks security level
 * @param level Target security level
 * @param key Security key
 * @return RTE status
 */
extern Rte_StatusType Swc_DiagnosticManager_UnlockSecurity(Swc_SecurityLevelType level,
                                                            const uint8* key);

/**
 * @brief Gets security level
 * @param level Pointer to store level
 * @return RTE status
 */
extern Rte_StatusType Swc_DiagnosticManager_GetSecurityLevel(Swc_SecurityLevelType* level);

/**
 * @brief Processes diagnostic request
 * @param request Diagnostic request
 * @param response Diagnostic response
 * @return RTE status
 */
extern Rte_StatusType Swc_DiagnosticManager_ProcessDiagnosticRequest(
    const Swc_DiagnosticRequestType* request,
    Swc_DiagnosticResponseType* response);

/**
 * @brief Gets DTC status
 * @param dtcCode DTC code
 * @param status Pointer to store status
 * @return RTE status
 */
extern Rte_StatusType Swc_DiagnosticManager_GetDtcStatus(uint32 dtcCode,
                                                          Swc_DtcStatusType* status);

/**
 * @brief Clears DTC
 * @param dtcCode DTC code (0xFFFFFF for all)
 * @return RTE status
 */
extern Rte_StatusType Swc_DiagnosticManager_ClearDtc(uint32 dtcCode);

/**
 * @brief Gets diagnostic manager status
 * @param status Pointer to store status
 * @return RTE status
 */
extern Rte_StatusType Swc_DiagnosticManager_GetStatus(Swc_DiagnosticManagerStatusType* status);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE INTERFACE MACROS
==================================================================================================*/
#define Rte_Write_DiagnosticSession(data) \
    Rte_Write_SWC_DIAGNOSTICMANAGER_PORT_SESSION_P(data)

#define Rte_Write_SecurityLevel(data) \
    Rte_Write_SWC_DIAGNOSTICMANAGER_PORT_SECURITY_P(data)

#define Rte_Write_DtcStatus(data) \
    Rte_Write_SWC_DIAGNOSTICMANAGER_PORT_DTC_STATUS_P(data)

#define Rte_Read_DiagnosticRequest(data) \
    Rte_Read_SWC_DIAGNOSTICMANAGER_PORT_DIAG_REQUEST_R(data)

#define Rte_Write_DiagnosticResponse(data) \
    Rte_Write_SWC_DIAGNOSTICMANAGER_PORT_DIAG_RESPONSE_P(data)

#endif /* SWC_DIAGNOSTICMANAGER_H */
