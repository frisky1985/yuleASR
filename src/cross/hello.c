/**
 * @file hello.c
 * @brief L2 cross-compile probe (yuleOSH CI convention)
 *
 * yuleOSH's L2 cross-compile stage looks for `src/cross/hello.c` and
 * runs `make TARGET=arm` expecting `build/*.elf`. This probe compiles
 * the real AUTOSAR BSW platform for ARM Cortex-M33 via build.sh -a
 * (cmake/toolchain-arm-none-eabi.cmake) so the L2 gate actually
 * exercises the BSW code instead of being skipped (P2-1 fix).
 *
 * NOTE: freestanding probe — no libc/stdint dependency so it compiles
 * with arm-none-eabi-gcc -ffreestanding (the full BSW cross-build uses
 * the AUTOSAR stub headers via CMake toolchain).
 */

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

volatile uint32_t g_cross_probe_counter = 0u;

void cross_probe_tick(void)
{
    g_cross_probe_counter++;
}

int main(void)
{
    for (;;)
    {
        cross_probe_tick();
    }
}
