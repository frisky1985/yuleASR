#!/bin/bash
# ETH-DDS Integration Build Script

set -e

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== ETH-DDS Integration Build Script ===${NC}"

# 确定构建目录
BUILD_DIR="build"
BUILD_TYPE="Release"
CROSS_COMPILE=""

# 解析参数
while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --clean)
            echo -e "${YELLOW}Cleaning build directory...${NC}"
            rm -rf ${BUILD_DIR}
            shift
            ;;
        --cross-compile)
            CROSS_COMPILE="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --debug          Build in debug mode"
            echo "  --clean          Clean build directory"
            echo "  --cross-compile  Set cross compiler prefix"
            echo "  --help           Show this help"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# 创建构建目录
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

# 配置CMake
echo -e "${GREEN}Configuring CMake...${NC}"
echo "Build type: ${BUILD_TYPE}"

CMAKE_ARGS="-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
CMAKE_ARGS="${CMAKE_ARGS} -DBUILD_EXAMPLES=ON"
CMAKE_ARGS="${CMAKE_ARGS} -DBUILD_TESTS=ON"

if [ -n "${CROSS_COMPILE}" ]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_C_COMPILER=${CROSS_COMPILE}gcc"
    CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_CXX_COMPILER=${CROSS_COMPILE}g++"
fi

cmake .. ${CMAKE_ARGS}

# 构建项目
echo -e "${GREEN}Building project...${NC}"
make -j$(nproc)

echo -e "${GREEN}Build completed successfully!${NC}"
echo ""
echo "Output files:"
echo "  Library: ${BUILD_DIR}/libeth_dds.a"
echo "  Examples: ${BUILD_DIR}/examples/"
echo ""
echo "To run example:"
echo "  ./${BUILD_DIR}/examples/basic_pubsub"
