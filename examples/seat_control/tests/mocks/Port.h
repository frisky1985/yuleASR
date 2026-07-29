/**
 * @file Port.h — Stub PORT for host-side testing
 */
#ifndef MOCK_PORT_H
#define MOCK_PORT_H

#include "Std_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

void Port_Init(const void* config);
void Port_SetPinDirection(uint16 pin, uint8 direction);
void Port_SetPinMode(uint16 pin, uint8 mode);
void Port_GetVersionInfo(Std_VersionInfoType* info);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_PORT_H */
