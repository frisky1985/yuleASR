#ifndef DCM_DID_H
#define DCM_DID_H
#include "Std_Types.h"
Std_ReturnType Dcm_DID_Read(uint16 did, uint8* data, uint16* len);
Std_ReturnType Dcm_DID_Write(uint16 did, const uint8* data, uint16 len);
#endif
