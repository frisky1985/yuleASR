#ifndef SOMEIPXF_H
#define SOMEIPXF_H
#include "Std_Types.h"
typedef struct { uint16 dummy; } SomeIpXf_ConfigType;
Std_ReturnType SomeIpXf_Init(const SomeIpXf_ConfigType* cfg);
#endif
