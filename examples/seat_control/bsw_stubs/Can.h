/**
 * @file Can.h
 * @brief CAN Driver — stub API header
 * @version 1.0.0
 * @date 2026-07-12
 */

#ifndef CAN_H
#define CAN_H

#include "Std_Types.h"

typedef uint16 Can_IdType;
typedef uint8  Can_HwHandleType;

void Can_Init(const void* config);
Std_ReturnType Can_Write(Can_HwHandleType hth, const void* pdu);
Std_ReturnType Can_SetBaudrate(Can_HwHandleType controller, uint16 baudrate);
void Can_MainFunction_Write(void);
void Can_MainFunction_Read(void);

#endif /* CAN_H */
