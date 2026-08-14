#!/bin/bash
# YuleTech AutoSAR SDK Export Script
# Usage: ./export_sdk.sh [output_dir] [build_dir]
#
# 功能: 将 yuleASR 构建产物打包成可交付的 SDK:
#   - 收集全部静态库 (*.a) → <sdk>/lib/
#   - 收集全部头文件 (*.h) → <sdk>/include/yuletech/
#   - 生成 YuleTechAutoSARConfig.cmake (find_package 支持 + 模块 target 导出)
#   - 复制 ConfigVersion.cmake
#
# 客户集成:
#   cmake -DCMAKE_PREFIX_PATH=<sdk> ..
#   或 set(YuleTechAutoSAR_DIR <sdk>/cmake) 后 find_package(YuleTechAutoSAR REQUIRED)
#   target_link_libraries(app PRIVATE YuleTechAutoSAR::service_com ...)

set -e

# 颜色
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

OUTPUT_DIR="${1:-${REPO_ROOT}/output/sdk}"
BUILD_DIR="${2:-${REPO_ROOT}/build}"

# 版本
PROJECT_VERSION="1.5.0"

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}YuleTech AutoSAR SDK Export${NC}"
echo -e "${GREEN}========================================${NC}"
echo "SDK output:   ${OUTPUT_DIR}"
echo "Build dir:    ${BUILD_DIR}"
echo "Version:      ${PROJECT_VERSION}"
echo -e "${GREEN}========================================${NC}"

# 检查构建目录
if [ ! -d "${BUILD_DIR}/lib" ]; then
    echo -e "${RED}Error: build directory not found or empty: ${BUILD_DIR}${NC}"
    echo "Run ./scripts/build/build.sh first, or pass build dir as 2nd argument."
    exit 1
fi

# 清理并重建 SDK 目录
rm -rf "${OUTPUT_DIR}"
mkdir -p "${OUTPUT_DIR}/lib"
mkdir -p "${OUTPUT_DIR}/include/yuletech"
mkdir -p "${OUTPUT_DIR}/cmake"

# 1. 收集静态库 (保持相对结构)
echo -e "${YELLOW}[1/4] 收集静态库...${NC}"
find "${BUILD_DIR}/lib" -name "*.a" | while read -r lib; do
    rel="${lib#"${BUILD_DIR}/lib/"}"
    mkdir -p "${OUTPUT_DIR}/lib/$(dirname "${rel}")"
    cp "${lib}" "${OUTPUT_DIR}/lib/${rel}"
done
LIB_COUNT=$(find "${OUTPUT_DIR}/lib" -name "*.a" | wc -l | tr -d ' ')
echo "  已收集 ${LIB_COUNT} 个静态库"

# 2. 收集头文件 (保持相对结构)
echo -e "${YELLOW}[2/4] 收集头文件...${NC}"
# src/ 下的头文件
find "${REPO_ROOT}/src" -name "*.h" | while read -r h; do
    rel="${h#"${REPO_ROOT}/src/"}"
    mkdir -p "${OUTPUT_DIR}/include/yuletech/$(dirname "${rel}")"
    cp "${h}" "${OUTPUT_DIR}/include/yuletech/${rel}"
done
# include/ 下的头文件
if [ -d "${REPO_ROOT}/include" ]; then
    find "${REPO_ROOT}/include" -name "*.h" | while read -r h; do
        rel="${h#"${REPO_ROOT}/include/"}"
        mkdir -p "${OUTPUT_DIR}/include/yuletech/$(dirname "${rel}")"
        cp "${h}" "${OUTPUT_DIR}/include/yuletech/${rel}"
    done
fi
HDR_COUNT=$(find "${OUTPUT_DIR}/include" -name "*.h" | wc -l | tr -d ' ')
echo "  已收集 ${HDR_COUNT} 个头文件"

# 3. 生成 YuleTechAutoSARConfig.cmake
echo -e "${YELLOW}[3/4] 生成 YuleTechAutoSARConfig.cmake...${NC}"
CONFIG_FILE="${OUTPUT_DIR}/cmake/YuleTechAutoSARConfig.cmake"

# 收集所有库的相对路径
LIBS=$(find "${OUTPUT_DIR}/lib" -name "*.a" | sed "s|${OUTPUT_DIR}/lib/||" | sort)

{
    cat << 'EOF'
# YuleTechAutoSARConfig.cmake
# 供客户工程 find_package(YuleTechAutoSAR REQUIRED) 使用。
# 用法:
#   set(YuleTechAutoSAR_DIR <sdk>/cmake)
#   find_package(YuleTechAutoSAR REQUIRED)
#   target_link_libraries(app PRIVATE YuleTechAutoSAR::service_com ...)

get_filename_component(_yuleasr_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(YuleTechAutoSAR_INCLUDE_DIR "${_yuleasr_root}/include")
set(YuleTechAutoSAR_LIBRARY_DIR "${_yuleasr_root}/lib")
set(YuleTechAutoSAR_VERSION "1.5.0")

# 聚合 include 目录: 根 include + 全部模块 include/ 子目录 (模块头文件分散在各层)
set(YuleTechAutoSAR_INCLUDE_DIRS
    "${YuleTechAutoSAR_INCLUDE_DIR}"
)
file(GLOB_RECURSE _yuleasr_inc_dirs LIST_DIRECTORIES true
    "${YuleTechAutoSAR_INCLUDE_DIR}/*/include")
foreach(_inc_dir IN LISTS _yuleasr_inc_dirs)
    if(IS_DIRECTORY "${_inc_dir}")
        list(APPEND YuleTechAutoSAR_INCLUDE_DIRS "${_inc_dir}")
    endif()
endforeach()

# 遍历 lib 下所有静态库, 生成 imported targets: YuleTechAutoSAR::<name>
file(GLOB_RECURSE _yuleasr_libs "${YuleTechAutoSAR_LIBRARY_DIR}/*.a")

foreach(_lib_path IN LISTS _yuleasr_libs)
    get_filename_component(_lib_name "${_lib_path}" NAME_WE)   # libservice_com
    string(REGEX REPLACE "^lib" "" _lib_name "${_lib_name}")   # service_com
    if(NOT TARGET YuleTechAutoSAR::${_lib_name})
        add_library(YuleTechAutoSAR::${_lib_name} STATIC IMPORTED)
        set_target_properties(YuleTechAutoSAR::${_lib_name} PROPERTIES
            IMPORTED_LOCATION "${_lib_path}"
            INTERFACE_INCLUDE_DIRECTORIES "${YuleTechAutoSAR_INCLUDE_DIRS}"
        )
    endif()
endforeach()

# 聚合 target: 链接全部 yuleASR 库
if(NOT TARGET YuleTechAutoSAR::ALL)
    add_library(YuleTechAutoSAR::ALL INTERFACE IMPORTED)
    set_target_properties(YuleTechAutoSAR::ALL PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${YuleTechAutoSAR_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES ""
    )
    foreach(_lib_path IN LISTS _yuleasr_libs)
        get_filename_component(_lib_name "${_lib_path}" NAME_WE)
        string(REGEX REPLACE "^lib" "" _lib_name "${_lib_name}")
        set_property(TARGET YuleTechAutoSAR::ALL APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES "YuleTechAutoSAR::${_lib_name}")
    endforeach()
endif()

unset(_yuleasr_root)
unset(_yuleasr_libs)
unset(_lib_path)
unset(_lib_name)
EOF
} > "${CONFIG_FILE}"
echo "  ✅ ${CONFIG_FILE}"

# 4. 复制 ConfigVersion.cmake
echo -e "${YELLOW}[4/4] 复制 ConfigVersion.cmake...${NC}"
if [ -f "${BUILD_DIR}/YuleTechAutoSARConfigVersion.cmake" ]; then
    cp "${BUILD_DIR}/YuleTechAutoSARConfigVersion.cmake" "${OUTPUT_DIR}/cmake/"
    echo "  ✅ 从 build 复制"
else
    # 生成一个最小版本文件
    cat > "${OUTPUT_DIR}/cmake/YuleTechAutoSARConfigVersion.cmake" << EOF
set(PACKAGE_VERSION "${PROJECT_VERSION}")
if(PACKAGE_FIND_VERSION VERSION_GREATER PACKAGE_VERSION)
    set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
    if(PACKAGE_FIND_VERSION VERSION_EQUAL PACKAGE_VERSION)
        set(PACKAGE_VERSION_EXACT TRUE)
    endif()
endif()
EOF
    echo "  ✅ 生成最小版本文件"
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}SDK 导出完成!${NC}"
echo -e "${GREEN}========================================${NC}"
echo "SDK 目录:     ${OUTPUT_DIR}"
echo "静态库:       ${LIB_COUNT} 个"
echo "头文件:       ${HDR_COUNT} 个"
echo ""
echo "客户集成示例:"
echo "  set(YuleTechAutoSAR_DIR ${OUTPUT_DIR}/cmake)"
echo "  find_package(YuleTechAutoSAR REQUIRED)"
echo "  target_link_libraries(app PRIVATE YuleTechAutoSAR::service_com YuleTechAutoSAR::mcal_mcu)"
