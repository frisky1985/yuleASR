# ECUAL Layer CMakeLists.txt
# Collects all ECU Abstraction submodules using file(GLOB)
# Libraries are prefixed with "ecual_" to avoid name conflicts
# across BSW layers (services, mcal, ecual may share module names)

cmake_minimum_required(VERSION 3.16)

# Get all submodule directories
file(GLOB ECUAL_SUBDIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/*)

foreach(subdir ${ECUAL_SUBDIRS})
    get_filename_component(module_name ${subdir} NAME)
    
    # Collect source files
    file(GLOB_RECURSE module_sources ${subdir}/src/*.c)
    
    if(module_sources)
        # Create library with ecual_ prefix to avoid name conflicts
        set(target_name "ecual_${module_name}")
        add_library(${target_name} STATIC ${module_sources})
        
        # Add include directory
        if(IS_DIRECTORY ${subdir}/include)
            target_include_directories(${target_name}
                PUBLIC ${subdir}/include
            )
        endif()
        
        set_target_properties(${target_name} PROPERTIES
            C_STANDARD 99
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/ecual
        )
        
        message(STATUS "  ECUAL module: ${module_name} (target: ${target_name})")
    endif()
endforeach()

message(STATUS "ECUAL layer configured")
