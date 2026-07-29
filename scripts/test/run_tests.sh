#!/bin/bash
#
# yuleASR 测试运行脚本
# 支持单元测试、集成测试、覆盖率分析
#

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 默认配置
TEST_TYPE="all"
BUILD_DIR="build"
COVERAGE=OFF
VERBOSE=OFF
JOBS=$(nproc 2>/dev/null || echo 4)

print_help() {
    echo -e "${BLUE}yuleASR 测试运行脚本${NC}"
    echo ""
    echo "用法: $0 [OPTIONS]"
    echo ""
    echo "选项:"
    echo "  -t, --type <type>    测试类型 (unit|integration|system|all, 默认: all)"
    echo "  -d, --dir <dir>      构建目录 (默认: build)"
    echo "  -c, --coverage       生成覆盖率报告"
    echo "  -v, --verbose        详细输出"
    echo "  -j, --jobs <n>       并行任务数"
    echo "  -h, --help           显示帮助"
    echo ""
    echo "示例:"
    echo "  $0                        # 运行所有测试"
    echo "  $0 -t unit                # 只运行单元测试"
    echo "  $0 -t unit -c             # 运行单元测试并生成覆盖率报告"
}

# 解析参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--type)
            TEST_TYPE="$2"
            shift 2
            ;;
        -d|--dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -c|--coverage)
            COVERAGE=ON
            shift
            ;;
        -v|--verbose)
            VERBOSE=ON
            shift
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        -h|--help)
            print_help
            exit 0
            ;;
        *)
            echo -e "${RED}错误: 未知参数 $1${NC}"
            print_help
            exit 1
            ;;
    esac
done

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  yuleASR 测试系统${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# 检查构建目录
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}错误: 构建目录不存在: $BUILD_DIR${NC}"
    echo "请先运行: ./scripts/build/build_all.sh"
    exit 1
fi

cd "$BUILD_DIR"

# 运行测试
run_tests() {
    local test_pattern="$1"
    local test_name="$2"
    
    echo -e "${YELLOW}运行${test_name}...${NC}"
    
    if [ "$VERBOSE" = "ON" ]; then
        ctest -R "$test_pattern" --output-on-failure -j"$JOBS" -V
    else
        ctest -R "$test_pattern" --output-on-failure -j"$JOBS"
    fi
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}${test_name} 通过${NC}"
    else
        echo -e "${RED}${test_name} 失败${NC}"
        return 1
    fi
}

# 根据类型运行测试
case $TEST_TYPE in
    unit)
        run_tests "unit_" "单元测试"
        ;;
    integration)
        run_tests "integration_" "集成测试"
        ;;
    system)
        run_tests "system_" "系统测试"
        ;;
    all)
        echo -e "${YELLOW}运行所有测试...${NC}"
        ctest --output-on-failure -j"$JOBS"
        ;;
    *)
        echo -e "${RED}错误: 未知测试类型: $TEST_TYPE${NC}"
        print_help
        exit 1
        ;;
esac

# 生成覆盖率报告
if [ "$COVERAGE" = "ON" ]; then
    echo -e "${YELLOW}生成覆盖率报告...${NC}"
    
    # 检查 lcov
    if ! command -v lcov &> /dev/null; then
        echo -e "${YELLOW}警告: lcov 未安装，无法生成覆盖率报告${NC}"
    else
        mkdir -p coverage
        lcov --capture --directory . --output-file coverage/coverage.info
        lcov --remove coverage/coverage.info '/usr/*' '*/third_party/*' '*/tests/*' --output-file coverage/coverage_filtered.info
        genhtml coverage/coverage_filtered.info --output-directory coverage/report
        echo -e "${GREEN}覆盖率报告已生成: coverage/report/index.html${NC}"
    fi
fi

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  测试完成!${NC}"
echo -e "${GREEN}========================================${NC}"
