/**
 * @file Rte_MemIf.h
 * @brief RTE Memory Interface Header - stub for compilation
 */
#ifndef RTE_MEMIF_H
#define RTE_MEMIF_H

#include "Std_Types.h"
#include "MemIf.h"

/* RTE MemIf job result */
typedef uint8 Rte_MemIfJobResultType;

/* RTE MemIf status */
typedef uint8 Rte_MemIfStatusType;

/* RTE MemIf functions */
extern Std_ReturnType Rte_MemIf_Write(uint16 BlockNumber, const uint8* DataPtr, uint16 Length);
extern Std_ReturnType Rte_MemIf_Read(uint16 BlockNumber, uint8* DataPtr, uint16 Length);
extern Std_ReturnType Rte_MemIf_Erase(uint16 BlockNumber);
extern Std_ReturnType Rte_MemIf_Invalidate(uint16 BlockNumber);
extern Rte_MemIfStatusType Rte_MemIf_GetStatus(void);
extern Rte_MemIfJobResultType Rte_MemIf_GetJobResult(void);

#endif /* RTE_MEMIF_H */
