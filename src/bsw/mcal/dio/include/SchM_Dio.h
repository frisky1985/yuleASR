/*==================================================================================================
 * SchM_Dio.h - scheduler header for Dio driver exclusive areas
 * @file SchM_Dio.h
 * @brief BswM scheduler stubs for Dio
 *================================================================================================*/
#include "Mcal.h"

#ifndef SCHM_DIO_H
#define SCHM_DIO_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SchM_Enter_Dio_WriteChannelGroup
#define SchM_Enter_Dio_WriteChannelGroup()   Mcal_DisableAllInterrupts()
#endif

#ifndef SchM_Exit_Dio_WriteChannelGroup
#define SchM_Exit_Dio_WriteChannelGroup()   Mcal_EnableAllInterrupts()
#endif

#ifndef SchM_Enter_Dio_MaskedWritePort
#define SchM_Enter_Dio_MaskedWritePort()   Mcal_DisableAllInterrupts()
#endif

#ifndef SchM_Exit_Dio_MaskedWritePort
#define SchM_Exit_Dio_MaskedWritePort()   Mcal_EnableAllInterrupts()
#endif

#ifdef __cplusplus
}
#endif

#endif /* SCHM_DIO_H */
