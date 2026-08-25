/** @file CanSm_Lcfg.c @brief CanSm Link-Time Configuration */
/* @req SWS_CanSM_00001 @req SWS_CanSM_00002 @req SWS_CanSM_00003 */

#include "CanSm.h"
#include "CanSm_Cfg.h"
const CanSm_ConfigType CanSm_Config = { 0U };
const CanSm_ConfigType* const CanSm_ConfigPtr = &CanSm_Config;
