#include "flash_persist.h"
#include <stdint.h>
#include <stdbool.h>

static int qemu_sh(int op, volatile void *params)
{
    register uint32_t r0 __asm("r0") = (uint32_t)op;
    register uint32_t r1 __asm("r1") = (uint32_t)params;
    __asm volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
    return (int)r0;
}

static uint32_t path_len(const char *s)
{
    uint32_t n = 0;
    while (s[n]) n++;
    return n;
}

void FlashPersist_Export(const char *path, const uint8_t *data, uint32_t len)
{
    volatile uint32_t open_params[3] = { (uint32_t)(uintptr_t)path, 2u, path_len(path) };
    int handle = qemu_sh(0x01, (volatile void *)open_params);
    if (handle < 0) return;

    volatile uint32_t write_params[3] = { (uint32_t)handle, (uint32_t)(uintptr_t)data, len };
    qemu_sh(0x05, write_params);

    volatile uint32_t close_params[1] = { (uint32_t)handle };
    qemu_sh(0x02, close_params);
}

bool FlashPersist_Import(const char *path, uint8_t *data, uint32_t max_len)
{
    volatile uint32_t open_params[3] = { (uint32_t)(uintptr_t)path, 0u, path_len(path) };
    int handle = qemu_sh(0x01, open_params);
    if (handle < 0) return false;

    volatile uint32_t read_params[3] = { (uint32_t)handle, (uint32_t)(uintptr_t)data, max_len };
    int ret = qemu_sh(0x06, read_params);

    volatile uint32_t close_params[1] = { (uint32_t)handle };
    qemu_sh(0x02, close_params);
    return (ret >= 0);
}
