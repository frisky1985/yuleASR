/** @file LinM_Lcfg.c @brief LinM Link-Time Configuration */
#include "LinM.h"
#include "LinM_Cfg.h"
extern const LinM_ConfigType* const LinM_ConfigPtr;
const LinM_ConfigType LinM_Config = { 0U };
const LinM_ConfigType* const LinM_ConfigPtr = &LinM_Config;
