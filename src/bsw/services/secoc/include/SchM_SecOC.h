/*==================================================================================================
 * SchM_SecOC.h - scheduler header for SecOC module exclusive areas
 *================================================================================================*/
#include "Mcal.h"

#ifndef SCHM_SECOC_H
#define SCHM_SECOC_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0
#define SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0()   Mcal_DisableAllInterrupts()
#endif

#ifndef SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0
#define SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0()   Mcal_EnableAllInterrupts()
#endif

#ifdef __cplusplus
}
#endif

#endif /* SCHM_SECOC_H */
