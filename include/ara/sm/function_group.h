/******************************************************************************
 * @file    function_group.h
 * @brief   Function Group Manager Header
 *
 * AUTOSAR Adaptive Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef SM_FUNCTION_GROUP_H
#define SM_FUNCTION_GROUP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sm_internal.h"
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * Configuration Constants
 ******************************************************************************/

#define FG_MAX_MEMBERS                  16U
#define FG_MAX_NAME_LENGTH              32U
#define FG_TRANSITION_TIMEOUT_MS        5000U

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* Member types */
typedef enum {
    FG_MEMBER_PROCESS = 0,
    FG_MEMBER_SERVICE,
    FG_MEMBER_CALLBACK
} FG_MemberType;

/* Member definition */
typedef struct {
    FG_MemberType type;
    char name[FG_MAX_NAME_LENGTH];
    uint32_t id;
    bool isActive;
    bool isRequired;
    void* context;
} FG_MemberType;

/* Function group configuration */
typedef struct {
    FunctionGroupNameType name;
    char displayName[FG_MAX_NAME_LENGTH];
    FG_MemberType members[FG_MAX_MEMBERS];
    uint32_t memberCount;
    uint32_t transitionTimeoutMs;
    bool autoRestartMembers;
} FG_ConfigType;

/* Function group runtime data */
typedef struct {
    FunctionGroupNameType name;
    FunctionGroupStateType currentState;
    FunctionGroupStateType targetState;
    bool transitionInProgress;
    uint32_t transitionStartTime;
    uint32_t activeMemberCount;
    bool isStable;
} FG_RuntimeType;

/******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/* Initialize function group manager */
Std_ReturnType FG_Init(void);

/* Deinitialize function group manager */
Std_ReturnType FG_Deinit(void);

/* Check if initialized */
bool FG_IsInitialized(void);

/* Create function group configuration */
Std_ReturnType FG_CreateConfig(FunctionGroupNameType name, FG_ConfigType* config);

/* Add member to function group */
Std_ReturnType FG_AddMember(FG_ConfigType* config, const FG_MemberType* member);

/* Remove member from function group */
Std_ReturnType FG_RemoveMember(FG_ConfigType* config, const char* memberName);

/* Get member by name */
Std_ReturnType FG_GetMember(const FG_ConfigType* config, const char* memberName, 
                            FG_MemberType* member);

/* Configure function group */
Std_ReturnType FG_Configure(FunctionGroupNameType name, const FG_ConfigType* config);

/* Start function group (transition to ON) */
Std_ReturnType FG_Start(FunctionGroupNameType name);

/* Stop function group (transition to OFF) */
Std_ReturnType FG_Stop(FunctionGroupNameType name);

/* Get function group runtime state */
Std_ReturnType FG_GetRuntimeState(FunctionGroupNameType name, FG_RuntimeType* runtime);

/* Check if function group is stable */
bool FG_IsStable(FunctionGroupNameType name);

/* Wait for function group to reach stable state */
Std_ReturnType FG_WaitForStable(FunctionGroupNameType name, uint32_t timeoutMs);

/* Notify member state change */
Std_ReturnType FG_NotifyMemberState(FunctionGroupNameType name, const char* memberName,
                                     bool isActive);

/* Get active member count */
uint32_t FG_GetActiveMemberCount(FunctionGroupNameType name);

/* Function group main function (call periodically) */
void FG_MainFunction(void);

#ifdef __cplusplus
}
#endif

#endif /* SM_FUNCTION_GROUP_H */
