# MCAL Layer CMakeLists.txt
# Collects all MCAL driver submodules using file(GLOB)
# Libraries are prefixed with "mcal_" to avoid name conflicts
# across BSW layers (services, mcal, ecual may share module names)

cmake_minimum_required(VERSION 3.16)

# Global include directories for all MCAL modules (Std_Types.h, Det.h, MemMap.h, etc.)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../os/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../services/det/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../rte/include
    ${CMAKE_SOURCE_DIR}/config/input/mcal
)

# Get all submodule directories
file(GLOB MCAL_SUBDIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/*)

foreach(subdir ${MCAL_SUBDIRS})
    get_filename_component(module_name ${subdir} NAME)
    
    # Collect source files
    file(GLOB_RECURSE module_sources ${subdir}/src/*.c)
    
    # Exclude unity-build _impl.c files (included by parent .c via #include)
    list(FILTER module_sources EXCLUDE REGEX ".*_impl\\.c$")
    
    if(module_sources)
        # Create library with mcal_ prefix to avoid name conflicts
        set(target_name "mcal_${module_name}")
        add_library(${target_name} STATIC ${module_sources})
        
        # Add include directory
        if(IS_DIRECTORY ${subdir}/include)
            target_include_directories(${target_name}
                PUBLIC ${subdir}/include
            )
        endif()
        
        set_target_properties(${target_name} PROPERTIES
            C_STANDARD 99
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/mcal
        )
        
        message(STATUS "  MCAL driver: ${module_name} (target: ${target_name})")
    endif()
endforeach()

message(STATUS "MCAL layer configured")
