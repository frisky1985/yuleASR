/** @file Can_Lcfg.c @brief Can Link-Time Configuration */
/* @req SWS_Can_00001 @req SWS_Can_00002 @req SWS_Can_00003 */

#include "Can.h"
#include "Can_Cfg.h"
extern const Can_ConfigType* const Can_ConfigPtr;
const Can_ConfigType Can_Config = { 0U };
const Can_ConfigType* const Can_ConfigPtr = &Can_Config;
