#!/bin/bash
# batch_final.sh — Final run with test fixes
set -euo pipefail
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_DIR"
find . -name "*.gcda" -delete 2>/dev/null

BASE="-std=c99 --coverage -g -O0 -fprofile-arcs -ftest-coverage"
BASE="$BASE -Wno-int-conversion -Wno-implicit-int -Wno-implicit-function-declaration -Wno-excess-initializers -Wno-macro-redefined"

INCLUDES="-I. -Iinclude/autosar -Icoverage_run -Isrc/bsw/os/include -Isrc/bsw/general/inc"
INCLUDES="$INCLUDES -Itests/mocks -Itests/unit/framework -Itests/stubs"
INCLUDES="$INCLUDES -Isrc/bsw/ecual/include -Isrc/rte/include"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/dio/include -Isrc/bsw/mcal/pwm/include"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/wdg/include -Isrc/bsw/mcal/gpt/include"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/icu/include -Isrc/bsw/mcal/can/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/det/include -Isrc/bsw/services/crc/include"

MOCK='-include tests/mocks/mock_registers.h'
STUBS='-include coverage_run/mcal_stubs.h'
UNITY_C="$PROJECT_DIR/tests/unit/framework/unity.c"
BUILD_DIR="$PROJECT_DIR/build-coverage-final"
mkdir -p "$BUILD_DIR/bin"
TOTAL=0; PASSED=0

report_coverage() {
    echo ""
    echo "=== 覆盖率报告 ==="
    local RAW_INFO="$PROJECT_DIR/coverage_final_raw.info"
    
    lcov --rc branch_coverage=1 --ignore-errors inconsistent \
        --capture --directory "$BUILD_DIR/bin" --output-file "$RAW_INFO" 2>&1
    lcov --rc branch_coverage=1 --ignore-errors unused \
        --remove "$RAW_INFO" '*/tests/*' '*/coverage_run/*' '*/third_party/*' '/usr/*' \
        -o "$PROJECT_DIR/coverage_final_src.info" 2>&1
    
    echo ""
    lcov --rc branch_coverage=1 --list "$PROJECT_DIR/coverage_final_src.info" 2>&1
    echo ""
    lcov --rc branch_coverage=1 --summary "$PROJECT_DIR/coverage_final_src.info" 2>&1 | head -6
    
    genhtml --rc branch_coverage=1 "$PROJECT_DIR/coverage_final_src.info" \
        --output-directory "$PROJECT_DIR/coverage_report_final" 2>&1 | tail -3
    echo "HTML报告: coverage_report_final/index.html"
}

echo ""
echo "=== 运行测试 ==="

# Dio - fully working
echo "--- Dio ---"
gcc $BASE $INCLUDES $STUBS $MOCK -o "$BUILD_DIR/bin/mcal_dio" \
    "coverage_run/test_mcal_dio.c" "tests/mocks/mock_registers.c" \
    "coverage_run/Det.c" "src/bsw/mcal/dio/src/Dio.c" \
    "src/bsw/mcal/dio/src/Dio_Lcfg.c" "$UNITY_C" -lm 2>"$BUILD_DIR/mcal_dio.err" && \
    "$BUILD_DIR/bin/mcal_dio" >"$BUILD_DIR/mcal_dio.out" 2>&1 && \
    echo "   PASS" || echo "   FAIL"

# Wdg - fully working  
echo "--- Wdg ---"
gcc $BASE $INCLUDES $STUBS $MOCK -o "$BUILD_DIR/bin/mcal_wdg" \
    "coverage_run/test_mcal_wdg.c" "tests/mocks/mock_registers.c" \
    "coverage_run/Det.c" "src/bsw/mcal/wdg/src/Wdg.c" "$UNITY_C" -lm \
    2>"$BUILD_DIR/mcal_wdg.err" && \
    "$BUILD_DIR/bin/mcal_wdg" >"$BUILD_DIR/mcal_wdg.out" 2>&1 && \
    echo "   PASS" || echo "   FAIL"

# CRC
echo "--- CRC ---"
gcc $BASE $INCLUDES $STUBS -o "$BUILD_DIR/bin/srv_crc" \
    "coverage_run/Det.c" "coverage_run/test_crc_coverage.c" \
    "src/bsw/services/crc/src/Crc.c" "$UNITY_C" -lm 2>"$BUILD_DIR/srv_crc.err" && \
    "$BUILD_DIR/bin/srv_crc" >"$BUILD_DIR/srv_crc.out" 2>&1 && \
    echo "   PASS" || echo "   FAIL"

report_coverage
