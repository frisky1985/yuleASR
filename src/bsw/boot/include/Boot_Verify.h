#ifndef BOOT_VERIFY_H
#define BOOT_VERIFY_H

#include "Boot_Types.h"

Boot_Result Boot_Verify_Signature(const uint8_t    *hash,
                                  const uint8_t    *signature,
                                  const Boot_PubKey *pub_key);
void Boot_Verify_Hash(const uint8_t *data,
                      uint32_t       len,
                      uint8_t       *digest);
int32_t Boot_Verify_ConstantCmp(const uint8_t *a,
                                const uint8_t *b,
                                uint32_t       len);

extern const Boot_PubKey g_boot_pubkey_sbl;
extern const Boot_PubKey g_boot_pubkey_app;

#endif /* BOOT_VERIFY_H */