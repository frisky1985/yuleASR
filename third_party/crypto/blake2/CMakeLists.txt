#**********************************************************************************************************************
# @file       CMakeLists.txt
# @brief      BLAKE2 Hash Algorithm Library Build Configuration
#
# Function:   Build configuration for BLAKE2 cryptographic hash library
#
# Features:
# - BLAKE2b (64-bit optimized, 512-bit output)
# - BLAKE2s (32-bit optimized, 256-bit output)
# - AUTOSAR Crypto Stack integration
# - MISRA-C:2012 compliant
#
# @author     YuleTech AutoSAR Team
# @version    1.0.0
# @date       2026-05-01
# @copyright  Shanghai Yule Electronics Technology Co., Ltd.
#**********************************************************************************************************************

cmake_minimum_required(VERSION 3.16)

#======================================================================================================================
# BLAKE2 Library Target
#======================================================================================================================
add_library(blake2 STATIC
    src/blake2b.c
    src/blake2s.c
    src/blake2_autosar.c
)

#======================================================================================================================
# Include Directories
#======================================================================================================================
target_include_directories(blake2
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include/autosar/classic/crypto
        ${CMAKE_SOURCE_DIR}/include/autosar/classic/platform
        ${CMAKE_SOURCE_DIR}/src/bsw/mcal/crypto/include
)

#======================================================================================================================
# Compile Definitions
#======================================================================================================================
target_compile_definitions(blake2
    PRIVATE
        $<$<BOOL:${BLAKE2_DEV_ERROR_DETECT}>:BLAKE2_DEV_ERROR_DETECT>
        $<$<BOOL:${BLAKE2_ENABLE_BENCHMARK}>:BLAKE2_ENABLE_BENCHMARK>
)

#======================================================================================================================
# Compile Options
#======================================================================================================================
# Compiler-specific options
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(blake2
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wcast-align
            -Wstrict-prototypes
            -Wmissing-prototypes
            -O2
            -fomit-frame-pointer
            $<$<CONFIG:Release>:-Os>
            $<$<CONFIG:Debug>:-g -O0 -DDEBUG>
    )
elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
    target_compile_options(blake2
        PRIVATE
            /W4
            /wd4127  # Disable conditional expression is constant warning
            /O2
            $<$<CONFIG:Release>:/Os>
            $<$<CONFIG:Debug>:/Od /Zi /DDEBUG>
    )
endif()

#======================================================================================================================
# MISRA-C Compliance Options
#======================================================================================================================
option(BLAKE2_MISRA_CHECK "Enable MISRA-C:2012 checking" OFF)

if(BLAKE2_MISRA_CHECK)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU")
        target_compile_options(blake2
            PRIVATE
                -Wconversion
                -Wsign-conversion
                -Wuninitialized
                -Wmaybe-uninitialized
        )
    endif()
endif()

#======================================================================================================================
# Link Dependencies
#======================================================================================================================
target_link_libraries(blake2
    PUBLIC
        crypto_stack_types
    PRIVATE
        $<$<TARGET_EXISTS:Det>:Det>
)

#======================================================================================================================
# Installation
#======================================================================================================================
install(TARGETS blake2
    EXPORT blake2-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES
    include/blake2.h
    DESTINATION include
)

install(EXPORT blake2-targets
    FILE blake2-targets.cmake
    NAMESPACE YuleTech::
    DESTINATION lib/cmake/blake2
)

#======================================================================================================================
# Testing Configuration
#======================================================================================================================
option(BLAKE2_BUILD_TESTS "Build BLAKE2 unit tests" ON)

if(BLAKE2_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

#======================================================================================================================
# Documentation
#======================================================================================================================
option(BLAKE2_BUILD_DOCS "Build BLAKE2 documentation" OFF)

if(BLAKE2_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(blake2_docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating BLAKE2 API documentation with Doxygen"
            VERBATIM
        )
    endif()
endif()

#======================================================================================================================
# Source Grouping (for IDE)
#======================================================================================================================
source_group("Header Files" FILES include/blake2.h)
source_group("Source Files\\Core" FILES src/blake2b.c src/blake2s.c)
source_group("Source Files\\AUTOSAR" FILES src/blake2_autosar.c)

#======================================================================================================================
# Summary
#======================================================================================================================
message(STATUS "BLAKE2 Configuration:")
message(STATUS "  DEV_ERROR_DETECT: ${BLAKE2_DEV_ERROR_DETECT}")
message(STATUS "  ENABLE_BENCHMARK: ${BLAKE2_ENABLE_BENCHMARK}")
message(STATUS "  MISRA_CHECK: ${BLAKE2_MISRA_CHECK}")
message(STATUS "  BUILD_TESTS: ${BLAKE2_BUILD_TESTS}")
message(STATUS "  BUILD_DOCS: ${BLAKE2_BUILD_DOCS}")
