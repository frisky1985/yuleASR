/** @file Mcu_Lcfg.c @brief Mcu Link-Time Configuration */
/* @req SWS_Mcu_00001 @req SWS_Mcu_00002 @req SWS_Mcu_00003 */

#include "Mcu.h"
#include "Mcu_Cfg.h"
const Mcu_ConfigType Mcu_Config = { 0U };
const Mcu_ConfigType* const Mcu_ConfigPtr = &Mcu_Config;
