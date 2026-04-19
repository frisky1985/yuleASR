/**
 * @file Swc_IOControl.h
 * @brief IO Control Software Component Header
 * @version 1.0.0
 * @date 2026-04-19
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Classic Platform 4.x - Application Software Component
 * Purpose: Input/Output control and management at application layer
 */

#ifndef SWC_IOCONTROL_H
#define SWC_IOCONTROL_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Rte_Swc.h"
#include "Std_Types.h"

/*==================================================================================================
*                                    COMPONENT TYPE DEFINITIONS
==================================================================================================*/

/**
 * @brief IO channel type
 */
typedef enum {
    IO_CHANNEL_DIGITAL_INPUT = 0,
    IO_CHANNEL_DIGITAL_OUTPUT,
    IO_CHANNEL_ANALOG_INPUT,
    IO_CHANNEL_ANALOG_OUTPUT,
    IO_CHANNEL_PWM_INPUT,
    IO_CHANNEL_PWM_OUTPUT
} Swc_IOChannelType;

/**
 * @brief IO channel state
 */
typedef enum {
    IO_STATE_INACTIVE = 0,
    IO_STATE_ACTIVE,
    IO_STATE_FAULT,
    IO_STATE_TEST
} Swc_IOStateType;

/**
 * @brief IO channel configuration
 */
typedef struct {
    uint16 channelId;
    Swc_IOChannelType channelType;
    uint8 portId;
    uint8 pinId;
    uint16 minValue;
    uint16 maxValue;
    boolean debounceEnabled;
    uint16 debounceTime;
} Swc_IOChannelConfigType;

/**
 * @brief Digital IO value
 */
typedef struct {
    uint16 channelId;
    boolean value;
    uint32 timestamp;
    boolean isValid;
} Swc_DigitalIOValueType;

/**
 * @brief Analog IO value
 */
typedef struct {
    uint16 channelId;
    uint16 rawValue;
    uint16 physicalValue;
    uint32 timestamp;
    boolean isValid;
} Swc_AnalogIOValueType;

/**
 * @brief PWM IO value
 */
typedef struct {
    uint16 channelId;
    uint16 dutyCycle;
    uint16 frequency;
    uint32 timestamp;
    boolean isValid;
} Swc_PwmIOValueType;

/**
 * @brief IO statistics
 */
typedef struct {
    uint32 digitalReads;
    uint32 digitalWrites;
    uint32 analogReads;
    uint32 analogWrites;
    uint32 pwmReads;
    uint32 pwmWrites;
    uint32 errorCount;
} Swc_IOStatisticsType;

/*==================================================================================================
*                                    RUNNABLE IDS
==================================================================================================*/
#define RTE_RUNNABLE_IOControl_Init                  0x51
#define RTE_RUNNABLE_IOControl_10ms                  0x52
#define RTE_RUNNABLE_IOControl_50ms                  0x53

/*==================================================================================================
*                                    PORT IDS
==================================================================================================*/
#define SWC_IOCONTROL_PORT_IO_STATE_P                0x51
#define SWC_IOCONTROL_PORT_DIGITAL_INPUT_R           0x52
#define SWC_IOCONTROL_PORT_DIGITAL_OUTPUT_P          0x53
#define SWC_IOCONTROL_PORT_ANALOG_INPUT_R            0x54
#define SWC_IOCONTROL_PORT_ANALOG_OUTPUT_P           0x55
#define SWC_IOCONTROL_PORT_PWM_INPUT_R               0x56
#define SWC_IOCONTROL_PORT_PWM_OUTPUT_P              0x57

/*==================================================================================================
*                                    COMPONENT API
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the IO Control component
 */
extern void Swc_IOControl_Init(void);

/**
 * @brief 10ms cyclic runnable - fast IO processing
 */
extern void Swc_IOControl_10ms(void);

/**
 * @brief 50ms cyclic runnable - slow IO processing
 */
extern void Swc_IOControl_50ms(void);

/**
 * @brief Reads digital input
 * @param channelId Channel ID
 * @param value Pointer to store value
 * @return RTE status
 */
extern Rte_StatusType Swc_IOControl_ReadDigitalInput(uint16 channelId, boolean* value);

/**
 * @brief Writes digital output
 * @param channelId Channel ID
 * @param value Value to write
 * @return RTE status
 */
extern Rte_StatusType Swc_IOControl_WriteDigitalOutput(uint16 channelId, boolean value);

/**
 * @brief Reads analog input
 * @param channelId Channel ID
 * @param value Pointer to store value
 * @return RTE status
 */
extern Rte_StatusType Swc_IOControl_ReadAnalogInput(uint16 channelId, uint16* value);

/**
 * @brief Writes analog output
 * @param channelId Channel ID
 * @param value Value to write
 * @return RTE status
 */
extern Rte_StatusType Swc_IOControl_WriteAnalogOutput(uint16 channelId, uint16 value);

/**
 * @brief Reads PWM input
 * @param channelId Channel ID
 * @param dutyCycle Pointer to store duty cycle
 * @param frequency Pointer to store frequency
 * @return RTE status
 */
extern Rte_StatusType Swc_IOControl_ReadPwmInput(uint16 channelId,
                                                  uint16* dutyCycle,
                                                  uint16* frequency);

/**
 * @brief Writes PWM output
 * @param channelId Channel ID
 * @param dutyCycle Duty cycle value
 * @param frequency Frequency value
 * @return RTE status
 */
extern Rte_StatusType Swc_IOControl_WritePwmOutput(uint16 channelId,
                                                    uint16 dutyCycle,
                                                    uint16 frequency);

/**
 * @brief Sets IO channel state
 * @param channelId Channel ID
 * @param state Target state
 * @return RTE status
 */
extern Rte_StatusType Swc_IOControl_SetChannelState(uint16 channelId,
                                                     Swc_IOStateType state);

/**
 * @brief Gets IO channel state
 * @param channelId Channel ID
 * @param state Pointer to store state
 * @return RTE status
 */
extern Rte_StatusType Swc_IOControl_GetChannelState(uint16 channelId,
                                                     Swc_IOStateType* state);

/**
 * @brief Gets IO statistics
 * @param stats Pointer to store statistics
 * @return RTE status
 */
extern Rte_StatusType Swc_IOControl_GetStatistics(Swc_IOStatisticsType* stats);

/**
 * @brief Resets IO statistics
 * @return RTE status
 */
extern Rte_StatusType Swc_IOControl_ResetStatistics(void);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE INTERFACE MACROS
==================================================================================================*/
#define Rte_Write_IOState(data) \
    Rte_Write_SWC_IOCONTROL_PORT_IO_STATE_P(data)

#define Rte_Read_DigitalInput(data) \
    Rte_Read_SWC_IOCONTROL_PORT_DIGITAL_INPUT_R(data)

#define Rte_Write_DigitalOutput(data) \
    Rte_Write_SWC_IOCONTROL_PORT_DIGITAL_OUTPUT_P(data)

#define Rte_Read_AnalogInput(data) \
    Rte_Read_SWC_IOCONTROL_PORT_ANALOG_INPUT_R(data)

#define Rte_Write_AnalogOutput(data) \
    Rte_Write_SWC_IOCONTROL_PORT_ANALOG_OUTPUT_P(data)

#define Rte_Read_PwmInput(data) \
    Rte_Read_SWC_IOCONTROL_PORT_PWM_INPUT_R(data)

#define Rte_Write_PwmOutput(data) \
    Rte_Write_SWC_IOCONTROL_PORT_PWM_OUTPUT_P(data)

#endif /* SWC_IOCONTROL_H */
