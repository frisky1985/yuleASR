# YuleTech AutoSAR BSW - 构建配置
cmake_minimum_required(VERSION 3.20)
project(yule_bsw C CXX)

# 项目根目录 (CMakeLists.txt 位于 tools/build 下，向上两级才是项目根)
set(PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)

# 源代码目录
set(SRC_DIR ${PROJECT_ROOT}/src/bsw)

# ============ 通用头文件 ============
include_directories(
    ${PROJECT_ROOT}/config/generated
    ${PROJECT_ROOT}/config/input/mcal
    ${PROJECT_ROOT}/include/autosar
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
    ${SRC_DIR}/mcal/eth/include
    ${SRC_DIR}/mcal/icu/include
    ${SRC_DIR}/mcal/ocu/include
    ${SRC_DIR}/ecual/canif/include
    ${SRC_DIR}/ecual/frtp/include
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
    ${SRC_DIR}/mcal/eth/src/Eth.c
    ${SRC_DIR}/mcal/eth/src/Eth_Irq.c
    ${SRC_DIR}/mcal/icu/src/Icu.c
    ${SRC_DIR}/mcal/icu/src/Icu_Irq.c
    ${SRC_DIR}/mcal/icu/src/Icu_Lcfg.c
    ${SRC_DIR}/mcal/ocu/src/Ocu.c
    ${SRC_DIR}/mcal/ocu/src/Ocu_Irq.c
    
    # ECUAL 层
    ${SRC_DIR}/ecual/canif/src/CanIf.c
    ${SRC_DIR}/ecual/frtp/src/FrTp.c
    ${SRC_DIR}/ecual/frtp/src/FrTp_Lcfg.c
    ${SRC_DIR}/ecual/frtp/src/FrTp_PrivUtil.c
    ${SRC_DIR}/ecual/frtp/src/FrTp_Rx.c
    ${SRC_DIR}/ecual/frtp/src/FrTp_Tx.c
    ${SRC_DIR}/ecual/frtp/src/FrTp_TxSm.c
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
    # ${SRC_DIR}/rte/src/Rte.c  # 暂不存在，后续添加
)

# ============ 库目标 ============
add_library(yule_bsw STATIC ${BSW_SOURCES})

target_compile_options(yule_bsw PRIVATE
    -Wall
    -Wextra
    -O2
)

# ============ Configurator ECUC 生成文件 ============
set(GENERATED_DIR ${PROJECT_ROOT}/config/generated)
set(ECUC_GENERATED_SOURCES
    ${GENERATED_DIR}/Ecuc_Can.c
    ${GENERATED_DIR}/Ecuc_Can_PBcfg.c
    ${GENERATED_DIR}/Ecuc_Can_Lcfg.c
    ${GENERATED_DIR}/Ecuc_Mcu.c
    ${GENERATED_DIR}/Ecuc_Mcu_PBcfg.c
    ${GENERATED_DIR}/Ecuc_Mcu_Lcfg.c
    ${GENERATED_DIR}/Ecuc_Port.c
    ${GENERATED_DIR}/Ecuc_Port_PBcfg.c
    ${GENERATED_DIR}/Ecuc_Port_Lcfg.c
)

option(ENABLE_ECUC_GENERATED "Build ECUC generated configuration files" OFF)

if(ENABLE_ECUC_GENERATED)
    add_library(yule_ecuc STATIC ${ECUC_GENERATED_SOURCES})
    target_compile_options(yule_ecuc PRIVATE -Wall -Wextra)
    message(STATUS "ECUC generated sources included: ${ECUC_GENERATED_SOURCES}")
endif()

# ============ 测试目标 ============
if(BUILD_TESTS)
    # 测试配置在 tests/CMakeLists.txt 中
    message(STATUS "Tests will be built from tests/CMakeLists.txt")
endif()

# ============ 打印信息 ============
message(STATUS "BSW Sources: ${BSW_SOURCES}")
