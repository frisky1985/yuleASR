/**=================================================================================================
 * @file SchM_Crypto.h
 * @brief Schedule Manager header for Crypto Driver
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *==================================================================================================*/

#ifndef SCHM_CRYPTO_H
#define SCHM_CRYPTO_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                        INCLUDE FILES
 *==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
 *                                    EXCLUSIVE AREA FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Enter exclusive area 0 (Job queue protection)
 */
extern void SchM_Enter_Crypto_CRYPTO_EXCLUSIVE_AREA_0(void);

/**
 * @brief Exit exclusive area 0 (Job queue protection)
 */
extern void SchM_Exit_Crypto_CRYPTO_EXCLUSIVE_AREA_0(void);

/**
 * @brief Enter exclusive area 1 (Key storage protection)
 */
extern void SchM_Enter_Crypto_CRYPTO_EXCLUSIVE_AREA_1(void);

/**
 * @brief Exit exclusive area 1 (Key storage protection)
 */
extern void SchM_Exit_Crypto_CRYPTO_EXCLUSIVE_AREA_1(void);

/**
 * @brief Enter exclusive area 2 (Hardware register protection)
 */
extern void SchM_Enter_Crypto_CRYPTO_EXCLUSIVE_AREA_2(void);

/**
 * @brief Exit exclusive area 2 (Hardware register protection)
 */
extern void SchM_Exit_Crypto_CRYPTO_EXCLUSIVE_AREA_2(void);

#ifdef __cplusplus
}
#endif

#endif /* SCHM_CRYPTO_H */
