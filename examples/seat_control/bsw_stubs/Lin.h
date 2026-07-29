/**
 * @file Lin.h
 * @brief LIN Driver — stub API header
 * @version 1.0.0
 * @date 2026-07-12
 */

#ifndef LIN_H
#define LIN_H

#include "Std_Types.h"

void Lin_Init(const void* config);
Std_ReturnType Lin_SendFrame(uint8 channel, uint8 frameId,
                             const uint8* data, uint8 length);
Std_ReturnType Lin_ReceiveFrame(uint8 channel, uint8 frameId,
                                uint8* buffer, uint8* length);
void Lin_MainFunction(void);

#endif /* LIN_H */
