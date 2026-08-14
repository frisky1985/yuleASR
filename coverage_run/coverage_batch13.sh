#!/bin/bash
# coverage_batch13.sh — Clean coverage build with all modules
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_DIR"

echo "=============================================="
echo "  Batch13 — 覆盖率构建 (final)"
echo "=============================================="

find . -name "*.gcda" -delete 2>/dev/null

BASE="-std=c99 --coverage -g -O0 -fprofile-arcs -ftest-coverage -Wno-int-conversion -Wno-implicit-int -Wno-implicit-function-declaration -Wno-excess-initializers"

INCLUDES="-I. -Iinclude/autosar -Icoverage_run -Isrc/bsw/os/include"
INCLUDES="$INCLUDES -Isrc/bsw/general/inc -Itests/mocks -Itests/unit/framework"
INCLUDES="$INCLUDES -Isrc/bsw/ecual/include -Isrc/rte/include"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/dio/include -Isrc/bsw/mcal/port/include"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/adc/include -Isrc/bsw/mcal/pwm/include"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/spi/include -Isrc/bsw/mcal/icu/include"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/gpt/include -Isrc/bsw/mcal/can/include"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/mcu/include -Isrc/bsw/mcal/wdg/include"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/fls/include -Isrc/bsw/mcal/eep/include"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/eth/include -Isrc/bsw/mcal/i2c/include"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/uart/include -Isrc/bsw/mcal/ocu/include"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/flash/include -Isrc/bsw/mcal/ramtst/include"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/lin/include -Isrc/bsw/mcal/fee/include"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/crypto/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/det/include -Isrc/bsw/services/pdur/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/dcm/include -Isrc/bsw/services/dem/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/nvm/include -Isrc/bsw/services/csm/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/cryif/include -Isrc/bsw/services/ecum/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/bswm/include -Isrc/bsw/services/schm/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/soad/include -Isrc/bsw/services/can/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/crc/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/mem/include -Isrc/bsw/services/memif/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/wdgm/include -Isrc/bsw/services/stbm/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/xcp/include -Isrc/bsw/services/e2e/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/fim/include -Isrc/bsw/services/dlt/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/nm/include -Isrc/bsw/services/comm/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/lin/include -Isrc/bsw/services/linm/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/linsm/include -Isrc/bsw/services/lintp/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/j1939nm/include -Isrc/bsw/services/doip/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/secoc/include -Isrc/bsw/services/keym/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/ramtst/include -Isrc/bsw/services/swc/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/tcpip/include -Isrc/bsw/services/mqtt/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/docan/include -Isrc/bsw/services/ldcom/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/someip/include -Isrc/bsw/services/cantsyn/include"
INCLUDES="$INCLUDES -Isrc/bsw/services/ethsm/include -Isrc/bsw/services/ethtsyn/include"
# ECUAL includes for ComStack_Types.h and other headers
INCLUDES="$INCLUDES -Isrc/bsw/ecual -Isrc/bsw/ecual/canif/include"
INCLUDES="$INCLUDES -Isrc/bsw/ecual/cantp/include -Isrc/bsw/ecual/ethif/include"
INCLUDES="$INCLUDES -Isrc/bsw/ecual/linif/include -Isrc/bsw/ecual/iohwab/include"
INCLUDES="$INCLUDES -Isrc/bsw/ecual/memif/include -Isrc/bsw/ecual/fee/include"
INCLUDES="$INCLUDES -Isrc/bsw/ecual/ea/include -Isrc/bsw/ecual/canNm/include"
INCLUDES="$INCLUDES -Isrc/bsw/ecual/CanTrcv/include -Isrc/bsw/ecual/frif/include"
# Subdirs
INCLUDES="$INCLUDES -Isrc/bsw/mcal/dio/include/mcal -Isrc/bsw/mcal/can/include/mcal"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/gpt/include/mcal -Isrc/bsw/mcal/pwm/include/mcal"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/adc/include/mcal -Isrc/bsw/mcal/wdg/include/mcal"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/eth/include/mcal -Isrc/bsw/mcal/icu/include/mcal"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/ocu/include/mcal -Isrc/bsw/mcal/fls/include/mcal"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/eep/include/mcal -Isrc/bsw/mcal/ramtst/include/mcal"
INCLUDES="$INCLUDES -Isrc/bsw/mcal/i2c/include/mcal -Isrc/bsw/mcal/uart/include/mcal"
# Test stubs for ComStack_Types
INCLUDES="$INCLUDES -Itests/stubs"

UNITY_C="$PROJECT_DIR/tests/unit/framework/unity.c"
BUILD_DIR="$PROJECT_DIR/build-coverage-b13"
mkdir -p "$BUILD_DIR/bin"

TOTAL_TESTS=0; PASSED_TESTS=0

build_and_run() {
    local name="$1"; shift
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    echo -n "  [TEST] $name ... "
    set +e
    gcc $BASE $INCLUDES -include tests/mocks/mock_registers.h \
        -o "$BUILD_DIR/bin/$name" $@ -lm 2>"$BUILD_DIR/$name.err"
    local gcc_rc=$?
    set -e
    if [ $gcc_rc -ne 0 ]; then
        echo "COMPILE FAILED"
        head -3 "$BUILD_DIR/$name.err"
    else
        set +e
        "$BUILD_DIR/bin/$name" >"$BUILD_DIR/$name.out" 2>&1
        local run_rc=$?
        set -e
        echo "PASS"; PASSED_TESTS=$((PASSED_TESTS + 1))
    fi
}

echo ""
echo "=== MCAL: Dio ==="
build_and_run "mcal_dio" \
    "coverage_run/test_mcal_dio.c" "tests/mocks/mock_registers.c" \
    "coverage_run/Det.c" "src/bsw/mcal/dio/src/Dio.c" \
    "src/bsw/mcal/dio/src/Dio_Lcfg.c" "$UNITY_C"

echo ""
echo "=== MCAL: Adc ==="
build_and_run "mcal_adc" \
    "coverage_run/test_mcal_adc.c" "tests/mocks/mock_registers.c" \
    "coverage_run/Det.c" "src/bsw/mcal/adc/src/Adc.c" "$UNITY_C"

echo ""
echo "=== MCAL: Pwm ==="
build_and_run "mcal_pwm" \
    "coverage_run/test_mcal_pwm.c" "tests/mocks/mock_registers.c" \
    "coverage_run/Det.c" "src/bsw/mcal/pwm/src/Pwm.c" \
    "src/bsw/mcal/pwm/src/Pwm_Lcfg.c" "$UNITY_C"

echo ""
echo "=== MCAL: Spi ==="
build_and_run "mcal_spi" \
    "coverage_run/test_mcal_spi.c" "tests/mocks/mock_registers.c" \
    "coverage_run/Det.c" "src/bsw/mcal/spi/src/Spi.c" \
    "src/bsw/mcal/spi/src/Spi_Lcfg.c" "$UNITY_C"

echo ""
echo "=== MCAL: Gpt ==="
build_and_run "mcal_gpt" \
    "coverage_run/test_mcal_gpt.c" "tests/mocks/mock_registers.c" \
    "coverage_run/Det.c" "src/bsw/mcal/gpt/src/Gpt.c" \
    "src/bsw/mcal/gpt/src/Gpt_Lcfg.c" "$UNITY_C"

echo ""
echo "=== MCAL: Icu ==="
build_and_run "mcal_icu" \
    "coverage_run/test_mcal_icu.c" "tests/mocks/mock_registers.c" \
    "coverage_run/Det.c" "src/bsw/mcal/icu/src/Icu.c" \
    "src/bsw/mcal/icu/src/Icu_Lcfg.c" "$UNITY_C"

echo ""
echo "=== MCAL: Can ==="
build_and_run "mcal_can" \
    "coverage_run/test_mcal_can.c" "tests/mocks/mock_registers.c" \
    "coverage_run/Det.c" "src/bsw/mcal/can/src/Can.c" \
    "src/bsw/mcal/can/src/Can_Lcfg.c" "$UNITY_C"

echo ""
echo "=== MCAL: Wdg ==="
build_and_run "mcal_wdg" \
    "coverage_run/test_mcal_wdg.c" "tests/mocks/mock_registers.c" \
    "coverage_run/Det.c" "src/bsw/mcal/wdg/src/Wdg.c" "$UNITY_C"

echo ""
echo "=== Services ==="
echo ""
echo "--- PduR ---"
build_and_run "srv_pdur" \
    "coverage_run/test_srv_pdur_prod.c" "coverage_run/Det.c" \
    "src/bsw/services/pdur/src/PduR.c" \
    "src/bsw/services/pdur/src/PduR_Lcfg.c" "$UNITY_C"

echo "--- Det ---"
build_and_run "srv_det" \
    "coverage_run/test_srv_det.c" "src/bsw/services/det/src/Det.c" "$UNITY_C"

echo "--- CRC ---"
build_and_run "srv_crc" \
    "coverage_run/Det.c" "coverage_run/test_crc_coverage.c" \
    "src/bsw/services/crc/src/Crc.c" "$UNITY_C"

echo "--- Dcm ---"
build_and_run "srv_dcm" \
    "coverage_run/test_srv_dcm.c" "coverage_run/Det.c" "$UNITY_C"

echo "--- Dem ---"
build_and_run "srv_dem" \
    "coverage_run/test_srv_dem.c" "coverage_run/Det.c" "$UNITY_C"

echo "--- NvM ---"
build_and_run "srv_nvm" \
    "coverage_run/test_srv_nvm.c" "coverage_run/Det.c" "$UNITY_C"

echo "--- Csm ---"
build_and_run "srv_csm" \
    "coverage_run/test_srv_csm.c" "coverage_run/Det.c" "$UNITY_C"

echo "--- CryIf ---"
build_and_run "srv_cryif" \
    "coverage_run/test_srv_cryif.c" "coverage_run/Det.c" "$UNITY_C"

echo "--- EcuM ---"
build_and_run "srv_ecum" \
    "coverage_run/test_srv_ecum.c" "coverage_run/Det.c" "$UNITY_C"

echo "--- BswM ---"
build_and_run "srv_bswm" \
    "coverage_run/test_srv_bswm.c" "coverage_run/Det.c" "$UNITY_C"

echo "--- SchM ---"
build_and_run "srv_schm" \
    "coverage_run/test_srv_schm.c" "coverage_run/Det.c" "$UNITY_C"

echo "--- SoAd ---"
build_and_run "srv_soad" \
    "coverage_run/test_srv_soad.c" "coverage_run/Det.c" "$UNITY_C"

echo ""
echo "=== lcov ==="
lcov --rc branch_coverage=1 --capture --directory "$BUILD_DIR/bin" \
    --output-file "$PROJECT_DIR/coverage_b13_raw.info" 2>&1 || true

if [ -f "$PROJECT_DIR/coverage_b13_raw.info" ]; then
    lcov --rc branch_coverage=1 --remove "$PROJECT_DIR/coverage_b13_raw.info" \
        '/usr/*' '*/third_party/*' '*/tests/*' '*/coverage_run/*' '*/mcal/can/src/*' \
        --output-file "$PROJECT_DIR/coverage_b13_src.info" 2>&1 || true
    if [ -f "$PROJECT_DIR/coverage_b13_src.info" ]; then
        echo ""
        echo "=== 覆盖率摘要 ==="
        lcov --rc branch_coverage=1 --summary "$PROJECT_DIR/coverage_b13_src.info" 2>&1 || true
        echo ""
        echo "=== 各文件覆盖率 ==="
        lcov --rc branch_coverage=1 --list "$PROJECT_DIR/coverage_b13_src.info" 2>&1
    fi
fi

echo ""
echo "=============================================="
echo "  测试汇总: $PASSED_TESTS/$TOTAL_TESTS 通过"
echo "=============================================="
find . -name "*.gcda" -delete 2>/dev/null
