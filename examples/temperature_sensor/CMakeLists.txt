# Temperature Sensor Example CMakeLists.txt

# 温度传感器发布者可执行文件
add_executable(temp_sensor_publisher
    temp_sensor_publisher.c
)

target_include_directories(temp_sensor_publisher PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/common/types
    ${CMAKE_SOURCE_DIR}/src/common/utils
)

target_link_libraries(temp_sensor_publisher
    eth_dds
    m  # 数学库
)

# 温度传感器订阅者可执行文件
add_executable(temp_sensor_subscriber
    temp_sensor_subscriber.c
)

target_include_directories(temp_sensor_subscriber PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/common/types
    ${CMAKE_SOURCE_DIR}/src/common/utils
)

target_link_libraries(temp_sensor_subscriber
    eth_dds
    m  # 数学库
)

# 安装目标
install(TARGETS temp_sensor_publisher temp_sensor_subscriber
    RUNTIME DESTINATION bin/examples
)

# 如果启用测试，添加测试目标
if(BUILD_TESTS)
    add_test(NAME temp_sensor_publisher_help 
        COMMAND temp_sensor_publisher --help)
    add_test(NAME temp_sensor_subscriber_help
        COMMAND temp_sensor_subscriber --help)
endif()

message(STATUS "Temperature sensor example configured:")
message(STATUS "  - temp_sensor_publisher")
message(STATUS "  - temp_sensor_subscriber")
