#!/bin/bash
# batch_crc_fix.sh — Fix CRC by linking Crc_Lcfg.c
set -euo pipefail
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_DIR"

BASE="-std=c99 --coverage -g -O0 -fprofile-arcs -ftest-coverage"
BASE="$BASE -Wno-int-conversion -Wno-implicit-int -Wno-implicit-function-declaration"

INCLUDES="-I. -Iinclude/autosar -Icoverage_run -Isrc/bsw/os/include -Isrc/bsw/general/inc"
INCLUDES="$INCLUDES -Itests/unit/framework -Itests/stubs"
INCLUDES="$INCLUDES -Isrc/bsw/services/det/include -Isrc/bsw/services/crc/include"

UNITY_C="$PROJECT_DIR/tests/unit/framework/unity.c"

echo "=== CRC with Crc_Lcfg ==="
gcc $BASE $INCLUDES -o /tmp/srv_crc_test \
    "coverage_run/Det.c" "coverage_run/test_crc_coverage.c" \
    "src/bsw/services/crc/src/Crc.c" \
    "src/bsw/services/crc/src/Crc_Lcfg.c" "$UNITY_C" -lm 2>&1

echo "COMPILE: $?"
if [ -x /tmp/srv_crc_test ]; then
    echo ""
    /tmp/srv_crc_test 2>&1 | tail -5
fi