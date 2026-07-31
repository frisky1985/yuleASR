/*==================================================================================================
 * SchM_Fee.h - scheduler header for Fee driver exclusive areas
 *================================================================================================*/
#include "Mcal.h"

#ifndef SCHM_FEE_H
#define SCHM_FEE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0
#define SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0()   Mcal_DisableAllInterrupts()
#endif

#ifndef SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0
#define SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0()   Mcal_EnableAllInterrupts()
#endif

#ifdef __cplusplus
}
#endif

#endif /* SCHM_FEE_H */
