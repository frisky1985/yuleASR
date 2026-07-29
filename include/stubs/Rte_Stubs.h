/**
 * @file Rte_Stubs.h
 * @brief RTE function stubs for compilation
 */
#ifndef RTE_STUBS_H
#define RTE_STUBS_H

#include "Std_Types.h"

/* RTE return codes */
#ifndef RTE_E_OK
#define RTE_E_OK        0u
#endif
#ifndef RTE_E_NOK
#define RTE_E_NOK       1u
#endif
#ifndef RTE_E_NOT_OK
#define RTE_E_NOT_OK    1u
#endif
#ifndef RTE_E_MAX_AGE_EXCEEDED
#define RTE_E_MAX_AGE_EXCEEDED 2u
#endif

/* RTE read/write functions */
static inline Std_ReturnType Rte_Read(uint32 port, void* data) {
    (void)port; (void)data; return RTE_E_OK;
}

static inline Std_ReturnType Rte_Write(uint32 port, const void* data) {
    (void)port; (void)data; return RTE_E_OK;
}

static inline uint32 Rte_GetTime(void) {
    return 0u;
}

/* RTE port limits */
#ifndef RTE_MAX_PORTS_PER_COMPONENT
#define RTE_MAX_PORTS_PER_COMPONENT 32u
#endif

#endif /* RTE_STUBS_H */
