# src/eth_sm/tests/CMakeLists.txt
# eth_sm 单元测试 — 状态机 UNINIT→INIT→READY (B3.1)

cmake_minimum_required(VERSION 3.16)

add_executable(test_eth_sm
    test_eth_sm.c
)

target_include_directories(test_eth_sm PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/eth_sm
)

target_link_libraries(test_eth_sm PRIVATE eth_sm)

add_test(NAME eth_sm COMMAND test_eth_sm)
