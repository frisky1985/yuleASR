# src/ethernet/tests/CMakeLists.txt
# ethernet 驱动单元测试 — eth_manager + 驱动寄存器模拟 (B3.2)

cmake_minimum_required(VERSION 3.16)

add_executable(test_eth_manager
    test_eth_manager.c
)

target_include_directories(test_eth_manager PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../driver
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/types
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/log
)

target_link_libraries(test_eth_manager PRIVATE eth_driver)

add_test(NAME eth_manager COMMAND test_eth_manager)
