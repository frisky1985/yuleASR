/**
 * @file Gpt.h — Stub GPT for host-side testing
 */
#ifndef MOCK_GPT_H
#define MOCK_GPT_H

#include "Std_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

void Gpt_Init(const void* config);
void Gpt_StartTimer(uint8 channel, uint32 value);
void Gpt_StopTimer(uint8 channel);
uint32 Gpt_GetTimeElapsed(uint8 channel);
uint32 Gpt_GetTimeRemaining(uint8 channel);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_GPT_H */
