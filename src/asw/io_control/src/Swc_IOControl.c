/**
 * @file Swc_IOControl.c
 * @brief IO Control Software Component Implementation
 * @version 1.0.0
 * @date 2026-04-19
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Classic Platform 4.x - Application Software Component
 * Purpose: Input/Output control and management at application layer
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Swc_IOControl.h"
#include "Rte.h"
#include "Det.h"

/*==================================================================================================
*                                    LOCAL MACROS
==================================================================================================*/
#define SWC_IOCONTROL_MODULE_ID             0x85
#define SWC_IOCONTROL_INSTANCE_ID           0x00

/* Maximum channels */
#define IO_MAX_DIGITAL_INPUTS               16
#define IO_MAX_DIGITAL_OUTPUTS              16
#define IO_MAX_ANALOG_INPUTS                8
#define IO_MAX_ANALOG_OUTPUTS               4
#define IO_MAX_PWM_INPUTS                   4
#define IO_MAX_PWM_OUTPUTS                  4

/* Debounce constants */
#define IO_DEBOUNCE_SAMPLES                 3

/*==================================================================================================
*                                    LOCAL TYPES
==================================================================================================*/
typedef struct {
    Swc_DigitalIOValueType inputs[IO_MAX_DIGITAL_INPUTS];
    Swc_DigitalIOValueType outputs[IO_MAX_DIGITAL_OUTPUTS];
    Swc_AnalogIOValueType analogInputs[IO_MAX_ANALOG_INPUTS];
    Swc_AnalogIOValueType analogOutputs[IO_MAX_ANALOG_OUTPUTS];
    Swc_PwmIOValueType pwmInputs[IO_MAX_PWM_INPUTS];
    Swc_PwmIOValueType pwmOutputs[IO_MAX_PWM_OUTPUTS];
    Swc_IOStatisticsType statistics;
    Swc_IOStateType state;
    uint8 numDigitalInputs;
    uint8 numDigitalOutputs;
    uint8 numAnalogInputs;
    uint8 numAnalogOutputs;
    uint8 numPwmInputs;
    uint8 numPwmOutputs;
    boolean isInitialized;
} Swc_IOControlInternalType;

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
#define RTE_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

STATIC Swc_IOControlInternalType swcIOControl = {
    .inputs = {{0}},
    .outputs = {{0}},
    .analogInputs = {{0}},
    .analogOutputs = {{0}},
    .pwmInputs = {{0}},
    .pwmOutputs = {{0}},
    .statistics = {0},
    .state = IO_STATE_INACTIVE,
    .numDigitalInputs = 0,
    .numDigitalOutputs = 0,
    .numAnalogInputs = 0,
    .numAnalogOutputs = 0,
    .numPwmInputs = 0,
    .numPwmOutputs = 0,
    .isInitialized = FALSE
};

/* Debounce counters */
STATIC uint8 digitalDebounce[IO_MAX_DIGITAL_INPUTS] = {0};

#define RTE_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC sint16 Swc_IOControl_FindDigitalInput(uint16 channelId);
STATIC sint16 Swc_IOControl_FindDigitalOutput(uint16 channelId);
STATIC sint16 Swc_IOControl_FindAnalogInput(uint16 channelId);
STATIC sint16 Swc_IOControl_FindAnalogOutput(uint16 channelId);
STATIC sint16 Swc_IOControl_FindPwmInput(uint16 channelId);
STATIC sint16 Swc_IOControl_FindPwmOutput(uint16 channelId);
STATIC void Swc_IOControl_ProcessDigitalInputs(void);
STATIC void Swc_IOControl_ProcessAnalogInputs(void);
STATIC void Swc_IOControl_ProcessPwmInputs(void);

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Finds digital input channel
 */
STATIC sint16 Swc_IOControl_FindDigitalInput(uint16 channelId)
{
    uint8 i;

    for (i = 0; i < swcIOControl.numDigitalInputs; i++) {
        if (swcIOControl.inputs[i].channelId == channelId) {
            return (sint16)i;
        }
    }

    return -1;
}

/**
 * @brief Finds digital output channel
 */
STATIC sint16 Swc_IOControl_FindDigitalOutput(uint16 channelId)
{
    uint8 i;

    for (i = 0; i < swcIOControl.numDigitalOutputs; i++) {
        if (swcIOControl.outputs[i].channelId == channelId) {
            return (sint16)i;
        }
    }

    return -1;
}

/**
 * @brief Finds analog input channel
 */
STATIC sint16 Swc_IOControl_FindAnalogInput(uint16 channelId)
{
    uint8 i;

    for (i = 0; i < swcIOControl.numAnalogInputs; i++) {
        if (swcIOControl.analogInputs[i].channelId == channelId) {
            return (sint16)i;
        }
    }

    return -1;
}

/**
 * @brief Finds analog output channel
 */
STATIC sint16 Swc_IOControl_FindAnalogOutput(uint16 channelId)
{
    uint8 i;

    for (i = 0; i < swcIOControl.numAnalogOutputs; i++) {
        if (swcIOControl.analogOutputs[i].channelId == channelId) {
            return (sint16)i;
        }
    }

    return -1;
}

/**
 * @brief Finds PWM input channel
 */
STATIC sint16 Swc_IOControl_FindPwmInput(uint16 channelId)
{
    uint8 i;

    for (i = 0; i < swcIOControl.numPwmInputs; i++) {
        if (swcIOControl.pwmInputs[i].channelId == channelId) {
            return (sint16)i;
        }
    }

    return -1;
}

/**
 * @brief Finds PWM output channel
 */
STATIC sint16 Swc_IOControl_FindPwmOutput(uint16 channelId)
{
    uint8 i;

    for (i = 0; i < swcIOControl.numPwmOutputs; i++) {
        if (swcIOControl.pwmOutputs[i].channelId == channelId) {
            return (sint16)i;
        }
    }

    return -1;
}

/**
 * @brief Processes digital inputs with debounce
 */
STATIC void Swc_IOControl_ProcessDigitalInputs(void)
{
    uint8 i;
    Swc_DigitalIOValueType inputValue;

    for (i = 0; i < swcIOControl.numDigitalInputs; i++) {
        /* Read from RTE */
        if (Rte_Read_DigitalInput(&inputValue) == RTE_E_OK) {
            if (inputValue.channelId == swcIOControl.inputs[i].channelId) {
                /* Simple debounce */
                if (inputValue.value == swcIOControl.inputs[i].value) {
                    if (digitalDebounce[i] < IO_DEBOUNCE_SAMPLES) {
                        digitalDebounce[i]++;
                    }
                } else {
                    digitalDebounce[i] = 0;
                }

                if (digitalDebounce[i] >= IO_DEBOUNCE_SAMPLES) {
                    swcIOControl.inputs[i].value = inputValue.value;
                    swcIOControl.inputs[i].timestamp = Rte_GetTime();
                    swcIOControl.inputs[i].isValid = TRUE;
                }
            }
        }
    }
}

/**
 * @brief Processes analog inputs
 */
STATIC void Swc_IOControl_ProcessAnalogInputs(void)
{
    uint8 i;
    Swc_AnalogIOValueType inputValue;

    for (i = 0; i < swcIOControl.numAnalogInputs; i++) {
        /* Read from RTE */
        if (Rte_Read_AnalogInput(&inputValue) == RTE_E_OK) {
            if (inputValue.channelId == swcIOControl.analogInputs[i].channelId) {
                swcIOControl.analogInputs[i].rawValue = inputValue.rawValue;
                swcIOControl.analogInputs[i].physicalValue = inputValue.physicalValue;
                swcIOControl.analogInputs[i].timestamp = Rte_GetTime();
                swcIOControl.analogInputs[i].isValid = TRUE;
            }
        }
    }
}

/**
 * @brief Processes PWM inputs
 */
STATIC void Swc_IOControl_ProcessPwmInputs(void)
{
    uint8 i;
    Swc_PwmIOValueType inputValue;

    for (i = 0; i < swcIOControl.numPwmInputs; i++) {
        /* Read from RTE */
        if (Rte_Read_PwmInput(&inputValue) == RTE_E_OK) {
            if (inputValue.channelId == swcIOControl.pwmInputs[i].channelId) {
                swcIOControl.pwmInputs[i].dutyCycle = inputValue.dutyCycle;
                swcIOControl.pwmInputs[i].frequency = inputValue.frequency;
                swcIOControl.pwmInputs[i].timestamp = Rte_GetTime();
                swcIOControl.pwmInputs[i].isValid = TRUE;
            }
        }
    }
}

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initializes the IO Control component
 */
void Swc_IOControl_Init(void)
{
    uint8 i;

    if (swcIOControl.isInitialized) {
        return;
    }

    /* Initialize digital inputs */
    for (i = 0; i < IO_MAX_DIGITAL_INPUTS; i++) {
        swcIOControl.inputs[i].channelId = 0;
        swcIOControl.inputs[i].value = FALSE;
        swcIOControl.inputs[i].timestamp = 0;
        swcIOControl.inputs[i].isValid = FALSE;
        digitalDebounce[i] = 0;
    }
    swcIOControl.numDigitalInputs = 0;

    /* Initialize digital outputs */
    for (i = 0; i < IO_MAX_DIGITAL_OUTPUTS; i++) {
        swcIOControl.outputs[i].channelId = 0;
        swcIOControl.outputs[i].value = FALSE;
        swcIOControl.outputs[i].timestamp = 0;
        swcIOControl.outputs[i].isValid = FALSE;
    }
    swcIOControl.numDigitalOutputs = 0;

    /* Initialize analog inputs */
    for (i = 0; i < IO_MAX_ANALOG_INPUTS; i++) {
        swcIOControl.analogInputs[i].channelId = 0;
        swcIOControl.analogInputs[i].rawValue = 0;
        swcIOControl.analogInputs[i].physicalValue = 0;
        swcIOControl.analogInputs[i].timestamp = 0;
        swcIOControl.analogInputs[i].isValid = FALSE;
    }
    swcIOControl.numAnalogInputs = 0;

    /* Initialize analog outputs */
    for (i = 0; i < IO_MAX_ANALOG_OUTPUTS; i++) {
        swcIOControl.analogOutputs[i].channelId = 0;
        swcIOControl.analogOutputs[i].rawValue = 0;
        swcIOControl.analogOutputs[i].physicalValue = 0;
        swcIOControl.analogOutputs[i].timestamp = 0;
        swcIOControl.analogOutputs[i].isValid = FALSE;
    }
    swcIOControl.numAnalogOutputs = 0;

    /* Initialize PWM inputs */
    for (i = 0; i < IO_MAX_PWM_INPUTS; i++) {
        swcIOControl.pwmInputs[i].channelId = 0;
        swcIOControl.pwmInputs[i].dutyCycle = 0;
        swcIOControl.pwmInputs[i].frequency = 0;
        swcIOControl.pwmInputs[i].timestamp = 0;
        swcIOControl.pwmInputs[i].isValid = FALSE;
    }
    swcIOControl.numPwmInputs = 0;

    /* Initialize PWM outputs */
    for (i = 0; i < IO_MAX_PWM_OUTPUTS; i++) {
        swcIOControl.pwmOutputs[i].channelId = 0;
        swcIOControl.pwmOutputs[i].dutyCycle = 0;
        swcIOControl.pwmOutputs[i].frequency = 0;
        swcIOControl.pwmOutputs[i].timestamp = 0;
        swcIOControl.pwmOutputs[i].isValid = FALSE;
    }
    swcIOControl.numPwmOutputs = 0;

    /* Initialize statistics */
    swcIOControl.statistics.digitalReads = 0;
    swcIOControl.statistics.digitalWrites = 0;
    swcIOControl.statistics.analogReads = 0;
    swcIOControl.statistics.analogWrites = 0;
    swcIOControl.statistics.pwmReads = 0;
    swcIOControl.statistics.pwmWrites = 0;
    swcIOControl.statistics.errorCount = 0;

    swcIOControl.state = IO_STATE_ACTIVE;
    swcIOControl.isInitialized = TRUE;

    Det_ReportError(SWC_IOCONTROL_MODULE_ID, SWC_IOCONTROL_INSTANCE_ID,
                    0x01, RTE_E_OK);
}

/**
 * @brief 10ms cyclic runnable - fast IO processing
 */
void Swc_IOControl_10ms(void)
{
    if (!swcIOControl.isInitialized) {
        return;
    }

    if (swcIOControl.state != IO_STATE_ACTIVE) {
        return;
    }

    /* Process digital inputs */
    Swc_IOControl_ProcessDigitalInputs();

    /* Process digital outputs */
    /* Outputs are written on-demand */
}

/**
 * @brief 50ms cyclic runnable - slow IO processing
 */
void Swc_IOControl_50ms(void)
{
    if (!swcIOControl.isInitialized) {
        return;
    }

    if (swcIOControl.state != IO_STATE_ACTIVE) {
        return;
    }

    /* Process analog inputs */
    Swc_IOControl_ProcessAnalogInputs();

    /* Process PWM inputs */
    Swc_IOControl_ProcessPwmInputs();

    /* Write state via RTE */
    (void)Rte_Write_IOState(&swcIOControl.state);
}

/**
 * @brief Reads digital input
 */
Rte_StatusType Swc_IOControl_ReadDigitalInput(uint16 channelId, boolean* value)
{
    sint16 channelIndex;

    if (value == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcIOControl.isInitialized) {
        return RTE_E_UNINIT;
    }

    channelIndex = Swc_IOControl_FindDigitalInput(channelId);

    if (channelIndex < 0) {
        return RTE_E_INVALID;
    }

    if (!swcIOControl.inputs[channelIndex].isValid) {
        return RTE_E_NOT_OK;
    }

    *value = swcIOControl.inputs[channelIndex].value;
    swcIOControl.statistics.digitalReads++;

    return RTE_E_OK;
}

/**
 * @brief Writes digital output
 */
Rte_StatusType Swc_IOControl_WriteDigitalOutput(uint16 channelId, boolean value)
{
    sint16 channelIndex;
    Swc_DigitalIOValueType outputValue;

    if (!swcIOControl.isInitialized) {
        return RTE_E_UNINIT;
    }

    if (swcIOControl.state != IO_STATE_ACTIVE) {
        return RTE_E_NOT_ACTIVE;
    }

    channelIndex = Swc_IOControl_FindDigitalOutput(channelId);

    if (channelIndex < 0) {
        /* Create new output channel */
        if (swcIOControl.numDigitalOutputs >= IO_MAX_DIGITAL_OUTPUTS) {
            return RTE_E_LIMIT;
        }
        channelIndex = swcIOControl.numDigitalOutputs;
        swcIOControl.outputs[channelIndex].channelId = channelId;
        swcIOControl.numDigitalOutputs++;
    }

    /* Update output value */
    swcIOControl.outputs[channelIndex].value = value;
    swcIOControl.outputs[channelIndex].timestamp = Rte_GetTime();
    swcIOControl.outputs[channelIndex].isValid = TRUE;

    /* Write via RTE */
    outputValue = swcIOControl.outputs[channelIndex];
    if (Rte_Write_DigitalOutput(&outputValue) == RTE_E_OK) {
        swcIOControl.statistics.digitalWrites++;
        return RTE_E_OK;
    } else {
        swcIOControl.statistics.errorCount++;
        return RTE_E_NOT_OK;
    }
}

/**
 * @brief Reads analog input
 */
Rte_StatusType Swc_IOControl_ReadAnalogInput(uint16 channelId, uint16* value)
{
    sint16 channelIndex;

    if (value == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcIOControl.isInitialized) {
        return RTE_E_UNINIT;
    }

    channelIndex = Swc_IOControl_FindAnalogInput(channelId);

    if (channelIndex < 0) {
        return RTE_E_INVALID;
    }

    if (!swcIOControl.analogInputs[channelIndex].isValid) {
        return RTE_E_NOT_OK;
    }

    *value = swcIOControl.analogInputs[channelIndex].physicalValue;
    swcIOControl.statistics.analogReads++;

    return RTE_E_OK;
}

/**
 * @brief Writes analog output
 */
Rte_StatusType Swc_IOControl_WriteAnalogOutput(uint16 channelId, uint16 value)
{
    sint16 channelIndex;
    Swc_AnalogIOValueType outputValue;

    if (!swcIOControl.isInitialized) {
        return RTE_E_UNINIT;
    }

    if (swcIOControl.state != IO_STATE_ACTIVE) {
        return RTE_E_NOT_ACTIVE;
    }

    channelIndex = Swc_IOControl_FindAnalogOutput(channelId);

    if (channelIndex < 0) {
        /* Create new output channel */
        if (swcIOControl.numAnalogOutputs >= IO_MAX_ANALOG_OUTPUTS) {
            return RTE_E_LIMIT;
        }
        channelIndex = swcIOControl.numAnalogOutputs;
        swcIOControl.analogOutputs[channelIndex].channelId = channelId;
        swcIOControl.numAnalogOutputs++;
    }

    /* Update output value */
    swcIOControl.analogOutputs[channelIndex].physicalValue = value;
    swcIOControl.analogOutputs[channelIndex].rawValue = value;
    swcIOControl.analogOutputs[channelIndex].timestamp = Rte_GetTime();
    swcIOControl.analogOutputs[channelIndex].isValid = TRUE;

    /* Write via RTE */
    outputValue = swcIOControl.analogOutputs[channelIndex];
    if (Rte_Write_AnalogOutput(&outputValue) == RTE_E_OK) {
        swcIOControl.statistics.analogWrites++;
        return RTE_E_OK;
    } else {
        swcIOControl.statistics.errorCount++;
        return RTE_E_NOT_OK;
    }
}

/**
 * @brief Reads PWM input
 */
Rte_StatusType Swc_IOControl_ReadPwmInput(uint16 channelId,
                                           uint16* dutyCycle,
                                           uint16* frequency)
{
    sint16 channelIndex;

    if (dutyCycle == NULL || frequency == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcIOControl.isInitialized) {
        return RTE_E_UNINIT;
    }

    channelIndex = Swc_IOControl_FindPwmInput(channelId);

    if (channelIndex < 0) {
        return RTE_E_INVALID;
    }

    if (!swcIOControl.pwmInputs[channelIndex].isValid) {
        return RTE_E_NOT_OK;
    }

    *dutyCycle = swcIOControl.pwmInputs[channelIndex].dutyCycle;
    *frequency = swcIOControl.pwmInputs[channelIndex].frequency;
    swcIOControl.statistics.pwmReads++;

    return RTE_E_OK;
}

/**
 * @brief Writes PWM output
 */
Rte_StatusType Swc_IOControl_WritePwmOutput(uint16 channelId,
                                             uint16 dutyCycle,
                                             uint16 frequency)
{
    sint16 channelIndex;
    Swc_PwmIOValueType outputValue;

    if (!swcIOControl.isInitialized) {
        return RTE_E_UNINIT;
    }

    if (swcIOControl.state != IO_STATE_ACTIVE) {
        return RTE_E_NOT_ACTIVE;
    }

    channelIndex = Swc_IOControl_FindPwmOutput(channelId);

    if (channelIndex < 0) {
        /* Create new output channel */
        if (swcIOControl.numPwmOutputs >= IO_MAX_PWM_OUTPUTS) {
            return RTE_E_LIMIT;
        }
        channelIndex = swcIOControl.numPwmOutputs;
        swcIOControl.pwmOutputs[channelIndex].channelId = channelId;
        swcIOControl.numPwmOutputs++;
    }

    /* Update output value */
    swcIOControl.pwmOutputs[channelIndex].dutyCycle = dutyCycle;
    swcIOControl.pwmOutputs[channelIndex].frequency = frequency;
    swcIOControl.pwmOutputs[channelIndex].timestamp = Rte_GetTime();
    swcIOControl.pwmOutputs[channelIndex].isValid = TRUE;

    /* Write via RTE */
    outputValue = swcIOControl.pwmOutputs[channelIndex];
    if (Rte_Write_PwmOutput(&outputValue) == RTE_E_OK) {
        swcIOControl.statistics.pwmWrites++;
        return RTE_E_OK;
    } else {
        swcIOControl.statistics.errorCount++;
        return RTE_E_NOT_OK;
    }
}

/**
 * @brief Sets IO channel state
 */
Rte_StatusType Swc_IOControl_SetChannelState(uint16 channelId,
                                              Swc_IOStateType state)
{
    (void)channelId;  /* Unused in this simplified implementation */

    if (!swcIOControl.isInitialized) {
        return RTE_E_UNINIT;
    }

    if (state > IO_STATE_TEST) {
        return RTE_E_INVALID;
    }

    swcIOControl.state = state;
    return RTE_E_OK;
}

/**
 * @brief Gets IO channel state
 */
Rte_StatusType Swc_IOControl_GetChannelState(uint16 channelId,
                                              Swc_IOStateType* state)
{
    (void)channelId;  /* Unused - global state */

    if (state == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcIOControl.isInitialized) {
        return RTE_E_UNINIT;
    }

    *state = swcIOControl.state;
    return RTE_E_OK;
}

/**
 * @brief Gets IO statistics
 */
Rte_StatusType Swc_IOControl_GetStatistics(Swc_IOStatisticsType* stats)
{
    if (stats == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcIOControl.isInitialized) {
        return RTE_E_UNINIT;
    }

    *stats = swcIOControl.statistics;
    return RTE_E_OK;
}

/**
 * @brief Resets IO statistics
 */
Rte_StatusType Swc_IOControl_ResetStatistics(void)
{
    if (!swcIOControl.isInitialized) {
        return RTE_E_UNINIT;
    }

    swcIOControl.statistics.digitalReads = 0;
    swcIOControl.statistics.digitalWrites = 0;
    swcIOControl.statistics.analogReads = 0;
    swcIOControl.statistics.analogWrites = 0;
    swcIOControl.statistics.pwmReads = 0;
    swcIOControl.statistics.pwmWrites = 0;
    swcIOControl.statistics.errorCount = 0;

    return RTE_E_OK;
}

#define RTE_STOP_SEC_CODE
#include "MemMap.h"
