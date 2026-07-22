/** @file IoHwAb_Lcfg.c @brief IoHwAb Link-Time Configuration */
#include "IoHwAb.h"
#include "IoHwAb_Cfg.h"
extern const IoHwAb_ConfigType* const IoHwAb_ConfigPtr;
extern const IoHwAb_ConfigType IoHwAb_Config;
const IoHwAb_ConfigType IoHwAb_Config = { 0U };
const IoHwAb_ConfigType* const IoHwAb_ConfigPtr = &IoHwAb_Config;
