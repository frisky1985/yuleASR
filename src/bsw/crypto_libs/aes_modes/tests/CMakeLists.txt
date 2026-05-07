########################################################################################################################
# CMakeLists.txt - AES算法模式测试
########################################################################################################################

cmake_minimum_required(VERSION 3.10)

# 测试可执行文件
add_executable(test_aes_modes test_aes_modes.c)

# 链接AES模式库
target_link_libraries(test_aes_modes PRIVATE aes_modes)

# 设置头文件路径
target_include_directories(test_aes_modes PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include/autosar/classic/common
)

# 添加测试
add_test(NAME AES_Modes_Test COMMAND test_aes_modes)

# 性能测试 (可选)
if(AES_MODES_ENABLE_BENCHMARK)
    add_executable(benchmark_aes benchmark_aes.c)
    target_link_libraries(benchmark_aes PRIVATE aes_modes)
    target_include_directories(benchmark_aes PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include/autosar/classic/common
    )
    add_test(NAME AES_Benchmark COMMAND benchmark_aes)
endif()
