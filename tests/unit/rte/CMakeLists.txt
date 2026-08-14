# RTE Unit Tests CMakeLists.txt
#
# This file adds RTE unit tests to the build.
# Use: cmake -DBUILD_TESTING=ON .. && make

cmake_minimum_required(VERSION 3.16)

# Collect RTE unit test sources
file(GLOB RTE_UNIT_TEST_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/test_rte_cs_operations.c
)

# Create test executable for each test file
foreach(test_src ${RTE_UNIT_TEST_SRCS})
    get_filename_component(test_name ${test_src} NAME_WE)
    set(TEST_TARGET "Rte_${test_name}_UnitTest")

    add_executable(${TEST_TARGET}
        ${test_src}
    )

    target_include_directories(${TEST_TARGET}
        PRIVATE
            ${TEST_FRAMEWORK_INCLUDE}
            ${CMAKE_CURRENT_SOURCE_DIR}/../../src/middleware/rte/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/common
            ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/general/inc
            ${CMAKE_CURRENT_SOURCE_DIR}/..
    )

    # Link with Rte library
    if(TARGET Rte)
        target_link_libraries(${TEST_TARGET} PRIVATE Rte)
    endif()

    if(TEST_FRAMEWORK_LIB)
        target_link_libraries(${TEST_TARGET} PRIVATE ${TEST_FRAMEWORK_LIB})
    endif()

    add_test(NAME ${TEST_TARGET} COMMAND ${TEST_TARGET})

    set_target_properties(${TEST_TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    message(STATUS "  Unit test: ${TEST_TARGET}")
endforeach()
