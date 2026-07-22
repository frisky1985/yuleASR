/** @file SchM_Lcfg.c @brief SchM Link-Time Configuration */
#include "SchM.h"
#include "SchM_Cfg.h"
extern const SchM_ConfigType* const SchM_ConfigPtr;
extern const SchM_ConfigType SchM_Config;
const SchM_ConfigType SchM_Config = { 0U };
const SchM_ConfigType* const SchM_ConfigPtr = &SchM_Config;
