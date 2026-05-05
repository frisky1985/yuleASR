# ECUAL (ECU Abstraction Layer) CMakeLists.txt
# ECUAL: CanIf, CanTp, IoHwAb, EthIf, MemIf, Fee, Ea, FrIf, LinIf

cmake_minimum_required(VERSION 3.16)

#======================
# CAN Interface (CanIf)
#======================
file(GLOB CANIF_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/canif/src/*.c
)
add_library(CanIf STATIC ${CANIF_SOURCES})
target_include_directories(CanIf
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/canif/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/can/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../pdur/include
)
set_target_properties(CanIf PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/ecual
)

#======================
# CAN Transport Protocol (CanTp)
#======================
file(GLOB CANTP_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/cantp/src/*.c
)
add_library(CanTp STATIC ${CANTP_SOURCES})
target_include_directories(CanTp
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/cantp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../canif/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../pdur/include
)
set_target_properties(CanTp PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/ecual
)

#======================
# I/O Hardware Abstraction (IoHwAb)
#======================
file(GLOB IOHWAB_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/iohwab/src/*.c
)
add_library(IoHwAb STATIC ${IOHWAB_SOURCES})
target_include_directories(IoHwAb
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/iohwab/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/adc/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/dio/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/pwm/include
)
set_target_properties(IoHwAb PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/ecual
)

#======================
# Ethernet Interface (EthIf)
#======================
file(GLOB ETHIF_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/ethif/src/*.c
)
add_library(EthIf STATIC ${ETHIF_SOURCES})
target_include_directories(EthIf
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/ethif/include
)
set_target_properties(EthIf PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/ecual
)

#======================
# Memory Interface (MemIf)
#======================
file(GLOB MEMIF_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/memif/src/*.c
)
add_library(MemIf STATIC ${MEMIF_SOURCES})
target_include_directories(MemIf
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/memif/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../fee/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../ea/include
)
set_target_properties(MemIf PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/ecual
)

#======================
# Flash EEPROM Emulation (Fee)
#======================
file(GLOB FEE_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/fee/src/*.c
)
add_library(Fee STATIC ${FEE_SOURCES})
target_include_directories(Fee
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/fee/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../memif/include
)
set_target_properties(Fee PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/ecual
)

#======================
# EEPROM Abstraction (Ea)
#======================
file(GLOB EA_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/ea/src/*.c
)
add_library(Ea STATIC ${EA_SOURCES})
target_include_directories(Ea
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/ea/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../memif/include
)
set_target_properties(Ea PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/ecual
)

#======================
# FlexRay Interface (FrIf)
#======================
file(GLOB FRIF_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/frif/src/*.c
)
add_library(FrIf STATIC ${FRIF_SOURCES})
target_include_directories(FrIf
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/frif/include
)
set_target_properties(FrIf PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/ecual
)

#======================
# LIN Interface (LinIf)
#======================
file(GLOB LINIF_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/linif/src/*.c
)
add_library(LinIf STATIC ${LINIF_SOURCES})
target_include_directories(LinIf
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/linif/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/lin/include
)
set_target_properties(LinIf PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/ecual
)

#======================
# ECUAL Meta-Target
#======================
add_library(Ecual STATIC)
target_link_libraries(Ecual INTERFACE
    CanIf
    CanTp
    IoHwAb
    EthIf
    MemIf
    Fee
    Ea
    FrIf
    LinIf
)

message(STATUS "ECUAL configured (9 modules)")
