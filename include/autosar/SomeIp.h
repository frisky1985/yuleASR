#ifndef SOMEIP_H
#define SOMEIP_H
#include "Std_Types.h"
typedef struct { uint16 dummy; } SomeIp_ConfigType;
Std_ReturnType SomeIp_Init(const SomeIp_ConfigType* cfg);
#endif
