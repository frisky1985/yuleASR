/******************************************************************************
 * @file    dcm.h
 * @brief   DCM (Diagnostic Communication Manager) Main Interface
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DCM_H
#define DCM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dcm_types.h"
#include "dcm_session.h"
#include "dcm_ecu_reset.h"
#include "dcm_security.h"
#include "dcm_communication.h"
#include "dcm_dynamic_did.h"
#include "dcm_memory.h"
#include "dcm_routine.h"

/******************************************************************************
 * DCM Version Information
 ******************************************************************************/
#define DCM_MAJOR_VERSION                       1U
#define DCM_MINOR_VERSION                       0U
#define DCM_PATCH_VERSION                       0U

/******************************************************************************
 * DCM Configuration
 ******************************************************************************/
typedef struct {
    /* Protocol Configuration */
    const Dcm_ProtocolConfigType *protocolConfigs;
    uint8_t numProtocols;
    
    /* Session Configuration */
    const Dcm_SessionControlConfigType *sessionConfig;
    
    /* ECU Reset Configuration */
    const Dcm_EcuResetConfigType *ecuResetConfig;
    
    /* Security Access Configuration */
    const Dcm_SecurityAccessConfigType *securityConfig;
    
    /* Communication Control Configuration */
    const Dcm_CommunicationControlConfigType *commControlConfig;
    
    /* Dynamic DID Configuration */
    const Dcm_DynamicDidConfigType *dynamicDidConfig;
    
    /* Memory Write Configuration */
    const Dcm_MemoryWriteConfigType *memoryWriteConfig;
    
    /* Routine Control Configuration */
    const Dcm_RoutineConfigType *routineConfigs;
    uint8_t numRoutines;
} Dcm_ConfigType;

/******************************************************************************
 * DCM Initialization and Main Functions
 ******************************************************************************/

/**
 * @brief Initialize DCM module
 *
 * @param config Pointer to DCM configuration
 * @return Dcm_ReturnType Initialization result
 */
Dcm_ReturnType Dcm_Init(const Dcm_ConfigType *config);

/**
 * @brief Deinitialize DCM module
 *
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_DeInit(void);

/**
 * @brief Main function - process pending operations
 *
 * @param elapsedTimeMs Time elapsed since last call in milliseconds
 *
 * @note Should be called periodically (e.g., every 10ms)
 */
void Dcm_MainFunction(uint32_t elapsedTimeMs);

/******************************************************************************
 * Service Processing Functions
 ******************************************************************************/

/**
 * @brief Process incoming diagnostic request
 *
 * @param request Pointer to request message
 * @param response Pointer to response message buffer
 * @return Dcm_ReturnType Processing result
 */
Dcm_ReturnType Dcm_ProcessRequest(const Dcm_RequestType *request,
                                  Dcm_ResponseType *response);

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

/**
 * @brief Get DCM module status
 *
 * @return bool True if initialized
 */
bool Dcm_IsInitialized(void);

/**
 * @brief Get DCM state
 *
 * @return Dcm_StateType Current DCM state
 */
Dcm_StateType Dcm_GetState(void);

/**
 * @brief Get version information
 *
 * @param version Pointer to version structure
 */
void Dcm_GetVersionInfo(uint8_t *major, uint8_t *minor, uint8_t *patch);

#ifdef __cplusplus
}
#endif

#endif /* DCM_H */
