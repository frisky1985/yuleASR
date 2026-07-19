/**
 * @file MemMap.h
 * @brief AUTOSAR Memory Mapping (Native Stub)
 *
 * Minimal stub for native (x86_64/Darwin) compilation.
 * All sections map to the default section.
 */
#ifndef MEMMAP_H
#define MEMMAP_H

/* No-op memory mapping macros */
#define MEMMAP_APPL_CODE
#define MEMMAP_APPL_DATA
#define MEMMAP_APPL_CONST
#define MEMMAP_APPL_BSS
#define MEMMAP_ERROR_CODE
#define MEMMAP_ERROR_DATA

/* Module-specific memory mapping macros (no-ops) */
#define DET_APPL_CODE
#define DET_APPL_DATA
#define DET_APPL_CONST
#define DET_APPL_BSS

#define BSWM_APPL_CODE
#define BSWM_APPL_DATA
#define BSWM_APPL_CONST
#define BSWM_APPL_BSS

#define COMM_APPL_CODE
#define COMM_APPL_DATA
#define COMM_APPL_CONST
#define COMM_APPL_BSS

#define COM_APPL_CODE
#define COM_APPL_DATA
#define COM_APPL_CONST
#define COM_APPL_BSS

#define DCM_APPL_CODE
#define DCM_APPL_DATA
#define DCM_APPL_CONST
#define DCM_APPL_BSS

#define ECUM_APPL_CODE
#define ECUM_APPL_DATA
#define ECUM_APPL_CONST
#define ECUM_APPL_BSS

#define NVM_APPL_CODE
#define NVM_APPL_DATA
#define NVM_APPL_CONST
#define NVM_APPL_BSS

#define CRC_APPL_CODE
#define CRC_APPL_DATA
#define CRC_APPL_CONST
#define CRC_APPL_BSS

#define WDGIF_APPL_CODE
#define WDGIF_APPL_DATA
#define WDGIF_APPL_CONST
#define WDGIF_APPL_BSS

#define ECUC_APPL_CODE

#define E2E_APPL_CODE
#define E2E_APPL_DATA
#define E2E_APPL_CONST
#define E2E_APPL_BSS

#endif /* MEMMAP_H */
