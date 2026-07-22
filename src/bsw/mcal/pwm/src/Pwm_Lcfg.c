/** @file Pwm_Lcfg.c @brief Pwm Link-Time Configuration */
#include "Pwm.h"
#include "Pwm_Cfg.h"
extern const Pwm_ConfigType* const Pwm_ConfigPtr;
const Pwm_ConfigType Pwm_Config = { 0U };
const Pwm_ConfigType* const Pwm_ConfigPtr = &Pwm_Config;
