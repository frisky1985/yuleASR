# S32K312 AUTOSAR BSW 构建配置
# 支持 GCC ARM Embedded 工具链

cmake_minimum_required(VERSION 3.16)
project(S32K312_AUTOSAR LANGUAGES C ASM)

#===================================================================================================
# 工具链配置
#===================================================================================================
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# 查找ARM GCC工具链
if(NOT DEFINED ENV{ARMGCC_DIR})
    message(WARNING "ARMGCC_DIR 环境变量未设置，使用默认路径")
    set(ARMGCC_DIR "/usr/local/gcc-arm-none-eabi")
else()
    set(ARMGCC_DIR $ENV{ARMGCC_DIR})
endif()

set(CMAKE_C_COMPILER "${ARMGCC_DIR}/bin/arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "${ARMGCC_DIR}/bin/arm-none-eabi-g++")
set(CMAKE_ASM_COMPILER "${ARMGCC_DIR}/bin/arm-none-eabi-gcc")
set(CMAKE_OBJCOPY "${ARMGCC_DIR}/bin/arm-none-eabi-objcopy")
set(CMAKE_SIZE "${ARMGCC_DIR}/bin/arm-none-eabi-size")
set(CMAKE_OBJDUMP "${ARMGCC_DIR}/bin/arm-none-eabi-objdump")

#===================================================================================================
# 处理器特性
#===================================================================================================
set(CPU_FLAGS 
    -mcpu=cortex-m7
    -mthumb
    -mfpu=fpv5-d16
    -mfloat-abi=hard
)

#===================================================================================================
# 编译选项
#===================================================================================================
set(COMMON_FLAGS
    ${CPU_FLAGS}
    -Wall
    -Wextra
    -Wshadow
    -Wdouble-promotion
    -Wformat=2
    -Wundef
    -fno-common
    -fdata-sections
    -ffunction-sections
    -ffast-math
    -fshort-enums
    -fomit-frame-pointer
    -ffreestanding
)

set(CMAKE_C_FLAGS_DEBUG "-O0 -g3 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -DRELEASE")

#===================================================================================================
# 定义目标
#===================================================================================================
set(S32K312_DEFINES
    -DS32K312
    -DS32K3XX
    -DCPU_S32K312
    -DDEBUG
    -DRELEASE
    -DAUTOSAR_OS_NOT_USED
)

#===================================================================================================
# 包含目录
#===================================================================================================
include_directories(
    # MCAL头文件
    ${CMAKE_SOURCE_DIR}/mcal/include
    
    # BSW头文件
    ${CMAKE_SOURCE_DIR}/../src/bsw/services/dem/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/services/dcm/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/services/dlt/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/common
    
    # RTE头文件
    ${CMAKE_SOURCE_DIR}/../rte/include
    
    # ASW头文件
    ${CMAKE_SOURCE_DIR}/../src/application/include
)

#===================================================================================================
# 源文件
#===================================================================================================
file(GLOB_RECURSE MCAL_SOURCES 
    "mcal/src/*.c"
)

file(GLOB_RECURSE BSW_SOURCES
    "../src/bsw/services/dem/src/*.c"
    "../src/bsw/services/dcm/src/*.c"
    "../src/bsw/services/dlt/src/*.c"
)

file(GLOB_RECURSE APP_SOURCES
    "../src/application/*.c"
    "startup/*.c"
    "startup/*.S"
)

#===================================================================================================
# 构建目标
#===================================================================================================
add_executable(${PROJECT_NAME}.elf
    ${MCAL_SOURCES}
    ${BSW_SOURCES}
    ${APP_SOURCES}
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
    ${COMMON_FLAGS}
    ${S32K312_DEFINES}
)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    ${CPU_FLAGS}
    -T${CMAKE_SOURCE_DIR}/linker/S32K312_flash.ld
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -specs=nano.specs
    -specs=nosys.specs
    -lm
    -lc
)

#===================================================================================================
# 生成Hex和Bin文件
#===================================================================================================
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
    COMMENT "生成 HEX 和 BIN 文件"
)

#===================================================================================================
# 清理目标
#===================================================================================================
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
    COMMENT "清除所有构建文件"
)

#===================================================================================================
# 测试目标
#===================================================================================================
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
