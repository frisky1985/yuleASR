/**
 * @file BswM.h
 * @brief BSW Mode Manager - AUTOSAR Services Module
 * @version 2.0.0
 * @date 2026-07-19
 * @author YuleTech
 *
 * @implements AUTOSAR_SWS_BSWModeManager.pdf
 */

#ifndef BSWM_H
#define BSWM_H

#include "Std_Types.h"
#include "BswM_Cfg.h"

#define BSWM_AR_RELEASE_MAJOR_VERSION   4U
#define BSWM_AR_RELEASE_MINOR_VERSION   4U
#define BSWM_AR_RELEASE_REVISION_VERSION 0U
#define BSWM_SW_MAJOR_VERSION           1U
#define BSWM_SW_MINOR_VERSION           0U
#define BSWM_SW_PATCH_VERSION           0U
#define BSWM_MODULE_ID              0x12U
#define BSWM_VENDOR_ID              0x0055U

#define BSWM_ECUM_REQUEST           0x01U
#define BSWM_COMM_REQUEST           0x02U
#define BSWM_DCM_REQUEST            0x03U
#define BSWM_NM_REQUEST             0x04U
#define BSWM_SCHM_REQUEST           0x05U

/* BswM Mode values - use enums to avoid macro conflicts */
#define BSWM_MODE_VALUE_OFF         0U
#define BSWM_MODE_VALUE_START       1U
#define BSWM_MODE_VALUE_RUN         2U
#define BSWM_MODE_VALUE_POST_RUN    3U
#define BSWM_MODE_VALUE_SLEEP       4U
#define BSWM_MODE_VALUE_SHUTDOWN    5U
#define BSWM_MODE_VALUE_WAKEUP      6U
#define BSWM_MODE_VALUE_STARTUP     7U

typedef uint8 BswM_ModeType;

typedef void (*BswM_ActionCallback)(BswM_ModeType Mode);

typedef struct {
    uint8  CompositionId;
    uint8  RequestSourceId;
    BswM_ModeType RequestedMode;
    boolean IsActive;
} BswM_ModeRequestPortType;

typedef struct {
    uint8 RuleId;
    uint8 ModeRequestPortIndex;
    BswM_ModeType TargetMode;
    uint8 Priority;
    boolean IsEnabled;
} BswM_RuleType;

typedef struct {
    uint8 ActionListId;
    uint8 RuleId;
    uint8 NumActions;
    BswM_ActionCallback* Actions;
} BswM_ActionListType;

typedef struct {
    uint8 NumModeRequestPorts;
    uint8 NumRules;
    uint8 NumActionLists;
    const BswM_ModeRequestPortType* ModeRequestPorts;
    const BswM_RuleType* Rules;
    const BswM_ActionListType* ActionLists;
} BswM_ConfigType;

/** @req SWS_BswM_00001 */
void BswM_Init(const BswM_ConfigType* ConfigPtr);
/** @req SWS_BswM_00002 */
void BswM_DeInit(void);
/** @req SWS_BswM_00010 */
Std_ReturnType BswM_RequestMode(uint8 SwCompositionId, BswM_ModeType Mode);
/** @req SWS_BswM_00011 */
BswM_ModeType BswM_GetCurrentMode(void);
/** @req SWS_BswM_00012 */
BswM_ModeType BswM_GetRequestedMode(void);
/** @req SWS_BswM_00020 */
void BswM_MainFunction(void);
/** @req SWS_BswM_00030 */
void BswM_GetVersionInfo(Std_VersionInfoType* versioninfo);

#endif /* BSWM_H */