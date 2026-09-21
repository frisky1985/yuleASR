# SecOC unit tests — substantiated against the production SecOC implementation
# Links the real service_secoc library with minimal dependency stubs.

cmake_minimum_required(VERSION 3.16)

set(SECOC_TEST_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/test_secoc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs.c
)

add_executable(secoc_test
    ${SECOC_TEST_SRC}
)

target_include_directories(secoc_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/secoc/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(secoc_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(secoc_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_secoc)
    target_link_libraries(secoc_test PRIVATE service_secoc)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(secoc_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME secoc_test COMMAND secoc_test)

message(STATUS "  Unit test: secoc_test (Security Onboard Communication)")
