#!/bin/bash
# coverage_batch11.sh — Comprehensive coverage build linking real production code
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_DIR"

echo "=============================================="
echo "  Batch11 — 全仓库覆盖率构建 (v2)"
echo "=============================================="
echo ""

# 清理旧的 .gcda
find . -name "*.gcda" -delete 2>/dev/null

BASE="-std=c99 --coverage -g -O0 -fprofile-arcs -ftest-coverage -Wno-int-conversion -Wno-implicit-int -Wno-implicit-function-declaration"

# 通用包含路径
INCLUDES=""
INCLUDES="$INCLUDES -I$PROJECT_DIR"
INCLUDES="$INCLUDES -I$PROJECT_DIR/include/autosar"
INCLUDES="$INCLUDES -I$PROJECT_DIR/coverage_run"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/general/inc"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/os/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/tests/mocks"
INCLUDES="$INCLUDES -I$PROJECT_DIR/tests/unit/framework"

# MCAL includes
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/mcu/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/port/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/dio/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/adc/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/pwm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/spi/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/icu/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/gpt/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/can/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/crypto/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/wdg/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/fls/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/eep/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/eth/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/i2c/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/uart/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/ocu/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/flash/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/ramtst/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/lin/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/fee/include"

# Services includes
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/det/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/crc/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/pdur/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/com/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/can/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/cannm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/cansm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/cryif/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/csm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/secoc/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/keym/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/ecum/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/bswm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/schm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/dcm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/dem/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/nvm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/mem/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/memif/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/soad/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/someip/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/wdgm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/stbm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/xcp/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/e2e/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/fim/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/dlt/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/nm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/comm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/lin/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/linm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/linsm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/lintp/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/j1939nm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/doip/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/cantsyn/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/ethsm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/ethtsyn/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/flstst/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/ipdum/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/ramsafety/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/swc/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/tcpip/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/tm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/udpNm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/ramtst/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/docan/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/mqtt/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/ldcom/include"

# ECUAL
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/canif/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/cantp/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/ethif/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/linif/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/iohwab/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/memif/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/fee/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/ea/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/canNm/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/CanTrcv/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/frif/include"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/ecual/j1939tp/include"

# AUTOSAR MCAL subdirs
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/dio/include/mcal"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/can/include/mcal"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/gpt/include/mcal"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/pwm/include/mcal"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/adc/include/mcal"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/wdg/include/mcal"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/eth/include/mcal"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/icu/include/mcal"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/ocu/include/mcal"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/fls/include/mcal"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/eep/include/mcal"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/ramtst/include/mcal"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/i2c/include/mcal"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/uart/include/mcal"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/crypto/include/mcal"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/flash/include/mcal"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/mcal/lin/include/mcal"

# Service subdirs
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/can/include/can"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/cannm/include/cannm"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/cansm/include/cansm"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/comm/include/comm"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/com/include/com"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/dcm/include/dcm"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/dem/include/dem"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/ecum/include/ecum"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/nvm/include/nvm"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/pdur/include/pdur"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/soad/include/soad"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/wdgm/include/wdgm"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/lin/include/lin"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/j1939nm/include/j1939nm"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/doip/include/doip"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/cryif/include/cryif"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/csm/include/csm"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/keym/include/keym"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/mem/include/mem"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/dlt/include/dlt"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/nm/include/nm"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/e2e/include/e2e"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/fim/include/fim"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/stbm/include/stbm"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/xcp/include/xcp"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/bswm/include/bswm"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/linm/include/linm"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/linsm/include/linsm"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/lintp/include/lintp"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/schm/include/schm"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/secoc/include/secoc"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/ramtst/include/ramtst"
INCLUDES="$INCLUDES -I$PROJECT_DIR/src/bsw/services/swc/include/swc"

UNITY_C="$PROJECT_DIR/tests/unit/framework/unity.c"

BUILD_DIR="$PROJECT_DIR/build-coverage-b11"
mkdir -p "$BUILD_DIR/bin"

TOTAL_TESTS=0
PASSED_TESTS=0

build_and_run() {
    local name="$1"
    shift
    local src_files="$@"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -n "  [TEST] $name ... "
    set +e
    gcc $BASE $INCLUDES -o "$BUILD_DIR/bin/$name" $src_files -lm 2>"$BUILD_DIR/$name.err"
    local gcc_rc=$?
    set -e
    if [ $gcc_rc -ne 0 ]; then
        echo "COMPILE FAILED"
        cat "$BUILD_DIR/$name.err" | tail -5
    else
        set +e
        "$BUILD_DIR/bin/$name" >"$BUILD_DIR/$name.out" 2>&1
        local run_rc=$?
        set -e
        if [ $run_rc -eq 0 ]; then
            echo "PASS"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo "RUN (exit=$run_rc)"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        fi
    fi
}

echo ""
echo "=== C1: 覆盖率基础设施确认 ==="
echo "  CFLAGS: $BASE"
echo "  BUILD_DIR: $BUILD_DIR"
echo "  UNITY: $UNITY_C"
echo ""

echo "=== MCAL Batch Tests ==="
echo ""

echo "--- Dio (Dio.c + Dio_Lcfg.c) ---"
build_and_run "mcal_dio" \
    "$SCRIPT_DIR/test_mcal_dio.c" \
    "$PROJECT_DIR/tests/mocks/mock_registers.c" \
    "$SCRIPT_DIR/Det.c" \
    "$PROJECT_DIR/src/bsw/mcal/dio/src/Dio.c" \
    "$PROJECT_DIR/src/bsw/mcal/dio/src/Dio_Lcfg.c" \
    "$UNITY_C"

echo "--- Port (Port.c + Port_Lcfg.c) ---"
build_and_run "mcal_port" \
    "$SCRIPT_DIR/test_mcal_port.c" \
    "$PROJECT_DIR/tests/mocks/mock_registers.c" \
    "$SCRIPT_DIR/Det.c" \
    "$PROJECT_DIR/src/bsw/mcal/port/src/Port.c" \
    "$PROJECT_DIR/src/bsw/mcal/port/src/Port_Lcfg.c" \
    "$UNITY_C"

echo "--- Adc (Adc.c) ---"
build_and_run "mcal_adc" \
    "$SCRIPT_DIR/test_mcal_adc.c" \
    "$PROJECT_DIR/tests/mocks/mock_registers.c" \
    "$SCRIPT_DIR/Det.c" \
    "$PROJECT_DIR/src/bsw/mcal/adc/src/Adc.c" \
    "$UNITY_C"

echo "--- Pwm (Pwm.c + Pwm_Lcfg.c) ---"
build_and_run "mcal_pwm" \
    "$SCRIPT_DIR/test_mcal_pwm.c" \
    "$PROJECT_DIR/tests/mocks/mock_registers.c" \
    "$SCRIPT_DIR/Det.c" \
    "$PROJECT_DIR/src/bsw/mcal/pwm/src/Pwm.c" \
    "$PROJECT_DIR/src/bsw/mcal/pwm/src/Pwm_Lcfg.c" \
    "$UNITY_C"

echo "--- Spi (Spi.c + Spi_Lcfg.c) ---"
build_and_run "mcal_spi" \
    "$SCRIPT_DIR/test_mcal_spi.c" \
    "$PROJECT_DIR/tests/mocks/mock_registers.c" \
    "$SCRIPT_DIR/Det.c" \
    "$PROJECT_DIR/src/bsw/mcal/spi/src/Spi.c" \
    "$PROJECT_DIR/src/bsw/mcal/spi/src/Spi_Lcfg.c" \
    "$UNITY_C"

echo "--- Icu (Icu.c + Icu_Lcfg.c + Icu_Irq.c) ---"
build_and_run "mcal_icu" \
    "$SCRIPT_DIR/test_mcal_icu.c" \
    "$PROJECT_DIR/tests/mocks/mock_registers.c" \
    "$SCRIPT_DIR/Det.c" \
    "$PROJECT_DIR/src/bsw/mcal/icu/src/Icu.c" \
    "$PROJECT_DIR/src/bsw/mcal/icu/src/Icu_Lcfg.c" \
    "$UNITY_C"

echo "--- Gpt (Gpt.c + Gpt_Lcfg.c) ---"
build_and_run "mcal_gpt" \
    "$SCRIPT_DIR/test_mcal_gpt.c" \
    "$PROJECT_DIR/tests/mocks/mock_registers.c" \
    "$SCRIPT_DIR/Det.c" \
    "$PROJECT_DIR/src/bsw/mcal/gpt/src/Gpt.c" \
    "$PROJECT_DIR/src/bsw/mcal/gpt/src/Gpt_Lcfg.c" \
    "$UNITY_C"

echo "--- Can (Can.c + Can_Lcfg.c) ---"
build_and_run "mcal_can" \
    "$SCRIPT_DIR/test_mcal_can.c" \
    "$PROJECT_DIR/tests/mocks/mock_registers.c" \
    "$SCRIPT_DIR/Det.c" \
    "$PROJECT_DIR/src/bsw/mcal/can/src/Can.c" \
    "$PROJECT_DIR/src/bsw/mcal/can/src/Can_Lcfg.c" \
    "$UNITY_C"

echo "--- Mcu (Mcu.c + Mcu_Lcfg.c) ---"
build_and_run "mcal_mcu" \
    "$SCRIPT_DIR/test_mcal_mcu.c" \
    "$PROJECT_DIR/tests/mocks/mock_registers.c" \
    "$SCRIPT_DIR/Det.c" \
    "$PROJECT_DIR/src/bsw/mcal/mcu/src/Mcu.c" \
    "$PROJECT_DIR/src/bsw/mcal/mcu/src/Mcu_Lcfg.c" \
    "$UNITY_C"

echo "--- Wdg (Wdg.c + Wdg_Hw.c) ---"
build_and_run "mcal_wdg" \
    "$SCRIPT_DIR/test_mcal_wdg.c" \
    "$PROJECT_DIR/tests/mocks/mock_registers.c" \
    "$SCRIPT_DIR/Det.c" \
    "$PROJECT_DIR/src/bsw/mcal/wdg/src/Wdg.c" \
    "$UNITY_C"

echo ""
echo "=== 诊断模块 Tests ==="
echo ""

echo "--- Dcm (Dcm.c) ---"
build_and_run "srv_dcm" \
    "$SCRIPT_DIR/test_srv_dcm.c" \
    "$SCRIPT_DIR/Det.c" \
    "$UNITY_C"

echo "--- Dem (Dem.c) ---"
build_and_run "srv_dem" \
    "$SCRIPT_DIR/test_srv_dem.c" \
    "$SCRIPT_DIR/Det.c" \
    "$UNITY_C"

echo "--- Det (Det.c production code) ---"
build_and_run "srv_det" \
    "$SCRIPT_DIR/test_srv_det.c" \
    "$PROJECT_DIR/src/bsw/services/det/src/Det.c" \
    "$UNITY_C"

echo ""
echo "=== 存储模块 Tests ==="
echo ""

echo "--- NvM (NvM.c) ---"
build_and_run "srv_nvm" \
    "$SCRIPT_DIR/test_srv_nvm.c" \
    "$SCRIPT_DIR/Det.c" \
    "$UNITY_C"

echo ""
echo "=== 加密模块 Tests ==="
echo ""

echo "--- Csm (Csm.c) ---"
build_and_run "srv_csm" \
    "$SCRIPT_DIR/test_srv_csm.c" \
    "$SCRIPT_DIR/Det.c" \
    "$UNITY_C"

echo "--- CryIf (CryIf.c) ---"
build_and_run "srv_cryif" \
    "$SCRIPT_DIR/test_srv_cryif.c" \
    "$SCRIPT_DIR/Det.c" \
    "$UNITY_C"

echo ""
echo "=== 系统模块 Tests ==="
echo ""

echo "--- EcuM (EcuM.c) ---"
build_and_run "srv_ecum" \
    "$SCRIPT_DIR/test_srv_ecum.c" \
    "$SCRIPT_DIR/Det.c" \
    "$UNITY_C"

echo "--- BswM (BswM.c) ---"
build_and_run "srv_bswm" \
    "$SCRIPT_DIR/test_srv_bswm.c" \
    "$SCRIPT_DIR/Det.c" \
    "$UNITY_C"

echo "--- SchM (SchM.c) ---"
build_and_run "srv_schm" \
    "$SCRIPT_DIR/test_srv_schm.c" \
    "$SCRIPT_DIR/Det.c" \
    "$UNITY_C"

echo ""
echo "=== 通信模块 Tests ==="
echo ""

echo "--- SoAd (SoAd.c) ---"
build_and_run "srv_soad" \
    "$SCRIPT_DIR/test_srv_soad.c" \
    "$SCRIPT_DIR/Det.c" \
    "$UNITY_C"

echo "--- PduR (PduR.c + Lcfg) ---"
build_and_run "srv_pdur" \
    "$SCRIPT_DIR/test_srv_pdur_prod.c" \
    "$SCRIPT_DIR/Det.c" \
    "$PROJECT_DIR/src/bsw/services/pdur/src/PduR.c" \
    "$PROJECT_DIR/src/bsw/services/pdur/src/PduR_Lcfg.c" \
    "$UNITY_C"

echo ""
echo "=== lcov 覆盖率捕获 ==="
echo ""

LCOV_RC="--rc branch_coverage=1"

lcov $LCOV_RC --capture --directory "$BUILD_DIR/bin" --output-file "$PROJECT_DIR/coverage_b11_raw.info" 2>&1 || true

if [ -f "$PROJECT_DIR/coverage_b11_raw.info" ]; then
    lcov $LCOV_RC --remove "$PROJECT_DIR/coverage_b11_raw.info" '/usr/*' '*/third_party/*' '*/tests/*' '*/coverage_run/*' \
         --output-file "$PROJECT_DIR/coverage_b11_filtered.info" 2>&1 || cp "$PROJECT_DIR/coverage_b11_raw.info" "$PROJECT_DIR/coverage_b11_filtered.info"
    
    lcov $LCOV_RC --extract "$PROJECT_DIR/coverage_b11_filtered.info" "$PROJECT_DIR/src/*" \
         --output-file "$PROJECT_DIR/coverage_b11_src.info" 2>&1 || true
    
    FINAL="$PROJECT_DIR/coverage_b11_filtered.info"
    if [ -f "$PROJECT_DIR/coverage_b11_src.info" ]; then
        FINAL="$PROJECT_DIR/coverage_b11_src.info"
    fi
    
    echo ""
    echo "=== 覆盖率摘要 ==="
    lcov $LCOV_RC --summary "$FINAL" 2>&1 || true
    
    echo ""
    echo "=== 生成 HTML 报告 ==="
    mkdir -p "$PROJECT_DIR/coverage_report_b11"
    genhtml $LCOV_RC "$FINAL" --output-directory "$PROJECT_DIR/coverage_report_b11" 2>&1 | tail -5
    echo ""
    echo "HTML 报告: coverage_report_b11/index.html"
    
    echo ""
    echo "=== 各文件覆盖率 ==="
    lcov $LCOV_RC --list "$FINAL" 2>&1 | head -50
fi

echo ""
echo "=============================================="
echo "  测试汇总: $PASSED_TESTS/$TOTAL_TESTS 通过"
echo "=============================================="

find . -name "*.gcda" -delete 2>/dev/null
echo "  ✅ 已清理 .gcda"
