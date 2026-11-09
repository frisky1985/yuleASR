#!/usr/bin/env bash
# run_mcdc_coverage.sh — host MC/DC measurement for ASIL modules
#
# Builds all unit tests with Clang's MC/DC instrumentation, runs them through
# CTest, merges the LLVM profiles and aggregates per-decision records into
# .yuleosh/reports/mcdc-coverage.json.
#
# Requirements: clang-18 + clang++-18 (+ libclang-rt-18-dev), llvm-18
# (llvm-profdata, llvm-cov), cmake, ninja/make. Both C and C++ must be Clang:
# the MC/DC and profile-instrumentation flags are not supported by g++.
#
# Usage:
#   bash tools/run_mcdc_coverage.sh
#   MCDC_GATE=1 bash tools/run_mcdc_coverage.sh   # also run the gate locally
#
# Environment overrides:
#   MCDC_BUILD_DIR, MCDC_PROFILE_DIR, MCDC_REPORT, MCDC_CC, MCDC_CXX, MCDC_JOBS

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${MCDC_BUILD_DIR:-$PROJECT_DIR/build-mcdc}"
PROFILE_DIR="${MCDC_PROFILE_DIR:-$BUILD_DIR/mcdc/profiles}"
REPORT="${MCDC_REPORT:-$PROJECT_DIR/.yuleosh/reports/mcdc-coverage.json}"
CC_NAME="${MCDC_CC:-clang-18}"
# The C++ compiler must be Clang as well: MC/DC instrumentation and the profile
# runtime (-fprofile-instr-generate) are Clang-only, so g++ cannot be used here.
CXX_NAME="${MCDC_CXX:-clang++-18}"

die() {
    echo "error: $*" >&2
    exit 1
}

resolve_tool() {
    local candidate
    for candidate in "$@"; do
        if command -v "$candidate" >/dev/null 2>&1; then
            command -v "$candidate"
            return 0
        fi
    done
    return 1
}

JOBS="${MCDC_JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}"
CC_BIN="$(resolve_tool "$CC_NAME" clang)" || die "clang not found (tried $CC_NAME, clang)"
CXX_BIN="$(resolve_tool "$CXX_NAME" clang++)" || die "C++ compiler not found (tried $CXX_NAME, clang++)"
PROFDATA_BIN="$(resolve_tool llvm-profdata-18 llvm-profdata)" || die "llvm-profdata not found (install llvm-18)"
LLVM_COV_BIN="$(resolve_tool llvm-cov-18 llvm-cov)" || die "llvm-cov not found (install llvm-18)"

# MC/DC profiles need the Clang instrumented runtime at link time.
PROFILE_RT="$("$CC_BIN" -print-file-name=libclang_rt.profile-x86_64.a 2>/dev/null || true)"
if [ ! -f "$PROFILE_RT" ]; then
    die "Clang profile runtime not found — install libclang-rt-18-dev"
fi

GENERATOR_ARGS=()
if command -v ninja >/dev/null 2>&1; then
    GENERATOR_ARGS=(-G Ninja)
fi

MCDC_FLAGS="-fcoverage-mcdc -fprofile-instr-generate -fcoverage-mapping -O0 -g"
# Compatibility flags required to build the existing GCC-oriented sources with
# Clang 18 without changing production code.
MCDC_COMPAT="-Wno-implicit-function-declaration -Wno-int-conversion \
-Wno-incompatible-function-pointer-types -Wno-unknown-warning-option"

echo "=== MC/DC measurement (host) ==="
echo "  compiler:   $CC_BIN"
echo "  llvm-cov:   $LLVM_COV_BIN"
echo "  build dir:  $BUILD_DIR"
echo "  report:     $REPORT"

echo " -> Configuring..."
# A generator switch or compiler change is not supported by an existing cache;
# recreate the dir when the requested configuration differs from the cached one.
if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
    if [ ${#GENERATOR_ARGS[@]} -gt 0 ] && ! grep -q "CMAKE_GENERATOR:INTERNAL=Ninja" "$BUILD_DIR/CMakeCache.txt"; then
        echo "    generator changed, removing $BUILD_DIR"
        rm -rf "$BUILD_DIR"
    elif ! grep -q "CMAKE_CXX_COMPILER:FILEPATH=$CXX_BIN" "$BUILD_DIR/CMakeCache.txt"; then
        echo "    compiler changed, removing $BUILD_DIR"
        rm -rf "$BUILD_DIR"
    fi
fi
cmake -B "$BUILD_DIR" -S "$PROJECT_DIR" "${GENERATOR_ARGS[@]}" \
    -DCMAKE_C_COMPILER="$CC_BIN" \
    -DCMAKE_CXX_COMPILER="$CXX_BIN" \
    -DBUILD_TESTING=ON \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_FLAGS="$MCDC_FLAGS $MCDC_COMPAT" \
    -DCMAKE_CXX_FLAGS="$MCDC_FLAGS $MCDC_COMPAT" \
    -DCMAKE_EXE_LINKER_FLAGS="-fprofile-instr-generate" >/dev/null

echo " -> Building..."
cmake --build "$BUILD_DIR" --parallel "$JOBS"

echo " -> Running tests and aggregating MC/DC..."
rm -rf "$PROFILE_DIR"
mkdir -p "$PROFILE_DIR"
MCDC_CC="$CC_BIN" python3 "$PROJECT_DIR/tools/aggregate_mcdc.py" \
    --build-dir "$BUILD_DIR" \
    --profile-dir "$PROFILE_DIR" \
    --output "$REPORT" \
    --root "$PROJECT_DIR" \
    --llvm-cov "$LLVM_COV_BIN" \
    --llvm-profdata "$PROFDATA_BIN" \
    --jobs "$JOBS"

if [ "${MCDC_GATE:-0}" = "1" ]; then
    echo " -> Running MC/DC gate..."
    python3 "$PROJECT_DIR/tools/check_mcdc_gate.py" "$REPORT"
fi

echo "=== MC/DC measurement complete ==="
