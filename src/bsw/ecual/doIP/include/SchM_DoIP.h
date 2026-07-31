/*==================================================================================================
 * SchM_DoIP.h - scheduler header for DoIP module exclusive areas
 *================================================================================================*/
#include "Mcal.h"

#ifndef SCHM_DOIP_H
#define SCHM_DOIP_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SchM_Enter_DoIP
#define SchM_Enter_DoIP()   Mcal_DisableAllInterrupts()
#endif

#ifndef SchM_Exit_DoIP
#define SchM_Exit_DoIP()   Mcal_EnableAllInterrupts()
#endif

#ifdef __cplusplus
}
#endif

#endif /* SCHM_DOIP_H */
