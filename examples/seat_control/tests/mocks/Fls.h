/**
 * @file Fls.h — Stub Flash for host-side testing
 */
#ifndef MOCK_FLS_H
#define MOCK_FLS_H

#include "Std_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

void Fls_Init(const void* config);
Std_ReturnType Fls_Write(uint32 address, const uint8* data, uint16 length);
Std_ReturnType Fls_Read(uint32 address, uint8* data, uint16 length);
Std_ReturnType Fls_Erase(uint32 address, uint16 length);
void Fls_MainFunction(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_FLS_H */
