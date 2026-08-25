#!/bin/bash
# ==============================================================================
# yuleASR Unified Build Entry Point
# ==============================================================================
# Usage:
#   ./build.sh [options] [target]
#
# Quick reference:
#   ./build.sh                  # Native host build (Debug)
#   ./build.sh -a               # ARM cross-compile for S32K312
#   ./build.sh --test           # Build + run unit tests
#   ./build.sh -a --module mcal # Cross-compile specific module
#   ./build.sh clean            # Clean all build artifacts
#   ./build.sh --help           # Full help
#
# Copyright (c) 2024 YuleTech
# ==============================================================================

set -e

# ── Defaults ────────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE="Debug"
BUILD_DIR=""
TOOLCHAIN=""
TARGET_ARCH="native"
CLEAN=0
CLEAN_ONLY=0
VERBOSE=0
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
BUILD_TEST="OFF"
BUILD_DOCS="OFF"
ENABLE_COVERAGE="OFF"
BUILD_MCAL="ON"
BUILD_EXAMPLES="OFF"
MODULE=""
CMAKE_TARGET="all"
EXTRA_CMAKE_ARGS=""

# ── Colors ──────────────────────────────────────────────────────────────────
if [ -t 1 ]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    BLUE='\033[0;34m'
    BOLD='\033[1m'
    NC='\033[0m'
else
    RED='' GREEN='' YELLOW='' BLUE='' BOLD='' NC=''
fi

# ── Help ────────────────────────────────────────────────────────────────────
show_help() {
    cat <<'HELP'
yuleASR Unified Build Script

USAGE:
    ./build.sh [OPTIONS] [TARGET]

TARGETS:
    all             Build all targets (default)
    clean           Clean build artifacts
    test            Build and run unit tests
    install         Install headers and libraries
    package         Create distributable package

OPTIONS:
    -h, --help          Show this help message
    -a, --arm           Cross-compile for ARM target (S32K312 Cortex-M33)
    -n, --native        Native host build (x86_64, for unit testing)
    -t, --type TYPE     Build type: Debug, Release, MinSizeRel, RelWithDebInfo
                        (default: Debug)
    -c, --clean         Clean build directory before building
    -v, --verbose       Show full compiler output
    -j, --jobs N        Parallel build jobs (default: auto-detect)
    --test              Enable unit test build
    --docs              Build API documentation (requires Doxygen)
    --coverage          Enable code coverage instrumentation
    --mcal              Build MCAL drivers (default: ON)
    --no-mcal           Skip MCAL drivers
    --module NAME       Build only a specific module (e.g., mcal, os, services)
    --examples          Build example applications
    --cmake-args ARGS   Pass extra arguments to cmake configure

EXAMPLES:
    # Native debug build
    ./build.sh

    # Native release build with tests
    ./build.sh -t Release --test

    # ARM cross-compile for S32K312
    ./build.sh -a

    # ARM cross-compile, release, verbose
    ./build.sh -a -t Release -v

    # Build and run unit tests with coverage
    ./build.sh --test --coverage

    # Build only the MCAL module
    ./build.sh --module mcal

    # Clean all build artifacts
    ./build.sh clean

    # Full CI pipeline: clean, configure, build, test, coverage
    ./build.sh -c --test --coverage

ENVIRONMENT VARIABLES:
    ARM_GCC_PATH        Path to ARM GCC toolchain (e.g., /opt/gcc-arm-none-eabi)
    CMAKE_GENERATOR     Override CMake generator (e.g., Ninja)
    CC / CXX            Override host compiler

HELP
}

# ── Argument parsing ────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -a|--arm)
            TARGET_ARCH="arm"
            shift
            ;;
        -n|--native)
            TARGET_ARCH="native"
            shift
            ;;
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN=1
            shift
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        --test)
            BUILD_TEST="ON"
            shift
            ;;
        --docs)
            BUILD_DOCS="ON"
            shift
            ;;
        --coverage)
            ENABLE_COVERAGE="ON"
            BUILD_TEST="ON"   # coverage requires tests
            shift
            ;;
        --mcal)
            BUILD_MCAL="ON"
            shift
            ;;
        --no-mcal)
            BUILD_MCAL="OFF"
            shift
            ;;
        --module)
            MODULE="$2"
            shift 2
            ;;
        --examples)
            BUILD_EXAMPLES="ON"
            shift
            ;;
        --cmake-args)
            EXTRA_CMAKE_ARGS="$2"
            shift 2
            ;;
        clean)
            CLEAN_ONLY=1
            shift
            ;;
        test)
            BUILD_TEST="ON"
            CMAKE_TARGET="all"
            shift
            ;;
        all|install|package)
            CMAKE_TARGET="$1"
            shift
            ;;
        *)
            echo -e "${RED}Error: Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# ── Determine build directory ───────────────────────────────────────────────
case $TARGET_ARCH in
    arm)
        BUILD_DIR="${SCRIPT_DIR}/build-arm"
        TOOLCHAIN="-DCMAKE_TOOLCHAIN_FILE=${SCRIPT_DIR}/cmake/toolchain-arm-none-eabi.cmake"
        ;;
    native|*)
        BUILD_DIR="${SCRIPT_DIR}/build-native"
        TOOLCHAIN=""
        ;;
esac

# ── Clean ───────────────────────────────────────────────────────────────────
do_clean() {
    local dirs=("${BUILD_DIR}")
    # Also clean the legacy build dir names if they exist
    if [ "$TARGET_ARCH" = "arm" ]; then
        dirs+=("${SCRIPT_DIR}/build-s0-arm")
    fi

    for d in "${dirs[@]}"; do
        if [ -d "$d" ]; then
            echo -e "${YELLOW}Cleaning: ${d}${NC}"
            rm -rf "$d"
        fi
    done

    # Clean generic build artifacts
    if [ -d "${SCRIPT_DIR}/build" ]; then
        echo -e "${YELLOW}Cleaning: ${SCRIPT_DIR}/build${NC}"
        rm -rf "${SCRIPT_DIR}/build"
    fi

    echo -e "${GREEN}Clean complete.${NC}"
}

if [ $CLEAN_ONLY -eq 1 ]; then
    do_clean
    exit 0
fi

if [ $CLEAN -eq 1 ]; then
    do_clean
fi

# ── Create build directory ──────────────────────────────────────────────────
mkdir -p "${BUILD_DIR}"

# ── Print build configuration ───────────────────────────────────────────────
echo -e "${GREEN}============================================${NC}"
echo -e "${GREEN}  yuleASR Build System${NC}"
echo -e "${GREEN}============================================${NC}"
echo -e "  Architecture:   ${BOLD}${TARGET_ARCH}${NC}"
echo -e "  Build type:     ${BUILD_TYPE}"
echo -e "  Build dir:      ${BUILD_DIR}"
echo -e "  Toolchain:      ${TOOLCHAIN:-native (host)}"
echo -e "  MCAL:           ${BUILD_MCAL}"
echo -e "  Tests:          ${BUILD_TEST}"
echo -e "  Coverage:       ${ENABLE_COVERAGE}"
echo -e "  Documentation:  ${BUILD_DOCS}"
echo -e "  Examples:       ${BUILD_EXAMPLES}"
echo -e "  Parallel jobs:  ${JOBS}"
if [ -n "$MODULE" ]; then
    echo -e "  Module:         ${BOLD}${MODULE}${NC}"
fi
echo -e "${GREEN}============================================${NC}"

# ── CMake configure arguments ───────────────────────────────────────────────
CMAKE_ARGS=(
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
    -DBUILD_TESTING="${BUILD_TEST}"
    -DBUILD_DOCUMENTATION="${BUILD_DOCS}"
    -DBUILD_EXAMPLES="${BUILD_EXAMPLES}"
    -DENABLE_COVERAGE="${ENABLE_COVERAGE}"
    -DYULE_ENABLE_MCAL="${BUILD_MCAL}"
)

if [ -n "$TOOLCHAIN" ]; then
    CMAKE_ARGS+=("$TOOLCHAIN")
fi

if [ -n "$EXTRA_CMAKE_ARGS" ]; then
    # shellcheck disable=SC2206
    CMAKE_ARGS+=($EXTRA_CMAKE_ARGS)
fi

# ── Configure ───────────────────────────────────────────────────────────────
echo -e "${BLUE}[1/3] Configuring...${NC}"
if [ $VERBOSE -eq 1 ]; then
    cmake -B "${BUILD_DIR}" -S "${SCRIPT_DIR}" "${CMAKE_ARGS[@]}"
else
    cmake -B "${BUILD_DIR}" -S "${SCRIPT_DIR}" "${CMAKE_ARGS[@]}" 2>&1 | tail -25
fi

# ── Build ───────────────────────────────────────────────────────────────────
echo -e "${BLUE}[2/3] Building...${NC}"

BUILD_CMD=(cmake --build "${BUILD_DIR}" --parallel "${JOBS}")

if [ "$CMAKE_TARGET" != "all" ]; then
    BUILD_CMD+=(--target "$CMAKE_TARGET")
fi

# If a specific module is requested, build only that target
if [ -n "$MODULE" ]; then
    BUILD_CMD+=(--target "$MODULE")
fi

if [ $VERBOSE -eq 1 ]; then
    BUILD_CMD+=(--verbose)
    "${BUILD_CMD[@]}"
else
    "${BUILD_CMD[@]}" 2>&1 | tail -30
fi

# ── Post-build: size summary for ARM builds ─────────────────────────────────
if [ "$TARGET_ARCH" = "arm" ] && [ -d "${BUILD_DIR}/bin" ]; then
    echo ""
    echo -e "${BLUE}Binary sizes:${NC}"
    for elf in "${BUILD_DIR}/bin/"*.elf; do
        if [ -f "$elf" ]; then
            arm-none-eabi-size "$elf" 2>/dev/null || size "$elf" 2>/dev/null || true
        fi
    done
fi

# ── Test ────────────────────────────────────────────────────────────────────
if [ "$BUILD_TEST" = "ON" ] && [ "$CMAKE_TARGET" != "install" ] && [ "$CMAKE_TARGET" != "package" ]; then
    echo -e "${BLUE}[3/3] Running tests...${NC}"
    if [ -n "$MODULE" ]; then
        # Run only tests matching the module name
        ctest --test-dir "${BUILD_DIR}" --output-on-failure -R "${MODULE}"
    else
        ctest --test-dir "${BUILD_DIR}" --output-on-failure
    fi
fi

# ── Install ─────────────────────────────────────────────────────────────────
if [ "$CMAKE_TARGET" = "install" ]; then
    echo -e "${BLUE}Installing...${NC}"
    cmake --install "${BUILD_DIR}"
fi

# ── Package ─────────────────────────────────────────────────────────────────
if [ "$CMAKE_TARGET" = "package" ]; then
    echo -e "${BLUE}Creating package...${NC}"
    cmake --build "${BUILD_DIR}" --target package
fi

# ── Done ────────────────────────────────────────────────────────────────────
echo ""
echo -e "${GREEN}============================================${NC}"
echo -e "${GREEN}  Build completed successfully!${NC}"
echo -e "${GREEN}============================================${NC}"
echo -e "  Output: ${BUILD_DIR}/"
if [ "$TARGET_ARCH" = "arm" ]; then
    echo -e "  ELF:    ${BUILD_DIR}/bin/*.elf"
    echo -e "  BIN:    ${BUILD_DIR}/bin/*.bin"
    echo -e "  HEX:    ${BUILD_DIR}/bin/*.hex"
    echo -e "  MAP:    ${BUILD_DIR}/*.map"
fi
if [ "$BUILD_TEST" = "ON" ]; then
    echo -e "  Tests:  ${BUILD_DIR}/tests/"
fi
echo ""
