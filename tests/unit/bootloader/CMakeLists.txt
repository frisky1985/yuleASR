# bl_partition 默认分区表边界测试 (Flash 容量 / 扇区几何)

cmake_minimum_required(VERSION 3.16)

add_executable(bl_partition_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_bl_partition.c
    ${CMAKE_SOURCE_DIR}/src/bootloader/bl_partition.c
)

target_include_directories(bl_partition_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bootloader
        ${CMAKE_SOURCE_DIR}/src/common/log
)

target_compile_options(bl_partition_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(bl_partition_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(bl_partition_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME bl_partition_test COMMAND bl_partition_test)

message(STATUS "  Unit test: bl_partition_test (Bootloader partition layout)")
