/**
 * @file i2c_reg_mock.h
 * @brief Host-side behavioural model of the Kinetis I2C register block.
 *
 * Force-included into the unit-under-test translation unit so that the
 * REG_READ8 / REG_WRITE8 macros resolve to this model instead of raw memory.
 */
#ifndef I2C_REG_MOCK_H
#define I2C_REG_MOCK_H

#include <stdint.h>

void    I2cMock_Reset(void);
uint8_t I2cMock_Read8(uint32_t Address);
void    I2cMock_Write8(uint32_t Address, uint8_t Value);

void    I2cMock_SetBusyStuck(uint8_t Enable);
void    I2cMock_SetNoInterruptFlag(uint8_t Enable);
void    I2cMock_SetNack(uint8_t Enable);
void    I2cMock_SetArbitrationLost(uint8_t Enable);
void    I2cMock_SetRxByte(uint8_t Value);

uint8_t I2cMock_GetReg(uint32_t Address);

#define REG_READ8(Address)              I2cMock_Read8((uint32_t)(Address))
#define REG_WRITE8(Address, Value)      I2cMock_Write8((uint32_t)(Address), (uint8_t)(Value))

#endif /* I2C_REG_MOCK_H */
