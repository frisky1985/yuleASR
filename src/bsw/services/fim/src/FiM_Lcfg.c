/** @file FiM_Lcfg.c @brief FiM Link-Time Configuration */
/* @req SWS_FiM_00001 @req SWS_FiM_00002 @req SWS_FiM_00003 */

#include "FiM.h"
#include "FiM_Cfg.h"
extern const FiM_ConfigType* const FiM_ConfigPtr;
const FiM_ConfigType FiM_Config = { 0U };
const FiM_ConfigType* const FiM_ConfigPtr = &FiM_Config;
