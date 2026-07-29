#ifndef ECUM_WAKEUPSOURCETYPE_H
#define ECUM_WAKEUPSOURCETYPE_H
#include "Std_Types.h"
typedef uint8 EcuM_WakeupSourceType;
#define ECUM_WAKEUP_SOURCE_POWER        0x00u
#define ECUM_WAKEUP_SOURCE_RESET        0x01u
#define ECUM_WAKEUP_SOURCE_INTERNAL     0x02u
#define ECUM_WAKEUP_SOURCE_EXTERNAL     0x03u
#define ECUM_WAKEUP_SOURCE_CAN          0x04u
#define ECUM_WAKEUP_SOURCE_LIN          0x05u
#define ECUM_WAKEUP_SOURCE_ETHERNET     0x06u
#endif
