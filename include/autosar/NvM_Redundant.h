#ifndef NVM_REDUNDANT_H
#define NVM_REDUNDANT_H
#include "Std_Types.h"
typedef struct { uint16 dummy; } NvM_Redundant_ConfigType;
Std_ReturnType NvM_Redundant_Init(const NvM_Redundant_ConfigType* cfg);
#endif
