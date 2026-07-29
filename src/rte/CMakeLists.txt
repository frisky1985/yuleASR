# RTE (Runtime Environment) CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

file(GLOB RTE_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/*.c
)

add_library(Rte STATIC ${RTE_SOURCES})

# Collect all service module include directories
file(GLOB RTE_SERVICE_SUBDIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/../bsw/services/*)
set(RTE_SERVICE_INCLUDE_DIRS)
foreach(subdir ${RTE_SERVICE_SUBDIRS})
    if(IS_DIRECTORY ${subdir}/include)
        list(APPEND RTE_SERVICE_INCLUDE_DIRS ${subdir}/include)
    endif()
endforeach()

# Collect all MCAL include directories
file(GLOB RTE_MCAL_SUBDIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/../bsw/mcal/*)
set(RTE_MCAL_INCLUDE_DIRS)
foreach(subdir ${RTE_MCAL_SUBDIRS})
    if(IS_DIRECTORY ${subdir}/include)
        list(APPEND RTE_MCAL_INCLUDE_DIRS ${subdir}/include)
    endif()
endforeach()

# Collect all ECUAL include directories
file(GLOB RTE_ECUAL_SUBDIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/../bsw/ecual/*)
set(RTE_ECUAL_INCLUDE_DIRS)
foreach(subdir ${RTE_ECUAL_SUBDIRS})
    if(IS_DIRECTORY ${subdir}/include)
        list(APPEND RTE_ECUAL_INCLUDE_DIRS ${subdir}/include)
    endif()
endforeach()

target_include_directories(Rte
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/generated
    PRIVATE
        ${RTE_SERVICE_INCLUDE_DIRS}
        ${RTE_MCAL_INCLUDE_DIRS}
        ${RTE_ECUAL_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/../platform/s32k312/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../bsw/os/include
)

set_target_properties(Rte PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/rte
)

message(STATUS "RTE configured")
