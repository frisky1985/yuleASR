# EthSM Unit Tests CMakeLists.txt
#
# P0-A 双网络回归测试: 编译真实生产源码 EthSM.c, mock EthIf/ComM/Det,
# 验证 trcvIdx/ctrlIdx 按网络正确传递 (08-07 MISRA 机械修复损坏的赋值回归)。
#
# Use: cmake -DBUILD_TESTING=ON .. && make EthSM_UnitTest && ctest -R EthSM

cmake_minimum_required(VERSION 3.16)

set(ETHSM_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_ethsm.c)
set(ETHSM_PRODUCT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/ethSm/src/EthSM.c)

add_executable(EthSM_UnitTest
    ${ETHSM_TEST_SRC}
    ${ETHSM_PRODUCT_SRC}
)

target_include_directories(EthSM_UnitTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/..                       # test_framework.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/ethSm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/ethif/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/comM/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/det/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/os/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/general/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/rte/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../config/input/ecual
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../config/input/services
)

set_target_properties(EthSM_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(EthSM_UnitTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME EthSM_UnitTest COMMAND EthSM_UnitTest)

message(STATUS "  Unit test: EthSM_UnitTest (双网络 trcvIdx/ctrlIdx 回归)")
