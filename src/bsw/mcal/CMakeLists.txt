# MCAL Layer CMakeLists.txt
# Collects all MCAL driver submodules using file(GLOB)
# Libraries are prefixed with "mcal_" to avoid name conflicts
# across BSW layers (services, mcal, ecual may share module names)

cmake_minimum_required(VERSION 3.16)

# Get all submodule directories
file(GLOB MCAL_SUBDIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/*)

# Pre-collect all MCAL include directories so every driver can include
# headers from any other MCAL driver
set(MCAL_INCLUDE_DIRS)
foreach(subdir ${MCAL_SUBDIRS})
    if(IS_DIRECTORY ${subdir}/include)
        list(APPEND MCAL_INCLUDE_DIRS ${subdir}/include)
    endif()
endforeach()

# Service layer include directories (for Det.h, etc. used by MCAL modules)
set(SERVICE_INCLUDE_DIRS)
if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../services)
    file(GLOB SERVICE_SUBDIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/../services/*)
    foreach(subdir ${SERVICE_SUBDIRS})
        if(IS_DIRECTORY ${subdir}/include)
            list(APPEND SERVICE_INCLUDE_DIRS ${subdir}/include)
        endif()
    endforeach()
endif()

foreach(subdir ${MCAL_SUBDIRS})
    get_filename_component(module_name ${subdir} NAME)
    
    # Collect source files
    file(GLOB_RECURSE module_sources ${subdir}/src/*.c)
    
    if(module_sources)
        # Create library with mcal_ prefix to avoid name conflicts
        set(target_name "mcal_${module_name}")
        add_library(${target_name} STATIC ${module_sources})
        
        # Add include directories:
        # - PUBLIC: Own module headers
        # - PRIVATE: All MCAL + Services include dirs
        target_include_directories(${target_name}
            PUBLIC ${subdir}/include
            PRIVATE ${MCAL_INCLUDE_DIRS} ${SERVICE_INCLUDE_DIRS}
        )
        
        set_target_properties(${target_name} PROPERTIES
            C_STANDARD 99
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/mcal
        )
        
        message(STATUS "  MCAL driver: ${module_name} (target: ${target_name})")
    endif()
endforeach()

message(STATUS "MCAL layer configured")
