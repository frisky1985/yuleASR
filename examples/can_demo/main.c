/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Example              : CAN Communication Demo
* Platform             : Generic Cortex-M
*
* Description          : CAN communication demo using Can, CanIf, and Com
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-19
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "Mcu.h"
#include "Port.h"
#include "Can.h"
#include "CanIf.h"
#include "Com.h"
#include "PduR.h"

/* CAN message IDs */
#define CAN_TX_MSG_ID       0x100
#define CAN_RX_MSG_ID       0x200

/* Message data */
static uint8 g_txData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
static uint8 g_rxData[8] = {0};

/* Message counter */
static volatile uint32 g_txCounter = 0;
static volatile uint32 g_rxCounter = 0;

/*==================================================================================================
*                                     CALLBACK FUNCTIONS
==================================================================================================*/
void CanIf_RxIndication(uint8 Hrh, Can_IdType CanId, uint8 CanDlc, const uint8* CanSduPtr)
{
    if (CanId == CAN_RX_MSG_ID)
    {
        g_rxCounter++;
        
        /* Copy received data */
        for (uint8 i = 0; i < CanDlc; i++)
        {
            g_rxData[i] = CanSduPtr[i];
        }
        
        /* Echo back with modified data */
        g_txData[0] = g_rxData[0] + 1;
        CanIf_Transmit(CanIfConf_CanIfTxPduCfg_TxPdu, g_txData, 8);
    }
}

void CanIf_TxConfirmation(PduIdType CanTxPduId)
{
    g_txCounter++;
}

/*==================================================================================================
*                                     MAIN FUNCTION
==================================================================================================*/
int main(void)
{
    Mcu_ConfigType mcuConfig;
    Port_ConfigType portConfig;
    Can_ConfigType canConfig;
    
    /* Initialize Mcu */
    mcuConfig.ClockSettings = NULL;
    mcuConfig.RamSectorSettings = NULL;
    mcuConfig.NumClockSettings = 0;
    mcuConfig.NumRamSectors = 0;
    Mcu_Init(&mcuConfig);
    
    /* Initialize Port */
    portConfig.Pins = NULL;
    portConfig.NumPins = 0;
    Port_Init(&portConfig);
    
    /* Initialize CAN */
    canConfig.Controllers = NULL;
    canConfig.NumControllers = 0;
    Can_Init(&canConfig);
    
    /* Initialize CanIf */
    CanIf_Init(NULL);
    
    /* Set CAN controller mode */
    CanIf_SetControllerMode(CanIfConf_CanIfCtrlCfg_CanIfCtrl, CAN_CS_STARTED);
    
    /* Main loop */
    while (1)
    {
        /* Periodic transmission */
        static uint32 counter = 0;
        counter++;
        
        if (counter >= 100000)
        {
            counter = 0;
            
            /* Update TX data */
            g_txData[0] = (uint8)(g_txCounter & 0xFF);
            g_txData[1] = (uint8)((g_txCounter >> 8) & 0xFF);
            
            /* Transmit message */
            CanIf_Transmit(CanIfConf_CanIfTxPduCfg_TxPdu, g_txData, 8);
        }
        
        /* Background processing */
        Can_MainFunction_Write();
        Can_MainFunction_Read();
        CanIf_MainFunction();
    }
    
    return 0;
}
