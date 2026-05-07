/*==================================================================================================
 *                              SCHEDULE MANAGER FOR CRYIF
 *==================================================================================================
 * FILENAME: SchM_CryIf.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Schedule Manager header for CryIf module
 *==================================================================================================
 */

#ifndef SCHM_CRYIF_H
#define SCHM_CRYIF_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
 *                                    EXCLUSIVE AREA MACROS
 *==================================================================================================*/

/**
 * @brief Enter exclusive area for CryIf critical section
 */
#define SchM_Enter_CryIf_CRYIF_EXCLUSIVE_AREA_0()   \
    do {                                            \
        /* Disable interrupts or use mutex */       \
    } while(0)

/**
 * @brief Exit exclusive area for CryIf critical section
 */
#define SchM_Exit_CryIf_CRYIF_EXCLUSIVE_AREA_0()    \
    do {                                            \
        /* Enable interrupts or release mutex */    \
    } while(0)

/**
 * @brief Enter exclusive area for CryIf key operations
 */
#define SchM_Enter_CryIf_CRYIF_EXCLUSIVE_AREA_1()   \
    do {                                            \
        /* Disable interrupts or use mutex */       \
    } while(0)

/**
 * @brief Exit exclusive area for CryIf key operations
 */
#define SchM_Exit_CryIf_CRYIF_EXCLUSIVE_AREA_1()    \
    do {                                            \
        /* Enable interrupts or release mutex */    \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* SCHM_CRYIF_H */
