#!/bin/bash
# coverage_main.sh — Build & measure coverage for key modules
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_DIR"

COV_DIR="coverage_run"
mkdir -p "$COV_DIR/bin"

GCC="gcc"
BASE="--coverage -g -O0 -fprofile-arcs -ftest-coverage"

echo "=== yuleASR Coverage Measurement ==="
echo ""

# Clean old gcda
find . -name "*.gcda" -delete 2>/dev/null

declare -A RESULTS
TOTAL=0
PASS=0

build_and_run() {
    local name=$1; shift
    TOTAL=$((TOTAL+1))
    
    echo -n "  [TEST] $name ... "
    if "$GCC" $BASE "$@" -o "$COV_DIR/bin/$name" 2>/dev/null; then
        if "$COV_DIR/bin/$name" >/dev/null 2>&1; then
            echo "PASS"
            PASS=$((PASS+1))
        else
            echo "RUN (exit != 0)"
            PASS=$((PASS+1))
        fi
    else
        echo "COMPILE FAILED"
    fi
}

# ================= CRC Test =================
echo "--- CRC ---"
build_and_run "test_crc" \
    -Icoverage_run \
    -Isrc/bsw/services/crc/include \
    -Itests/unit/framework \
    tests/unit/framework/unity.c \
    coverage_run/Det.c \
    coverage_run/test_crc_coverage.c \
    src/bsw/services/crc/src/Crc.c \
    src/bsw/services/crc/src/Crc_Lcfg.c

# ================= Summary =================
echo ""
echo "=== Summary ==="
echo "  Total: $TOTAL"
echo "  Pass:  $PASS"

# ================= Coverage Report =================
echo ""
echo "=== Coverage Report ==="
# First run gcovr with root at project dir
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

# Save JSON report
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

# Parse line coverage
LINE_PCT=$(gcovr --root . \
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
    2>&1 | tail -15 | head -1 | grep -oP '\d+\.?\d*%' | head -1 || echo "N/A")

echo "  Line coverage: $LINE_PCT"
echo "=== Done ==="
