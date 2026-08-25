/** @file Nm_Lcfg.c @brief Nm Link-Time Configuration */
/* @req SWS_Nm_00001 @req SWS_Nm_00002 @req SWS_Nm_00003 */

#include "Nm.h"
#include "Nm_Cfg.h"
extern const Nm_ConfigType* const Nm_ConfigPtr;
extern const Nm_ConfigType Nm_Config;
const Nm_ConfigType Nm_Config = { 0U };
const Nm_ConfigType* const Nm_ConfigPtr = &Nm_Config;
