#ifndef CANIF_CONTROLLERMODETYPE_H
#define CANIF_CONTROLLERMODETYPE_H
#include "Std_Types.h"
typedef uint8 CanIf_ControllerModeType;
#define CANIF_CS_UNINIT         0x00u
#define CANIF_CS_STARTED        0x01u
#define CANIF_CS_STOPPED        0x02u
#define CANIF_CS_SLEEP          0x03u
#define CANIF_NUM_CONTROLLERS   2u
#endif
