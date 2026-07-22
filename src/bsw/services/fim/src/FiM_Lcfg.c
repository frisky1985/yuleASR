/** @file FiM_Lcfg.c @brief FiM Link-Time Configuration */
#include "FiM.h"
#include "FiM_Cfg.h"
extern const FiM_ConfigType* const FiM_ConfigPtr;
const FiM_ConfigType FiM_Config = { 0U };
const FiM_ConfigType* const FiM_ConfigPtr = &FiM_Config;
