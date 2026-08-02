/******************************************************************************
 * @file MemIf_Types.h
 * @brief AutoSAR MemIf Types (Native Stub)
 ******************************************************************************/
#ifndef MEMIF_TYPES_H
#define MEMIF_TYPES_H
#include "Std_Types.h"

/* Version info macros (used by Fls.h / Eep.h version checks) */
#define MEMIF_TYPES_AR_RELEASE_MAJOR_VERSION  (4u)
#define MEMIF_TYPES_AR_RELEASE_MINOR_VERSION  (7u)
#define MEMIF_TYPES_AR_RELEASE_REVISION_VERSION (0u)

/*
 * Type/macro definitions are provided by MemIf.h (ecual layer) when present.
 * Guard with #ifndef MEMIF_H so the two headers never conflict.
 */
#ifndef MEMIF_H
typedef uint8 MemIf_JobResultType;
typedef uint8 MemIf_StatusType;
#define MEMIF_JOB_OK        ((MemIf_JobResultType)0U)
#define MEMIF_JOB_FAILED    ((MemIf_JobResultType)1U)
#define MEMIF_JOB_PENDING   ((MemIf_JobResultType)2U)
#define MEMIF_JOB_CANCELED  ((MemIf_JobResultType)3U)
#define MEMIF_UNINIT        ((MemIf_StatusType)0U)
#define MEMIF_IDLE          ((MemIf_StatusType)1U)
#define MEMIF_BUSY          ((MemIf_StatusType)2U)
#define MEMIF_BUSY_INTERNAL ((MemIf_StatusType)3U)
#endif /* MEMIF_H */
#endif
