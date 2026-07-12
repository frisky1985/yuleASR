#ifndef BOOT_LOADER_H
#define BOOT_LOADER_H

#include "Boot_Types.h"

void Boot_Loader_Main(void) __attribute__((noreturn));
void Boot_Loader_Jump(uint32_t target_addr) __attribute__((noreturn));
void Boot_Loader_EnterRecovery(void) __attribute__((noreturn));
Boot_Decision Boot_Loader_ResolveBootTarget(void);
Boot_Result Boot_Loader_ConfirmBoot(void);

#endif /* BOOT_LOADER_H */