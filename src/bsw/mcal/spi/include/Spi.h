/**
 * @file Spi.h
 * @brief SPI Driver interface following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: SPI Driver (SPI)
 * Layer: MCAL (Microcontroller Driver Layer)
 */

#ifndef SPI_H
#define SPI_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Spi_Cfg.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define SPI_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define SPI_MODULE_ID                   (0x56U) /* SPI Driver Module ID */
#define SPI_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define SPI_AR_RELEASE_MINOR_VERSION    (0x04U)
#define SPI_AR_RELEASE_REVISION_VERSION (0x00U)
#define SPI_SW_MAJOR_VERSION            (0x01U)
#define SPI_SW_MINOR_VERSION            (0x00U)
#define SPI_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                         CONFIGURATION VARIANTS
==================================================================================================*/
#define SPI_VARIANT_PRE_COMPILE         (0x01U)
#define SPI_VARIANT_LINK_TIME           (0x02U)
#define SPI_VARIANT_POST_BUILD          (0x03U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define SPI_SID_INIT                    (0x00U)
#define SPI_SID_DEINIT                  (0x01U)
#define SPI_SID_WRITEIB                 (0x02U)
#define SPI_SID_ASYNCTRANSMIT           (0x03U)
#define SPI_SID_READIB                  (0x04U)
#define SPI_SID_SETUPEB                 (0x05U)
#define SPI_SID_ASYNCTRANSFER           (0x06U)
#define SPI_SID_GETSTATUS               (0x07U)
#define SPI_SID_GETJOBRESULT            (0x08U)
#define SPI_SID_GETSEQUENCERESULT       (0x09U)
#define SPI_SID_GETVERSIONINFO          (0x0AU)
#define SPI_SID_SYNCTRANSMIT            (0x0BU)
#define SPI_SID_GETHWUNITSTATUS         (0x0CU)
#define SPI_SID_CANCEL                  (0x0DU)
#define SPI_SID_SETASYNCMODE            (0x0EU)
#define SPI_SID_MAINFUNCTIONHANDLING    (0x10U)
#define SPI_SID_MAINFUNCTIONDRIVING     (0x11U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define SPI_E_PARAM_CHANNEL             (0x0AU)
#define SPI_E_PARAM_JOB                 (0x0BU)
#define SPI_E_PARAM_SEQ                 (0x0CU)
#define SPI_E_PARAM_LENGTH              (0x0DU)
#define SPI_E_PARAM_UNIT                (0x0EU)
#define SPI_E_PARAM_POINTER             (0x10U)
#define SPI_E_PARAM_CONFIG              (0x11U)
#define SPI_E_ALREADY_INITIALIZED       (0x12U)
#define SPI_E_UNINIT                    (0x1AU)
#define SPI_E_SEQ_PENDING               (0x1BU)
#define SPI_E_SEQ_IN_PROCESS            (0x1CU)
#define SPI_E_PARITY                    (0x1DU)
#define SPI_E_INVALID_DEINIT            (0x1EU)
#define SPI_E_HARDWARE_ERROR            (0x1FU)

/*==================================================================================================
*                                    SPI STATUS TYPE
==================================================================================================*/
typedef enum {
    SPI_UNINIT = 0,
    SPI_IDLE,
    SPI_BUSY
} Spi_StatusType;

/*==================================================================================================
*                                    SPI JOB RESULT TYPE
==================================================================================================*/
typedef enum {
    SPI_JOB_OK = 0,
    SPI_JOB_PENDING,
    SPI_JOB_FAILED,
    SPI_JOB_QUEUED
} Spi_JobResultType;

/*==================================================================================================
*                                    SPI SEQUENCE RESULT TYPE
==================================================================================================*/
typedef enum {
    SPI_SEQ_OK = 0,
    SPI_SEQ_PENDING,
    SPI_SEQ_FAILED,
    SPI_SEQ_CANCELLED
} Spi_SeqResultType;

/*==================================================================================================
*                                    SPI DATA BUFFER TYPE
==================================================================================================*/
typedef enum {
    SPI_IB_BUFFER = 0,
    SPI_EB_BUFFER
} Spi_BufferType;

/*==================================================================================================
*                                    SPI TRANSFER MODE
==================================================================================================*/
typedef enum {
    SPI_POLLING_MODE = 0,
    SPI_INTERRUPT_MODE
} Spi_AsyncModeType;

/*==================================================================================================
*                                    SPI CHANNEL TYPE
==================================================================================================*/
typedef uint8 Spi_ChannelType;

/*==================================================================================================
*                                    SPI JOB TYPE
==================================================================================================*/
typedef uint16 Spi_JobType;

/*==================================================================================================
*                                    SPI SEQUENCE TYPE
==================================================================================================*/
typedef uint8 Spi_SequenceType;

/*==================================================================================================
*                                    SPI HW UNIT TYPE
==================================================================================================*/
typedef uint8 Spi_HWUnitType;

/*==================================================================================================
*                                    SPI NUMBER OF DATA ELEMENTS TYPE
==================================================================================================*/
typedef uint16 Spi_NumberOfDataType;

/*==================================================================================================
*                                    SPI DATA BUFFER TYPE
==================================================================================================*/
typedef struct {
    uint8* DestPtr;
    const uint8* SrcPtr;
    Spi_NumberOfDataType Length;
} Spi_DataBufferType;

/*==================================================================================================
*                                    SPI CHANNEL CONFIG TYPE
==================================================================================================*/
typedef struct {
    Spi_ChannelType ChannelId;
    uint32 DefaultData;
    Spi_NumberOfDataType DataWidth;
    Spi_NumberOfDataType MaxDataLength;
    Spi_BufferType BufferType;
    boolean TransferStart;
} Spi_ChannelConfigType;

/*==================================================================================================
*                                    SPI JOB CONFIG TYPE
==================================================================================================*/
typedef struct {
    Spi_JobType JobId;
    uint32 HwUnit;
    uint32 ChipSelect;
    uint32 Baudrate;
    uint32 TimeCs2Clk;
    uint32 TimeClk2Cs;
    uint32 TimeCs2Cs;
    const Spi_ChannelType* Channels;
    uint8 NumChannels;
    boolean CsPolarity;
    uint32 SpiDataShiftEdge;
    uint32 SpiShiftClockIdleLevel;
} Spi_JobConfigType;

/*==================================================================================================
*                                    SPI SEQUENCE CONFIG TYPE
==================================================================================================*/
typedef struct {
    Spi_SequenceType SequenceId;
    const Spi_JobType* Jobs;
    uint8 NumJobs;
    boolean Interruptible;
} Spi_SequenceConfigType;

/*==================================================================================================
*                                    SPI EXTERNAL DEVICE CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint32 DeviceId;
    uint32 CsPin;
    uint32 Baudrate;
    uint32 DataWidth;
    uint32 ShiftClockIdleLevel;
    uint32 DataShiftEdge;
    uint32 TimeCs2Clk;
    uint32 TimeClk2Cs;
    uint32 TimeCs2Cs;
} Spi_ExternalDeviceType;

/*==================================================================================================
*                                    SPI CONFIG TYPE
==================================================================================================*/
typedef struct {
    const Spi_ChannelConfigType* Channels;
    uint8 NumChannels;
    const Spi_JobConfigType* Jobs;
    uint8 NumJobs;
    const Spi_SequenceConfigType* Sequences;
    uint8 NumSequences;
    const Spi_ExternalDeviceType* ExternalDevices;
    uint8 NumExternalDevices;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean InterruptibleSeqAllowed;
    Spi_AsyncModeType AsyncMode;
    uint32 MaxBufferSize;
} Spi_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define SPI_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Spi_ConfigType Spi_Config;

#define SPI_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define SPI_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the SPI driver
 * @param Config Pointer to configuration structure
 */
void Spi_Init(const Spi_ConfigType* Config);

/**
 * @brief Deinitializes the SPI driver
 * @return Result of operation
 */
Std_ReturnType Spi_DeInit(void);

/**
 * @brief Writes data to IB buffer
 * @param Channel Channel to write
 * @param DataBufferPtr Pointer to data buffer
 */
void Spi_WriteIB(Spi_ChannelType Channel, const Spi_DataBufferType* DataBufferPtr);

/**
 * @brief Asynchronous transmit of a sequence
 * @param Sequence Sequence to transmit
 * @return Result of operation
 */
Std_ReturnType Spi_AsyncTransmit(Spi_SequenceType Sequence);

/**
 * @brief Reads data from IB buffer
 * @param Channel Channel to read
 * @param DataBufferPtr Pointer to data buffer
 */
void Spi_ReadIB(Spi_ChannelType Channel, Spi_DataBufferType* DataBufferPtr);

/**
 * @brief Sets up external buffer
 * @param Channel Channel to setup
 * @param SrcDataBufferPtr Source buffer pointer
 * @param DesDataBufferPtr Destination buffer pointer
 * @param Length Data length
 * @return Result of operation
 */
Std_ReturnType Spi_SetupEB(Spi_ChannelType Channel,
                           const Spi_DataBufferType* SrcDataBufferPtr,
                           Spi_DataBufferType* DesDataBufferPtr,
                           Spi_NumberOfDataType Length);

/**
 * @brief Gets SPI driver status
 * @return Current status
 */
Spi_StatusType Spi_GetStatus(void);

/**
 * @brief Gets job result
 * @param Job Job to check
 * @return Job result
 */
Spi_JobResultType Spi_GetJobResult(Spi_JobType Job);

/**
 * @brief Gets sequence result
 * @param Seq Sequence to check
 * @return Sequence result
 */
Spi_SeqResultType Spi_GetSequenceResult(Spi_SequenceType Seq);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void Spi_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Synchronous transmit of a sequence
 * @param Sequence Sequence to transmit
 * @return Result of operation
 */
Std_ReturnType Spi_SyncTransmit(Spi_SequenceType Sequence);

/**
 * @brief Gets hardware unit status
 * @param HWUnit Hardware unit to check
 * @return Hardware unit status
 */
Spi_StatusType Spi_GetHWUnitStatus(Spi_HWUnitType HWUnit);

/**
 * @brief Cancels a sequence
 * @param Sequence Sequence to cancel
 */
void Spi_Cancel(Spi_SequenceType Sequence);

/**
 * @brief Sets asynchronous mode
 * @param Mode Asynchronous mode to set
 */
void Spi_SetAsyncMode(Spi_AsyncModeType Mode);

/**
 * @brief Main function for handling
 */
void Spi_MainFunction_Handling(void);

/**
 * @brief Main function for driving
 */
void Spi_MainFunction_Driving(void);

#define SPI_STOP_SEC_CODE
#include "MemMap.h"

#endif /* SPI_H */
