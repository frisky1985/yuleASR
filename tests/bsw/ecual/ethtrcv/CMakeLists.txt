# EthTrcv unit tests — substantiated against the production EthTrcv driver
# (src/bsw/ecual/ethtrcv). Links the real ecual_ethtrcv library; the external
# dependencies (Det_ReportError, Eth_ReadMii/Eth_WriteMii and the Lcfg
# configuration symbols) are provided by the test source itself as host-safe
# mocks/stubs.

cmake_minimum_required(VERSION 3.16)

add_executable(ethtrcv_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_ethtrcv.c
)

target_include_directories(ethtrcv_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/ethtrcv/include
        ${CMAKE_SOURCE_DIR}/src/bsw/mcal/eth/include
        ${CMAKE_SOURCE_DIR}/src/bsw/mcal/spi/include
        ${CMAKE_SOURCE_DIR}/src/bsw/mcal/i2c/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/dem/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/ecual
)

target_compile_options(ethtrcv_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(ethtrcv_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET ecual_ethtrcv)
    target_link_libraries(ethtrcv_test PRIVATE ecual_ethtrcv)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(ethtrcv_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME ethtrcv_test COMMAND ethtrcv_test)

message(STATUS "  Unit test: ethtrcv_test (Ethernet Transceiver Driver)")
