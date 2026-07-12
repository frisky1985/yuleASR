/**
 * @file Gpt.h
 * @brief GPT Driver — stub API header
 * @version 1.0.0
 * @date 2026-07-12
 */

#ifndef GPT_H
#define GPT_H

#include "Std_Types.h"

void Gpt_Init(const void* config);
void Gpt_StartTimer(uint8 channel, uint32 value);
void Gpt_StopTimer(uint8 channel);
uint32 Gpt_GetTimeElapsed(uint8 channel);
uint32 Gpt_GetTimeRemaining(uint8 channel);

#endif /* GPT_H */
