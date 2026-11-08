# src/autosar/adaptive/tests/CMakeLists.txt
# Adaptive exec / sm 平台服务宿主机单元测试 (P3 覆盖率提升)

cmake_minimum_required(VERSION 3.16)

add_executable(test_adaptive_exec
    test_adaptive_exec.c
)
target_link_libraries(test_adaptive_exec PRIVATE eth_adaptive_exec)
add_test(NAME adaptive_exec COMMAND test_adaptive_exec)

add_executable(test_adaptive_sm
    test_adaptive_sm.c
)
target_link_libraries(test_adaptive_sm PRIVATE eth_adaptive_sm)
add_test(NAME adaptive_sm COMMAND test_adaptive_sm)
