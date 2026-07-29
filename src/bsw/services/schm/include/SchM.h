#ifndef SCHM_H
#define SCHM_H
#include "Std_Types.h"
typedef enum { SCHM_POINT_ZERO = 0, SCHM_POINT_ONE, SCHM_POINT_TWO, SCHM_POINT_MAX } SchM_PointType;
void SchM_Init(void);
void SchM_Deinit(void);
void SchM_Start(void);
void SchM_Stop(void);
void SchM_SwitchPoint(SchM_PointType point);
#endif
