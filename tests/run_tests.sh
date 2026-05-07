#!/bin/bash
#==================================================================================================
# Project              : YuleTech AutoSAR BSW
# Script               : Build and Run Unit Tests
# Date                 : 2026-04-27
#
# (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
# All Rights Reserved.
#
# Description: 编译并运行所有单元测试
#==================================================================================================

set -e  # 遇到错误立即退出

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=============================================="
echo "  YuleTech BSW 单元测试构建与执行"
echo "=============================================="
echo ""

# 获取脚本所在目录
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
TEST_DIR="$SCRIPT_DIR/unit"
BUILD_DIR="$SCRIPT_DIR/build"

# 创建构建目录
mkdir -p "$BUILD_DIR"

# 包含路径
INCLUDES="-I$SCRIPT_DIR/../../src/bsw/common"
INCLUDES="$INCLUDES -I$SCRIPT_DIR/../../src/bsw/mcal/mcu/include"
INCLUDES="$INCLUDES -I$SCRIPT_DIR/../../src/bsw/mcal/port/include"
INCLUDES="$INCLUDES -I$SCRIPT_DIR/../../src/bsw/mcal/can/include"
INCLUDES="$INCLUDES -I$SCRIPT_DIR/../../src/bsw/services/pdur/include"
INCLUDES="$INCLUDES -I$SCRIPT_DIR/../../src/bsw/services/com/include"
INCLUDES="$INCLUDES -I$SCRIPT_DIR/../../src/bsw/services/nvm/include"

# 编译所有测试
echo -e "${YELLOW}编译单元测试...${NC}"
cd "$TEST_DIR"

TEST_FILES=$(find . -name "test_*.c" -type f)
FAILED_TESTS=()
PASSED_TESTS=()

for test_file in $TEST_FILES; do
    test_name=$(basename "$test_file" .c)
    echo -e "\n${YELLOW}编译: $test_name${NC}"
    
    # 编译测试
    if gcc -o "$BUILD_DIR/$test_name" \
        "$test_file" \
        $INCLUDES \
        -lm 2>&1; then
        
        echo -e "${GREEN}✓ 编译成功${NC}"
        
        # 运行测试
        echo -e "${YELLOW}执行: $test_name${NC}"
        if "$BUILD_DIR/$test_name"; then
            echo -e "${GREEN}✓ 测试通过${NC}"
            PASSED_TESTS+=("$test_name")
        else
            echo -e "${RED}✗ 测试失败${NC}"
            FAILED_TESTS+=("$test_name")
        fi
    else
        echo -e "${RED}✗ 编译失败${NC}"
        FAILED_TESTS+=("$test_name (编译失败)")
    fi
done

# 打印汇总
echo ""
echo "=============================================="
echo "  测试汇总"
echo "=============================================="

echo -e "\n${GREEN}通过的测试 (${#PASSED_TESTS[@]}):${NC}"
for test in "${PASSED_TESTS[@]}"; do
    echo "  ✓ $test"
done

if [ ${#FAILED_TESTS[@]} -gt 0 ]; then
    echo -e "\n${RED}失败的测试 (${#FAILED_TESTS[@]}):${NC}"
    for test in "${FAILED_TESTS[@]}"; do
        echo "  ✗ $test"
    done
    
    echo -e "\n${RED}=============================================="
    echo "  测试结果: 失败"
    echo "==============================================${NC}"
    exit 1
else
    echo -e "\n${GREEN}=============================================="
    echo "  测试结果: 全部通过"
    echo "==============================================${NC}"
    exit 0
fi
