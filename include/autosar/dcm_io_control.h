#ifndef DCM_IO_CONTROL_H
#define DCM_IO_CONTROL_H
#include "Std_Types.h"
void Dcm_IOControl_Reset(void);
Std_ReturnType Dcm_IOControl_Process(const uint8* data, uint16 len);
#endif
