/**
 * @file BswM_Lcfg.c
 * @brief BswM Configuration Tables
 */

#include "BswM.h"
#include "BswM_Cfg.h"

/* Mode Request Ports */
extern const BswM_ConfigType BswM_Config;
static BswM_ModeRequestPortType BswM_ModeRequestPorts[BSWM_MAX_MODE_REQUEST_PORTS] = {
    { 0U, BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_STARTUP, TRUE },
    { 1U, BSWM_COMM_REQUEST, BSWM_MODE_VALUE_RUN, TRUE },
    { 2U, BSWM_DCM_REQUEST, BSWM_MODE_VALUE_RUN, FALSE }
};

/* Rules */
static const BswM_RuleType BswM_Rules[BSWM_MAX_RULES] = {
    { 0U, 0U, BSWM_MODE_VALUE_RUN, 0U, TRUE },
    { 1U, 1U, BSWM_MODE_VALUE_SHUTDOWN, 1U, FALSE }
};

/* Configuration */
static const BswM_ConfigType BswM_Config = {
    .NumModeRequestPorts = 3U,
    .NumRules = 2U,
    .NumActionLists = 1U,
    .ModeRequestPorts = BswM_ModeRequestPorts,
    .Rules = BswM_Rules
};