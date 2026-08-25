#!/bin/bash
#============================================================================
# build.sh - C1: QEMU Assert Infrastructure Verification
# Usage: ./build.sh [run]
#============================================================================
set -euo pipefail
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_ROOT="$( cd "$SCRIPT_DIR/../../.." && pwd )"
QM33="$REPO_ROOT/tests/qemu_m33"
COMMON="$SCRIPT_DIR/../common"

CROSS=arm-none-eabi-gcc
CFLAGS="-mcpu=cortex-m33 -mthumb -mfloat-abi=soft -ffreestanding -nostdlib -O2 -Wall -Wno-unused-function"
LIBGCC_NOFP=$(dirname "$(arm-none-eabi-gcc -print-libgcc-file-name)")/thumb/v8-m.main/nofp
INCLUDES="-I$COMMON -I$QM33/src -I$QM33 -I$QM33/third_party/FreeRTOS-Kernel/include -I$QM33/third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure -I$REPO_ROOT/include/autosar"

KERNEL_SRCS="$QM33/third_party/FreeRTOS-Kernel/tasks.c $QM33/third_party/FreeRTOS-Kernel/queue.c $QM33/third_party/FreeRTOS-Kernel/list.c $QM33/third_party/FreeRTOS-Kernel/timers.c $QM33/third_party/FreeRTOS-Kernel/event_groups.c $QM33/third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure/port.c $QM33/third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c $QM33/third_party/FreeRTOS-Kernel/portable/MemMang/heap_4.c"

APP_SRCS="$QM33/src/startup_m33.s $QM33/src/Uart_Cfg.c $QM33/src/hooks.c $QM33/src/libc_stubs.c $COMMON/qemu_assert.c $COMMON/unity_uart_output.c $SCRIPT_DIR/main_assert_test.c"

OUT="$SCRIPT_DIR/qemu_assert_infra.elf"
echo "==> Building $OUT ..."
$CROSS $CFLAGS $INCLUDES -T "$QM33/qemu_m33.ld" $KERNEL_SRCS $APP_SRCS -L"$LIBGCC_NOFP" -lgcc -o "$OUT"
arm-none-eabi-size "$OUT"

if [ "${1:-}" = "run" ]; then
    echo "==> Running under QEMU ..."
    "$SCRIPT_DIR/../ci/run_qemu_test.sh" "$OUT" "QEMU_FULL_STACK_PASS" "/tmp/c1_assert.log"
fi
echo "==> Build OK"
