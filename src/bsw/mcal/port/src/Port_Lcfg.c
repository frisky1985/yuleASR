/** @file Port_Lcfg.c @brief Port Link-Time Configuration */
#include "Port.h"
#include "Port_Cfg.h"
extern const Port_ConfigType* const Port_ConfigPtr;
const Port_ConfigType Port_Config = { 0U };
const Port_ConfigType* const Port_ConfigPtr = &Port_Config;
