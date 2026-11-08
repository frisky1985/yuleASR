/**
 * @file i2c_reg_mock.c
 * @brief Behavioural I2C register model used by the host unit tests.
 */
#include "i2c_reg_mock.h"

#define I2C_MOCK_ROWS       (5U)
#define I2C_MOCK_WINDOW     (0x20U)

#define I2C_MOCK_REG_I2CR   (0x08U)
#define I2C_MOCK_REG_I2SR   (0x0CU)
#define I2C_MOCK_REG_I2DR   (0x10U)

#define I2C_MOCK_I2CR_MSTA  (0x20U)
#define I2C_MOCK_I2CR_RSTA  (0x04U)

#define I2C_MOCK_I2SR_ICF   (0x80U)
#define I2C_MOCK_I2SR_IBB   (0x20U)
#define I2C_MOCK_I2SR_IAL   (0x10U)
#define I2C_MOCK_I2SR_RXAK  (0x01U)

static uint8_t I2cMock_Regs[I2C_MOCK_ROWS][I2C_MOCK_WINDOW];
static uint8_t I2cMock_Master;
static uint8_t I2cMock_RepeatedStart;
static uint8_t I2cMock_BusyStuck;
static uint8_t I2cMock_NoIcf;
static uint8_t I2cMock_Nack;
static uint8_t I2cMock_ArbLost;
static uint8_t I2cMock_RxByte;

static uint8_t I2cMock_Row(uint32_t Address, uint8_t* Offset)
{
    uint8_t row = 4U;

    if (((Address & 0xFFFF0000UL) == 0x30A20000UL)) { row = 0U; }
    else if (((Address & 0xFFFF0000UL) == 0x30A30000UL)) { row = 1U; }
    else if (((Address & 0xFFFF0000UL) == 0x30A40000UL)) { row = 2U; }
    else if (((Address & 0xFFFF0000UL) == 0x30A50000UL)) { row = 3U; }
    else { row = 4U; }

    *Offset = (uint8_t)(Address & (I2C_MOCK_WINDOW - 1U));
    return row;
}

static uint8_t I2cMock_BusBusy(void)
{
    if (I2cMock_BusyStuck != 0U) {
        return 1U;
    }
    if ((I2cMock_Master != 0U) && (I2cMock_RepeatedStart == 0U)) {
        return 1U;
    }
    return 0U;
}

void I2cMock_Reset(void)
{
    uint8_t i;
    uint8_t j;

    for (i = 0U; i < I2C_MOCK_ROWS; i++) {
        for (j = 0U; j < I2C_MOCK_WINDOW; j++) {
            I2cMock_Regs[i][j] = 0U;
        }
    }
    I2cMock_Master = 0U;
    I2cMock_RepeatedStart = 0U;
    I2cMock_BusyStuck = 0U;
    I2cMock_NoIcf = 0U;
    I2cMock_Nack = 0U;
    I2cMock_ArbLost = 0U;
    I2cMock_RxByte = 0xA5U;
}

void I2cMock_SetBusyStuck(uint8_t Enable)
{
    I2cMock_BusyStuck = (Enable != 0U) ? 1U : 0U;
}

void I2cMock_SetNoInterruptFlag(uint8_t Enable)
{
    I2cMock_NoIcf = (Enable != 0U) ? 1U : 0U;
}

void I2cMock_SetNack(uint8_t Enable)
{
    I2cMock_Nack = (Enable != 0U) ? 1U : 0U;
}

void I2cMock_SetArbitrationLost(uint8_t Enable)
{
    I2cMock_ArbLost = (Enable != 0U) ? 1U : 0U;
}

void I2cMock_SetRxByte(uint8_t Value)
{
    I2cMock_RxByte = Value;
}

uint8_t I2cMock_Read8(uint32_t Address)
{
    uint8_t offset;
    uint8_t row = I2cMock_Row(Address, &offset);

    if (offset == I2C_MOCK_REG_I2DR) {
        return I2cMock_RxByte;
    }

    if (offset == I2C_MOCK_REG_I2SR) {
        uint8_t status = 0U;

        if (I2cMock_NoIcf == 0U) {
            status |= I2C_MOCK_I2SR_ICF;
        }
        if (I2cMock_BusBusy() != 0U) {
            status |= I2C_MOCK_I2SR_IBB;
        }
        if (I2cMock_ArbLost != 0U) {
            status |= I2C_MOCK_I2SR_IAL;
        }
        if (I2cMock_Nack != 0U) {
            status |= I2C_MOCK_I2SR_RXAK;
        }
        return status;
    }

    return I2cMock_Regs[row][offset];
}

void I2cMock_Write8(uint32_t Address, uint8_t Value)
{
    uint8_t offset;
    uint8_t row = I2cMock_Row(Address, &offset);

    if (offset == I2C_MOCK_REG_I2CR) {
        if ((Value & I2C_MOCK_I2CR_RSTA) != 0U) {
            /* Repeated START requested: the bus becomes free for the
             * following START sequence. */
            I2cMock_Regs[row][offset] = (uint8_t)(Value & (uint8_t)~I2C_MOCK_I2CR_RSTA);
            I2cMock_Master = 1U;
            I2cMock_RepeatedStart = 1U;
        } else if ((Value & I2C_MOCK_I2CR_MSTA) != 0U) {
            I2cMock_Regs[row][offset] = Value;
            I2cMock_Master = 1U;
            I2cMock_RepeatedStart = 0U;
        } else {
            I2cMock_Regs[row][offset] = Value;
            I2cMock_Master = 0U;
            I2cMock_RepeatedStart = 0U;
        }
        return;
    }

    I2cMock_Regs[row][offset] = Value;
}

uint8_t I2cMock_GetReg(uint32_t Address)
{
    uint8_t offset;
    uint8_t row = I2cMock_Row(Address, &offset);

    return I2cMock_Regs[row][offset];
}
