/******************************************************************************
 * @file    saferam.h
 * @brief   SafeRAM - Unified Safe RAM Protection System
 *
 * AUTOSAR R22-11 compliant
 * ISO 26262 ASIL-D Safety Level
 *
 * This is the main header file for the SafeRAM protection system.
 * Include this file to access all SafeRAM functionality.
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#ifndef SAFERAM_H
#define SAFERAM_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Module Inclusion
 ******************************************************************************/

#include "saferam_manager.h"
#include "ram_partition.h"
#include "stack_protection.h"
#include "heap_protection.h"
#include "safe_data.h"
#include "fault_injection.h"

/******************************************************************************
 * SafeRAM Overview
 ******************************************************************************/

/**
 * SafeRAM Protection System
 * =========================
 *
 * SafeRAM provides comprehensive RAM protection for automotive safety-critical
 * systems compliant with ISO 26262 ASIL-D requirements.
 *
 * Components:
 * 1. RAM Partition Protection (ram_partition.c/h)
 *    - Memory partition management (Stack/Heap/Static/BSS)
 *    - Inter-partition guard zones
 *    - Red/Yellow/Green safety level classification
 *    - MPU/MMU integration
 *
 * 2. Stack Protection (stack_protection.c/h)
 *    - Stack overflow detection (Stack Canary)
 *    - Stack depth monitoring
 *    - Task stack checking
 *    - Interrupt stack monitoring
 *
 * 3. Heap Protection (heap_protection.c/h)
 *    - Heap memory validation
 *    - Memory block overlap detection
 *    - Zero-on-free
 *    - Double-free detection
 *    - Buffer overflow/underflow detection
 *
 * 4. Safe Data (safe_data.c/h)
 *    - Consistency checksum validation
 *    - Inverted data storage (1-out-of-2)
 *    - Multiple copy redundancy (2-out-of-3)
 *    - Timestamp monitoring
 *    - Data freshness validation
 *
 * 5. Fault Injection (fault_injection.c/h)
 *    - ECC error injection
 *    - Memory bit flip injection
 *    - Out-of-bounds access testing
 *    - Drift fault detection
 *    - Safety mechanism testing
 *
 * 6. SafeRAM Manager (saferam_manager.c/h)
 *    - Centralized initialization
 *    - Periodic self-check scheduling
 *    - Error callback management
 *    - Safety counter management
 *    - Diagnostic information recording
 *
 * Usage Example:
 * --------------
 * // Initialize SafeRAM
 * SafeRamConfigType config;
 * memset(&config, 0, sizeof(config));
 * config.flags = SAFERAM_FLAG_ENABLE_PARTITIONS |
 *                SAFERAM_FLAG_ENABLE_STACK |
 *                SAFERAM_FLAG_ENABLE_HEAP;
 * config.asil_level = ASIL_D;
 * config.periodic_interval_ms = 1000;
 *
 * SafeRam_Init(&config);
 * SafeRam_Start();
 *
 * // Register error callback
 * SafeRam_RegisterErrorCallback(MyErrorHandler);
 *
 * // Main loop
 * while (1) {
 *     SafeRam_PeriodicTask();
 *     // Application code
 * }
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

/******************************************************************************
 * Version Information
 ******************************************************************************/

#define SAFERAM_SYSTEM_VERSION_MAJOR    1U
#define SAFERAM_SYSTEM_VERSION_MINOR    0U
#define SAFERAM_SYSTEM_VERSION_PATCH    0U

/******************************************************************************
 * Integration Macros
 ******************************************************************************/

/* Quick initialization macro */
#define SAFERAM_INIT_DEFAULT() \
    do { \
        SafeRamConfigType _cfg; \
        memset(&_cfg, 0, sizeof(_cfg)); \
        _cfg.flags = SAFERAM_FLAG_ENABLE_PARTITIONS | \
                     SAFERAM_FLAG_ENABLE_STACK | \
                     SAFERAM_FLAG_ENABLE_HEAP | \
                     SAFERAM_FLAG_ENABLE_PERIODIC | \
                     SAFERAM_FLAG_ENABLE_COUNTERS; \
        _cfg.asil_level = ASIL_D; \
        _cfg.periodic_interval_ms = 1000; \
        _cfg.response_warning = SAFERAM_RESPONSE_LOG; \
        _cfg.response_error = SAFERAM_RESPONSE_NOTIFY; \
        _cfg.response_critical = SAFERAM_RESPONSE_SAFE_STATE; \
        SafeRam_Init(&_cfg); \
    } while(0)

/* Quick check macro */
#define SAFERAM_CHECK() \
    SafeRam_PeriodicTask()

/* Health check macro */
#define SAFERAM_IS_HEALTHY() \
    SafeRam_IsHealthy()

/******************************************************************************
 * Error Codes
 ******************************************************************************/

/* SafeRAM system error codes */
#define SAFERAM_E_OK                    0x00000000U
#define SAFERAM_E_NOT_INITIALIZED       0x00000001U
#define SAFERAM_E_ALREADY_INITIALIZED   0x00000002U
#define SAFERAM_E_INVALID_CONFIG        0x00000003U
#define SAFERAM_E_OUT_OF_MEMORY         0x00000004U
#define SAFERAM_E_PARTITION_ERROR       0x00000005U
#define SAFERAM_E_STACK_ERROR           0x00000006U
#define SAFERAM_E_HEAP_ERROR            0x00000007U
#define SAFERAM_E_DATA_ERROR            0x00000008U
#define SAFERAM_E_FAULT_INJECTION       0x00000009U
#define SAFERAM_E_TIMEOUT               0x0000000AU
#define SAFERAM_E_INVALID_POINTER       0x0000000BU
#define SAFERAM_E_CORRUPTION            0x0000000CU
#define SAFERAM_E_SAFETY_VIOLATION      0x0000000DU

/******************************************************************************
 * Reset Reasons
 ******************************************************************************/

#define SAFERAM_RESET_REASON_ERRORS     0x00000001U
#define SAFERAM_RESET_REASON_CRITICAL   0x00000002U
#define SAFERAM_RESET_REASON_SAFE_STATE 0x00000003U
#define SAFERAM_RESET_REASON_WATCHDOG   0x00000004U
#define SAFERAM_RESET_REASON_REQUESTED  0x00000005U

/******************************************************************************
 * Feature Test Macros
 ******************************************************************************/

#define SAFERAM_HAS_PARTITION_PROTECTION    1
#define SAFERAM_HAS_STACK_PROTECTION        1
#define SAFERAM_HAS_HEAP_PROTECTION         1
#define SAFERAM_HAS_SAFE_DATA               1
#define SAFERAM_HAS_FAULT_INJECTION         1
#define SAFERAM_HAS_MANAGER                 1

#ifdef __cplusplus
}
#endif

#endif /* SAFERAM_H */
