# ECUAL Layer CMakeLists.txt
# Collects all ECU Abstraction submodules using file(GLOB)
# Libraries are prefixed with "ecual_" to avoid name conflicts
# across BSW layers (services, mcal, ecual may share module names)

cmake_minimum_required(VERSION 3.16)

# Get all submodule directories
file(GLOB ECUAL_SUBDIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/*)

# Pre-collect all ECUAL include directories
set(ECUAL_INCLUDE_DIRS)
foreach(subdir ${ECUAL_SUBDIRS})
    if(IS_DIRECTORY ${subdir}/include)
        list(APPEND ECUAL_INCLUDE_DIRS ${subdir}/include)
    endif()
endforeach()

# Service layer include directories (for PduR.h, Det.h, ComM.h, etc.)
set(SERVICE_INCLUDE_DIRS)
if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../services)
    file(GLOB SERVICE_SUBDIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/../services/*)
    foreach(subdir ${SERVICE_SUBDIRS})
        if(IS_DIRECTORY ${subdir}/include)
            list(APPEND SERVICE_INCLUDE_DIRS ${subdir}/include)
        endif()
    endforeach()
endif()

# MCAL layer include directories (for Can.h, Lin.h, Eth.h, etc.)
set(MCAL_INCLUDE_DIRS)
if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../mcal)
    file(GLOB MCAL_SUBDIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/*)
    foreach(subdir ${MCAL_SUBDIRS})
        if(IS_DIRECTORY ${subdir}/include)
            list(APPEND MCAL_INCLUDE_DIRS ${subdir}/include)
        endif()
    endforeach()
endif()

foreach(subdir ${ECUAL_SUBDIRS})
    get_filename_component(module_name ${subdir} NAME)
    
    # Collect source files
    file(GLOB_RECURSE module_sources ${subdir}/src/*.c)
    
    if(module_sources)
        # Create library with ecual_ prefix to avoid name conflicts
        set(target_name "ecual_${module_name}")
        add_library(${target_name} STATIC ${module_sources})
        
        # Add include directories:
        # - PUBLIC: Own module headers
        # - PRIVATE: All ECUAL + Services + MCAL include dirs
        target_include_directories(${target_name}
            PUBLIC ${subdir}/include
            PRIVATE ${ECUAL_INCLUDE_DIRS} ${SERVICE_INCLUDE_DIRS} ${MCAL_INCLUDE_DIRS}
        )
        
        set_target_properties(${target_name} PROPERTIES
            C_STANDARD 99
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/ecual
        )
        
        message(STATUS "  ECUAL module: ${module_name} (target: ${target_name})")
    endif()
endforeach()

message(STATUS "ECUAL layer configured")
