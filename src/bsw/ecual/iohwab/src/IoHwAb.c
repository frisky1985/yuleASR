/**
 * @file IoHwAb.c
 * @brief I/O Hardware Abstraction implementation
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include "IoHwAb.h"
#include "IoHwAb_Cfg.h"
#include "Adc.h"
#include "Dio.h"
#include "Pwm.h"
#include "Spi.h"
#include "Det.h"

#define IOHWAB_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static boolean IoHwAb_DriverInitialized = FALSE;
static const IoHwAb_ConfigType* IoHwAb_ConfigPtr = NULL_PTR;
static IoHwAb_AnalogValueType IoHwAb_AnalogBuffer[IOHWAB_NUM_ANALOG_CHANNELS];
static boolean IoHwAb_DigitalBuffer[IOHWAB_NUM_DIGITAL_CHANNELS];
static IoHwAb_PwmValueType IoHwAb_PwmBuffer[IOHWAB_NUM_PWM_CHANNELS];

#define IOHWAB_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

#define IOHWAB_START_SEC_CODE
#include "MemMap.h"

void IoHwAb_Init(const IoHwAb_ConfigType* ConfigPtr)
{
    #if (IOHWAB_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_INIT, IOHWAB_E_PARAM_POINTER);
        return;
    }
    if (IoHwAb_DriverInitialized == TRUE) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_INIT, IOHWAB_E_ALREADY_INITIALIZED);
        return;
    }
    #endif
    
    IoHwAb_ConfigPtr = ConfigPtr;
    
    /* Initialize ADC driver */
    Adc_Init(NULL_PTR);  /* ADC uses its own global config */
    
    /* Initialize DIO driver (no init needed for DIO) */
    
    /* Initialize PWM driver */
    Pwm_Init(NULL_PTR);  /* PWM uses its own global config */
    
    /* Initialize SPI driver */
    Spi_Init(NULL_PTR);  /* SPI uses its own global config */
    
    /* Clear internal buffers */
    for (uint8 i = 0U; i < IOHWAB_NUM_ANALOG_CHANNELS; i++) {
        IoHwAb_AnalogBuffer[i] = 0U;
    }
    
    for (uint8 i = 0U; i < IOHWAB_NUM_DIGITAL_CHANNELS; i++) {
        IoHwAb_DigitalBuffer[i] = FALSE;
    }
    
    for (uint8 i = 0U; i < IOHWAB_NUM_PWM_CHANNELS; i++) {
        IoHwAb_PwmBuffer[i].DutyCycle = 0U;
        IoHwAb_PwmBuffer[i].Frequency = 0U;
    }
    
    IoHwAb_DriverInitialized = TRUE;
}

void IoHwAb_DeInit(void)
{
    #if (IOHWAB_DEV_ERROR_DETECT == STD_ON)
    if (IoHwAb_DriverInitialized == FALSE) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_DEINIT, IOHWAB_E_UNINIT);
        return;
    }
    #endif
    
    /* Deinitialize PWM driver */
    Pwm_DeInit();
    
    /* Deinitialize ADC driver */
    Adc_DeInit();
    
    /* Deinitialize SPI driver */
    (void)Spi_DeInit();
    
    IoHwAb_ConfigPtr = NULL_PTR;
    IoHwAb_DriverInitialized = FALSE;
}

IoHwAb_ReturnType IoHwAb_AnalogRead(IoHwAb_ChannelType Channel, IoHwAb_AnalogValueType* Value)
{
    #if (IOHWAB_DEV_ERROR_DETECT == STD_ON)
    if (IoHwAb_DriverInitialized == FALSE) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_ANALOGREAD, IOHWAB_E_UNINIT);
        return IOHWAB_NOT_OK;
    }
    if (Channel >= IOHWAB_NUM_ANALOG_CHANNELS) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_ANALOGREAD, IOHWAB_E_PARAM_CHANNEL);
        return IOHWAB_NOT_OK;
    }
    if (Value == NULL_PTR) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_ANALOGREAD, IOHWAB_E_PARAM_POINTER);
        return IOHWAB_NOT_OK;
    }
    #endif
    
    const IoHwAb_AnalogChannelConfigType* channelConfig = &IoHwAb_ConfigPtr->AnalogChannels[Channel];
    
    /* Start ADC conversion for the configured ADC channel */
    Adc_StartGroupConversion(0U);  /* Use group 0 for single channel conversion */
    
    /* Wait for conversion to complete (simplified - in real implementation use interrupt) */
    while (Adc_GetGroupStatus(0U) == ADC_BUSY) {
        /* Wait */
    }
    
    /* Read ADC result */
    Adc_ValueGroupType adcValue;
    Std_ReturnType status = Adc_ReadGroup(0U, &adcValue);
    
    if (status == E_OK) {
        /* Apply scaling and offset */
        float32 scaledValue = ((float32)adcValue * channelConfig->ScalingFactor) + channelConfig->Offset;
        
        /* Convert to IoHwAb value (0-65535 range based on resolution) */
        *Value = (IoHwAb_AnalogValueType)((scaledValue / channelConfig->MaxValue) * 65535.0f);
        
        /* Update internal buffer */
        IoHwAb_AnalogBuffer[Channel] = *Value;
        
        return IOHWAB_OK;
    }
    
    return IOHWAB_NOT_OK;
}

IoHwAb_ReturnType IoHwAb_AnalogWrite(IoHwAb_ChannelType Channel, IoHwAb_AnalogValueType Value)
{
    #if (IOHWAB_DEV_ERROR_DETECT == STD_ON)
    if (IoHwAb_DriverInitialized == FALSE) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_ANALOGWRITE, IOHWAB_E_UNINIT);
        return IOHWAB_NOT_OK;
    }
    if (Channel >= IOHWAB_NUM_ANALOG_CHANNELS) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_ANALOGWRITE, IOHWAB_E_PARAM_CHANNEL);
        return IOHWAB_NOT_OK;
    }
    #endif
    
    /* Note: Analog write would require DAC hardware support
     * For now, this is a placeholder that could use PWM as DAC alternative */
    (void)Channel;
    (void)Value;
    
    return IOHWAB_NOT_OK;  /* Not implemented - no DAC on i.MX8M Mini */
}

IoHwAb_ReturnType IoHwAb_DigitalRead(IoHwAb_ChannelType Channel, IoHwAb_DigitalValueType* Value)
{
    #if (IOHWAB_DEV_ERROR_DETECT == STD_ON)
    if (IoHwAb_DriverInitialized == FALSE) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_DIGITALREAD, IOHWAB_E_UNINIT);
        return IOHWAB_NOT_OK;
    }
    if (Channel >= IOHWAB_NUM_DIGITAL_CHANNELS) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_DIGITALREAD, IOHWAB_E_PARAM_CHANNEL);
        return IOHWAB_NOT_OK;
    }
    if (Value == NULL_PTR) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_DIGITALREAD, IOHWAB_E_PARAM_POINTER);
        return IOHWAB_NOT_OK;
    }
    #endif
    
    const IoHwAb_DigitalChannelConfigType* channelConfig = &IoHwAb_ConfigPtr->DigitalChannels[Channel];
    
    /* Read from DIO */
    Dio_LevelType dioLevel = Dio_ReadChannel(channelConfig->DioChannel);
    
    /* Apply inversion if configured */
    if (channelConfig->Inverted) {
        *Value = (dioLevel == STD_LOW) ? TRUE : FALSE;
    } else {
        *Value = (dioLevel == STD_HIGH) ? TRUE : FALSE;
    }
    
    /* Update internal buffer */
    IoHwAb_DigitalBuffer[Channel] = *Value;
    
    return IOHWAB_OK;
}

IoHwAb_ReturnType IoHwAb_DigitalWrite(IoHwAb_ChannelType Channel, IoHwAb_DigitalValueType Value)
{
    #if (IOHWAB_DEV_ERROR_DETECT == STD_ON)
    if (IoHwAb_DriverInitialized == FALSE) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_DIGITALWRITE, IOHWAB_E_UNINIT);
        return IOHWAB_NOT_OK;
    }
    if (Channel >= IOHWAB_NUM_DIGITAL_CHANNELS) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_DIGITALWRITE, IOHWAB_E_PARAM_CHANNEL);
        return IOHWAB_NOT_OK;
    }
    #endif
    
    const IoHwAb_DigitalChannelConfigType* channelConfig = &IoHwAb_ConfigPtr->DigitalChannels[Channel];
    
    /* Apply inversion if configured */
    Dio_LevelType dioLevel;
    if (channelConfig->Inverted) {
        dioLevel = (Value == TRUE) ? STD_LOW : STD_HIGH;
    } else {
        dioLevel = (Value == TRUE) ? STD_HIGH : STD_LOW;
    }
    
    /* Write to DIO */
    Dio_WriteChannel(channelConfig->DioChannel, dioLevel);
    
    /* Update internal buffer */
    IoHwAb_DigitalBuffer[Channel] = Value;
    
    return IOHWAB_OK;
}

IoHwAb_ReturnType IoHwAb_PwmSetDuty(IoHwAb_ChannelType Channel, uint16 DutyCycle)
{
    #if (IOHWAB_DEV_ERROR_DETECT == STD_ON)
    if (IoHwAb_DriverInitialized == FALSE) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_PWMSETDUTY, IOHWAB_E_UNINIT);
        return IOHWAB_NOT_OK;
    }
    if (Channel >= IOHWAB_NUM_PWM_CHANNELS) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_PWMSETDUTY, IOHWAB_E_PARAM_CHANNEL);
        return IOHWAB_NOT_OK;
    }
    if (DutyCycle > IOHWAB_PWM_DUTY_SCALE) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_PWMSETDUTY, IOHWAB_E_PARAM_VALUE);
        return IOHWAB_NOT_OK;
    }
    #endif
    
    const IoHwAb_PwmChannelConfigType* channelConfig = &IoHwAb_ConfigPtr->PwmChannels[Channel];
    
    /* Convert IoHwAb duty cycle (0-10000 = 0-100.00%) to PWM driver format (0-0x8000) */
    uint16 pwmDuty = (uint16)(((uint32)DutyCycle * 0x8000U) / IOHWAB_PWM_DUTY_SCALE);
    
    /* Set PWM duty cycle */
    Pwm_SetDutyCycle(channelConfig->PwmChannel, pwmDuty);
    
    /* Update internal buffer */
    IoHwAb_PwmBuffer[Channel].DutyCycle = DutyCycle;
    
    return IOHWAB_OK;
}

IoHwAb_ReturnType IoHwAb_PwmSetFreqAndDuty(IoHwAb_ChannelType Channel, uint32 Frequency, uint16 DutyCycle)
{
    #if (IOHWAB_DEV_ERROR_DETECT == STD_ON)
    if (IoHwAb_DriverInitialized == FALSE) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_PWMSETFREQANDDUTY, IOHWAB_E_UNINIT);
        return IOHWAB_NOT_OK;
    }
    if (Channel >= IOHWAB_NUM_PWM_CHANNELS) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_PWMSETFREQANDDUTY, IOHWAB_E_PARAM_CHANNEL);
        return IOHWAB_NOT_OK;
    }
    if (DutyCycle > IOHWAB_PWM_DUTY_SCALE) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_PWMSETFREQANDDUTY, IOHWAB_E_PARAM_VALUE);
        return IOHWAB_NOT_OK;
    }
    #endif
    
    const IoHwAb_PwmChannelConfigType* channelConfig = &IoHwAb_ConfigPtr->PwmChannels[Channel];
    
    /* Calculate period from frequency (assuming 24MHz PWM clock) */
    Pwm_PeriodType period = 24000000U / Frequency;
    
    /* Convert IoHwAb duty cycle (0-10000 = 0-100.00%) to PWM driver format (0-0x8000) */
    uint16 pwmDuty = (uint16)(((uint32)DutyCycle * 0x8000U) / IOHWAB_PWM_DUTY_SCALE);
    
    /* Set PWM period and duty cycle */
    Pwm_SetPeriodAndDuty(channelConfig->PwmChannel, period, pwmDuty);
    
    /* Update internal buffer */
    IoHwAb_PwmBuffer[Channel].Frequency = Frequency;
    IoHwAb_PwmBuffer[Channel].DutyCycle = DutyCycle;
    
    return IOHWAB_OK;
}

IoHwAb_ReturnType IoHwAb_SpiTransfer(uint8 DeviceId, const uint8* TxData, uint8* RxData, uint16 Length)
{
    #if (IOHWAB_DEV_ERROR_DETECT == STD_ON)
    if (IoHwAb_DriverInitialized == FALSE) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_SPITRANSFER, IOHWAB_E_UNINIT);
        return IOHWAB_NOT_OK;
    }
    if (DeviceId >= IOHWAB_NUM_SPI_DEVICES) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_SPITRANSFER, IOHWAB_E_PARAM_CHANNEL);
        return IOHWAB_NOT_OK;
    }
    if ((TxData == NULL_PTR) || (RxData == NULL_PTR)) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_SPITRANSFER, IOHWAB_E_PARAM_POINTER);
        return IOHWAB_NOT_OK;
    }
    #endif
    
    const IoHwAb_SpiDeviceConfigType* deviceConfig = &IoHwAb_ConfigPtr->SpiDevices[DeviceId];
    
    /* Setup external buffer for SPI transfer */
    Spi_DataBufferType txBuffer;
    txBuffer.SrcPtr = TxData;
    txBuffer.DestPtr = NULL_PTR;
    txBuffer.Length = Length;
    
    Spi_DataBufferType rxBuffer;
    rxBuffer.SrcPtr = NULL_PTR;
    rxBuffer.DestPtr = RxData;
    rxBuffer.Length = Length;
    
    /* Setup EB buffers */
    Std_ReturnType status = Spi_SetupEB(0U, &txBuffer, &rxBuffer, Length);
    
    if (status != E_OK) {
        return IOHWAB_NOT_OK;
    }
    
    /* Perform synchronous transfer */
    status = Spi_SyncTransmit(deviceConfig->SpiSequence);
    
    if (status == E_OK) {
        return IOHWAB_OK;
    }
    
    return IOHWAB_NOT_OK;
}

void IoHwAb_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (IOHWAB_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_GETVERSIONINFO, IOHWAB_E_PARAM_POINTER);
        return;
    }
    #endif
    
    versioninfo->vendorID = IOHWAB_VENDOR_ID;
    versioninfo->moduleID = IOHWAB_MODULE_ID;
    versioninfo->sw_major_version = IOHWAB_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = IOHWAB_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = IOHWAB_SW_PATCH_VERSION;
}

void IoHwAb_MainFunction(void)
{
    if (IoHwAb_DriverInitialized == FALSE) {
        return;
    }
    
    /* Periodic processing for analog channels */
    for (uint8 i = 0U; i < IoHwAb_ConfigPtr->NumAnalogChannels; i++) {
        /* Trigger ADC conversions for channels that need periodic sampling */
        /* In a real implementation, this would use DMA or interrupt-driven sampling */
    }
    
    /* Periodic processing for digital channels */
    for (uint8 i = 0U; i < IoHwAb_ConfigPtr->NumDigitalChannels; i++) {
        /* Read digital inputs and update buffers */
        IoHwAb_DigitalValueType value;
        (void)IoHwAb_DigitalRead(i, &value);
    }
    
    /* Call SPI main function for handling async operations */
    Spi_MainFunction_Handling();
}

#define IOHWAB_STOP_SEC_CODE
#include "MemMap.h"
