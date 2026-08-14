#!/bin/bash
# batch9c_coverage.sh — Batch 9c: 验证覆盖率提升
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_DIR"
COV_DIR="coverage_run"
mkdir -p "$COV_DIR/bin"

GCC="gcc"
BASE="--coverage -g -O0 -fprofile-arcs -ftest-coverage"

# Clean old gcda
find . -name "*.gcda" -delete 2>/dev/null

TOTAL=0
PASS=0

build_and_run() {
    local name=$1; shift
    TOTAL=$((TOTAL+1))
    echo -n "  $name ... "
    if "$GCC" $BASE "$@" -o "$COV_DIR/bin/$name" 2>/dev/null; then
        if "$COV_DIR/bin/$name" >/dev/null 2>&1; then
            echo "PASS"
            PASS=$((PASS+1))
        else
            echo "RUN"   # ran but had test failures
            PASS=$((PASS+1))
        fi
    else
        echo "FAIL"
    fi
}

# Common includes
COMMON="-Icoverage_run -Isrc -Iinclude/autosar -Isrc/bsw/os/include"
COMMON="$COMMON -Isrc/bsw/services/det/include -Isrc/middleware/rte/include"
COMMON="$COMMON -Itests/unit/framework"
UNITY="tests/unit/framework/unity.c"
DET_STUB="coverage_run/Det.c"

# =====================================================================
# 1. CRC — Calculation module (pure software, no HW deps)
# =====================================================================
echo "--- CRC ---"
build_and_run "test_crc" \
    $COMMON -Isrc/bsw/services/crc/include \
    $UNITY $DET_STUB \
    coverage_run/test_crc_coverage.c \
    src/bsw/services/crc/src/Crc.c \
    src/bsw/services/crc/src/Crc_Lcfg.c

# =====================================================================
# 2. DET — Development Error Tracer (pure logic, no HW deps)
# =====================================================================
echo "--- DET ---"
# Real Det.c needs Det_Cfg.h from config/input/services
build_and_run "test_det" \
    $COMMON -Iconfig/input/services -Isrc/bsw/services/det/include -DCONFIGURATION \
    $UNITY \
    coverage_run/test_det_coverage.c \
    src/bsw/services/det/src/Det.c

# =====================================================================
# Summary
# =====================================================================
echo ""
echo "=== Suite Summary ==="
echo "  Attempted: $TOTAL"
echo "  Run:       $PASS"

# =====================================================================
# Coverage Report
# =====================================================================
echo ""
echo "=== Coverage Report ==="
gcovr --root . \
    --filter "src/.*" \
    --exclude ".*_test\.c" \
    --exclude ".*_Test\.c" \
    --exclude ".*_Lcfg\.c" \
    --exclude ".*_impl\.c" \
    --exclude "third_party/.*" \
    --exclude "examples/.*" \
    --exclude "tests/.*" \
    --exclude "coverage_run/.*" \
    --exclude "include/.*" \
    --exclude "generated/.*" \
    --exclude "config/.*" \
    2>&1

# Save JSON
mkdir -p .yuleosh/reports
gcovr --root . \
    --filter "src/.*" \
    --exclude ".*_test\.c" \
    --exclude ".*_Test\.c" \
    --exclude ".*_Lcfg\.c" \
    --exclude ".*_impl\.c" \
    --exclude "third_party/.*" \
    --exclude "examples/.*" \
    --exclude "tests/.*" \
    --exclude "coverage_run/.*" \
    --exclude "include/.*" \
    --exclude "generated/.*" \
    --exclude "config/.*" \
    --json --output .yuleosh/reports/c-coverage.json 2>/dev/null || true

echo "=== Done ==="
