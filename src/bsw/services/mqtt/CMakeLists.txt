# MQTT 模块构建配置

# 源文件
set(MQTT_SOURCES
    src/Mqtt.c
    src/Mqtt_Lcfg.c
)

# 头文件路径
set(MQTT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 创建静态库
add_library(mqtt STATIC ${MQTT_SOURCES})

# 设置包含目录
target_include_directories(mqtt PUBLIC
    ${MQTT_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src/bsw/common
    ${CMAKE_SOURCE_DIR}/src/bsw/services/com/include
)

# 设置编译选项
target_compile_options(mqtt PRIVATE
    -Wall
    -Wextra
    -O2
)

# 安装目标
install(TARGETS mqtt
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include/mqtt
)
