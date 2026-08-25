#!/bin/bash
#============================================================================
# run_qemu_test.sh - Run a single QEMU test image and assert PASS marker
#
# Usage: ./run_qemu_test.sh <elf> <pass_marker> <log_file>
#============================================================================

ELF="${1:?missing elf path}"
MARKER="${2:?missing pass marker}"
LOG="${3:?missing log file}"

QEMU_MACHINE="${QEMU_MACHINE:-mps2-an521}"
QEMU_CPU="${QEMU_CPU:-cortex-m33}"
QEMU_TIMEOUT="${QEMU_TIMEOUT:-30}"

: > "${LOG}"

qemu-system-arm \
    -machine "${QEMU_MACHINE}" \
    -cpu "${QEMU_CPU}" \
    -kernel "${ELF}" \
    -display none \
    -serial mon:stdio \
    --semihosting-config enable=on,target=native \
    >> "${LOG}" 2>&1 &
QEMU_PID=$!

sleep "${QEMU_TIMEOUT}" && kill "${QEMU_PID}" 2>/dev/null &
WATCHDOG_PID=$!

wait "${QEMU_PID}"
RC=$?

kill "${WATCHDOG_PID}" 2>/dev/null || true
wait "${WATCHDOG_PID}" 2>/dev/null || true

cat "${LOG}"

if grep -q "${MARKER}" "${LOG}"; then
    echo "PASS: ${ELF} (${MARKER})"
    exit 0
else
    echo "FAIL: marker '${MARKER}' not found in ${LOG}" >&2
    exit 1
fi
