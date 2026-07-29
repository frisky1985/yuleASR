/**
 * @file Port.h
 * @brief PORT Driver — stub API header
 * @version 1.0.0
 * @date 2026-07-12
 */

#ifndef PORT_H
#define PORT_H

#include "Std_Types.h"

typedef uint16 Port_PinType;

void Port_Init(const void* config);
void Port_SetPinDirection(Port_PinType pin, uint8 direction);
void Port_SetPinMode(Port_PinType pin, uint8 mode);
void Port_GetVersionInfo(Std_VersionInfoType* info);

#endif /* PORT_H */
