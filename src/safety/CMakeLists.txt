# Safety Layer CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

file(GLOB SAFETY_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

add_library(Safety STATIC ${SAFETY_SOURCES})

target_include_directories(Safety
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../bsw/mcal/wdg/include
)

set_target_properties(Safety PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/safety
)

message(STATUS "Safety layer configured")
