# Safety (saferam) unit tests
#
# Compiles the real production sources under src/safety/saferam/ directly into
# each test executable (Apple Clang needs this for .gcda generation).
#
# Platform note: the saferam modules address RAM through 32-bit integers
# (guard zones, canaries, heap pool, fault-injection targets all cast
# uint32_t -> pointer).  On Apple Silicon the default __PAGEZERO reserves the
# whole low 4 GB, so tests build as x86_64 (Rosetta) with a 1-page pagezero,
# which makes low 4 GB addresses mappable again via mmap(MAP_FIXED).  The test
# fixtures then map a RAM pool at 0x20000000 (see safety_test_mem.h).

cmake_minimum_required(VERSION 3.16)

set(SAFERAM_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/safety/saferam)

set(SAFERAM_ALL_SOURCES
    ${SAFERAM_SRC_DIR}/saferam_manager.c
    ${SAFERAM_SRC_DIR}/ram_partition.c
    ${SAFERAM_SRC_DIR}/stack_protection.c
    ${SAFERAM_SRC_DIR}/heap_protection.c
    ${SAFERAM_SRC_DIR}/safe_data.c
    ${SAFERAM_SRC_DIR}/fault_injection.c
)

if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    # Single x86_64 slice: fat binaries with a custom pagezero are killed
    # at launch under Rosetta.
    set(CMAKE_OSX_ARCHITECTURES x86_64)
    set(SAFETY_ARCH_FLAGS -arch x86_64)
    # -arch must also reach the link driver; pagezero_size unlocks low 4 GB.
    set(SAFETY_LINK_FLAGS -arch x86_64 -Wl,-pagezero_size,0x1000)
endif()

function(safety_add_test NAME MAIN_SRC)
    set(extra_sources ${ARGN})
    add_executable(${NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/${MAIN_SRC}
        ${extra_sources}
    )

    target_include_directories(${NAME} PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${SAFERAM_SRC_DIR}
    )

    target_compile_options(${NAME} PRIVATE
        -Wall -Wextra -g -O0
        -Wno-unused-parameter
        -Wno-unused-function
        -Wno-int-to-pointer-cast
        ${SAFETY_ARCH_FLAGS}
    )

    if(SAFETY_LINK_FLAGS)
        target_link_options(${NAME} PRIVATE ${SAFETY_LINK_FLAGS})
    endif()

    set_target_properties(${NAME} PROPERTIES
        C_STANDARD 99
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    # Compile unity.c directly as x86_64 to match Safety's arch override.
    # TEST_FRAMEWORK_LIB (top-level libUnity.a) is arm64 on Apple Silicon,
    # so a Safety target compiled as x86_64 would fail to link it.
    if(TEST_FRAMEWORK_INCLUDE)
        target_sources(${NAME} PRIVATE ${TEST_FRAMEWORK_INCLUDE}/unity.c)
    endif()

    add_test(NAME ${NAME} COMMAND ${NAME})
    message(STATUS "  Unit test: ${NAME} (saferam)")
endfunction()

# Manager needs every sub-module it schedules checks against.
safety_add_test(Safety_Manager_UnitTest test_saferam_manager.c ${SAFERAM_ALL_SOURCES})

# Stand-alone module tests (link only the module under test).
safety_add_test(Safety_Partition_UnitTest test_ram_partition.c
    ${SAFERAM_SRC_DIR}/ram_partition.c)
safety_add_test(Safety_Stack_UnitTest test_stack_protection.c
    ${SAFERAM_SRC_DIR}/stack_protection.c)
safety_add_test(Safety_Heap_UnitTest test_heap_protection.c
    ${SAFERAM_SRC_DIR}/heap_protection.c)
safety_add_test(Safety_Data_UnitTest test_safe_data.c
    ${SAFERAM_SRC_DIR}/safe_data.c)
safety_add_test(Safety_FaultInj_UnitTest test_fault_injection.c
    ${SAFERAM_SRC_DIR}/fault_injection.c)
