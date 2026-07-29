/**
 * @file I2c.h
 * @brief I2C Driver interface following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-05-01
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: I2C Driver (I2C)
 * Layer: MCAL (Microcontroller Driver Layer)
 */

#ifndef I2C_H
#define I2C_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "I2c_Cfg.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define I2C_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define I2C_MODULE_ID                   (0x57U) /* I2C Driver Module ID */
#define I2C_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define I2C_AR_RELEASE_MINOR_VERSION    (0x04U)
#define I2C_AR_RELEASE_REVISION_VERSION (0x00U)
#define I2C_SW_MAJOR_VERSION            (0x01U)
#define I2C_SW_MINOR_VERSION            (0x00U)
#define I2C_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                         CONFIGURATION VARIANTS
==================================================================================================*/
#define I2C_VARIANT_PRE_COMPILE         (0x01U)
#define I2C_VARIANT_LINK_TIME           (0x02U)
#define I2C_VARIANT_POST_BUILD          (0x03U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define I2C_SID_INIT                    (0x00U)
#define I2C_SID_DEINIT                  (0x01U)
#define I2C_SID_WRITEBYTES              (0x02U)
#define I2C_SID_READBYTES               (0x03U)
#define I2C_SID_WRITEREAD               (0x04U)
#define I2C_SID_GETSTATUS               (0x05U)
#define I2C_SID_GETVERSIONINFO          (0x06U)
#define I2C_SID_SETCLOCKMODE            (0x07U)
#define I2C_SID_ENABLEINTERRUPT         (0x08U)
#define I2C_SID_DISABLEINTERRUPT        (0x09U)
#define I2C_SID_SETSLAVEADDRESS         (0x0AU)
#define I2C_SID_GETBUSSTATE             (0x0BU)
#define I2C_SID_CLEARBUS                (0x0CU)
#define I2C_SID_SOFTWARERESET           (0x0DU)
#define I2C_SID_SETTRANSFERMODE         (0x0EU)
#define I2C_SID_MAINFUNCTION            (0x0FU)
#define I2C_SID_CANCELTRANSFER          (0x10U)
#define I2C_SID_PREPARESLAVEBUFFER      (0x11U)
#define I2C_SID_SLAVEWRITEBUFFER        (0x12U)
#define I2C_SID_SLAVEREADBUFFER         (0x13U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define I2C_E_PARAM_CHANNEL             (0x01U)
#define I2C_E_PARAM_POINTER             (0x02U)
#define I2C_E_PARAM_LENGTH              (0x03U)
#define I2C_E_PARAM_ADDRESS             (0x04U)
#define I2C_E_PARAM_MODE                (0x05U)
#define I2C_E_PARAM_CONFIG              (0x06U)
#define I2C_E_ALREADY_INITIALIZED       (0x07U)
#define I2C_E_UNINIT                    (0x08U)
#define I2C_E_BUSY                      (0x09U)
#define I2C_E_TIMEOUT                   (0x0AU)
#define I2C_E_ARBITRATION_LOST          (0x0BU)
#define I2C_E_BUS_ERROR                 (0x0CU)
#define I2C_E_ACK_ERROR                 (0x0DU)
#define I2C_E_OVERRUN                   (0x0EU)
#define I2C_E_UNDERRUN                  (0x0FU)
#define I2C_E_DMA_ERROR                 (0x10U)
#define I2C_E_INVALID_STATE             (0x11U)
#define I2C_E_CLOCK_STRETCH_TIMEOUT     (0x12U)

/*==================================================================================================
*                                    I2C STATUS TYPE
==================================================================================================*/
typedef enum {
    I2C_UNINIT = 0,
    I2C_IDLE,
    I2C_BUSY,
    I2C_ERROR
} I2c_StatusType;

/*==================================================================================================
*                                    I2C OPERATION MODE
==================================================================================================*/
typedef enum {
    I2C_MODE_MASTER = 0,
    I2C_MODE_SLAVE
} I2c_OpModeType;

/*==================================================================================================
*                                    I2C TRANSFER MODE
==================================================================================================*/
typedef enum {
    I2C_TRANSFER_POLLING = 0,
    I2C_TRANSFER_INTERRUPT,
    I2C_TRANSFER_DMA
} I2c_TransferModeType;

/*==================================================================================================
*                                    I2C ADDRESS MODE
==================================================================================================*/
typedef enum {
    I2C_ADDR_MODE_7BIT = 0,
    I2C_ADDR_MODE_10BIT,
    I2C_ADDR_MODE_16BIT
} I2c_AddrModeType;

/*==================================================================================================
*                                    I2C CLOCK MODE
==================================================================================================*/
typedef enum {
    I2C_CLOCK_STANDARD = 0,     /* 100 KHz */
    I2C_CLOCK_FAST,             /* 400 KHz */
    I2C_CLOCK_FAST_PLUS,        /* 1 MHz */
    I2C_CLOCK_HIGH_SPEED        /* 3.4 MHz */
} I2c_ClockModeType;

/*==================================================================================================
*                                    I2C BUS STATE
==================================================================================================*/
typedef enum {
    I2C_BUS_STATE_IDLE = 0,
    I2C_BUS_STATE_OWNER,
    I2C_BUS_STATE_BUSY
} I2c_BusStateType;

/*==================================================================================================
*                                    I2C RESULT TYPE
==================================================================================================*/
typedef enum {
    I2C_RESULT_OK = 0,
    I2C_RESULT_PENDING,
    I2C_RESULT_FAILED,
    I2C_RESULT_TIMEOUT,
    I2C_RESULT_CANCELLED
} I2c_ResultType;

/*==================================================================================================
*                                    I2C CHANNEL TYPE
==================================================================================================*/
typedef uint8 I2c_ChannelType;

/*==================================================================================================
*                                    I2C HW UNIT TYPE
==================================================================================================*/
typedef uint8 I2c_HWUnitType;

/*==================================================================================================
*                                    I2C ADDRESS TYPE
==================================================================================================*/
typedef uint16 I2c_AddressType;

/*==================================================================================================
*                                    I2C DATA TYPE
==================================================================================================*/
typedef uint8 I2c_DataType;

/*==================================================================================================
*                                    I2C LENGTH TYPE
==================================================================================================*/
typedef uint16 I2c_LengthType;

/*==================================================================================================
*                                    I2C NOTIFICATION FUNCTION POINTER
==================================================================================================*/
typedef void (*I2c_NotifyType)(void);

/*==================================================================================================
*                                    I2C CONFIGURATION TYPE
==================================================================================================*/

/** @brief I2C slave address configuration */
typedef struct {
    I2c_AddressType Address;
    I2c_AddrModeType AddrMode;
    boolean GeneralCallEnabled;
} I2c_SlaveAddressConfigType;

/** @brief I2C master configuration */
typedef struct {
    I2c_ClockModeType ClockMode;
    uint32 CustomClockFreq;     /* Used when ClockMode is custom */
    boolean MultiMasterEnabled;
    boolean ClockStretchingEnabled;
} I2c_MasterConfigType;

/** @brief I2C slave configuration */
typedef struct {
    I2c_SlaveAddressConfigType SlaveAddresses[I2C_MAX_SLAVE_ADDRESSES];
    uint8 NumSlaveAddresses;
    boolean DualAddressEnabled;
    boolean GeneralCallEnabled;
} I2c_SlaveConfigType;

/** @brief I2C SMBus configuration */
typedef struct {
    boolean SmbModeEnabled;
    boolean PecEnabled;
    boolean AlertEnabled;
    boolean HostNotifyEnabled;
    uint32 SmbTimeout;
} I2c_SmbusConfigType;

/** @brief I2C DMA configuration */
typedef struct {
    boolean DmaTxEnabled;
    boolean DmaRxEnabled;
    uint8 DmaTxChannel;
    uint8 DmaRxChannel;
    uint8 DmaTxInterruptPriority;
    uint8 DmaRxInterruptPriority;
} I2c_DmaConfigType;

/** @brief I2C interrupt configuration */
typedef struct {
    boolean InterruptEnabled;
    uint8 InterruptPriority;
    uint8 ErrorInterruptPriority;
} I2c_InterruptConfigType;

/** @brief I2C channel configuration */
typedef struct {
    I2c_ChannelType ChannelId;
    I2c_HWUnitType HwUnit;
    I2c_OpModeType OpMode;
    I2c_TransferModeType TransferMode;
    I2c_MasterConfigType MasterConfig;
    I2c_SlaveConfigType SlaveConfig;
    I2c_SmbusConfigType SmbusConfig;
    I2c_DmaConfigType DmaConfig;
    I2c_InterruptConfigType InterruptConfig;
    I2c_NotifyType TxNotification;
    I2c_NotifyType RxNotification;
    I2c_NotifyType ErrorNotification;
} I2c_ChannelConfigType;

/** @brief I2C global configuration */
typedef struct {
    const I2c_ChannelConfigType* Channels;
    uint8 NumChannels;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    uint32 PeripheralClockFreq;
} I2c_ConfigType;

/*==================================================================================================
*                                    I2C DATA BUFFER TYPE
==================================================================================================*/
typedef struct {
    I2c_DataType* DataPtr;
    I2c_LengthType Length;
    I2c_LengthType Transferred;
} I2c_DataBufferType;

/*==================================================================================================
*                                    I2C TRANSFER REQUEST TYPE
==================================================================================================*/
typedef struct {
    I2c_ChannelType Channel;
    I2c_AddressType SlaveAddress;
    I2c_AddrModeType AddrMode;
    I2c_DataType* TxBuffer;
    I2c_LengthType TxLength;
    I2c_DataType* RxBuffer;
    I2c_LengthType RxLength;
    boolean SendStop;
} I2c_TransferRequestType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define I2C_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const I2c_ConfigType I2c_Config;

#define I2C_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define I2C_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the I2C driver
 * @param Config Pointer to configuration structure
 */
void I2c_Init(const I2c_ConfigType* Config);

/**
 * @brief Deinitializes the I2C driver
 * @return Result of operation
 */
Std_ReturnType I2c_DeInit(void);

/**
 * @brief Writes bytes to I2C slave device
 * @param Channel I2C channel to use
 * @param SlaveAddress Slave device address
 * @param DataBuffer Pointer to data to write
 * @param Length Number of bytes to write
 * @param AddrMode Address mode (7-bit or 10-bit)
 * @return Result of operation
 */
Std_ReturnType I2c_WriteBytes(I2c_ChannelType Channel,
                               I2c_AddressType SlaveAddress,
                               const I2c_DataType* DataBuffer,
                               I2c_LengthType Length,
                               I2c_AddrModeType AddrMode);

/**
 * @brief Reads bytes from I2C slave device
 * @param Channel I2C channel to use
 * @param SlaveAddress Slave device address
 * @param DataBuffer Pointer to buffer for received data
 * @param Length Number of bytes to read
 * @param AddrMode Address mode (7-bit or 10-bit)
 * @return Result of operation
 */
Std_ReturnType I2c_ReadBytes(I2c_ChannelType Channel,
                              I2c_AddressType SlaveAddress,
                              I2c_DataType* DataBuffer,
                              I2c_LengthType Length,
                              I2c_AddrModeType AddrMode);

/**
 * @brief Combined write-read operation (write then read without stop)
 * @param Channel I2C channel to use
 * @param SlaveAddress Slave device address
 * @param TxBuffer Pointer to data to write
 * @param TxLength Number of bytes to write
 * @param RxBuffer Pointer to buffer for received data
 * @param RxLength Number of bytes to read
 * @param AddrMode Address mode (7-bit or 10-bit)
 * @return Result of operation
 */
Std_ReturnType I2c_WriteRead(I2c_ChannelType Channel,
                              I2c_AddressType SlaveAddress,
                              const I2c_DataType* TxBuffer,
                              I2c_LengthType TxLength,
                              I2c_DataType* RxBuffer,
                              I2c_LengthType RxLength,
                              I2c_AddrModeType AddrMode);

/**
 * @brief Gets I2C driver status
 * @return Current status
 */
I2c_StatusType I2c_GetStatus(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void I2c_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Sets clock mode for master operation
 * @param Channel I2C channel
 * @param ClockMode Clock mode to set
 * @return Result of operation
 */
Std_ReturnType I2c_SetClockMode(I2c_ChannelType Channel, I2c_ClockModeType ClockMode);

/**
 * @brief Enables interrupt for specified channel
 * @param Channel I2C channel
 * @return Result of operation
 */
Std_ReturnType I2c_EnableInterrupt(I2c_ChannelType Channel);

/**
 * @brief Disables interrupt for specified channel
 * @param Channel I2C channel
 * @return Result of operation
 */
Std_ReturnType I2c_DisableInterrupt(I2c_ChannelType Channel);

/**
 * @brief Sets slave address for slave mode
 * @param Channel I2C channel
 * @param SlaveAddress Slave address to set
 * @param AddrMode Address mode
 * @return Result of operation
 */
Std_ReturnType I2c_SetSlaveAddress(I2c_ChannelType Channel,
                                    I2c_AddressType SlaveAddress,
                                    I2c_AddrModeType AddrMode);

/**
 * @brief Gets current bus state
 * @param Channel I2C channel
 * @return Bus state
 */
I2c_BusStateType I2c_GetBusState(I2c_ChannelType Channel);

/**
 * @brief Clears I2C bus (sends 9 clock pulses)
 * @param Channel I2C channel
 * @return Result of operation
 */
Std_ReturnType I2c_ClearBus(I2c_ChannelType Channel);

/**
 * @brief Performs software reset of I2C module
 * @param Channel I2C channel
 * @return Result of operation
 */
Std_ReturnType I2c_SoftwareReset(I2c_ChannelType Channel);

/**
 * @brief Sets transfer mode (polling, interrupt, or DMA)
 * @param Channel I2C channel
 * @param TransferMode Transfer mode to set
 * @return Result of operation
 */
Std_ReturnType I2c_SetTransferMode(I2c_ChannelType Channel, I2c_TransferModeType TransferMode);

/**
 * @brief Cancels ongoing transfer
 * @param Channel I2C channel
 * @return Result of operation
 */
Std_ReturnType I2c_CancelTransfer(I2c_ChannelType Channel);

/**
 * @brief Prepares slave buffer for slave mode reception
 * @param Channel I2C channel
 * @param Buffer Pointer to buffer
 * @param Length Buffer length
 * @return Result of operation
 */
Std_ReturnType I2c_PrepareSlaveBuffer(I2c_ChannelType Channel,
                                       I2c_DataType* Buffer,
                                       I2c_LengthType Length);

/**
 * @brief Writes data from slave buffer
 * @param Channel I2C channel
 * @param Buffer Pointer to data buffer
 * @param Length Data length
 * @return Result of operation
 */
Std_ReturnType I2c_SlaveWriteBuffer(I2c_ChannelType Channel,
                                     const I2c_DataType* Buffer,
                                     I2c_LengthType Length);

/**
 * @brief Reads data to slave buffer
 * @param Channel I2C channel
 * @param Buffer Pointer to buffer
 * @param Length Number of bytes to read
 * @return Result of operation
 */
Std_ReturnType I2c_SlaveReadBuffer(I2c_ChannelType Channel,
                                    I2c_DataType* Buffer,
                                    I2c_LengthType Length);

/**
 * @brief Main function for I2C driver
 */
void I2c_MainFunction(void);

#define I2C_STOP_SEC_CODE
#include "MemMap.h"

#endif /* I2C_H */
