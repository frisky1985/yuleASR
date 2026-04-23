# YuleTech AutoSAR BSW - 构建配置
cmake_minimum_required(VERSION 3.20)

# 源代码目录
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src/bsw)

# ============ 通用头文件 ============
include_directories(
    ${SRC_DIR}/general/inc
    ${SRC_DIR}/mcal/mcu/include
    ${SRC_DIR}/mcal/port/include
    ${SRC_DIR}/mcal/dio/include
    ${SRC_DIR}/mcal/can/include
    ${SRC_DIR}/mcal/spi/include
    ${SRC_DIR}/mcal/gpt/include
    ${SRC_DIR}/mcal/pwm/include
    ${SRC_DIR}/mcal/adc/include
    ${SRC_DIR}/mcal/wdg/include
    ${SRC_DIR}/ecual/canif/include
    ${SRC_DIR}/ecual/cantp/include
    ${SRC_DIR}/ecual/ethif/include
    ${SRC_DIR}/ecual/frif/include
    ${SRC_DIR}/ecual/linif/include
    ${SRC_DIR}/ecual/iohwab/include
    ${SRC_DIR}/ecual/memif/include
    ${SRC_DIR}/ecual/fee/include
    ${SRC_DIR}/ecual/ea/include
    ${SRC_DIR}/services/com/include
    ${SRC_DIR}/services/pdur/include
    ${SRC_DIR}/services/nvm/include
    ${SRC_DIR}/services/dcm/include
    ${SRC_DIR}/services/dem/include
    ${SRC_DIR}/os/include
    ${SRC_DIR}/rte/include
)

# ============ 源文件列表 ============
set(BSW_SOURCES
    # MCAL 层
    ${SRC_DIR}/mcal/mcu/src/Mcu.c
    ${SRC_DIR}/mcal/port/src/Port.c
    ${SRC_DIR}/mcal/dio/src/Dio.c
    ${SRC_DIR}/mcal/can/src/Can.c
    ${SRC_DIR}/mcal/spi/src/Spi.c
    ${SRC_DIR}/mcal/gpt/src/Gpt.c
    ${SRC_DIR}/mcal/pwm/src/Pwm.c
    ${SRC_DIR}/mcal/adc/src/Adc.c
    ${SRC_DIR}/mcal/wdg/src/Wdg.c
    
    # ECUAL 层
    ${SRC_DIR}/ecual/canif/src/CanIf.c
    ${SRC_DIR}/ecual/cantp/src/CanTp.c
    ${SRC_DIR}/ecual/ethif/src/EthIf.c
    ${SRC_DIR}/ecual/frif/src/FrIf.c
    ${SRC_DIR}/ecual/linif/src/LinIf.c
    ${SRC_DIR}/ecual/iohwab/src/IoHwAb.c
    ${SRC_DIR}/ecual/memif/src/MemIf.c
    ${SRC_DIR}/ecual/fee/src/Fee.c
    ${SRC_DIR}/ecual/ea/src/Ea.c
    
    # Service 层
    ${SRC_DIR}/services/com/src/Com.c
    ${SRC_DIR}/services/pdur/src/PduR.c
    ${SRC_DIR}/services/nvm/src/NvM.c
    ${SRC_DIR}/services/dcm/src/Dcm.c
    ${SRC_DIR}/services/dem/src/Dem.c
    
    # OS 层
    ${SRC_DIR}/os/src/Os.c
    
    # RTE 层
    ${SRC_DIR}/rte/src/Rte.c
)

# ============ 库目标 ============
add_library(yule_bsw STATIC ${BSW_SOURCES})

target_compile_options(yule_bsw PRIVATE
    -Wall
    -Wextra
    -O2
)

# ============ 测试目标 ============
if(BUILD_TESTS)
    # 测试配置在 tests/CMakeLists.txt 中
    message(STATUS "Tests will be built from tests/CMakeLists.txt")
endif()

# ============ 打印信息 ============
message(STATUS "BSW Sources: ${BSW_SOURCES}")
