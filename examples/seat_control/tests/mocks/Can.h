/**
 * @file Can.h — Stub CAN for host-side testing
 */
#ifndef MOCK_CAN_H
#define MOCK_CAN_H

#include "Std_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

void Can_Init(const void* config);
Std_ReturnType Can_Write(uint8 hth, const void* pdu);
Std_ReturnType Can_SetBaudrate(uint8 controller, uint16 baudrate);
void Can_MainFunction_Write(void);
void Can_MainFunction_Read(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_CAN_H */
