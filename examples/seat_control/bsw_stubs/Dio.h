/**
 * @file Dio.h
 * @brief DIO Driver — stub API header
 * @version 1.0.0
 * @date 2026-07-12
 */

#ifndef DIO_H
#define DIO_H

#include "Std_Types.h"

typedef uint16 Dio_ChannelType;
typedef uint8  Dio_PortType;
typedef uint32 Dio_PortLevelType;
typedef uint8  Dio_LevelType;

void Dio_Init(const void* config);
Dio_LevelType Dio_ReadChannel(Dio_ChannelType channel);
void Dio_WriteChannel(Dio_ChannelType channel, Dio_LevelType level);
Dio_PortLevelType Dio_ReadPort(Dio_PortType port);
void Dio_WritePort(Dio_PortType port, Dio_PortLevelType level);
void Dio_GetVersionInfo(Std_VersionInfoType* info);

#endif /* DIO_H */
