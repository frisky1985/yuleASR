#ifndef FLASH_PERSIST_H
#define FLASH_PERSIST_H

#include <stdint.h>
#include <stdbool.h>

void FlashPersist_Export(const char *path, const uint8_t *data, uint32_t len);
bool FlashPersist_Import(const char *path, uint8_t *data, uint32_t max_len);

#endif
