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
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/can/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/spi/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/i2c/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/fls/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/lin/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/uart/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../services/soad/include
    ${CMAKE_SOURCE_DIR}/src/platform/s32k312/include
    ${CMAKE_SOURCE_DIR}/src/common/include
    ${CMAKE_SOURCE_DIR}/include/autosar
    ${CMAKE_SOURCE_DIR}/config/input/ecual
    ${CMAKE_SOURCE_DIR}/config/input/services
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

# ── Phase 2 duplicate-module convergence (forwarding shims) ──────────────────
# These ECUAL modules are thin duplicates whose canonical implementation lives
# in the services layer. Their src/*.c files are excluded from compilation to
# avoid duplicate symbol definitions at link time; the CMake target itself is
# kept (fed with a generated comment-only stub) so target names, PUBLIC include
# paths and existing link references stay valid.
#   j1939tp -> services/j1939tp   (NULL check + DET reporting)
#   memif   -> services/memif     (MemMap/Rte integration; the ecual-only API
#                                  MemIf_EraseImmediateBlock was merged into
#                                  the canonical implementation — NvM depends
#                                  on it)
#   xcp     -> services/xcp       (superset implementation, 2086 vs 1087 LOC)
#   fim     -> services/fim       (AUTOSAR layering: FiM belongs to Services)
set(ECUAL_SHIMMED_MODULES
    j1939tp
    memif
    xcp
    fim
    someipsd
)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated/shim-stubs")

foreach(subdir ${ECUAL_SUBDIRS})
    get_filename_component(module_name ${subdir} NAME)

    list(FIND ECUAL_SHIMMED_MODULES "${module_name}" _shimmed_idx)
    if(NOT _shimmed_idx EQUAL -1)
        # Shimmed module: exclude real sources, keep target alive with an
        # empty (comment-only) stub translation unit.
        set(module_sources "${CMAKE_BINARY_DIR}/generated/shim-stubs/ecual_${module_name}_shim.c")
        file(WRITE "${module_sources}"
            "/* Phase 2 duplicate-module convergence: forwarding shim for ecual_${module_name}.\n"
            " * Canonical implementation lives in the services layer; the sources of\n"
            " * this module are excluded from the build to avoid duplicate symbols.\n"
            " */\n")
    else()
        file(GLOB_RECURSE module_sources ${subdir}/src/*.c)
    endif()
    
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