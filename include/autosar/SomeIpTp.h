#ifndef SOMEIPTP_H
#define SOMEIPTP_H
#include "Std_Types.h"
typedef struct { uint16 dummy; } SomeIpTp_ConfigType;
Std_ReturnType SomeIpTp_Init(const SomeIpTp_ConfigType* cfg);
#endif
