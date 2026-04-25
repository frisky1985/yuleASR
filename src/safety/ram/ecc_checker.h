/******************************************************************************
 * @file    ecc_checker.h
 * @brief   ECC Checker - SECDED Error Detection and Correction
 *
 * AUTOSAR R22-11 compliant
 * ISO 26262 ASIL-D Safety Level
 ******************************************************************************/

#ifndef ECC_CHECKER_H
#define ECC_CHECKER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../common/autosar_types.h"
#include "ecc_encoder.h"

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* ECC check result */
typedef enum {
    ECC_CHECK_OK = 0,           /* No error */
    ECC_CHECK_SINGLE_ERROR,     /* Single bit error corrected */
    ECC_CHECK_DOUBLE_ERROR,     /* Double bit error detected */
    ECC_CHECK_ERROR             /* General error */
} EccCheckResultType;

/* ECC checker result structure */
typedef struct {
    uint32_t data;              /* Corrected data (32-bit) */
    uint64_t data64;            /* Corrected data (64-bit) */
    uint8_t ecc_code;           /* Original ECC code */
    uint8_t syndrome;           /* Error syndrome */
    EccCheckResultType result;  /* Check result */
    uint8_t error_position;     /* Error bit position (if single error) */
} EccCheckedType;

/******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Check and correct 32-bit data with ECC7
 * @param data Data to check
 * @param ecc_code ECC code
 * @param result Pointer to store result
 * @return E_OK on success
 */
Std_ReturnType EccChecker_Check32(uint32_t data, uint8_t ecc_code, EccCheckedType* result);

/**
 * @brief Check and correct 64-bit data with ECC8
 * @param data Data to check
 * @param ecc_code ECC code
 * @param result Pointer to store result
 * @return E_OK on success
 */
Std_ReturnType EccChecker_Check64(uint64_t data, uint8_t ecc_code, EccCheckedType* result);

/**
 * @brief Calculate syndrome for 32-bit data
 * @param data Data
 * @param ecc_code ECC code
 * @return Syndrome value
 */
uint8_t EccChecker_CalculateSyndrome32(uint32_t data, uint8_t ecc_code);

/**
 * @brief Calculate syndrome for 64-bit data
 * @param data Data
 * @param ecc_code ECC code
 * @return Syndrome value
 */
uint8_t EccChecker_CalculateSyndrome64(uint64_t data, uint8_t ecc_code);

#ifdef __cplusplus
}
#endif

#endif /* ECC_CHECKER_H */
