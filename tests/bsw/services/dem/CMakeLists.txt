# Dem unit tests — substantiated against the production Dem implementation.
# Links the real service_dem library; the only external dependency
# (Det_ReportError) is provided by the test file itself, so no mocks are linked.

cmake_minimum_required(VERSION 3.16)

set(DEM_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_dem.c)

add_executable(dem_test ${DEM_TEST_SRC})

target_include_directories(dem_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/dem/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
)

target_compile_options(dem_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(dem_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_dem)
    target_link_libraries(dem_test PRIVATE service_dem)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(dem_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME dem_test COMMAND dem_test)

message(STATUS "  Unit test: dem_test (Diagnostic Event Manager)")
