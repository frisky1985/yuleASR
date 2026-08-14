#!/bin/bash
# focused_coverage.sh — Targeted coverage build for production code
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_DIR"

echo "=============================================="
echo "  Focused Coverage Build — Production Code"
echo "=============================================="

find . -name "*.gcda" -delete 2>/dev/null

BASE="-std=c99 --coverage -g -O0 -fprofile-arcs -ftest-coverage -DTEST_BUILD -DPLATFORM_UNIT_TEST"

INCLUDES="-I$PROJECT_DIR/include/autosar"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/general/inc"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/os/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/mcu/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/port/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/dio/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/gpt/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/pwm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/eep/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/ocu/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/fls/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/det/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/pdur/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/com/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/nvm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/dcm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/dem/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/can/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/cannm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/cansm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/cryif/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/dlt/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/fim/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/comm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/ecum/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/bswm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/soad/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/wdgm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/schm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/j1939nm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/lin/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/linm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/linsm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/lintp/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/stbm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/xcp/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/secoc/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/keym/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/doip/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/csm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/mem/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/nm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/e2e/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/ramtst/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/canif/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/j1939tp/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/cantp/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/ethif/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/frif/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/linif/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/iohwab/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/memif/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/fee/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/ea/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/middleware/rte/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/coverage_run"
INCLUDES="$INCLUDES -I$PROJECT_DIR/tests/unit/framework"
INCLUDES="$INCLUDES -I$PROJECT_DIR/tests/mocks"
INCLUDES="$INCLUDES -I$PROJECT_DIR/tests/stubs"
INCLUDES="$INCLUDES -I$PROJECT_DIR/third_party"

BUILD_DIR="build-focused-cov"
mkdir -p "$BUILD_DIR"

UNITY_C="$PROJECT_DIR/tests/unit/framework/unity.c"

# Helper function: compile production module with its test
build_and_measure() {
    local name="$1"
    local test_src="$2"
    local prod_srcs="$3"
    local extra_cflags="${4:-}"

    echo ""
    echo "=== $name ==="
    set +e
    gcc $BASE $extra_cflags $INCLUDES -o "$BUILD_DIR/$name" $test_src $prod_srcs "$UNITY_C" -lm 2>"$BUILD_DIR/${name}.err"
    local rc=$?
    if [ $rc -ne 0 ]; then
        echo "  BUILD FAILED (rc=$rc): $(head -5 $BUILD_DIR/${name}.err | head -1)"
    else
        echo "  BUILD OK"
        "$BUILD_DIR/$name" >"$BUILD_DIR/${name}.out" 2>&1
        local run_rc=$?
        if [ $run_rc -eq 0 ]; then
            echo "  RUN PASS"
        else
            echo "  RUN FAIL (rc=$run_rc)"
        fi
    fi
    set -e
}

# Phase 1: CRC
build_and_measure "test_crc_cov" \
    "coverage_run/Det.c coverage_run/test_crc_coverage.c" \
    "src/bsw/services/crc/src/Crc.c src/bsw/services/crc/src/Crc_Lcfg.c"

# Phase 2: Det
build_and_measure "test_det_cov" \
    "coverage_run/Det.c coverage_run/test_det_coverage.c" \
    "src/bsw/services/det/src/Det.c" \
    "" 2>/dev/null || echo "  SKIP (test_det_coverage.c missing)"

# Phase 3: MCAL — standalone (basic types only)
# These compile with test code + production code for coverage
for module in dio gpt pwm eep ocu fls; do
    PROD_SRC="src/bsw/mcal/${module}/src/$(echo $module | tr '[:lower:]' '[:upper:]' | cut -c1)$(echo $module | cut -c2-).c"
    TEST_SRC="tests/unit/autosar/mcal/test_${module}.c"
    if [ -f "$PROD_SRC" ] && [ -f "$TEST_SRC" ]; then
        build_and_measure "test_${module}_cov" "$TEST_SRC" "$PROD_SRC" \
            "-I$PROJECT_DIR/src/bsw/mcal/${module}/include"
    fi
done

# Phase 4: Coverage capture
echo ""
echo "=== Coverage capture ==="
which lcov >/dev/null 2>&1 && {
    lcov --capture --directory "$BUILD_DIR" --output-file coverage_focused.info --ignore-errors gcov,empty 2>&1 || true
    
    lcov --remove coverage_focused.info '/usr/*' '*/tests/*' '*/coverage_run/*' '*/third_party/*' '/Library/*' \
         --output-file coverage_focused_filtered.info --ignore-errors unused,empty 2>&1 || true
    
    if [ -f coverage_focused_filtered.info ] && [ -s coverage_focused_filtered.info ]; then
        echo ""
        echo "=== Production Coverage Summary ==="
        lcov --summary coverage_focused_filtered.info 2>&1 || true
        mkdir -p coverage_report_focused
        genhtml coverage_focused_filtered.info --output-directory coverage_report_focused 2>&1 | tail -5
        
        # Generate CI report
        echo ""
        echo "=== CI coverage report ==="
        mkdir -p .yuleosh/reports
        python3 tools/generate_c_coverage_json.py coverage_focused_filtered.info .yuleosh/reports/c-coverage.json 2>/dev/null || true
    else
        echo "  ⚠️  No coverage data generated"
    fi
}

echo ""
echo "=== Done ==="
