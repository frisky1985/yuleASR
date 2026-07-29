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

message(STATUS "Tests configured")
