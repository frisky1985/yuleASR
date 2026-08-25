#!/bin/bash
#============================================================================
# build.sh - C5: NvM Persist Verification (write/read two phases)
# Usage: ./build.sh [write|read|run]
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

COMMON_SRCS="$QM33/src/startup_m33.s $QM33/src/Uart_Cfg.c $QM33/src/hooks.c $QM33/src/libc_stubs.c $COMMON/qemu_assert.c $COMMON/flash_persist.c"

build_one() {
    local name="$1"
    local src="$2"
    local out="$SCRIPT_DIR/qemu_nvm_${name}.elf"
    echo "==> Building $out ..."
    $CROSS $CFLAGS $INCLUDES -T "$QM33/qemu_m33.ld" $KERNEL_SRCS $COMMON_SRCS "$src" -L"$LIBGCC_NOFP" -lgcc -o "$out"
    arm-none-eabi-size "$out"
}

case "${1:-}" in
    write)
        build_one "write" "$SCRIPT_DIR/main_nvm_write.c"
        ;;
    read)
        build_one "read" "$SCRIPT_DIR/main_nvm_read.c"
        ;;
    run)
        build_one "write" "$SCRIPT_DIR/main_nvm_write.c"
        build_one "read" "$SCRIPT_DIR/main_nvm_read.c"
        echo "==> Running write phase ..."
        "$SCRIPT_DIR/../ci/run_qemu_test.sh" "$SCRIPT_DIR/qemu_nvm_write.elf" "QEMU_FULL_STACK_PASS" "/tmp/c5_nvm_write.log"
        echo "==> Running read phase ..."
        "$SCRIPT_DIR/../ci/run_qemu_test.sh" "$SCRIPT_DIR/qemu_nvm_read.elf" "QEMU_FULL_STACK_PASS" "/tmp/c5_nvm_read.log"
        ;;
    "")
        build_one "write" "$SCRIPT_DIR/main_nvm_write.c"
        build_one "read" "$SCRIPT_DIR/main_nvm_read.c"
        ;;
    *)
        echo "Usage: $0 [write|read|run]"
        exit 1
        ;;
esac
echo "==> Build OK"
