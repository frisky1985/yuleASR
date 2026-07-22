/** @file Gpt_Lcfg.c @brief Gpt Link-Time Configuration */
#include "Gpt.h"
#include "Gpt_Cfg.h"
extern const Gpt_ConfigType* const Gpt_ConfigPtr;
const Gpt_ConfigType Gpt_Config = { 0U };
const Gpt_ConfigType* const Gpt_ConfigPtr = &Gpt_Config;
