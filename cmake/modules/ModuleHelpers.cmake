# ModuleHelpers.cmake
# Helper functions for YuleTech AutoSAR CMake build system

# Include guard
if(__YULETECH_MODULE_HELPERS__)
    return()
endif()
set(__YULETECH_MODULE_HELPERS__ TRUE)

include(CMakeParseArguments)

#
# Function: yule_add_module
# Add an AutoSAR BSW module as a static library
#
# Usage:
#   yule_add_module(
#       NAME <module_name>
#       SOURCES <source_files...>
#       [INCLUDES <include_dirs...>]
#       [DEPENDS <dependencies...>]
#       [STANDARD <c_standard>]
#   )
#
function(yule_add_module)
    set(options)
    set(oneValueArgs NAME STANDARD)
    set(multiValueArgs SOURCES INCLUDES DEPENDS)
    cmake_parse_arguments(MODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT MODULE_NAME)
        message(FATAL_ERROR "yule_add_module: NAME is required")
    endif()
    if(NOT MODULE_SOURCES)
        message(FATAL_ERROR "yule_add_module: SOURCES is required")
    endif()

    # Create library
    add_library(${MODULE_NAME} STATIC ${MODULE_SOURCES})

    # Set include directories
    target_include_directories(${MODULE_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include/${MODULE_NAME}>
        PRIVATE
            ${MODULE_INCLUDES}
    )

    # Set compile standard
    if(MODULE_STANDARD)
        set_target_properties(${MODULE_NAME} PROPERTIES
            C_STANDARD ${MODULE_STANDARD}
            C_STANDARD_REQUIRED ON
        )
    else()
        set_target_properties(${MODULE_NAME} PROPERTIES
            C_STANDARD 99
            C_STANDARD_REQUIRED ON
        )
    endif()

    # Link dependencies
    if(MODULE_DEPENDS)
        target_link_libraries(${MODULE_NAME} PUBLIC ${MODULE_DEPENDS})
    endif()

    # Common compile options for embedded/AutoSAR code
    target_compile_options(${MODULE_NAME} PRIVATE
        -Wall
        -Wextra
        -Wshadow
        -Wcast-align
        -Wwrite-strings
        -Wstrict-prototypes
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:MinSizeRel>:-Os>
    )

    # Common compile definitions
    target_compile_definitions(${MODULE_NAME} PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
        $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
    )

    # Set output properties
    set_target_properties(${MODULE_NAME} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    message(STATUS "Added module: ${MODULE_NAME}")
endfunction()

#
# Function: yule_add_executable
# Add an executable target with proper linker configuration
#
# Usage:
#   yule_add_executable(
#       NAME <exe_name>
#       SOURCES <source_files...>
#       [LINKER_SCRIPT <path>]
#       [DEPENDS <dependencies...>]
#   )
#
function(yule_add_executable)
    set(options)
    set(oneValueArgs NAME LINKER_SCRIPT)
    set(multiValueArgs SOURCES DEPENDS)
    cmake_parse_arguments(EXE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT EXE_NAME)
        message(FATAL_ERROR "yule_add_executable: NAME is required")
    endif()
    if(NOT EXE_SOURCES)
        message(FATAL_ERROR "yule_add_executable: SOURCES is required")
    endif()

    # Create executable
    add_executable(${EXE_NAME} ${EXE_SOURCES})

    # Link dependencies
    if(EXE_DEPENDS)
        target_link_libraries(${EXE_NAME} PRIVATE ${EXE_DEPENDS})
    endif()

    # Linker script
    if(EXE_LINKER_SCRIPT)
        target_link_options(${EXE_NAME} PRIVATE
            -T${EXE_LINKER_SCRIPT}
            -Wl,-Map=${CMAKE_BINARY_DIR}/${EXE_NAME}.map
        )
        set_target_properties(${EXE_NAME} PROPERTIES
            LINK_DEPENDS ${EXE_LINKER_SCRIPT}
        )
    endif()

    # Generate binary, hex, and listing files (for embedded targets)
    if(CMAKE_CROSSCOMPILING)
        add_custom_command(TARGET ${EXE_NAME} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${EXE_NAME}> ${CMAKE_BINARY_DIR}/${EXE_NAME}.bin
            COMMENT "Generating binary file: ${EXE_NAME}.bin"
        )
        add_custom_command(TARGET ${EXE_NAME} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${EXE_NAME}> ${CMAKE_BINARY_DIR}/${EXE_NAME}.hex
            COMMENT "Generating hex file: ${EXE_NAME}.hex"
        )
        add_custom_command(TARGET ${EXE_NAME} POST_BUILD
            COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:${EXE_NAME}> > ${CMAKE_BINARY_DIR}/${EXE_NAME}.lst
            COMMENT "Generating listing file: ${EXE_NAME}.lst"
        )
        add_custom_command(TARGET ${EXE_NAME} POST_BUILD
            COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${EXE_NAME}>
            COMMENT "Size information for: ${EXE_NAME}"
        )
    endif()

    set_target_properties(${EXE_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    message(STATUS "Added executable: ${EXE_NAME}")
endfunction()

#
# Function: yule_add_test
# Add a unit test executable
#
# Usage:
#   yule_add_test(
#       NAME <test_name>
#       SOURCES <source_files...>
#       [DEPENDS <dependencies...>]
#   )
#
function(yule_add_test)
    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES DEPENDS)
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT TEST_NAME)
        message(FATAL_ERROR "yule_add_test: NAME is required")
    endif()
    if(NOT TEST_SOURCES)
        message(FATAL_ERROR "yule_add_test: SOURCES is required")
    endif()

    # Create test executable
    add_executable(${TEST_NAME} ${TEST_SOURCES})

    # Link dependencies
    if(TEST_DEPENDS)
        target_link_libraries(${TEST_NAME} PRIVATE ${TEST_DEPENDS})
    endif()

    # Register with CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

    set_target_properties(${TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    message(STATUS "Added test: ${TEST_NAME}")
endfunction()

#
# Function: yule_collect_sources
# Collect all source files in a directory
#
# Usage:
#   yule_collect_sources(
#       OUTPUT <variable>
#       [DIRS <directories...>]
#       [PATTERNS <patterns...>]
#   )
#
function(yule_collect_sources)
    set(options)
    set(oneValueArgs OUTPUT)
    set(multiValueArgs DIRS PATTERNS)
    cmake_parse_arguments(COLLECT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT COLLECT_OUTPUT)
        message(FATAL_ERROR "yule_collect_sources: OUTPUT is required")
    endif()

    if(NOT COLLECT_DIRS)
        set(COLLECT_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    if(NOT COLLECT_PATTERNS)
        set(COLLECT_PATTERNS "*.c")
    endif()

    set(_sources)
    foreach(_dir ${COLLECT_DIRS})
        foreach(_pattern ${COLLECT_PATTERNS})
            file(GLOB _files "${_dir}/${_pattern}")
            list(APPEND _sources ${_files})
        endforeach()
    endforeach()

    set(${COLLECT_OUTPUT} ${_sources} PARENT_SCOPE)
endfunction()

#
# Function: yule_add_coverage
# Enable code coverage for a target
#
function(yule_add_coverage target)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE --coverage)
        target_link_options(${target} PRIVATE --coverage)
    endif()
endfunction()
