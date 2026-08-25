/** @file SchM_Lcfg.c @brief SchM Link-Time Configuration */
/* @req SWS_SchM_00001 @req SWS_SchM_00002 @req SWS_SchM_00005 */

#include "SchM.h"
#include "SchM_Cfg.h"
extern const SchM_ConfigType* const SchM_ConfigPtr;
extern const SchM_ConfigType SchM_Config;
const SchM_ConfigType SchM_Config = { 0U };
const SchM_ConfigType* const SchM_ConfigPtr = &SchM_Config;
