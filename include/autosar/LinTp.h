#ifndef LINTP_H
#define LINTP_H
#include "Std_Types.h"
typedef struct { uint16 dummy; } LinTp_ConfigType;
Std_ReturnType LinTp_Init(const LinTp_ConfigType* cfg);
#endif
