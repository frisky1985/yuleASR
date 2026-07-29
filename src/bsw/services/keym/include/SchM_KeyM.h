/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/*==================================================================================================
 *                                KEY MANAGER (KeyM) - SCHEDULER
 *==================================================================================================
 * FILENAME: SchM_KeyM.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Scheduler header file for Key Manager module exclusive areas
 *==================================================================================================
 */

#ifndef SCHM_KEYM_H
#define SCHM_KEYM_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                    EXCLUSIVE AREA MACROS
 *==================================================================================================*/

/* Note: In a real implementation, these macros would use OS mechanisms
 * for critical section protection (e.g., spinlocks, mutexes) */

#ifndef SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0
#define SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0()
#endif

#ifndef SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0
#define SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0()
#endif

#ifdef __cplusplus
}
#endif

#endif /* SCHM_KEYM_H */
