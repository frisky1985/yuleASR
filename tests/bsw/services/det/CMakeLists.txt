# Det unit tests — substantiated against the production Det implementation
# Links the real service_det library with minimal dependency stubs.

cmake_minimum_required(VERSION 3.16)

set(DET_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_det.c)

add_executable(det_test
    ${DET_TEST_SRC}
)

target_include_directories(det_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(det_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(det_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_det)
    target_link_libraries(det_test PRIVATE service_det)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(det_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME det_test COMMAND det_test)

message(STATUS "  Unit test: det_test (Development Error Tracer)")
