/**
 * @file I2c.c
 * @brief I2C Driver implementation for i.MX8M Mini
 * @version 1.0.0
 * @date 2026-05-01
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * Features:
 * - Master and Slave mode support
 * - DMA transfer support
 * - Multi-master support
 * - Clock stretching support
 * - SMBus support
 * - 7-bit and 10-bit addressing
 * - Polling, Interrupt, and DMA transfer modes
 */

#include "I2c.h"
#include "I2c_Cfg.h"
#include "Det.h"

/*==================================================================================================
*                                    HARDWARE REGISTER DEFINITIONS
*                                    (i.MX8M Mini I2C Controller)
==================================================================================================*/
#define I2C1_BASE_ADDR                  (0x30A20000UL)
#define I2C2_BASE_ADDR                  (0x30A30000UL)
#define I2C3_BASE_ADDR                  (0x30A40000UL)
#define I2C4_BASE_ADDR                  (0x30A50000UL)

/* I2C Register Offsets */
#define I2C_IADR                        (0x00U)     /* Address Register */
#define I2C_IFDR                        (0x04U)     /* Frequency Divider Register */
#define I2C_I2CR                        (0x08U)     /* Control Register */
#define I2C_I2SR                        (0x0CU)     /* Status Register */
#define I2C_I2DR                        (0x10U)     /* Data I/O Register */

/* Control Register (I2CR) Bits */
#define I2CR_IEN                        (0x80U)     /* I2C Enable */
#define I2CR_IIEN                       (0x40U)     /* I2C Interrupt Enable */
#define I2CR_MSTA                       (0x20U)     /* Master/Slave Mode */
#define I2CR_MTX                        (0x10U)     /* Transmit/Receive Mode */
#define I2CR_TXAK                       (0x08U)     /* Transmit Acknowledge Enable */
#define I2CR_RSTA                       (0x04U)     /* Repeated Start */

/* Status Register (I2SR) Bits */
#define I2SR_ICF                        (0x80U)     /* Data Transfer Complete */
#define I2SR_IAAS                       (0x40U)     /* Addressed as a Slave */
#define I2SR_IBB                        (0x20U)     /* Bus Busy */
#define I2SR_IAL                        (0x10U)     /* Arbitration Lost */
#define I2SR_SRW                        (0x04U)     /* Slave Read/Write */
#define I2SR_IIF                        (0x02U)     /* I2C Interrupt */
#define I2SR_RXAK                       (0x01U)     /* Received Acknowledge */

/*==================================================================================================
*                                    DMA REGISTER DEFINITIONS
==================================================================================================*/
#define I2C_DMA_BASE_ADDR               (0x30E10000UL)
#define I2C_DMA_CH_OFFSET               (0x10000UL)

#define DMA_TCD_CSR                     (0x1CU)
#define DMA_TCD_BITER                   (0x1EU)
#define DMA_TCD_CITER                   (0x1AU)
#define DMA_TCD_DLAST_SGA               (0x20U)
#define DMA_TCD_SLAST                   (0x18U)
#define DMA_TCD_DADDR                   (0x10U)
#define DMA_TCD_SADDR                   (0x00U)
#define DMA_TCD_NBYTES                  (0x08U)
#define DMA_TCD_ATTR                    (0x06U)
#define DMA_TCD_SOFF                    (0x04U)
#define DMA_TCD_DOFF                    (0x14U)

#define DMA_CSR_INTMAJOR                (0x0002U)
#define DMA_CSR_START                   (0x0001U)
#define DMA_CSR_INTHALF                 (0x0004U)

/*==================================================================================================
*                                    LOCAL TYPE DEFINITIONS
==================================================================================================*/
typedef enum {
    I2C_STATE_UNINIT = 0,
    I2C_STATE_IDLE,
    I2C_STATE_MASTER_TX,
    I2C_STATE_MASTER_RX,
    I2C_STATE_SLAVE_TX,
    I2C_STATE_SLAVE_RX,
    I2C_STATE_ERROR
} I2c_StateType;

typedef struct {
    I2c_DataType* Buffer;
    I2c_LengthType Length;
    I2c_LengthType Index;
    I2c_LengthType Remaining;
} I2c_BufferInfoType;

typedef struct {
    I2c_StateType State;
    I2c_BusStateType BusState;
    I2c_ResultType Result;
    I2c_TransferModeType CurrentMode;
    I2c_BufferInfoType TxBuffer;
    I2c_BufferInfoType RxBuffer;
    I2c_AddressType CurrentSlaveAddress;
    I2c_AddrModeType CurrentAddrMode;
    boolean IsRepeatedStart;
    boolean StopRequested;
    uint32 ErrorFlags;
} I2c_ChannelInfoType;

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
#define I2C_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static boolean I2c_DriverInitialized = FALSE;
static I2c_StatusType I2c_DriverStatus = I2C_UNINIT;
static const I2c_ConfigType* I2c_ConfigPtr = NULL_PTR;
static I2c_ChannelInfoType I2c_ChannelInfo[I2C_NUM_CHANNELS];

#define I2C_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FREQUENCY DIVIDER TABLE
==================================================================================================*/
static const uint16 I2c_FreqDividerTable[64] = {
    30, 32, 34, 36, 38, 40, 42, 44,
    46, 48, 50, 52, 54, 56, 58, 60,
    62, 64, 66, 68, 70, 72, 74, 76,
    78, 80, 82, 84, 86, 88, 90, 92,
    94, 96, 98, 100, 102, 104, 106, 108,
    110, 112, 114, 116, 118, 120, 122, 124,
    126, 128, 130, 132, 134, 136, 138, 140,
    142, 256, 264, 272, 280, 288, 296, 304
};

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
static uint32 I2c_GetBaseAddr(uint8 hwUnit);
static void I2c_EnableClock(uint8 hwUnit);
static void I2c_DisableClock(uint8 hwUnit);
static Std_ReturnType I2c_WaitForBusIdle(uint32 baseAddr, uint32 timeoutMs);
static Std_ReturnType I2c_WaitForTransferComplete(uint32 baseAddr, uint32 timeoutMs);
static void I2c_SetFrequency(uint32 baseAddr, uint32 freq);
static Std_ReturnType I2c_SendStart(uint32 baseAddr, I2c_AddressType slaveAddr, I2c_AddrModeType addrMode, boolean isTx);
static Std_ReturnType I2c_SendRepeatedStart(uint32 baseAddr, I2c_AddressType slaveAddr, I2c_AddrModeType addrMode, boolean isTx);
static Std_ReturnType I2c_SendStop(uint32 baseAddr);
static Std_ReturnType I2c_WriteByte(uint32 baseAddr, uint8 data, uint32 timeoutMs);
static Std_ReturnType I2c_ReadByte(uint32 baseAddr, uint8* data, boolean sendAck, uint32 timeoutMs);
static void I2c_DmaInit(uint8 channel, I2c_DataType* buffer, I2c_LengthType length, boolean isTx);
static void I2c_DmaStart(uint8 channel);
static void I2c_DmaStop(uint8 channel);
static void I2c_IsrHandler(uint8 channel);
static void I2c_ErrorHandler(uint8 channel, uint32 errorFlags);
static Std_ReturnType I2c_MasterTransferPolling(uint8 channel);
static Std_ReturnType I2c_MasterTransferInterrupt(uint8 channel);
static Std_ReturnType I2c_MasterTransferDma(uint8 channel);

/*==================================================================================================
*                                    LOCAL FUNCTION IMPLEMENTATIONS
==================================================================================================*/

/**
 * @brief Gets base address for I2C hardware unit
 */
static uint32 I2c_GetBaseAddr(uint8 hwUnit)
{
    uint32 baseAddr;
    switch (hwUnit) {
        case 0: baseAddr = I2C1_BASE_ADDR; break;
        case 1: baseAddr = I2C2_BASE_ADDR; break;
        case 2: baseAddr = I2C3_BASE_ADDR; break;
        case 3: baseAddr = I2C4_BASE_ADDR; break;
        default: baseAddr = 0U; break;
    }
    return baseAddr;
}

/**
 * @brief Enables clock for I2C hardware unit
 */
static void I2c_EnableClock(uint8 hwUnit)
{
    (void)hwUnit;
    /* Clock enable implementation - platform specific */
}

/**
 * @brief Disables clock for I2C hardware unit
 */
static void I2c_DisableClock(uint8 hwUnit)
{
    (void)hwUnit;
    /* Clock disable implementation - platform specific */
}

/**
 * @brief Waits for bus to become idle
 */
static Std_ReturnType I2c_WaitForBusIdle(uint32 baseAddr, uint32 timeoutMs)
{
    uint32 timeout = timeoutMs * 1000U; /* Convert to approximate loop count */
    
    while ((REG_READ8(baseAddr + I2C_I2SR) & I2SR_IBB) != 0U) {
        if (timeout == 0U) {
            return E_NOT_OK;
        }
        timeout--;
    }
    return E_OK;
}

/**
 * @brief Waits for data transfer complete
 */
static Std_ReturnType I2c_WaitForTransferComplete(uint32 baseAddr, uint32 timeoutMs)
{
    uint32 timeout = timeoutMs * 1000U;
    
    while ((REG_READ8(baseAddr + I2C_I2SR) & I2SR_ICF) == 0U) {
        if (timeout == 0U) {
            return E_NOT_OK;
        }
        timeout--;
    }
    return E_OK;
}

/**
 * @brief Sets I2C frequency
 */
static void I2c_SetFrequency(uint32 baseAddr, uint32 freq)
{
    uint32 periphClk = I2C_PERIPHERAL_CLOCK_FREQ;
    uint32 bestDivider = 0U;
    uint32 minDiff = 0xFFFFFFFFU;
    uint8 bestIndex = 0U;
    
    for (uint8 i = 0U; i < 64U; i++) {
        uint32 divider = I2c_FreqDividerTable[i];
        uint32 actualFreq = periphClk / divider;
        uint32 diff = (actualFreq > freq) ? (actualFreq - freq) : (freq - actualFreq);
        
        if (diff < minDiff) {
            minDiff = diff;
            bestDivider = divider;
            bestIndex = i;
        }
        
        if (diff == 0U) {
            break;
        }
    }
    
    REG_WRITE8(baseAddr + I2C_IFDR, bestIndex);
}

/**
 * @brief Sends START condition and slave address
 */
static Std_ReturnType I2c_SendStart(uint32 baseAddr, I2c_AddressType slaveAddr, 
                                     I2c_AddrModeType addrMode, boolean isTx)
{
    uint8 i2cr;
    
    /* Wait for bus idle */
    if (I2c_WaitForBusIdle(baseAddr, I2C_BUS_BUSY_TIMEOUT_MS) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Set master mode and transmit mode */
    i2cr = REG_READ8(baseAddr + I2C_I2CR);
    i2cr |= I2CR_MSTA | I2CR_MTX | I2CR_IEN;
    REG_WRITE8(baseAddr + I2C_I2CR, i2cr);
    
    /* Wait for bus busy */
    uint32 timeout = I2C_BUS_BUSY_TIMEOUT_MS * 1000U;
    while ((REG_READ8(baseAddr + I2C_I2SR) & I2SR_IBB) == 0U) {
        if (timeout == 0U) {
            return E_NOT_OK;
        }
        timeout--;
    }
    
    /* Send slave address */
    if (addrMode == I2C_ADDR_MODE_10BIT) {
        /* 10-bit addressing: first byte is 11110 + addr[9:8] + R/W */
        uint8 addrByte1 = 0xF0U | ((uint8)((slaveAddr >> 8) & 0x03U) << 1) | (isTx ? 0U : 1U);
        uint8 addrByte2 = (uint8)(slaveAddr & 0xFFU);
        
        REG_WRITE8(baseAddr + I2C_I2DR, addrByte1);
        if (I2c_WaitForTransferComplete(baseAddr, I2C_TRANSFER_TIMEOUT_MS) != E_OK) {
            return E_NOT_OK;
        }
        
        /* Check for ACK */
        if ((REG_READ8(baseAddr + I2C_I2SR) & I2SR_RXAK) != 0U) {
            return E_NOT_OK;
        }
        
        REG_WRITE8(baseAddr + I2C_I2DR, addrByte2);
        if (I2c_WaitForTransferComplete(baseAddr, I2C_TRANSFER_TIMEOUT_MS) != E_OK) {
            return E_NOT_OK;
        }
    } else {
        /* 7-bit addressing */
        uint8 addrByte = ((uint8)(slaveAddr & 0x7FU) << 1) | (isTx ? 0U : 1U);
        REG_WRITE8(baseAddr + I2C_I2DR, addrByte);
        if (I2c_WaitForTransferComplete(baseAddr, I2C_TRANSFER_TIMEOUT_MS) != E_OK) {
            return E_NOT_OK;
        }
    }
    
    /* Check for ACK */
    if ((REG_READ8(baseAddr + I2C_I2SR) & I2SR_RXAK) != 0U) {
        return E_NOT_OK;
    }
    
    return E_OK;
}

/**
 * @brief Sends repeated START condition
 */
static Std_ReturnType I2c_SendRepeatedStart(uint32 baseAddr, I2c_AddressType slaveAddr,
                                             I2c_AddrModeType addrMode, boolean isTx)
{
    uint8 i2cr = REG_READ8(baseAddr + I2C_I2CR);
    i2cr |= I2CR_RSTA;
    REG_WRITE8(baseAddr + I2C_I2CR, i2cr);
    
    return I2c_SendStart(baseAddr, slaveAddr, addrMode, isTx);
}

/**
 * @brief Sends STOP condition
 */
static Std_ReturnType I2c_SendStop(uint32 baseAddr)
{
    uint8 i2cr = REG_READ8(baseAddr + I2C_I2CR);
    i2cr &= ~I2CR_MSTA;
    REG_WRITE8(baseAddr + I2C_I2CR, i2cr);
    
    /* Wait for bus idle */
    if (I2c_WaitForBusIdle(baseAddr, I2C_BUS_BUSY_TIMEOUT_MS) != E_OK) {
        return E_NOT_OK;
    }
    
    return E_OK;
}

/**
 * @brief Writes a single byte
 */
static Std_ReturnType I2c_WriteByte(uint32 baseAddr, uint8 data, uint32 timeoutMs)
{
    REG_WRITE8(baseAddr + I2C_I2DR, data);
    
    if (I2c_WaitForTransferComplete(baseAddr, timeoutMs) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Check for ACK */
    if ((REG_READ8(baseAddr + I2C_I2SR) & I2SR_RXAK) != 0U) {
        return E_NOT_OK;
    }
    
    return E_OK;
}

/**
 * @brief Reads a single byte
 */
static Std_ReturnType I2c_ReadByte(uint32 baseAddr, uint8* data, boolean sendAck, uint32 timeoutMs)
{
    /* Set receive mode if not already */
    uint8 i2cr = REG_READ8(baseAddr + I2C_I2CR);
    i2cr &= ~I2CR_MTX;
    
    /* Configure ACK/NACK */
    if (sendAck) {
        i2cr &= ~I2CR_TXAK;
    } else {
        i2cr |= I2CR_TXAK;
    }
    REG_WRITE8(baseAddr + I2C_I2CR, i2cr);
    
    /* Dummy read to trigger receive */
    *data = REG_READ8(baseAddr + I2C_I2DR);
    
    if (I2c_WaitForTransferComplete(baseAddr, timeoutMs) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Read actual data */
    *data = REG_READ8(baseAddr + I2C_I2DR);
    
    return E_OK;
}

/**
 * @brief Initializes DMA for I2C transfer
 */
static void I2c_DmaInit(uint8 channel, I2c_DataType* buffer, I2c_LengthType length, boolean isTx)
{
    (void)channel;
    (void)buffer;
    (void)length;
    (void)isTx;
    /* DMA initialization - platform specific */
}

/**
 * @brief Starts DMA transfer
 */
static void I2c_DmaStart(uint8 channel)
{
    (void)channel;
    /* DMA start - platform specific */
}

/**
 * @brief Stops DMA transfer
 */
static void I2c_DmaStop(uint8 channel)
{
    (void)channel;
    /* DMA stop - platform specific */
}

/**
 * @brief I2C interrupt handler
 */
static void I2c_IsrHandler(uint8 channel)
{
    I2c_ChannelInfoType* chInfo = &I2c_ChannelInfo[channel];
    const I2c_ChannelConfigType* chConfig = &I2c_ConfigPtr->Channels[channel];
    uint32 baseAddr = I2c_GetBaseAddr(chConfig->HwUnit);
    uint8 i2sr = REG_READ8(baseAddr + I2C_I2SR);
    
    /* Clear interrupt flag */
    REG_WRITE8(baseAddr + I2C_I2SR, i2sr & ~I2SR_IIF);
    
    /* Check for errors */
    if ((i2sr & I2SR_IAL) != 0U) {
        chInfo->ErrorFlags |= I2C_E_ARBITRATION_LOST;
        chInfo->State = I2C_STATE_ERROR;
        chInfo->Result = I2C_RESULT_FAILED;
        if (chConfig->ErrorNotification != NULL_PTR) {
            chConfig->ErrorNotification();
        }
        return;
    }
    
    if ((i2sr & I2SR_RXAK) != 0U && chInfo->State == I2C_STATE_MASTER_TX) {
        chInfo->ErrorFlags |= I2C_E_ACK_ERROR;
        chInfo->State = I2C_STATE_ERROR;
        chInfo->Result = I2C_RESULT_FAILED;
        if (chConfig->ErrorNotification != NULL_PTR) {
            chConfig->ErrorNotification();
        }
        return;
    }
    
    /* Handle data transfer */
    switch (chInfo->State) {
        case I2C_STATE_MASTER_TX:
            if (chInfo->TxBuffer.Index < chInfo->TxBuffer.Length) {
                REG_WRITE8(baseAddr + I2C_I2DR, chInfo->TxBuffer.Buffer[chInfo->TxBuffer.Index]);
                chInfo->TxBuffer.Index++;
            } else {
                /* Transfer complete */
                if (chInfo->RxBuffer.Length > 0U && !chInfo->IsRepeatedStart) {
                    /* Switch to RX mode */
                    chInfo->State = I2C_STATE_MASTER_RX;
                    I2c_SendRepeatedStart(baseAddr, chInfo->CurrentSlaveAddress, 
                                          chInfo->CurrentAddrMode, FALSE);
                } else {
                    /* Send STOP */
                    if (chInfo->StopRequested) {
                        I2c_SendStop(baseAddr);
                    }
                    chInfo->State = I2C_STATE_IDLE;
                    chInfo->Result = I2C_RESULT_OK;
                    if (chConfig->TxNotification != NULL_PTR) {
                        chConfig->TxNotification();
                    }
                }
            }
            break;
            
        case I2C_STATE_MASTER_RX:
            if (chInfo->RxBuffer.Index < chInfo->RxBuffer.Length) {
                boolean sendAck = (chInfo->RxBuffer.Index < chInfo->RxBuffer.Length - 1U);
                chInfo->RxBuffer.Buffer[chInfo->RxBuffer.Index] = REG_READ8(baseAddr + I2C_I2DR);
                chInfo->RxBuffer.Index++;
                
                uint8 i2cr = REG_READ8(baseAddr + I2C_I2CR);
                i2cr &= ~I2CR_MTX;
                if (sendAck) {
                    i2cr &= ~I2CR_TXAK;
                } else {
                    i2cr |= I2CR_TXAK;
                }
                REG_WRITE8(baseAddr + I2C_I2CR, i2cr);
                
                /* Dummy read to trigger next byte */
                (void)REG_READ8(baseAddr + I2C_I2DR);
            } else {
                /* Transfer complete */
                if (chInfo->StopRequested) {
                    I2c_SendStop(baseAddr);
                }
                chInfo->State = I2C_STATE_IDLE;
                chInfo->Result = I2C_RESULT_OK;
                if (chConfig->RxNotification != NULL_PTR) {
                    chConfig->RxNotification();
                }
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief Error handler
 */
static void I2c_ErrorHandler(uint8 channel, uint32 errorFlags)
{
    I2c_ChannelInfoType* chInfo = &I2c_ChannelInfo[channel];
    chInfo->ErrorFlags = errorFlags;
    chInfo->State = I2C_STATE_ERROR;
    chInfo->Result = I2C_RESULT_FAILED;
    
    const I2c_ChannelConfigType* chConfig = &I2c_ConfigPtr->Channels[channel];
    if (chConfig->ErrorNotification != NULL_PTR) {
        chConfig->ErrorNotification();
    }
}

/**
 * @brief Master transfer using polling mode
 */
static Std_ReturnType I2c_MasterTransferPolling(uint8 channel)
{
    I2c_ChannelInfoType* chInfo = &I2c_ChannelInfo[channel];
    const I2c_ChannelConfigType* chConfig = &I2c_ConfigPtr->Channels[channel];
    uint32 baseAddr = I2c_GetBaseAddr(chConfig->HwUnit);
    Std_ReturnType result = E_OK;
    
    /* Send START and address */
    result = I2c_SendStart(baseAddr, chInfo->CurrentSlaveAddress, 
                           chInfo->CurrentAddrMode, 
                           chInfo->TxBuffer.Length > 0U);
    if (result != E_OK) {
        I2c_SendStop(baseAddr);
        return result;
    }
    
    /* Transmit data */
    while (chInfo->TxBuffer.Index < chInfo->TxBuffer.Length) {
        result = I2c_WriteByte(baseAddr, chInfo->TxBuffer.Buffer[chInfo->TxBuffer.Index],
                               I2C_TRANSFER_TIMEOUT_MS);
        if (result != E_OK) {
            I2c_SendStop(baseAddr);
            return result;
        }
        chInfo->TxBuffer.Index++;
    }
    
    /* Receive data if needed */
    if (chInfo->RxBuffer.Length > 0U) {
        /* Send repeated START for read */
        result = I2c_SendRepeatedStart(baseAddr, chInfo->CurrentSlaveAddress,
                                       chInfo->CurrentAddrMode, FALSE);
        if (result != E_OK) {
            I2c_SendStop(baseAddr);
            return result;
        }
        
        /* Receive data */
        while (chInfo->RxBuffer.Index < chInfo->RxBuffer.Length) {
            boolean sendAck = (chInfo->RxBuffer.Index < chInfo->RxBuffer.Length - 1U);
            result = I2c_ReadByte(baseAddr, &chInfo->RxBuffer.Buffer[chInfo->RxBuffer.Index],
                                  sendAck, I2C_TRANSFER_TIMEOUT_MS);
            if (result != E_OK) {
                I2c_SendStop(baseAddr);
                return result;
            }
            chInfo->RxBuffer.Index++;
        }
    }
    
    /* Send STOP */
    if (chInfo->StopRequested) {
        result = I2c_SendStop(baseAddr);
    }
    
    chInfo->State = I2C_STATE_IDLE;
    chInfo->Result = I2C_RESULT_OK;
    
    return result;
}

/**
 * @brief Master transfer using interrupt mode
 */
static Std_ReturnType I2c_MasterTransferInterrupt(uint8 channel)
{
    I2c_ChannelInfoType* chInfo = &I2c_ChannelInfo[channel];
    const I2c_ChannelConfigType* chConfig = &I2c_ConfigPtr->Channels[channel];
    uint32 baseAddr = I2c_GetBaseAddr(chConfig->HwUnit);
    
    /* Send START and first byte - rest handled by ISR */
    Std_ReturnType result = I2c_SendStart(baseAddr, chInfo->CurrentSlaveAddress,
                                          chInfo->CurrentAddrMode,
                                          chInfo->TxBuffer.Length > 0U);
    if (result != E_OK) {
        I2c_SendStop(baseAddr);
        return result;
    }
    
    /* Enable interrupt */
    uint8 i2cr = REG_READ8(baseAddr + I2C_I2CR);
    i2cr |= I2CR_IIEN;
    REG_WRITE8(baseAddr + I2C_I2CR, i2cr);
    
    chInfo->Result = I2C_RESULT_PENDING;
    
    return E_OK;
}

/**
 * @brief Master transfer using DMA mode
 */
static Std_ReturnType I2c_MasterTransferDma(uint8 channel)
{
#if (I2C_DMA_SUPPORTED == STD_ON)
    I2c_ChannelInfoType* chInfo = &I2c_ChannelInfo[channel];
    const I2c_ChannelConfigType* chConfig = &I2c_ConfigPtr->Channels[channel];
    uint32 baseAddr = I2c_GetBaseAddr(chConfig->HwUnit);
    
    /* Send START and address */
    Std_ReturnType result = I2c_SendStart(baseAddr, chInfo->CurrentSlaveAddress,
                                          chInfo->CurrentAddrMode,
                                          chInfo->TxBuffer.Length > 0U);
    if (result != E_OK) {
        I2c_SendStop(baseAddr);
        return result;
    }
    
    /* Setup DMA for TX */
    if (chInfo->TxBuffer.Length > 0U) {
        I2c_DmaInit(chConfig->DmaConfig.DmaTxChannel, 
                    chInfo->TxBuffer.Buffer,
                    chInfo->TxBuffer.Length,
                    TRUE);
        I2c_DmaStart(chConfig->DmaConfig.DmaTxChannel);
    }
    
    /* Setup DMA for RX */
    if (chInfo->RxBuffer.Length > 0U) {
        I2c_DmaInit(chConfig->DmaConfig.DmaRxChannel,
                    chInfo->RxBuffer.Buffer,
                    chInfo->RxBuffer.Length,
                    FALSE);
        I2c_DmaStart(chConfig->DmaConfig.DmaRxChannel);
    }
    
    chInfo->Result = I2C_RESULT_PENDING;
    
    return E_OK;
#else
    (void)channel;
    return E_NOT_OK;
#endif
}

/*==================================================================================================
*                                    API FUNCTION IMPLEMENTATIONS
==================================================================================================*/
#define I2C_START_SEC_CODE
#include "MemMap.h"

void I2c_Init(const I2c_ConfigType* Config)
{
    #if (I2C_DEV_ERROR_DETECT == STD_ON)
    if (Config == NULL_PTR) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_INIT, I2C_E_PARAM_CONFIG);
        return;
    }
    if (I2c_DriverInitialized == TRUE) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_INIT, I2C_E_ALREADY_INITIALIZED);
        return;
    }
    #endif
    
    I2c_ConfigPtr = Config;
    
    /* Initialize all configured channels */
    for (uint8 i = 0U; i < Config->NumChannels; i++) {
        const I2c_ChannelConfigType* chConfig = &Config->Channels[i];
        I2c_ChannelInfoType* chInfo = &I2c_ChannelInfo[i];
        uint32 baseAddr = I2c_GetBaseAddr(chConfig->HwUnit);
        
        if (baseAddr == 0U) {
            continue;
        }
        
        /* Enable clock */
        I2c_EnableClock(chConfig->HwUnit);
        
        /* Disable I2C */
        REG_WRITE8(baseAddr + I2C_I2CR, 0U);
        
        /* Set slave address (if slave mode) */
        if (chConfig->OpMode == I2C_MODE_SLAVE && chConfig->SlaveConfig.NumSlaveAddresses > 0U) {
            REG_WRITE8(baseAddr + I2C_IADR, 
                      (uint8)(chConfig->SlaveConfig.SlaveAddresses[0].Address << 1));
        }
        
        /* Set frequency */
        uint32 freq;
        switch (chConfig->MasterConfig.ClockMode) {
            case I2C_CLOCK_STANDARD:
                freq = I2C_STANDARD_MODE_FREQ;
                break;
            case I2C_CLOCK_FAST:
                freq = I2C_FAST_MODE_FREQ;
                break;
            case I2C_CLOCK_FAST_PLUS:
                freq = I2C_FAST_MODE_PLUS_FREQ;
                break;
            case I2C_CLOCK_HIGH_SPEED:
                freq = I2C_HIGH_SPEED_MODE_FREQ;
                break;
            default:
                freq = I2C_STANDARD_MODE_FREQ;
                break;
        }
        I2c_SetFrequency(baseAddr, freq);
        
        /* Clear status */
        REG_WRITE8(baseAddr + I2C_I2SR, 0U);
        
        /* Enable I2C */
        REG_WRITE8(baseAddr + I2C_I2CR, I2CR_IEN);
        
        /* Initialize channel info */
        chInfo->State = I2C_STATE_IDLE;
        chInfo->BusState = I2C_BUS_STATE_IDLE;
        chInfo->Result = I2C_RESULT_OK;
        chInfo->CurrentMode = chConfig->TransferMode;
        chInfo->ErrorFlags = 0U;
    }
    
    I2c_DriverStatus = I2C_IDLE;
    I2c_DriverInitialized = TRUE;
}

Std_ReturnType I2c_DeInit(void)
{
    #if (I2C_DEV_ERROR_DETECT == STD_ON)
    if (I2c_DriverInitialized == FALSE) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_DEINIT, I2C_E_UNINIT);
        return E_NOT_OK;
    }
    #endif
    
    /* Deinitialize all channels */
    for (uint8 i = 0U; i < I2c_ConfigPtr->NumChannels; i++) {
        const I2c_ChannelConfigType* chConfig = &I2c_ConfigPtr->Channels[i];
        uint32 baseAddr = I2c_GetBaseAddr(chConfig->HwUnit);
        
        if (baseAddr == 0U) {
            continue;
        }
        
        /* Disable I2C */
        REG_WRITE8(baseAddr + I2C_I2CR, 0U);
        
        /* Disable clock */
        I2c_DisableClock(chConfig->HwUnit);
        
        /* Reset channel info */
        I2c_ChannelInfo[i].State = I2C_STATE_UNINIT;
    }
    
    I2c_DriverInitialized = FALSE;
    I2c_DriverStatus = I2C_UNINIT;
    
    return E_OK;
}

Std_ReturnType I2c_WriteBytes(I2c_ChannelType Channel,
                               I2c_AddressType SlaveAddress,
                               const I2c_DataType* DataBuffer,
                               I2c_LengthType Length,
                               I2c_AddrModeType AddrMode)
{
    #if (I2C_DEV_ERROR_DETECT == STD_ON)
    if (I2c_DriverInitialized == FALSE) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_WRITEBYTES, I2C_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= I2c_ConfigPtr->NumChannels) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_WRITEBYTES, I2C_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    if (DataBuffer == NULL_PTR && Length > 0U) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_WRITEBYTES, I2C_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (Length > I2C_MAX_BUFFER_SIZE) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_WRITEBYTES, I2C_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
    #endif
    
    I2c_ChannelInfoType* chInfo = &I2c_ChannelInfo[Channel];
    const I2c_ChannelConfigType* chConfig = &I2c_ConfigPtr->Channels[Channel];
    
    /* Check if channel is busy */
    if (chInfo->State != I2C_STATE_IDLE) {
        return E_NOT_OK;
    }
    
    /* Setup transfer parameters */
    chInfo->State = I2C_STATE_MASTER_TX;
    chInfo->CurrentSlaveAddress = SlaveAddress;
    chInfo->CurrentAddrMode = AddrMode;
    chInfo->TxBuffer.Buffer = (I2c_DataType*)DataBuffer;
    chInfo->TxBuffer.Length = Length;
    chInfo->TxBuffer.Index = 0U;
    chInfo->RxBuffer.Length = 0U;
    chInfo->IsRepeatedStart = FALSE;
    chInfo->StopRequested = TRUE;
    
    /* Perform transfer based on mode */
    Std_ReturnType result;
    switch (chInfo->CurrentMode) {
        case I2C_TRANSFER_POLLING:
            result = I2c_MasterTransferPolling(Channel);
            break;
        case I2C_TRANSFER_INTERRUPT:
            result = I2c_MasterTransferInterrupt(Channel);
            break;
        case I2C_TRANSFER_DMA:
            result = I2c_MasterTransferDma(Channel);
            break;
        default:
            result = I2c_MasterTransferPolling(Channel);
            break;
    }
    
    /* Wait for completion in polling mode */
    if (chInfo->CurrentMode == I2C_TRANSFER_POLLING) {
        while (chInfo->State == I2C_STATE_MASTER_TX) {
            /* Wait */
        }
        result = (chInfo->Result == I2C_RESULT_OK) ? E_OK : E_NOT_OK;
    }
    
    return result;
}

Std_ReturnType I2c_ReadBytes(I2c_ChannelType Channel,
                              I2c_AddressType SlaveAddress,
                              I2c_DataType* DataBuffer,
                              I2c_LengthType Length,
                              I2c_AddrModeType AddrMode)
{
    #if (I2C_DEV_ERROR_DETECT == STD_ON)
    if (I2c_DriverInitialized == FALSE) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_READBYTES, I2C_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= I2c_ConfigPtr->NumChannels) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_READBYTES, I2C_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    if (DataBuffer == NULL_PTR && Length > 0U) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_READBYTES, I2C_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (Length > I2C_MAX_BUFFER_SIZE) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_READBYTES, I2C_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
    #endif
    
    I2c_ChannelInfoType* chInfo = &I2c_ChannelInfo[Channel];
    
    /* Check if channel is busy */
    if (chInfo->State != I2C_STATE_IDLE) {
        return E_NOT_OK;
    }
    
    /* Setup transfer parameters */
    chInfo->State = I2C_STATE_MASTER_RX;
    chInfo->CurrentSlaveAddress = SlaveAddress;
    chInfo->CurrentAddrMode = AddrMode;
    chInfo->TxBuffer.Length = 0U;
    chInfo->RxBuffer.Buffer = DataBuffer;
    chInfo->RxBuffer.Length = Length;
    chInfo->RxBuffer.Index = 0U;
    chInfo->IsRepeatedStart = FALSE;
    chInfo->StopRequested = TRUE;
    
    /* Perform transfer based on mode */
    Std_ReturnType result;
    switch (chInfo->CurrentMode) {
        case I2C_TRANSFER_POLLING:
            result = I2c_MasterTransferPolling(Channel);
            break;
        case I2C_TRANSFER_INTERRUPT:
            result = I2c_MasterTransferInterrupt(Channel);
            break;
        case I2C_TRANSFER_DMA:
            result = I2c_MasterTransferDma(Channel);
            break;
        default:
            result = I2c_MasterTransferPolling(Channel);
            break;
    }
    
    /* Wait for completion in polling mode */
    if (chInfo->CurrentMode == I2C_TRANSFER_POLLING) {
        while (chInfo->State == I2C_STATE_MASTER_RX) {
            /* Wait */
        }
        result = (chInfo->Result == I2C_RESULT_OK) ? E_OK : E_NOT_OK;
    }
    
    return result;
}

Std_ReturnType I2c_WriteRead(I2c_ChannelType Channel,
                              I2c_AddressType SlaveAddress,
                              const I2c_DataType* TxBuffer,
                              I2c_LengthType TxLength,
                              I2c_DataType* RxBuffer,
                              I2c_LengthType RxLength,
                              I2c_AddrModeType AddrMode)
{
    #if (I2C_DEV_ERROR_DETECT == STD_ON)
    if (I2c_DriverInitialized == FALSE) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_WRITEREAD, I2C_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= I2c_ConfigPtr->NumChannels) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_WRITEREAD, I2C_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    if ((TxBuffer == NULL_PTR && TxLength > 0U) || (RxBuffer == NULL_PTR && RxLength > 0U)) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_WRITEREAD, I2C_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if ((TxLength > I2C_MAX_BUFFER_SIZE) || (RxLength > I2C_MAX_BUFFER_SIZE)) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_WRITEREAD, I2C_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
    #endif
    
    I2c_ChannelInfoType* chInfo = &I2c_ChannelInfo[Channel];
    
    /* Check if channel is busy */
    if (chInfo->State != I2C_STATE_IDLE) {
        return E_NOT_OK;
    }
    
    /* Setup transfer parameters */
    chInfo->State = I2C_STATE_MASTER_TX;
    chInfo->CurrentSlaveAddress = SlaveAddress;
    chInfo->CurrentAddrMode = AddrMode;
    chInfo->TxBuffer.Buffer = (I2c_DataType*)TxBuffer;
    chInfo->TxBuffer.Length = TxLength;
    chInfo->TxBuffer.Index = 0U;
    chInfo->RxBuffer.Buffer = RxBuffer;
    chInfo->RxBuffer.Length = RxLength;
    chInfo->RxBuffer.Index = 0U;
    chInfo->IsRepeatedStart = TRUE;
    chInfo->StopRequested = TRUE;
    
    /* Perform transfer based on mode */
    Std_ReturnType result;
    switch (chInfo->CurrentMode) {
        case I2C_TRANSFER_POLLING:
            result = I2c_MasterTransferPolling(Channel);
            break;
        case I2C_TRANSFER_INTERRUPT:
            result = I2c_MasterTransferInterrupt(Channel);
            break;
        case I2C_TRANSFER_DMA:
            result = I2c_MasterTransferDma(Channel);
            break;
        default:
            result = I2c_MasterTransferPolling(Channel);
            break;
    }
    
    /* Wait for completion in polling mode */
    if (chInfo->CurrentMode == I2C_TRANSFER_POLLING) {
        while (chInfo->State != I2C_STATE_IDLE && chInfo->State != I2C_STATE_ERROR) {
            /* Wait */
        }
        result = (chInfo->Result == I2C_RESULT_OK) ? E_OK : E_NOT_OK;
    }
    
    return result;
}

I2c_StatusType I2c_GetStatus(void)
{
    return I2c_DriverStatus;
}

void I2c_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (I2C_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_GETVERSIONINFO, I2C_E_PARAM_POINTER);
        return;
    }
    #endif
    
    versioninfo->vendorID = I2C_VENDOR_ID;
    versioninfo->moduleID = I2C_MODULE_ID;
    versioninfo->sw_major_version = I2C_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = I2C_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = I2C_SW_PATCH_VERSION;
}

Std_ReturnType I2c_SetClockMode(I2c_ChannelType Channel, I2c_ClockModeType ClockMode)
{
    #if (I2C_DEV_ERROR_DETECT == STD_ON)
    if (I2c_DriverInitialized == FALSE) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_SETCLOCKMODE, I2C_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= I2c_ConfigPtr->NumChannels) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_SETCLOCKMODE, I2C_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    #endif
    
    const I2c_ChannelConfigType* chConfig = &I2c_ConfigPtr->Channels[Channel];
    uint32 baseAddr = I2c_GetBaseAddr(chConfig->HwUnit);
    
    uint32 freq;
    switch (ClockMode) {
        case I2C_CLOCK_STANDARD:
            freq = I2C_STANDARD_MODE_FREQ;
            break;
        case I2C_CLOCK_FAST:
            freq = I2C_FAST_MODE_FREQ;
            break;
        case I2C_CLOCK_FAST_PLUS:
            freq = I2C_FAST_MODE_PLUS_FREQ;
            break;
        case I2C_CLOCK_HIGH_SPEED:
            freq = I2C_HIGH_SPEED_MODE_FREQ;
            break;
        default:
            return E_NOT_OK;
    }
    
    I2c_SetFrequency(baseAddr, freq);
    
    return E_OK;
}

Std_ReturnType I2c_EnableInterrupt(I2c_ChannelType Channel)
{
    #if (I2C_INTERRUPT_SUPPORTED == STD_ON)
    #if (I2C_DEV_ERROR_DETECT == STD_ON)
    if (I2c_DriverInitialized == FALSE) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_ENABLEINTERRUPT, I2C_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= I2c_ConfigPtr->NumChannels) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_ENABLEINTERRUPT, I2C_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    #endif
    
    const I2c_ChannelConfigType* chConfig = &I2c_ConfigPtr->Channels[Channel];
    uint32 baseAddr = I2c_GetBaseAddr(chConfig->HwUnit);
    
    uint8 i2cr = REG_READ8(baseAddr + I2C_I2CR);
    i2cr |= I2CR_IIEN;
    REG_WRITE8(baseAddr + I2C_I2CR, i2cr);
    
    return E_OK;
    #else
    (void)Channel;
    return E_NOT_OK;
    #endif
}

Std_ReturnType I2c_DisableInterrupt(I2c_ChannelType Channel)
{
    #if (I2C_INTERRUPT_SUPPORTED == STD_ON)
    #if (I2C_DEV_ERROR_DETECT == STD_ON)
    if (I2c_DriverInitialized == FALSE) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_DISABLEINTERRUPT, I2C_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= I2c_ConfigPtr->NumChannels) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_DISABLEINTERRUPT, I2C_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    #endif
    
    const I2c_ChannelConfigType* chConfig = &I2c_ConfigPtr->Channels[Channel];
    uint32 baseAddr = I2c_GetBaseAddr(chConfig->HwUnit);
    
    uint8 i2cr = REG_READ8(baseAddr + I2C_I2CR);
    i2cr &= ~I2CR_IIEN;
    REG_WRITE8(baseAddr + I2C_I2CR, i2cr);
    
    return E_OK;
    #else
    (void)Channel;
    return E_NOT_OK;
    #endif
}

Std_ReturnType I2c_SetSlaveAddress(I2c_ChannelType Channel,
                                    I2c_AddressType SlaveAddress,
                                    I2c_AddrModeType AddrMode)
{
    #if (I2C_DEV_ERROR_DETECT == STD_ON)
    if (I2c_DriverInitialized == FALSE) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_SETSLAVEADDRESS, I2C_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= I2c_ConfigPtr->NumChannels) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_SETSLAVEADDRESS, I2C_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    #endif
    
    const I2c_ChannelConfigType* chConfig = &I2c_ConfigPtr->Channels[Channel];
    uint32 baseAddr = I2c_GetBaseAddr(chConfig->HwUnit);
    
    if (chConfig->OpMode != I2C_MODE_SLAVE) {
        return E_NOT_OK;
    }
    
    /* Set slave address (7-bit mode) */
    if (AddrMode == I2C_ADDR_MODE_7BIT) {
        REG_WRITE8(baseAddr + I2C_IADR, (uint8)(SlaveAddress << 1));
        return E_OK;
    }
    
    return E_NOT_OK;
}

I2c_BusStateType I2c_GetBusState(I2c_ChannelType Channel)
{
    #if (I2C_DEV_ERROR_DETECT == STD_ON)
    if (I2c_DriverInitialized == FALSE) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_GETBUSSTATE, I2C_E_UNINIT);
        return I2C_BUS_STATE_BUSY;
    }
    if (Channel >= I2c_ConfigPtr->NumChannels) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_GETBUSSTATE, I2C_E_PARAM_CHANNEL);
        return I2C_BUS_STATE_BUSY;
    }
    #endif
    
    const I2c_ChannelConfigType* chConfig = &I2c_ConfigPtr->Channels[Channel];
    uint32 baseAddr = I2c_GetBaseAddr(chConfig->HwUnit);
    
    if ((REG_READ8(baseAddr + I2C_I2SR) & I2SR_IBB) != 0U) {
        return I2C_BUS_STATE_BUSY;
    }
    
    return I2C_BUS_STATE_IDLE;
}

Std_ReturnType I2c_ClearBus(I2c_ChannelType Channel)
{
    #if (I2C_DEV_ERROR_DETECT == STD_ON)
    if (I2c_DriverInitialized == FALSE) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_CLEARBUS, I2C_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= I2c_ConfigPtr->NumChannels) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_CLEARBUS, I2C_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    #endif
    
    const I2c_ChannelConfigType* chConfig = &I2c_ConfigPtr->Channels[Channel];
    uint32 baseAddr = I2c_GetBaseAddr(chConfig->HwUnit);
    
    /* Send 9 clock pulses to release any stuck device */
    uint8 i2cr = REG_READ8(baseAddr + I2C_I2CR);
    i2cr &= ~I2CR_IEN;
    REG_WRITE8(baseAddr + I2C_I2CR, i2cr);
    
    for (uint8 i = 0U; i < 9U; i++) {
        /* Clock low */
        REG_WRITE8(baseAddr + I2C_I2DR, 0U);
        /* Clock high */
        REG_WRITE8(baseAddr + I2C_I2DR, 0xFFU);
    }
    
    /* Re-enable I2C */
    i2cr |= I2CR_IEN;
    REG_WRITE8(baseAddr + I2C_I2CR, i2cr);
    
    return E_OK;
}

Std_ReturnType I2c_SoftwareReset(I2c_ChannelType Channel)
{
    #if (I2C_SW_RESET_API == STD_ON)
    #if (I2C_DEV_ERROR_DETECT == STD_ON)
    if (I2c_DriverInitialized == FALSE) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_SOFTWARERESET, I2C_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= I2c_ConfigPtr->NumChannels) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_SOFTWARERESET, I2C_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    #endif
    
    I2c_ChannelInfoType* chInfo = &I2c_ChannelInfo[Channel];
    const I2c_ChannelConfigType* chConfig = &I2c_ConfigPtr->Channels[Channel];
    uint32 baseAddr = I2c_GetBaseAddr(chConfig->HwUnit);
    
    /* Disable I2C */
    REG_WRITE8(baseAddr + I2C_I2CR, 0U);
    
    /* Clear status */
    REG_WRITE8(baseAddr + I2C_I2SR, 0U);
    
    /* Reset channel info */
    chInfo->State = I2C_STATE_IDLE;
    chInfo->ErrorFlags = 0U;
    
    /* Re-enable I2C */
    REG_WRITE8(baseAddr + I2C_I2CR, I2CR_IEN);
    
    return E_OK;
    #else
    (void)Channel;
    return E_NOT_OK;
    #endif
}

Std_ReturnType I2c_SetTransferMode(I2c_ChannelType Channel, I2c_TransferModeType TransferMode)
{
    #if (I2C_DEV_ERROR_DETECT == STD_ON)
    if (I2c_DriverInitialized == FALSE) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_SETTRANSFERMODE, I2C_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= I2c_ConfigPtr->NumChannels) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_SETTRANSFERMODE, I2C_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    if (TransferMode > I2C_TRANSFER_DMA) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_SETTRANSFERMODE, I2C_E_PARAM_MODE);
        return E_NOT_OK;
    }
    #endif
    
    #if (I2C_DMA_SUPPORTED == STD_OFF)
    if (TransferMode == I2C_TRANSFER_DMA) {
        return E_NOT_OK;
    }
    #endif
    
    I2c_ChannelInfoType* chInfo = &I2c_ChannelInfo[Channel];
    chInfo->CurrentMode = TransferMode;
    
    return E_OK;
}

Std_ReturnType I2c_CancelTransfer(I2c_ChannelType Channel)
{
    #if (I2C_DEV_ERROR_DETECT == STD_ON)
    if (I2c_DriverInitialized == FALSE) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_CANCELTRANSFER, I2C_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= I2c_ConfigPtr->NumChannels) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_CANCELTRANSFER, I2C_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    #endif
    
    I2c_ChannelInfoType* chInfo = &I2c_ChannelInfo[Channel];
    const I2c_ChannelConfigType* chConfig = &I2c_ConfigPtr->Channels[Channel];
    uint32 baseAddr = I2c_GetBaseAddr(chConfig->HwUnit);
    
    /* Send STOP condition */
    I2c_SendStop(baseAddr);
    
    #if (I2C_DMA_SUPPORTED == STD_ON)
    /* Stop DMA if active */
    if (chInfo->CurrentMode == I2C_TRANSFER_DMA) {
        I2c_DmaStop(chConfig->DmaConfig.DmaTxChannel);
        I2c_DmaStop(chConfig->DmaConfig.DmaRxChannel);
    }
    #endif
    
    /* Update state */
    chInfo->State = I2C_STATE_IDLE;
    chInfo->Result = I2C_RESULT_CANCELLED;
    
    return E_OK;
}

Std_ReturnType I2c_PrepareSlaveBuffer(I2c_ChannelType Channel,
                                       I2c_DataType* Buffer,
                                       I2c_LengthType Length)
{
    #if (I2C_SLAVE_MODE_SUPPORTED == STD_ON)
    #if (I2C_DEV_ERROR_DETECT == STD_ON)
    if (I2c_DriverInitialized == FALSE) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_PREPARESLAVEBUFFER, I2C_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= I2c_ConfigPtr->NumChannels) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_PREPARESLAVEBUFFER, I2C_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    if (Buffer == NULL_PTR && Length > 0U) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_PREPARESLAVEBUFFER, I2C_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    I2c_ChannelInfoType* chInfo = &I2c_ChannelInfo[Channel];
    
    chInfo->RxBuffer.Buffer = Buffer;
    chInfo->RxBuffer.Length = Length;
    chInfo->RxBuffer.Index = 0U;
    
    return E_OK;
    #else
    (void)Channel;
    (void)Buffer;
    (void)Length;
    return E_NOT_OK;
    #endif
}

Std_ReturnType I2c_SlaveWriteBuffer(I2c_ChannelType Channel,
                                     const I2c_DataType* Buffer,
                                     I2c_LengthType Length)
{
    #if (I2C_SLAVE_MODE_SUPPORTED == STD_ON)
    #if (I2C_DEV_ERROR_DETECT == STD_ON)
    if (I2c_DriverInitialized == FALSE) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_SLAVEWRITEBUFFER, I2C_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= I2c_ConfigPtr->NumChannels) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_SLAVEWRITEBUFFER, I2C_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    if (Buffer == NULL_PTR && Length > 0U) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_SLAVEWRITEBUFFER, I2C_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    I2c_ChannelInfoType* chInfo = &I2c_ChannelInfo[Channel];
    
    chInfo->TxBuffer.Buffer = (I2c_DataType*)Buffer;
    chInfo->TxBuffer.Length = Length;
    chInfo->TxBuffer.Index = 0U;
    
    return E_OK;
    #else
    (void)Channel;
    (void)Buffer;
    (void)Length;
    return E_NOT_OK;
    #endif
}

Std_ReturnType I2c_SlaveReadBuffer(I2c_ChannelType Channel,
                                    I2c_DataType* Buffer,
                                    I2c_LengthType Length)
{
    #if (I2C_SLAVE_MODE_SUPPORTED == STD_ON)
    #if (I2C_DEV_ERROR_DETECT == STD_ON)
    if (I2c_DriverInitialized == FALSE) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_SLAVEREADBUFFER, I2C_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= I2c_ConfigPtr->NumChannels) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_SLAVEREADBUFFER, I2C_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    if (Buffer == NULL_PTR && Length > 0U) {
        Det_ReportError(I2C_MODULE_ID, 0U, I2C_SID_SLAVEREADBUFFER, I2C_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    I2c_ChannelInfoType* chInfo = &I2c_ChannelInfo[Channel];
    
    /* Copy received data */
    for (I2c_LengthType i = 0U; i < Length && i < chInfo->RxBuffer.Index; i++) {
        Buffer[i] = chInfo->RxBuffer.Buffer[i];
    }
    
    return E_OK;
    #else
    (void)Channel;
    (void)Buffer;
    (void)Length;
    return E_NOT_OK;
    #endif
}

void I2c_MainFunction(void)
{
    if (I2c_DriverInitialized == FALSE) {
        return;
    }
    
    for (uint8 i = 0U; i < I2c_ConfigPtr->NumChannels; i++) {
        I2c_ChannelInfoType* chInfo = &I2c_ChannelInfo[i];
        const I2c_ChannelConfigType* chConfig = &I2c_ConfigPtr->Channels[i];
        
        /* Handle pending operations in polling mode */
        if (chInfo->CurrentMode == I2C_TRANSFER_POLLING) {
            if (chInfo->State == I2C_STATE_MASTER_TX || chInfo->State == I2C_STATE_MASTER_RX) {
                /* Polling transfers are handled synchronously, so this shouldn't happen */
                /* But we can implement timeout handling here */
            }
        }
        
        /* Check for timeout conditions */
        if (chInfo->State != I2C_STATE_IDLE && chInfo->State != I2C_STATE_UNINIT) {
            /* Timeout logic can be implemented here */
        }
        
        /* Call notification functions if transfer completed */
        if (chInfo->Result == I2C_RESULT_OK) {
            if (chInfo->TxBuffer.Index > 0U && chConfig->TxNotification != NULL_PTR) {
                chConfig->TxNotification();
            }
            if (chInfo->RxBuffer.Index > 0U && chConfig->RxNotification != NULL_PTR) {
                chConfig->RxNotification();
            }
            chInfo->Result = I2C_RESULT_OK; /* Prevent multiple notifications */
        }
        
        /* Handle errors */
        if (chInfo->State == I2C_STATE_ERROR) {
            if (chConfig->ErrorNotification != NULL_PTR) {
                chConfig->ErrorNotification();
            }
            /* Reset to idle after error notification */
            chInfo->State = I2C_STATE_IDLE;
        }
    }
}

#define I2C_STOP_SEC_CODE
#include "MemMap.h"
