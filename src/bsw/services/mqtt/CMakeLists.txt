# MQTT 模块构建配置 - 集成mbedTLS TLS支持

#============================================================================
# 选项
#============================================================================
option(MQTT_SUPPORT_TLS "启用MQTT TLS支持" ON)
option(MQTT_MBEDTLS_FROM_SUBMODULE "使用子模块中的mbedTLS" ON)

#============================================================================
# 源文件
#============================================================================
set(MQTT_SOURCES
    src/Mqtt.c
    src/Mqtt_Lcfg.c
)

set(MQTT_HEADERS
    include/Mqtt.h
    include/Mqtt_Cfg.h
)

# TLS支持
if(MQTT_SUPPORT_TLS)
    list(APPEND MQTT_SOURCES
        src/Mqtt_Tls.c
        src/Mqtt_CertMgr.c
    )
    list(APPEND MQTT_HEADERS
        include/Mqtt_Tls.h
        include/Mqtt_CertMgr.h
    )
endif()

#============================================================================
# mbedTLS配置
#============================================================================
if(MQTT_SUPPORT_TLS)
    if(MQTT_MBEDTLS_FROM_SUBMODULE)
        # 使用项目中的mbedTLS子模块
        set(MBEDTLS_ROOT ${CMAKE_SOURCE_DIR}/third_party/mbedtls)
        set(YULE_MBEDTLS_ADAPTER_ROOT ${CMAKE_SOURCE_DIR}/third_party/yule-mbedtls-adapter)
        
        # 添加mbedTLS适配层
        add_subdirectory(${YULE_MBEDTLS_ADAPTER_ROOT} ${CMAKE_BINARY_DIR}/yule-mbedtls-adapter)
        
        # mbedTLS配置文件
        set(MBEDTLS_CONFIG_FILE "<config-yule-autosar.h>")
    else()
        # 查找系统mbedTLS
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(MBEDTLS mbedtls)
        endif()
        
        if(NOT MBEDTLS_FOUND)
            # 尝试查找系统mbedTLS
            find_path(MBEDTLS_INCLUDE_DIR mbedtls/ssl.h)
            find_library(MBEDTLS_LIBRARY mbedtls)
            find_library(MBEDX509_LIBRARY mbedx509)
            find_library(MBEDCRYPTO_LIBRARY mbedcrypto)
            
            if(MBEDTLS_INCLUDE_DIR AND MBEDTLS_LIBRARY)
                set(MBEDTLS_FOUND TRUE)
            endif()
        endif()
    endif()
endif()

#============================================================================
# 创建库
#============================================================================
add_library(mqtt STATIC ${MQTT_SOURCES})

#============================================================================
# 头文件路径
#============================================================================
target_include_directories(mqtt PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/bsw/common
    ${CMAKE_SOURCE_DIR}/src/bsw/services/com/include
)

if(MQTT_SUPPORT_TLS)
    target_include_directories(mqtt PUBLIC
        ${YULE_MBEDTLS_ADAPTER_ROOT}/include
    )
    
    if(MQTT_MBEDTLS_FROM_SUBMODULE)
        target_include_directories(mqtt PUBLIC
            ${MBEDTLS_ROOT}/include
        )
    else()
        target_include_directories(mqtt PUBLIC
            ${MBEDTLS_INCLUDE_DIRS}
        )
    endif()
endif()

#============================================================================
# 编译定义
#============================================================================
if(MQTT_SUPPORT_TLS)
    target_compile_definitions(mqtt PUBLIC
        MQTT_SUPPORT_TLS=STD_ON
        MBEDTLS_CONFIG_FILE=${MBEDTLS_CONFIG_FILE}
    )
else()
    target_compile_definitions(mqtt PUBLIC
        MQTT_SUPPORT_TLS=STD_OFF
    )
endif()

#============================================================================
# 编译选项
#============================================================================
target_compile_options(mqtt PRIVATE
    -Wall
    -Wextra
    -Werror=implicit-function-declaration
    -O2
    -ffunction-sections
    -fdata-sections
)

if(MQTT_SUPPORT_TLS)
    target_compile_options(mqtt PRIVATE
        -Os                      # TLS代码优化代码大小
    )
endif()

#============================================================================
# 链接依赖
#============================================================================
if(MQTT_SUPPORT_TLS)
    if(MQTT_MBEDTLS_FROM_SUBMODULE)
        target_link_libraries(mqtt PUBLIC
            yule-mbedtls-adapter
        )
    else()
        if(MBEDTLS_FOUND)
            target_link_libraries(mqtt PUBLIC
                ${MBEDTLS_LIBRARIES}
            )
        else()
            target_link_libraries(mqtt PUBLIC
                ${MBEDTLS_LIBRARY}
                ${MBEDX509_LIBRARY}
                ${MBEDCRYPTO_LIBRARY}
            )
        endif()
    endif()
endif()

#============================================================================
# 链接选项
#============================================================================
target_link_options(mqtt PUBLIC
    -Wl,--gc-sections
)

#============================================================================
# 安装目标
#============================================================================
install(TARGETS mqtt
    ARCHIVE DESTINATION lib
)

install(FILES ${MQTT_HEADERS}
    DESTINATION include/mqtt
)

#============================================================================
# 测试
#============================================================================
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
