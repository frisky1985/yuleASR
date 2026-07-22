#ifndef COM_PRIVATE_H
#define COM_PRIVATE_H
#include "Std_Types.h"
#include "Com.h"
#define COM_MAX_IPDU_GROUPS  32U
#define COM_MAX_SIGNALS      256U
typedef struct { uint8 dummy; } Com_InternalData;
extern Com_InternalData Com_GateData[];
#endif
