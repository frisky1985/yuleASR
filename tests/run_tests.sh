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
#              支持 --coverage 模式：编译时加入覆盖率选项，运行测试后生成 gcov 行级覆盖率报告
#==================================================================================================

set -e  # 遇到错误立即退出

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 默认值
COVERAGE_MODE=0
COVERAGE_DIR=""
GENERATE_GCOVR=1

# 解析参数
while [[ $# -gt 0 ]]; do
    case $1 in
        --coverage)
            COVERAGE_MODE=1
            shift
            ;;
        --no-gcovr)
            GENERATE_GCOVR=0
            shift
            ;;
        -h|--help)
            echo "用法: $0 [--coverage] [--no-gcovr]"
            echo ""
            echo "选项:"
            echo "  --coverage    启用代码覆盖率编译与 gcov/gcovr 报告生成"
            echo "  --no-gcovr    启用覆盖率但跳过 gcovr HTML/JSON 报告（仅运行 gcov）"
            echo "  -h, --help    显示此帮助信息"
            echo ""
            echo "示例:"
            echo "  $0                      普通模式构建并运行测试"
            echo "  $0 --coverage           覆盖率模式：编译、测试、生成 gcov/gcovr 报告"
            echo "  $0 --coverage --no-gcovr 仅生成 .gcov 文件，跳过 gcovr HTML 报告"
            exit 0
            ;;
        *)
            echo -e "${RED}未知选项: $1${NC}"
            echo "用法: $0 [--coverage] [--no-gcovr]"
            exit 1
            ;;
    esac
done

echo "=============================================="
echo "  YuleTech BSW 单元测试构建与执行"
if [ $COVERAGE_MODE -eq 1 ]; then
    echo "  模式: 覆盖率 (COVERAGE)"
fi
echo "=============================================="
echo ""

# 获取脚本所在目录
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
TEST_DIR="$SCRIPT_DIR/unit"
BUILD_DIR="$SCRIPT_DIR/build"

# 覆盖率输出目录
COVERAGE_DIR="$SCRIPT_DIR/coverage_reports"

# 创建构建目录
mkdir -p "$BUILD_DIR"
if [ $COVERAGE_MODE -eq 1 ]; then
    mkdir -p "$COVERAGE_DIR"
fi

# 包含路径
INCLUDES="-I$SCRIPT_DIR/../../src/bsw/common"
INCLUDES="$INCLUDES -I$SCRIPT_DIR/../../src/bsw/mcal/mcu/include"
INCLUDES="$INCLUDES -I$SCRIPT_DIR/../../src/bsw/mcal/port/include"
INCLUDES="$INCLUDES -I$SCRIPT_DIR/../../src/bsw/mcal/can/include"
INCLUDES="$INCLUDES -I$SCRIPT_DIR/../../src/bsw/services/pdur/include"
INCLUDES="$INCLUDES -I$SCRIPT_DIR/../../src/bsw/services/com/include"
INCLUDES="$INCLUDES -I$SCRIPT_DIR/../../src/bsw/services/nvm/include"

# 编译选项
COMPILE_OPTS=""
if [ $COVERAGE_MODE -eq 1 ]; then
    COMPILE_OPTS="--coverage"
    echo -e "${YELLOW}[覆盖率] 编译选项: $COMPILE_OPTS${NC}"
fi

# 编译所有测试
echo -e "${YELLOW}编译单元测试...${NC}"
cd "$TEST_DIR"

TEST_FILES=$(find . -name "test_*.c" -type f 2>/dev/null || true)
if [ -z "$TEST_FILES" ]; then
    echo -e "${YELLOW}未找到 test_*.c 文件，尝试在子目录中查找...${NC}"
    TEST_FILES=$(find . -name "*_test.c" -type f -not -path "*/build/*" 2>/dev/null || true)
fi

FAILED_TESTS=()
PASSED_TESTS=()

for test_file in $TEST_FILES; do
    test_name=$(basename "$test_file" .c)
    echo -e "\n${YELLOW}编译: $test_name${NC}"
    
    # 编译测试
    if gcc -o "$BUILD_DIR/$test_name" \
        "$test_file" \
        $INCLUDES \
        $COMPILE_OPTS \
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
fi

# ============================================================
# 覆盖率报告生成
# ============================================================
if [ $COVERAGE_MODE -eq 1 ]; then
    echo ""
    echo "=============================================="
    echo -e "${YELLOW}  生成覆盖率报告...${NC}"
    echo "=============================================="
    
    # 1. 运行 gcov 生成 .gcov 文件
    echo -e "\n${YELLOW}[1/3] 运行 gcov 生成行级覆盖率...${NC}"
    
    # 收集所有 .gcda 文件并运行 gcov
    GCDA_FILES=$(find "$BUILD_DIR" -name "*.gcda" -type f 2>/dev/null || true)
    if [ -z "$GCDA_FILES" ]; then
        echo -e "${YELLOW}  未找到 .gcda 文件，gcov 可能尚未生成覆盖率数据${NC}"
        echo -e "${YELLOW}  尝试回退：对 build 目录直接运行 gcov...${NC}"
    fi
    
    # 进入 build 目录对每个 .gcda 运行 gcov
    cd "$BUILD_DIR"
    GCOV_COUNT=0
    for gcda_file in $(find . -name "*.gcda" -type f 2>/dev/null || true); do
        gcda_dir=$(dirname "$gcda_file")
        echo "  处理: $gcda_file"
        gcov "$gcda_file" --relative-only 2>/dev/null || gcov "$gcda_file" 2>/dev/null || true
        GCOV_COUNT=$((GCOV_COUNT + 1))
    done
    
    # 将 .gcov 文件拷贝到报告目录
    find . -name "*.gcov" -type f 2>/dev/null | while read gcov_file; do
        cp "$gcov_file" "$COVERAGE_DIR/"
    done
    
    echo -e "${GREEN}  gcov 处理完成, 处理了 $GCOV_COUNT 个 .gcda 文件${NC}"
    GCOV_OUTPUT_COUNT=$(ls "$COVERAGE_DIR"/*.gcov 2>/dev/null | wc -l | tr -d ' ')
    echo -e "${GREEN}  生成了 $GCOV_OUTPUT_COUNT 个 .gcov 行级覆盖率文件${NC}"
    
    # 2. 使用 gcovr 生成汇总报告（如果可用）
    if [ $GENERATE_GCOVR -eq 1 ]; then
        echo -e "\n${YELLOW}[2/3] 尝试使用 gcovr 生成汇总报告...${NC}"
        
        if command -v gcovr &> /dev/null; then
            GCOVR_PATH=$(command -v gcovr)
            echo -e "${GREEN}  gcovr 可用: $GCOVR_PATH${NC}"
            
            # 切换到项目根目录
            cd "$SCRIPT_DIR/../.."
            
            # HTML 报告
            echo "  生成 HTML 报告..."
            mkdir -p "$COVERAGE_DIR/html"
            gcovr --root . \
                  --filter "src/.*" \
                  --exclude "tests/.*" \
                  --exclude "third_party/.*" \
                  --exclude-unreachable-branches \
                  --html --html-details \
                  --output "$COVERAGE_DIR/html/index.html" \
                  "$BUILD_DIR" 2>&1 || echo -e "${YELLOW}  gcovr HTML 报告生成跳过${NC}"
            
            # JSON 报告
            echo "  生成 JSON 报告..."
            gcovr --root . \
                  --filter "src/.*" \
                  --exclude "tests/.*" \
                  --exclude "third_party/.*" \
                  --json \
                  --output "$COVERAGE_DIR/coverage.json" \
                  "$BUILD_DIR" 2>&1 || echo -e "${YELLOW}  gcovr JSON 报告生成跳过${NC}"
            
            # 文本摘要
            echo "  生成文本摘要..."
            gcovr --root . \
                  --filter "src/.*" \
                  --exclude "tests/.*" \
                  --exclude "third_party/.*" \
                  "$BUILD_DIR" 2>&1 | tee "$COVERAGE_DIR/coverage_summary.txt" || true
            
            echo -e "${GREEN}  gcovr 报告生成完成${NC}"
        else
            echo -e "${YELLOW}  gcovr 不可用, 跳过汇总报告${NC}"
            echo -e "${YELLOW}  安装: pip install gcovr${NC}"
            GENERATE_GCOVR=0
        fi
    fi
    
    # 3. 生成摘要信息
    echo -e "\n${YELLOW}[3/3] 覆盖率摘要...${NC}"
    SUMMARY_FILE="$COVERAGE_DIR/coverage_summary.txt"
    if [ -f "$SUMMARY_FILE" ]; then
        echo ""
        echo "=============================================="
        echo "  行级覆盖率摘要"
        echo "=============================================="
        cat "$SUMMARY_FILE"
    fi
    
    echo ""
    echo -e "${GREEN}=============================================="
    echo "  覆盖率报告生成完成"
    echo "==============================================${NC}"
    echo "  .gcov 文件:      $COVERAGE_DIR/*.gcov"
    if [ $GENERATE_GCOVR -eq 1 ] && command -v gcovr &> /dev/null; then
        echo "  HTML 报告:       $COVERAGE_DIR/html/index.html"
        echo "  JSON 报告:       $COVERAGE_DIR/coverage.json"
    fi
    echo "=============================================="
    
    # 回到脚本目录
    cd "$SCRIPT_DIR"
fi

echo ""
echo -e "${GREEN}=============================================="
echo "  测试结果: 全部通过"
echo "==============================================${NC}"
exit 0
