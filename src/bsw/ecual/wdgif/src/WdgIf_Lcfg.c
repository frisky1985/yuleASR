/** @file WdgIf_Lcfg.c @brief WdgIf Link-Time Configuration */
/* @req SWS_WdgIf_00001 @req SWS_WdgIf_00004 @req SWS_WdgIf_00005 */

#include "WdgIf.h"
#include "WdgIf_Cfg.h"
const WdgIf_ConfigType WdgIf_Config = { 0U };
const WdgIf_ConfigType* const WdgIf_ConfigPtr = &WdgIf_Config;
