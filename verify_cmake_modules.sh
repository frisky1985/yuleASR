#!/bin/bash
# CMake模块配置验证脚本

echo "======================================"
echo "CMake模块配置验证"
echo "======================================"
echo ""

# 定义颜色
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

ERRORS=0

# 检查源文件存在性
check_source_files() {
    local module=$1
    local path=$2
    local files=$3
    
    echo -n "检查 $module 源文件... "
    
    for file in $files; do
        if [ ! -f "$path/$file" ]; then
            echo -e "${RED}✗${NC} 缺失: $path/$file"
            ERRORS=$((ERRORS+1))
            return 1
        fi
    done
    
    echo -e "${GREEN}✓${NC} 通过"
    return 0
}

# 检查头文件存在性
check_header_files() {
    local module=$1
    local path=$2
    local files=$3
    
    echo -n "检查 $module 头文件... "
    
    for file in $files; do
        if [ ! -f "$path/$file" ]; then
            echo -e "${RED}✗${NC} 缺失: $path/$file"
            ERRORS=$((ERRORS+1))
            return 1
        fi
    done
    
    echo -e "${GREEN}✓${NC} 通过"
    return 0
}

# 检查CMake配置中的模块引用
check_cmake_refs() {
    local file=$1
    local module=$2
    local refs=$3
    
    echo -n "检查 $file 中的 $module 引用... "
    
    for ref in $refs; do
        if ! grep -q "$ref" "$file"; then
            echo -e "${RED}✗${NC} 缺失引用: $ref"
            ERRORS=$((ERRORS+1))
            return 1
        fi
    done
    
    echo -e "${GREEN}✓${NC} 通过"
    return 0
}

echo "1. 检查源文件存在性"
echo "--------------------------------------"
check_source_files "Eth" "src/bsw/mcal/eth/src" "Eth.c Eth_Irq.c"
check_source_files "Icu" "src/bsw/mcal/icu/src" "Icu.c Icu_Irq.c Icu_Lcfg.c"
check_source_files "Ocu" "src/bsw/mcal/ocu/src" "Ocu.c Ocu_Irq.c"
check_source_files "FrTp" "src/bsw/ecual/frtp/src" "FrTp.c FrTp_Lcfg.c FrTp_PrivUtil.c FrTp_Rx.c FrTp_Tx.c FrTp_TxSm.c"
echo ""

echo "2. 检查头文件存在性"
echo "--------------------------------------"
check_header_files "Eth" "src/bsw/mcal/eth/include" "Eth.h Eth_Cfg.h Eth_Lcfg.h Eth_Private.h"
check_header_files "Icu" "src/bsw/mcal/icu/include" "Icu.h Icu_Cfg.h Icu_Lcfg.h Icu_Private.h"
check_header_files "Ocu" "src/bsw/mcal/ocu/include" "Ocu.h Ocu_Cfg.h Ocu_Lcfg.h Ocu_Private.h"
check_header_files "FrTp" "src/bsw/ecual/frtp/include" "FrTp.h FrTp_Cfg.h FrTp_Lcfg.h FrTp_Private.h"
echo ""

echo "3. 检查 tools/build/CMakeLists.txt 配置"
echo "--------------------------------------"
check_cmake_refs "tools/build/CMakeLists.txt" "Eth" "mcal/eth/include mcal/eth/src/Eth.c mcal/eth/src/Eth_Irq.c"
check_cmake_refs "tools/build/CMakeLists.txt" "Icu" "mcal/icu/include mcal/icu/src/Icu.c mcal/icu/src/Icu_Irq.c mcal/icu/src/Icu_Lcfg.c"
check_cmake_refs "tools/build/CMakeLists.txt" "Ocu" "mcal/ocu/include mcal/ocu/src/Ocu.c mcal/ocu/src/Ocu_Irq.c"
check_cmake_refs "tools/build/CMakeLists.txt" "FrTp" "ecual/frtp/include ecual/frtp/src/FrTp.c"
echo ""

echo "4. 检查 tests/CMakeLists.txt 配置"
echo "--------------------------------------"
check_cmake_refs "tests/CMakeLists.txt" "Eth" "mcal/eth/include mcal/eth/src/Eth.c"
check_cmake_refs "tests/CMakeLists.txt" "Icu" "mcal/icu/include mcal/icu/src/Icu.c"
check_cmake_refs "tests/CMakeLists.txt" "Ocu" "mcal/ocu/include mcal/ocu/src/Ocu.c"
check_cmake_refs "tests/CMakeLists.txt" "FrTp" "ecual/frtp/include ecual/frtp/src/FrTp.c"
echo ""

echo "======================================"
if [ $ERRORS -eq 0 ]; then
    echo -e "${GREEN}验证完成: 所有检查通过!${NC}"
    echo ""
    echo "模块统计:"
    echo "  - Eth: 2个源文件, 4个头文件"
    echo "  - Icu: 3个源文件, 4个头文件"
    echo "  - Ocu: 2个源文件, 4个头文件"
    echo "  - FrTp: 6个源文件, 4个头文件"
    echo "  总计: 13个源文件, 16个头文件"
    exit 0
else
    echo -e "${RED}验证失败: 发现 $ERRORS 个问题${NC}"
    exit 1
fi
