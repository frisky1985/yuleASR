# SoAd (Socket Adapter) unit tests — substantiated against the production
# SoAd.c implementation.  The real SoAd source is compiled directly into the
# test binary; TcpIp / Det / PduR / Posix socket interactions are captured by
# stubs in stubs.c so the tests run on the host without lwIP or a real stack.

cmake_minimum_required(VERSION 3.16)

add_executable(test_soad
    ${CMAKE_SOURCE_DIR}/src/bsw/services/soad/src/SoAd.c
    stubs.c
    test_soad.c
)

target_include_directories(test_soad
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/soad/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/tcpip/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(test_soad PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-unused-function
    -g
    -O0
)

set_target_properties(test_soad PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(test_soad PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME test_soad COMMAND test_soad)

message(STATUS "  Unit test: test_soad (Socket Adapter)")
