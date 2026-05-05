# Integration Layer CMakeLists.txt
# EcuM, BswM

cmake_minimum_required(VERSION 3.16)

file(GLOB INTEGRATION_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

add_library(Integration STATIC ${INTEGRATION_SOURCES})

target_include_directories(Integration
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/mcu/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../os/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../services/dcm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../services/dem/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../services/wdgm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../rte/include
)

set_target_properties(Integration PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/integration
)

message(STATUS "Integration layer configured (EcuM, BswM)")
