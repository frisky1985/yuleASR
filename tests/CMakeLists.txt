# Tests CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

# Find test framework (Unity)
find_package(Unity QUIET)

if(NOT UNITY_FOUND)
    # Use bundled Unity or simple test framework
    message(STATUS "Unity not found, using simple test framework")
    set(TEST_FRAMEWORK_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/framework)
    set(TEST_FRAMEWORK_LIB)
else()
    set(TEST_FRAMEWORK_INCLUDE ${UNITY_INCLUDE_DIRS})
    set(TEST_FRAMEWORK_LIB Unity)
endif()

# Enable testing
enable_testing()

# Unit tests
add_subdirectory(unit)

# Integration tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration)
    add_subdirectory(integration)
endif()

# MCAL mock tests (links real production source with MockHAL)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/mock/CMakeLists_MCAL_Tests.txt)
    include(${CMAKE_CURRENT_SOURCE_DIR}/mock/CMakeLists_MCAL_Tests.txt)
endif()

# ── S1: S0 smoke test — native runtime verification (FreeRTOS Posix port) ──
# Only on native (non-cross) builds: the Posix port runs on Linux/macOS compile
# but is only *supported* at runtime on Linux (see sprint-contract-next.md).
if(NOT CMAKE_CROSSCOMPILING)
    add_executable(s0_smoke_test
        ${CMAKE_CURRENT_SOURCE_DIR}/s0_smoke_test.c
    )

    target_link_libraries(s0_smoke_test
        PRIVATE
            Os
            Rte
            service_det
    )

    set_target_properties(s0_smoke_test PROPERTIES
        C_STANDARD 99
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    add_test(NAME s0_smoke COMMAND s0_smoke_test)
endif()

message(STATUS "Tests configured")
