#==================================================================================================
# Project              : YuleTech AutoSAR BSW
# Build System         : CMake for Unit Tests
#
# SW Version           : 1.0.0
# Build Date           : 2026-04-23
#
# (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
# All Rights Reserved.
#==================================================================================================

cmake_minimum_required(VERSION 3.14)
project(YuleTech_BSW_Tests C)

#==================================================================================================
# Configuration
#==================================================================================================
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Test framework selection: CMOCKA or UNITY
option(USE_CMOCKA "Use CMocka test framework" ON)
option(USE_UNITY "Use Unity test framework" OFF)
option(COVERAGE "Enable code coverage" OFF)

#==================================================================================================
# Compiler Flags
#==================================================================================================
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter -Wno-unused-variable")
    
    if(COVERAGE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    endif()
endif()

#==================================================================================================
# Include Directories
#==================================================================================================
include_directories(
    ${CMAKE_SOURCE_DIR}/../src/bsw/general/inc
    ${CMAKE_SOURCE_DIR}/../src/bsw/mcal/mcu/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/mcal/port/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/mcal/dio/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/mcal/can/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/mcal/spi/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/mcal/gpt/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/mcal/pwm/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/mcal/adc/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/mcal/wdg/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/ecual/canif/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/ecual/iohwab/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/ecual/cantp/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/ecual/ethif/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/ecual/memif/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/ecual/fee/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/ecual/ea/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/ecual/frif/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/ecual/linif/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/services/com/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/services/pdur/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/services/nvm/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/services/dcm/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/services/dem/include
    ${CMAKE_SOURCE_DIR}/../src/bsw/os/include
    ${CMAKE_SOURCE_DIR}/mocks
    ${CMAKE_SOURCE_DIR}/unit
)

#==================================================================================================
# Find Test Framework
#==================================================================================================
if(USE_CMOCKA)
    find_package(cmocka QUIET)
    if(NOT cmocka_FOUND)
        # Use bundled cmocka or fallback to custom framework
        message(STATUS "CMocka not found, using built-in test framework")
        set(USE_BUILT_IN ON)
    else()
        set(TEST_LIBS cmocka)
    endif()
elseif(USE_UNITY)
    # Unity framework - can be added as external project
    message(STATUS "Using Unity test framework")
    set(TEST_LIBS unity)
else()
    set(USE_BUILT_IN ON)
endif()

#==================================================================================================
# Mock Sources
#==================================================================================================
set(MOCK_SOURCES
    mocks/mock_det.c
    mocks/mock_mcal.c
    mocks/mock_ecual.c
    mocks/mock_services.c
    mocks/mock_os.c
)

#==================================================================================================
# MCAL Driver Sources (for testing)
#==================================================================================================
set(MCAL_SOURCES
    ${CMAKE_SOURCE_DIR}/../src/bsw/mcal/mcu/src/Mcu.c
    ${CMAKE_SOURCE_DIR}/../src/bsw/mcal/port/src/Port.c
    ${CMAKE_SOURCE_DIR}/../src/bsw/mcal/dio/src/Dio.c
    ${CMAKE_SOURCE_DIR}/../src/bsw/mcal/can/src/Can.c
    ${CMAKE_SOURCE_DIR}/../src/bsw/mcal/spi/src/Spi.c
    ${CMAKE_SOURCE_DIR}/../src/bsw/mcal/gpt/src/Gpt.c
    ${CMAKE_SOURCE_DIR}/../src/bsw/mcal/pwm/src/Pwm.c
    ${CMAKE_SOURCE_DIR}/../src/bsw/mcal/adc/src/Adc.c
    ${CMAKE_SOURCE_DIR}/../src/bsw/mcal/wdg/src/Wdg.c
)

#==================================================================================================
# Unit Test Executables
#==================================================================================================
# MCAL Tests
add_executable(test_mcu unit/mcal/test_mcu.c ${MOCK_SOURCES})
add_executable(test_port unit/mcal/test_port.c ${MOCK_SOURCES})
add_executable(test_dio unit/mcal/test_dio.c ${MOCK_SOURCES})
add_executable(test_can unit/mcal/test_can.c ${MOCK_SOURCES})
add_executable(test_spi unit/mcal/test_spi.c ${MOCK_SOURCES})
add_executable(test_gpt unit/mcal/test_gpt.c ${MOCK_SOURCES})
add_executable(test_pwm unit/mcal/test_pwm.c ${MOCK_SOURCES})
add_executable(test_adc unit/mcal/test_adc.c ${MOCK_SOURCES})
add_executable(test_wdg unit/mcal/test_wdg.c ${MOCK_SOURCES})

# Set link libraries for all tests
set(TEST_EXECUTABLES
    test_mcu test_port test_dio test_can test_spi
    test_gpt test_pwm test_adc test_wdg
)

foreach(test_exec ${TEST_EXECUTABLES})
    if(USE_BUILT_IN)
        # Built-in framework doesn't need linking
    else()
        target_link_libraries(${test_exec} ${TEST_LIBS})
    endif()
endforeach()

#==================================================================================================
# Enable Testing
#==================================================================================================
enable_testing()

# Add tests
add_test(NAME Mcu_Test COMMAND test_mcu)
add_test(NAME Port_Test COMMAND test_port)
add_test(NAME Dio_Test COMMAND test_dio)
add_test(NAME Can_Test COMMAND test_can)
add_test(NAME Spi_Test COMMAND test_spi)
add_test(NAME Gpt_Test COMMAND test_gpt)
add_test(NAME Pwm_Test COMMAND test_pwm)
add_test(NAME Adc_Test COMMAND test_adc)
add_test(NAME Wdg_Test COMMAND test_wdg)

#==================================================================================================
# Custom Targets
#==================================================================================================
# Run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ${TEST_EXECUTABLES}
    COMMENT "Running all unit tests"
)

# Coverage report
if(COVERAGE)
    find_program(GCOVR gcovr)
    if(GCOVR)
        add_custom_target(coverage
            COMMAND ${GCOVR} -r ${CMAKE_SOURCE_DIR}/.. --html --html-details -o coverage.html
            COMMENT "Generating coverage report"
        )
    endif()
endif()
