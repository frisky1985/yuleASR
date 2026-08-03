#!/bin/bash
#============================================================================
# build.sh - Build yuleASR Os-layer verification image for QEMU mps2-an521
#
# Verifies the yuleASR AUTOSAR Os wrapper (src/bsw/os/Os.c) on a real
# ARMv8-M (Cortex-M33) core model, booting through:
#   main() -> StartOS() -> Os_Internal_StartOS() -> Os_InitTasks()
#          -> Os_Internal_ActivateTask() -> vTaskStartScheduler()
#
# Usage:
#   ./build.sh          build only
#   ./build.sh run      build + run under QEMU (expect QEMU_M33_OS_PASS)
#============================================================================
set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# yuleASR repo root (tests/qemu_m33 -> ../../ = repo root)
REPO_ROOT="$( cd "$SCRIPT_DIR/../.." && pwd )"

CROSS=arm-none-eabi-gcc
CFLAGS="-mcpu=cortex-m33 -mthumb -mfloat-abi=softfp -mfpu=fpv5-sp-d16 -ffreestanding -nostdlib -O2 -Wall -Werror"
INCLUDES="\
    -I. \
    -Iinclude \
    -Isrc \
    -Ithird_party/FreeRTOS-Kernel/include \
    -Ithird_party/FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure \
    -I$REPO_ROOT/src/bsw/os/include \
    -I$REPO_ROOT/include/autosar \
"

KERNEL_SRCS="\
    third_party/FreeRTOS-Kernel/tasks.c \
    third_party/FreeRTOS-Kernel/queue.c \
    third_party/FreeRTOS-Kernel/list.c \
    third_party/FreeRTOS-Kernel/timers.c \
    third_party/FreeRTOS-Kernel/event_groups.c \
    third_party/FreeRTOS-Kernel/stream_buffer.c \
    third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure/port.c \
    third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c \
    third_party/FreeRTOS-Kernel/portable/MemMang/heap_4.c \
"

APP_SRCS="\
    src/startup_m33.s \
    src/Uart_Cfg.c \
    src/hooks.c \
    src/libc_stubs.c \
    os_test_main.c \
    os_test_cfg.c \
"

# yuleASR production Os wrapper - THIS is the code under test.
OS_SRCS="\
    $REPO_ROOT/src/bsw/os/src/Os.c \
"

echo "==> Building qemu_m33_os.elf ..."
$CROSS $CFLAGS $INCLUDES -T qemu_m33.ld $KERNEL_SRCS $APP_SRCS $OS_SRCS -o qemu_m33_os.elf

echo "==> Verify ELF ..."
arm-none-eabi-readelf -h qemu_m33_os.elf | grep -E "Entry|Machine"
arm-none-eabi-size qemu_m33_os.elf

if [ "${1:-}" = "run" ]; then
    echo "==> Running under QEMU (mps2-an521) ..."
    qemu-system-arm -machine mps2-an521 -cpu cortex-m33 \
        -kernel qemu_m33_os.elf -nographic -serial stdio 2>&1 | head -40
fi

echo "==> Build OK"
