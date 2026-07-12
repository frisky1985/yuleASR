/**
 * @file Lin.h — Stub LIN for host-side testing
 */
#ifndef MOCK_LIN_H
#define MOCK_LIN_H

#include "Std_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

void Lin_Init(const void* config);
Std_ReturnType Lin_SendFrame(uint8 channel, uint8 id,
                             const uint8* data, uint8 length);
Std_ReturnType Lin_ReceiveFrame(uint8 channel, uint8 id,
                                uint8* buffer, uint8* length);
void Lin_MainFunction(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_LIN_H */
