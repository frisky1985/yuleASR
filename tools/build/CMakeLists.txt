cmake_minimum_required(VERSION 3.20)

# Project
project(YuleTech_BSW VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler options
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/../../src/bsw/common
    ${CMAKE_SOURCE_DIR}/../../src/bsw/mcal/mcu/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/mcal/port/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/mcal/dio/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/mcal/can/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/mcal/spi/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/mcal/gpt/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/mcal/pwm/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/mcal/adc/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/mcal/wdg/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/ecual/canif/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/ecual/iohwab/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/ecual/cantp/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/ecual/ethif/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/ecual/memif/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/ecual/fee/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/ecual/ea/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/ecual/frif/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/ecual/linif/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/services/com/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/services/pdur/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/services/nvm/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/services/dcm/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/services/dem/include
    ${CMAKE_SOURCE_DIR}/../../src/bsw/rte/include
)

# Source files
set(BSW_SOURCES
    # MCAL
    ${CMAKE_SOURCE_DIR}/../../src/bsw/mcal/mcu/src/Mcu.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/mcal/port/src/Port.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/mcal/dio/src/Dio.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/mcal/can/src/Can.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/mcal/spi/src/Spi.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/mcal/gpt/src/Gpt.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/mcal/pwm/src/Pwm.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/mcal/adc/src/Adc.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/mcal/wdg/src/Wdg.c
    
    # ECUAL
    ${CMAKE_SOURCE_DIR}/../../src/bsw/ecual/canif/src/CanIf.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/ecual/iohwab/src/IoHwAb.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/ecual/cantp/src/CanTp.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/ecual/ethif/src/EthIf.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/ecual/memif/src/MemIf.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/ecual/fee/src/Fee.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/ecual/ea/src/Ea.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/ecual/frif/src/FrIf.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/ecual/linif/src/LinIf.c
    
    # Services
    ${CMAKE_SOURCE_DIR}/../../src/bsw/services/com/src/Com.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/services/pdur/src/PduR.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/services/nvm/src/NvM.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/services/dcm/src/Dcm.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/services/dem/src/Dem.c
    
    # RTE
    ${CMAKE_SOURCE_DIR}/../../src/bsw/rte/src/Rte.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/rte/src/Rte_ComInterface.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/rte/src/Rte_NvMInterface.c
    ${CMAKE_SOURCE_DIR}/../../src/bsw/rte/src/Rte_Scheduler.c
)

# Create library
add_library(bsw STATIC ${BSW_SOURCES})

# Target properties
target_compile_definitions(bsw PRIVATE
    BSW_VERSION_MAJOR=1
    BSW_VERSION_MINOR=0
    BSW_VERSION_PATCH=0
)

# Installation
install(TARGETS bsw
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/../../src/bsw/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Print summary
message(STATUS "YuleTech BSW Build Configuration")
message(STATUS "================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C Standard: C${CMAKE_C_STANDARD}")
message(STATUS "Source files: ${BSW_SOURCES}")
