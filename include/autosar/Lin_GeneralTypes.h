#ifndef LIN_GENERALTYPES_H
#define LIN_GENERALTYPES_H
#include "Std_Types.h"
typedef uint16 Lin_SlaveErrorType;
typedef uint16 Lin_FrameRespType;
#define LIN_FRAMERESP_TX        ((Lin_FrameRespType)0U)
#define LIN_FRAMERESP_RX        ((Lin_FrameRespType)1U)
#define LIN_FRAMERESP_IGNORE    ((Lin_FrameRespType)2U)
#endif
