#!/bin/bash
# YuleTech AutoSAR Build Script
# Usage: ./build.sh [options] [target]

set -e

# Default settings
BUILD_TYPE="Debug"
BUILD_DIR="build"
TOOLCHAIN=""
CLEAN=0
VERBOSE=0
JOBS=$(nproc 2>/dev/null || echo 4)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Help message
function show_help() {
    echo "YuleTech AutoSAR Build Script"
    echo ""
    echo "Usage: $0 [options] [target]"
    echo ""
    echo "Options:"
    echo "  -h, --help           Show this help message"
    echo "  -c, --clean          Clean build directory before building"
    echo "  -t, --type TYPE      Build type (Debug, Release, MinSizeRel)"
    echo "  -a, --arm            Cross-compile for ARM (S32K312)"
    echo "  -n, --native         Native build (host)"
    echo "  -v, --verbose        Verbose build output"
    echo "  -j, --jobs N         Number of parallel jobs (default: auto)"
    echo "  --test               Enable testing"
    echo "  --docs               Build documentation"
    echo "  --coverage           Enable code coverage"
    echo "  --mcal               Build MCAL drivers"
    echo "  --no-mcal            Skip MCAL drivers"
    echo ""
    echo "Targets:"
    echo "  all                  Build all targets (default)"
    echo "  clean                Clean build directory"
    echo "  test                 Run tests"
    echo "  install              Install to system"
    echo "  package              Create package"
    echo ""
    echo "Examples:"
    echo "  $0 -c -a                    Clean build for ARM target"
    echo "  $0 -t Release --mcal        Release build with MCAL"
    echo "  $0 --test test              Build and run tests"
    echo ""
}

# Parse arguments
TARGET="all"
CMAKE_ARGS=""
BUILD_MCAL="ON"
BUILD_TEST="OFF"
BUILD_DOCS="OFF"
ENABLE_COVERAGE="OFF"

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -c|--clean)
            CLEAN=1
            shift
            ;;
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -a|--arm)
            TOOLCHAIN="-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake"
            BUILD_DIR="build-arm"
            shift
            ;;
        -n|--native)
            TOOLCHAIN=""
            BUILD_DIR="build-native"
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
        clean|all|test|install|package)
            TARGET="$1"
            shift
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# Set cmake arguments
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
CMAKE_ARGS="${CMAKE_ARGS} -DBUILD_TESTING=${BUILD_TEST}"
CMAKE_ARGS="${CMAKE_ARGS} -DBUILD_DOCUMENTATION=${BUILD_DOCS}"
CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_COVERAGE=${ENABLE_COVERAGE}"
CMAKE_ARGS="${CMAKE_ARGS} -DYULE_ENABLE_MCAL=${BUILD_MCAL}"
CMAKE_ARGS="${CMAKE_ARGS} ${TOOLCHAIN}"

# Clean if requested
if [ $CLEAN -eq 1 ] || [ "$TARGET" = "clean" ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf ${BUILD_DIR}
    if [ "$TARGET" = "clean" ]; then
        exit 0
    fi
fi

# Create build directory
mkdir -p ${BUILD_DIR}

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}YuleTech AutoSAR Build${NC}"
echo -e "${GREEN}========================================${NC}"
echo "Build type:    ${BUILD_TYPE}"
echo "Build dir:     ${BUILD_DIR}"
echo "Target:        ${TARGET}"
echo "MCAL drivers:  ${BUILD_MCAL}"
echo "Testing:       ${BUILD_TEST}"
echo "Documentation: ${BUILD_DOCS}"
echo "Coverage:      ${ENABLE_COVERAGE}"
echo "Toolchain:     ${TOOLCHAIN:-native}"
echo -e "${GREEN}========================================${NC}"

# Configure
echo -e "${YELLOW}Configuring...${NC}"
if [ $VERBOSE -eq 1 ]; then
    cmake -B ${BUILD_DIR} -S . ${CMAKE_ARGS}
else
    cmake -B ${BUILD_DIR} -S . ${CMAKE_ARGS} 2>&1 | tail -20
fi

# Build
echo -e "${YELLOW}Building...${NC}"
if [ $VERBOSE -eq 1 ]; then
    cmake --build ${BUILD_DIR} --target ${TARGET} --parallel ${JOBS}
else
    cmake --build ${BUILD_DIR} --target ${TARGET} --parallel ${JOBS} 2>&1 | tail -30
fi

# Run tests if requested
if [ "$TARGET" = "test" ]; then
    echo -e "${YELLOW}Running tests...${NC}"
    ctest --test-dir ${BUILD_DIR} --output-on-failure
fi

# Install
if [ "$TARGET" = "install" ]; then
    echo -e "${YELLOW}Installing...${NC}"
    cmake --install ${BUILD_DIR}
fi

# Package
if [ "$TARGET" = "package" ]; then
    echo -e "${YELLOW}Creating package...${NC}"
    cmake --build ${BUILD_DIR} --target package
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Build completed successfully!${NC}"
echo -e "${GREEN}========================================${NC}"
echo "Output directory: ${BUILD_DIR}/"
