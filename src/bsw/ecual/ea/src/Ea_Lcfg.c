/** @file Ea_Lcfg.c @brief Ea Link-Time Configuration */
/* @req SWS_Ea_00001 @req SWS_Ea_00005 @req SWS_Ea_00006 */

#include "Ea.h"
#include "Ea_Cfg.h"
extern const Ea_ConfigType* const Ea_ConfigPtr;
const Ea_ConfigType Ea_Config = { 0U };
const Ea_ConfigType* const Ea_ConfigPtr = &Ea_Config;
