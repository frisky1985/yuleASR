#!/bin/bash
#============================================================================
# run_all_qemu_tests.sh - Batch-run all QEMU full-stack tests
#
# Used by CI (ci.yml qemu-full-stack job).
#============================================================================
set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

declare -A TESTS=(
    ["p0_assert"]="../p0_assert_infra/qemu_assert_infra.elf|QEMU_FULL_STACK_PASS"
    ["p1a_os"]="../p1a_os_schedule/qemu_os_schedule.elf|QEMU_FULL_STACK_PASS"
    ["p1b_ecum"]="../p1b_ecum_startup/qemu_ecum_startup.elf|QEMU_FULL_STACK_PASS"
    ["p2a_can"]="../p2a_can_loopback/qemu_can_loopback.elf|QEMU_FULL_STACK_PASS"
    ["p2b_write"]="../p2b_nvm_persist/qemu_nvm_write.elf|QEMU_FULL_STACK_PASS"
    ["p2b_read"]="../p2b_nvm_persist/qemu_nvm_read.elf|QEMU_FULL_STACK_PASS"
    ["p2c_uds"]="../p2c_uds_inject/qemu_uds_inject.elf|QEMU_FULL_STACK_PASS"
    ["p3a_ram_ecc"]="../p3a_ram_ecc/qemu_ram_ecc.elf|QEMU_FULL_STACK_PASS"
    ["p3b_wdg"]="../p3b_wdg_timeout/qemu_wdg_timeout.elf|QEMU_FULL_STACK_PASS"
    ["p3c_secoc"]="../p3c_secoc_loopback/qemu_secoc_loopback.elf|QEMU_FULL_STACK_PASS"
    ["p3d_irq"]="../p3d_irq_driven/qemu_irq_driven.elf|QEMU_FULL_STACK_PASS"
)

PASS_COUNT=0
FAIL_COUNT=0

for name in "${!TESTS[@]}"; do
    IFS='|' read -r elf marker <<< "${TESTS[$name]}"
    log="${name}.log"

    if [ ! -f "$elf" ]; then
        echo "SKIP: ${name} (elf not found: ${elf})"
        continue
    fi

    echo "--- Running ${name} ---"
    if ./run_qemu_test.sh "$elf" "$marker" "$log"; then
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
done

echo ""
echo "==============================="
echo "QEMU Full-Stack: ${PASS_COUNT} PASS / ${FAIL_COUNT} FAIL"
echo "==============================="

exit $FAIL_COUNT
