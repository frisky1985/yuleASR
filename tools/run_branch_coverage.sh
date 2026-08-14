#!/bin/bash
# run_branch_coverage.sh — yuleASR ASIL branch-coverage rebuild (P0-3)
#
# Builds native host test binaries for the ASIL-relevant BSW modules
# (E2E, NvM, WdgM, Com, Crc, Os timing protection, Det), runs them, and
# produces an lcov report with BRANCH coverage enabled.
#
# All binaries link the REAL production sources from src/ (unchanged);
# only BSW layer dependencies (MemIf device layer, FreeRTOS tick source)
# are stubbed for the host (coverage_run/asil/asil_stubs.c).
#
# Outputs:
#   build-coverage-asil/bin/            test binaries + .gcda/.gcno
#   coverage_asil_raw.info              raw lcov capture (branch data)
#   coverage_asil_src.info              filtered to src/ only
#   coverage_report_asil/               genhtml HTML report
#   .yuleosh/reports/c-coverage.json    CI-readable coverage report
#   .yuleosh/reports/branch-coverage-report.md   reproducibility notes
#
# Usage:
#   bash tools/run_branch_coverage.sh
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_DIR"

BUILD_DIR="build-coverage-asil"
mkdir -p "$BUILD_DIR/bin"

echo "=============================================="
echo "  yuleASR ASIL Branch Coverage Rebuild"
echo "  $(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo "=============================================="

# Clean stale coverage artifacts
find . -name "*.gcda" -delete 2>/dev/null || true

BASE="-std=c99 --coverage -g -O0 -fprofile-arcs -ftest-coverage"
BASE="$BASE -Wno-int-conversion -Wno-implicit-int -Wno-implicit-function-declaration"
BASE="$BASE -Wno-excess-initializers -Wno-macro-redefined -Wno-unused-variable"
BASE="$BASE -Wno-unused-function -Wno-unused-parameter -Wno-nonportable-include-path"

# Module include dirs FIRST so real headers shadow include/autosar stubs.
INC="-Isrc/bsw/services/det/include -Isrc/bsw/services/crc/include"
INC="$INC -Isrc/bsw/services/e2e/include -Isrc/bsw/services/nvm/include"
INC="$INC -Isrc/bsw/services/wdgm/include -Isrc/bsw/services/com/include"
INC="$INC -Isrc/bsw/services/com/include/com -Isrc/bsw/ecual/include"
INC="$INC -Isrc/bsw/mcal/wdg/include -Isrc/bsw/ecual/wdgif/include"
INC="$INC -Isrc/bsw/services/schm/include -Isrc/bsw/services/memif/include"
INC="$INC -Isrc/bsw/services/fee/include -Isrc/bsw/services/ramsafety/include"
INC="$INC -Isrc/bsw/services/csm/include -Isrc/bsw/services/cryif/include"
INC="$INC -Isrc/bsw/services/secoc/include -Isrc/bsw/mcal/eep/include"
INC="$INC -Isrc/bsw/mcal/fls/include -Isrc/bsw/mcal/flash/include"
INC="$INC -Isrc/bsw/mcal/mcu/include -Isrc/bsw/mcal/port/include"
INC="$INC -Isrc/bsw/mcal/gpt/include -Isrc/bsw/mcal/adc/include"
INC="$INC -Isrc/bsw/mcal/can/include -Isrc/bsw/mcal/dio/include"
INC="$INC -Isrc/bsw/mcal/icu/include -Isrc/bsw/mcal/pwm/include"
INC="$INC -Isrc/bsw/mcal/spi/include -Isrc/bsw/mcal/uart/include"
INC="$INC -Isrc/bsw/mcal/lin/include -Isrc/bsw/mcal/eth/include"
INC="$INC -Isrc/bsw/mcal/crypto/include -Isrc/bsw/ecual/canif/include"
INC="$INC -Isrc/bsw/ecual/canNm/include -Isrc/bsw/ecual/cantp/include"
INC="$INC -Isrc/bsw/ecual/ethif/include -Isrc/bsw/ecual/linif/include"
INC="$INC -Isrc/bsw/ecual/iohwab/include -Isrc/bsw/ecual/frif/include"
INC="$INC -Isrc/bsw/ecual/memif/include -Isrc/bsw/ecual/fee/include"
INC="$INC -Isrc/bsw/ecual/ea/include -Isrc/bsw/services/bswm/include"
INC="$INC -Isrc/bsw/services/ecum/include -Isrc/bsw/services/dem/include"
INC="$INC -Isrc/bsw/services/dcm/include -Isrc/bsw/services/comM/include"
INC="$INC -Isrc/bsw/services/dlt/include -Isrc/bsw/ecual/dlt/include"
INC="$INC -Isrc/bsw/services/pdur/include"
INC="$INC -Isrc/bsw/os/include -Isrc/bsw/general/inc"
INC="$INC -Iinclude/autosar -Itests/stubs -Isrc/middleware/rte/include"
INC="$INC -Itests/unit/framework -Itests/mocks -Ithird_party/cmocka"
INC="$INC -Ithird_party/freertos/include -Ithird_party/freertos/portable/posix"
INC="$INC -I. -Icoverage_run -Icoverage_run/asil"

UNITY_C="tests/unit/framework/unity.c"
DET_C="src/bsw/services/det/src/Det.c src/bsw/services/det/src/Det_Lcfg.c"
CRC_C="src/bsw/services/crc/src/Crc.c src/bsw/services/crc/src/Crc_Lcfg.c"
STUBS="coverage_run/asil/asil_stubs.c"

TOTAL=0; PASSED=0; FAILED=0

build_run() {
    local name="$1"; shift
    TOTAL=$((TOTAL + 1))
    echo -n "  [TEST] $name ... "
    set +e
    gcc $BASE $INC -o "$BUILD_DIR/bin/$name" "$@" -lm 2>"$BUILD_DIR/$name.err"
    local rc=$?
    set -e
    if [ $rc -ne 0 ]; then
        echo "COMPILE FAILED"
        head -5 "$BUILD_DIR/bin/$name.err" 2>/dev/null || head -5 "$BUILD_DIR/$name.err"
        FAILED=$((FAILED + 1))
        return 1
    fi
    set +e
    "$BUILD_DIR/bin/$name" >"$BUILD_DIR/$name.out" 2>&1
    rc=$?
    set -e
    if [ $rc -eq 0 ]; then
        echo "PASS"
        PASSED=$((PASSED + 1))
    else
        echo "RUN FAILED (exit=$rc)"
        tail -5 "$BUILD_DIR/$name.out"
        FAILED=$((FAILED + 1))
    fi
}

echo ""
echo "=== ASIL module coverage tests ==="

echo "--- Crc ---"
build_run srv_crc \
    "coverage_run/test_crc_coverage.c" $DET_C $CRC_C "$UNITY_C"

echo "--- Det ---"
# White-box build: Det_Test.c observes Det.c internal state
gcc $BASE $INC -Dstatic= -c src/bsw/services/det/src/Det.c \
    -o "$BUILD_DIR/bin/det_whitebox.o" 2>"$BUILD_DIR/det_whitebox.err" || {
        echo "  [TEST] det_whitebox ... COMPILE FAILED"; head -5 "$BUILD_DIR/det_whitebox.err"; FAILED=$((FAILED+1));
    }
if [ -f "$BUILD_DIR/bin/det_whitebox.o" ]; then
    TOTAL=$((TOTAL + 1))
    echo -n "  [TEST] det ... "
    set +e
    gcc $BASE $INC -include coverage_run/det_test_shim.h \
        -o "$BUILD_DIR/bin/det" tests/unit/det/Det_Test.c \
        "$BUILD_DIR/bin/det_whitebox.o" \
        -lm 2>"$BUILD_DIR/det.err"
    rc=$?
    set -e
    if [ $rc -ne 0 ]; then
        echo "COMPILE FAILED"; head -5 "$BUILD_DIR/det.err"; FAILED=$((FAILED+1));
    else
        set +e; "$BUILD_DIR/bin/det" >"$BUILD_DIR/det.out" 2>&1; rc=$?; set -e
        if [ $rc -eq 0 ]; then echo "PASS"; PASSED=$((PASSED+1)); else
            echo "RUN FAILED (exit=$rc)"; tail -5 "$BUILD_DIR/det.out"; FAILED=$((FAILED+1)); fi
    fi
fi

echo "--- E2E ---"
build_run srv_e2e \
    "coverage_run/asil/test_e2e_coverage.c" $STUBS \
    src/bsw/services/e2e/src/E2E.c src/bsw/services/e2e/src/E2E_P01.c \
    src/bsw/services/e2e/src/E2E_P02.c src/bsw/services/e2e/src/E2E_P04.c \
    src/bsw/services/e2e/src/E2E_P05.c src/bsw/services/e2e/src/E2E_P06.c \
    src/bsw/services/e2e/src/E2E_P07.c src/bsw/services/e2e/src/E2E_Lcfg.c \
    $CRC_C $DET_C

echo "--- NvM ---"
build_run srv_nvm \
    "coverage_run/asil/test_nvm_coverage.c" $STUBS \
    src/bsw/services/nvm/src/NvM.c src/bsw/services/nvm/src/NvM_Redundant.c \
    src/bsw/services/nvm/src/NvM_EccHandler.c src/bsw/services/nvm/src/NvM_EccHandler_Cfg.c \
    $DET_C

echo "--- WdgM ---"
build_run srv_wdgm \
    "coverage_run/asil/test_wdgm_coverage.c" $STUBS \
    src/bsw/services/wdgm/src/Wdgm.c src/bsw/services/wdgm/src/WdgM_Cfg.c \
    $DET_C

echo "--- Com (host cfg override: COM_NUM_OF_SIGNALS=8) ---"
build_run srv_com \
    "coverage_run/asil/test_com_coverage.c" "coverage_run/stubs_com.c" \
    -include coverage_run/asil/com_cfg_override.h \
    src/bsw/services/com/src/Com.c src/bsw/services/com/src/Com_Lcfg.c \
    $DET_C

echo "--- OS timing protection ---"
build_run srv_os_timing \
    "coverage_run/asil/test_os_timing_coverage.c" $STUBS \
    -DOS_RESOURCE_COUNT=1 \
    src/bsw/os/src/Os_TimingProtection.c \
    $DET_C

echo ""
echo "=== Tests: $PASSED passed / $FAILED failed / $TOTAL total ==="
if [ "$FAILED" -gt 0 ]; then
    echo "ERROR: $FAILED coverage test(s) failed — refusing to publish coverage data."
    exit 1
fi

echo ""
echo "=== lcov capture (branch coverage enabled) ==="
lcov --rc branch_coverage=1 --ignore-errors inconsistent \
    --capture --directory "$BUILD_DIR/bin" \
    --output-file "coverage_asil_raw.info" 2>&1 | tail -2

echo ""
echo "=== Filter to production src/ ==="
lcov --rc branch_coverage=1 --ignore-errors inconsistent,unused \
    --remove "coverage_asil_raw.info" \
    '*/tests/*' '*/coverage_run/*' '*/third_party/*' '/usr/*' '*/include/*' \
    --output-file "coverage_asil_src.info" 2>&1 | tail -2

echo ""
echo "=== Summary (line + branch) ==="
lcov --rc branch_coverage=1 --ignore-errors inconsistent \
    --summary "coverage_asil_src.info" 2>&1 | grep -E "lines|functions|branches|hit|found" | head -8

echo ""
echo "=== Per-file detail ==="
lcov --rc branch_coverage=1 --ignore-errors inconsistent \
    --list "coverage_asil_src.info" 2>&1 | grep -E "src/bsw|Path|Rate" | head -30

echo ""
echo "=== Generate CI JSON ==="
python3 tools/generate_c_coverage_json.py "coverage_asil_src.info" \
    ".yuleosh/reports/c-coverage.json"

echo ""
echo "=== Generate HTML report ==="
genhtml --rc branch_coverage=1 --ignore-errors inconsistent \
    "coverage_asil_src.info" \
    --output-directory "coverage_report_asil" 2>&1 | tail -2

find . -name "*.gcda" -delete 2>/dev/null || true
echo ""
echo "DONE. HTML: coverage_report_asil/index.html"
