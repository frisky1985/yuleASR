/** @file IoHwAb_Lcfg.c @brief IoHwAb Link-Time Configuration */
/* @req SWS_IoHwAb_00001 @req SWS_IoHwAb_00002 @req SWS_IoHwAb_00004 */

#include "IoHwAb.h"
#include "IoHwAb_Cfg.h"
extern const IoHwAb_ConfigType* const IoHwAb_ConfigPtr;
extern const IoHwAb_ConfigType IoHwAb_Config;
const IoHwAb_ConfigType IoHwAb_Config = { 0U };
const IoHwAb_ConfigType* const IoHwAb_ConfigPtr = &IoHwAb_Config;
