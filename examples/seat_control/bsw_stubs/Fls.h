/**
 * @file Fls.h
 * @brief Flash Driver — stub API header
 * @version 1.0.0
 * @date 2026-07-12
 */

#ifndef FLS_H
#define FLS_H

#include "Std_Types.h"

void Fls_Init(const void* config);
Std_ReturnType Fls_Write(uint32 address, const uint8* data, uint16 length);
Std_ReturnType Fls_Read(uint32 address, uint8* data, uint16 length);
Std_ReturnType Fls_Erase(uint32 address, uint16 length);
void Fls_MainFunction(void);

#endif /* FLS_H */
