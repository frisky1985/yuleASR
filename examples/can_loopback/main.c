/**
 * @file main.c
 * @brief CAN Loopback Example
 * @version 1.0.0
 * 
 * This example demonstrates CAN communication in loopback mode.
 */

#include "Can.h"
#include "CanIf.h"
#include "Mcu.h"

#define CAN_TX_ID   0x123
#define CAN_RX_ID   0x123

static uint8_t txData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
static uint8_t rxData[8];
static boolean rxReceived = FALSE;

void CanIf_RxIndication(const Can_HwHandleType Hth, const Can_IdType CanId,
                        const uint8_t CanDlc, const uint8_t* CanSduPtr)
{
    if (CanId == CAN_RX_ID)
    {
        for (uint8 i = 0; i < CanDlc; i++)
        {
            rxData[i] = CanSduPtr[i];
        }
        rxReceived = TRUE;
    }
}

int main(void)
{
    /* Initialize MCU */
    Mcu_Init(&Mcu_Config);
    
    /* Initialize CAN */
    Can_Init(&Can_Config);
    
    /* Set CAN to loopback mode */
    Can_SetControllerMode(0, CAN_CS_STARTED);
    
    /* Main loop */
    while (1)
    {
        /* Send message */
        Can_Write(0, CAN_TX_ID, txData, 8);
        
        /* Wait for reception */
        while (!rxReceived);
        rxReceived = FALSE;
        
        /* Verify data */
        if (rxData[0] == txData[0])
        {
            /* Success - toggle LED or continue */
        }
    }
    
    return 0;
}
