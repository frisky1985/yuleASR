# Examples CMakeLists.txt
# ETH-DDS Integration Example Applications

# 基本发布订阅示例
add_executable(basic_pubsub
    basic_pubsub.c
)

target_include_directories(basic_pubsub PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/common/types
    ${CMAKE_SOURCE_DIR}/src/common/utils
)

target_link_libraries(basic_pubsub
    eth_dds
)

# 温度传感器示例
add_subdirectory(temperature_sensor)

# 安装目标
install(TARGETS basic_pubsub
    RUNTIME DESTINATION bin/examples
)

message(STATUS "Examples configured:")
message(STATUS "  - basic_pubsub")
message(STATUS "  - temperature_sensor/*")
