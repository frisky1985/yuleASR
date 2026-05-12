/**
 * @file Eep_Lcfg.c
 * @brief EEPROM Link-Time Configuration
 */

#include "Eep.h"
#include "Eep_Cfg.h"

const Eep_ConfigType Eep_Config = {
    .BaseAddress = EEP_BASE_ADDRESS,
    .Size = EEP_SIZE,
    .JobCallCycle = EEP_JOB_CALL_CYCLE
};
