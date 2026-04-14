/**
 * @file IoHwAb.h
 * @brief I/O Hardware Abstraction module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: I/O Hardware Abstraction (IOHWAB)
 * Layer: ECU Abstraction Layer (ECUAL)
 */

#ifndef IOHWAB_H
#define IOHWAB_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "IoHwAb_Cfg.h"
#include "Adc.h"
#include "Dio.h"
#include "Pwm.h"
#include "Spi.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define IOHWAB_VENDOR_ID                (0x01U) /* YuleTech Vendor ID */
#define IOHWAB_MODULE_ID                (0x7AU) /* IOHWAB Module ID */
#define IOHWAB_AR_RELEASE_MAJOR_VERSION (0x04U)
#define IOHWAB_AR_RELEASE_MINOR_VERSION (0x04U)
#define IOHWAB_AR_RELEASE_REVISION_VERSION (0x00U)
#define IOHWAB_SW_MAJOR_VERSION         (0x01U)
#define IOHWAB_SW_MINOR_VERSION         (0x00U)
#define IOHWAB_SW_PATCH_VERSION         (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define IOHWAB_SID_INIT                 (0x01U)
#define IOHWAB_SID_DEINIT               (0x02U)
#define IOHWAB_SID_ANALOGREAD           (0x03U)
#define IOHWAB_SID_ANALOGWRITE          (0x04U)
#define IOHWAB_SID_DIGITALREAD          (0x05U)
#define IOHWAB_SID_DIGITALWRITE         (0x06U)
#define IOHWAB_SID_PWMSETDUTY           (0x07U)
#define IOHWAB_SID_PWMSETFREQANDDUTY    (0x08U)
#define IOHWAB_SID_SPITRANSFER          (0x09U)
#define IOHWAB_SID_GETVERSIONINFO       (0x0AU)
#define IOHWAB_SID_MAINFUNCTION         (0x0BU)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define IOHWAB_E_PARAM_POINTER          (0x01U)
#define IOHWAB_E_PARAM_CHANNEL          (0x02U)
#define IOHWAB_E_PARAM_VALUE            (0x03U)
#define IOHWAB_E_UNINIT                 (0x04U)
#define IOHWAB_E_ALREADY_INITIALIZED    (0x05U)
#define IOHWAB_E_PARAM_CONFIG           (0x06U)
#define IOHWAB_E_BUSY                   (0x07U)
#define IOHWAB_E_TIMEOUT                (0x08U)

/*==================================================================================================
*                                    IOHWAB RETURN TYPE
==================================================================================================*/
typedef enum {
    IOHWAB_OK = 0,
    IOHWAB_NOT_OK,
    IOHWAB_BUSY,
    IOHWAB_TIMEOUT
} IoHwAb_ReturnType;

/*==================================================================================================
*                                    IOHWAB CHANNEL TYPE
==================================================================================================*/
typedef uint16 IoHwAb_ChannelType;

/*==================================================================================================
*                                    IOHWAB SIGNAL TYPE
==================================================================================================*/
typedef uint16 IoHwAb_SignalType;

/*==================================================================================================
*                                    IOHWAB ANALOG CHANNEL CONFIG TYPE
==================================================================================================*/
typedef struct {
    IoHwAb_ChannelType ChannelId;
    Adc_ChannelType AdcChannel;
    uint16 Resolution;
    float32 MinValue;
    float32 MaxValue;
    float32 ScalingFactor;
    float32 Offset;
} IoHwAb_AnalogChannelConfigType;

/*==================================================================================================
*                                    IOHWAB DIGITAL CHANNEL CONFIG TYPE
==================================================================================================*/
typedef struct {
    IoHwAb_ChannelType ChannelId;
    Dio_ChannelType DioChannel;
    boolean Inverted;
} IoHwAb_DigitalChannelConfigType;

/*==================================================================================================
*                                    IOHWAB PWM CHANNEL CONFIG TYPE
==================================================================================================*/
typedef struct {
    IoHwAb_ChannelType ChannelId;
    Pwm_ChannelType PwmChannel;
    Pwm_PeriodType DefaultPeriod;
    uint16 DefaultDutyCycle;
} IoHwAb_PwmChannelConfigType;

/*==================================================================================================
*                                    IOHWAB SPI DEVICE CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint8 DeviceId;
    Spi_SequenceType SpiSequence;
    uint8 ChipSelectPin;
    uint32 Baudrate;
} IoHwAb_SpiDeviceConfigType;

/*==================================================================================================
*                                    IOHWAB CONFIG TYPE
==================================================================================================*/
typedef struct {
    const IoHwAb_AnalogChannelConfigType* AnalogChannels;
    uint8 NumAnalogChannels;
    const IoHwAb_DigitalChannelConfigType* DigitalChannels;
    uint8 NumDigitalChannels;
    const IoHwAb_PwmChannelConfigType* PwmChannels;
    uint8 NumPwmChannels;
    const IoHwAb_SpiDeviceConfigType* SpiDevices;
    uint8 NumSpiDevices;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
} IoHwAb_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define IOHWAB_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const IoHwAb_ConfigType IoHwAb_Config;

#define IOHWAB_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    ANALOG VALUE TYPE
==================================================================================================*/
typedef uint16 IoHwAb_AnalogValueType;

/*==================================================================================================
*                                    DIGITAL VALUE TYPE
==================================================================================================*/
typedef boolean IoHwAb_DigitalValueType;

/*==================================================================================================
*                                    PWM VALUE TYPE
==================================================================================================*/
typedef struct {
    uint16 DutyCycle;
    uint32 Frequency;
} IoHwAb_PwmValueType;

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define IOHWAB_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the I/O Hardware Abstraction
 * @param ConfigPtr Pointer to configuration structure
 */
void IoHwAb_Init(const IoHwAb_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the I/O Hardware Abstraction
 */
void IoHwAb_DeInit(void);

/**
 * @brief Reads an analog channel
 * @param Channel Channel to read
 * @param Value Pointer to store value
 * @return Result of operation
 */
IoHwAb_ReturnType IoHwAb_AnalogRead(IoHwAb_ChannelType Channel, IoHwAb_AnalogValueType* Value);

/**
 * @brief Writes to an analog channel (DAC)
 * @param Channel Channel to write
 * @param Value Value to write
 * @return Result of operation
 */
IoHwAb_ReturnType IoHwAb_AnalogWrite(IoHwAb_ChannelType Channel, IoHwAb_AnalogValueType Value);

/**
 * @brief Reads a digital channel
 * @param Channel Channel to read
 * @param Value Pointer to store value
 * @return Result of operation
 */
IoHwAb_ReturnType IoHwAb_DigitalRead(IoHwAb_ChannelType Channel, IoHwAb_DigitalValueType* Value);

/**
 * @brief Writes to a digital channel
 * @param Channel Channel to write
 * @param Value Value to write
 * @return Result of operation
 */
IoHwAb_ReturnType IoHwAb_DigitalWrite(IoHwAb_ChannelType Channel, IoHwAb_DigitalValueType Value);

/**
 * @brief Sets PWM duty cycle
 * @param Channel Channel to set
 * @param DutyCycle Duty cycle value (0-10000 = 0-100.00%)
 * @return Result of operation
 */
IoHwAb_ReturnType IoHwAb_PwmSetDuty(IoHwAb_ChannelType Channel, uint16 DutyCycle);

/**
 * @brief Sets PWM frequency and duty cycle
 * @param Channel Channel to set
 * @param Frequency Frequency in Hz
 * @param DutyCycle Duty cycle value (0-10000 = 0-100.00%)
 * @return Result of operation
 */
IoHwAb_ReturnType IoHwAb_PwmSetFreqAndDuty(IoHwAb_ChannelType Channel, uint32 Frequency, uint16 DutyCycle);

/**
 * @brief Performs SPI transfer
 * @param DeviceId SPI device ID
 * @param TxData Transmit data buffer
 * @param RxData Receive data buffer
 * @param Length Data length
 * @return Result of operation
 */
IoHwAb_ReturnType IoHwAb_SpiTransfer(uint8 DeviceId, const uint8* TxData, uint8* RxData, uint16 Length);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void IoHwAb_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Main function for periodic processing
 */
void IoHwAb_MainFunction(void);

#define IOHWAB_STOP_SEC_CODE
#include "MemMap.h"

#endif /* IOHWAB_H */
