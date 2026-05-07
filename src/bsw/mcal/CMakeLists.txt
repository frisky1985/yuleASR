# MCAL (Microcontroller Driver) Layer CMakeLists.txt
# MCAL Drivers: Mcu, Port, Dio, Can, Spi, Gpt, Pwm, Adc, Wdg, Icu, Lin, Crypto

cmake_minimum_required(VERSION 3.16)

option(MCAL_BUILD_DRIVERS "Build MCAL drivers" ON)

if(NOT MCAL_BUILD_DRIVERS)
    message(STATUS "MCAL driver compilation disabled")
    return()
endif()

# Common MCAL includes
set(MCAL_COMMON_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../general/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../platform/s32k312/include
)

#======================
# MCU Driver
#======================
file(GLOB MCU_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/mcu/src/*.c
)
add_library(McalMcu STATIC ${MCU_SOURCES})
target_include_directories(McalMcu
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/mcu/include
    PRIVATE
        ${MCAL_COMMON_INCLUDES}
)
target_compile_definitions(McalMcu PRIVATE MCAL_ENABLE)
set_target_properties(McalMcu PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/mcal
)

#======================
# Port Driver
#======================
file(GLOB PORT_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/port/src/*.c
)
add_library(McalPort STATIC ${PORT_SOURCES})
target_include_directories(McalPort
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/port/include
    PRIVATE
        ${MCAL_COMMON_INCLUDES}
)
set_target_properties(McalPort PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/mcal
)

#======================
# DIO Driver
#======================
file(GLOB DIO_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/dio/src/*.c
)
add_library(McalDio STATIC ${DIO_SOURCES})
target_include_directories(McalDio
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/dio/include
    PRIVATE
        ${MCAL_COMMON_INCLUDES}
)
set_target_properties(McalDio PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/mcal
)

#======================
# CAN Driver
#======================
file(GLOB CAN_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/can/src/*.c
)
add_library(McalCan STATIC ${CAN_SOURCES})
target_include_directories(McalCan
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/can/include
    PRIVATE
        ${MCAL_COMMON_INCLUDES}
)
set_target_properties(McalCan PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/mcal
)

#======================
# SPI Driver
#======================
file(GLOB SPI_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/spi/src/*.c
)
add_library(McalSpi STATIC ${SPI_SOURCES})
target_include_directories(McalSpi
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/spi/include
    PRIVATE
        ${MCAL_COMMON_INCLUDES}
)
set_target_properties(McalSpi PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/mcal
)

#======================
# GPT Driver (General Purpose Timer)
#======================
file(GLOB GPT_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/gpt/src/*.c
)
add_library(McalGpt STATIC ${GPT_SOURCES})
target_include_directories(McalGpt
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/gpt/include
    PRIVATE
        ${MCAL_COMMON_INCLUDES}
)
set_target_properties(McalGpt PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/mcal
)

#======================
# PWM Driver
#======================
file(GLOB PWM_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/pwm/src/*.c
)
add_library(McalPwm STATIC ${PWM_SOURCES})
target_include_directories(McalPwm
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/pwm/include
    PRIVATE
        ${MCAL_COMMON_INCLUDES}
)
set_target_properties(McalPwm PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/mcal
)

#======================
# ADC Driver
#======================
file(GLOB ADC_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/adc/src/*.c
)
add_library(McalAdc STATIC ${ADC_SOURCES})
target_include_directories(McalAdc
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/adc/include
    PRIVATE
        ${MCAL_COMMON_INCLUDES}
)
set_target_properties(McalAdc PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/mcal
)

#======================
# WDG Driver
#======================
file(GLOB WDG_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/wdg/src/*.c
)
add_library(McalWdg STATIC ${WDG_SOURCES})
target_include_directories(McalWdg
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/wdg/include
    PRIVATE
        ${MCAL_COMMON_INCLUDES}
)
set_target_properties(McalWdg PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/mcal
)

#======================
# ICU Driver (Input Capture Unit)
#======================
file(GLOB ICU_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/icu/src/*.c
)
add_library(McalIcu STATIC ${ICU_SOURCES})
target_include_directories(McalIcu
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/icu/include
    PRIVATE
        ${MCAL_COMMON_INCLUDES}
)
set_target_properties(McalIcu PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/mcal
)

#======================
# LIN Driver
#======================
file(GLOB LIN_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/lin/src/*.c
)
add_library(McalLin STATIC ${LIN_SOURCES})
target_include_directories(McalLin
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/lin/include
    PRIVATE
        ${MCAL_COMMON_INCLUDES}
)
set_target_properties(McalLin PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/mcal
)

#======================
# Crypto Driver (Cryptographic Services with HSM support)
#======================
file(GLOB CRYPTO_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto/src/*.c
)
add_library(McalCrypto STATIC ${CRYPTO_SOURCES})
target_include_directories(McalCrypto
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/crypto/include
    PRIVATE
        ${MCAL_COMMON_INCLUDES}
)
target_compile_definitions(McalCrypto PRIVATE 
    MCAL_ENABLE
    MBEDTLS_CONFIG_FILE=<mbedtls_config.h>
)
set_target_properties(McalCrypto PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/mcal
)

#======================
# MCAL Meta-Target
#======================
add_library(McalDrivers INTERFACE)
target_link_libraries(McalDrivers INTERFACE
    McalMcu
    McalPort
    McalDio
    McalCan
    McalSpi
    McalGpt
    McalPwm
    McalAdc
    McalWdg
    McalIcu
    McalLin
    McalCrypto
)

message(STATUS "MCAL Drivers configured (11 drivers)")
