# CDD (Complex Device Driver) Layer CMakeLists.txt
# Builds CDD modules: Safety, Boot, Fvm, Hsm, RamEcc, Lockstep

cmake_minimum_required(VERSION 3.16)

# Global includes for Std_Types.h, Det.h, etc.
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../os/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../services/det/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../rte/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/crypto/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/eth/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/dio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/can/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/lin/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/fee/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/ethif/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/canif/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/linif/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/ea/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../services/tcpip/include
    ${CMAKE_SOURCE_DIR}/config/input/services
    ${CMAKE_SOURCE_DIR}/config/input/mcal
    ${CMAKE_SOURCE_DIR}/include/autosar
    ${CMAKE_SOURCE_DIR}/include/autosar/classic/com
    ${CMAKE_SOURCE_DIR}/src/common/include
    ${CMAKE_SOURCE_DIR}/src/platform/s32k312/include
)

# Build only Cdd_Fvm (required by EcuM lifecycle integration)
# Other CDD modules (Hsm, Boot, Safety, etc.) have pre-existing compilation
# issues and are not yet integrated into the build.
set(CDD_FVM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Cdd_Fvm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Cdd_Fvm_Hw.c
)

if(CDD_FVM_SOURCES)
    add_library(cdd STATIC ${CDD_FVM_SOURCES})

    target_include_directories(cdd
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    set_target_properties(cdd PROPERTIES
        C_STANDARD 99
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/cdd
    )

    message(STATUS "  CDD module configured (target: cdd)")
endif()

message(STATUS "CDD layer configured")
