/**
 * @file Dio.h — Mock DIO for host-side testing
 *
 * Provides controllable Dio_ReadChannel and records writes.
 * Use mock_Dio_SetChannel(channel, level) to inject inputs.
 * Use mock_Dio_GetWriteChannel(channel) to inspect outputs.
 */
#ifndef MOCK_DIO_H
#define MOCK_DIO_H

#include "Std_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16 Dio_ChannelType;
typedef uint8  Dio_PortType;
typedef uint32 Dio_PortLevelType;
typedef uint8  Dio_LevelType;

/* --- Mock control functions --- */

/** Set the value Dio_ReadChannel will return for a channel. */
void mock_Dio_SetChannel(Dio_ChannelType channel, Dio_LevelType level);

/** Get the last written value for a channel (or STD_LOW if never written). */
Dio_LevelType mock_Dio_GetWriteChannel(Dio_ChannelType channel);

/** Clear all mock state. */
void mock_Dio_Reset(void);

/** Reset all mock modules (DIO, ADC, PWM) to default state. */
void mock_All_Reset(void);

/* --- AUTOSAR API (mock implementations) --- */
void    Dio_Init(const void* config);
Dio_LevelType Dio_ReadChannel(Dio_ChannelType channel);
void    Dio_WriteChannel(Dio_ChannelType channel, Dio_LevelType level);
Dio_PortLevelType Dio_ReadPort(Dio_PortType port);
void    Dio_WritePort(Dio_PortType port, Dio_PortLevelType level);
void    Dio_GetVersionInfo(Std_VersionInfoType* info);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_DIO_H */
