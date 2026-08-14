#!/bin/bash
#============================================================================
# build_asw.sh - Build yuleASR Os-layer + ASW component verification image
#
# Verifies the FULL yuleASR stack on QEMU mps2-an521 (Cortex-M33):
#   Os layer (production Os.c + Os_Cfg.c + Os_TaskEntries.c)
#   + Rte (Rte.c, Rte_Scheduler.c, Rte_AswScheduler.c)
#   + 8 ASW components
#   + BSW MainFunction stubs (real BSW stack out of scope)
#
# Usage:
#   ./build_asw.sh          build only
#   ./build_asw.sh run      build + run under QEMU (expect QEMU_M33_ASW_PASS)
#============================================================================
set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# yuleASR repo root (tests/qemu_m33 -> ../../ = repo root)
REPO_ROOT="$( cd "$SCRIPT_DIR/../.." && pwd )"

CROSS=arm-none-eabi-gcc
# NOTE: QEMU mps2-an521's cortex-m33 model has NO FPU (CPACR is read-only 0),
# so any VFP instruction (vldr/vadd) faults with NOCP. Compile the QEMU image
# with -mfloat-abi=soft (software float) so the compiler emits __aeabi_* calls
# instead of VFP instructions. The production S32K312 target keeps its FPU
# (-mfpu=fpv5-sp-d16); only this QEMU verification image is built soft-float.
# libgcc soft-float multilib is pulled in explicitly (homebrew arm-none-eabi
# toolchain does not search it when -nostdlib is used).
LIBGCC_NOFP=$(dirname "$(arm-none-eabi-gcc -print-libgcc-file-name)")/thumb/v8-m.main/nofp
CFLAGS="-mcpu=cortex-m33 -mthumb -mfloat-abi=soft -ffreestanding -nostdlib -O2 -Wall \
  -Wno-unused-but-set-variable -Wno-missing-braces -Wno-unused-variable -Wno-unused-function"
INCLUDES="\
    -I. \
    -Iinclude \
    -Isrc \
    -Ithird_party/FreeRTOS-Kernel/include \
    -Ithird_party/FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure \
    -I$REPO_ROOT/src/bsw/os/include \
    -I$REPO_ROOT/include/autosar \
    -I$REPO_ROOT/src/middleware/rte/include \
    -I$REPO_ROOT/src/bsw/services/bswm/include \
    -I$REPO_ROOT/src/bsw/services/com/include \
    -I$REPO_ROOT/src/bsw/services/canif/include \
    -I$REPO_ROOT/src/bsw/ecual/canif/include \
    -I$REPO_ROOT/src/bsw/services/dcm/include \
    -I$REPO_ROOT/src/bsw/services/nvm/include \
    -I$REPO_ROOT/src/bsw/services/dem/include \
    -I$REPO_ROOT/src/application \
    -I$REPO_ROOT/src/application/engine_control/include \
    -I$REPO_ROOT/src/application/vehicle_dynamics/include \
    -I$REPO_ROOT/src/application/diagnostic_manager/include \
    -I$REPO_ROOT/src/application/communication_manager/include \
    -I$REPO_ROOT/src/application/storage_manager/include \
    -I$REPO_ROOT/src/application/io_control/include \
    -I$REPO_ROOT/src/application/mode_manager/include \
    -I$REPO_ROOT/src/application/watchdog_manager/include \
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
    os_asw_main.c \
    os_asw_bsw_stubs.c \
"

# yuleASR production Os layer + Rte + ASW (THE code under test)
YULE_SRCS="\
    $REPO_ROOT/src/bsw/os/src/Os.c \
    $REPO_ROOT/src/bsw/os/src/Os_Cfg.c \
    $REPO_ROOT/src/bsw/os/src/Os_TaskEntries.c \
    $REPO_ROOT/src/middleware/rte/src/Rte.c \
    $REPO_ROOT/src/middleware/rte/src/Rte_Scheduler.c \
    $REPO_ROOT/src/middleware/rte/src/Rte_AswScheduler.c \
    $REPO_ROOT/src/middleware/rte/src/Rte_SwcPortApi.c \
    $REPO_ROOT/src/application/engine_control/src/Swc_EngineControl.c \
    $REPO_ROOT/src/application/vehicle_dynamics/src/Swc_VehicleDynamics.c \
    $REPO_ROOT/src/application/diagnostic_manager/src/Swc_DiagnosticManager.c \
    $REPO_ROOT/src/application/communication_manager/src/Swc_CommunicationManager.c \
    $REPO_ROOT/src/application/storage_manager/src/Swc_StorageManager.c \
    $REPO_ROOT/src/application/io_control/src/Swc_IOControl.c \
    $REPO_ROOT/src/application/mode_manager/src/Swc_ModeManager.c \
    $REPO_ROOT/src/application/watchdog_manager/src/Swc_WatchdogManager.c \
"

echo "==> Building qemu_m33_asw.elf ..."
$CROSS $CFLAGS $INCLUDES -T qemu_m33.ld $KERNEL_SRCS $APP_SRCS $YULE_SRCS -L"$LIBGCC_NOFP" -lgcc -o qemu_m33_asw.elf

echo "==> Verify ELF ..."
arm-none-eabi-readelf -h qemu_m33_asw.elf | grep -E "Entry|Machine"
arm-none-eabi-size qemu_m33_asw.elf

if [ "${1:-}" = "run" ]; then
    echo "==> Running under QEMU (mps2-an521) ..."
    qemu-system-arm -machine mps2-an521 -cpu cortex-m33 \
        -kernel qemu_m33_asw.elf -nographic -serial stdio 2>&1 | head -60
fi

echo "==> Build OK"
