#!/bin/bash
#==================================================================================================
# Project              : yuleASR AUTOSAR BSW
# Script               : Run Native SIL Smoke (real BSW modules, host build)
# Date                 : 2026-08-07
#
# Description: P0-3 SIL 落地 — 从 hello.elf 示例升级为真实产品用例。
#              在 host 上编译真实 BSW 模块（Crc / E2E_P01）并执行冒烟断言，
#              输出 .osh/ci/sil-test-results.json（SIL 证据链）。
#
# 用法:
#   ./tests/sil/run_native_sil.sh          # 编译 + 运行 + 写结果
#   ./tests/sil/run_native_sil.sh --build  # 仅编译
#==================================================================================================

set -e

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SIL_DIR="${PROJECT_ROOT}/tests/sil"
BUILD_DIR="${PROJECT_ROOT}/build-sil"
RESULT_DIR="${PROJECT_ROOT}/.osh/ci"
RESULT_FILE="${RESULT_DIR}/sil-test-results.json"
CC="${CC:-cc}"

echo -e "${CYAN}== yuleASR Native SIL Smoke (real BSW modules) ==${NC}"

# ---- 编译：真实 BSW 模块 Crc + E2E_P01，host 原生 ----
mkdir -p "${BUILD_DIR}"
echo "  Compiling real BSW modules (Crc.c, E2E_P01.c) natively..."
"${CC}" -std=c99 -O2 -Wall -Wextra \
  -I "${PROJECT_ROOT}/src/bsw/services/crc/include" \
  -I "${PROJECT_ROOT}/src/bsw/services/e2e/include" \
  -I "${PROJECT_ROOT}/src/bsw/services/det/include" \
  -I "${PROJECT_ROOT}/src/bsw/common" \
  -I "${PROJECT_ROOT}/src/bsw/services" \
  -I "${PROJECT_ROOT}/include/autosar" \
  "${SIL_DIR}/sil_smoke_main.c" \
  "${PROJECT_ROOT}/src/bsw/services/crc/src/Crc.c" \
  "${PROJECT_ROOT}/src/bsw/services/crc/src/Crc_Lcfg.c" \
  "${PROJECT_ROOT}/src/bsw/services/e2e/src/E2E_P01.c" \
  -o "${BUILD_DIR}/sil_smoke"
echo -e "${GREEN}  Build OK: ${BUILD_DIR}/sil_smoke${NC}"

if [ "$1" = "--build" ]; then
  exit 0
fi

# ---- 运行冒烟 ----
echo "  Running SIL smoke..."
START=$(date +%s)
set +e
OUTPUT=$("${BUILD_DIR}/sil_smoke" 2>&1)
RUN_RC=$?
set -e
END=$(date +%s)
ELAPSED=$((END - START))

echo "${OUTPUT}"

if [ ${RUN_RC} -eq 0 ]; then
  ALL_PASSED=true
  STATUS="passed"
  echo -e "${GREEN}  SIL smoke: ALL PASS${NC}"
else
  ALL_PASSED=false
  STATUS="failed"
  echo -e "${RED}  SIL smoke: FAILED (rc=${RUN_RC})${NC}"
fi

# ---- 写证据链结果（用 python3 保证 JSON 合法转义） ----
mkdir -p "${RESULT_DIR}"
ALL_PASSED_BOOL="false"; [ "${ALL_PASSED}" = "true" ] && ALL_PASSED_BOOL="true"
TIMESTAMP=$(date -u +%Y-%m-%dT%H:%M:%SZ)
LOG_SNIPPET_JSON=$(python3 -c "import json,sys; print(json.dumps(sys.stdin.read()[-2000:]))" <<< "${OUTPUT}")
cat > "${RESULT_FILE}" <<EOF
{
  "layer": 2,
  "stage": "sil-tests",
  "mode": "native-host",
  "modules": ["Crc", "E2E_P01"],
  "timestamp": "${TIMESTAMP}",
  "all_passed": ${ALL_PASSED_BOOL},
  "results": [
    {
      "elf": "sil_smoke_crc_e2e",
      "module": "Crc/E2E_P01",
      "platform": "host-native",
      "passed": ${ALL_PASSED_BOOL},
      "elapsed": ${ELAPSED},
      "error": null,
      "assertion_failures": [],
      "log_snippet": ${LOG_SNIPPET_JSON}
    }
  ]
}
EOF
echo -e "${GREEN}  Results: ${RESULT_FILE}${NC}"

[ "${ALL_PASSED}" = "true" ] || exit 1
