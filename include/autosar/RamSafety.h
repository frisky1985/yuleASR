#ifndef RAMSAFETY_H
#define RAMSAFETY_H
#include "Std_Types.h"
typedef struct { uint16 dummy; } RamSafety_ConfigType;
Std_ReturnType RamSafety_Init(const RamSafety_ConfigType* cfg);
Std_ReturnType RamSafety_MainFunction(void);
#endif
