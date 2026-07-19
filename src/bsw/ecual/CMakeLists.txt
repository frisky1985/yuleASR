# ECUAL Layer CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

# Global includes
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../os/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../services/det/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../rte/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/eth/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/dio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/adc/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/port/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../services/soad/include
)

# Collect all ECUAL + services include directories for cross-module use
file(GLOB ECUAL_SUBDIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/*)
file(GLOB SERVICE_SUBDIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/../services/*)

set(CROSS_INCLUDE_DIRS "")
foreach(subdir ${ECUAL_SUBDIRS})
    if(IS_DIRECTORY ${subdir}/include)
        list(APPEND CROSS_INCLUDE_DIRS ${subdir}/include)
    endif()
endforeach()
foreach(subdir ${SERVICE_SUBDIRS})
    if(IS_DIRECTORY ${subdir}/include)
        list(APPEND CROSS_INCLUDE_DIRS ${subdir}/include)
    endif()
endforeach()

foreach(subdir ${ECUAL_SUBDIRS})
    get_filename_component(module_name ${subdir} NAME)
    
    file(GLOB_RECURSE module_sources ${subdir}/src/*.c)
    
    if(module_sources)
        set(target_name "ecual_${module_name}")
        add_library(${target_name} STATIC ${module_sources})
        
        if(IS_DIRECTORY ${subdir}/include)
            target_include_directories(${target_name}
                PUBLIC ${subdir}/include
            )
        endif()
        
        target_include_directories(${target_name}
            PUBLIC ${CROSS_INCLUDE_DIRS}
        )
        
        set_target_properties(${target_name} PROPERTIES
            C_STANDARD 99
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/ecual
        )
    endif()
endforeach()