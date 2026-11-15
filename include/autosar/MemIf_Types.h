/******************************************************************************
 * @file MemIf_Types.h
 * @brief AutoSAR SWS_MemIf_Types — canonical type definitions
 *
 * This header is the single source of truth for MemIf value types.
 * Both ecual-layer MemIf.h and services-layer MemIf.h include this file
 * rather than re-defining the enums themselves (prevents ODR violations
 * caused by differing include order across translation units).
 ******************************************************************************/
#ifndef MEMIF_TYPES_H
#define MEMIF_TYPES_H
#include "Std_Types.h"

/* Version info macros (used by Fls.h / Eep.h version checks) */
#define MEMIF_TYPES_AR_RELEASE_MAJOR_VERSION   (4u)
#define MEMIF_TYPES_AR_RELEASE_MINOR_VERSION   (7u)
#define MEMIF_TYPES_AR_RELEASE_REVISION_VERSION (0u)

/*===========================================================================
 *                           MemIf value types (AUTOSAR canonical)
 *===========================================================================*/

/** @brief Job result of a memory interface operation (AUTOSAR SWS_MemIf) */
typedef enum {
    MEMIF_JOB_OK = 0,           /**< Job completed successfully */
    MEMIF_JOB_FAILED,           /**< Job failed */
    MEMIF_JOB_PENDING,          /**< Job is still pending */
    MEMIF_JOB_CANCELED,         /**< Job was canceled */
    MEMIF_BLOCK_INCONSISTENT,   /**< Block data is inconsistent */
    MEMIF_BLOCK_INVALID         /**< Block does not exist or is invalid */
} MemIf_JobResultType;

/** @brief Memory interface status (AUTOSAR SWS_MemIf + services-layer extension) */
typedef enum {
    MEMIF_UNINIT = 0,           /**< Module not initialized (services extension) */
    MEMIF_IDLE,                 /**< Module initialized and idle */
    MEMIF_BUSY,                 /**< Module busy with operation */
    MEMIF_BUSY_INTERNAL         /**< Module busy internally (e.g. background GC) */
} MemIf_StatusType;

/** @brief Memory interface mode (AUTOSAR SWS_MemIf + services-layer extension) */
typedef enum {
    MEMIF_MODE_UNINIT = 0,      /**< Not initialized (services extension) */
    MEMIF_MODE_SLOW,            /**< Slow read/write mode */
    MEMIF_MODE_FAST             /**< Fast read/write mode */
} MemIf_ModeType;

#endif /* MEMIF_TYPES_H */
