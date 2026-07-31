/*==================================================================================================
 * SchM_Uart.h - scheduler header for Uart driver exclusive areas
 *================================================================================================*/
#include "Mcal.h"

#ifndef SCHM_UART_H
#define SCHM_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SchM_Enter_Uart_UART_EXCLUSIVE_AREA_0
#define SchM_Enter_Uart_UART_EXCLUSIVE_AREA_0()   Mcal_DisableAllInterrupts()
#endif

#ifndef SchM_Exit_Uart_UART_EXCLUSIVE_AREA_0
#define SchM_Exit_Uart_UART_EXCLUSIVE_AREA_0()   Mcal_EnableAllInterrupts()
#endif

#ifdef __cplusplus
}
#endif

#endif /* SCHM_UART_H */
