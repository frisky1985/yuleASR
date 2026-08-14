# 测试构建配置

# Unity源文件
set(UNITY_SOURCES
    unity/unity.c
)

# 测试源文件
set(TEST_SOURCES
    unity/test_domain.c
    unity/test_topic.c
    unity/test_publisher.c
    unity/test_subscriber.c
    unity/test_writer.c
    unity/test_reader.c
    unity/test_qos.c
    unity/test_buffer_pool.c
    unity/test_cdr.c
)

# 创建测试可执行文件
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name}
        ${test_source}
        ${UNITY_SOURCES}
        ../src/core/domain.c
        ../src/core/topic.c
        ../src/core/publisher.c
        ../src/core/subscriber.c
        ../src/core/writer.c
        ../src/core/reader.c
        ../src/qos/qos_policy.c
        ../src/utils/buffer_pool.c
        ../src/serialization/cdr.c
    )
    
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/unity
    )
    
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
