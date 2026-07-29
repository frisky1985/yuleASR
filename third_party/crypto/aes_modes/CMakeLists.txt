########################################################################################################################
# CMakeLists.txt - AES算法模式库构建配置
#
# 功能: 构建AES模式库，支持所有常用AES加密模式
#       - ECB, CBC, CFB, OFB, CTR, GCM, CCM
#       AUTOSAR Crypto Driver适配层
#
# @author     YuleTech AutoSAR Team
# @version    1.0.0
# @date       2026-05-01
########################################################################################################################

cmake_minimum_required(VERSION 3.10)

########################################################################################################################
# 项目配置
########################################################################################################################
project(aes_modes
    VERSION 1.0.0
    DESCRIPTION "AES算法模式库 - YuleTech AutoSAR项目"
    LANGUAGES C
)

########################################################################################################################
# 编译选项
########################################################################################################################
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

########################################################################################################################
# 编译标志
########################################################################################################################
set(AES_MODES_C_FLAGS
    -Wall
    -Wextra
    -Werror
    -Wpedantic
    -Os                    # 优化大小 (嵌入式系统首选)
    -g                     # 调试信息
    -fdata-sections
    -ffunction-sections
    -fomit-frame-pointer
    -fno-strict-aliasing
    -fshort-enums
)

# ARM Cortex-M7特定优化 (针对S32K312)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "cortex-m")
    list(APPEND AES_MODES_C_FLAGS
        -mcpu=cortex-m7
        -mthumb
        -mfpu=fpv5-sp-d16
        -mfloat-abi=hard
    )
endif()

########################################################################################################################
# 源文件列表
########################################################################################################################
set(AES_MODES_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/aes_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/aes_ecb.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/aes_cbc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/aes_cfb.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/aes_ofb.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/aes_ctr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/aes_gcm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/aes_ccm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/aes_autosar.c
)

set(AES_MODES_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/aes_modes.h
)

########################################################################################################################
# 创建静态库
########################################################################################################################
add_library(aes_modes STATIC ${AES_MODES_SOURCES})

# 设置包含目录
target_include_directories(aes_modes
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar/classic/crypto
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar/classic/common
        ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/crypto/include
)

# 设置编译选项
target_compile_options(aes_modes PRIVATE ${AES_MODES_C_FLAGS})

# 预处理器定义
target_compile_definitions(aes_modes
    PUBLIC
        AES_MODES_SUPPORTED=1
    PRIVATE
        $<$<CONFIG:Debug>:AES_MODES_DEBUG>
        $<$<CONFIG:Release>:AES_MODES_RELEASE>
)

########################################################################################################################
# 可配置选项
########################################################################################################################
option(AES_MODES_ENABLE_HSM "启用HSM硬件加速" ON)
option(AES_MODES_ENABLE_TESTS "启用测试" ON)
option(AES_MODES_ENABLE_BENCHMARK "启用性能测试" OFF)

if(AES_MODES_ENABLE_HSM)
    target_compile_definitions(aes_modes PUBLIC AES_MODES_HSM_ENABLED=1)
    target_link_libraries(aes_modes PUBLIC crypto_s32k312_hsm)
endif()

########################################################################################################################
# 链接依赖
########################################################################################################################
target_link_libraries(aes_modes
    PUBLIC
        autosar_crypto_stack
        std_types
)

########################################################################################################################
# 安装
########################################################################################################################
install(TARGETS aes_modes
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${AES_MODES_HEADERS}
    DESTINATION include/aes_modes
)

########################################################################################################################
# 测试配置
########################################################################################################################
if(AES_MODES_ENABLE_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    enable_testing()
    add_subdirectory(tests)
endif()

########################################################################################################################
# 输出信息
########################################################################################################################
message(STATUS "AES Modes Library Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Sources: ${AES_MODES_SOURCES}")
message(STATUS "  HSM Enabled: ${AES_MODES_ENABLE_HSM}")
message(STATUS "  Tests Enabled: ${AES_MODES_ENABLE_TESTS}")
