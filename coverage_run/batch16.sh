#!/bin/bash
# batch16.sh — All compilable modules coverage
set -euo pipefail
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_DIR"
find . -name "*.gcda" -delete 2>/dev/null

BASE="-std=c99 --coverage -g -O0 -fprofile-arcs -ftest-coverage"
BASE="$BASE -Wno-int-conversion -Wno-implicit-int -Wno-implicit-function-declaration -Wno-excess-initializers"

INCLUDES="-I. -Iinclude/autosar -Icoverage_run -Isrc/bsw/os/include -Isrc/bsw/general/inc"
INCLUDES="$INCLUDES -Itests/mocks -Itests/unit/framework -Itests/stubs"
INCLUDES="$INCLUDES -Isrc/bsw/ecual/include -Isrc/rte/include"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/dio/include -Isrc/bsw/mcal/pwm/include"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/wdg/include -Isrc/bsw/mcal/gpt/include"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/icu/include -Isrc/bsw/mcal/can/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/det/include -Isrc/bsw/services/crc/include"

UNITY_C="$PROJECT_DIR/tests/unit/framework/unity.c"
BUILD_DIR="$PROJECT_DIR/build-coverage-b16"
mkdir -p "$BUILD_DIR/bin"
TOTAL=0; PASSED=0

build() {
    local name="$1"; shift
    TOTAL=$((TOTAL + 1))
    echo -n "  [TEST] $name ... "
    set +e
    gcc $BASE $INCLUDES -include tests/mocks/mock_registers.h \
        -o "$BUILD_DIR/bin/$name" $@ -lm 2>"$BUILD_DIR/$name.err"
    local rc=$?
    set -e
    if [ $rc -ne 0 ]; then
        echo "COMPILE FAILED"
        head -3 "$BUILD_DIR/$name.err"
    else
        set +e
        "$BUILD_DIR/bin/$name" >"$BUILD_DIR/$name.out" 2>&1
        set -e
        echo "PASS"; PASSED=$((PASSED + 1))
    fi
}

echo ""
echo "=== All Modules ===
"
echo "--- Dio ---"
build "mcal_dio" \
    "coverage_run/test_mcal_dio.c" "tests/mocks/mock_registers.c" \
    "coverage_run/Det.c" "src/bsw/mcal/dio/src/Dio.c" \
    "src/bsw/mcal/dio/src/Dio_Lcfg.c" "$UNITY_C"

echo "--- Pwm ---"
build "mcal_pwm" \
    "coverage_run/test_mcal_pwm.c" "tests/mocks/mock_registers.c" \
    "coverage_run/Det.c" "src/bsw/mcal/pwm/src/Pwm.c" \
    "src/bsw/mcal/pwm/src/Pwm_Lcfg.c" "$UNITY_C"

echo "--- Wdg ---"
build "mcal_wdg" \
    "coverage_run/test_mcal_wdg.c" "tests/mocks/mock_registers.c" \
    "coverage_run/Det.c" "src/bsw/mcal/wdg/src/Wdg.c" "$UNITY_C"

echo "--- Gpt ---"
build "mcal_gpt" \
    "coverage_run/test_mcal_gpt.c" "tests/mocks/mock_registers.c" \
    "coverage_run/Det.c" "src/bsw/mcal/gpt/src/Gpt.c" \
    "src/bsw/mcal/gpt/src/Gpt_Lcfg.c" "$UNITY_C"

echo "--- Icu ---"
build "mcal_icu" \
    "coverage_run/test_mcal_icu.c" "tests/mocks/mock_registers.c" \
    "coverage_run/Det.c" "src/bsw/mcal/icu/src/Icu.c" \
    "src/bsw/mcal/icu/src/Icu_Lcfg.c" "$UNITY_C"

echo "--- Can ---"
build "mcal_can" \
    "coverage_run/test_mcal_can.c" "tests/mocks/mock_registers.c" \
    "coverage_run/Det.c" "src/bsw/mcal/can/src/Can.c" \
    "src/bsw/mcal/can/src/Can_Lcfg.c" "$UNITY_C"

echo "--- CRC ---"
build "srv_crc" \
    "coverage_run/Det.c" "coverage_run/test_crc_coverage.c" \
    "src/bsw/services/crc/src/Crc.c" "$UNITY_C"

echo ""
echo "=== lcov ==="
lcov --rc branch_coverage=1 --capture --directory "$BUILD_DIR/bin" \
    --output-file "$PROJECT_DIR/coverage_b16_raw.info" 2>&1 || true

if [ -f "$PROJECT_DIR/coverage_b16_raw.info" ]; then
    lcov --rc branch_coverage=1 --extract "$PROJECT_DIR/coverage_b16_raw.info" \
        "$PROJECT_DIR/src/*" \
        --output-file "$PROJECT_DIR/coverage_b16_src.info" 2>&1 || true
    
    echo ""
    echo "=== 覆盖率摘要 ==="
    lcov --rc branch_coverage=1 --summary "$PROJECT_DIR/coverage_b16_src.info" 2>&1 | head -8
    
    echo ""
    echo "=== 各文件行覆盖率 ==="
    lcov --rc branch_coverage=1 --list "$PROJECT_DIR/coverage_b16_src.info" 2>&1
fi

echo ""
echo "=== 测试: $PASSED/$TOTAL 通过 ==="
find . -name "*.gcda" -delete 2>/dev/null
